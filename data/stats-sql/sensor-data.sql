BEGIN TRANSACTION;

-- Table structure for table `sensor`

CREATE TABLE `sensor` (
  id                    INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  name                  TEXT    NOT NULL, -- Text id name (short language independant name)
  techlevel             TEXT    NOT NULL, -- Technology level of this component
  buildPower            NUMERIC NOT NULL, -- Power required to build this component
  buildPoints           NUMERIC NOT NULL, -- Time required to build this component
  weight                NUMERIC NOT NULL, -- Component's weight (mass?)
  hitpoints             NUMERIC NOT NULL, -- Component's hitpoints - SEEMS TO BE UNUSED
  systempoints          NUMERIC NOT NULL, -- Space the component takes in the droid - SEEMS TO BE UNUSED
  body                  NUMERIC NOT NULL, -- Component's body points
  GfxFile               TEXT,             -- The IMD to draw for this component
  mountGfx              TEXT,             -- The turret mount to use
  range                 NUMERIC NOT NULL, -- Sensor range
  location              TEXT    NOT NULL, -- specifies whether the Sensor is default or for the Turret
  type                  TEXT    NOT NULL, -- used for combat
  time                  NUMERIC NOT NULL, -- time delay before associated weapon droids 'know' where the attack is from
  power                 NUMERIC NOT NULL, -- Sensor power (put against ecm power)
  designable            NUMERIC NOT NULL  -- flag to indicate whether this component can be used in the design screen
);

-- Data for table `sensor`

INSERT INTO `sensor` VALUES(NULL, 'ZNULLSENSOR', 'Level All', 1, 1, 1, 1, 1, 1, 'gnlsnsr1.PIE', 'trlsnsr1.PIE', 1024, 'DEFAULT', 'STANDARD', 0, 500, 0);
INSERT INTO `sensor` VALUES(NULL, 'UplinkSensor', 'Level All', 1, 1, 1, 1, 1, 200, 'miupdish.PIE', 'TRLSNSR1.PIE', 2048, 'TURRET', 'STANDARD', 0, 1000, 0);
INSERT INTO `sensor` VALUES(NULL, 'Sys-VTOLRadarTower01', 'Level Two-Three', 20, 100, 100, 0, 0, 200, 'GNMSNSR2.PIE', 'TRMECM2.PIE', 2048, 'TURRET', 'VTOL INTERCEPT', 0, 1000, 0);
INSERT INTO `sensor` VALUES(NULL, 'Sys-VTOLCBTurret01', 'Level Two-Three', 20, 100, 100, 0, 0, 200, 'GNHSNSR3.PIE', 'TRHSNSR3.PIE', 2048, 'TURRET', 'VTOL CB', 0, 1000, 1);
INSERT INTO `sensor` VALUES(NULL, 'Sys-VTOLCBTower01', 'Level Two-Three', 20, 100, 100, 0, 0, 200, 'GNHSNSR3.PIE', 'TRHSNSR3.PIE', 2048, 'TURRET', 'VTOL CB', 0, 1000, 0);
INSERT INTO `sensor` VALUES(NULL, 'Sys-VstrikeTurret01', 'Level All', 20, 100, 100, 0, 0, 0, 'GNMSNSR2.PIE', 'TRLSNSR1.PIE', 2048, 'TURRET', 'VTOL INTERCEPT', 0, 1000, 1);
INSERT INTO `sensor` VALUES(NULL, 'Sys-NXLinkTurret01', 'Level Three', 20, 100, 100, 0, 0, 200, 'GNMECM2.PIE', 'TRMSNSR2.PIE', 2048, 'TURRET', 'STANDARD', 0, 1000, 1);
INSERT INTO `sensor` VALUES(NULL, 'Sys-CBTurret01', 'Level All', 20, 100, 100, 0, 0, 200, 'GNMECM2.PIE', 'TRMSNSR2.PIE', 2048, 'TURRET', 'INDIRECT CB', 0, 1000, 1);
INSERT INTO `sensor` VALUES(NULL, 'Sys-CBTower01', 'Level All', 20, 100, 100, 0, 0, 200, 'GNMECM2.PIE', 'TRMSNSR2.PIE', 2048, 'TURRET', 'INDIRECT CB', 0, 1000, 0);
INSERT INTO `sensor` VALUES(NULL, 'SensorTurret1Mk1', 'Level All', 20, 100, 100, 1, 5, 200, 'GNLSNSR1.PIE', 'TRLSNSR1.PIE', 1536, 'TURRET', 'STANDARD', 0, 1000, 1);
INSERT INTO `sensor` VALUES(NULL, 'SensorTower2Mk1', 'Level All', 5, 25, 100, 1, 5, 200, 'GNLSNSR1.PIE', 'TRMSNSR2.PIE', 2048, 'TURRET', 'STANDARD', 0, 100, 0);
INSERT INTO `sensor` VALUES(NULL, 'SensorTower1Mk1', 'Level All', 5, 25, 100, 1, 5, 200, 'GNLSNSR1.PIE', 'TRLSNSR1.PIE', 2048, 'TURRET', 'STANDARD', 0, 100, 0);
INSERT INTO `sensor` VALUES(NULL, 'NavGunSensor', 'Level All', 0, 0, 0, 1, 1, 0, NULL, NULL, 2048, 'DEFAULT', 'STANDARD', 0, 500, 0);
INSERT INTO `sensor` VALUES(NULL, 'DefaultSensor1Mk1', 'Level All', 0, 0, 0, 1, 1, 0, NULL, NULL, 1024, 'DEFAULT', 'STANDARD', 0, 500, 0);
INSERT INTO `sensor` VALUES(NULL, 'CCSensor', 'Level All', 1, 1, 1, 1, 1, 200, 'misensor.PIE', 'TRLSNSR1.PIE', 2048, 'TURRET', 'STANDARD', 0, 1000, 0);
INSERT INTO `sensor` VALUES(NULL, 'BaBaSensor', 'Level All', 1, 1, 1, 1, 1, 1, NULL, NULL, 640, 'DEFAULT', 'STANDARD', 0, 100, 0);

COMMIT TRANSACTION;
