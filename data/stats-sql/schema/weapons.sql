-- Table structure for table `weapons`

CREATE TABLE `weapons` (
  id                    INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  name                  TEXT    NOT NULL, -- Text id name (short language independant name)
  techlevel             TEXT    NOT NULL, -- Technology level of this component
  buildPower            NUMERIC NOT NULL, -- Power required to build this component
  buildPoints           NUMERIC NOT NULL, -- Time required to build this component
  weight                NUMERIC NOT NULL, -- Component's weight (mass?)
  hitpoints             NUMERIC NOT NULL, -- Component's hitpoints - SEEMS TO BE UNUSED
  systempoints          NUMERIC NOT NULL, -- Space the component takes in the droid - SEEMS TO BE UNUSED
  body                  NUMERIC NOT NULL, -- Component's body points
  GfxFile               TEXT,             -- The IMD to draw for this component
  mountGfx              TEXT,             -- The turret mount to use
  muzzleGfx             TEXT,             -- The muzzle flash
  flightGfx             TEXT,             -- The ammo in flight
  hitGfx                TEXT,             -- The ammo hitting a target
  missGfx               TEXT,             -- The ammo missing a target
  waterGfx              TEXT,             -- The ammo hitting water
  trailGfx              TEXT,             -- The trail used for in flight
  short_range           NUMERIC NOT NULL, -- Max distance to target for short range shot
  long_range            NUMERIC NOT NULL, -- Max distance to target for long range shot
  short_range_accuracy  NUMERIC NOT NULL, -- Chance to hit at short range
  long_range_accuracy   NUMERIC NOT NULL, -- Chance to hit at long range
  firePause             NUMERIC NOT NULL, -- Time between each weapon fire
  numExplosions         INTEGER NOT NULL, -- The number of explosions per shot
  rounds_per_salvo      INTEGER NOT NULL, -- The number of rounds per salvo(magazine)
  reload_time_per_salvo NUMERIC NOT NULL, -- Time to reload the round of ammo (salvo fire)
  damage                NUMERIC NOT NULL, -- How much damage the weapon causes
  radius                NUMERIC NOT NULL, -- Basic blast radius of weapon
  radiusHit             NUMERIC NOT NULL, -- Chance to hit in the blast radius
  radiusDamage          NUMERIC NOT NULL, -- Damage done in the blast radius
  incenTime             NUMERIC NOT NULL, -- How long the round burns
  incenDamage           NUMERIC NOT NULL, -- Damage done each burn cycle
  incenRadius           NUMERIC NOT NULL, -- Burn radius of the round
  directLife            NUMERIC NOT NULL, -- How long a direct fire weapon is visible. Measured in 1/100 sec.
  radiusLife            NUMERIC NOT NULL, -- How long a blast radius is visible
  flightSpeed           NUMERIC NOT NULL, -- speed ammo travels at
  indirectHeight        NUMERIC NOT NULL, -- how high the ammo travels for indirect fire
  fireOnMove            TEXT    NOT NULL, -- indicates whether the droid has to stop before firing
  weaponClass           TEXT    NOT NULL, -- the class of weapon
  weaponSubClass        TEXT    NOT NULL, -- the subclass to which the weapon belongs
  movement              TEXT    NOT NULL, -- which projectile model to use for the bullet
  weaponEffect          TEXT    NOT NULL, -- which type of warhead is associated with the weapon
  rotate                NUMERIC NOT NULL, -- amount the weapon(turret) can rotate 0 = none
  maxElevation          NUMERIC NOT NULL, -- max amount the turret can be elevated up
  minElevation          NUMERIC NOT NULL, -- min amount the turret can be elevated down
  facePlayer            TEXT    NOT NULL, -- flag to make the (explosion) effect face the player when drawn
  faceInFlight          TEXT    NOT NULL, -- flag to make the inflight effect face the player when drawn
  recoilValue           NUMERIC NOT NULL, -- used to compare with weight to see if recoils or not
  minRange              NUMERIC NOT NULL, -- Min distance to target for shot
  lightWorld            TEXT    NOT NULL, -- flag to indicate whether the effect lights up the world
  effectSize            NUMERIC NOT NULL, -- size of the effect 100 = normal, 50 = half etc
  surfaceToAir          NUMERIC NOT NULL, -- indicates how good in the air - SHOOT_ON_GROUND, SHOOT_IN_AIR or both
  numAttackRuns         NUMERIC NOT NULL, -- number of attack runs a VTOL droid can do with this weapon
  designable            NUMERIC NOT NULL, -- flag to indicate whether this component can be used in the design screen
  penetrate             NUMERIC NOT NULL  -- flag to indicate whether pentrate droid or not
);
