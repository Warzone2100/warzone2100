-- Table structure for table `brain`

CREATE TABLE `brain` (
  `BrainName` TEXT NOT NULL,
  `techLevel` TEXT NOT NULL,
  `buildPower` INTEGER NOT NULL,
  `buildPoints` INTEGER NOT NULL,
  `weight` INTEGER NOT NULL,
  `hitPoints` INTEGER NOT NULL,
  `systemPoints` INTEGER NOT NULL,
  `weaponName` TEXT NOT NULL,
  `progCap` INTEGER NOT NULL,
  PRIMARY KEY(`BrainName`)
);

-- Data for table `brain`

BEGIN TRANSACTION;
INSERT INTO `brain` (`BrainName`, `techLevel`, `buildPower`, `buildPoints`, `weight`, `hitPoints`, `systemPoints`, `weaponName`, `progCap`) VALUES('ZNULLBRAIN', 'Level All', 0, 0, 0, 0, 0, 'ZNULLWEAPON', 0);
INSERT INTO `brain` (`BrainName`, `techLevel`, `buildPower`, `buildPoints`, `weight`, `hitPoints`, `systemPoints`, `weaponName`, `progCap`) VALUES('CommandBrain01', 'Level All', 1, 1, 1, 1, 1, 'CommandTurret1', 0);
COMMIT TRANSACTION;
