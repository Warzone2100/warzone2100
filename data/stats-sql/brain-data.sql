BEGIN TRANSACTION;

-- Table structure for table `brain`

CREATE TABLE `brain` (
  id                    INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  name                  TEXT    NOT NULL, -- Text id name (short language independant name)
  techlevel             TEXT    NOT NULL, -- Technology level of this component
  buildPower            NUMERIC NOT NULL, -- Power required to build this component
  buildPoints           NUMERIC NOT NULL, -- Time required to build this component
  weight                NUMERIC NOT NULL, -- Component's weight (mass?)
  hitpoints             NUMERIC NOT NULL, -- Component's hitpoints - SEEMS TO BE UNUSED
  systempoints          NUMERIC NOT NULL, -- Space the component takes in the droid - SEEMS TO BE UNUSED
  weapon                INTEGER,          -- A reference to `weapons`.`id`, refers to the weapon stats associated with this brain (can be NULL for none) - for Command Droids
  program_capcity       INTEGER NOT NULL  -- Program's capacity
);

-- Data for table `brain`

INSERT INTO `brain` SELECT NULL, 'ZNULLBRAIN', 'Level All', 0, 0, 0, 0, 0, `weapons`.`id`, 0 FROM `weapons` WHERE `weapons`.`name`='ZNULLWEAPON';
INSERT INTO `brain` SELECT NULL, 'CommandBrain01' ,'Level All', 1, 1, 1, 1, 1, `weapons`.`id`, 0 FROM `weapons` WHERE `weapons`.`name`='CommandTurret1';

COMMIT TRANSACTION;
