BEGIN TRANSACTION;

-- Data for table `propulsion`

INSERT INTO `propulsion` VALUES(NULL, 'ZNULLPROP', 'Level All', 0, 0, 0, 0, 0, 0, 'MIBNKDRL.PIE', 'Wheeled', 0, 0);
INSERT INTO `propulsion` VALUES(NULL, 'wheeled03', 'Level All', 150, 50, 200, 1, 1, 300, 'PRLRWHL1.PIE', 'Wheeled', 175, 0);
INSERT INTO `propulsion` VALUES(NULL, 'wheeled02', 'Level All', 100, 50, 250, 1, 1, 200, 'PRLRWHL1.PIE', 'Wheeled', 175, 0);
INSERT INTO `propulsion` VALUES(NULL, 'wheeled01', 'Level All', 50, 50, 300, 1, 1, 100, 'PRLRWHL1.PIE', 'Wheeled', 175, 1);
INSERT INTO `propulsion` VALUES(NULL, 'V-Tol03', 'Level All', 300, 125, 50, 0, 1, 300, 'DPVTOL.PIE', 'Lift', 800, 0);
INSERT INTO `propulsion` VALUES(NULL, 'V-Tol02', 'Level All', 250, 125, 50, 0, 1, 150, 'DPVTOL.PIE', 'Lift', 800, 0);
INSERT INTO `propulsion` VALUES(NULL, 'V-Tol', 'Level All', 150, 125, 50, 0, 1, 100, 'DPVTOL.PIE', 'Lift', 700, 1);
INSERT INTO `propulsion` VALUES(NULL, 'tracked03', 'Level All', 275, 125, 550, 1, 1, 800, 'PRLRTRK1.PIE', 'Tracked', 125, 0);
INSERT INTO `propulsion` VALUES(NULL, 'tracked02', 'Level All', 200, 125, 600, 1, 1, 600, 'PRLRTRK1.PIE', 'Tracked', 125, 0);
INSERT INTO `propulsion` VALUES(NULL, 'tracked01', 'Level All', 125, 125, 650, 1, 1, 400, 'PRLRTRK1.PIE', 'Tracked', 125, 1);
INSERT INTO `propulsion` VALUES(NULL, 'hover03', 'Level All', 200, 100, 100, 1, 1, 300, 'PRLHOV1.PIE', 'Hover', 200, 0);
INSERT INTO `propulsion` VALUES(NULL, 'hover02', 'Level All', 150, 100, 150, 1, 1, 200, 'PRLHOV1.PIE', 'Hover', 225, 0);
INSERT INTO `propulsion` VALUES(NULL, 'hover01', 'Level All', 100, 100, 200, 1, 1, 150, 'PRLHOV1.PIE', 'Hover', 200, 1);
INSERT INTO `propulsion` VALUES(NULL, 'HalfTrack03', 'Level All', 125, 75, 300, 1, 1, 500, 'PRLRHTR1.PIE', 'Half-Tracked', 150, 1);
INSERT INTO `propulsion` VALUES(NULL, 'HalfTrack02', 'Level All', 100, 75, 350, 1, 1, 350, 'PRLRHTR1.PIE', 'Half-Tracked', 150, 1);
INSERT INTO `propulsion` VALUES(NULL, 'HalfTrack', 'Level All', 75, 75, 400, 1, 1, 200, 'PRLRHTR1.PIE', 'Half-Tracked', 150, 1);
INSERT INTO `propulsion` VALUES(NULL, 'CyborgLegs03', 'Level All', 10, 50, 100, 1, 1, 150, NULL, 'Legged', 400, 0);
INSERT INTO `propulsion` VALUES(NULL, 'CyborgLegs02', 'Level All', 10, 50, 100, 1, 1, 100, NULL, 'Legged', 400, 0);
INSERT INTO `propulsion` VALUES(NULL, 'CyborgLegs', 'Level All', 10, 50, 100, 1, 1, 50, NULL, 'Legged', 400, 0);
INSERT INTO `propulsion` VALUES(NULL, 'BaBaProp', 'Level All', 0, 15, 10, 1, 5, 1, NULL, 'Wheeled', 200, 0);
INSERT INTO `propulsion` VALUES(NULL, 'BaBaLegs', 'Level All', 0, 15, 10, 1, 5, 1, NULL, 'Legged', 200, 0);

COMMIT TRANSACTION;
