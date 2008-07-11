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

#endif // __INCLUDED_DB_TEMPLATE_SCHEMA_STRUCTDEF_STATS_DB2_TPL_H__
