#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <fcntl.h>      // file controls e.g. O_RDWR
#include <unistd.h>     // write(), read(), close()
#include <errno.h>      // integer errors and strerror()
#include <termios.h>    // struct termios, POSIX terminal control definitions

#include <mysql.h>
#define DB_Nrecords 50

int fd_serialport;
struct termios *poldtty;
MYSQL *con;

void display_termios(struct termios* tty)
{
    printf("\nc_iflag=%07d,\nc_oflag=%07d,\nc_cflag=%07d,\nc_lflag=%07d,\n\n",tty->c_iflag,\
           tty->c_oflag, tty->c_cflag, tty->c_lflag);
    printf("c_cc[VINTR]=%3d\n",tty->c_cc[VINTR]);
    printf("c_cc[VQUIT]=%3d\n",tty->c_cc[VQUIT]);
    printf("c_cc[VERASE]=%3d\n",tty->c_cc[VERASE]);
    printf("c_cc[VKILL]=%3d\n",tty->c_cc[VKILL]);
    printf("c_cc[VEOF]=%3d\n",tty->c_cc[VEOF]);
    printf("c_cc[VTIME]=%3d\n",tty->c_cc[VTIME]);
    printf("c_cc[VMIN]=%3d\n",tty->c_cc[VMIN]);
    printf("c_cc[VSWTC]=%3d\n",tty->c_cc[VSWTC]);
    printf("c_cc[VSTART]=%3d\n",tty->c_cc[VSTART]);
    printf("c_cc[VSTOP]=%3d\n",tty->c_cc[VSTOP]);
    printf("c_cc[VSUSP]=%3d\n",tty->c_cc[VSUSP]);
    printf("c_cc[VEOL]=%3d\n",tty->c_cc[VEOL]);
    printf("c_cc[VREPRINT]=%3d\n",tty->c_cc[VREPRINT]);
    printf("c_cc[VDISCARD]=%3d\n",tty->c_cc[VDISCARD]);
    printf("c_cc[VWERASE]=%3d\n",tty->c_cc[VWERASE]);
    printf("c_cc[VLNEXT]=%3d\n",tty->c_cc[VLNEXT]);
    printf("c_cc[VEOL2]=%3d\n\n",tty->c_cc[VEOL2]);

    return;
}

void save_termios(struct termios* tty)
{
    FILE *fp;
    fp=fopen("saved_termios.dat","wb");

    fwrite(tty, 1, sizeof(struct termios), fp);
    if(!ferror(fp))
        puts("termios struct saved");

    fclose(fp);
    return;
}
void load_termios(struct termios* tty)
{
    FILE *fp;
    fp=fopen("saved_termios.dat","rb");

    fread(tty, 1, sizeof(struct termios), fp);
    if(!ferror(fp))
        puts("termios struct loaded");

    fclose(fp);
    return;
}

void sigtstp_ISR(int signo)
{
    puts("\nreceived SIGTSTP, ending mysql & serial port connections...");
    if(tcsetattr(fd_serialport,TCSANOW,poldtty))
        fprintf(stderr,"on program closing, couldn't tcsetattr: error%d: %s\n",errno,strerror(errno));
    close(fd_serialport);
    mysql_close(con);
    mysql_library_end();

    exit(0);
}

int main()
{
    short int num_bytes, LRC;    //LRC - longitudinal checksum <=> checksum8 2's complement
    char buf[128], qry[128], byte, sbyte[3];
    int value;


    //{//connect to DB
    if(mysql_library_init(0,NULL,NULL))
    {
        fprintf(stderr, "couldn't initialize mysql client library\n");
        exit(2);
    }
    con = mysql_init(NULL);
    if(!mysql_real_connect(con,"localhost","metricinserter","EMmetrics123",\
        "EnergyMeter_metrics",0,NULL,0))
    {
        fprintf(stderr, "couldn't connect: error:%s",mysql_error(con));
        exit(3);
    }//}
    //{//connect to usbserial device, save config, set config, etc.
    fd_serialport = open("/dev/ttyUSB0", O_RDWR);
    if(fd_serialport < 0)
    {
        fprintf(stderr,"Error %d from open: %s\n", errno, strerror(errno));
        exit(4);
    }

    struct termios oldtty, newtty;
    poldtty = &oldtty;
    if(tcgetattr(fd_serialport, &oldtty))
    {
        fprintf(stderr,"Error %d from tcgetattr: %s\n", errno, strerror(errno));
        exit(5);
    }

    //load_termios(&oldtty);
    display_termios(&oldtty);
    //save_termios(&oldtty);

    memset(&newtty,0,sizeof(struct termios));
    newtty.c_iflag |= 0;
    newtty.c_oflag |= 0;
    newtty.c_cflag |= CS8|CREAD|CLOCAL;
    newtty.c_lflag |= ICANON;
    newtty.c_cc[VTIME] = 1;
    newtty.c_cc[VEOF] = 4;
    cfsetispeed(&newtty, B9600);
    cfsetospeed(&newtty, B9600);
    if(tcsetattr(fd_serialport, TCSANOW, &newtty))
    {
        fprintf(stderr,"Error %d from tcsetattr: %s", errno, strerror(errno));
        exit(6);
    }//}

    //{//initialize the table with DB_Nrecords
    if(mysql_query(con, "DELETE FROM monophase1_V"))
    {
        fprintf(stderr, "couldn't query: error: %s", mysql_error(con));
        exit(4);
    }
    for(value=0; value<DB_Nrecords; value++)
    {
        if(mysql_query(con, "INSERT INTO monophase1_V (value) VALUES (0)"))
        {
            fprintf(stderr, "couldn't query: error: %s", mysql_error(con));
            exit(4);
        }
    }
    if(mysql_query(con, "UPDATE lastinsertids SET lid=LAST_INSERT_ID() WHERE tablename='monophase1_V'"))
    {
        fprintf(stderr, "couldn't query: error: %s", mysql_error(con));
        exit(4);
    }//}

    puts("Inserting data from usbserial into DB,\n"
         "end the program with SIGTSTP (Ctrl+Z on Linux).");
    signal(SIGTSTP, sigtstp_ISR);    ///end the program with SIGTSTP (Ctrl+Z on Linux)
    while(1)
    {
        num_bytes=read(fd_serialport, buf, 127);
        if(num_bytes<0)
        {
            fprintf(stderr,"Error %d from read: %s", errno, strerror(errno));
            exit(7);
        }
        if(num_bytes!=19)
        {
            fprintf(stderr,"num_bytes=%hd, different than the expected 19!\n",num_bytes);
            continue;
        }

        buf[num_bytes]='\0';
        if(strncmp(buf,":01",3))
            continue;
        //strncpy(sbyte,buf+1,2);
        //sbyte[2]='\0';
        sscanf(buf+1,"%2hhx",&byte);
        LRC = -byte;

        if(strncmp(buf+3,"03",2))
            continue;
        sscanf(buf+3,"%2hhx",&byte);
        LRC = LRC - byte;

        if(strncmp(buf+5,"04",2))
            continue;
        sscanf(buf+5,"%2hhx",&byte);
        LRC -= byte;

        sscanf(buf+7,"%2hhx",&byte);
        LRC -= byte;
        sscanf(buf+9,"%2hhx",&byte);
        LRC -= byte;
        sscanf(buf+11,"%2hhx",&byte);
        LRC -= byte;
        sscanf(buf+13,"%2hhx",&byte);
        LRC -= byte;

        sscanf(buf+15,"%2hhx",&byte);
        if(byte!=(char)LRC)        //compare computed LRC with response LRC
        {
            puts("checksum failed");
            continue;
        }

        sscanf(buf+7,"%8x",&value);
        sprintf(qry,"CALL insert1_V(%d)",value);
        if(mysql_query(con,qry))
        {
            fprintf(stderr, "couldn't query: error: %s", mysql_error(con));
            exit(8);
        }

        printf("byte=%#X\n",byte);
        printf("value=%#X\n",value);
        printf("LRC=%#X\n",LRC);
        printf("Read: %s",buf);
    }


    return 1;
}
