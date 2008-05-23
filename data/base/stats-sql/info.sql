-- Displays a list of structures and their associated weapons
CREATE VIEW `structureweapons_list` AS
SELECT `structures`.`name` AS `structure`,
       `weapons`.`name` AS `weapon`
FROM `structureweapons`,
     `structures`,
     `weapons`
WHERE `structures`.`id`=`structureweapons`.`structure`
  AND `weapons`.`id`=`structureweapons`.`weapon`;

-- Displays all brains and their associated weapons
CREATE VIEW `brain_list` AS
SELECT `brain`.`id` AS `id`,
       `brain`.`name` AS `name`,
       `brain`.`techlevel` AS `techlevel`,
       `brain`.`buildPower` AS `buildPower`,
       `brain`.`buildPoints` AS `buildPoints`,
       `brain`.`weight` AS `weight`,
       `brain`.`hitpoints` AS `hitpoints`,
       `brain`.`systempoints` AS `systempoints`,
       `weapons`.`name` AS `weapon`,
       `brain`.`program_capacity` AS `program_capacity`
FROM `brain`,
     `weapons`
WHERE `weapons`.`id`=`brain`.`weapon`;

-- Displays all structures and their associated ECMs and sensors
CREATE VIEW `structures_list` AS
SELECT `structures`.`id` AS `id`,
       `structures`.`name` AS `name`,
       `structures`.`type` AS `type`,
       `structures`.`techlevel` AS `techlevel`,
       `structures`.`strength` AS `strength`,
       `structures`.`terrain_type` AS `terrain_type`,
       `structures`.`baseWidth` AS `baseWidth`,
       `structures`.`baseBreadth` AS `baseBreadth`,
       `structures`.`foundationType` AS `foundationType`,
       `structures`.`buildPoints` AS `buildPoints`,
       `structures`.`height` AS `height`,
       `structures`.`armourValue` AS `armourValue`,
       `structures`.`bodyPoints` AS `bodyPoints`,
       `structures`.`repairSystem` AS `repairSystem`,
       `structures`.`powerToBuild` AS `powerToBuild`,
       `structures`.`resistance` AS `resistance`,
       `structures`.`sizeModifier` AS `sizeModifier`,
       `ecm`.`name` AS `ecm`,
       `sensor`.`name` AS `sensor`,
       `structures`.`GfxFile` AS `GfxFile`,
       `structures`.`BaseGfxFile` AS `BaseGfxFile`,
       `structures`.`num_functions` AS `num_functions`,
       `structures`.`num_weapons` AS `num_weapons`
FROM `structures`,
     `ecm`,
     `sensor`
WHERE `ecm`.`id`=`structures`.`ecm`
  AND `sensor`.`id`=`structures`.`sensor`;
