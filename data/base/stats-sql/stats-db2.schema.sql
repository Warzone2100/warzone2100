-- This file is generated automatically, do not edit, change the source (../../../src/stats-db2.tpl) instead.

-- Elements common to all stats structures
CREATE TABLE `BASE` (
	-- Automatically generated ID to link the inheritance hierarchy.
	unique_inheritance_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,

	-- Unique language independant name that can be used to identify a specific
	-- stats instance
	pName TEXT NOT NULL UNIQUE
);

CREATE VIEW `BASES` AS SELECT
	-- Automatically generated ID to link the inheritance hierarchy.
	`BASE`.unique_inheritance_id AS unique_inheritance_id,
	-- Unique language independant name that can be used to identify a specific
	-- stats instance
	`BASE`.`pName` AS `pName`
	FROM `BASE`;

-- Stats common to all (droid?) components
CREATE TABLE `COMPONENT` (
	-- Automatically generated ID to link the inheritance hierarchy.
	unique_inheritance_id INTEGER PRIMARY KEY NOT NULL,

	-- Power required to build this component
	buildPower INTEGER NOT NULL,

	-- Build points (which are rate-limited in the construction units) required
	-- to build this component.
	buildPoints INTEGER NOT NULL,

	-- Weight of this component
	weight INTEGER NOT NULL,

	-- Body points of this component
	body INTEGER NOT NULL,

	-- Indicates whether this component is "designable" and can thus be used in
	-- the design screen.
	designable INTEGER NOT NULL,

	-- The "base" IMD model representing this component in 3D space.
	pIMD TEXT
);

CREATE VIEW `COMPONENTS` AS SELECT
	-- Automatically generated ID to link the inheritance hierarchy.
	`BASE`.unique_inheritance_id AS unique_inheritance_id,
	-- Unique language independant name that can be used to identify a specific
	-- stats instance
	`BASE`.`pName` AS `pName`,

	-- Power required to build this component
	`COMPONENT`.`buildPower` AS `buildPower`,

	-- Build points (which are rate-limited in the construction units) required
	-- to build this component.
	`COMPONENT`.`buildPoints` AS `buildPoints`,

	-- Weight of this component
	`COMPONENT`.`weight` AS `weight`,

	-- Body points of this component
	`COMPONENT`.`body` AS `body`,

	-- Indicates whether this component is "designable" and can thus be used in
	-- the design screen.
	`COMPONENT`.`designable` AS `designable`,

	-- The "base" IMD model representing this component in 3D space.
	`COMPONENT`.`pIMD` AS `pIMD`
	FROM `BASE` INNER JOIN `COMPONENT` ON `BASE`.`unique_inheritance_id` = `COMPONENT`.`unique_inheritance_id`;

CREATE TABLE `PROPULSION` (
	-- Automatically generated ID to link the inheritance hierarchy.
	unique_inheritance_id INTEGER PRIMARY KEY NOT NULL,

	-- Max speed for the droid
	maxSpeed INTEGER NOT NULL,

	-- Type of propulsion used - index into PropulsionTable
	propulsionType TEXT NOT NULL
);

CREATE VIEW `PROPULSIONS` AS SELECT
	-- Automatically generated ID to link the inheritance hierarchy.
	`BASE`.unique_inheritance_id AS unique_inheritance_id,
	-- Unique language independant name that can be used to identify a specific
	-- stats instance
	`BASE`.`pName` AS `pName`,

	-- Power required to build this component
	`COMPONENT`.`buildPower` AS `buildPower`,

	-- Build points (which are rate-limited in the construction units) required
	-- to build this component.
	`COMPONENT`.`buildPoints` AS `buildPoints`,

	-- Weight of this component
	`COMPONENT`.`weight` AS `weight`,

	-- Body points of this component
	`COMPONENT`.`body` AS `body`,

	-- Indicates whether this component is "designable" and can thus be used in
	-- the design screen.
	`COMPONENT`.`designable` AS `designable`,

	-- The "base" IMD model representing this component in 3D space.
	`COMPONENT`.`pIMD` AS `pIMD`,

	-- Max speed for the droid
	`PROPULSION`.`maxSpeed` AS `maxSpeed`,

	-- Type of propulsion used - index into PropulsionTable
	`PROPULSION`.`propulsionType` AS `propulsionType`
	FROM `BASE` INNER JOIN `COMPONENT` ON `BASE`.`unique_inheritance_id` = `COMPONENT`.`unique_inheritance_id` INNER JOIN `PROPULSION` ON `COMPONENT`.`unique_inheritance_id` = `PROPULSION`.`unique_inheritance_id`;

CREATE TABLE `SENSOR` (
	-- Automatically generated ID to link the inheritance hierarchy.
	unique_inheritance_id INTEGER PRIMARY KEY NOT NULL,

	-- Sensor range.
	range INTEGER NOT NULL,

	-- Sensor power (put against ecm power).
	power INTEGER NOT NULL,

	-- specifies whether the Sensor is default or for the Turret.
	location TEXT NOT NULL,

	-- used for combat
	type TEXT NOT NULL,

	-- Time delay before the associated weapon droids 'know' where the attack is
	-- from.
	time INTEGER NOT NULL,

	-- The turret mount to use.
	pMountGraphic TEXT
);

CREATE VIEW `SENSORS` AS SELECT
	-- Automatically generated ID to link the inheritance hierarchy.
	`BASE`.unique_inheritance_id AS unique_inheritance_id,
	-- Unique language independant name that can be used to identify a specific
	-- stats instance
	`BASE`.`pName` AS `pName`,

	-- Power required to build this component
	`COMPONENT`.`buildPower` AS `buildPower`,

	-- Build points (which are rate-limited in the construction units) required
	-- to build this component.
	`COMPONENT`.`buildPoints` AS `buildPoints`,

	-- Weight of this component
	`COMPONENT`.`weight` AS `weight`,

	-- Body points of this component
	`COMPONENT`.`body` AS `body`,

	-- Indicates whether this component is "designable" and can thus be used in
	-- the design screen.
	`COMPONENT`.`designable` AS `designable`,

	-- The "base" IMD model representing this component in 3D space.
	`COMPONENT`.`pIMD` AS `pIMD`,

	-- Sensor range.
	`SENSOR`.`range` AS `range`,

	-- Sensor power (put against ecm power).
	`SENSOR`.`power` AS `power`,

	-- specifies whether the Sensor is default or for the Turret.
	`SENSOR`.`location` AS `location`,

	-- used for combat
	`SENSOR`.`type` AS `type`,

	-- Time delay before the associated weapon droids 'know' where the attack is
	-- from.
	`SENSOR`.`time` AS `time`,

	-- The turret mount to use.
	`SENSOR`.`pMountGraphic` AS `pMountGraphic`
	FROM `BASE` INNER JOIN `COMPONENT` ON `BASE`.`unique_inheritance_id` = `COMPONENT`.`unique_inheritance_id` INNER JOIN `SENSOR` ON `COMPONENT`.`unique_inheritance_id` = `SENSOR`.`unique_inheritance_id`;

CREATE TABLE `ECM` (
	-- Automatically generated ID to link the inheritance hierarchy.
	unique_inheritance_id INTEGER PRIMARY KEY NOT NULL,

	-- ECM range.
	range INTEGER NOT NULL,

	-- ECM power (put against sensor power).
	power INTEGER NOT NULL,

	-- Specifies whether the ECM is default or for the Turret.
	location INTEGER NOT NULL,

	-- The turret mount to use.
	pMountGraphic TEXT
);

CREATE VIEW `ECMS` AS SELECT
	-- Automatically generated ID to link the inheritance hierarchy.
	`BASE`.unique_inheritance_id AS unique_inheritance_id,
	-- Unique language independant name that can be used to identify a specific
	-- stats instance
	`BASE`.`pName` AS `pName`,

	-- Power required to build this component
	`COMPONENT`.`buildPower` AS `buildPower`,

	-- Build points (which are rate-limited in the construction units) required
	-- to build this component.
	`COMPONENT`.`buildPoints` AS `buildPoints`,

	-- Weight of this component
	`COMPONENT`.`weight` AS `weight`,

	-- Body points of this component
	`COMPONENT`.`body` AS `body`,

	-- Indicates whether this component is "designable" and can thus be used in
	-- the design screen.
	`COMPONENT`.`designable` AS `designable`,

	-- The "base" IMD model representing this component in 3D space.
	`COMPONENT`.`pIMD` AS `pIMD`,

	-- ECM range.
	`ECM`.`range` AS `range`,

	-- ECM power (put against sensor power).
	`ECM`.`power` AS `power`,

	-- Specifies whether the ECM is default or for the Turret.
	`ECM`.`location` AS `location`,

	-- The turret mount to use.
	`ECM`.`pMountGraphic` AS `pMountGraphic`
	FROM `BASE` INNER JOIN `COMPONENT` ON `BASE`.`unique_inheritance_id` = `COMPONENT`.`unique_inheritance_id` INNER JOIN `ECM` ON `COMPONENT`.`unique_inheritance_id` = `ECM`.`unique_inheritance_id`;

CREATE TABLE `REPAIR` (
	-- Automatically generated ID to link the inheritance hierarchy.
	unique_inheritance_id INTEGER PRIMARY KEY NOT NULL,

	-- FIXME: UDWORD COMPONENT::body; doesn't exist for this component
	--%csv-file "repair.txt";
	-- How much damage is restored to Body Points and armour each Repair Cycle.
	repairPoints INTEGER NOT NULL,

	-- Whether armour can be repaired or not.
	repairArmour INTEGER NOT NULL,

	-- Specifies whether the Repair is default or for the Turret.
	location INTEGER NOT NULL,

	-- Time delay for repair cycle.
	time INTEGER NOT NULL,

	-- The turret mount to use.
	pMountGraphic TEXT
);

CREATE VIEW `REPAIRS` AS SELECT
	-- Automatically generated ID to link the inheritance hierarchy.
	`BASE`.unique_inheritance_id AS unique_inheritance_id,
	-- Unique language independant name that can be used to identify a specific
	-- stats instance
	`BASE`.`pName` AS `pName`,

	-- Power required to build this component
	`COMPONENT`.`buildPower` AS `buildPower`,

	-- Build points (which are rate-limited in the construction units) required
	-- to build this component.
	`COMPONENT`.`buildPoints` AS `buildPoints`,

	-- Weight of this component
	`COMPONENT`.`weight` AS `weight`,

	-- Body points of this component
	`COMPONENT`.`body` AS `body`,

	-- Indicates whether this component is "designable" and can thus be used in
	-- the design screen.
	`COMPONENT`.`designable` AS `designable`,

	-- The "base" IMD model representing this component in 3D space.
	`COMPONENT`.`pIMD` AS `pIMD`,

	-- FIXME: UDWORD COMPONENT::body; doesn't exist for this component
	--%csv-file "repair.txt";
	-- How much damage is restored to Body Points and armour each Repair Cycle.
	`REPAIR`.`repairPoints` AS `repairPoints`,

	-- Whether armour can be repaired or not.
	`REPAIR`.`repairArmour` AS `repairArmour`,

	-- Specifies whether the Repair is default or for the Turret.
	`REPAIR`.`location` AS `location`,

	-- Time delay for repair cycle.
	`REPAIR`.`time` AS `time`,

	-- The turret mount to use.
	`REPAIR`.`pMountGraphic` AS `pMountGraphic`
	FROM `BASE` INNER JOIN `COMPONENT` ON `BASE`.`unique_inheritance_id` = `COMPONENT`.`unique_inheritance_id` INNER JOIN `REPAIR` ON `COMPONENT`.`unique_inheritance_id` = `REPAIR`.`unique_inheritance_id`;

CREATE TABLE `WEAPON` (
	-- Automatically generated ID to link the inheritance hierarchy.
	unique_inheritance_id INTEGER PRIMARY KEY NOT NULL,

	-- WEAPON's CSV layout differs from the rest for this field
	-- Max distance to target for short range shot
	shortRange INTEGER NOT NULL,

	-- Max distance to target for long range shot
	longRange INTEGER NOT NULL,

	-- Min distance to target for shot
	minRange INTEGER NOT NULL,

	-- Chance to hit at short range
	shortHit INTEGER NOT NULL,

	-- Chance to hit at long range
	longHit INTEGER NOT NULL,

	-- Time between each weapon fire
	firePause INTEGER NOT NULL,

	-- The number of explosions per shot
	numExplosions INTEGER NOT NULL,

	-- The number of rounds per salvo(magazine)
	numRounds INTEGER NOT NULL,

	-- Time to reload the round of ammo (salvo fire)
	reloadTime INTEGER NOT NULL,

	-- How much damage the weapon causes
	damage INTEGER NOT NULL,

	-- Basic blast radius of weapon
	radius INTEGER NOT NULL,

	-- Chance to hit in the blast radius
	radiusHit INTEGER NOT NULL,

	-- Damage done in the blast radius
	radiusDamage INTEGER NOT NULL,

	-- How long the round burns
	incenTime INTEGER NOT NULL,

	-- Damage done each burn cycle
	incenDamage INTEGER NOT NULL,

	-- Burn radius of the round
	incenRadius INTEGER NOT NULL,

	-- speed ammo travels at
	flightSpeed INTEGER NOT NULL,

	-- how high the ammo travels for indirect fire
	indirectHeight INTEGER NOT NULL,

	-- indicates whether the droid has to stop before firing
	fireOnMove TEXT NOT NULL,

	-- the class of weapon
	weaponClass TEXT NOT NULL,

	-- the subclass to which the weapon belongs
	weaponSubClass TEXT NOT NULL,

	-- which projectile model to use for the bullet
	movementModel TEXT NOT NULL,

	-- which type of warhead is associated with the weapon
	weaponEffect TEXT NOT NULL,

	-- used to compare with weight to see if recoils or not
	recoilValue INTEGER NOT NULL,

	-- amount the weapon(turret) can rotate 0 = none
	rotate INTEGER NOT NULL,

	-- max amount the turret can be elevated up
	maxElevation INTEGER NOT NULL,

	-- min amount the turret can be elevated down
	minElevation INTEGER NOT NULL,

	-- flag to make the (explosion) effect face the player when drawn
	facePlayer INTEGER NOT NULL,

	-- flag to make the inflight effect face the player when drawn
	faceInFlight INTEGER NOT NULL,

	-- size of the effect 100 = normal, 50 = half etc
	effectSize INTEGER NOT NULL,

	-- flag to indicate whether the effect lights up the world
	lightWorld INTEGER NOT NULL,

	-- indicates how good in the air - SHOOT_ON_GROUND, SHOOT_IN_AIR or both
	surfaceToAir INTEGER NOT NULL,

	-- number of attack runs a VTOL droid can do with this weapon
	vtolAttackRuns INTEGER NOT NULL,

	-- flag to indicate whether pentrate droid or not
	penetrate INTEGER NOT NULL,

	-- Graphics control stats
	-- How long a direct fire weapon is visible. Measured in 1/100 sec.
	directLife INTEGER NOT NULL,

	-- How long a blast radius is visible
	radiusLife INTEGER NOT NULL,

	-- Graphics used for the weapon
	-- The turret mount to use
	pMountGraphic TEXT,

	-- The muzzle flash
	pMuzzleGraphic TEXT NOT NULL,

	-- The ammo in flight
	pInFlightGraphic TEXT NOT NULL,

	-- The ammo hitting a target
	pTargetHitGraphic TEXT NOT NULL,

	-- The ammo missing a target
	pTargetMissGraphic TEXT NOT NULL,

	-- The ammo hitting water
	pWaterHitGraphic TEXT NOT NULL,

	-- The trail used for in flight
	pTrailGraphic TEXT,

);

CREATE VIEW `WEAPONS` AS SELECT
	-- Automatically generated ID to link the inheritance hierarchy.
	`BASE`.unique_inheritance_id AS unique_inheritance_id,
	-- Unique language independant name that can be used to identify a specific
	-- stats instance
	`BASE`.`pName` AS `pName`,

	-- Power required to build this component
	`COMPONENT`.`buildPower` AS `buildPower`,

	-- Build points (which are rate-limited in the construction units) required
	-- to build this component.
	`COMPONENT`.`buildPoints` AS `buildPoints`,

	-- Weight of this component
	`COMPONENT`.`weight` AS `weight`,

	-- Body points of this component
	`COMPONENT`.`body` AS `body`,

	-- Indicates whether this component is "designable" and can thus be used in
	-- the design screen.
	`COMPONENT`.`designable` AS `designable`,

	-- The "base" IMD model representing this component in 3D space.
	`COMPONENT`.`pIMD` AS `pIMD`,

	-- WEAPON's CSV layout differs from the rest for this field
	-- Max distance to target for short range shot
	`WEAPON`.`shortRange` AS `shortRange`,

	-- Max distance to target for long range shot
	`WEAPON`.`longRange` AS `longRange`,

	-- Min distance to target for shot
	`WEAPON`.`minRange` AS `minRange`,

	-- Chance to hit at short range
	`WEAPON`.`shortHit` AS `shortHit`,

	-- Chance to hit at long range
	`WEAPON`.`longHit` AS `longHit`,

	-- Time between each weapon fire
	`WEAPON`.`firePause` AS `firePause`,

	-- The number of explosions per shot
	`WEAPON`.`numExplosions` AS `numExplosions`,

	-- The number of rounds per salvo(magazine)
	`WEAPON`.`numRounds` AS `numRounds`,

	-- Time to reload the round of ammo (salvo fire)
	`WEAPON`.`reloadTime` AS `reloadTime`,

	-- How much damage the weapon causes
	`WEAPON`.`damage` AS `damage`,

	-- Basic blast radius of weapon
	`WEAPON`.`radius` AS `radius`,

	-- Chance to hit in the blast radius
	`WEAPON`.`radiusHit` AS `radiusHit`,

	-- Damage done in the blast radius
	`WEAPON`.`radiusDamage` AS `radiusDamage`,

	-- How long the round burns
	`WEAPON`.`incenTime` AS `incenTime`,

	-- Damage done each burn cycle
	`WEAPON`.`incenDamage` AS `incenDamage`,

	-- Burn radius of the round
	`WEAPON`.`incenRadius` AS `incenRadius`,

	-- speed ammo travels at
	`WEAPON`.`flightSpeed` AS `flightSpeed`,

	-- how high the ammo travels for indirect fire
	`WEAPON`.`indirectHeight` AS `indirectHeight`,

	-- indicates whether the droid has to stop before firing
	`WEAPON`.`fireOnMove` AS `fireOnMove`,

	-- the class of weapon
	`WEAPON`.`weaponClass` AS `weaponClass`,

	-- the subclass to which the weapon belongs
	`WEAPON`.`weaponSubClass` AS `weaponSubClass`,

	-- which projectile model to use for the bullet
	`WEAPON`.`movementModel` AS `movementModel`,

	-- which type of warhead is associated with the weapon
	`WEAPON`.`weaponEffect` AS `weaponEffect`,

	-- used to compare with weight to see if recoils or not
	`WEAPON`.`recoilValue` AS `recoilValue`,

	-- amount the weapon(turret) can rotate 0 = none
	`WEAPON`.`rotate` AS `rotate`,

	-- max amount the turret can be elevated up
	`WEAPON`.`maxElevation` AS `maxElevation`,

	-- min amount the turret can be elevated down
	`WEAPON`.`minElevation` AS `minElevation`,

	-- flag to make the (explosion) effect face the player when drawn
	`WEAPON`.`facePlayer` AS `facePlayer`,

	-- flag to make the inflight effect face the player when drawn
	`WEAPON`.`faceInFlight` AS `faceInFlight`,

	-- size of the effect 100 = normal, 50 = half etc
	`WEAPON`.`effectSize` AS `effectSize`,

	-- flag to indicate whether the effect lights up the world
	`WEAPON`.`lightWorld` AS `lightWorld`,

	-- indicates how good in the air - SHOOT_ON_GROUND, SHOOT_IN_AIR or both
	`WEAPON`.`surfaceToAir` AS `surfaceToAir`,

	-- number of attack runs a VTOL droid can do with this weapon
	`WEAPON`.`vtolAttackRuns` AS `vtolAttackRuns`,

	-- flag to indicate whether pentrate droid or not
	`WEAPON`.`penetrate` AS `penetrate`,

	-- Graphics control stats
	-- How long a direct fire weapon is visible. Measured in 1/100 sec.
	`WEAPON`.`directLife` AS `directLife`,

	-- How long a blast radius is visible
	`WEAPON`.`radiusLife` AS `radiusLife`,

	-- Graphics used for the weapon
	-- The turret mount to use
	`WEAPON`.`pMountGraphic` AS `pMountGraphic`,

	-- The muzzle flash
	`WEAPON`.`pMuzzleGraphic` AS `pMuzzleGraphic`,

	-- The ammo in flight
	`WEAPON`.`pInFlightGraphic` AS `pInFlightGraphic`,

	-- The ammo hitting a target
	`WEAPON`.`pTargetHitGraphic` AS `pTargetHitGraphic`,

	-- The ammo missing a target
	`WEAPON`.`pTargetMissGraphic` AS `pTargetMissGraphic`,

	-- The ammo hitting water
	`WEAPON`.`pWaterHitGraphic` AS `pWaterHitGraphic`,

	-- The trail used for in flight
	`WEAPON`.`pTrailGraphic` AS `pTrailGraphic`
	FROM `BASE` INNER JOIN `COMPONENT` ON `BASE`.`unique_inheritance_id` = `COMPONENT`.`unique_inheritance_id` INNER JOIN `WEAPON` ON `COMPONENT`.`unique_inheritance_id` = `WEAPON`.`unique_inheritance_id`;

CREATE TABLE `BRAIN` (
	-- Automatically generated ID to link the inheritance hierarchy.
	unique_inheritance_id INTEGER PRIMARY KEY NOT NULL,

	-- FIXME: UDWORD COMPONENT::body; doesn't exist for this component
	-- FIXME: bool COMPONENT::designable; doesn't exist for this component
	--%csv-file "brain.txt";
	-- Program capacity
	progCap INTEGER NOT NULL,

	-- Weapon stats associated with this brain - for Command Droids
	psWeaponStat INTEGER NOT NULL
);

CREATE VIEW `BRAINS` AS SELECT
	-- Automatically generated ID to link the inheritance hierarchy.
	`BASE`.unique_inheritance_id AS unique_inheritance_id,
	-- Unique language independant name that can be used to identify a specific
	-- stats instance
	`BASE`.`pName` AS `pName`,

	-- Power required to build this component
	`COMPONENT`.`buildPower` AS `buildPower`,

	-- Build points (which are rate-limited in the construction units) required
	-- to build this component.
	`COMPONENT`.`buildPoints` AS `buildPoints`,

	-- Weight of this component
	`COMPONENT`.`weight` AS `weight`,

	-- Body points of this component
	`COMPONENT`.`body` AS `body`,

	-- Indicates whether this component is "designable" and can thus be used in
	-- the design screen.
	`COMPONENT`.`designable` AS `designable`,

	-- The "base" IMD model representing this component in 3D space.
	`COMPONENT`.`pIMD` AS `pIMD`,

	-- FIXME: UDWORD COMPONENT::body; doesn't exist for this component
	-- FIXME: bool COMPONENT::designable; doesn't exist for this component
	--%csv-file "brain.txt";
	-- Program capacity
	`BRAIN`.`progCap` AS `progCap`,

	-- Weapon stats associated with this brain - for Command Droids
	`BRAIN`.`psWeaponStat` AS `psWeaponStat`
	FROM `BASE` INNER JOIN `COMPONENT` ON `BASE`.`unique_inheritance_id` = `COMPONENT`.`unique_inheritance_id` INNER JOIN `BRAIN` ON `COMPONENT`.`unique_inheritance_id` = `BRAIN`.`unique_inheritance_id`;

CREATE TABLE `CONSTRUCT` (
	-- Automatically generated ID to link the inheritance hierarchy.
	unique_inheritance_id INTEGER PRIMARY KEY NOT NULL,

	-- The number of points contributed each cycle
	constructPoints INTEGER NOT NULL,

	-- The turret mount to use
	pMountGraphic TEXT
);

CREATE VIEW `CONSTRUCTS` AS SELECT
	-- Automatically generated ID to link the inheritance hierarchy.
	`BASE`.unique_inheritance_id AS unique_inheritance_id,
	-- Unique language independant name that can be used to identify a specific
	-- stats instance
	`BASE`.`pName` AS `pName`,

	-- Power required to build this component
	`COMPONENT`.`buildPower` AS `buildPower`,

	-- Build points (which are rate-limited in the construction units) required
	-- to build this component.
	`COMPONENT`.`buildPoints` AS `buildPoints`,

	-- Weight of this component
	`COMPONENT`.`weight` AS `weight`,

	-- Body points of this component
	`COMPONENT`.`body` AS `body`,

	-- Indicates whether this component is "designable" and can thus be used in
	-- the design screen.
	`COMPONENT`.`designable` AS `designable`,

	-- The "base" IMD model representing this component in 3D space.
	`COMPONENT`.`pIMD` AS `pIMD`,

	-- The number of points contributed each cycle
	`CONSTRUCT`.`constructPoints` AS `constructPoints`,

	-- The turret mount to use
	`CONSTRUCT`.`pMountGraphic` AS `pMountGraphic`
	FROM `BASE` INNER JOIN `COMPONENT` ON `BASE`.`unique_inheritance_id` = `COMPONENT`.`unique_inheritance_id` INNER JOIN `CONSTRUCT` ON `COMPONENT`.`unique_inheritance_id` = `CONSTRUCT`.`unique_inheritance_id`;

