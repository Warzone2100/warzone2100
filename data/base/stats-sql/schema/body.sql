-- Table structure for table `body`

CREATE TABLE `body` (
  id                    INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  name                  TEXT    NOT NULL, -- Text id name (short language independant name)
  techlevel             TEXT    NOT NULL, -- Technology level of this component
  size                  TEXT    NOT NULL, -- How big the body is - affects how hit
  buildPower            NUMERIC NOT NULL, -- Power required to build this component
  buildPoints           NUMERIC NOT NULL, -- Time required to build this component
  weight                NUMERIC NOT NULL, -- Component's weight (mass?)
  body                  NUMERIC NOT NULL, -- Component's body points
  GfxFile               TEXT,             -- The IMD to draw for this component
  systempoints          NUMERIC NOT NULL, -- Space the component takes in the droid - SEEMS TO BE UNUSED
  weapon_slots          NUMERIC NOT NULL, -- The number of weapon slots on the body
  power_output          NUMERIC NOT NULL, -- this is the engine output of the body
  armour_front_kinetic  NUMERIC NOT NULL, -- A measure of how much protection the armour provides. Cross referenced with the weapon types.
  armour_front_heat     NUMERIC NOT NULL, -- 
  armour_rear_kinetic   NUMERIC NOT NULL, -- 
  armour_rear_heat      NUMERIC NOT NULL, -- 
  armour_left_kinetic   NUMERIC NOT NULL, -- 
  armour_left_heat      NUMERIC NOT NULL, -- 
  armour_right_kinetic  NUMERIC NOT NULL, -- 
  armour_right_heat     NUMERIC NOT NULL, -- 
  armour_top_kinetic    NUMERIC NOT NULL, -- 
  armour_top_heat       NUMERIC NOT NULL, -- 
  armour_bottom_kinetic NUMERIC NOT NULL, -- 
  armour_bottom_heat    NUMERIC NOT NULL, -- 
  flameIMD              TEXT,             -- pointer to which flame graphic to use - for VTOLs only at the moment
  designable            NUMERIC NOT NULL  -- flag to indicate whether this component can be used in the design screen
);
