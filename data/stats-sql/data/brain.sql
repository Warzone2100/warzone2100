BEGIN TRANSACTION;

-- Data for table `brain`

INSERT INTO `brain` SELECT NULL, 'ZNULLBRAIN', 'Level All', 0, 0, 0, 0, 0, `weapons`.`id`, 0 FROM `weapons` WHERE `weapons`.`name`='ZNULLWEAPON';
INSERT INTO `brain` SELECT NULL, 'CommandBrain01' ,'Level All', 1, 1, 1, 1, 1, `weapons`.`id`, 0 FROM `weapons` WHERE `weapons`.`name`='CommandTurret1';

COMMIT TRANSACTION;
