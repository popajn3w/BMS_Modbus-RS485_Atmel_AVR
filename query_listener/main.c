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
    short int num_bytes;
    char qry[128];


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
    save_termios(&oldtty);

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


    //{//initialize the tables with DB_Nrecords
    if(mysql_query(con, "DELETE FROM monophase1"))
    {
        fprintf(stderr, "couldn't query: error: %s", mysql_error(con));
        exit(4);
    }
    if(mysql_query(con, "DELETE FROM triphase2"))
    {
        fprintf(stderr, "couldn't query: error: %s", mysql_error(con));
        exit(4);
    }
    if(mysql_query(con, "DELETE FROM triphase3"))
    {
        fprintf(stderr, "couldn't query: error: %s", mysql_error(con));
        exit(4);
    }
    if(mysql_query(con, "DELETE FROM triphase4"))
    {
        fprintf(stderr, "couldn't query: error: %s", mysql_error(con));
        exit(4);
    }


    for(num_bytes=0; num_bytes<DB_Nrecords; num_bytes++)
    {
        if(mysql_query(con, "INSERT INTO monophase1 (current,Pactive,Preactive,Papparent,voltage) VALUES (0,0,0,0,0)"))
        {
            fprintf(stderr, "couldn't query: error: %s", mysql_error(con));
            exit(4);
        }
    }
    for(num_bytes=0; num_bytes<DB_Nrecords; num_bytes++)
    {
        if(mysql_query(con, "INSERT INTO triphase2 (current1,current2,current3,Pactive1,Pactive2,Pactive3,"
                       "Preactive1,Preactive2,Preactive3,Papparent1,Papparent2,Papparent3) "
                       "VALUES (0,0,0,0,0,0,0,0,0,0,0,0)"))
        {
            fprintf(stderr, "couldn't query: error: %s", mysql_error(con));
            exit(4);
        }
    }
    for(num_bytes=0; num_bytes<DB_Nrecords; num_bytes++)
    {
        if(mysql_query(con, "INSERT INTO triphase3 (current1,current2,current3,Pactive1,Pactive2,Pactive3,"
                       "Preactive1,Preactive2,Preactive3,Papparent1,Papparent2,Papparent3) "
                       "VALUES (0,0,0,0,0,0,0,0,0,0,0,0)"))
        {
            fprintf(stderr, "couldn't query: error: %s", mysql_error(con));
            exit(4);
        }
    }
    for(num_bytes=0; num_bytes<DB_Nrecords; num_bytes++)
    {
        if(mysql_query(con, "INSERT INTO triphase4 (current1,current2,current3,Pactive1,Pactive2,Pactive3,"
                       "Preactive1,Preactive2,Preactive3,Papparent1,Papparent2,Papparent3) "
                       "VALUES (0,0,0,0,0,0,0,0,0,0,0,0)"))
        {
            fprintf(stderr, "couldn't query: error: %s", mysql_error(con));
            exit(4);
        }
    }//}

    puts("Inserting data from usbserial into DB,\n"
         "end the program with SIGTSTP (Ctrl+Z on Linux).");
    signal(SIGTSTP, sigtstp_ISR);    ///end the program with SIGTSTP (Ctrl+Z on Linux)
    while(1)
    {
        num_bytes=read(fd_serialport, qry, 127);
        if(num_bytes<0)
        {
            fprintf(stderr,"Error %d from read: %s", errno, strerror(errno));
            exit(7);
        }
        qry[num_bytes]='\0';

        if(mysql_query(con,qry))
        {
            fprintf(stderr, "couldn't query: error: %s", mysql_error(con));
            //exit(8);
        }

        printf("Read: %s",qry);
    }




    /*if(mysql_query(con, "CALL insert1_V(22062)"))
    {
        fprintf(stderr, "couldn't query: error: %s", mysql_error(con));
        exit(4);
    }*/


    return 1;
}
