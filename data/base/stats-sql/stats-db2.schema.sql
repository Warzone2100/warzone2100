-- This file is generated automatically, do not edit, change the source (../../../src/stats-db2.tpl) instead.

-- Elements common to all stats structures
CREATE TABLE `BASE` (
	-- Automatically generated ID to link the inheritance hierarchy.
	unique_inheritance_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,

	-- Unique language independant name that can be used to identify a specific
	-- stats instance
	pName TEXT NOT NULL UNIQUE
);

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

CREATE TABLE `PROPULSION` (
	-- Automatically generated ID to link the inheritance hierarchy.
	unique_inheritance_id INTEGER PRIMARY KEY NOT NULL,

	-- Max speed for the droid
	maxSpeed INTEGER NOT NULL,

	-- Type of propulsion used - index into PropulsionTable
	propulsionType INTEGER NOT NULL
);

CREATE TABLE `SENSOR` (
	-- Automatically generated ID to link the inheritance hierarchy.
	unique_inheritance_id INTEGER PRIMARY KEY NOT NULL,

	-- Sensor range.
	range INTEGER NOT NULL,

	-- Sensor power (put against ecm power).
	power INTEGER NOT NULL,

	-- specifies whether the Sensor is default or for the Turret.
	location INTEGER NOT NULL,

	-- used for combat
	type INTEGER NOT NULL,

	-- Time delay before the associated weapon droids 'know' where the attack is
	-- from.
	time INTEGER NOT NULL,

	-- The turret mount to use.
	pMountGraphic TEXT
);

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

CREATE TABLE `REPAIR` (
	-- Automatically generated ID to link the inheritance hierarchy.
	unique_inheritance_id INTEGER PRIMARY KEY NOT NULL,

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

CREATE TABLE `WEAPON` (
	-- Automatically generated ID to link the inheritance hierarchy.
	unique_inheritance_id INTEGER PRIMARY KEY NOT NULL,

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
	fireOnMove INTEGER NOT NULL,

	-- the class of weapon
	weaponClass INTEGER NOT NULL,

	-- the subclass to which the weapon belongs
	weaponSubClass INTEGER NOT NULL,

	-- which projectile model to use for the bullet
	movementModel INTEGER NOT NULL,

	-- which type of warhead is associated with the weapon
	weaponEffect INTEGER NOT NULL,

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

	-- Audio
	iAudioFireID INTEGER NOT NULL,

	iAudioImpactID INTEGER NOT NULL
);

CREATE TABLE `BRAIN` (
	-- Automatically generated ID to link the inheritance hierarchy.
	unique_inheritance_id INTEGER PRIMARY KEY NOT NULL,

	-- Program capacity
	progCap INTEGER NOT NULL,

	-- Weapon stats associated with this brain - for Command Droids
	psWeaponStat INTEGER NOT NULL
);

CREATE TABLE `CONSTRUCT` (
	-- Automatically generated ID to link the inheritance hierarchy.
	unique_inheritance_id INTEGER PRIMARY KEY NOT NULL,

	-- The number of points contributed each cycle
	constructPoints INTEGER NOT NULL,

	-- The turret mount to use
	pMountGraphic TEXT
);

