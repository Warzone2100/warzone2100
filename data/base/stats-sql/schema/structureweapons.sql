-- Table structure for table `structureweapons`

CREATE TABLE `structureweapons` (
  structure             INTEGER NOT NULL, -- Reference to `structures`.`id`
  weapon                INTEGER NOT NULL, -- Reference to `weapons`.`id`
  UNIQUE(structure,weapon)
);
