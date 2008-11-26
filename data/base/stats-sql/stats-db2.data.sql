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
	1,
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
	1,
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
