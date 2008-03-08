BEGIN TRANSACTION;

-- Table structure for table `construction`

CREATE TABLE `construction` (
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
  construction_points   INTEGER NOT NULL, -- The number of points contributed each cycle
  designable            INTEGER NOT NULL  -- flag to indicate whether this component can be used in the design screen
);

-- Data for table `construction`

INSERT INTO `construction` VALUES(NULL, 'ZNULLCONSTRUCT', 'Level All', 0, 0, 0, 0, 0, 0, 'TRLCON.PIE', 'TRLCON.PIE', 0, 0);
INSERT INTO `construction` VALUES(NULL, 'Spade1Mk1', 'Level All', 17, 85, 600, 20, 5, 50, 'TRLCON.PIE', 0, 10, 1);

COMMIT TRANSACTION;
