-- This file is generated automatically, do not edit, change the source (../../../src/stats-db2.tpl) instead.

BEGIN TRANSACTION;

-- Data for table `PROPULSION`, extracted from ../stats/propulsion.txt

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'ZNULLPROP'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	0,
	0,
	0,
	0,
	0,
	'MIBNKDRL.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='ZNULLPROP';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	0,
	'Wheeled'
FROM `BASE`
WHERE `BASE`.`pName`='ZNULLPROP';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'wheeled03'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	150,
	50,
	200,
	300,
	0,
	'PRLRWHL1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='wheeled03';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	175,
	'Wheeled'
FROM `BASE`
WHERE `BASE`.`pName`='wheeled03';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'wheeled02'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	100,
	50,
	250,
	200,
	0,
	'PRLRWHL1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='wheeled02';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	175,
	'Wheeled'
FROM `BASE`
WHERE `BASE`.`pName`='wheeled02';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'wheeled01'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	50,
	50,
	300,
	100,
	1,
	'PRLRWHL1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='wheeled01';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	175,
	'Wheeled'
FROM `BASE`
WHERE `BASE`.`pName`='wheeled01';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'V-Tol03'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	300,
	125,
	50,
	300,
	0,
	'DPVTOL.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='V-Tol03';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	800,
	'Lift'
FROM `BASE`
WHERE `BASE`.`pName`='V-Tol03';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'V-Tol02'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	250,
	125,
	50,
	150,
	0,
	'DPVTOL.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='V-Tol02';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	800,
	'Lift'
FROM `BASE`
WHERE `BASE`.`pName`='V-Tol02';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'V-Tol'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	150,
	125,
	50,
	100,
	1,
	'DPVTOL.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='V-Tol';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	700,
	'Lift'
FROM `BASE`
WHERE `BASE`.`pName`='V-Tol';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'tracked03'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	275,
	125,
	550,
	800,
	0,
	'PRLRTRK1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='tracked03';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	125,
	'Tracked'
FROM `BASE`
WHERE `BASE`.`pName`='tracked03';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'tracked02'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	200,
	125,
	600,
	600,
	0,
	'PRLRTRK1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='tracked02';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	125,
	'Tracked'
FROM `BASE`
WHERE `BASE`.`pName`='tracked02';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'tracked01'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	125,
	125,
	650,
	400,
	1,
	'PRLRTRK1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='tracked01';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	125,
	'Tracked'
FROM `BASE`
WHERE `BASE`.`pName`='tracked01';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'hover03'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	200,
	100,
	100,
	300,
	0,
	'PRLHOV1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='hover03';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	200,
	'Hover'
FROM `BASE`
WHERE `BASE`.`pName`='hover03';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'hover02'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	150,
	100,
	150,
	200,
	0,
	'PRLHOV1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='hover02';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	225,
	'Hover'
FROM `BASE`
WHERE `BASE`.`pName`='hover02';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'hover01'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	100,
	100,
	200,
	150,
	1,
	'PRLHOV1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='hover01';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	200,
	'Hover'
FROM `BASE`
WHERE `BASE`.`pName`='hover01';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'HalfTrack03'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	125,
	75,
	300,
	500,
	0,
	'PRLRHTR1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='HalfTrack03';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	150,
	'Half-Tracked'
FROM `BASE`
WHERE `BASE`.`pName`='HalfTrack03';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'HalfTrack02'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	100,
	75,
	350,
	350,
	0,
	'PRLRHTR1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='HalfTrack02';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	150,
	'Half-Tracked'
FROM `BASE`
WHERE `BASE`.`pName`='HalfTrack02';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'HalfTrack'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	75,
	75,
	400,
	200,
	1,
	'PRLRHTR1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='HalfTrack';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	150,
	'Half-Tracked'
FROM `BASE`
WHERE `BASE`.`pName`='HalfTrack';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'CyborgLegs03'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	10,
	50,
	100,
	150,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='CyborgLegs03';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	400,
	'Legged'
FROM `BASE`
WHERE `BASE`.`pName`='CyborgLegs03';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'CyborgLegs02'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	10,
	50,
	100,
	100,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='CyborgLegs02';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	400,
	'Legged'
FROM `BASE`
WHERE `BASE`.`pName`='CyborgLegs02';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'CyborgLegs'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	10,
	50,
	100,
	50,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='CyborgLegs';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	400,
	'Legged'
FROM `BASE`
WHERE `BASE`.`pName`='CyborgLegs';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'BaBaProp'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	0,
	15,
	10,
	1,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BaBaProp';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	200,
	'Wheeled'
FROM `BASE`
WHERE `BASE`.`pName`='BaBaProp';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'BaBaLegs'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	0,
	15,
	10,
	1,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BaBaLegs';
INSERT INTO `PROPULSION` (
	`unique_inheritance_id`,
	`maxSpeed`,
	`propulsionType`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	200,
	'Legged'
FROM `BASE`
WHERE `BASE`.`pName`='BaBaLegs';

-- Data for table `SENSOR`, extracted from ../stats/sensor.txt

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'ZNULLSENSOR'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1,
	1,
	1,
	1,
	0,
	'gnlsnsr1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='ZNULLSENSOR';
INSERT INTO `SENSOR` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`type`,
	`time`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1024,
	500,
	'DEFAULT',
	'STANDARD',
	0,
	'trlsnsr1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='ZNULLSENSOR';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'UplinkSensor'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1,
	1,
	1,
	200,
	0,
	'miupdish.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='UplinkSensor';
INSERT INTO `SENSOR` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`type`,
	`time`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	2048,
	1000,
	'TURRET',
	'STANDARD',
	0,
	'TRLSNSR1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='UplinkSensor';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Sys-VTOLRadarTower01'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	20,
	100,
	100,
	200,
	0,
	'GNMSNSR2.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Sys-VTOLRadarTower01';
INSERT INTO `SENSOR` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`type`,
	`time`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	2048,
	1000,
	'TURRET',
	'VTOL INTERCEPT',
	0,
	'TRMECM2.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Sys-VTOLRadarTower01';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Sys-VTOLCBTurret01'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	20,
	100,
	100,
	200,
	1,
	'GNHSNSR3.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Sys-VTOLCBTurret01';
INSERT INTO `SENSOR` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`type`,
	`time`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	2048,
	1000,
	'TURRET',
	'VTOL CB',
	0,
	'TRHSNSR3.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Sys-VTOLCBTurret01';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Sys-VTOLCBTower01'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	20,
	100,
	100,
	200,
	0,
	'GNHSNSR3.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Sys-VTOLCBTower01';
INSERT INTO `SENSOR` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`type`,
	`time`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	2048,
	1000,
	'TURRET',
	'VTOL CB',
	0,
	'TRHSNSR3.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Sys-VTOLCBTower01';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Sys-VstrikeTurret01'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	20,
	100,
	100,
	0,
	1,
	'GNMSNSR2.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Sys-VstrikeTurret01';
INSERT INTO `SENSOR` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`type`,
	`time`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	2048,
	1000,
	'TURRET',
	'VTOL INTERCEPT',
	0,
	'TRLSNSR1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Sys-VstrikeTurret01';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Sys-NXLinkTurret01'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	20,
	100,
	100,
	200,
	1,
	'GNMECM2.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Sys-NXLinkTurret01';
INSERT INTO `SENSOR` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`type`,
	`time`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	2048,
	1000,
	'TURRET',
	'STANDARD',
	0,
	'TRMSNSR2.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Sys-NXLinkTurret01';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Sys-CBTurret01'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	20,
	100,
	100,
	200,
	1,
	'GNMECM2.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Sys-CBTurret01';
INSERT INTO `SENSOR` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`type`,
	`time`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	2048,
	1000,
	'TURRET',
	'INDIRECT CB',
	0,
	'TRMSNSR2.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Sys-CBTurret01';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Sys-CBTower01'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	20,
	100,
	100,
	200,
	0,
	'GNMECM2.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Sys-CBTower01';
INSERT INTO `SENSOR` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`type`,
	`time`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	2048,
	1000,
	'TURRET',
	'INDIRECT CB',
	0,
	'TRMSNSR2.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Sys-CBTower01';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'SensorTurret1Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	20,
	100,
	100,
	200,
	1,
	'GNLSNSR1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='SensorTurret1Mk1';
INSERT INTO `SENSOR` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`type`,
	`time`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1536,
	1000,
	'TURRET',
	'STANDARD',
	0,
	'TRLSNSR1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='SensorTurret1Mk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'SensorTower2Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	5,
	25,
	100,
	200,
	0,
	'GNLSNSR1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='SensorTower2Mk1';
INSERT INTO `SENSOR` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`type`,
	`time`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	2048,
	100,
	'TURRET',
	'STANDARD',
	0,
	'TRMSNSR2.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='SensorTower2Mk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'SensorTower1Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	5,
	25,
	100,
	200,
	0,
	'GNLSNSR1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='SensorTower1Mk1';
INSERT INTO `SENSOR` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`type`,
	`time`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	2048,
	100,
	'TURRET',
	'STANDARD',
	0,
	'TRLSNSR1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='SensorTower1Mk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'NavGunSensor'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	0,
	0,
	0,
	0,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='NavGunSensor';
INSERT INTO `SENSOR` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`type`,
	`time`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	2048,
	500,
	'DEFAULT',
	'STANDARD',
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='NavGunSensor';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'DefaultSensor1Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	0,
	0,
	0,
	0,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='DefaultSensor1Mk1';
INSERT INTO `SENSOR` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`type`,
	`time`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1024,
	500,
	'DEFAULT',
	'STANDARD',
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='DefaultSensor1Mk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'CCSensor'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1,
	1,
	1,
	200,
	0,
	'misensor.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='CCSensor';
INSERT INTO `SENSOR` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`type`,
	`time`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	2048,
	1000,
	'TURRET',
	'STANDARD',
	0,
	'TRLSNSR1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='CCSensor';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'BaBaSensor'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1,
	1,
	1,
	1,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BaBaSensor';
INSERT INTO `SENSOR` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`type`,
	`time`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	100,
	'DEFAULT',
	'STANDARD',
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BaBaSensor';

-- Data for table `ECM`, extracted from ../stats/ecm.txt

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'ZNULLECM'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	0,
	0,
	0,
	0,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='ZNULLECM';
INSERT INTO `ECM` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	0,
	50,
	'DEFAULT',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='ZNULLECM';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'RepairCentre'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	0,
	0,
	0,
	0,
	0,
	'GNHREPAR.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='RepairCentre';
INSERT INTO `ECM` (
	`unique_inheritance_id`,
	`range`,
	`power`,
	`location`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	0,
	0,
	'TURRET',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='RepairCentre';

-- Data for table `WEAPON`, extracted from ../stats/weapons.txt

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'ZNULLWEAPON'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	0,
	250,
	0,
	0,
	0,
	'Mibnkgun.pie'
FROM `BASE`
WHERE `BASE`.`pName`='ZNULLWEAPON';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	0,
	0,
	0,
	0,
	0,
	0,
	1,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	20,
	'NO',
	'MISC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	100,
	0,
	90,
	'-30',
	'YES',
	'YES',
	100,
	'NO',
	0,
	0,
	0,
	0,
	0,
	'MIBNKTUR.PIE',
	'FXLRocPd.PIE',
	'FXMflare.PIE',
	'FXMPExp.PIE',
	'FXMPExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='ZNULLWEAPON';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'TUTMG'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	10,
	50,
	200,
	75,
	1,
	'GNLMG1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='TUTMG';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	768,
	0,
	75,
	50,
	5,
	1,
	0,
	0,
	10,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	20,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'NO',
	100,
	0,
	0,
	10,
	0,
	'TRLMG1.PIE',
	'FXLMgun.PIE',
	'FXTracer.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='TUTMG';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Superweapon2'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	10,
	50,
	100,
	9999,
	0,
	'GNLMG1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Superweapon2';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	768,
	0,
	2,
	1,
	4,
	1,
	0,
	0,
	9999,
	1000,
	100,
	1000,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'CANNON',
	'DIRECT',
	'ANTI TANK',
	20,
	180,
	90,
	'-60',
	'YES',
	'NO',
	25,
	'NO',
	0,
	0,
	0,
	10,
	100,
	'TRLMG1.PIE',
	'FXLMgun.PIE',
	'FXTracer.PIE',
	'FXGRDexl.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Superweapon2';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Superweapon'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	10,
	50,
	100,
	9999,
	0,
	'GNLMG1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Superweapon';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	768,
	0,
	99,
	90,
	4,
	1,
	0,
	0,
	9999,
	64,
	99,
	1000,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI TANK',
	20,
	180,
	90,
	'-60',
	'YES',
	'NO',
	25,
	'NO',
	0,
	0,
	0,
	10,
	100,
	'TRLMG1.PIE',
	'FXLMgun.PIE',
	'FXTracer.PIE',
	'FXGRDexl.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Superweapon';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'SpyTurret01'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	75,
	375,
	0,
	999,
	0,
	'GNHECM3.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='SpyTurret01';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	256,
	384,
	0,
	30,
	100,
	10,
	0,
	0,
	0,
	1,
	0,
	0,
	0,
	0,
	0,
	0,
	1200,
	0,
	'NO',
	'KINETIC',
	'ELECTRONIC',
	'DIRECT',
	'ANTI PERSONNEL',
	0,
	180,
	90,
	'-60',
	'YES',
	'YES',
	100,
	'NO',
	0,
	0,
	0,
	0,
	0,
	'TRHECM3.PIE',
	'FXLRocPd.PIE',
	'FXMflare.PIE',
	'FXMPExp.PIE',
	'FXMPExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='SpyTurret01';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Rocket-VTOL-Pod'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	75,
	375,
	200,
	5,
	1,
	'GNLRCKTP.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-VTOL-Pod';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	128,
	50,
	30,
	5,
	2,
	0,
	0,
	50,
	0,
	0,
	0,
	0,
	0,
	0,
	1500,
	20,
	'YES',
	'KINETIC',
	'ROCKET',
	'DIRECT',
	'ANTI TANK',
	10,
	180,
	20,
	'-60',
	'YES',
	'YES',
	25,
	'YES',
	100,
	4,
	0,
	10,
	10,
	'TRLRCKTP.PIE',
	'FXLRocPd.PIE',
	'FXMflare.PIE',
	'FXMPExp.PIE',
	'FXMPExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-VTOL-Pod';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Rocket-VTOL-LtA-T'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	100,
	500,
	250,
	5,
	1,
	'GNMRCKTA.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-VTOL-LtA-T';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	1152,
	128,
	50,
	60,
	1,
	2,
	2,
	120,
	240,
	0,
	0,
	0,
	0,
	0,
	0,
	1200,
	20,
	'YES',
	'KINETIC',
	'SLOW ROCKET',
	'DIRECT',
	'ANTI AIRCRAFT',
	0,
	180,
	90,
	'-60',
	'YES',
	'NO',
	50,
	'YES',
	100,
	1,
	0,
	0,
	10,
	'TRMRCKTA.PIE',
	'FXMRocAt.PIE',
	'FXMPLME.PIE',
	'FXGRDexl.PIE',
	'FXGRDexl.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-VTOL-LtA-T';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Rocket-VTOL-HvyA-T'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	250,
	1250,
	750,
	5,
	1,
	'Gnmrcktb.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-VTOL-HvyA-T';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	1152,
	128,
	50,
	60,
	1,
	2,
	2,
	160,
	375,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	60,
	'YES',
	'KINETIC',
	'SLOW ROCKET',
	'DIRECT',
	'ANTI AIRCRAFT',
	0,
	180,
	0,
	'-60',
	'YES',
	'NO',
	75,
	'YES',
	100,
	1,
	0,
	0,
	10,
	'Trmrcktb.PIE',
	'FXMRocAt.PIE',
	'FXMPLME.PIE',
	'FXMExp.PIE',
	'FXMExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-VTOL-HvyA-T';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Rocket-VTOL-BB'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	150,
	750,
	750,
	5,
	1,
	'Gnmrktbb.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-VTOL-BB';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	1152,
	128,
	50,
	70,
	200,
	2,
	0,
	0,
	180,
	0,
	0,
	0,
	0,
	0,
	0,
	1200,
	20,
	'YES',
	'KINETIC',
	'ROCKET',
	'DIRECT',
	'BUNKER BUSTER',
	0,
	180,
	20,
	'-60',
	'YES',
	'NO',
	25,
	'YES',
	0,
	1,
	0,
	10,
	10,
	'TRMRKTBB.PIE',
	'FXLRocPd.PIE',
	'FXSPLME.PIE',
	'FXGRDexl.PIE',
	'FXGRDexl.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-VTOL-BB';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Rocket-Pod'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	75,
	375,
	200,
	5,
	1,
	'GNLRCKTP.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-Pod';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	128,
	50,
	30,
	10,
	2,
	0,
	0,
	20,
	0,
	0,
	0,
	0,
	0,
	0,
	1500,
	20,
	'YES',
	'KINETIC',
	'ROCKET',
	'DIRECT',
	'ANTI TANK',
	10,
	180,
	90,
	'-60',
	'YES',
	'YES',
	25,
	'YES',
	0,
	0,
	0,
	10,
	10,
	'TRLRCKTP.PIE',
	'FXLRocPd.PIE',
	'FXMflare.PIE',
	'FXMPExp.PIE',
	'FXMPExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-Pod';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Rocket-MRL'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	100,
	500,
	250,
	5,
	1,
	'GNMRCKT.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-MRL';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	384,
	960,
	128,
	40,
	50,
	1,
	2,
	8,
	150,
	30,
	16,
	100,
	30,
	0,
	0,
	0,
	1200,
	20,
	'YES',
	'KINETIC',
	'ROCKET',
	'INDIRECT',
	'ARTILLERY ROUND',
	0,
	180,
	90,
	'-60',
	'YES',
	'YES',
	25,
	'YES',
	0,
	0,
	0,
	0,
	10,
	'TRMRCKT.PIE',
	'FXMRoc.PIE',
	'FXMflare.PIE',
	'FXGRDexl.PIE',
	'FXGRDexl.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-MRL';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Rocket-LtA-T'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	100,
	500,
	250,
	5,
	1,
	'GNMRCKTA.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-LtA-T';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	1152,
	128,
	50,
	60,
	1,
	2,
	2,
	120,
	160,
	0,
	0,
	0,
	0,
	0,
	0,
	1200,
	20,
	'YES',
	'KINETIC',
	'SLOW ROCKET',
	'DIRECT',
	'ANTI TANK',
	0,
	180,
	90,
	'-60',
	'YES',
	'NO',
	50,
	'YES',
	0,
	0,
	0,
	0,
	10,
	'TRMRCKTA.PIE',
	'FXMRocAt.PIE',
	'FXMPLME.PIE',
	'FXGRDexl.PIE',
	'FXGRDexl.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-LtA-T';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Rocket-IDF'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	300,
	1500,
	10000,
	5,
	1,
	'GNHRCKT.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-IDF';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1024,
	11000,
	640,
	70,
	80,
	1,
	2,
	6,
	600,
	100,
	64,
	99,
	50,
	0,
	0,
	0,
	1000,
	10,
	'NO',
	'KINETIC',
	'SLOW ROCKET',
	'INDIRECT',
	'ARTILLERY ROUND',
	0,
	0,
	90,
	'-60',
	'YES',
	'NO',
	50,
	'YES',
	0,
	0,
	0,
	0,
	0,
	'TRHRCKT.PIE',
	'FXLRocPd.PIE',
	'FXHPLME.PIE',
	'FXMNExp.PIE',
	'FXMNExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-IDF';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Rocket-HvyA-T'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	250,
	1250,
	250,
	5,
	1,
	'Gnmrcktb.pie'
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-HvyA-T';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	1152,
	128,
	50,
	60,
	1,
	2,
	2,
	160,
	250,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	60,
	'YES',
	'KINETIC',
	'SLOW ROCKET',
	'DIRECT',
	'ANTI TANK',
	0,
	180,
	90,
	0,
	'YES',
	'NO',
	75,
	'YES',
	0,
	0,
	0,
	0,
	10,
	'Trmrcktb.pie',
	'FXMRocAt.PIE',
	'FXMPLME.PIE',
	'FXGRDexl.PIE',
	'FXGRDexl.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-HvyA-T';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Rocket-BB'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	150,
	750,
	250,
	5,
	1,
	'Gnmrktbb.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-BB';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	1152,
	128,
	60,
	70,
	200,
	2,
	0,
	0,
	125,
	0,
	0,
	0,
	0,
	0,
	0,
	1200,
	20,
	'YES',
	'KINETIC',
	'SLOW ROCKET',
	'DIRECT',
	'BUNKER BUSTER',
	0,
	180,
	90,
	'-60',
	'YES',
	'NO',
	25,
	'YES',
	0,
	0,
	0,
	10,
	10,
	'TRMRKTBB.PIE',
	'FXLRocPd.PIE',
	'FXSPLME.PIE',
	'FXGRDexl.PIE',
	'FXGRDexl.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Rocket-BB';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'RailGun3Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	400,
	1600,
	5000,
	750,
	1,
	'GNHGSS.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='RailGun3Mk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1024,
	2048,
	0,
	80,
	70,
	80,
	1,
	0,
	0,
	300,
	0,
	50,
	30,
	0,
	0,
	0,
	900,
	20,
	'YES',
	'KINETIC',
	'GAUSS',
	'DIRECT',
	'ANTI TANK',
	100,
	180,
	90,
	'-60',
	'YES',
	'NO',
	150,
	'YES',
	0,
	0,
	0,
	10,
	10,
	'TRHGSS.PIE',
	'FXHGauss.PIE',
	'FXGammoH.PIE',
	'Fxflech2.PIE',
	'Fxflech2.PIE',
	'FXVLSWav.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='RailGun3Mk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'RailGun2Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	300,
	1200,
	2000,
	500,
	1,
	'GNMGSS.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='RailGun2Mk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1536,
	0,
	80,
	70,
	60,
	1,
	0,
	0,
	200,
	0,
	0,
	0,
	0,
	0,
	0,
	1100,
	20,
	'YES',
	'KINETIC',
	'GAUSS',
	'DIRECT',
	'ANTI TANK',
	0,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'YES',
	0,
	0,
	0,
	0,
	0,
	'TRMGSS.PIE',
	'FXMGauss.PIE',
	'FXGammoM.PIE',
	'Fxflech2.PIE',
	'Fxflech2.PIE',
	'FXMSWave.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='RailGun2Mk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'RailGun2-VTOL'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	300,
	1200,
	1000,
	250,
	1,
	'GNMGSS.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='RailGun2-VTOL';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1536,
	0,
	80,
	70,
	60,
	1,
	0,
	0,
	400,
	0,
	0,
	0,
	0,
	0,
	0,
	1100,
	20,
	'YES',
	'KINETIC',
	'GAUSS',
	'DIRECT',
	'ANTI AIRCRAFT',
	0,
	180,
	0,
	'-60',
	'YES',
	'NO',
	100,
	'YES',
	0,
	2,
	0,
	0,
	0,
	'TRMGSS.PIE',
	'FXMGauss.PIE',
	'FXGammoM.PIE',
	'Fxflech2.PIE',
	'Fxflech2.PIE',
	'FXMSWave.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='RailGun2-VTOL';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'RailGun1Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	250,
	1000,
	400,
	400,
	1,
	'GNLGSS.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='RailGun1Mk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1536,
	0,
	80,
	70,
	40,
	1,
	3,
	0,
	150,
	0,
	0,
	0,
	0,
	0,
	0,
	1200,
	20,
	'YES',
	'KINETIC',
	'GAUSS',
	'DIRECT',
	'ANTI TANK',
	0,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'NO',
	0,
	0,
	0,
	10,
	0,
	'TRLGSS.PIE',
	'FXLGauss.PIE',
	'FXGAmmo.PIE',
	'FXFlecht.PIE',
	'FXFlecht.PIE',
	'FXFlecht.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='RailGun1Mk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'RailGun1-VTOL'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	250,
	1000,
	600,
	200,
	1,
	'GNLGSS.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='RailGun1-VTOL';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1536,
	0,
	70,
	60,
	40,
	1,
	3,
	0,
	320,
	0,
	0,
	0,
	0,
	0,
	0,
	1200,
	20,
	'YES',
	'KINETIC',
	'GAUSS',
	'DIRECT',
	'ANTI AIRCRAFT',
	0,
	180,
	0,
	'-60',
	'YES',
	'NO',
	75,
	'NO',
	100,
	2,
	0,
	10,
	0,
	'TRLGSS.PIE',
	'FXLGauss.PIE',
	'FXGAmmo.PIE',
	'FXFlecht.PIE',
	'FXFlecht.PIE',
	'FXFlecht.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='RailGun1-VTOL';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'QuadRotAAGun'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	150,
	1200,
	10000,
	200,
	1,
	'gnhair2.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='QuadRotAAGun';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	2048,
	0,
	70,
	75,
	3,
	2,
	0,
	0,
	45,
	64,
	25,
	20,
	0,
	0,
	0,
	1500,
	20,
	'YES',
	'KINETIC',
	'A-A GUN',
	'DIRECT',
	'ANTI AIRCRAFT',
	20,
	180,
	90,
	'-45',
	'YES',
	'NO',
	20,
	'YES',
	1,
	0,
	0,
	10,
	100,
	'TRHAIR.PIE',
	'FXCan40m.PIE',
	'FXAALSH2.PIE',
	'FXAIREXP.PIE',
	'FXAIREXP.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='QuadRotAAGun';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'QuadMg1AAGun'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	100,
	400,
	10000,
	175,
	1,
	'gnlair.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='QuadMg1AAGun';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	2048,
	0,
	70,
	75,
	6,
	2,
	0,
	0,
	40,
	64,
	25,
	20,
	0,
	0,
	0,
	1400,
	20,
	'YES',
	'KINETIC',
	'A-A GUN',
	'DIRECT',
	'ANTI AIRCRAFT',
	30,
	180,
	90,
	'-45',
	'YES',
	'NO',
	20,
	'YES',
	1,
	0,
	0,
	10,
	100,
	'TRMAIR.PIE',
	'FXCan40m.PIE',
	'FXAALSHT.PIE',
	'FXAIREXP.PIE',
	'FXAIREXP.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='QuadMg1AAGun';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'PlasmaHeavy'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	75,
	375,
	10000,
	5,
	0,
	'GNHPLASM.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='PlasmaHeavy';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1920,
	10000,
	768,
	40,
	99,
	60,
	2,
	0,
	0,
	500,
	256,
	99,
	250,
	30,
	100,
	32,
	400,
	20,
	'NO',
	'HEAT',
	'FLAME',
	'INDIRECT',
	'ANTI TANK',
	50,
	180,
	90,
	'-30',
	'YES',
	'YES',
	200,
	'YES',
	0,
	0,
	0,
	10,
	10,
	'TRHPLASM.PIE',
	'FXMHowt.PIE',
	'FXMflare.PIE',
	'FXLExp.PIE',
	'FXLExp.PIE',
	'FXMSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='PlasmaHeavy';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'NX-CyborgPulseLas'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	125,
	375,
	120,
	100,
	0,
	'CY_LAS.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='NX-CyborgPulseLas';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1536,
	0,
	80,
	65,
	30,
	1,
	0,
	0,
	100,
	0,
	0,
	0,
	0,
	0,
	0,
	1600,
	20,
	'YES',
	'HEAT',
	'ENERGY',
	'DIRECT',
	'ANTI PERSONNEL',
	10,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'YES',
	0,
	0,
	0,
	10,
	0,
	'Cybodyjp.pie',
	'FXLasRot.PIE',
	'FXLFLSH.PIE',
	'FXFLSHL.PIE',
	'FXFLSHL.PIE',
	'FXFLSHL.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='NX-CyborgPulseLas';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'NX-CyborgMiss'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	250,
	700,
	120,
	100,
	0,
	'CY_MISS.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='NX-CyborgMiss';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	2048,
	64,
	70,
	80,
	100,
	2,
	0,
	0,
	300,
	0,
	0,
	0,
	0,
	0,
	0,
	900,
	20,
	'YES',
	'KINETIC',
	'MISSILE',
	'HOMING-DIRECT',
	'ANTI TANK',
	20,
	180,
	90,
	'-60',
	'YES',
	'NO',
	50,
	'YES',
	100,
	0,
	0,
	0,
	10,
	'Cybodyjp.pie',
	'FXLRocPd.PIE',
	'FXMflare.PIE',
	'FXMExp.PIE',
	'FXMExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='NX-CyborgMiss';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'NX-CyborgChaingun'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	10,
	50,
	120,
	50,
	0,
	'cy_gun.pie'
FROM `BASE`
WHERE `BASE`.`pName`='NX-CyborgChaingun';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	768,
	0,
	70,
	50,
	6,
	1,
	0,
	0,
	13,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	20,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'NO',
	0,
	0,
	0,
	10,
	0,
	'Cybodyjp.pie',
	'FXLMgun2.PIE',
	'FXTracer.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='NX-CyborgChaingun';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'NX-Cyb-Rail1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	200,
	800,
	120,
	100,
	0,
	'CY_RAIL.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='NX-Cyb-Rail1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1536,
	0,
	80,
	70,
	30,
	1,
	0,
	0,
	140,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'GAUSS',
	'DIRECT',
	'ANTI TANK',
	50,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'NO',
	0,
	0,
	0,
	10,
	0,
	'Cybodyjp.pie',
	'FXLGauss.PIE',
	'FXGAmmo.PIE',
	'FXFlecht.PIE',
	'FXFlecht.PIE',
	'FXFlecht.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='NX-Cyb-Rail1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'NEXUSlink'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	75,
	375,
	0,
	300,
	0,
	'GNHECM3.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='NEXUSlink';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	1024,
	0,
	99,
	100,
	20,
	0,
	0,
	0,
	10,
	0,
	0,
	0,
	0,
	0,
	0,
	1200,
	0,
	'NO',
	'KINETIC',
	'ELECTRONIC',
	'DIRECT',
	'ANTI PERSONNEL',
	0,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'YES',
	0,
	0,
	0,
	0,
	0,
	'TRHECM3.PIE',
	'FXHBLas.PIE',
	'FXMflare.PIE',
	'FXSFlms.PIE',
	'FXSFlms.PIE',
	'FXSFlms.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='NEXUSlink';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Mortar3ROTARYMk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	300,
	900,
	1000,
	5,
	1,
	'GNHMORT.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Mortar3ROTARYMk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1152,
	2304,
	128,
	40,
	50,
	20,
	2,
	0,
	0,
	40,
	64,
	99,
	40,
	0,
	0,
	0,
	1000,
	20,
	'NO',
	'KINETIC',
	'MORTARS',
	'INDIRECT',
	'ARTILLERY ROUND',
	150,
	0,
	90,
	0,
	'YES',
	'NO',
	100,
	'YES',
	0,
	0,
	0,
	0,
	10,
	'TRHRMORT.PIE',
	'FXMMort.PIE',
	'FXCAmmo.PIE',
	'FXLExp.PIE',
	'FXLExp.PIE',
	'FXLSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Mortar3ROTARYMk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Mortar2Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	200,
	1000,
	5000,
	5,
	1,
	'GNHMORT2.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Mortar2Mk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1152,
	2304,
	128,
	40,
	50,
	90,
	2,
	0,
	0,
	80,
	96,
	99,
	80,
	0,
	0,
	0,
	1000,
	20,
	'NO',
	'KINETIC',
	'MORTARS',
	'INDIRECT',
	'ARTILLERY ROUND',
	150,
	0,
	90,
	0,
	'YES',
	'NO',
	100,
	'YES',
	0,
	0,
	0,
	0,
	10,
	'TRHRMORT.PIE',
	'FXMMort.PIE',
	'FXCAmmo.PIE',
	'FXMNExp.PIE',
	'FXLExp.PIE',
	'FXMNExp.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Mortar2Mk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Mortar1Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	100,
	500,
	2000,
	5,
	1,
	'GNMMORT.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Mortar1Mk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1152,
	2304,
	128,
	40,
	50,
	60,
	2,
	0,
	0,
	50,
	64,
	99,
	40,
	0,
	0,
	0,
	1000,
	20,
	'NO',
	'KINETIC',
	'MORTARS',
	'INDIRECT',
	'ARTILLERY ROUND',
	150,
	0,
	90,
	0,
	'YES',
	'NO',
	75,
	'YES',
	0,
	0,
	0,
	0,
	10,
	'TRMMORT.PIE',
	'FxCan75m.PIE',
	'FXTracer.PIE',
	'FXLExp.PIE',
	'FXLExp.PIE',
	'FXLSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Mortar1Mk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Missile-VTOL-AT'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	300,
	1200,
	750,
	50,
	1,
	'Gnmmslat.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Missile-VTOL-AT';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	2048,
	0,
	70,
	80,
	0,
	2,
	2,
	75,
	600,
	64,
	99,
	20,
	0,
	0,
	0,
	900,
	20,
	'YES',
	'KINETIC',
	'MISSILE',
	'HOMING-DIRECT',
	'ANTI AIRCRAFT',
	20,
	180,
	20,
	'-30',
	'YES',
	'NO',
	50,
	'YES',
	100,
	1,
	0,
	0,
	10,
	'Trmmslat.PIE',
	'FXMRocAt.PIE',
	'FXMPLME.PIE',
	'FXMExp.PIE',
	'FXMExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Missile-VTOL-AT';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Missile-MdArt'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	400,
	1200,
	1000,
	50,
	1,
	'Gnmmslaa.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Missile-MdArt';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	2000,
	12000,
	512,
	50,
	80,
	2,
	2,
	4,
	300,
	200,
	96,
	99,
	100,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MISSILE',
	'INDIRECT',
	'ARTILLERY ROUND',
	10,
	180,
	90,
	'-30',
	'YES',
	'NO',
	100,
	'YES',
	0,
	0,
	0,
	0,
	10,
	'Trmmslaa.PIE',
	'FXMMort.PIE',
	'FXMPLME.PIE',
	'FXMExp.PIE',
	'FXMExp.PIE',
	'FXMExp.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Missile-MdArt';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Missile-LtSAM'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	200,
	800,
	400,
	50,
	1,
	'Gnmmslsa.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Missile-LtSAM';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	2000,
	2562,
	128,
	60,
	70,
	5,
	2,
	2,
	150,
	250,
	0,
	0,
	0,
	0,
	0,
	0,
	800,
	20,
	'YES',
	'KINETIC',
	'MISSILE',
	'HOMING-DIRECT',
	'ANTI AIRCRAFT',
	25,
	180,
	90,
	'-45',
	'YES',
	'YES',
	100,
	'YES',
	1,
	0,
	0,
	0,
	10,
	'Trmmslsa.PIE',
	'FXATMiss.PIE',
	'FXMflare.PIE',
	'FXMExp.PIE',
	'FXSExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Missile-LtSAM';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Missile-HvySAM'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	300,
	1200,
	6000,
	10,
	1,
	'Gnhmslsa.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Missile-HvySAM';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	2000,
	2562,
	512,
	60,
	70,
	5,
	2,
	4,
	150,
	250,
	0,
	0,
	0,
	0,
	0,
	0,
	700,
	20,
	'NO',
	'KINETIC',
	'MISSILE',
	'HOMING-DIRECT',
	'ANTI AIRCRAFT',
	25,
	180,
	90,
	'-45',
	'YES',
	'YES',
	100,
	'YES',
	1,
	0,
	0,
	0,
	10,
	'Trhmslsa.PIE',
	'FXICBM.PIE',
	'FXMflare.PIE',
	'FXMExp.PIE',
	'FXMExp.PIE',
	'FXMExp.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Missile-HvySAM';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Missile-HvyArt'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	500,
	1500,
	10000,
	50,
	1,
	'gnhmslab.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Missile-HvyArt';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	2000,
	25000,
	512,
	50,
	80,
	5,
	1,
	4,
	400,
	250,
	128,
	99,
	150,
	0,
	0,
	0,
	800,
	20,
	'NO',
	'KINETIC',
	'MISSILE',
	'INDIRECT',
	'ARTILLERY ROUND',
	25,
	0,
	45,
	'-30',
	'YES',
	'NO',
	100,
	'YES',
	0,
	0,
	0,
	0,
	10,
	'trhmslab.PIE',
	'FXICBM.PIE',
	'FXHPLME.PIE',
	'FXMExp.PIE',
	'FXMExp.PIE',
	'FXMExp.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Missile-HvyArt';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Missile-A-T'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	300,
	1200,
	400,
	10,
	1,
	'Gnmmslat.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Missile-A-T';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	2048,
	64,
	70,
	80,
	0,
	2,
	2,
	75,
	300,
	0,
	0,
	0,
	0,
	0,
	0,
	900,
	20,
	'YES',
	'KINETIC',
	'MISSILE',
	'HOMING-DIRECT',
	'ANTI TANK',
	20,
	180,
	90,
	'-30',
	'YES',
	'NO',
	50,
	'YES',
	0,
	0,
	0,
	0,
	0,
	'Trmmslat.PIE',
	'FXMRocAt.PIE',
	'FXMPLME.PIE',
	'FXMExp.PIE',
	'FXMExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Missile-A-T';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'MG4ROTARYMk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	100,
	500,
	600,
	300,
	1,
	'GNMMG2.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='MG4ROTARYMk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1152,
	0,
	75,
	50,
	4,
	1,
	0,
	0,
	22,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	30,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'NO',
	100,
	0,
	0,
	10,
	0,
	'TRMMG.PIE',
	'FXMgnVul.PIE',
	'FXTracer.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='MG4ROTARYMk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'MG4ROTARY-VTOL'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	100,
	500,
	300,
	10,
	1,
	'GNMMG2.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='MG4ROTARY-VTOL';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1152,
	0,
	75,
	50,
	4,
	1,
	0,
	0,
	66,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI AIRCRAFT',
	30,
	180,
	0,
	'-60',
	'YES',
	'NO',
	100,
	'NO',
	100,
	6,
	0,
	10,
	0,
	'TRMMG.PIE',
	'FXMgnVul.PIE',
	'FXTracer.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='MG4ROTARY-VTOL';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'MG4ROTARY-Pillbox'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	100,
	500,
	600,
	300,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='MG4ROTARY-Pillbox';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1152,
	0,
	75,
	50,
	4,
	1,
	0,
	0,
	22,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	30,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'NO',
	0,
	0,
	0,
	10,
	0,
	NULL,
	'FXMgnVul.PIE',
	'FXTracer.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='MG4ROTARY-Pillbox';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'MG3Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	50,
	250,
	600,
	175,
	1,
	'GNMMG1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='MG3Mk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	0,
	75,
	50,
	7,
	1,
	0,
	0,
	18,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	20,
	180,
	90,
	'-60',
	'YES',
	'NO',
	150,
	'NO',
	100,
	0,
	0,
	10,
	0,
	'TRMMG.PIE',
	'FXMgnVic.PIE',
	'FXTracer.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='MG3Mk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'MG3-VTOL'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	50,
	250,
	300,
	10,
	1,
	'GNMMG1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='MG3-VTOL';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	0,
	75,
	50,
	7,
	1,
	0,
	0,
	54,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI AIRCRAFT',
	20,
	180,
	0,
	'-60',
	'YES',
	'NO',
	150,
	'NO',
	100,
	4,
	0,
	10,
	0,
	'TRMMG.PIE',
	'FXMgnVic.PIE',
	'FXTracer.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='MG3-VTOL';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'MG3-Pillbox'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	50,
	250,
	600,
	175,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='MG3-Pillbox';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	0,
	75,
	50,
	7,
	1,
	0,
	0,
	18,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	30,
	180,
	90,
	'-60',
	'YES',
	'NO',
	150,
	'NO',
	0,
	0,
	0,
	10,
	0,
	NULL,
	'FXMgnVic.PIE',
	'FXTracer.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='MG3-Pillbox';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'MG2Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	25,
	125,
	400,
	125,
	1,
	'GNLMG2.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='MG2Mk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	768,
	0,
	75,
	50,
	6,
	1,
	0,
	0,
	14,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	20,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'NO',
	100,
	0,
	0,
	10,
	0,
	'TRLMG2.PIE',
	'FXLMgun2.PIE',
	'FXTracr2.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='MG2Mk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'MG2-VTOL'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	25,
	125,
	350,
	10,
	1,
	'GNLMG2.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='MG2-VTOL';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	768,
	0,
	75,
	50,
	6,
	1,
	0,
	0,
	42,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI AIRCRAFT',
	20,
	180,
	0,
	'-60',
	'YES',
	'NO',
	100,
	'NO',
	100,
	4,
	0,
	10,
	0,
	'TRLMG2.PIE',
	'FXLMgun2.PIE',
	'FXTracr2.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='MG2-VTOL';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'MG2-Pillbox'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	25,
	125,
	400,
	125,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='MG2-Pillbox';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	768,
	0,
	75,
	50,
	6,
	1,
	0,
	0,
	14,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	100,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'NO',
	0,
	0,
	0,
	10,
	0,
	NULL,
	'FXLMgun2.PIE',
	'FXTracr2.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='MG2-Pillbox';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'MG1Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	10,
	50,
	200,
	75,
	1,
	'GNLMG1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='MG1Mk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	768,
	0,
	75,
	50,
	5,
	1,
	0,
	0,
	10,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	20,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'NO',
	100,
	0,
	0,
	10,
	0,
	'TRLMG1.PIE',
	'FXLMgun.PIE',
	'FXTracer.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='MG1Mk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'MG1-VTOL'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	10,
	50,
	200,
	75,
	1,
	'GNLMG1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='MG1-VTOL';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	768,
	0,
	75,
	50,
	5,
	1,
	0,
	0,
	20,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI AIRCRAFT',
	20,
	180,
	0,
	'-60',
	'YES',
	'NO',
	100,
	'NO',
	100,
	4,
	0,
	10,
	0,
	'TRLMG1.PIE',
	'FXLMgun.PIE',
	'FXTracer.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='MG1-VTOL';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'MG1-Pillbox'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	10,
	50,
	200,
	75,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='MG1-Pillbox';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	768,
	0,
	75,
	50,
	5,
	1,
	0,
	0,
	10,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	100,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'NO',
	0,
	0,
	0,
	10,
	0,
	NULL,
	'FXLMgun.PIE',
	'FXTracer.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='MG1-Pillbox';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'LasSat'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	100,
	500,
	600,
	500,
	0,
	'GNHBLAS.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='LasSat';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	0,
	70,
	50,
	30,
	1,
	0,
	0,
	4000,
	256,
	99,
	3000,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'HEAT',
	'LAS_SAT',
	'DIRECT',
	'ARTILLERY ROUND',
	0,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'YES',
	0,
	0,
	0,
	10,
	0,
	'TRMLAS.PIE',
	'FXHBLas.PIE',
	'FXBeam.PIE',
	'FXMelt.PIE',
	'FXSFlms.PIE',
	'FXSFlms.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='LasSat';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Laser3BEAMMk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	150,
	600,
	300,
	100,
	1,
	'GNMRLAS.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Laser3BEAMMk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1536,
	0,
	80,
	65,
	30,
	1,
	0,
	0,
	100,
	0,
	0,
	0,
	0,
	0,
	0,
	1600,
	20,
	'YES',
	'HEAT',
	'ENERGY',
	'DIRECT',
	'ANTI PERSONNEL',
	10,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'YES',
	0,
	0,
	0,
	10,
	0,
	'TRMLAS.PIE',
	'FXHBLas.PIE',
	'FXLFLSH.PIE',
	'FXFLSHL.PIE',
	'FXFLSHL.PIE',
	'FXFLSHL.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Laser3BEAMMk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Laser3BEAM-VTOL'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	150,
	600,
	300,
	50,
	1,
	'GNMRLAS.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Laser3BEAM-VTOL';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1536,
	0,
	80,
	65,
	30,
	1,
	0,
	0,
	200,
	0,
	0,
	0,
	0,
	0,
	0,
	1600,
	20,
	'YES',
	'HEAT',
	'ENERGY',
	'DIRECT',
	'ANTI PERSONNEL',
	10,
	180,
	0,
	'-60',
	'YES',
	'NO',
	100,
	'YES',
	100,
	2,
	0,
	10,
	0,
	'TRMLAS.PIE',
	'FXHBLas.PIE',
	'FXLFLSH.PIE',
	'FXFLSHL.PIE',
	'FXFLSHL.PIE',
	'FXFLSHL.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Laser3BEAM-VTOL';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Laser2PULSEMk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	200,
	800,
	500,
	250,
	1,
	'GNMLAS.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Laser2PULSEMk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1024,
	2048,
	0,
	80,
	70,
	50,
	1,
	0,
	0,
	200,
	0,
	0,
	0,
	0,
	0,
	0,
	1800,
	20,
	'YES',
	'HEAT',
	'ENERGY',
	'DIRECT',
	'ANTI PERSONNEL',
	15,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'YES',
	0,
	0,
	0,
	10,
	0,
	'TRMLAS.PIE',
	'FXLasRot.PIE',
	'FXPLAmmo.PIE',
	'FXLENFL.PIE',
	'FXLENFL.PIE',
	'FXMExp.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Laser2PULSEMk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Laser2PULSE-VTOL'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	200,
	800,
	500,
	125,
	1,
	'GNMLAS.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Laser2PULSE-VTOL';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1024,
	2048,
	0,
	70,
	50,
	50,
	1,
	3,
	15,
	400,
	0,
	0,
	0,
	0,
	0,
	0,
	1800,
	20,
	'YES',
	'HEAT',
	'ENERGY',
	'DIRECT',
	'ANTI TANK',
	15,
	180,
	0,
	'-60',
	'YES',
	'NO',
	100,
	'YES',
	100,
	2,
	0,
	10,
	0,
	'TRMLAS.PIE',
	'FXLasRot.PIE',
	'FXPLAmmo.PIE',
	'FXLENFL.PIE',
	'FXLENFL.PIE',
	'FXMExp.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Laser2PULSE-VTOL';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Howitzer150Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	350,
	1250,
	15000,
	5,
	1,
	'GNHHOWT.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Howitzer150Mk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1920,
	10000,
	128,
	40,
	50,
	300,
	2,
	0,
	0,
	250,
	128,
	99,
	250,
	0,
	0,
	0,
	1000,
	20,
	'NO',
	'KINETIC',
	'HOWITZERS',
	'INDIRECT',
	'ARTILLERY ROUND',
	250,
	0,
	90,
	0,
	'YES',
	'NO',
	200,
	'YES',
	0,
	0,
	0,
	0,
	10,
	'TRHHOWT.PIE',
	'FXHHowt.PIE',
	'FXCAmmo.PIE',
	'FXVLExp.PIE',
	'FXVLExp.PIE',
	'FXLSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Howitzer150Mk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Howitzer105Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	250,
	1000,
	10000,
	5,
	1,
	'GNMHOWT.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Howitzer105Mk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1920,
	5000,
	128,
	40,
	50,
	200,
	2,
	0,
	0,
	150,
	128,
	99,
	150,
	0,
	0,
	0,
	1000,
	20,
	'NO',
	'KINETIC',
	'HOWITZERS',
	'INDIRECT',
	'ARTILLERY ROUND',
	250,
	0,
	90,
	0,
	'YES',
	'NO',
	250,
	'YES',
	0,
	0,
	0,
	10,
	10,
	'TRMHOWT.PIE',
	'FxCan75m.PIE',
	'FXCAmmo.PIE',
	'FXLExp.PIE',
	'FXLExp.PIE',
	'FXMSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Howitzer105Mk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Howitzer03-Rot'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	400,
	1600,
	10000,
	5,
	1,
	'GNHHOWT2.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Howitzer03-Rot';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1920,
	7000,
	128,
	40,
	50,
	40,
	2,
	0,
	0,
	100,
	128,
	99,
	100,
	0,
	0,
	0,
	1000,
	20,
	'NO',
	'KINETIC',
	'HOWITZERS',
	'INDIRECT',
	'ARTILLERY ROUND',
	250,
	0,
	90,
	0,
	'YES',
	'NO',
	200,
	'YES',
	0,
	0,
	0,
	0,
	10,
	'TRHHOW2.PIE',
	'FXHHowt2.PIE',
	'FXCAmmo.PIE',
	'FXLExp.PIE',
	'FXLExp.PIE',
	'FXLSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Howitzer03-Rot';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Flame2'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	80,
	400,
	1000,
	75,
	1,
	'GNMFLMR.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Flame2';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	384,
	512,
	64,
	75,
	50,
	150,
	1,
	0,
	0,
	50,
	0,
	0,
	0,
	60,
	20,
	96,
	800,
	20,
	'NO',
	'HEAT',
	'FLAME',
	'DIRECT',
	'FLAMER',
	0,
	180,
	90,
	'-60',
	'YES',
	'NO',
	1,
	'YES',
	0,
	0,
	0,
	10,
	0,
	'TRMFLMR.PIE',
	'FXCan20m.PIE',
	'FXLProj.PIE',
	'FXMNExp.PIE',
	'FXMNExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Flame2';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Flame1Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	40,
	200,
	250,
	40,
	1,
	'GNLFLMR.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Flame1Mk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	256,
	384,
	64,
	90,
	40,
	120,
	1,
	0,
	0,
	30,
	0,
	0,
	0,
	60,
	15,
	32,
	800,
	20,
	'NO',
	'HEAT',
	'FLAME',
	'DIRECT',
	'FLAMER',
	0,
	180,
	90,
	'-60',
	'YES',
	'NO',
	1,
	'YES',
	0,
	0,
	0,
	10,
	0,
	'TRLFLMR.PIE',
	'FXCan20m.PIE',
	'FXLThrow.PIE',
	'FXMETHIT.PIE',
	'FXMETHIT.PIE',
	'FXMETHIT.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Flame1Mk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'CyborgRotMG'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	90,
	270,
	120,
	50,
	0,
	'cy_gun.pie'
FROM `BASE`
WHERE `BASE`.`pName`='CyborgRotMG';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	768,
	0,
	70,
	50,
	6,
	1,
	0,
	0,
	13,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	20,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'NO',
	0,
	0,
	0,
	10,
	0,
	'Cybody.pie',
	'FXLMgun2.PIE',
	'FXTracer.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='CyborgRotMG';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'CyborgRocket'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	125,
	500,
	120,
	50,
	0,
	'cy_rkt.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='CyborgRocket';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	896,
	64,
	50,
	70,
	200,
	2,
	0,
	0,
	100,
	0,
	0,
	0,
	0,
	0,
	0,
	1200,
	20,
	'YES',
	'KINETIC',
	'SLOW ROCKET',
	'DIRECT',
	'ANTI TANK',
	0,
	180,
	90,
	'-60',
	'YES',
	'YES',
	25,
	'YES',
	100,
	0,
	0,
	0,
	10,
	'cybody.PIE',
	'FXLRocPd.PIE',
	'fxmflare.PIE',
	'fxmexp.PIE',
	'fxmexp.PIE',
	'fxssplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='CyborgRocket';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'CyborgFlamer01'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	50,
	200,
	120,
	10,
	0,
	'cy_flame.pie'
FROM `BASE`
WHERE `BASE`.`pName`='CyborgFlamer01';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	256,
	384,
	64,
	80,
	40,
	120,
	1,
	0,
	0,
	25,
	0,
	0,
	0,
	50,
	10,
	32,
	800,
	20,
	'YES',
	'HEAT',
	'FLAME',
	'DIRECT',
	'FLAMER',
	0,
	180,
	90,
	'-60',
	'YES',
	'NO',
	1,
	'YES',
	0,
	0,
	0,
	10,
	0,
	'Cybody.pie',
	'FXCan20m.PIE',
	'FXLThrow.PIE',
	'FXMETHIT.PIE',
	'FXMETHIT.PIE',
	'FXMETHIT.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='CyborgFlamer01';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'CyborgChaingun'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	40,
	160,
	120,
	50,
	0,
	'cy_gun.pie'
FROM `BASE`
WHERE `BASE`.`pName`='CyborgChaingun';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	768,
	0,
	70,
	50,
	6,
	1,
	0,
	0,
	15,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	20,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'NO',
	0,
	0,
	0,
	10,
	0,
	'Cybody.pie',
	'FXLMgun2.PIE',
	'FXTracer.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='CyborgChaingun';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'CyborgCannon'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	60,
	240,
	120,
	50,
	0,
	'cy_CAN.pie'
FROM `BASE`
WHERE `BASE`.`pName`='CyborgCannon';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	512,
	758,
	0,
	70,
	50,
	40,
	2,
	0,
	0,
	30,
	16,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'CANNON',
	'DIRECT',
	'ANTI TANK',
	100,
	180,
	90,
	'-60',
	'YES',
	'NO',
	25,
	'YES',
	0,
	0,
	0,
	10,
	0,
	'Cybody.pie',
	'FXCan20m.PIE',
	'FXcam20.PIE',
	'FXGRDexl.PIE',
	'FXGRDexl.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='CyborgCannon';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Cyb-Wpn-Rail1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	200,
	800,
	120,
	100,
	0,
	'CY_RAIL.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Cyb-Wpn-Rail1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1536,
	0,
	80,
	70,
	40,
	1,
	3,
	0,
	140,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'GAUSS',
	'DIRECT',
	'ANTI TANK',
	50,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'NO',
	0,
	0,
	0,
	10,
	0,
	'Cybody.PIE',
	'FXLGauss.PIE',
	'FXGAmmo.PIE',
	'FXFlecht.PIE',
	'FXFlecht.PIE',
	'FXFlecht.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Cyb-Wpn-Rail1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Cyb-Wpn-Laser'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	150,
	600,
	120,
	100,
	0,
	'CY_LAS.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Cyb-Wpn-Laser';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1536,
	0,
	80,
	65,
	30,
	1,
	0,
	0,
	100,
	0,
	0,
	0,
	0,
	0,
	0,
	1600,
	20,
	'YES',
	'HEAT',
	'ENERGY',
	'DIRECT',
	'ANTI PERSONNEL',
	10,
	180,
	90,
	'-60',
	'YES',
	'NO',
	100,
	'YES',
	0,
	0,
	0,
	10,
	0,
	'Cybody.PIE',
	'FXLasRot.PIE',
	'FXLFLSH.PIE',
	'FXFLSHL.PIE',
	'FXFLSHL.PIE',
	'FXFLSHL.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Cyb-Wpn-Laser';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Cyb-Wpn-Atmiss'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	250,
	1000,
	120,
	100,
	0,
	'CY_MISS.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Cyb-Wpn-Atmiss';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	1536,
	64,
	70,
	80,
	100,
	2,
	0,
	0,
	250,
	0,
	0,
	0,
	0,
	0,
	0,
	900,
	20,
	'YES',
	'KINETIC',
	'MISSILE',
	'HOMING-DIRECT',
	'ANTI TANK',
	20,
	180,
	90,
	'-30',
	'YES',
	'NO',
	50,
	'YES',
	100,
	0,
	0,
	0,
	0,
	'Cybody.PIE',
	'FXATMiss.PIE',
	'FXMPLME.PIE',
	'FXMExp.PIE',
	'FXMExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Cyb-Wpn-Atmiss';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'CommandTurret1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	100,
	100,
	1000,
	999,
	0,
	'GNLCMD1.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='CommandTurret1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1536,
	1536,
	0,
	90,
	99,
	1,
	1,
	0,
	0,
	4,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	0,
	'YES',
	'KINETIC',
	'COMMAND',
	'DIRECT',
	'ANTI PERSONNEL',
	0,
	180,
	90,
	'-30',
	'YES',
	'NO',
	1,
	'YES',
	0,
	0,
	0,
	0,
	0,
	'TRLCMD1.PIE',
	'FXHBLas.PIE',
	'FXBeam.PIE',
	'FXSFlms.PIE',
	'FXSFlms.PIE',
	'FXSFlms.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='CommandTurret1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Cannon5VulcanMk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	200,
	800,
	7500,
	500,
	1,
	'GNMVCAN.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Cannon5VulcanMk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1024,
	0,
	70,
	50,
	20,
	2,
	0,
	0,
	28,
	32,
	25,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'CANNON',
	'DIRECT',
	'ANTI TANK',
	25,
	180,
	90,
	'-60',
	'YES',
	'NO',
	50,
	'YES',
	0,
	0,
	0,
	10,
	0,
	'TRMVCAN.PIE',
	'FXVulCan.PIE',
	'FXTracer.PIE',
	'FXMExp.PIE',
	'FXMExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Cannon5VulcanMk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Cannon5Vulcan-VTOL'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	200,
	800,
	1500,
	10,
	1,
	'GNMVCAN.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Cannon5Vulcan-VTOL';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1024,
	0,
	70,
	50,
	20,
	2,
	0,
	0,
	84,
	32,
	25,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'CANNON',
	'DIRECT',
	'ANTI TANK',
	25,
	180,
	0,
	'-60',
	'YES',
	'NO',
	50,
	'YES',
	100,
	4,
	0,
	10,
	0,
	'TRMVCAN.PIE',
	'FXVulCan.PIE',
	'FXTracer.PIE',
	'FXMExp.PIE',
	'FXMExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Cannon5Vulcan-VTOL';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Cannon4AUTOMk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	175,
	700,
	5000,
	400,
	1,
	'GNLACAN.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Cannon4AUTOMk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1152,
	0,
	70,
	50,
	45,
	2,
	0,
	0,
	55,
	64,
	25,
	10,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'CANNON',
	'DIRECT',
	'ANTI TANK',
	100,
	180,
	90,
	'-60',
	'YES',
	'NO',
	50,
	'YES',
	0,
	0,
	0,
	10,
	10,
	'TRLACAN.PIE',
	'FXCan20A.PIE',
	'FXTracer.PIE',
	'FXGRDexl.PIE',
	'FXGRDexl.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Cannon4AUTOMk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Cannon4AUTO-VTOL'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	175,
	700,
	1000,
	10,
	1,
	'GNLACAN.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Cannon4AUTO-VTOL';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1152,
	0,
	70,
	50,
	45,
	2,
	0,
	0,
	165,
	64,
	25,
	10,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'CANNON',
	'DIRECT',
	'ANTI TANK',
	100,
	180,
	0,
	'-60',
	'YES',
	'NO',
	50,
	'YES',
	100,
	4,
	0,
	10,
	10,
	'TRLACAN.PIE',
	'FXCan20A.PIE',
	'FXTracer.PIE',
	'FXMExp.PIE',
	'FXMExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Cannon4AUTO-VTOL';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Cannon375mmMk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	200,
	800,
	10000,
	500,
	1,
	'GNHCAN.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Cannon375mmMk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	1024,
	0,
	70,
	50,
	50,
	2,
	0,
	0,
	70,
	64,
	25,
	20,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'CANNON',
	'DIRECT',
	'ANTI TANK',
	150,
	180,
	90,
	'-60',
	'YES',
	'NO',
	90,
	'YES',
	0,
	0,
	0,
	10,
	100,
	'TRHCAN.PIE',
	'FxCan75m.PIE',
	'FXCAmmo.PIE',
	'FXGRDexl.PIE',
	'FXGRDexl.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Cannon375mmMk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Cannon2A-TMk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	125,
	500,
	5000,
	350,
	1,
	'GNMCAN.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Cannon2A-TMk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	0,
	70,
	50,
	45,
	2,
	0,
	0,
	45,
	32,
	25,
	10,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'CANNON',
	'DIRECT',
	'ANTI TANK',
	100,
	180,
	90,
	'-60',
	'YES',
	'NO',
	60,
	'YES',
	0,
	0,
	0,
	10,
	100,
	'TRMCAN.PIE',
	'FXCan40m.PIE',
	'FXCAmmo.PIE',
	'FXGRDexl.PIE',
	'FXGRDexl.PIE',
	'FXMSteam.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Cannon2A-TMk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Cannon1Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	75,
	375,
	1000,
	200,
	1,
	'GNLCAN.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Cannon1Mk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	0,
	70,
	50,
	40,
	2,
	0,
	0,
	30,
	16,
	20,
	5,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'CANNON',
	'DIRECT',
	'ANTI TANK',
	100,
	180,
	90,
	'-90',
	'YES',
	'NO',
	30,
	'YES',
	0,
	0,
	0,
	10,
	100,
	'TRLCAN.PIE',
	'FXCan20m.PIE',
	'FXcam20.PIE',
	'FXGRDexl.PIE',
	'FXGRDexl.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Cannon1Mk1';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Cannon1-VTOL'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	75,
	375,
	1000,
	10,
	1,
	'GNLCAN.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Cannon1-VTOL';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	0,
	70,
	50,
	40,
	2,
	0,
	0,
	90,
	16,
	20,
	5,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'CANNON',
	'DIRECT',
	'ANTI TANK',
	100,
	180,
	0,
	'-90',
	'YES',
	'NO',
	30,
	'YES',
	100,
	4,
	0,
	10,
	100,
	'TRLCAN.PIE',
	'FXCan20m.PIE',
	'FXcam20.PIE',
	'FXMExp.PIE',
	'FXMExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Cannon1-VTOL';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'BusCannon'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	10,
	10,
	10,
	10,
	0,
	'GNLCAN.pie'
FROM `BASE`
WHERE `BASE`.`pName`='BusCannon';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	0,
	60,
	50,
	30,
	1,
	0,
	0,
	28,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'CANNON',
	'DIRECT',
	'ANTI TANK',
	100,
	180,
	90,
	'-30',
	'YES',
	'NO',
	25,
	'YES',
	0,
	0,
	0,
	10,
	0,
	'Exturret.pie',
	'FXLMgun.PIE',
	'FXTracer.PIE',
	'FXGRDexl.PIE',
	'FXGRDexl.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BusCannon';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'BuggyMG'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	3,
	11,
	1,
	1,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BuggyMG';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	0,
	60,
	50,
	5,
	1,
	0,
	0,
	16,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	100,
	180,
	90,
	'-30',
	'YES',
	'NO',
	100,
	'NO',
	0,
	0,
	0,
	10,
	0,
	NULL,
	'FXLMgun.PIE',
	'FXTracer.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BuggyMG';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'bTrikeMG'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	2,
	11,
	1,
	1,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='bTrikeMG';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	0,
	60,
	50,
	5,
	1,
	0,
	0,
	13,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	100,
	180,
	90,
	'-30',
	'YES',
	'NO',
	100,
	'NO',
	0,
	0,
	0,
	10,
	0,
	NULL,
	'FXLMgun.PIE',
	'FXTracer.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='bTrikeMG';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'BTowerMG'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	12,
	16,
	1,
	1,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BTowerMG';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	0,
	70,
	50,
	5,
	1,
	0,
	0,
	16,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	100,
	180,
	90,
	'-30',
	'YES',
	'NO',
	100,
	'NO',
	0,
	0,
	0,
	10,
	0,
	NULL,
	'FXLMgun.PIE',
	'FXTracRD.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BTowerMG';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Bomb4-VTOL-HvyINC'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	225,
	1000,
	9000,
	50,
	1,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Bomb4-VTOL-HvyINC';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	384,
	512,
	0,
	80,
	70,
	100,
	1,
	0,
	0,
	400,
	0,
	0,
	0,
	200,
	100,
	128,
	475,
	20,
	'YES',
	'KINETIC',
	'BOMB',
	'ERRATIC-DIRECT',
	'ARTILLERY ROUND',
	10,
	180,
	0,
	'-80',
	'YES',
	'NO',
	100,
	'YES',
	0,
	1,
	0,
	10,
	10,
	'trmvtlin.PIE',
	'FXLRocPd.PIE',
	'Fxmbmbi2.PIE',
	'FXLExp.PIE',
	'FXLExp.PIE',
	'FXMSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Bomb4-VTOL-HvyINC';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Bomb3-VTOL-LtINC'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	175,
	700,
	500,
	10,
	1,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Bomb3-VTOL-LtINC';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	384,
	512,
	0,
	80,
	70,
	2,
	2,
	6,
	10,
	100,
	0,
	0,
	0,
	200,
	50,
	128,
	475,
	20,
	'YES',
	'KINETIC',
	'BOMB',
	'ERRATIC-DIRECT',
	'ARTILLERY ROUND',
	10,
	180,
	0,
	'-80',
	'YES',
	'NO',
	50,
	'YES',
	0,
	1,
	0,
	10,
	0,
	'trlvtlin.PIE',
	'FXLRocPd.PIE',
	'Fxlbmbi1.PIE',
	'FXLExp.PIE',
	'FXLExp.PIE',
	'FXMSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Bomb3-VTOL-LtINC';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Bomb2-VTOL-HvHE'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	200,
	800,
	10000,
	50,
	1,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Bomb2-VTOL-HvHE';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	384,
	512,
	0,
	80,
	70,
	1,
	1,
	2,
	2,
	500,
	156,
	99,
	400,
	0,
	0,
	0,
	475,
	20,
	'YES',
	'KINETIC',
	'BOMB',
	'ERRATIC-DIRECT',
	'ARTILLERY ROUND',
	10,
	180,
	0,
	'-80',
	'YES',
	'NO',
	200,
	'YES',
	0,
	1,
	0,
	10,
	10,
	'trmvtlhe.PIE',
	'FXLRocPd.PIE',
	'Fxmbmbx2.PIE',
	'FXLExp.PIE',
	'FXLExp.PIE',
	'FXMSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Bomb2-VTOL-HvHE';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Bomb1-VTOL-LtHE'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	150,
	600,
	500,
	10,
	1,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Bomb1-VTOL-LtHE';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	384,
	512,
	0,
	80,
	70,
	1,
	2,
	3,
	100,
	100,
	192,
	99,
	60,
	0,
	0,
	0,
	475,
	20,
	'YES',
	'KINETIC',
	'BOMB',
	'DIRECT',
	'ARTILLERY ROUND',
	10,
	180,
	0,
	'-80',
	'YES',
	'NO',
	100,
	'YES',
	0,
	1,
	0,
	10,
	10,
	'trlvtlhe.PIE',
	'FXLRocPd.PIE',
	'Fxlbmbx1.PIE',
	'FXLExp.PIE',
	'FXLExp.PIE',
	'FXMSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Bomb1-VTOL-LtHE';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'BJeepMG'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	4,
	16,
	1,
	1,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BJeepMG';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	0,
	60,
	50,
	5,
	1,
	0,
	0,
	14,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	20,
	180,
	90,
	'-30',
	'YES',
	'NO',
	100,
	'NO',
	0,
	0,
	0,
	10,
	0,
	NULL,
	'FXLMgun.PIE',
	'FXTracRD.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BJeepMG';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'BabaRocket'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	10,
	10,
	100,
	10,
	0,
	'GNLRCKT.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='BabaRocket';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	128,
	30,
	20,
	1,
	2,
	6,
	100,
	20,
	64,
	80,
	20,
	0,
	0,
	0,
	1200,
	20,
	'YES',
	'KINETIC',
	'ROCKET',
	'DIRECT',
	'ANTI TANK',
	0,
	0,
	90,
	'-30',
	'YES',
	'NO',
	50,
	'YES',
	0,
	0,
	0,
	0,
	10,
	'TRLRCKT.PIE',
	'FXLRocPd.PIE',
	'FXMFLARE.PIE',
	'FXGRDexl.PIE',
	'FXGRDexl.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BabaRocket';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'BabaPitRocketAT'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	100,
	500,
	500,
	20,
	0,
	'GNLMSL.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='BabaPitRocketAT';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	128,
	50,
	30,
	3,
	2,
	2,
	120,
	50,
	0,
	0,
	0,
	0,
	0,
	0,
	1200,
	20,
	'YES',
	'KINETIC',
	'ROCKET',
	'DIRECT',
	'ANTI TANK',
	0,
	180,
	90,
	'-30',
	'YES',
	'NO',
	100,
	'YES',
	0,
	0,
	0,
	0,
	10,
	'TRLRCKT.PIE',
	'FXLRocPd.PIE',
	'FXMFLARE.PIE',
	'FXSExp.PIE',
	'FXSExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BabaPitRocketAT';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'BabaPitRocket'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	75,
	375,
	250,
	10,
	0,
	'GNLRCKTP.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='BabaPitRocket';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	128,
	60,
	30,
	6,
	2,
	0,
	0,
	16,
	0,
	0,
	0,
	0,
	0,
	0,
	1200,
	20,
	'YES',
	'KINETIC',
	'ROCKET',
	'DIRECT',
	'ANTI TANK',
	10,
	180,
	90,
	'-30',
	'YES',
	'NO',
	25,
	'YES',
	0,
	0,
	0,
	10,
	10,
	'TRLRCKTP.PIE',
	'FXLRocPd.PIE',
	'FXMFLARE.PIE',
	'FXMPExp.PIE',
	'FXMPExp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BabaPitRocket';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'BaBaMG'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	1,
	1,
	1,
	1,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BaBaMG';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	0,
	50,
	40,
	4,
	1,
	0,
	0,
	10,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'MACHINE GUN',
	'DIRECT',
	'ANTI PERSONNEL',
	20,
	0,
	90,
	'-30',
	'YES',
	'NO',
	100,
	'NO',
	0,
	0,
	0,
	10,
	0,
	NULL,
	'FXLMgun.PIE',
	'FXTracer.PIE',
	'FXMETHIT.PIE',
	'FXDIRTsp.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BaBaMG';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'BabaFlame'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	40,
	200,
	10,
	35,
	0,
	'GNLFLMR.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='BabaFlame';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	256,
	384,
	64,
	90,
	40,
	120,
	1,
	0,
	0,
	25,
	0,
	0,
	0,
	90,
	10,
	32,
	750,
	20,
	'NO',
	'HEAT',
	'FLAME',
	'DIRECT',
	'FLAMER',
	0,
	180,
	90,
	'-60',
	'YES',
	'NO',
	1,
	'YES',
	0,
	0,
	0,
	10,
	0,
	'TRLFLMR.PIE',
	'FXCan20m.PIE',
	'FXLThrow.PIE',
	'FXMETHIT.PIE',
	'FXMETHIT.PIE',
	'FXMETHIT.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BabaFlame';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'BaBaCannon'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	10,
	10,
	10,
	10,
	0,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BaBaCannon';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	640,
	960,
	0,
	60,
	40,
	30,
	1,
	0,
	0,
	28,
	0,
	0,
	0,
	0,
	0,
	0,
	1000,
	20,
	'YES',
	'KINETIC',
	'CANNON',
	'DIRECT',
	'ANTI TANK',
	100,
	180,
	90,
	'-30',
	'YES',
	'NO',
	25,
	'YES',
	0,
	0,
	0,
	10,
	0,
	NULL,
	'FXLMgun.PIE',
	'FXTracer.PIE',
	'FXGRDexl.PIE',
	'FXGRDexl.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='BaBaCannon';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'AAGun2Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	150,
	450,
	10000,
	200,
	1,
	'GNHAIR.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='AAGun2Mk1';
INSERT INTO `WEAPON` (
	`unique_inheritance_id`,
	`shortRange`,
	`longRange`,
	`minRange`,
	`shortHit`,
	`longHit`,
	`firePause`,
	`numExplosions`,
	`numRounds`,
	`reloadTime`,
	`damage`,
	`radius`,
	`radiusHit`,
	`radiusDamage`,
	`incenTime`,
	`incenDamage`,
	`incenRadius`,
	`flightSpeed`,
	`indirectHeight`,
	`fireOnMove`,
	`weaponClass`,
	`weaponSubClass`,
	`movementModel`,
	`weaponEffect`,
	`recoilValue`,
	`rotate`,
	`maxElevation`,
	`minElevation`,
	`facePlayer`,
	`faceInFlight`,
	`effectSize`,
	`lightWorld`,
	`surfaceToAir`,
	`vtolAttackRuns`,
	`penetrate`,
	`directLife`,
	`radiusLife`,
	`pMountGraphic`,
	`pMuzzleGraphic`,
	`pInFlightGraphic`,
	`pTargetHitGraphic`,
	`pTargetMissGraphic`,
	`pWaterHitGraphic`,
	`pTrailGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	768,
	2048,
	0,
	70,
	75,
	1,
	2,
	2,
	10,
	45,
	64,
	100,
	40,
	0,
	0,
	0,
	1200,
	20,
	'NO',
	'KINETIC',
	'A-A GUN',
	'DIRECT',
	'ANTI AIRCRAFT',
	100,
	180,
	90,
	'-45',
	'YES',
	'NO',
	100,
	'YES',
	1,
	0,
	0,
	10,
	100,
	'TRHAIR.PIE',
	'FxCan75m.PIE',
	'FXCAmmo.PIE',
	'FXAIREXP.PIE',
	'FXAIREXP.PIE',
	'FXSSplsh.PIE',
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='AAGun2Mk1';

-- Data for table `CONSTRUCT`, extracted from ../stats/construction.txt

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'ZNULLCONSTRUCT'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	0,
	0,
	0,
	0,
	0,
	'TRLCON.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='ZNULLCONSTRUCT';
INSERT INTO `CONSTRUCT` (
	`unique_inheritance_id`,
	`constructPoints`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	0,
	'TRLCON.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='ZNULLCONSTRUCT';

INSERT INTO `BASE` (
	`pName`
)
VALUES (
	'Spade1Mk1'
);
INSERT INTO `COMPONENT` (
	`unique_inheritance_id`,
	`buildPower`,
	`buildPoints`,
	`weight`,
	`body`,
	`designable`,
	`pIMD`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	17,
	85,
	600,
	50,
	1,
	'TRLCON.PIE'
FROM `BASE`
WHERE `BASE`.`pName`='Spade1Mk1';
INSERT INTO `CONSTRUCT` (
	`unique_inheritance_id`,
	`constructPoints`,
	`pMountGraphic`
)
SELECT
	`BASE`.`unique_inheritance_id`,
	10,
	NULL
FROM `BASE`
WHERE `BASE`.`pName`='Spade1Mk1';

COMMIT TRANSACTION;
