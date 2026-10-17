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
 *  Routines for managing object's memory
 */

#ifndef __INCLUDED_SRC_OBJMEM_H__
#define __INCLUDED_SRC_OBJMEM_H__

#include "objectdef.h"
#include "object_lists_types.h"

#include <list>

struct GameWorld;
struct WorldObjectState;

/* The list of destroyed objects */
using DestroyedObjectsList = std::list<BASE_OBJECT*>;
extern DestroyedObjectsList psDestroyedObj;

/* The allocation state for synchronised/unsynchronised object IDs.
 * Must be snapshotted/restored so a resumed client keeps allocating unique, in-sync IDs. */
struct ObjectIdState
{
	uint32_t synchObjID;
	uint32_t unsynchObjID;
};

/* Capture/restore the object ID allocation counters */
ObjectIdState getObjectIdState();
void setObjectIdState(const ObjectIdState &s);

/* Initialise the object heaps */
bool objmemInitialise();

/* Release the object heaps */
void objmemShutdown();

/* General housekeeping for the object system */
void objmemUpdate();

// The time to stamp on an object that dies (BASE_OBJECT::died, or the impactTime passed to destroyDroid() and the
// like). objmemUpdate() runs at the end of every tick: it raises eventDestroyed for the objects that died in that
// tick, and frees the ones that died before it.

/// gameTime. In a tick, the object gets eventDestroyed at the end of that tick. Between ticks, it is freed at the end
/// of the next tick without eventDestroyed. Game and level setup removals rely on that.
uint32_t deathTimeNow();

/// The first moment of the tick being processed, or between ticks of the next one, so the object always gets
/// eventDestroyed. For deaths caused by a message processed between ticks (every client processes it before that
/// same tick).
uint32_t deathTimeInTick();

/* Free every object pending destruction now, whenever it died, without raising eventDestroyed.
 * For the end of a level, when no tick will see them again. */
void objmemFreeAllDestroyed();

/* Remove tile visibility for objects killed this tick in the given world, against that
 * world's own map. Safe to call repeatedly; clears the world's pending-removal queue. */
void flushPendingVisRemoval(GameWorld& world);

/* Remove an object from the destroyed list, finally freeing its memory
 * Hopefully by this time, no pointers still refer to it! */
bool objmemDestroy(BASE_OBJECT* psObj, bool checkRefs);

/// Generates a new, (hopefully) unique object id.
uint32_t generateNewObjectId();
/// Generates a new, (hopefully) unique object id, which all clients agree on.
uint32_t generateSynchronisedObjectId();

/* add the droid to the Droid Lists */
void addDroid(DROID *psDroidToAdd, PerPlayerDroidLists& pList);

/*destroy a droid */
void killDroid(DROID *psDel, WorldObjectState& objState);

/* Remove all droids */
void freeAllDroids(GameWorld& world);

/*Remove a single Droid from its list*/
void removeDroid(DROID *psDroidToRemove, PerPlayerDroidLists& pList);

/*Removes all droids that may be stored in the limbo lists*/
void freeAllLimboDroids();

/* add the structure to the Structure Lists */
void addStructure(STRUCTURE *psStructToAdd, WorldObjectState& objState);

/* Destroy a structure */
void killStruct(STRUCTURE *psDel, WorldObjectState& objState);

/* Remove all structures */
void freeAllStructs(GameWorld& world);

/*Remove a single Structure from a list*/
void removeStructureFromList(STRUCTURE *psStructToRemove, WorldObjectState& objState);

/* add the feature to the Feature Lists */
void addFeature(FEATURE *psFeatureToAdd, WorldObjectState& objState);

/*Remove a single Feature from its lists (the reverse of addFeature)*/
void removeFeatureFromList(FEATURE *psFeatureToRemove, WorldObjectState& objState);

/* Destroy a feature */
void killFeature(FEATURE *psDel, WorldObjectState& objState);

/* Remove all features */
void freeAllFeatures(GameWorld& world);

/* Create a new Flag Position */
bool createFlagPosition(FLAG_POSITION **ppsNew, UDWORD player);
/* add the Flag Position to the Flag Position Lists */
void addFlagPosition(FLAG_POSITION *psFlagPosToAdd, WorldObjectState& objState);
/* Remove a Flag Position from the Lists */
void removeFlagPosition(FLAG_POSITION *psDel);
/* Transfer a Flag Position to a new player */
void transferFlagPositionToPlayer(WorldObjectState& objState, FLAG_POSITION *psFlagPos, UDWORD originalPlayer, UDWORD newPlayer);
// free all flag positions
void freeAllFlagPositions(WorldObjectState& objState);

// Find a base object from it's id
template <typename ObjectType>
BASE_OBJECT* getBaseObjFromId(const std::list<ObjectType*>& list, unsigned id)
{
	auto objIt = std::find_if(list.begin(), list.end(), [id](ObjectType* obj)
	{
		return obj->id == id;
	});
	return objIt != list.end() ? *objIt : nullptr;
}

BASE_OBJECT *getBaseObjFromData(unsigned id, unsigned player, OBJECT_TYPE type);
BASE_OBJECT *getBaseObjFromId(UDWORD id);

UDWORD getRepairIdFromFlag(const FLAG_POSITION *psFlag);

void objCount(int *droids, int *structures, int *features);

#ifdef DEBUG
void checkFactoryFlags();
#endif

#endif // __INCLUDED_SRC_OBJMEM_H__
