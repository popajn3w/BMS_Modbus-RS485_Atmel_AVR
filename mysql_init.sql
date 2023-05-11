

CREATE USER 'testuser1'@'localhost' IDENTIFIED BY 'maisuchili';
GRANT ALL ON *.* TO 'testuser1'@'localhost';

CREATE USER 'testuser2'@'192.168.0.101' IDENTIFIED BY 'maisuchili';
GRANT ALL ON *.* TO 'testuser2'@'192.168.0.101';

CREATE USER 'metricinserter'@'localhost' IDENTIFIED BY 'EMmetrics123';
GRANT SELECT, UPDATE, INSERT, DELETE, EXECUTE ON EnergyMeter_metrics.* TO 'metricinserter'@'localhost';

CREATE USER 'grafanauser'@'localhost' IDENTIFIED BY 'NiceGraphs-1234';
GRANT SELECT ON EnergyMeter_metrics.* TO 'grafanauser'@'localhost';


CREATE DATABASE EnergyMeter_metrics;
USE EnergyMeter_metrics;

CREATE TABLE monophase1_V(
	id SMALLINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
	value DECIMAL(7,2) NOT NULL,
	time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE lastinsertids(
	id TINYINT PRIMARY KEY AUTO_INCREMENT,
	tablename VARCHAR(255) NOT NULL UNIQUE,
	lid SMALLINT NOT NULL
);


/* nu merge
DELIMITER //
CREATE TRIGGER limitDBrecords BEFORE INSERT ON monophase1_V FOR EACH ROW
BEGIN
	DECLARE N SMALLINT;
	SELECT COUNT(*) FROM monophase1_V INTO N;
	IF (N>=10) THEN
		DELETE FROM monophase1_V WHERE id=new.id-N;
	END IF;
END//
DELIMITER ;
*/

/* nici asa nu merge, nu pot fi modificate decat valorile old, new din randul afectat de
   actiunea care a invocat trigger ul
DELIMITER //
CREATE TRIGGER limitDBrecords AFTER INSERT ON monophase1_V FOR EACH ROW
BEGIN
	DECLARE N SMALLINT;
	SELECT COUNT(*) FROM monophase1_V INTO N;
	IF (N>=11) THEN
		DELETE FROM monophase1_V WHERE id=new.id-N+1;
	END IF;
END//
DELIMITER ;
*/

/* nu se poate scala din cauza LAST_INSERT_ID()
DELIMITER //
CREATE PROCEDURE insert1_V(IN val INT)
BEGIN
	DECLARE N SMALLINT;
	SELECT COUNT(*) FROM monophase1_V INTO N;
	IF(N>=10) THEN
	DELETE FROM monophase1_V WHERE id = LAST_INSERT_ID() - N + 1;
	END IF;
	INSERT INTO monophase1_V (value) VALUES (val);
END//
DELIMITER ;
*/

DELIMITER //
CREATE PROCEDURE insert1_V(IN val DECIMAL(9,2))
BEGIN
	DECLARE lastid SMALLINT;
	SELECT lid FROM lastinsertids WHERE tablename = 'monophase1_V' INTO lastid;
	DELETE FROM monophase1_V WHERE id = lastid-50+1;
	INSERT INTO monophase1_V (value) VALUES (val/100);
	UPDATE lastinsertids SET lid=LAST_INSERT_ID() WHERE tablename = 'monophase1_V';
END//
DELIMITER ;

/*
MySQL:
root			maisuchili
testuser1		maisuchili	- teste de pe Linux
testuser2		maisuchili	- teste de pe Windows
metricinserter		EMmetrics123	- insereaza date in functie de raspunsurile contoarelor
grafanauser		NiceGraphs-1234 - folosit de Grafana
Grafana:

root		givemegraphs
webuser		lookatthosegraphs
*/