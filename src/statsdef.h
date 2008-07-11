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

#define NULL_COMP (-1)

/*
 * Stats structures type definitions
 */

#define    SHOOT_ON_GROUND  0x01
#define    SHOOT_IN_AIR     0x02

//Special angles representing top or bottom hit
#define HIT_ANGLE_TOP 361
#define HIT_ANGLE_BOTTOM 362

typedef struct _body_stats
{
	/* common stats - N.B. system points for a body
	 * are the limit for all other components
	 */
	STATS_COMPONENT;

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
	STATS_COMPONENT;

	UDWORD          progCap;                ///< Program capacity
	struct WEAPON_STATS* psWeaponStat;     ///< weapon stats associated with this brain - for Command Droids
} BRAIN_STATS;

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
