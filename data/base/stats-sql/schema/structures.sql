-- Table structure for table `structures`

CREATE TABLE `structures` (
  id                    INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  name                  TEXT    NOT NULL, -- Text id name (short language independant name)
  type                  TEXT    NOT NULL, -- The type of structure
  techlevel             TEXT    NOT NULL, -- Technology level of this component
  strength              TEXT    NOT NULL, -- strength against the weapon effects
  terrain_type          INTEGER,          -- The type of terrain the structure has to be built next to - may be none (NULL)
  baseWidth             NUMERIC NOT NULL, -- The width of the base in tiles
  baseBreadth           NUMERIC NOT NULL, -- The breadth of the base in tiles
  foundationType        TEXT    NOT NULL, -- The type of foundation for the structure
  buildPoints           NUMERIC NOT NULL, -- The number of build points required to build the structure
  height                NUMERIC NOT NULL, -- The height above/below the terrain (negative values denote below the terrain)
  armourValue           NUMERIC NOT NULL, -- The armour value for the structure (can be upgraded)
  bodyPoints            NUMERIC NOT NULL, -- The structure's body points (A structure goes off-line when 50% of its body points are lost
  repairSystem          NUMERIC NOT NULL, -- The repair system points are added to the body points until fully restored . The points are then added to the Armour Points
  powerToBuild          NUMERIC NOT NULL, -- How much power this structure requires to be built
  resistance            NUMERIC,          -- The number used to determine whether a structure can resist an enemy takeover (0 = cannot be attacked electrically)
  sizeModifier          NUMERIC NOT NULL, -- The larger the target, the easier to hit
  ecm                   INTEGER,          -- Which ECM is standard for the structure, if any (is a reference to `ecm`.`id`)
  sensor                INTEGER,          -- Which sensor is standard for the structure, if any (is a reference to `sensor`.`id`)
  GfxFile               TEXT,             -- The IMD to draw for this component
  BaseGfxFile           TEXT,             -- The base IMD to draw for this structure
  num_functions         INTEGER NOT NULL, -- Number of functions for default
  num_weapons           INTEGER NOT NULL  -- Number of weapons for default
);
