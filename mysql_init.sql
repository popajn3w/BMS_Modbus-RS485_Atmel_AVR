DROP DATABASE IF EXISTS EnergyMeter_metrics;
CREATE DATABASE EnergyMeter_metrics;
USE EnergyMeter_metrics;

CREATE USER 'testuser1'@'localhost' IDENTIFIED BY 'maisuchili';
GRANT ALL ON *.* TO 'testuser1'@'localhost';

CREATE USER 'testuser2'@'192.168.0.101' IDENTIFIED BY 'maisuchili';
GRANT ALL ON *.* TO 'testuser2'@'192.168.0.101';

CREATE USER 'metricinserter'@'localhost' IDENTIFIED BY 'EMmetrics123';
GRANT SELECT, UPDATE, INSERT, DELETE, EXECUTE ON EnergyMeter_metrics.* TO 'metricinserter'@'localhost';

CREATE USER 'grafanauser'@'localhost' IDENTIFIED BY 'NiceGraphs-1234';
GRANT SELECT ON EnergyMeter_metrics.* TO 'grafanauser'@'localhost';


CREATE TABLE monophase1(
	id SMALLINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
	current DECIMAL(10,3) NOT NULL,
	Pactive DECIMAL(10,2) NOT NULL,
	Preactive DECIMAL(10,2) NOT NULL,
	Papparent DECIMAL(10,2) NOT NULL,
	voltage DECIMAL(10,2),
	time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE lastinsertids(
	id TINYINT PRIMARY KEY AUTO_INCREMENT,
	tablename VARCHAR(255) NOT NULL UNIQUE,
	lid SMALLINT NOT NULL
);


DELIMITER //
CREATE PROCEDURE insert_em1(IN I DECIMAL(13,3), IN Pa DECIMAL(12,2), IN Q DECIMAL(12,2), IN V DECIMAL(12,2))
BEGIN
	DECLARE lastid SMALLINT;
	SELECT lid FROM lastinsertids WHERE tablename = 'monophase1' INTO lastid;
	DELETE FROM monophase1 WHERE id = lastid-50+1;
	INSERT INTO monophase1 (current,Pactive,Preactive,Papparent,voltage) VALUES
	(I/1000, Pa/100, Q/100, SQRT(POW(Pa/100,2)+POW(Q/100,2)), V/100);
	UPDATE lastinsertids SET lid=LAST_INSERT_ID() WHERE tablename = 'monophase1';
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
