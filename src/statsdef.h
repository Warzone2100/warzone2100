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
/** \file
 *  Definitions for the stats system.
 */

#ifndef __INCLUDED_STATSDEF_H__
#define __INCLUDED_STATSDEF_H__

// Automatically generated file, which is intended to eventually replace this one
#include "stats-db2.h"

#include "lib/ivis_common/ivisdef.h"

/* Elements common to all stats structures */
#define STATS_BASE \
	UDWORD          ref;    /**< Unique ID of the item */ \
	char*           pName   /**< pointer to the text id name (i.e. short language-independant name) */

/* Stats common to all stats structs */
typedef struct _base_stats
{
	STATS_BASE;
} BASE_STATS;

#define NULL_COMP (-1)

/*
 * Stats structures type definitions
 */

/* Stats common to all droid components */
#define COMPONENT_STATS \
	STATS_BASE;                             /**< Basic stats */ \
	UDWORD          buildPower;             /**< Power required to build the component */ \
	UDWORD          buildPoints;            /**< Time required to build the component */ \
	UDWORD          weight;                 /**< Component's weight */ \
	UDWORD          body;                   /**< Component's body points */ \
	BOOL            design;                 /**< flag to indicate whether this component can be used in the design screen */ \
	iIMDShape*      pIMD                    /**< The IMD to draw for this component */

/* Stats common to all components */
typedef struct _comp_base_stats
{
	COMPONENT_STATS;
} COMP_BASE_STATS;

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
	NUM_HIT_SIDES   // should be the last one
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

	UBYTE           size;           ///< How big the body is - affects how hit
	UDWORD          weaponSlots;    ///< The number of weapon slots on the body
	UDWORD          armourValue[NUM_HIT_SIDES][WC_NUM_WEAPON_CLASSES];   ///< A measure of how much protection the armour provides. Cross referenced with the weapon types.

	// A measure of how much energy the power plant outputs
	UDWORD          powerOutput;    ///< this is the engine output of the body
	iIMDShape**     ppIMDList;      ///< list of IMDs to use for propulsion unit - up to numPropulsionStats
	iIMDShape*      pFlameIMD;      ///< pointer to which flame graphic to use - for VTOLs only at the moment
} BODY_STATS;

typedef struct _brain_stats
{
	/* Common stats */
	COMPONENT_STATS;

	UDWORD          progCap;                ///< Program capacity
	struct _weapon_stats* psWeaponStat;     ///< weapon stats associated with this brain - for Command Droids
} BRAIN_STATS;

//defines the left and right sides for propulsion IMDs
typedef enum _prop_side
{
	LEFT_PROP,
	RIGHT_PROP,

	NUM_PROP_SIDES
} PROP_SIDE;

typedef struct _propulsion_stats
{
	/* Common stats */
	COMPONENT_STATS;

	UDWORD          maxSpeed;       ///< Max speed for the droid
	PROPULSION_TYPE propulsionType; ///< Type of propulsion used - index into PropulsionTable
} PROPULSION_STATS;

typedef enum _sensor_type
{
	STANDARD_SENSOR,
	INDIRECT_CB_SENSOR,
	VTOL_CB_SENSOR,
	VTOL_INTERCEPT_SENSOR,
	SUPER_SENSOR,           ///< works as all of the above together! - new for updates
} SENSOR_TYPE;


typedef struct _sensor_stats
{
	/* Common stats */
	COMPONENT_STATS;

	UDWORD          range;                  ///< Sensor range
	UDWORD          power;                  ///< Sensor power (put against ecm power)
	UDWORD          location;               ///< specifies whether the Sensor is default or for the Turret
	SENSOR_TYPE     type;                   ///< used for combat
	UDWORD          time;                   ///< time delay before associated weapon droids 'know' where the attack is from
	iIMDShape*      pMountGraphic;          ///< The turret mount to use
} SENSOR_STATS;

typedef struct _ecm_stats
{
	/* Common stats */
	COMPONENT_STATS;

	UDWORD          range;                  ///< ECM range
	UDWORD          power;                  ///< ECM power (put against sensor power)
	UDWORD          location;               ///< specifies whether the ECM is default or for the Turret
	iIMDShape*      pMountGraphic;          ///< The turret mount to use
} ECM_STATS;

typedef struct _repair_stats
{
	/* Common stats */
	COMPONENT_STATS;

	UDWORD          repairPoints;           ///< How much damage is restored to Body Points and armour each Repair Cycle
	BOOL            repairArmour;           ///< whether armour can be repaired or not
	UDWORD          location;               ///< specifies whether the Repair is default or for the Turret
	UDWORD          time;                   ///< time delay for repair cycle
	iIMDShape*      pMountGraphic;          ///< The turret mount to use
} REPAIR_STATS;

typedef struct _weapon_stats
{
	/* Common stats */
	COMPONENT_STATS;

	UDWORD          shortRange;             ///< Max distance to target for short range shot
	UDWORD          longRange;              ///< Max distance to target for long range shot
	UDWORD          minRange;               ///< Min distance to target for shot
	UDWORD          shortHit;               ///< Chance to hit at short range
	UDWORD          longHit;                ///< Chance to hit at long range
	UDWORD          firePause;              ///< Time between each weapon fire
	UDWORD          numExplosions;          ///< The number of explosions per shot
	UBYTE           numRounds;              ///< The number of rounds per salvo(magazine)
	UDWORD          reloadTime;             ///< Time to reload the round of ammo (salvo fire)
	UDWORD          damage;                 ///< How much damage the weapon causes
	UDWORD          radius;                 ///< Basic blast radius of weapon
	UDWORD          radiusHit;              ///< Chance to hit in the blast radius
	UDWORD          radiusDamage;           ///< Damage done in the blast radius
	UDWORD          incenTime;              ///< How long the round burns
	UDWORD          incenDamage;            ///< Damage done each burn cycle
	UDWORD          incenRadius;            ///< Burn radius of the round
	UDWORD          flightSpeed;            ///< speed ammo travels at
	UDWORD          indirectHeight;         ///< how high the ammo travels for indirect fire
	FIREONMOVE      fireOnMove;             ///< indicates whether the droid has to stop before firing
	WEAPON_CLASS    weaponClass;            ///< the class of weapon
	WEAPON_SUBCLASS weaponSubClass;         ///< the subclass to which the weapon belongs

	MOVEMENT_MODEL  movementModel;          ///< which projectile model to use for the bullet
	WEAPON_EFFECT   weaponEffect;           ///< which type of warhead is associated with the weapon
	UDWORD          recoilValue;            ///< used to compare with weight to see if recoils or not
	UBYTE           rotate;                 ///< amount the weapon(turret) can rotate 0 = none
	UBYTE           maxElevation;           ///< max amount the turret can be elevated up
	SBYTE           minElevation;           ///< min amount the turret can be elevated down
	UBYTE           facePlayer;             ///< flag to make the (explosion) effect face the player when drawn
	UBYTE           faceInFlight;           ///< flag to make the inflight effect face the player when drawn
	UBYTE           effectSize;             ///< size of the effect 100 = normal, 50 = half etc
	BOOL            lightWorld;             ///< flag to indicate whether the effect lights up the world
	UBYTE           surfaceToAir;           ///< indicates how good in the air - SHOOT_ON_GROUND, SHOOT_IN_AIR or both
	UBYTE           vtolAttackRuns;         ///< number of attack runs a VTOL droid can do with this weapon
	BOOL            penetrate;              ///< flag to indicate whether pentrate droid or not

	/* Graphics control stats */
	UDWORD          directLife;             ///< How long a direct fire weapon is visible. Measured in 1/100 sec.
	UDWORD          radiusLife;             ///< How long a blast radius is visible

	/* Graphics used for the weapon */
	iIMDShape*       pMountGraphic;         ///< The turret mount to use
	iIMDShape*       pMuzzleGraphic;        ///< The muzzle flash
	iIMDShape*       pInFlightGraphic;      ///< The ammo in flight
	iIMDShape*       pTargetHitGraphic;     ///< The ammo hitting a target
	iIMDShape*       pTargetMissGraphic;    ///< The ammo missing a target
	iIMDShape*       pWaterHitGraphic;      ///< The ammo hitting water
	iIMDShape*       pTrailGraphic;         ///< The trail used for in flight

	/* Audio */
	SDWORD          iAudioFireID;
	SDWORD          iAudioImpactID;
} WEAPON_STATS;

typedef struct _construct_stats
{
	/* Common stats */
	COMPONENT_STATS;

	UDWORD          constructPoints;        ///< The number of points contributed each cycle
	iIMDShape*      pMountGraphic;          ///< The turret mount to use
} CONSTRUCT_STATS;

/************************************************************************************
 * Additional stats tables
 ************************************************************************************/

typedef struct _propulsion_types
{
	UWORD           powerRatioMult;         ///< Multiplier for the calculated power ratio of the droid
	UDWORD          travel;                 ///< Which medium the propulsion travels in
	SWORD           startID;                ///< sound to play when this prop type starts
	SWORD           idleID;                 ///< sound to play when this prop type is idle
	SWORD           moveOffID;              ///< sound to link moveID and idleID
	SWORD           moveID;                 ///< sound to play when this prop type is moving
	SWORD           hissID;                 ///< sound to link moveID and idleID
	SWORD           shutDownID;             ///< sound to play when this prop type shuts down
} PROPULSION_TYPES;

typedef struct _terrain_table
{
	UDWORD          speedFactor;    ///< factor to multiply the speed by depending on the method of propulsion and the terrain type - to be divided by 100 before use
} TERRAIN_TABLE;

typedef struct _special_ability
{
	char*       pName;      ///< Text name of the component
} SPECIAL_ABILITY;

typedef UWORD WEAPON_MODIFIER;

/* weapon stats which can be upgraded by research*/
typedef struct _weapon_upgrade
{
	UBYTE           firePause;
	UWORD           shortHit;
	UWORD           longHit;
	UWORD           damage;
	UWORD           radiusDamage;
	UWORD           incenDamage;
	UWORD           radiusHit;
} WEAPON_UPGRADE;

/*sensor stats which can be upgraded by research*/
typedef struct _sensor_upgrade
{
	UWORD           power;
	UWORD           range;
} SENSOR_UPGRADE;

/*ECM stats which can be upgraded by research*/
typedef struct _ecm_upgrade
{
	UWORD           power;
	UDWORD		range;
} ECM_UPGRADE;

/*repair stats which can be upgraded by research*/
typedef struct _repair_upgrade
{
	UWORD           repairPoints;
} REPAIR_UPGRADE;

/*constructor stats which can be upgraded by research*/
typedef struct _constructor_upgrade
{
	UWORD           constructPoints;
} CONSTRUCTOR_UPGRADE;

/*body stats which can be upgraded by research*/
typedef struct _body_upgrade
{
	UWORD           powerOutput;
	UWORD           body;
	UWORD           armourValue[WC_NUM_WEAPON_CLASSES];
} BODY_UPGRADE;

#endif // __INCLUDED_STATSDEF_H__
