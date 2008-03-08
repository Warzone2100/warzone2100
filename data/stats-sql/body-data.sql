BEGIN TRANSACTION;

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

-- Data for table `body`

INSERT INTO `body` VALUES(NULL, 'ZNULLBODY', 'Level All', 'LIGHT', 0, 0, 0, 0, 'MIBNKBOD.PIE', 20, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'TransporterBody', 'Level All', 'SUPER HEAVY', 0, 5000, 150, 5000, 'drtrans.pie', 100, 1, 2000, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'Superbody', 'Level All', 'HEAVY', 10, 10, 2700, 9000, 'DRHBOD11.PIE', 500, 1, 40000, 999, 999, 999, 999, 999, 999, 999, 999, 999, 999, 999, 999, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'FireBody', 'Level One', 'LIGHT', 4, 75, 3000, 200, 'EXFIRE.PIE', 50, 1, 4000, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'CybRotMgGrd', 'Level All', 'LIGHT', 25, 100, 150, 200, 'cybd_std.pie', 100, 1, 600, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'CyborgRkt1Ground', 'Level All', 'LIGHT', 30, 125, 150, 200, 'cybd_std.pie', 100, 1, 500, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'CyborgFlamerGrd', 'Level All', 'LIGHT', 30, 125, 150, 200, 'cybd_std.pie', 100, 1, 500, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'CyborgChain1Ground', 'Level All', 'LIGHT', 30, 125, 150, 200, 'cybd_std.pie', 100, 1, 500, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'CyborgCannonGrd', 'Level All', 'LIGHT', 30, 125, 150, 200, 'cybd_std.pie', 100, 1, 500, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'CybNXRail1Jmp', 'Level All', 'LIGHT', 30, 125, 150, 370, 'cybd_std.pie', 100, 1, 675, 18, 15, 18, 15, 18, 15, 18, 15, 18, 15, 18, 15, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'CybNXPulseLasJmp', 'Level All', 'LIGHT', 30, 125, 150, 370, 'cybd_std.pie', 100, 1, 675, 18, 15, 18, 15, 18, 15, 18, 15, 18, 15, 18, 15, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'CybNXMissJmp', 'Level All', 'LIGHT', 30, 125, 150, 370, 'cybd_std.pie', 100, 1, 675, 18, 15, 18, 15, 18, 15, 18, 15, 18, 15, 18, 15, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'CybFlamer01CGrd', 'Level All', 'LIGHT', 25, 100, 150, 200, 'cybd_std.pie', 100, 1, 600, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'Cyb-Bod-Rail1', 'Level All', 'LIGHT', 30, 125, 150, 200, 'cybd_std.pie', 100, 1, 500, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'Cyb-Bod-Las1', 'Level All', 'LIGHT', 30, 125, 150, 200, 'cybd_std.pie', 100, 1, 500, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'Cyb-Bod-Atmiss', 'Level All', 'LIGHT', 30, 125, 150, 200, 'cybd_std.pie', 100, 1, 500, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'BusBody', 'Level One', 'LIGHT', 4, 75, 2000, 200, 'EXSCHOOL.PIE', 50, 1, 4000, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'Body9REC', 'Level All', 'HEAVY', 90, 420, 3300, 225, 'DRHBOD09.PIE', 500, 1, 18000, 22, 15, 22, 15, 22, 15, 22, 15, 22, 15, 22, 15, 'fxvtl09.pie', 1);
INSERT INTO `body` VALUES(NULL, 'Body8MBT', 'Level All', 'MEDIUM', 37, 250, 1500, 125, 'DRMBOD08.PIE', 250, 1, 15000, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 'fxvtl5to8.pie', 1);
INSERT INTO `body` VALUES(NULL, 'Body7ABT', 'Level Three', 'MEDIUM', 150, 600, 1500, 200, 'DRMBOD07.PIE', 250, 1, 15000, 24, 20, 24, 20, 24, 20, 24, 20, 24, 20, 24, 20, 'fxvtl5to8.pie', 1);
INSERT INTO `body` VALUES(NULL, 'Body6SUPP', 'Level Two-Three', 'MEDIUM', 70, 300, 2500, 145, 'DRMBOD06.PIE', 250, 1, 13000, 18, 9, 18, 9, 18, 9, 18, 9, 18, 9, 18, 9, 'fxvtl5to8.pie', 1);
INSERT INTO `body` VALUES(NULL, 'Body5REC', 'Level All', 'MEDIUM', 50, 250, 2000, 130, 'DRMBOD05.PIE', 250, 1, 15000, 15, 6, 15, 6, 15, 6, 15, 6, 15, 6, 15, 6, 'fxvtl5to8.pie', 1);
INSERT INTO `body` VALUES(NULL, 'Body4ABT', 'Level All', 'LIGHT', 20, 100, 450, 55, 'DRLBOD04.PIE', 100, 1, 5000, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 'fxvtl04.pie', 1);
INSERT INTO `body` VALUES(NULL, 'Body3MBT', 'Level Three', 'LIGHT', 100, 400, 450, 100, 'DRLBOD03.PIE', 100, 1, 5000, 20, 15, 20, 15, 20, 15, 20, 15, 20, 15, 20, 15, 'fxvtl2and3.pie', 1);
INSERT INTO `body` VALUES(NULL, 'Body2SUP', 'Level All', 'LIGHT', 50, 220, 750, 85, 'DRLBOD02.PIE', 100, 1, 4000, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, 'fxvtl2and3.pie', 1);
INSERT INTO `body` VALUES(NULL, 'Body1REC', 'Level All', 'LIGHT', 30, 150, 600, 65, 'DRLBOD01.PIE', 100, 1, 5000, 10, 4, 10, 4, 10, 4, 10, 4, 10, 4, 10, 4, 'fxvtl01.pie', 1);
INSERT INTO `body` VALUES(NULL, 'Body12SUP', 'Level All', 'HEAVY', 55, 350, 2100, 180, 'DRHBOD12.PIE', 500, 1, 20000, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 'fxvtl12.pie', 1);
INSERT INTO `body` VALUES(NULL, 'Body11ABT', 'Level All', 'HEAVY', 70, 350, 2700, 200, 'DRHBOD11.PIE', 500, 1, 20000, 20, 9, 20, 9, 20, 9, 20, 9, 20, 9, 20, 9, 'fxvtl11.pie', 1);
INSERT INTO `body` VALUES(NULL, 'Body10MBT', 'Level Three', 'HEAVY', 200, 800, 2500, 300, 'DRHBOD10.PIE', 500, 1, 23000, 28, 25, 28, 25, 28, 25, 28, 25, 28, 25, 28, 25, 'fxvtl10.pie', 1);
INSERT INTO `body` VALUES(NULL, 'B4body-sml-trike01', 'Level One', 'LIGHT', 2, 65, 675, 80, 'extrike.PIE', 35, 1, 2100, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'B3bodyRKbuggy01', 'Level One', 'LIGHT', 3, 80, 900, 100, 'exbugRK.PIE', 50, 1, 2200, 3, 1, 3, 1, 3, 1, 3, 1, 3, 1, 3, 1, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'B3body-sml-buggy01', 'Level One', 'LIGHT', 3, 80, 900, 100, 'exbuggy.PIE', 50, 1, 2200, 3, 1, 3, 1, 3, 1, 3, 1, 3, 1, 3, 1, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'B2RKJeepBody', 'Level One', 'LIGHT', 4, 75, 900, 120, 'EXjeepRK.PIE', 50, 1, 2200, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'B2JeepBody', 'Level One', 'LIGHT', 4, 75, 900, 120, 'EXjeep.PIE', 50, 1, 2200, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, NULL, 0);
INSERT INTO `body` VALUES(NULL, 'B1BaBaPerson01', 'Level All', 'HEAVY', 1, 20, 100, 29, 'EXBLOKE.PIE', 50, 1, 125, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, NULL, 0);

COMMIT TRANSACTION;
