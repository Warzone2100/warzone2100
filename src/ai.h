/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

#include "droiddef.h"

#define ALLIANCE_BROKEN		0			// states of alliance between players
#define ALLIANCE_REQUESTED	1
#define ALLIANCE_INVITATION	2
#define ALLIANCE_FORMED		3
#define	ALLIANCE_NULL		4			// for setting values only.

enum AllianceType
{
	NO_ALLIANCES,        // FFA
	ALLIANCES,           // Players can make and break alliances during the game.
	ALLIANCES_TEAMS,     // Alliances are set before the game.
	ALLIANCES_UNSHARED,  // Alliances are set before the game. No shared research.
};

/// Amount of time to rage at the world when frustrated (10 seconds)
#define FRUSTRATED_TIME (1000 * 10)

// alliances
extern uint8_t alliances[MAX_PLAYER_SLOTS][MAX_PLAYER_SLOTS];
extern PlayerMask alliancebits[MAX_PLAYER_SLOTS];
extern PlayerMask satuplinkbits;

/** Check no alliance has formed. This is a define to make sure we inline it. */
#define aiCheckAlliances(_s1, _s2) (alliances[_s1][_s2] == ALLIANCE_FORMED)

/* Initialise the AI system */
bool aiInitialise(void);

/* Shutdown the AI system */
bool aiShutdown(void);

/* Initialise a droid structure for AI */
//extern bool aiInitDroid(DROID *psDroid);

/* Do the AI for a droid */
void aiUpdateDroid(DROID *psDroid);

// Find the nearest best target for a droid
// returns integer representing quality of choice, -1 if failed
int aiBestNearestTarget(DROID *psDroid, BASE_OBJECT **ppsObj, int weapon_slot, int extraRange = 0);

// Are there a lot of bullets heading towards the structure?
bool aiObjectIsProbablyDoomed(BASE_OBJECT *psObject);

// Update the expected damage of the object.
void aiObjectAddExpectedDamage(BASE_OBJECT *psObject, SDWORD damage);

/* See if there is a target in range added int weapon_slot*/
bool aiChooseTarget(BASE_OBJECT *psObj,
						   BASE_OBJECT **ppsTarget, int weapon_slot, bool bUpdateTarget, UWORD *targetOrigin);

/** See if there is a target in range for Sensor objects. */
bool aiChooseSensorTarget(BASE_OBJECT *psObj, BASE_OBJECT **ppsTarget);

/*set of rules which determine whether the weapon associated with the object
can fire on the propulsion type of the target*/
bool validTarget(BASE_OBJECT *psObject, BASE_OBJECT *psTarget, int weapon_slot);
// Check if any of the weapons can target the target
bool checkAnyWeaponsTarget(BASE_OBJECT *psObject, BASE_OBJECT *psTarget);
// Check properties of the AllianceType enum.
static inline bool alliancesFixed(int t) { return t != ALLIANCES; }
static inline bool alliancesSharedVision(int t) { return t == ALLIANCES_TEAMS || t == ALLIANCES_UNSHARED; }
static inline bool alliancesSharedResearch(int t) { return t == ALLIANCES || t == ALLIANCES_TEAMS; }
static inline bool alliancesSetTeamsBeforeGame(int t) { return t == ALLIANCES_TEAMS || t == ALLIANCES_UNSHARED; }
static inline bool alliancesCanGiveResearchAndRadar(int t) { return t == ALLIANCES; }
static inline bool alliancesCanGiveAnything(int t) { return t != NO_ALLIANCES; }

#endif // __INCLUDED_SRC_AI_H__
