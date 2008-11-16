-- This file is generated automatically, do not edit, change the source (../../../src/stats-db2.tpl) instead.

-- Extracted from ../stats/sensor.txt
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
-- Extracted from ../stats/construction.txt
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
