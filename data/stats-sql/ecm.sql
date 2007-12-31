-- Table structure for table `ecm`

CREATE TABLE `ecm` (
  `ECMName` TEXT NOT NULL,
  `techLevel` TEXT NOT NULL,
  `buildPower` INTEGER NOT NULL,
  `buildPoints` INTEGER NOT NULL,
  `weight` INTEGER NOT NULL,
  `hitPoints` INTEGER NOT NULL,
  `systemPoints` INTEGER NOT NULL,
  `body` INTEGER NOT NULL,
  `GfxFile` TEXT NOT NULL,
  `mountGfx` TEXT NOT NULL,
  `location` TEXT NOT NULL,
  `power` INTEGER NOT NULL,
  `designable` INTEGER NOT NULL,
  PRIMARY KEY(`ECMName`)
);

-- Data for table `ecm`

BEGIN TRANSACTION;
INSERT INTO `ecm` (`ECMName`, `techLevel`, `buildPower`, `buildPoints`, `weight`, `hitPoints`, `systemPoints`, `body`, `GfxFile`, `mountGfx`, `location`, `power`, `designable`) VALUES('ZNULLECM', 'Level All', 0, 0, 0, 0, 0, 0, '0', '0', 'DEFAULT', 50, 0);
INSERT INTO `ecm` (`ECMName`, `techLevel`, `buildPower`, `buildPoints`, `weight`, `hitPoints`, `systemPoints`, `body`, `GfxFile`, `mountGfx`, `location`, `power`, `designable`) VALUES('RepairCentre', 'Level All', 0, 0, 0, 0, 0, 0, 'GNHREPAR.PIE', '0', 'TURRET', 0, 0);
COMMIT TRANSACTION;
