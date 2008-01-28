/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/*
 * StatsDef.h
 *
 * Structure definitions for the stats system
 *
 */
#ifndef _statsdef_h
#define _statsdef_h

#include "lib/ivis_common/ivisdef.h"

/* Elements common to all stats structures */
#define STATS_BASE \
	UDWORD			ref;			/* Unique ID of the item */ \
	char			*pName			/* pointer to the text id name (i.e. short language-independant name) */

/* Stats common to all stats structs */
typedef struct _base_stats
{
	STATS_BASE;
} BASE_STATS;

/*
if any types are added BEFORE 'COMP_BODY' - then Save/Load Game will have to be
altered since it loops through the components from 1->MAX_COMP
*/
typedef enum _component_type
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

	COMP_NUMCOMPONENTS
} COMPONENT_TYPE;

#define NULL_COMP	(-1)

//ALL components and structures and research topics have a tech level to which they belong
typedef enum _tech_level
{
	TECH_LEVEL_ONE,
	TECH_LEVEL_TWO,
	TECH_LEVEL_THREE,
	TECH_LEVEL_ONE_TWO,
	TECH_LEVEL_TWO_THREE,
    TECH_LEVEL_ALL,

	MAX_TECH_LEVELS
} TECH_LEVEL;

/***********************************************************************************
 *
 * Stats structures type definitions
 */

/* Stats common to all droid components */
#define COMPONENT_STATS \
	STATS_BASE;							/* Basic stats */ \
	TECH_LEVEL		techLevel;			/* technology level of the component */ \
	UDWORD			buildPower;			/* Power required to build the component */ \
	UDWORD			buildPoints;		/* Time required to build the component */ \
	UDWORD			weight;				/* Component's weight */ \
	UDWORD			hitPoints;			/* Component's hit points - SEEMS TO BE UNUSED */ \
	UDWORD			systemPoints;		/* Space the component takes in the droid - SEEMS TO BE UNUSED */ \
	UDWORD			body;				/* Component's body points */ \
	BOOL			design;				/* flag to indicate whether this component can*/ \
										/* be used in the design screen*/ \
	iIMDShape		*pIMD				/* The IMD to draw for this component */

/* Stats common to all components */
typedef struct _comp_base_stats
{
	COMPONENT_STATS;
} COMP_BASE_STATS;

/* LOC used for holding locations for Sensors and ECM's*/
typedef enum _loc
{
	LOC_DEFAULT,
	LOC_TURRET
} LOC;

/* SIZE used for specifying body size */
typedef enum _size
{
	SIZE_LIGHT,
	SIZE_MEDIUM,
	SIZE_HEAVY,

	SIZE_SUPER_HEAVY
} BODY_SIZE;

//only using KINETIC and HEAT for now
typedef enum _weapon_class
{
	WC_KINETIC,		//bullets etc
	//WC_EXPLOSIVE,	//rockets etc - classed as WC_KINETIC now to save space in struct _droid AB 25/11/98
	WC_HEAT,		//laser etc
	//WC_MISC,		//others we haven't thought of! - classed as WC_HEAT now to save space in struct _droid AB 25/11/98

	NUM_WEAPON_CLASS
} WEAPON_CLASS;

// weapon subclasses used to define which weapons are affected by weapon upgrade functions
// Watermelon:added a new subclass to do some tests
typedef enum _weapon_subclass
{
	WSC_MGUN,
	WSC_CANNON,
	//WSC_ARTILLARY,
    WSC_MORTARS,
	WSC_MISSILE,
	WSC_ROCKET,
	WSC_ENERGY,
	WSC_GAUSS,
	WSC_FLAME,
	//WSC_CLOSECOMBAT,
    WSC_HOWITZERS,
	WSC_ELECTRONIC,
    WSC_AAGUN,
    WSC_SLOWMISSILE,
    WSC_SLOWROCKET,
    WSC_LAS_SAT,
    WSC_BOMB,
    WSC_COMMAND,
    WSC_EMP,
	WSC_COUNTER,

	NUM_WEAPON_SUBCLASS,

	INVALID_SUBCLASS
} WEAPON_SUBCLASS;

// used to define which projectile model to use for the weapon
typedef enum _movement_model
{
	MM_DIRECT,
	MM_INDIRECT,
	MM_HOMINGDIRECT,
	MM_HOMINGINDIRECT,
	MM_ERRATICDIRECT,
	MM_SWEEP,

	NUM_MOVEMENT_MODEL,

	INVALID_MOVEMENT
} MOVEMENT_MODEL;

//used to modify the damage to a propuslion type (or structure) based on weapon
typedef enum _weapon_effect
{
	WE_ANTI_PERSONNEL,
	WE_ANTI_TANK,
	WE_BUNKER_BUSTER,
	WE_ARTILLERY_ROUND,
	WE_FLAMER,
    WE_ANTI_AIRCRAFT,

	WE_NUMEFFECTS,

	INVALID_WEAPON_EFFECT
} WEAPON_EFFECT;

#define    SHOOT_ON_GROUND  0x01
#define    SHOOT_IN_AIR     0x02


//Sides used for droid impact
typedef enum _hit_sides
{
	HIT_SIDE_FRONT,
	HIT_SIDE_REAR,
	HIT_SIDE_LEFT,
	HIT_SIDE_RIGHT,
	HIT_SIDE_TOP,
	HIT_SIDE_BOTTOM,
	NUM_HIT_SIDES		//should be last one
} HIT_SIDE;

//Special angles representing top or bottom hit
#define HIT_ANGLE_TOP 361
#define HIT_ANGLE_BOTTOM 362

typedef struct _body_stats
{
	/* common stats - N.B. system points for a body
	 * are the limit for all other components
	 */
	COMPONENT_STATS;

	UBYTE		size;				// How big the body is - affects how hit
	UDWORD		weaponSlots;		// The number of weapon slots on the body
	UDWORD		armourValue[NUM_HIT_SIDES][NUM_WEAPON_CLASS];	// A measure of how much protection the armour provides
												// cross-ref with the weapon types
	// A measure of how much energy the power plant outputs
	UDWORD		powerOutput;		// this is the engine output of the body
	iIMDShape	**ppIMDList;			//list of IMDs to use for propulsion unit - up to numPropulsionStats
    iIMDShape   *pFlameIMD;     //pointer to which flame graphic to use - for VTOLs only at the moment
} BODY_STATS;

typedef struct _brain_stats
{
	/* Common stats */
	COMPONENT_STATS;

	UDWORD		progCap;			// Program capacity
	struct _weapon_stats	*psWeaponStat;	//weapon stats associated with this brain - for Command Droids
} BRAIN_STATS;

//defines the left and right sides for propulsion IMDs
typedef enum _prop_side
{
	LEFT_PROP,
	RIGHT_PROP,

	NUM_PROP_SIDES
} PROP_SIDE;

typedef enum _propulsion_type
{
	WHEELED,
	TRACKED,
	LEGGED,
	HOVER,
	SKI,
	LIFT,
	PROPELLOR,
	HALF_TRACKED,
	JUMP,

	NUM_PROP_TYPES,

	INVALID_PROP_TYPE
} PROPULSION_TYPE;

typedef struct _propulsion_stats
{
	/* Common stats */
	COMPONENT_STATS;

	UDWORD		maxSpeed;			// Max speed for the droid
	UBYTE		propulsionType;		// Type of propulsion used - index
									// into PropulsionTable
} PROPULSION_STATS;

typedef enum _sensor_type
{
	STANDARD_SENSOR,
	INDIRECT_CB_SENSOR,
	VTOL_CB_SENSOR,
	VTOL_INTERCEPT_SENSOR,
    SUPER_SENSOR,           //works as all of the above together! - new for updates - added 11/06/99 AB

} SENSOR_TYPE;


typedef struct _sensor_stats
{
	/* Common stats */
	COMPONENT_STATS;

	UDWORD		range;				// Sensor range
	UDWORD		power;				// Sensor power (put against ecm power)
	UDWORD		location;			// specifies whether the Sensor is default or for the Turret
	SENSOR_TYPE	type;				// used for combat
	UDWORD		time;				//time delay before associated weapon droids 'know' where the
									//attack is from
	iIMDShape	*pMountGraphic;		// The turret mount to use
} SENSOR_STATS;

typedef struct _ecm_stats
{
	/* Common stats */
	COMPONENT_STATS;

	UDWORD		range;				// ECM range
	UDWORD		power;				// ECM power (put against sensor power)
	UDWORD		location;			// specifies whether the ECM is default or for the Turret
	iIMDShape	*pMountGraphic;		// The turret mount to use
} ECM_STATS;

typedef struct _repair_stats
{
	/* Common stats */
	COMPONENT_STATS;

	UDWORD				repairPoints;		// How much damage is restored to Body Points
											// and armour each Repair Cycle
	BOOL				repairArmour;		// whether armour can be repaired or not
	UDWORD				location;			// specifies whether the Repair is default or for the Turret
	UDWORD				time;				// time delay for repair cycle
	iIMDShape	*pMountGraphic;		// The turret mount to use
} REPAIR_STATS;

typedef enum _fireonmove
{
	FOM_NO,			//no capability - droid must stop
	FOM_PARTIAL,	//partial capability - droid has 50% chance to hit
	FOM_YES,		//full capability - droid fires normally on move

} FIREONMOVE;

typedef struct _weapon_stats
{
	/* Common stats */
	COMPONENT_STATS;

	UDWORD			shortRange;			// Max distance to target for short range shot
	UDWORD			longRange;			// Max distance to target for long range shot
	UDWORD			minRange;			// Min distance to target for shot
	UDWORD			shortHit;			// Chance to hit at short range
	UDWORD			longHit;			// Chance to hit at long range
	UDWORD			firePause;			// Time between each weapon fire
	UDWORD			numExplosions;		// The number of explosions per shot
	UBYTE			numRounds;			// The number of rounds per salvo(magazine)
	UDWORD			reloadTime;			// Time to reload the round of ammo (salvo fire)
	UDWORD			damage;				// How much damage the weapon causes
	UDWORD			radius;				// Basic blast radius of weapon
	UDWORD			radiusHit;			// Chance to hit in the blast radius
	UDWORD			radiusDamage;		// Damage done in the blast radius
	UDWORD			incenTime;			// How long the round burns
	UDWORD			incenDamage;		// Damage done each burn cycle
	UDWORD			incenRadius;		// Burn radius of the round
	UDWORD			flightSpeed;		// speed ammo travels at
	UDWORD			indirectHeight;		// how high the ammo travels for indirect fire
	FIREONMOVE		fireOnMove;			// indicates whether the droid has to stop before firing
	WEAPON_CLASS	weaponClass;		// the class of weapon - see enum WEAPON_CLASS
	WEAPON_SUBCLASS weaponSubClass;		// the subclass to which the weapon belongs - see
										// enum WEAPON_SUBCLASS
	MOVEMENT_MODEL	movementModel;		// which projectile model to use for the bullet
	WEAPON_EFFECT	weaponEffect;		// which type of warhead is associated with the weapon
	UDWORD			recoilValue;		// used to compare with weight to see if recoils or not
	UBYTE			rotate;				// amount the weapon(turret) can rotate 0 = none
	UBYTE			maxElevation;		// max amount the turret can be elevated up
	SBYTE			minElevation;		// min amount the turret can be elevated down
	UBYTE			facePlayer;			// flag to make the (explosion) effect face the player when drawn
	UBYTE			faceInFlight;		// flag to make the inflight effect face the player when drawn
	UBYTE			effectSize;			// size of the effect 100 = normal, 50 = half etc
	BOOL			lightWorld;			// flag to indicate whether the effect lights up the world
	UBYTE			surfaceToAir;		// indicates how good in the air - SHOOT_ON_GROUND, SHOOT_IN_AIR or both
	UBYTE			vtolAttackRuns;		// number of attack runs a VTOL droid can do with this weapon
	BOOL			penetrate;			// flag to indicate whether pentrate droid or not

	/* Graphics control stats */
	UDWORD			directLife;			// How long a direct fire weapon is visible
										// Measured in 1/100 sec.
	UDWORD			radiusLife;			// How long a blast radius is visible

	/* Graphics used for the weapon */
	iIMDShape		*pMountGraphic;		// The turret mount to use
	iIMDShape		*pMuzzleGraphic;	// The muzzle flash
	iIMDShape		*pInFlightGraphic;	// The ammo in flight
	iIMDShape		*pTargetHitGraphic;	// The ammo hitting a target
	iIMDShape		*pTargetMissGraphic;// The ammo missing a target
	iIMDShape		*pWaterHitGraphic;	// The ammo hitting water
	iIMDShape		*pTrailGraphic;		// The trail used for in flight

	/* Audio */
	SDWORD			iAudioFireID;
	SDWORD			iAudioImpactID;
} WEAPON_STATS;

typedef struct _construct_stats
{
	/* Common stats */
	COMPONENT_STATS;

	UDWORD		constructPoints;	/*The number of points contributed each cycle*/
	iIMDShape	*pMountGraphic;		// The turret mount to use
} CONSTRUCT_STATS;

/************************************************************************************
*	Additional stats tables
************************************************************************************/

typedef enum _travel_medium
{
	GROUND,
	AIR
} TRAVEL_MEDIUM;

typedef struct _propulsion_types
{
	UWORD				powerRatioMult;		// Multiplier for the calculated power ratio of
											// the droid
	UDWORD				travel;				// Which medium the propulsion travels in
	SWORD				startID;			//sound to play when this prop type starts
	SWORD				idleID;				//sound to play when this prop type is idle
	SWORD				moveOffID;			//sound to link moveID and idleID
	SWORD				moveID;				//sound to play when this prop type is moving
	SWORD				hissID;				//sound to link moveID and idleID
	SWORD				shutDownID;			//sound to play when this prop type shuts down

} PROPULSION_TYPES;

typedef struct _terrain_table
{
	UDWORD	speedFactor;		// factor to multiply the speed by depending on the
								// method of propulsion and the terrain type - to be
								// divided by 100 before use
} TERRAIN_TABLE;

typedef struct _special_ability
{
	char *pName;				// Text name of the component
} SPECIAL_ABILITY;

typedef UWORD WEAPON_MODIFIER;

/* weapon stats which can be upgraded by research*/
typedef struct _weapon_upgrade
{
	UBYTE			firePause;
	UWORD			shortHit;
	UWORD			longHit;
	UWORD			damage;
	UWORD			radiusDamage;
	UWORD			incenDamage;
	UWORD			radiusHit;
} WEAPON_UPGRADE;

/*sensor stats which can be upgraded by research*/
typedef struct _sensor_upgrade
{
	UWORD			power;
	UWORD			range;
} SENSOR_UPGRADE;

/*ECM stats which can be upgraded by research*/
typedef struct _ecm_upgrade
{
	UWORD			power;
} ECM_UPGRADE;

/*repair stats which can be upgraded by research*/
typedef struct _repair_upgrade
{
	UWORD			repairPoints;
} REPAIR_UPGRADE;

/*constructor stats which can be upgraded by research*/
typedef struct _constructor_upgrade
{
	UWORD			constructPoints;
} CONSTRUCTOR_UPGRADE;

/*body stats which can be upgraded by research*/
typedef struct _body_upgrade
{
	UWORD			powerOutput;
	UWORD			body;
	UWORD           armourValue[NUM_WEAPON_CLASS];
} BODY_UPGRADE;

#endif
