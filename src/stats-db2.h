/* This file is generated automatically, do not edit, change the source (stats-db2.tpl) instead. */

#ifndef __INCLUDED_DB_TEMPLATE_SCHEMA_STRUCTDEF_STATS_DB2_H_H__
#define __INCLUDED_DB_TEMPLATE_SCHEMA_STRUCTDEF_STATS_DB2_H_H__

#line 1 "stats-db2.tpl.struct.h"
#include "lib/ivis_common/ivisdef.h"
#line 9 "stats-db2.h"

/**
 * if any types are added BEFORE 'COMP_BODY' - then Save/Load Game will have to
 * be  altered since it loops through the components from 1->MAX_COMP
 */
typedef enum COMPONENT_TYPE
{
	COMP_UNKNOWN,

	COMP_BODY,

	COMP_BRAIN,

	COMP_PROPULSION,

	COMP_REPAIRUNIT,

	COMP_ECM,

	COMP_SENSOR,

	COMP_CONSTRUCT,

	COMP_WEAPON,

	/**
	 * The number of enumerators in this enum.
	 */
	COMP_NUMCOMPONENTS,
} COMPONENT_TYPE;

/**
 * LOC used for holding locations for Sensors and ECM's
 */
typedef enum LOC
{
	LOC_DEFAULT,

	LOC_TURRET,
} LOC;

/**
 * SIZE used for specifying body size
 */
typedef enum BODY_SIZE
{
	SIZE_LIGHT,

	SIZE_MEDIUM,

	SIZE_HEAVY,

	SIZE_SUPER_HEAVY,
} BODY_SIZE;

/**
 * only using KINETIC and HEAT for now
 */
typedef enum WEAPON_CLASS
{
	/**
	 * Bullets, etc.
	 */
	WC_KINETIC,

	/**
	 * Rockets, etc. - classed as KINETIC now to save space in DROID
	 *EXPLOSIVE
	 * Laser, etc.
	 */
	WC_HEAT,

	/**
	 * The number of enumerators in this enum.
	 */
	WC_NUM_WEAPON_CLASSES,
} WEAPON_CLASS;

/**
 * weapon subclasses used to define which weapons are affected by weapon upgrade
 * functions
 *
 * Watermelon:added a new subclass to do some tests
 */
typedef enum WEAPON_SUBCLASS
{
	WSC_MGUN,

	WSC_CANNON,

	/**
	 *ARTILLARY
	 */
	WSC_MORTARS,

	WSC_MISSILE,

	WSC_ROCKET,

	WSC_ENERGY,

	WSC_GAUSS,

	WSC_FLAME,

	/**
	 *CLOSECOMBAT
	 */
	WSC_HOWITZERS,

	WSC_ELECTRONIC,

	WSC_AAGUN,

	WSC_SLOWMISSILE,

	WSC_SLOWROCKET,

	WSC_LAS_SAT,

	WSC_BOMB,

	WSC_COMMAND,

	WSC_EMP,

	/**
	 * Counter missile
	 */
	WSC_COUNTER,

	/**
	 * The number of enumerators in this enum.
	 */
	WSC_NUM_WEAPON_SUBCLASSES,
} WEAPON_SUBCLASS;

/**
 * Used to define which projectile model to use for the weapon.
 */
typedef enum MOVEMENT_MODEL
{
	MM_DIRECT,

	MM_INDIRECT,

	MM_HOMINGDIRECT,

	MM_HOMINGINDIRECT,

	MM_ERRATICDIRECT,

	MM_SWEEP,

	/**
	 * The number of enumerators in this enum.
	 */
	NUM_MOVEMENT_MODEL,
} MOVEMENT_MODEL;

/**
 * Used to modify the damage to a propuslion type (or structure) based on
 * weapon.
 */
typedef enum WEAPON_EFFECT
{
	WE_ANTI_PERSONNEL,

	WE_ANTI_TANK,

	WE_BUNKER_BUSTER,

	WE_ARTILLERY_ROUND,

	WE_FLAMER,

	WE_ANTI_AIRCRAFT,

	/**
	 * The number of enumerators in this enum.
	 */
	WE_NUMEFFECTS,
} WEAPON_EFFECT;

/**
 * Sides used for droid impact
 */
typedef enum HIT_SIDE
{
	HIT_SIDE_FRONT,

	HIT_SIDE_REAR,

	HIT_SIDE_LEFT,

	HIT_SIDE_RIGHT,

	HIT_SIDE_TOP,

	HIT_SIDE_BOTTOM,

	/**
	 * The number of enumerators in this enum.
	 */
	NUM_HIT_SIDES,
} HIT_SIDE;

/**
 * Defines the left and right sides for propulsion IMDs
 */
typedef enum PROP_SIDE
{
	LEFT_PROP,

	RIGHT_PROP,

	/**
	 * The number of enumerators in this enum.
	 */
	NUM_PROP_SIDES,
} PROP_SIDE;

typedef enum PROPULSION_TYPE
{
	PROPULSION_TYPE_WHEELED,

	PROPULSION_TYPE_TRACKED,

	PROPULSION_TYPE_LEGGED,

	PROPULSION_TYPE_HOVER,

	PROPULSION_TYPE_SKI,

	PROPULSION_TYPE_LIFT,

	PROPULSION_TYPE_PROPELLOR,

	PROPULSION_TYPE_HALF_TRACKED,

	PROPULSION_TYPE_JUMP,

	/**
	 * The number of enumerators in this enum.
	 */
	PROPULSION_TYPE_NUM,
} PROPULSION_TYPE;

typedef enum SENSOR_TYPE
{
	STANDARD_SENSOR,

	INDIRECT_CB_SENSOR,

	VTOL_CB_SENSOR,

	VTOL_INTERCEPT_SENSOR,

	/**
	 * Works as all of the above together! - new for updates
	 */
	SUPER_SENSOR,
} SENSOR_TYPE;

typedef enum FIREONMOVE
{
	/**
	 * no capability - droid must stop
	 */
	FOM_NO,

	/**
	 * partial capability - droid has 50% chance to hit
	 */
	FOM_PARTIAL,

	/**
	 * full capability - droid fires normally on move
	 */
	FOM_YES,
} FIREONMOVE;

typedef enum TRAVEL_MEDIUM
{
	GROUND,

	AIR,
} TRAVEL_MEDIUM;

/**
 * Elements common to all stats structures
 */
typedef struct BASE_STATS
{
	/**
	 * Unique ID of the item
	 */
	UDWORD ref;

	/**
	 * Unique language independant name that can be used to identify a specific
	 * stats instance
	 *
	 * Unique across all instances
	 */
	char*            pName;
} BASE_STATS;

#define STATS_BASE \
	UDWORD ref; \
	char*            pName

/**
 * Stats common to all (droid?) components
 */
typedef struct COMPONENT_STATS
{
	/* BEGIN of inherited "BASE" definition */
	/**
	 * Unique ID of the item
	 */
	UDWORD ref;

	/**
	 * Unique language independant name that can be used to identify a specific
	 * stats instance
	 *
	 * Unique across all instances
	 */
	char*            pName;
	/* END of inherited "BASE" definition */
	/**
	 * Power required to build this component
	 */
	UDWORD           buildPower;

	/**
	 * Build points (which are rate-limited in the construction units) required
	 * to build this component.
	 */
	UDWORD           buildPoints;

	/**
	 * Weight of this component
	 */
	UDWORD           weight;

	/**
	 * Body points of this component
	 */
	UDWORD           body;

	/**
	 * Indicates whether this component is "designable" and can thus be used in
	 * the design screen.
	 */
	bool             designable;

	/**
	 * The "base" IMD model representing this component in 3D space.
	 *
	 * This field is optional and can be NULL to indicate that it has no value
	 */
	iIMDShape*       pIMD;
} COMPONENT_STATS;

#define STATS_COMPONENT \
	UDWORD ref; \
	char*            pName; \
	UDWORD           buildPower; \
	UDWORD           buildPoints; \
	UDWORD           weight; \
	UDWORD           body; \
	bool             designable; \
	iIMDShape*       pIMD

typedef struct PROPULSION_STATS
{
	/* BEGIN of inherited "COMPONENT" definition */
	/* BEGIN of inherited "BASE" definition */
	/**
	 * Unique ID of the item
	 */
	UDWORD ref;

	/**
	 * Unique language independant name that can be used to identify a specific
	 * stats instance
	 *
	 * Unique across all instances
	 */
	char*            pName;
	/* END of inherited "BASE" definition */
	/**
	 * Power required to build this component
	 */
	UDWORD           buildPower;

	/**
	 * Build points (which are rate-limited in the construction units) required
	 * to build this component.
	 */
	UDWORD           buildPoints;

	/**
	 * Weight of this component
	 */
	UDWORD           weight;

	/**
	 * Body points of this component
	 */
	UDWORD           body;

	/**
	 * Indicates whether this component is "designable" and can thus be used in
	 * the design screen.
	 */
	bool             designable;

	/**
	 * The "base" IMD model representing this component in 3D space.
	 *
	 * This field is optional and can be NULL to indicate that it has no value
	 */
	iIMDShape*       pIMD;
	/* END of inherited "COMPONENT" definition */
	/**
	 * Max speed for the droid
	 */
	UDWORD           maxSpeed;

	/**
	 * Type of propulsion used - index into PropulsionTable
	 */
	PROPULSION_TYPE propulsionType;
} PROPULSION_STATS;

typedef struct SENSOR_STATS
{
	/* BEGIN of inherited "COMPONENT" definition */
	/* BEGIN of inherited "BASE" definition */
	/**
	 * Unique ID of the item
	 */
	UDWORD ref;

	/**
	 * Unique language independant name that can be used to identify a specific
	 * stats instance
	 *
	 * Unique across all instances
	 */
	char*            pName;
	/* END of inherited "BASE" definition */
	/**
	 * Power required to build this component
	 */
	UDWORD           buildPower;

	/**
	 * Build points (which are rate-limited in the construction units) required
	 * to build this component.
	 */
	UDWORD           buildPoints;

	/**
	 * Weight of this component
	 */
	UDWORD           weight;

	/**
	 * Body points of this component
	 */
	UDWORD           body;

	/**
	 * Indicates whether this component is "designable" and can thus be used in
	 * the design screen.
	 */
	bool             designable;

	/**
	 * The "base" IMD model representing this component in 3D space.
	 *
	 * This field is optional and can be NULL to indicate that it has no value
	 */
	iIMDShape*       pIMD;
	/* END of inherited "COMPONENT" definition */
	/**
	 * Sensor range.
	 */
	UDWORD           range;

	/**
	 * Sensor power (put against ecm power).
	 */
	UDWORD           power;

	/**
	 * specifies whether the Sensor is default or for the Turret.
	 */
	UDWORD           location;

	/**
	 * used for combat
	 */
	SENSOR_TYPE type;

	/**
	 * Time delay before the associated weapon droids 'know' where the attack is
	 * from.
	 */
	UDWORD           time;

	/**
	 * The turret mount to use.
	 */
	iIMDShape*       pMountGraphic;
} SENSOR_STATS;

typedef struct ECM_STATS
{
	/* BEGIN of inherited "COMPONENT" definition */
	/* BEGIN of inherited "BASE" definition */
	/**
	 * Unique ID of the item
	 */
	UDWORD ref;

	/**
	 * Unique language independant name that can be used to identify a specific
	 * stats instance
	 *
	 * Unique across all instances
	 */
	char*            pName;
	/* END of inherited "BASE" definition */
	/**
	 * Power required to build this component
	 */
	UDWORD           buildPower;

	/**
	 * Build points (which are rate-limited in the construction units) required
	 * to build this component.
	 */
	UDWORD           buildPoints;

	/**
	 * Weight of this component
	 */
	UDWORD           weight;

	/**
	 * Body points of this component
	 */
	UDWORD           body;

	/**
	 * Indicates whether this component is "designable" and can thus be used in
	 * the design screen.
	 */
	bool             designable;

	/**
	 * The "base" IMD model representing this component in 3D space.
	 *
	 * This field is optional and can be NULL to indicate that it has no value
	 */
	iIMDShape*       pIMD;
	/* END of inherited "COMPONENT" definition */
	/**
	 * ECM range.
	 */
	UDWORD           range;

	/**
	 * ECM power (put against sensor power).
	 */
	UDWORD           power;

	/**
	 * Specifies whether the ECM is default or for the Turret.
	 */
	UDWORD           location;

	/**
	 * The turret mount to use.
	 */
	iIMDShape*       pMountGraphic;
} ECM_STATS;

typedef struct REPAIR_STATS
{
	/* BEGIN of inherited "COMPONENT" definition */
	/* BEGIN of inherited "BASE" definition */
	/**
	 * Unique ID of the item
	 */
	UDWORD ref;

	/**
	 * Unique language independant name that can be used to identify a specific
	 * stats instance
	 *
	 * Unique across all instances
	 */
	char*            pName;
	/* END of inherited "BASE" definition */
	/**
	 * Power required to build this component
	 */
	UDWORD           buildPower;

	/**
	 * Build points (which are rate-limited in the construction units) required
	 * to build this component.
	 */
	UDWORD           buildPoints;

	/**
	 * Weight of this component
	 */
	UDWORD           weight;

	/**
	 * Body points of this component
	 */
	UDWORD           body;

	/**
	 * Indicates whether this component is "designable" and can thus be used in
	 * the design screen.
	 */
	bool             designable;

	/**
	 * The "base" IMD model representing this component in 3D space.
	 *
	 * This field is optional and can be NULL to indicate that it has no value
	 */
	iIMDShape*       pIMD;
	/* END of inherited "COMPONENT" definition */
	/**
	 * How much damage is restored to Body Points and armour each Repair Cycle.
	 */
	UDWORD           repairPoints;

	/**
	 * Whether armour can be repaired or not.
	 */
	bool             repairArmour;

	/**
	 * Specifies whether the Repair is default or for the Turret.
	 */
	UDWORD           location;

	/**
	 * Time delay for repair cycle.
	 */
	UDWORD           time;

	/**
	 * The turret mount to use.
	 */
	iIMDShape*       pMountGraphic;
} REPAIR_STATS;

typedef struct WEAPON_STATS
{
	/* BEGIN of inherited "COMPONENT" definition */
	/* BEGIN of inherited "BASE" definition */
	/**
	 * Unique ID of the item
	 */
	UDWORD ref;

	/**
	 * Unique language independant name that can be used to identify a specific
	 * stats instance
	 *
	 * Unique across all instances
	 */
	char*            pName;
	/* END of inherited "BASE" definition */
	/**
	 * Power required to build this component
	 */
	UDWORD           buildPower;

	/**
	 * Build points (which are rate-limited in the construction units) required
	 * to build this component.
	 */
	UDWORD           buildPoints;

	/**
	 * Weight of this component
	 */
	UDWORD           weight;

	/**
	 * Body points of this component
	 */
	UDWORD           body;

	/**
	 * Indicates whether this component is "designable" and can thus be used in
	 * the design screen.
	 */
	bool             designable;

	/**
	 * The "base" IMD model representing this component in 3D space.
	 *
	 * This field is optional and can be NULL to indicate that it has no value
	 */
	iIMDShape*       pIMD;
	/* END of inherited "COMPONENT" definition */
	/**
	 * Max distance to target for short range shot
	 */
	UDWORD           shortRange;

	/**
	 * Max distance to target for long range shot
	 */
	UDWORD           longRange;

	/**
	 * Min distance to target for shot
	 */
	UDWORD           minRange;

	/**
	 * Chance to hit at short range
	 */
	UDWORD           shortHit;

	/**
	 * Chance to hit at long range
	 */
	UDWORD           longHit;

	/**
	 * Time between each weapon fire
	 */
	UDWORD           firePause;

	/**
	 * The number of explosions per shot
	 */
	UDWORD           numExplosions;

	/**
	 * The number of rounds per salvo(magazine)
	 */
	UBYTE           numRounds;

	/**
	 * Time to reload the round of ammo (salvo fire)
	 */
	UDWORD           reloadTime;

	/**
	 * How much damage the weapon causes
	 */
	UDWORD           damage;

	/**
	 * Basic blast radius of weapon
	 */
	UDWORD           radius;

	/**
	 * Chance to hit in the blast radius
	 */
	UDWORD           radiusHit;

	/**
	 * Damage done in the blast radius
	 */
	UDWORD           radiusDamage;

	/**
	 * How long the round burns
	 */
	UDWORD           incenTime;

	/**
	 * Damage done each burn cycle
	 */
	UDWORD           incenDamage;

	/**
	 * Burn radius of the round
	 */
	UDWORD           incenRadius;

	/**
	 * speed ammo travels at
	 */
	UDWORD           flightSpeed;

	/**
	 * how high the ammo travels for indirect fire
	 */
	UDWORD           indirectHeight;

	/**
	 * indicates whether the droid has to stop before firing
	 */
	FIREONMOVE fireOnMove;

	/**
	 * the class of weapon
	 */
	WEAPON_CLASS weaponClass;

	/**
	 * the subclass to which the weapon belongs
	 */
	WEAPON_SUBCLASS weaponSubClass;

	/**
	 * which projectile model to use for the bullet
	 */
	MOVEMENT_MODEL movementModel;

	/**
	 * which type of warhead is associated with the weapon
	 */
	WEAPON_EFFECT weaponEffect;

	/**
	 * used to compare with weight to see if recoils or not
	 */
	UDWORD           recoilValue;

	/**
	 * amount the weapon(turret) can rotate 0 = none
	 */
	UBYTE           rotate;

	/**
	 * max amount the turret can be elevated up
	 */
	UBYTE           maxElevation;

	/**
	 * min amount the turret can be elevated down
	 */
	SBYTE           minElevation;

	/**
	 * flag to make the (explosion) effect face the player when drawn
	 */
	UBYTE           facePlayer;

	/**
	 * flag to make the inflight effect face the player when drawn
	 */
	UBYTE           faceInFlight;

	/**
	 * size of the effect 100 = normal, 50 = half etc
	 */
	UBYTE           effectSize;

	/**
	 * flag to indicate whether the effect lights up the world
	 */
	bool             lightWorld;

	/**
	 * indicates how good in the air - SHOOT_ON_GROUND, SHOOT_IN_AIR or both
	 */
	UBYTE           surfaceToAir;

	/**
	 * number of attack runs a VTOL droid can do with this weapon
	 */
	UBYTE           vtolAttackRuns;

	/**
	 * flag to indicate whether pentrate droid or not
	 */
	bool             penetrate;

	/**
	 * Graphics control stats
	 * How long a direct fire weapon is visible. Measured in 1/100 sec.
	 */
	UDWORD           directLife;

	/**
	 * How long a blast radius is visible
	 */
	UDWORD           radiusLife;

	/**
	 * Graphics used for the weapon
	 * The turret mount to use
	 */
	iIMDShape*       pMountGraphic;

	/**
	 * The muzzle flash
	 */
	iIMDShape*       pMuzzleGraphic;

	/**
	 * The ammo in flight
	 */
	iIMDShape*       pInFlightGraphic;

	/**
	 * The ammo hitting a target
	 */
	iIMDShape*       pTargetHitGraphic;

	/**
	 * The ammo missing a target
	 */
	iIMDShape*       pTargetMissGraphic;

	/**
	 * The ammo hitting water
	 */
	iIMDShape*       pWaterHitGraphic;

	/**
	 * The trail used for in flight
	 */
	iIMDShape*       pTrailGraphic;

	/**
	 * Audio
	 */
	SDWORD           iAudioFireID;

	SDWORD           iAudioImpactID;
} WEAPON_STATS;

typedef struct BRAIN_STATS
{
	/* BEGIN of inherited "COMPONENT" definition */
	/* BEGIN of inherited "BASE" definition */
	/**
	 * Unique ID of the item
	 */
	UDWORD ref;

	/**
	 * Unique language independant name that can be used to identify a specific
	 * stats instance
	 *
	 * Unique across all instances
	 */
	char*            pName;
	/* END of inherited "BASE" definition */
	/**
	 * Power required to build this component
	 */
	UDWORD           buildPower;

	/**
	 * Build points (which are rate-limited in the construction units) required
	 * to build this component.
	 */
	UDWORD           buildPoints;

	/**
	 * Weight of this component
	 */
	UDWORD           weight;

	/**
	 * Body points of this component
	 */
	UDWORD           body;

	/**
	 * Indicates whether this component is "designable" and can thus be used in
	 * the design screen.
	 */
	bool             designable;

	/**
	 * The "base" IMD model representing this component in 3D space.
	 *
	 * This field is optional and can be NULL to indicate that it has no value
	 */
	iIMDShape*       pIMD;
	/* END of inherited "COMPONENT" definition */
	/**
	 * Program capacity
	 */
	UDWORD           progCap;

	/**
	 * Weapon stats associated with this brain - for Command Droids
	 */
	WEAPON_STATS* psWeaponStat;
} BRAIN_STATS;

typedef struct CONSTRUCT_STATS
{
	/* BEGIN of inherited "COMPONENT" definition */
	/* BEGIN of inherited "BASE" definition */
	/**
	 * Unique ID of the item
	 */
	UDWORD ref;

	/**
	 * Unique language independant name that can be used to identify a specific
	 * stats instance
	 *
	 * Unique across all instances
	 */
	char*            pName;
	/* END of inherited "BASE" definition */
	/**
	 * Power required to build this component
	 */
	UDWORD           buildPower;

	/**
	 * Build points (which are rate-limited in the construction units) required
	 * to build this component.
	 */
	UDWORD           buildPoints;

	/**
	 * Weight of this component
	 */
	UDWORD           weight;

	/**
	 * Body points of this component
	 */
	UDWORD           body;

	/**
	 * Indicates whether this component is "designable" and can thus be used in
	 * the design screen.
	 */
	bool             designable;

	/**
	 * The "base" IMD model representing this component in 3D space.
	 *
	 * This field is optional and can be NULL to indicate that it has no value
	 */
	iIMDShape*       pIMD;
	/* END of inherited "COMPONENT" definition */
	/**
	 * The number of points contributed each cycle
	 */
	UDWORD           constructPoints;

	/**
	 * The turret mount to use
	 */
	iIMDShape*       pMountGraphic;
} CONSTRUCT_STATS;

#endif // __INCLUDED_DB_TEMPLATE_SCHEMA_STRUCTDEF_STATS_DB2_TPL_H__
