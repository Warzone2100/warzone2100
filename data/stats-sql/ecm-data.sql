BEGIN TRANSACTION;

-- Table structure for table `ecm`

CREATE TABLE `ecm` (
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
  location              TEXT    NOT NULL, -- specifies whether the ECM is default or for the Turret
  power                 NUMERIC NOT NULL, -- ECM power (put against sensor power)
  range                 NUMERIC NOT NULL, -- ECM range
  designable            NUMERIC NOT NULL  -- flag to indicate whether this component can be used in the design screen
);

-- Data for table `ecm`

INSERT INTO `ecm` VALUES(NULL, 'ZNULLECM', 'Level All', 0, 0, 0, 0, 0, 0, NULL, NULL, 'DEFAULT', 50, 0, 0);
INSERT INTO `ecm` VALUES(NULL, 'RepairCentre', 'Level All', 0, 0, 0, 0, 0, 0, 'GNHREPAR.PIE', NULL, 'TURRET', 0, 0, 0);

COMMIT TRANSACTION;
