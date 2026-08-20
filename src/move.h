/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
 *  Interface for the unit movement system
 */

#ifndef __INCLUDED_SRC_MOVE_H__
#define __INCLUDED_SRC_MOVE_H__

#include "objectdef.h"
#include "fpath.h"

struct WorldMapState;

/* Set a target location for a droid to move to  - returns a bool based on if there is a path to the destination (true if there is a path)*/
bool moveDroidTo(DROID *psDroid, UDWORD x, UDWORD y, FPATH_MOVETYPE moveType = FMT_MOVE);

/* Set a target location for a droid to move to  - returns a bool based on if there is a path to the destination (true if there is a path)*/
// the droid will not join a formation when it gets to the location
bool moveDroidToNoFormation(DROID *psDroid, UDWORD x, UDWORD y, FPATH_MOVETYPE moveType = FMT_MOVE);

// move a droid directly to a location (used by vtols only)
void moveDroidToDirect(DROID *psDroid, UDWORD x, UDWORD y);

// Get a droid to turn towards a locaton
void moveTurnDroid(DROID *psDroid, UDWORD x, UDWORD y);

/* Stop a droid */
void moveStopDroid(DROID *psDroid);

/*Stops a droid dead in its tracks - doesn't allow for any little skidding bits*/
void moveReallyStopDroid(DROID *psDroid);

/* Get a droid to do a frame's worth of moving */
void moveUpdateDroid(DROID *psDroid);

SDWORD moveCalcDroidSpeed(DROID *psDroid);

/* update body and turret to local slope */
void updateDroidOrientation(DROID *psDroid, const WorldMapState& mapState);

/* audio callback used to kill movement sounds */
bool moveCheckDroidMovingAndVisible(void *psObj);

const char *moveDescription(MOVE_STATUS status);

bool moveSetFormationSpeedLimiting(uint32_t player, bool enabled);
bool moveToggleFormationSpeedLimiting(uint32_t player, bool *pBoolResultingValue);
bool moveFormationSpeedLimitingOn(uint32_t player);
// Set the flag directly with no synchronising net message, for GameState restore only.
void moveRestoreFormationSpeedLimiting(uint32_t player, bool enabled);
bool recvSyncOptChange(NETQUEUE queue);

void moveInit();

// Get the collision radius of an object
int moveObjRadius(const BASE_OBJECT *psObj);

// Check if a droid is blocked
bool moveBlocked(DROID *psDroid);

/// Runtime-tunable local steering/avoidance/shuffle constants.
/// Editable from the debug menu's "Pathfinding" tab (see wzscriptdebug.cpp)
/// NOT SYNCED OVER THE NETWORK - for single-player debug use only
extern int moveTuning_MOVE_SHUFFLETIME;      ///< How long a shuffle can propagate before they all stop.
extern int moveTuning_BLOCK_TIME;            ///< Length of time a droid has to be stationary to be considered blocked.
extern int moveTuning_SHUFFLE_BLOCK_TIME;    ///< As BLOCK_TIME, but while shuffling out of the way.
extern int moveTuning_BLOCK_PAUSETIME;       ///< How long a droid has to be stationary before it stops trying to move.
extern int moveTuning_BLOCK_PAUSERELEASE;    ///< How long since the last bump before a droid is released from the block pause.
extern int moveTuning_BLOCK_DIST;            ///< How far a droid has to move before it is no longer considered 'stationary'.
extern int moveTuning_BLOCK_DIR;             ///< How far a droid has to rotate (degrees) before it is no longer considered 'stationary'.
extern int moveTuning_SHUFFLE_DIST;          ///< Distance to consider other droids for a shuffle.
extern int moveTuning_SHUFFLE_MOVE;          ///< How far to move for a shuffle.

#endif // __INCLUDED_SRC_MOVE_H__
