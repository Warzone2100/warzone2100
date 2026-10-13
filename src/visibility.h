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

#ifndef __INCLUDED_SRC_VISIBILITY__
#define __INCLUDED_SRC_VISIBILITY__

#include "objectdef.h"
#include "raycast.h"
#include "stats.h"
#include "droid.h"

#include <string>
#include <vector>

#define LINE_OF_FIRE_MINIMUM 5

struct WorldMapState;

// initialise the visibility stuff
bool visInitialise();

struct GameWorld;

/* Check which tiles can be seen by an object */
void visTilesUpdate(BASE_OBJECT *psObj, WorldMapState& mapState);

/// Recompute, in both worlds, the tile coverage of the objects that the changes recorded since the last call may
/// have changed (terrain heights, see WorldMapState::heightChanges, and range upgrades, see visNoteRangeChange),
/// so the coverage stays what a recompute from the current state gives. Called at the start and end of every tick.
void visFlushCoverageChanges();

/// Whether there are changes the next visFlushCoverageChanges() will apply (test tooling).
bool visCoverageChangesPending();

/// A player's sensor or jammer range changed (an upgrade): visFlushCoverageChanges() recomputes the tile coverage of
/// all their droids and structures, in both worlds.
void visNoteRangeChange(int player);

/// Recompute the tile coverage of every droid and structure in the world from its current state, and drop the
/// pending changes for it (a world restored from a save).
void visRecomputeCoverage(GameWorld& world);

/// Lift propulsion: the height band of the droid's current height, which its vision height is taken from.
int visLiftHeightBand(const DROID *psDroid);

void revealAll(WorldMapState& mapState, UBYTE player);

/* Check whether psViewer can see psTarget
 * psViewer should be an object that has some form of sensor,
 * currently droids and structures.
 * psTarget can be any type of BASE_OBJECT (e.g. a tree).
 */
int visibleObject(const BASE_OBJECT *psViewer, const BASE_OBJECT *psTarget, bool wallsBlock);

/** Can shooter hit target with direct fire weapon? */
bool lineOfFire(const SIMPLE_OBJECT *psViewer, const BASE_OBJECT *psTarget, int weapon_slot, bool wallsBlock);

/** How much of target can the player hit with direct fire weapon? */
int areaOfFire(const SIMPLE_OBJECT *psViewer, const BASE_OBJECT *psTarget, int weapon_slot, bool wallsBlock);

/** How much of target can the player hit with direct fire weapon? */
int arcOfFire(const SIMPLE_OBJECT *psViewer, const BASE_OBJECT *psTarget, int weapon_slot, bool wallsBlock);

// Find the wall that is blocking LOS to a target (if any)
STRUCTURE *visGetBlockingWall(const BASE_OBJECT *psViewer, const BASE_OBJECT *psTarget);

bool hasSharedVision(unsigned viewer, unsigned ally);

void processVisibility();  ///< Calls processVisibilitySelf and processVisibilityVision on all objects.

// update the visibility reduction
void visUpdateLevel();

void setUnderTilesVis(BASE_OBJECT *psObj, WorldMapState& mapState, UDWORD player);

void visRemoveVisibilityOffWorld(BASE_OBJECT *psObj);
void visRemoveVisibility(BASE_OBJECT *psObj, WorldMapState& mapState);

// fast test for whether obj2 is in range of obj1
static inline bool visObjInRange(const BASE_OBJECT *psObj1, const BASE_OBJECT *psObj2, SDWORD range)
{
	int32_t xdiff = psObj1->pos.x - psObj2->pos.x, ydiff = psObj1->pos.y - psObj2->pos.y;

	return abs(xdiff) <= range && abs(ydiff) <= range && xdiff * xdiff + ydiff * ydiff <= range;
}

// If we have ECM, use this for range instead. Otherwise, the sensor's range will be used for
// jamming range, which we do not want. Rather limit ECM unit sensor range to jammer range.
inline int objSensorRange(const BASE_OBJECT *psObj)
{
	if (psObj->type == OBJ_DROID)
	{
		const int ecmrange = ((const DROID*)psObj)->getECMStats()->upgrade[psObj->player].range;
		if (ecmrange > 0)
		{
			return ecmrange;
		}
		return ((const DROID*)psObj)->getSensorStats()->upgrade[psObj->player].range;
	}
	else if (psObj->type == OBJ_STRUCTURE)
	{
		const int ecmrange = ((const STRUCTURE *)psObj)->pStructureType->pECM->upgrade[psObj->player].range;
		if (ecmrange)
		{
			return ecmrange;
		}
		return ((const STRUCTURE *)psObj)->pStructureType->pSensor->upgrade[psObj->player].range;
	}
	return 0;
}

static inline int objJammerPower(const BASE_OBJECT *psObj)
{
	if (psObj->type == OBJ_DROID)
	{
		return ((const DROID*)psObj)->getECMStats()->upgrade[psObj->player].range;
	}
	else if (psObj->type == OBJ_STRUCTURE)
	{
		return ((const STRUCTURE *)psObj)->pStructureType->pECM->upgrade[psObj->player].range;
	}
	return 0;
}

void removeSpotters();
bool removeSpotter(uint32_t id);
uint32_t addSpotter(WorldMapState& mapState, int x, int y, int player, int radius, bool radar, uint32_t expiry = 0);

/// Plain snapshot of a live script spotter (apsInvisibleViewers) for GameState serialization.
struct SpotterSaveData
{
	int x;
	int y;
	int player;
	int sensorRadius;
	int sensorType;      ///< 0 = vision, 1 = radar
	uint32_t expiryTime; ///< when to self-destruct, 0 = never
	uint32_t id;
};
/// Enumerate all live spotters (any player) as plain data, in list order.
std::vector<SpotterSaveData> spotterEnumerateForSave();
/// Recreate a spotter from saved data, preserving its synchronised id (the ctor's fresh id-bump of
/// the object-ID counter is corrected by the counters-restored-last rule). Mirrors addSpotter's watch.
void spotterRestore(WorldMapState& mapState, const SpotterSaveData& d);

struct CoverageCheckResult
{
	size_t checked = 0;          ///< droids and structures compared
	size_t differing = 0;        ///< of those, how many have different watched tiles after a recompute
	std::string firstDifference; ///< a description of the first one that differs
	size_t inconsistentTiles = 0; ///< tile/player pairs whose watcher, sensor or jammer count isn't the sum of the watched tiles
	std::string firstInconsistentTile;
	size_t unexplored = 0;       ///< objects (and spotters) with a watched tile not explored for every player they share vision with
	std::string firstUnexplored;
};

/// Test tooling: recompute the tile coverage of every droid and structure in the world from its current state,
/// compare it with the coverage it has, and put everything back as it was.
/// The coverage is meant to be a function of the current state at the end of every tick (a loaded game can only
/// recompute it), so any difference is coverage that depends on how the game got there.
/// Also checks, on the coverage as it is:
/// - that every tile's per-player watcher, sensor and jammer counts are the sum of the watched tiles of the world's
///   objects (including those whose visibility removal is pending) and spotters, so a recompute can rebuild them
/// - that every watched tile is explored for every player the object shares vision with (alliancebits),
///   which lets a restore keep the exploration its recompute marks
///   (forming a shared-vision alliance shares the explored tiles, see hqReward)
CoverageCheckResult visCheckCoverage(GameWorld& world);

#endif // __INCLUDED_SRC_VISIBILITY__
