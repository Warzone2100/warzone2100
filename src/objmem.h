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

#include <array>
#include <list>

/* The lists of objects allocated */
template <typename ObjectType, unsigned PlayerCount>
using PerPlayerObjectLists = std::array<std::list<ObjectType*>, PlayerCount>;

using PerPlayerDroidLists = PerPlayerObjectLists<DROID, MAX_PLAYERS>;
using DroidList = typename PerPlayerDroidLists::value_type;
extern PerPlayerDroidLists apsDroidLists;

using PerPlayerStructureLists = PerPlayerObjectLists<STRUCTURE, MAX_PLAYERS>;
using StructureList = typename PerPlayerStructureLists::value_type;
extern PerPlayerStructureLists apsStructLists;

using PerPlayerFeatureLists = PerPlayerObjectLists<FEATURE, MAX_PLAYERS>;
using FeatureList = typename PerPlayerFeatureLists::value_type;
extern PerPlayerFeatureLists apsFeatureLists;

using PerPlayerFlagPositionLists = PerPlayerObjectLists<FLAG_POSITION, MAX_PLAYERS>;
using FlagPositionList = typename PerPlayerFlagPositionLists::value_type;
extern PerPlayerFlagPositionLists apsFlagPosLists;

using PerPlayerExtractorLists = PerPlayerStructureLists;
using ExtractorList = typename PerPlayerExtractorLists::value_type;
extern PerPlayerExtractorLists apsExtractorLists;

using GlobalSensorList = PerPlayerObjectLists<BASE_OBJECT, 1>;
using SensorList = typename GlobalSensorList::value_type;
extern GlobalSensorList apsSensorList;

using GlobalOilList = PerPlayerObjectLists<FEATURE, 1>;
using OilList = typename GlobalOilList::value_type;
extern GlobalOilList apsOilList;

/* The list of destroyed objects */
using DestroyedObjectsList = std::list<BASE_OBJECT*>;
extern DestroyedObjectsList psDestroyedObj;

/* Initialise the object heaps */
bool objmemInitialise();

/* Release the object heaps */
void objmemShutdown();

/* General housekeeping for the object system */
void objmemUpdate();

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
void killDroid(DROID *psDel);

/* Remove all droids */
void freeAllDroids();

/*Remove a single Droid from its list*/
void removeDroid(DROID *psDroidToRemove, PerPlayerDroidLists& pList);

/*Removes all droids that may be stored in the mission lists*/
void freeAllMissionDroids();

/*Removes all droids that may be stored in the limbo lists*/
void freeAllLimboDroids();

/* add the structure to the Structure Lists */
void addStructure(STRUCTURE *psStructToAdd);

/* Destroy a structure */
void killStruct(STRUCTURE *psDel);

/* Remove all structures */
void freeAllStructs();

/*Remove a single Structure from a list*/
void removeStructureFromList(STRUCTURE *psStructToRemove, PerPlayerStructureLists& pList);

/* add the feature to the Feature Lists */
void addFeature(FEATURE *psFeatureToAdd);

/* Destroy a feature */
void killFeature(FEATURE *psDel);

/* Remove all features */
void freeAllFeatures();

/* Create a new Flag Position */
bool createFlagPosition(FLAG_POSITION **ppsNew, UDWORD player);
/* add the Flag Position to the Flag Position Lists */
void addFlagPosition(FLAG_POSITION *psFlagPosToAdd);
/* Remove a Flag Position from the Lists */
void removeFlagPosition(FLAG_POSITION *psDel);
/* Transfer a Flag Position to a new player */
void transferFlagPositionToPlayer(FLAG_POSITION *psFlagPos, UDWORD originalPlayer, UDWORD newPlayer);
// free all flag positions
void freeAllFlagPositions();
// used to add flag position to a specific list (ex. from assignFactoryCommandDroid)
void addFlagPositionToList(FLAG_POSITION* psFlagPosToAdd, PerPlayerFlagPositionLists& list);

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
