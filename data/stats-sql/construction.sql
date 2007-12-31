-- Table structure for table `construction`

CREATE TABLE `construction` (
  `ConstructName` TEXT NOT NULL,
  `techLevel` TEXT NOT NULL,
  `buildPower` INTEGER NOT NULL,
  `buildPoints` INTEGER NOT NULL,
  `weight` INTEGER NOT NULL,
  `hitPoints` INTEGER NOT NULL,
  `systemPoints` INTEGER NOT NULL,
  `body` INTEGER NOT NULL,
  `GfxFile` TEXT NOT NULL,
  `mountGfx` TEXT NOT NULL,
  `constructPoints` INTEGER NOT NULL,
  `designable` INTEGER NOT NULL,
  PRIMARY KEY(`ConstructName`)
);

-- Data for table `construction`

BEGIN TRANSACTION;
INSERT INTO `construction` (`ConstructName`, `techLevel`, `buildPower`, `buildPoints`, `weight`, `hitPoints`, `systemPoints`, `body`, `GfxFile`, `mountGfx`, `constructPoints`, `designable`) VALUES('ZNULLCONSTRUCT', 'Level All', 0, 0, 0, 0, 0, 0, 'TRLCON.PIE', 'TRLCON.PIE', 0, 0);
INSERT INTO `construction` (`ConstructName`, `techLevel`, `buildPower`, `buildPoints`, `weight`, `hitPoints`, `systemPoints`, `body`, `GfxFile`, `mountGfx`, `constructPoints`, `designable`) VALUES('Spade1Mk1', 'Level All', 17, 85, 600, 20, 5, 50, 'TRLCON.PIE', '0', 10, 1);
COMMIT TRANSACTION;
