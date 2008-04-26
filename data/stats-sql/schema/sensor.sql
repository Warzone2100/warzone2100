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
