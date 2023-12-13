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
/*
 * ObjMem.c
 *
 * Object memory management functions.
 *
 */
#include <string.h>

#include "lib/framework/frame.h"
#include "objects.h"
#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"
#include "hci.h"
#include "map.h"
#include "power.h"
#include "objects.h"
#include "mission.h"
#include "structuredef.h"
#include "structure.h"
#include "droid.h"
#include "mapgrid.h"
#include "combat.h"
#include "visibility.h"
#include "qtscript.h"

#include <algorithm>

// the initial value for the object ID
#define OBJ_ID_INIT 20000

/* The id number for the next object allocated
 * Each object will have a unique id number irrespective of type
 */
uint32_t                unsynchObjID;
uint32_t                synchObjID;

/* The lists of objects allocated */
PerPlayerDroidLists apsDroidLists;
PerPlayerStructureLists apsStructLists;
PerPlayerFeatureLists apsFeatureLists;		///< Only player zero is valid for features. TODO: Reduce to single list.
PerPlayerExtractorLists apsExtractorLists;
GlobalOilList apsOilList;
GlobalSensorList apsSensorList; ///< List of sensors in the game.

/*The list of Flag Positions allocated */
PerPlayerFlagPositionLists apsFlagPosLists;

/* The list of destroyed objects */
DestroyedObjectsList psDestroyedObj;

/* Forward function declarations */
#ifdef DEBUG
static void objListIntegCheck();
#endif


/* Initialise the object heaps */
bool objmemInitialise()
{
	// reset the object ID number
	unsynchObjID = OBJ_ID_INIT / 2; // /2 so that object IDs start around OBJ_ID_INIT*8, in case that's important when loading maps.
	synchObjID   = OBJ_ID_INIT * 4; // *4 so that object IDs start around OBJ_ID_INIT*8, in case that's important when loading maps.

	return true;
}

/* Release the object heaps */
void objmemShutdown()
{
}

// Check that psVictim is not referred to by any other object in the game. We can dump out some extra data in debug builds that help track down sources of dangling pointer errors.
#ifdef DEBUG
#define BADREF(func, line) "Illegal reference to object %d from %s line %d", psVictim->id, func, line
#else
#define BADREF(func, line) "Illegal reference to object %d", psVictim->id
#endif
static bool checkReferences(BASE_OBJECT *psVictim)
{
	for (int plr = 0; plr < MAX_PLAYERS; ++plr)
	{
		for (STRUCTURE *psStruct : apsStructLists[plr])
		{
			if (psStruct == psVictim)
			{
				continue;  // Don't worry about self references.
			}

			for (unsigned i = 0; i < psStruct->numWeaps; ++i)
			{
				ASSERT_OR_RETURN(false, psStruct->psTarget[i] != psVictim, BADREF(psStruct->targetFunc[i], psStruct->targetLine[i]));
			}
		}
		for (DROID *psDroid : apsDroidLists[plr])
		{
			if (psDroid == psVictim)
			{
				continue;  // Don't worry about self references.
			}

			ASSERT_OR_RETURN(false, psDroid->order.psObj != psVictim, "Illegal reference to object %d", psVictim->id);

			ASSERT_OR_RETURN(false, psDroid->psBaseStruct != psVictim, "Illegal reference to object %d", psVictim->id);

			for (unsigned i = 0; i < psDroid->numWeaps; ++i)
			{
				if (psDroid->psActionTarget[i] == psVictim)
				{
					ASSERT_OR_RETURN(false, psDroid->psActionTarget[i] != psVictim, BADREF(psDroid->actionTargetFunc[i], psDroid->actionTargetLine[i]));
				}
			}
		}
	}
	return true;
}

/* Remove an object from the destroyed list, finally freeing its memory
 * Hopefully by this time, no pointers still refer to it! */
static bool objmemDestroy(BASE_OBJECT *psObj)
{
	switch (psObj->type)
	{
	case OBJ_DROID:
		debug(LOG_MEMORY, "freeing droid at %p", static_cast<void *>(psObj));
		break;

	case OBJ_STRUCTURE:
		debug(LOG_MEMORY, "freeing structure at %p", static_cast<void *>(psObj));
		break;

	case OBJ_FEATURE:
		debug(LOG_MEMORY, "freeing feature at %p", static_cast<void *>(psObj));
		break;

	default:
		ASSERT(!"unknown object type", "unknown object type in destroyed list at 0x%p", static_cast<void *>(psObj));
	}
	if (!checkReferences(psObj))
	{
		return false;
	}
	debug(LOG_MEMORY, "BASE_OBJECT* 0x%p is freed.", static_cast<void *>(psObj));
	delete psObj;
	return true;
}

/* General housekeeping for the object system */
void objmemUpdate()
{
#ifdef DEBUG
	// do a general validity check first
	objListIntegCheck();
#endif

	/* Go through the destroyed objects list looking for objects that
	   were destroyed before this turn */

	/* First remove the objects from the start of the list */
	DestroyedObjectsList::iterator it = psDestroyedObj.begin();
	while (it != psDestroyedObj.end())
	{
		if ((*it)->died <= gameTime - deltaGameTime)
		{
			objmemDestroy(*it);
			it = psDestroyedObj.erase(it);
		}
		else
		{
			// do the object died callback
			triggerEventDestroyed(*it++);
		}
	}
}

uint32_t generateNewObjectId()
{
	// Generate even ID for unsynchronized objects. This is needed for debug objects, templates and other border lines cases that should preferably be removed one day.
	return unsynchObjID++*MAX_PLAYERS * 2 + selectedPlayer * 2; // Was taken from createObject, where 'player' was used instead of 'selectedPlayer'. Hope there are no stupid hacks that try to recover 'player' from the last 3 bits.
}

uint32_t generateSynchronisedObjectId()
{
	// Generate odd ID for synchronized objects
	uint32_t ret = synchObjID++ * 2 + 1;
	syncDebug("New objectId = %u", ret);
	return ret;
}

/* Add the object to its list
 * \param list is a pointer to the object list
 */
template <typename OBJECT>
static inline void addObjectToList(PerPlayerObjectLists<OBJECT, MAX_PLAYERS>& list, OBJECT* object, int player)
{
	ASSERT_OR_RETURN(, object != nullptr, "Invalid pointer");

	// Prepend the object to the top of the list
	list[player].emplace_front(object);
}

/* Add the object to its list
 * \param list is a pointer to the object list
 */
template <typename FunctionList, typename OBJECT>
static inline void addObjectToFuncList(FunctionList& list, OBJECT *object, int player)
{
	ASSERT_OR_RETURN(, object != nullptr, "Invalid pointer");
	ASSERT_OR_RETURN(, !object->hasExtraFunction, "%s(%p) is already in a function list!", objInfo(object), static_cast<void *>(object));

	// Prepend the object to the top of the list
	list[player].emplace_front(object);
	object->hasExtraFunction = true;
}

/* Move an object from the active list to the destroyed list.
 * \param list is a pointer to the object list
 * \param del is a pointer to the object to remove
 */
template <typename OBJECT>
static inline void destroyObject(PerPlayerObjectLists<OBJECT, MAX_PLAYERS>& list, OBJECT* object)
{
	ASSERT_OR_RETURN(, object != nullptr, "Invalid pointer");
	ASSERT(gameTime - deltaGameTime <= gameTime || gameTime == 2, "Expected %u <= %u, bad time", gameTime - deltaGameTime, gameTime);

	auto it = std::find(list[object->player].begin(), list[object->player].end(), object);
	ASSERT(it != list[object->player].end(), "Object %s(%d) not found in list", objInfo(object), object->id);

	if (it != list[object->player].end())
	{
		list[object->player].erase(it);

		// Prepend the object to the destruction list
		psDestroyedObj.emplace_front((BASE_OBJECT*)object);

		// Set destruction time
		object->died = gameTime;
	}
	scriptRemoveObject(object);
}

/* Remove an object from the active list
 * \param list is a pointer to the object list
 * \param remove is a pointer to the object to remove
 * \param type is the type of the object
 */
template <typename OBJECT>
static inline void removeObjectFromList(PerPlayerObjectLists<OBJECT, MAX_PLAYERS>& list, OBJECT* object, int player)
{
	ASSERT_OR_RETURN(, object != nullptr, "Invalid pointer");

	auto it = std::find(list[player].begin(), list[player].end(), object);
	ASSERT_OR_RETURN(, it != list[player].end(), "Object %p not found in list", static_cast<void*>(object));
	list[player].erase(it);
}

/* Remove an object from the relevant function list. An object can only be in one function list at a time!
 * \param list is a pointer to the object list
 * \param remove is a pointer to the object to remove
 * \param type is the type of the object
 */
template <typename FunctionList, typename OBJECT>
static inline void removeObjectFromFuncList(FunctionList& list, OBJECT *object, int player)
{
	ASSERT_OR_RETURN(, object != nullptr, "Invalid pointer");

	// Iterate through the list and find the item before the object to delete
	auto it = std::find(list[player].begin(), list[player].end(), object);
	ASSERT_OR_RETURN(, it != list[player].end(), "Object %p not found in list", static_cast<void*>(object));
	list[player].erase(it);
	object->hasExtraFunction = false;
}

template <typename OBJECT>
static inline void releaseAllObjectsInList(PerPlayerObjectLists<OBJECT, MAX_PLAYERS>& lists)
{
	// Iterate through all players' object lists
	for (auto& list: lists)
	{
		// Iterate through all objects in list
		for (OBJECT* psCurr : list)
		{
			// FIXME: the next call is disabled for now, yes, it will leak memory again.
			// issue is with campaign games, and the swapping pointers 'trick' Pumpkin uses.
			//	visRemoveVisibility(psCurr);
			// Release object's memory
			delete psCurr;
		}
		list.clear();
	}
}

/***************************************************************************************
 *
 * The actual object memory management functions for the different object types
 */

/***************************  DROID  *********************************/

/* add the droid to the Droid Lists */
void addDroid(DROID *psDroidToAdd, PerPlayerDroidLists& pList)
{
	DROID_GROUP	*psGroup;

	addObjectToList(pList, psDroidToAdd, psDroidToAdd->player);

	/* Whenever a droid gets added to a list other than the current list
	 * its died flag is set to NOT_CURRENT_LIST so that anything targetting
	 * it will cancel itself - HACK?! */
	if (&pList[psDroidToAdd->player] == &apsDroidLists[psDroidToAdd->player])
	{
		psDroidToAdd->died = false;
		if (psDroidToAdd->droidType == DROID_SENSOR)
		{
			addObjectToFuncList(apsSensorList, (BASE_OBJECT *)psDroidToAdd, 0);
		}

		// commanders have to get their group back if not already loaded
		if (psDroidToAdd->droidType == DROID_COMMAND && !psDroidToAdd->psGroup)
		{
			psGroup = grpCreate();
			psGroup->add(psDroidToAdd);
		}
	}
	else if (&pList[psDroidToAdd->player] == &mission.apsDroidLists[psDroidToAdd->player])
	{
		if (psDroidToAdd->droidType == DROID_SENSOR)
		{
			addObjectToFuncList(mission.apsSensorList, (BASE_OBJECT *)psDroidToAdd, 0);
		}
	}
}

/* Destroy a droid */
void killDroid(DROID *psDel)
{
	int i;

	ASSERT(psDel->type == OBJ_DROID,
	       "killUnit: pointer is not a unit");
	ASSERT(psDel->player < MAX_PLAYERS,
	       "killUnit: invalid player for unit");

	setDroidTarget(psDel, nullptr);
	for (i = 0; i < MAX_WEAPONS; i++)
	{
		setDroidActionTarget(psDel, nullptr, i);
	}
	setDroidBase(psDel, nullptr);
	if (psDel->droidType == DROID_SENSOR)
	{
		removeObjectFromFuncList(apsSensorList, (BASE_OBJECT *)psDel, 0);
	}

	destroyObject(apsDroidLists, psDel);
}

/* Remove all droids */
void freeAllDroids()
{
	releaseAllObjectsInList(apsDroidLists);
}

/*Remove a single Droid from a list*/
void removeDroid(DROID* psDroidToRemove, PerPlayerDroidLists& pList)
{
	ASSERT_OR_RETURN(, psDroidToRemove->type == OBJ_DROID, "Pointer is not a unit");
	ASSERT_OR_RETURN(, psDroidToRemove->player < MAX_PLAYERS, "Invalid player for unit");
	removeObjectFromList(pList, psDroidToRemove, psDroidToRemove->player);

	/* Whenever a droid is removed from the current list its died
	 * flag is set to NOT_CURRENT_LIST so that anything targetting
	 * it will cancel itself, and we know it is not really on the map. */
	if (&pList[psDroidToRemove->player] == &apsDroidLists[psDroidToRemove->player])
	{
		if (psDroidToRemove->droidType == DROID_SENSOR)
		{
			removeObjectFromFuncList(apsSensorList, (BASE_OBJECT*)psDroidToRemove, 0);
		}
		psDroidToRemove->died = NOT_CURRENT_LIST;
	}
	else if (&pList[psDroidToRemove->player] == &mission.apsDroidLists[psDroidToRemove->player])
	{
		if (psDroidToRemove->droidType == DROID_SENSOR)
		{
			removeObjectFromFuncList(mission.apsSensorList, (BASE_OBJECT*)psDroidToRemove, 0);
		}
	}
}

/*Removes all droids that may be stored in the mission lists*/
void freeAllMissionDroids()
{
	releaseAllObjectsInList(mission.apsDroidLists);
}

/*Removes all droids that may be stored in the limbo lists*/
void freeAllLimboDroids()
{
	releaseAllObjectsInList(apsLimboDroids);
}

/**************************  STRUCTURE  *******************************/

/* add the structure to the Structure Lists */
void addStructure(STRUCTURE *psStructToAdd)
{
	addObjectToList(apsStructLists, psStructToAdd, psStructToAdd->player);
	if (psStructToAdd->pStructureType->pSensor
	    && psStructToAdd->pStructureType->pSensor->location == LOC_TURRET)
	{
		addObjectToFuncList(apsSensorList, (BASE_OBJECT *)psStructToAdd, 0);
	}
	else if (psStructToAdd->pStructureType->type == REF_RESOURCE_EXTRACTOR)
	{
		addObjectToFuncList(apsExtractorLists, psStructToAdd, psStructToAdd->player);
	}
}

/* Destroy a structure */
void killStruct(STRUCTURE *psBuilding)
{
	int i;

	ASSERT(psBuilding->type == OBJ_STRUCTURE,
	       "killStruct: pointer is not a droid");
	ASSERT(psBuilding->player < MAX_PLAYERS,
	       "killStruct: invalid player for structure");

	if (psBuilding->pStructureType->pSensor
	    && psBuilding->pStructureType->pSensor->location == LOC_TURRET)
	{
		removeObjectFromFuncList(apsSensorList, (BASE_OBJECT *)psBuilding, 0);
	}
	else if (psBuilding->pStructureType->type == REF_RESOURCE_EXTRACTOR)
	{
		removeObjectFromFuncList(apsExtractorLists, psBuilding, psBuilding->player);
	}

	for (i = 0; i < MAX_WEAPONS; i++)
	{
		setStructureTarget(psBuilding, nullptr, i, ORIGIN_UNKNOWN);
	}

	if (psBuilding->pFunctionality != nullptr)
	{
		if (StructIsFactory(psBuilding))
		{
			FACTORY *psFactory = &psBuilding->pFunctionality->factory;

			// remove any commander from the factory
			if (psFactory->psCommander != nullptr)
			{
				assignFactoryCommandDroid(psBuilding, nullptr);
			}

			// remove any assembly points
			if (psFactory->psAssemblyPoint != nullptr)
			{
				removeFlagPosition(psFactory->psAssemblyPoint);
				psFactory->psAssemblyPoint = nullptr;
			}
		}
		else if (psBuilding->pStructureType->type == REF_REPAIR_FACILITY)
		{
			REPAIR_FACILITY *psRepair = &psBuilding->pFunctionality->repairFacility;

			if (psRepair->psDeliveryPoint)
			{
				// free up repair fac stuff
				removeFlagPosition(psRepair->psDeliveryPoint);
				psRepair->psDeliveryPoint = nullptr;
			}
		}
	}

	destroyObject(apsStructLists, psBuilding);
}

/* Remove heapall structures */
void freeAllStructs()
{
	releaseAllObjectsInList(apsStructLists);
}

/*Remove a single Structure from a list*/
void removeStructureFromList(STRUCTURE *psStructToRemove, PerPlayerStructureLists& pList)
{
	ASSERT(psStructToRemove->type == OBJ_STRUCTURE,
	       "removeStructureFromList: pointer is not a structure");
	ASSERT(psStructToRemove->player < MAX_PLAYERS,
	       "removeStructureFromList: invalid player for structure");
	removeObjectFromList(pList, psStructToRemove, psStructToRemove->player);
	if (psStructToRemove->pStructureType->pSensor
	    && psStructToRemove->pStructureType->pSensor->location == LOC_TURRET)
	{
		removeObjectFromFuncList(apsSensorList, (BASE_OBJECT *)psStructToRemove, 0);
	}
	else if (psStructToRemove->pStructureType->type == REF_RESOURCE_EXTRACTOR)
	{
		removeObjectFromFuncList(apsExtractorLists, psStructToRemove, psStructToRemove->player);
	}
}

/**************************  FEATURE  *********************************/

/* add the feature to the Feature Lists */
void addFeature(FEATURE *psFeatureToAdd)
{
	addObjectToList(apsFeatureLists, psFeatureToAdd, 0);
	if (psFeatureToAdd->psStats->subType == FEAT_OIL_RESOURCE)
	{
		addObjectToFuncList(apsOilList, psFeatureToAdd, 0);
	}
}

/* Destroy a feature */
// set the player to 0 since features have player = maxplayers+1. This screws up destroyObject
// it's a bit of a hack, but hey, it works
void killFeature(FEATURE *psDel)
{
	ASSERT(psDel->type == OBJ_FEATURE,
	       "killFeature: pointer is not a feature");
	psDel->player = 0;
	destroyObject(apsFeatureLists, psDel);

	if (psDel->psStats->subType == FEAT_OIL_RESOURCE)
	{
		removeObjectFromFuncList(apsOilList, psDel, 0);
	}
}

/* Remove all features */
void freeAllFeatures()
{
	releaseAllObjectsInList(apsFeatureLists);
}

/**************************  FLAG_POSITION ********************************/

/* Create a new Flag Position */
bool createFlagPosition(FLAG_POSITION **ppsNew, UDWORD player)
{
	ASSERT(player < MAX_PLAYERS, "createFlagPosition: invalid player number");

	*ppsNew = (FLAG_POSITION *)calloc(1, sizeof(FLAG_POSITION));
	if (*ppsNew == nullptr)
	{
		debug(LOG_ERROR, "Out of memory");
		return false;
	}
	(*ppsNew)->type = POS_DELIVERY;
	(*ppsNew)->player = player;
	(*ppsNew)->frameNumber = 0;
	(*ppsNew)->selected = false;
	(*ppsNew)->coords.x = ~0;
	(*ppsNew)->coords.y = ~0;
	(*ppsNew)->coords.z = ~0;
	return true;
}

static bool isFlagPositionInList(FLAG_POSITION *psFlagPosToAdd, const PerPlayerFlagPositionLists& list)
{
	ASSERT_OR_RETURN(false, psFlagPosToAdd != nullptr, "Invalid FlagPosition pointer");
	ASSERT_OR_RETURN(false, psFlagPosToAdd->player < MAX_PLAYERS, "Invalid FlagPosition player: %u", psFlagPosToAdd->player);
	const auto& flagPosList = list[psFlagPosToAdd->player];
	return flagPosList.end() != std::find(flagPosList.begin(), flagPosList.end(), psFlagPosToAdd);
}

/* add the Flag Position to the Flag Position Lists */
void addFlagPositionToList(FLAG_POSITION *psFlagPosToAdd, PerPlayerFlagPositionLists& list)
{
	ASSERT_OR_RETURN(, psFlagPosToAdd != nullptr, "Invalid FlagPosition pointer");
	ASSERT_OR_RETURN(, psFlagPosToAdd->coords.x != ~0, "flag has invalid position");
	ASSERT_OR_RETURN(, psFlagPosToAdd->player < MAX_PLAYERS, "Invalid FlagPosition player: %u", psFlagPosToAdd->player);
	if (isFlagPositionInList(psFlagPosToAdd, list))
	{
		debug(LOG_INFO, "FlagPosition is already in the list - ignoring");
		return;
	}
	list[psFlagPosToAdd->player].emplace_front(psFlagPosToAdd);
}

/* add the Flag Position to the Flag Position Lists */
void addFlagPosition(FLAG_POSITION *psFlagPosToAdd)
{
	addFlagPositionToList(psFlagPosToAdd, apsFlagPosLists);
}

// Remove it from the list, but don't delete it!
static bool removeFlagPositionFromList(FLAG_POSITION *psRemove)
{
	ASSERT_OR_RETURN(false, psRemove != nullptr, "Invalid Flag Position pointer");
	ASSERT_OR_RETURN(false, psRemove->player < MAX_PLAYERS, "Invalid Flag Position player: %" PRIu32, psRemove->player);

	auto& flagPosList = apsFlagPosLists[psRemove->player];
	auto it = std::find(flagPosList.begin(), flagPosList.end(), psRemove);
	if (it != flagPosList.end())
	{
		flagPosList.erase(it);
		return true;
	}

	return false;
}

/* Remove a Flag Position from the Lists */
void removeFlagPosition(FLAG_POSITION *psDel)
{
	ASSERT_OR_RETURN(, psDel != nullptr, "Invalid Flag Position pointer");

	if (removeFlagPositionFromList(psDel))
	{
		free(psDel);
	}
	else
	{
		ASSERT(false, "Did not find flag position in expected list?");
	}
}

/* Transfer a Flag Position to a new player */
void transferFlagPositionToPlayer(FLAG_POSITION *psFlagPos, UDWORD originalPlayer, UDWORD newPlayer)
{
	ASSERT_OR_RETURN(, psFlagPos != nullptr, "Invalid Flag Position pointer");
	ASSERT(originalPlayer == psFlagPos->player, "Unexpected originalPlayer (%" PRIu32 ") does not match current flagPos->player (%" PRIu32 ")", originalPlayer, psFlagPos->player);
	ASSERT(removeFlagPositionFromList(psFlagPos), "Did not find flag position in expected list?");
	psFlagPos->player = newPlayer;
	addFlagPosition(psFlagPos);
}

// free all flag positions
void freeAllFlagPositions()
{
	for (uint32_t player = 0; player < MAX_PLAYERS; player++)
	{
		for (const auto& flagPos : apsFlagPosLists[player])
		{
			ASSERT(player == flagPos->player, "Player mismatch? (flagPos->player == %" PRIu32 ", expecting: %d", flagPos->player, player);
			free(flagPos);
		}
		apsFlagPosLists[player].clear();
	}
}


#ifdef DEBUG
// check all flag positions for duplicate delivery points
void checkFactoryFlags()
{
	static std::vector<unsigned> factoryDeliveryPointCheck[NUM_FLAG_TYPES];  // Static to save allocations.

	//check the flags
	for (unsigned player = 0; player < MAX_PLAYERS; ++player)
	{
		//clear the check array
		for (int type = 0; type < NUM_FLAG_TYPES; ++type)
		{
			factoryDeliveryPointCheck[type].clear();
		}

		for (const auto& flagPos : apsFlagPosLists[player])
		{
			if ((flagPos->type == POS_DELIVERY) &&//check this is attached to a unique factory
				(flagPos->factoryType != REPAIR_FLAG))
			{
				unsigned type = flagPos->factoryType;
				unsigned factory = flagPos->factoryInc;
				factoryDeliveryPointCheck[type].push_back(factory);
			}
		}
		for (int type = 0; type < NUM_FLAG_TYPES; ++type)
		{
			std::sort(factoryDeliveryPointCheck[type].begin(), factoryDeliveryPointCheck[type].end());
			ASSERT(std::unique(factoryDeliveryPointCheck[type].begin(), factoryDeliveryPointCheck[type].end()) == factoryDeliveryPointCheck[type].end(), "DUPLICATE FACTORY DELIVERY POINT FOUND");
		}
	}
}
#endif


/**************************  OBJECT ACCESS FUNCTIONALITY ********************************/

template <typename ObjectType>
BASE_OBJECT* getBaseObjFromId(const std::list<ObjectType*>& list, unsigned id)
{
	auto objIt = std::find_if(list.begin(), list.end(), [id](ObjectType* obj)
	{
		return obj->id == id;
	});
	return objIt != list.end() ? *objIt : nullptr;
}

static BASE_OBJECT* getBaseObjFromDroidId(const DroidList& list, unsigned id)
{
	for (DROID* psObj : list)
	{
		if (psObj->id == id)
		{
			return psObj;
		}
		// if transporter check any droids in the grp
		if ((psObj->type == OBJ_DROID) && isTransporter((DROID*)psObj))
		{
			ASSERT_OR_RETURN(nullptr, ((DROID*)psObj)->psGroup != nullptr, "Transporter has null group?");
			for (DROID* psTrans = ((DROID*)psObj)->psGroup->psList; psTrans != nullptr; psTrans = psTrans->psGrpNext)
			{
				if (psTrans->id == id)
				{
					return (BASE_OBJECT*)psTrans;
				}
			}
		}
	}
	return nullptr;
}

// Find a base object from its id
BASE_OBJECT *getBaseObjFromData(unsigned id, unsigned player, OBJECT_TYPE type)
{
	ASSERT_OR_RETURN(nullptr, player < MAX_PLAYERS || type == OBJ_FEATURE, "Invalid player: %u", player);

	switch (type)
	{
	case OBJ_DROID:
		{
			auto pDroid = getBaseObjFromDroidId(apsDroidLists[player], id);
			if (pDroid)
			{
				return pDroid;
			}
			pDroid = getBaseObjFromDroidId(mission.apsDroidLists[player], id);
			if (pDroid)
			{
				return pDroid;
			}
			if (player == 0)
			{
				pDroid = getBaseObjFromDroidId(apsLimboDroids[0], id);
				if (pDroid)
				{
					return pDroid;
				}
			}
			break;
		}
	case OBJ_STRUCTURE:
		{
			auto pStruct = getBaseObjFromId(apsStructLists[player], id);
			if (pStruct)
			{
				return pStruct;
			}
			pStruct = getBaseObjFromId(mission.apsStructLists[player], id);
			if (pStruct)
			{
				return pStruct;
			}
			break;
		}
	case OBJ_FEATURE:
		{
			if (player == 0)
			{
				auto pFeat = getBaseObjFromId(apsFeatureLists[0], id);
				if (pFeat)
				{
					return pFeat;
				}
				pFeat = getBaseObjFromId(mission.apsFeatureLists[0], id);
				if (pFeat)
				{
					return pFeat;
				}
				break;
			}
		}
	default:
		break;
	}
	return nullptr;
}

// Find a base object from it's id
BASE_OBJECT *getBaseObjFromId(UDWORD id)
{
	for (size_t type = OBJ_DROID; type != OBJ_NUM_TYPES; ++type)
	{
		for (size_t player = 0; player != MAX_PLAYERS; ++player)
		{
			auto psObj = getBaseObjFromData(id, player, static_cast<OBJECT_TYPE>(type));
			if (psObj)
			{
				return psObj;
			}
		}
	}
	ASSERT(!"couldn't find a BASE_OBJ with ID", "getBaseObjFromId() failed for id %d", id);

	return nullptr;
}

static UDWORD getRepairIdFromFlagSingleList(FLAG_POSITION* psFlag, uint32_t player, const StructureList& list)
{
	for (STRUCTURE* psObj : list)
	{
		if (psObj->pFunctionality)
		{
			if (psObj->pStructureType->type == REF_REPAIR_FACILITY)
			{
				//check for matching delivery point
				REPAIR_FACILITY* psRepair = ((REPAIR_FACILITY*)psObj->pFunctionality);
				if (psRepair->psDeliveryPoint == psFlag)
				{
					return psObj->id;
				}
			}
		}
	}
	return UDWORD_MAX;
}

UDWORD getRepairIdFromFlag(FLAG_POSITION *psFlag)
{
	unsigned int i;
	UDWORD			player;


	player = psFlag->player;

	//probably don't need to check mission list
	for (i = 0; i < 2; ++i)
	{
		switch (i)
		{
		case 0:
		{
			auto id = getRepairIdFromFlagSingleList(psFlag, player, apsStructLists[player]);
			if (id != UDWORD_MAX)
			{
				return id;
			}
			break;
		}
		case 1:
		{
			auto id = getRepairIdFromFlagSingleList(psFlag, player, mission.apsStructLists[player]);
			if (id != UDWORD_MAX)
			{
				return id;
			}
			break;
		}
		default:
			break;
		}
	}
	ASSERT(!"unable to find repair id for FLAG_POSITION", "getRepairIdFromFlag() failed");

	return UDWORD_MAX;
}

// integrity check the lists
#ifdef DEBUG
static void objListIntegCheck()
{
	SDWORD			player;

	for (player = 0; player < MAX_PLAYERS; player += 1)
	{
		for (DROID* psCurr : apsDroidLists[player])
		{
			ASSERT(psCurr->type == OBJ_DROID &&
			       (SDWORD)psCurr->player == player,
			       "objListIntegCheck: misplaced object in the droid list for player %d",
			       player);
		}
	}
	for (player = 0; player < MAX_PLAYERS; player += 1)
	{
		for (STRUCTURE* psStruct : apsStructLists[player])
		{
			ASSERT(psStruct->type == OBJ_STRUCTURE &&
			       (SDWORD)psStruct->player == player,
			       "objListIntegCheck: misplaced %s(%p) in the structure list for player %d, is owned by %d",
			       objInfo(psStruct), (void*)psStruct, player, (int)psStruct->player);
		}
	}
	for (BASE_OBJECT* obj : apsFeatureLists[0])
	{
		ASSERT(obj->type == OBJ_FEATURE,
		       "objListIntegCheck: misplaced object in the feature list");
	}
	for (const auto& obj : psDestroyedObj)
	{
		ASSERT(obj->died > 0, "objListIntegCheck: Object in destroyed list but not dead!");
	}
}
#endif

void objCount(int *droids, int *structures, int *features)
{
	*droids = 0;
	*structures = 0;
	*features = 0;

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		for (DROID *psDroid : apsDroidLists[i])
		{
			(*droids)++;
			if (isTransporter(psDroid) && psDroid->psGroup && psDroid->psGroup->psList)
			{
				DROID *psTrans = psDroid->psGroup->psList;

				for (psTrans = psTrans->psGrpNext; psTrans != nullptr; psTrans = psTrans->psGrpNext)
				{
					(*droids)++;
				}
			}
		}

		*structures += apsStructLists[i].size();
	}

	*features += apsFeatureLists[0].size();
}
