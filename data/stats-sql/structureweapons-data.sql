BEGIN TRANSACTION;

-- Table structure for table `structureweapons`

CREATE TABLE `structureweapons` (
  structure             INTEGER NOT NULL, -- Reference to `structures`.`id`
  weapon                INTEGER NOT NULL, -- Reference to `weapons`.`id`
  UNIQUE(structure,weapon)
);

-- Data for table `structureweapons`

INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='A0BaBaRocketPit' AND `weapons`.`name`='BabaPitRocket';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='A0CannonTower' AND `weapons`.`name`='BaBaCannon';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='WallTower01' AND `weapons`.`name`='MG3Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='WallTower02' AND `weapons`.`name`='Cannon1Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='WallTower03' AND `weapons`.`name`='Cannon2A-TMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='WallTower04' AND `weapons`.`name`='Cannon375mmMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='A0BaBaBunker' AND `weapons`.`name`='MG3Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='GuardTower1' AND `weapons`.`name`='MG3Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='GuardTower2' AND `weapons`.`name`='MG2Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='GuardTower3' AND `weapons`.`name`='MG3Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='GuardTower4' AND `weapons`.`name`='Flame1Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='GuardTower5' AND `weapons`.`name`='Rocket-LtA-T';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='GuardTower6' AND `weapons`.`name`='Rocket-Pod';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='PillBox2' AND `weapons`.`name`='MG2-Pillbox';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='PillBox3' AND `weapons`.`name`='MG3-Pillbox';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='PillBox4' AND `weapons`.`name`='Cannon1Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='A0BaBaGunTowerEND' AND `weapons`.`name`='BuggyMG';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='PillBox1' AND `weapons`.`name`='MG3-Pillbox';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Emplacement-Howitzer105' AND `weapons`.`name`='Howitzer105Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Emplacement-MortarPit01' AND `weapons`.`name`='Mortar1Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='PillBox5' AND `weapons`.`name`='Flame1Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='WallTower05' AND `weapons`.`name`='Flame1Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='WallTower06' AND `weapons`.`name`='Rocket-LtA-T';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='PillBox6' AND `weapons`.`name`='Rocket-LtA-T';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='A0BaBaRocketPitAT' AND `weapons`.`name`='BabaPitRocketAT';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Tower-Projector' AND `weapons`.`name`='Flame2';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='WallTower-Projector' AND `weapons`.`name`='Flame2';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Emplacement-RotMor' AND `weapons`.`name`='Mortar3ROTARYMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Emplacement-RotHow' AND `weapons`.`name`='Howitzer03-Rot';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='AASite-QuadBof' AND `weapons`.`name`='AAGun2Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='AASite-QuadMg1' AND `weapons`.`name`='QuadMg1AAGun';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Tower-RotMg' AND `weapons`.`name`='MG4ROTARYMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Tower-VulcanCan' AND `weapons`.`name`='Cannon4AUTOMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Wall-RotMg' AND `weapons`.`name`='MG4ROTARYMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='AASite-QuadRotMg' AND `weapons`.`name`='QuadRotAAGun';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Wall-VulcanCan' AND `weapons`.`name`='Cannon5VulcanMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='A0BaBaFlameTower' AND `weapons`.`name`='Flame1Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='A0BaBaGunTower' AND `weapons`.`name`='BuggyMG';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Emplacement-PrisLas' AND `weapons`.`name`='Laser3BEAMMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='WallTower-PulseLas' AND `weapons`.`name`='Laser3BEAMMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='WallTower-Rail2' AND `weapons`.`name`='RailGun2Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='GuardTower-BeamLas' AND `weapons`.`name`='Laser3BEAMMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='GuardTower-ATMiss' AND `weapons`.`name`='Missile-A-T';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='GuardTower-Rail1' AND `weapons`.`name`='RailGun1Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Emplacement-Rail3' AND `weapons`.`name`='RailGun3Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Emplacement-Rocket06-IDF' AND `weapons`.`name`='Rocket-IDF';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='CO-Tower-MG3' AND `weapons`.`name`='MG3Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='CO-Tower-RotMG' AND `weapons`.`name`='MG4ROTARYMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='CO-Tower-MdCan' AND `weapons`.`name`='Cannon2A-TMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='CO-WallTower-HvCan' AND `weapons`.`name`='Cannon375mmMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='CO-WallTower-RotCan' AND `weapons`.`name`='Cannon5VulcanMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='CO-Tower-HVCan' AND `weapons`.`name`='Cannon4AUTOMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='CO-Tower-HvFlame' AND `weapons`.`name`='Flame2';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='CO-Tower-LtATRkt' AND `weapons`.`name`='Rocket-LtA-T';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='NX-Tower-Rail1' AND `weapons`.`name`='RailGun1Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='NX-Tower-ATMiss' AND `weapons`.`name`='Missile-A-T';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='NX-Tower-PulseLas' AND `weapons`.`name`='Laser2PULSEMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='NX-WallTower-Rail2' AND `weapons`.`name`='RailGun2Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='NX-WallTower-BeamLas' AND `weapons`.`name`='Laser3BEAMMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='NX-WallTower-Rail3' AND `weapons`.`name`='RailGun3Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='NX-Emp-MedArtMiss-Pit' AND `weapons`.`name`='Missile-MdArt';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='NX-Emp-MultiArtMiss-Pit' AND `weapons`.`name`='Missile-HvyArt';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='NX-Emp-Plasma-Pit' AND `weapons`.`name`='PlasmaHeavy';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='P0-AASite-SAM1' AND `weapons`.`name`='Missile-LtSAM';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='P0-AASite-SAM2' AND `weapons`.`name`='Missile-HvySAM';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Emplacement-MRL-pit' AND `weapons`.`name`='Rocket-MRL';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='GuardTower-RotMg' AND `weapons`.`name`='MG4ROTARYMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Emplacement-HvyATrocket' AND `weapons`.`name`='Rocket-HvyA-T';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Emplacement-HPVcannon' AND `weapons`.`name`='Cannon4AUTOMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Emplacement-PulseLaser' AND `weapons`.`name`='Laser2PULSEMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Emplacement-Rail2' AND `weapons`.`name`='RailGun2Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Emplacement-HvART-pit' AND `weapons`.`name`='Missile-HvyArt';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='WallTower-HvATrocket' AND `weapons`.`name`='Rocket-HvyA-T';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='WallTower-HPVcannon' AND `weapons`.`name`='Cannon4AUTOMk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='WallTower-Atmiss' AND `weapons`.`name`='Missile-A-T';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='WallTower-Rail3' AND `weapons`.`name`='RailGun3Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Emplacement-MortarPit02' AND `weapons`.`name`='Mortar2Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Emplacement-MdART-pit' AND `weapons`.`name`='Missile-MdArt';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Emplacement-Howitzer150' AND `weapons`.`name`='Howitzer150Mk1';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Pillbox-RotMG' AND `weapons`.`name`='MG4ROTARY-Pillbox';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='Sys-NEXUSLinkTOW' AND `weapons`.`name`='NEXUSlink';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='CO-Tower-HvATRkt' AND `weapons`.`name`='Rocket-HvyA-T';
INSERT INTO `structureweapons` SELECT `structures`.`id`, `weapons`.`id` FROM `structures`, `weapons` WHERE `structures`.`name`='A0BaBaMortarPit' AND `weapons`.`name`='Mortar1Mk1';

COMMIT TRANSACTION;
