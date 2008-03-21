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
/** @file
 *  Definitions for the AI system structures
 */

#ifndef __INCLUDED_SRC_AI_H__
#define __INCLUDED_SRC_AI_H__

#include "objectdef.h"
#include "action.h" //DROID_OACTION_INFO

#define ALLIANCE_BROKEN		0			// states of alliance between players
#define ALLIANCE_REQUESTED	1
#define ALLIANCE_INVITATION	2
#define ALLIANCE_FORMED		3
#define	ALLIANCE_NULL		4			// for setting values only.

#define NO_ALLIANCES		0			//alliance possibilities for games.
#define ALLIANCES			1
#define	ALLIANCES_TEAMS		2			//locked teams
//#define GROUP_WINS		2

/* Weights used for target selection code,
 * target distance is used as 'common currency'
 */
#define	WEIGHT_DIST_TILE			11						//In points used in weaponmodifier.txt and structuremodifier.txt
#define	WEIGHT_DIST_TILE_DROID		WEIGHT_DIST_TILE		//How much weight a distance of 1 tile (128 world units) has when looking for the best nearest target
#define	WEIGHT_DIST_TILE_STRUCT		WEIGHT_DIST_TILE
#define	WEIGHT_HEALTH_DROID			(WEIGHT_DIST_TILE * 10)	//How much weight unit damage has (100% of damage is equaly weighted as 10 tiles distance)
//~100% damage should be ~8 tiles (max sensor range)
#define	WEIGHT_HEALTH_STRUCT		(WEIGHT_DIST_TILE * 10)

#define	WEIGHT_NOT_VISIBLE_F		10						//We really don't like objects we can't see

#define	WEIGHT_SERVICE_DROIDS		(WEIGHT_DIST_TILE_DROID * 5)		//We don't want them to be repairing droids or structures while we are after them
#define	WEIGHT_WEAPON_DROIDS		(WEIGHT_DIST_TILE_DROID * 3)		//We prefer to go after anything that has a gun and can hurt us
#define	WEIGHT_COMMAND_DROIDS		(WEIGHT_DIST_TILE_DROID * 7)		//Commanders get a higher priority
#define	WEIGHT_MILITARY_STRUCT		WEIGHT_DIST_TILE_STRUCT				//Droid/cyborg factories, repair facility; shouldn't have too much weight
#define	WEIGHT_WEAPON_STRUCT		WEIGHT_WEAPON_DROIDS				//Same as weapon droids (?)
#define	WEIGHT_DERRICK_STRUCT		(WEIGHT_WEAPON_STRUCT +	WEIGHT_DIST_TILE_STRUCT * 4)	//Even if it's 4 tiles further away than defenses we still choose it

#define	WEIGHT_STRUCT_NOTBUILT_F	8						//Humans won't fool us anymore!

#define OLD_TARGET_THRESHOLD		(WEIGHT_DIST_TILE * 4)	//it only makes sense to switch target if new one is 4+ tiles closer

#define	EMP_DISABLED_PENALTY_F		10								//EMP shouldn't attack emped targets again
#define	EMP_STRUCT_PENALTY_F		(EMP_DISABLED_PENALTY_F * 2)	//EMP don't attack structures, should be bigger than EMP_DISABLED_PENALTY_F

//Some weights for the units attached to a commander
#define	WEIGHT_CMD_RANK				(WEIGHT_DIST_TILE * 4)			//A single rank is as important as 4 tiles distance
#define	WEIGHT_CMD_SAME_TARGET		WEIGHT_DIST_TILE				//Don't want this to be too high, since a commander can have many units assigned

// alliances
extern UBYTE alliances[MAX_PLAYERS][MAX_PLAYERS];

/* Check no alliance has formed*/
extern BOOL aiCheckAlliances(UDWORD s1,UDWORD s2);

/* Initialise the AI system */
extern BOOL aiInitialise(void);

/* Shutdown the AI system */
extern BOOL aiShutdown(void);

/* Initialise a droid structure for AI */
extern BOOL aiInitDroid(DROID *psDroid);

/* Do the AI for a droid */
extern void aiUpdateDroid(DROID *psDroid);

// Find the nearest best target for a droid
// returns integer representing quality of choice, -1 if failed
extern SDWORD aiBestNearestTarget(DROID *psDroid, BASE_OBJECT **ppsObj, int weapon_slot);

/* See if there is a target in range added int weapon_slot*/
extern BOOL aiChooseTarget(BASE_OBJECT *psObj,
						   BASE_OBJECT **ppsTarget, int weapon_slot, BOOL bUpdateTarget);

/*set the droid to attack if wihin range otherwise move to target*/
extern void attackTarget(DROID *psDroid, BASE_OBJECT *psTarget);

/* See if there is a target in range for Sensor objects*/
extern BOOL aiChooseSensorTarget(BASE_OBJECT *psObj, BASE_OBJECT **ppsTarget);

/*set of rules which determine whether the weapon associated with the object
can fire on the propulsion type of the target*/
extern BOOL validTarget(BASE_OBJECT *psObject, BASE_OBJECT *psTarget, int weapon_slot);

#endif // __INCLUDED_SRC_AI_H__
