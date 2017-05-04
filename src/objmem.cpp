/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
#include "lib/script/script.h"
#include "scriptvals.h"
#include "scripttabs.h"
#include "scriptcb.h"
#include "mission.h"
#include "structuredef.h"
#include "structure.h"
#include "droid.h"
#include "mapgrid.h"
#include "combat.h"
#include "visibility.h"
#include "qtscript.h"

// the initial value for the object ID
#define OBJ_ID_INIT 20000

/* The id number for the next object allocated
 * Each object will have a unique id number irrespective of type
 */
uint32_t                unsynchObjID;
uint32_t                synchObjID;

/* The lists of objects allocated */
DROID			*apsDroidLists[MAX_PLAYERS];
STRUCTURE		*apsStructLists[MAX_PLAYERS];
FEATURE			*apsFeatureLists[MAX_PLAYERS];		///< Only player zero is valid for features. TODO: Reduce to single list.
STRUCTURE		*apsExtractorLists[MAX_PLAYERS];
FEATURE			*apsOilList[1];
BASE_OBJECT		*apsSensorList[1];			///< List of sensors in the game.

/*The list of Flag Positions allocated */
FLAG_POSITION	*apsFlagPosLists[MAX_PLAYERS];

/* The list of destroyed objects */
BASE_OBJECT		*psDestroyedObj = nullptr;

/* Forward function declarations */
#ifdef DEBUG
static void objListIntegCheck(void);
#endif


/* Initialise the object heaps */
bool objmemInitialise(void)
{
	// reset the object ID number
	unsynchObjID = OBJ_ID_INIT / 2; // /2 so that object IDs start around OBJ_ID_INIT*8, in case that's important when loading maps.
	synchObjID   = OBJ_ID_INIT * 4; // *4 so that object IDs start around OBJ_ID_INIT*8, in case that's important when loading maps.

	return true;
}

/* Release the object heaps */
void objmemShutdown(void)
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
		for (STRUCTURE *psStruct = apsStructLists[plr]; psStruct != nullptr; psStruct = psStruct->psNext)
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
		for (DROID *psDroid = apsDroidLists[plr]; psDroid != nullptr; psDroid = psDroid->psNext)
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
		debug(LOG_MEMORY, "freeing droid at %p", psObj);
		break;

	case OBJ_STRUCTURE:
		debug(LOG_MEMORY, "freeing structure at %p", psObj);
		break;

	case OBJ_FEATURE:
		debug(LOG_MEMORY, "freeing feature at %p", psObj);
		break;

	default:
		ASSERT(!"unknown object type", "unknown object type in destroyed list at 0x%p", psObj);
	}
	if (!checkReferences(psObj))
	{
		return false;
	}
	debug(LOG_MEMORY, "BASE_OBJECT* 0x%p is freed.", psObj);
	delete psObj;
	return true;
}

/* General housekeeping for the object system */
void objmemUpdate(void)
{
	BASE_OBJECT		*psCurr, *psNext, *psPrev;

#ifdef DEBUG
	// do a general validity check first
	objListIntegCheck();
#endif

	// tell the script system about any destroyed objects
	if (psDestroyedObj != nullptr)
	{
		scrvUpdateBasePointers();
	}

	/* Go through the destroyed objects list looking for objects that
	   were destroyed before this turn */

	/* First remove the objects from the start of the list */
	while (psDestroyedObj != nullptr && psDestroyedObj->died <= gameTime - deltaGameTime)
	{
		psNext = psDestroyedObj->psNext;
		objmemDestroy(psDestroyedObj);
		psDestroyedObj = psNext;
	}

	/* Now see if there are any further down the list
	Keep track of the previous object to set its Next pointer*/
	for (psCurr = psPrev = psDestroyedObj; psCurr != nullptr; psCurr = psNext)
	{
		psNext = psCurr->psNext;
		if (psCurr->died <= gameTime - deltaGameTime)
		{
			/*set the linked list up - you will never be deleting the top
			of the list, so don't have to check*/
			psPrev->psNext = psNext;
			objmemDestroy(psCurr);
		}
		else
		{
			// do the object died callback
			psCBObjDestroyed = psCurr;
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_OBJ_DESTROYED);
			switch (psCurr->type)
			{
			case OBJ_DROID:
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DROID_DESTROYED);
				break;
			case OBJ_STRUCTURE:
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_STRUCT_DESTROYED);
				break;
			case OBJ_FEATURE:
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_FEATURE_DESTROYED);
				break;
			default:
				break;
			}
			psCBObjDestroyed = nullptr;
			triggerEventDestroyed(psCurr);

			psPrev = psCurr;
		}
	}
}

uint32_t generateNewObjectId(void)
{
	// Generate even ID for unsynchronized objects. This is needed for debug objects, templates and other border lines cases that should preferably be removed one day.
	return unsynchObjID++*MAX_PLAYERS * 2 + selectedPlayer * 2; // Was taken from createObject, where 'player' was used instead of 'selectedPlayer'. Hope there are no stupid hacks that try to recover 'player' from the last 3 bits.
}

uint32_t generateSynchronisedObjectId(void)
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
static inline void addObjectToList(OBJECT *list[], OBJECT *object, int player)
{
	ASSERT_OR_RETURN(, object != nullptr, "Invalid pointer");

	// Prepend the object to the top of the list
	object->psNext = list[player];
	list[player] = object;
}

/* Add the object to its list
 * \param list is a pointer to the object list
 */
template <typename OBJECT>
static inline void addObjectToFuncList(OBJECT *list[], OBJECT *object, int player)
{
	ASSERT_OR_RETURN(, object != nullptr, "Invalid pointer");
	ASSERT_OR_RETURN(, static_cast<OBJECT *>(object->psNextFunc) == nullptr, "%s(%p) is already in a function list!", objInfo(object), object);

	// Prepend the object to the top of the list
	object->psNextFunc = list[player];
	list[player] = object;
}

/* Move an object from the active list to the destroyed list.
 * \param list is a pointer to the object list
 * \param del is a pointer to the object to remove
 */
template <typename OBJECT>
static inline void destroyObject(OBJECT *list[], OBJECT *object)
{
	ASSERT_OR_RETURN(, object != nullptr, "Invalid pointer");
	ASSERT(gameTime - deltaGameTime <= gameTime || gameTime == 2, "Expected %u <= %u, bad time", gameTime - deltaGameTime, gameTime);

	// If the message to remove is the first one in the list then mark the next one as the first
	if (list[object->player] == object)
	{
		list[object->player] = list[object->player]->psNext;
		object->psNext = psDestroyedObj;
		psDestroyedObj = (BASE_OBJECT *)object;
		object->died = gameTime;
		scriptRemoveObject(object);
		return;
	}

	// Iterate through the list and find the item before the object to delete
	OBJECT *psPrev = nullptr, *psCurr;
	for (psCurr = list[object->player]; (psCurr != object) && (psCurr != nullptr); psCurr = psCurr->psNext)
	{
		psPrev = psCurr;
	}

	ASSERT(psCurr != nullptr, "Object %s(%d) not found in list", objInfo(object), object->id);

	if (psCurr != nullptr)
	{
		// Modify the "next" pointer of the previous item to
		// point to the "next" item of the item to delete.
		psPrev->psNext = psCurr->psNext;

		// Prepend the object to the destruction list
		object->psNext = psDestroyedObj;
		psDestroyedObj = object;

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
static inline void removeObjectFromList(OBJECT *list[], OBJECT *object, int player)
{
	ASSERT_OR_RETURN(, object != nullptr, "Invalid pointer");

	// If the message to remove is the first one in the list then mark the next one as the first
	if (list[player] == object)
	{
		list[player] = list[player]->psNext;
		return;
	}

	// Iterate through the list and find the item before the object to delete
	OBJECT *psPrev = nullptr, *psCurr;
	for (psCurr = list[player]; (psCurr != object) && (psCurr != nullptr); psCurr = psCurr->psNext)
	{
		psPrev = psCurr;
	}

	ASSERT_OR_RETURN(, psCurr != nullptr, "Object %p not found in list", object);

	// Modify the "next" pointer of the previous item to
	// point to the "next" item of the item to delete.
	psPrev->psNext = psCurr->psNext;
}

/* Remove an object from the relevant function list. An object can only be in one function list at a time!
 * \param list is a pointer to the object list
 * \param remove is a pointer to the object to remove
 * \param type is the type of the object
 */
template <typename OBJECT>
static inline void removeObjectFromFuncList(OBJECT *list[], OBJECT *object, int player)
{
	ASSERT_OR_RETURN(, object != nullptr, "Invalid pointer");

	// If the message to remove is the first one in the list then mark the next one as the first
	if (list[player] == object)
	{
		list[player] = list[player]->psNextFunc;
		object->psNextFunc = nullptr;
		return;
	}

	// Iterate through the list and find the item before the object to delete
	OBJECT *psPrev = nullptr, *psCurr;
	for (psCurr = list[player]; psCurr != object && psCurr != nullptr; psCurr = psCurr->psNextFunc)
	{
		psPrev = psCurr;
	}

	ASSERT_OR_RETURN(, psCurr != nullptr, "Object %p not found in list", object);

	// Modify the "next" pointer of the previous item to
	// point to the "next" item of the item to delete.
	psPrev->psNextFunc = psCurr->psNextFunc;
	object->psNextFunc = nullptr;
}

template <typename OBJECT>
static inline void releaseAllObjectsInList(OBJECT *list[])
{
	// Iterate through all players' object lists
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		// Iterate through all objects in list
		OBJECT *psNext;
		for (OBJECT *psCurr = list[i]; psCurr != nullptr; psCurr = psNext)
		{
			psNext = psCurr->psNext;

			// FIXME: the next call is disabled for now, yes, it will leak memory again.
			// issue is with campaign games, and the swapping pointers 'trick' Pumpkin uses.
			//	visRemoveVisibility(psCurr);
			// Release object's memory
			delete psCurr;
		}
		list[i] = nullptr;
	}
}

/***************************************************************************************
 *
 * The actual object memory management functions for the different object types
 */

/***************************  DROID  *********************************/

/* add the droid to the Droid Lists */
void addDroid(DROID *psDroidToAdd, DROID *pList[MAX_PLAYERS])
{
	DROID_GROUP	*psGroup;

	addObjectToList(pList, psDroidToAdd, psDroidToAdd->player);

	/* Whenever a droid gets added to a list other than the current list
	 * its died flag is set to NOT_CURRENT_LIST so that anything targetting
	 * it will cancel itself - HACK?! */
	if (pList[psDroidToAdd->player] == apsDroidLists[psDroidToAdd->player])
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
	else if (pList[psDroidToAdd->player] == mission.apsDroidLists[psDroidToAdd->player])
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
void freeAllDroids(void)
{
	releaseAllObjectsInList(apsDroidLists);
}

/*Remove a single Droid from a list*/
void removeDroid(DROID *psDroidToRemove, DROID *pList[MAX_PLAYERS])
{
	ASSERT_OR_RETURN(, psDroidToRemove->type == OBJ_DROID, "Pointer is not a unit");
	ASSERT_OR_RETURN(, psDroidToRemove->player < MAX_PLAYERS, "Invalid player for unit");
	removeObjectFromList(pList, psDroidToRemove, psDroidToRemove->player);

	/* Whenever a droid is removed from the current list its died
	 * flag is set to NOT_CURRENT_LIST so that anything targetting
	 * it will cancel itself, and we know it is not really on the map. */
	if (pList[psDroidToRemove->player] == apsDroidLists[psDroidToRemove->player])
	{
		if (psDroidToRemove->droidType == DROID_SENSOR)
		{
			removeObjectFromFuncList(apsSensorList, (BASE_OBJECT *)psDroidToRemove, 0);
		}
		psDroidToRemove->died = NOT_CURRENT_LIST;
	}
	else if (pList[psDroidToRemove->player] == mission.apsDroidLists[psDroidToRemove->player])
	{
		if (psDroidToRemove->droidType == DROID_SENSOR)
		{
			removeObjectFromFuncList(mission.apsSensorList, (BASE_OBJECT *)psDroidToRemove, 0);
		}
	}
}

/*Removes all droids that may be stored in the mission lists*/
void freeAllMissionDroids(void)
{
	releaseAllObjectsInList(mission.apsDroidLists);
}

/*Removes all droids that may be stored in the limbo lists*/
void freeAllLimboDroids(void)
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
	       "killStruct: invalid player for stucture");

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
void freeAllStructs(void)
{
	releaseAllObjectsInList(apsStructLists);
}

/*Remove a single Structure from a list*/
void removeStructureFromList(STRUCTURE *psStructToRemove, STRUCTURE *pList[MAX_PLAYERS])
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
void freeAllFeatures(void)
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

/* add the Flag Position to the Flag Position Lists */
void addFlagPosition(FLAG_POSITION *psFlagPosToAdd)
{
	ASSERT_OR_RETURN(, psFlagPosToAdd != nullptr, "Invalid FlagPosition pointer");
	ASSERT_OR_RETURN(, psFlagPosToAdd->coords.x != ~0, "flag has invalid position");

	psFlagPosToAdd->psNext = apsFlagPosLists[psFlagPosToAdd->player];
	apsFlagPosLists[psFlagPosToAdd->player] = psFlagPosToAdd;
}

/* Remove a Flag Position from the Lists */
void removeFlagPosition(FLAG_POSITION *psDel)
{
	FLAG_POSITION		*psPrev = nullptr, *psCurr;

	ASSERT_OR_RETURN(, psDel != nullptr, "Invalid Flag Position pointer");

	if (apsFlagPosLists[psDel->player] == psDel)
	{
		apsFlagPosLists[psDel->player] = apsFlagPosLists[psDel->player]->psNext;
		free(psDel);
	}
	else
	{
		for (psCurr = apsFlagPosLists[psDel->player]; (psCurr != psDel) &&
		     (psCurr != nullptr); psCurr = psCurr->psNext)
		{
			psPrev = psCurr;
		}
		if (psCurr != nullptr)
		{
			psPrev->psNext = psCurr->psNext;
			free(psCurr);
		}
	}
}


// free all flag positions
void freeAllFlagPositions(void)
{
	FLAG_POSITION	*psNext;
	SDWORD			player;

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		while (apsFlagPosLists[player])
		{
			psNext = apsFlagPosLists[player]->psNext;
			free(apsFlagPosLists[player]);
			apsFlagPosLists[player] = psNext;
		}
	}
}


#ifdef DEBUG
// check all flag positions for duplicate delivery points
void checkFactoryFlags(void)
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

		FLAG_POSITION *psFlag = apsFlagPosLists[player];
		while (psFlag)
		{
			if ((psFlag->type == POS_DELIVERY) &&//check this is attached to a unique factory
			    (psFlag->factoryType != REPAIR_FLAG))
			{
				unsigned type = psFlag->factoryType;
				unsigned factory = psFlag->factoryInc;
				factoryDeliveryPointCheck[type].push_back(factory);
			}
			psFlag = psFlag->psNext;
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

// Find a base object from it's id
BASE_OBJECT *getBaseObjFromData(unsigned id, unsigned player, OBJECT_TYPE type)
{
	BASE_OBJECT		*psObj;
	DROID			*psTrans;

	for (int i = 0; i < 3; ++i)
	{
		psObj = nullptr;
		switch (i)
		{
		case 0:
			switch (type)
			{
			case OBJ_DROID: psObj = apsDroidLists[player]; break;
			case OBJ_STRUCTURE: psObj = apsStructLists[player]; break;
			case OBJ_FEATURE: psObj = apsFeatureLists[0];
			default: break;
			}
			break;
		case 1:
			switch (type)
			{
			case OBJ_DROID: psObj = mission.apsDroidLists[player]; break;
			case OBJ_STRUCTURE: psObj = mission.apsStructLists[player]; break;
			case OBJ_FEATURE: psObj = mission.apsFeatureLists[0]; break;
			default: break;
			}
			break;
		case 2:
			if (player == 0 && type == OBJ_DROID)
			{
				psObj = apsLimboDroids[0];
			}
			break;
		}

		while (psObj)
		{
			if (psObj->id == id)
			{
				return psObj;
			}
			// if transporter check any droids in the grp
			if ((psObj->type == OBJ_DROID) && isTransporter((DROID *)psObj))
			{
				for (psTrans = ((DROID *)psObj)->psGroup->psList; psTrans != nullptr; psTrans = psTrans->psGrpNext)
				{
					if (psTrans->id == id)
					{
						return (BASE_OBJECT *)psTrans;
					}
				}
			}
			psObj = psObj->psNext;
		}
	}
	ASSERT(false, "failed to find id %d for player %d", id, player);

	return nullptr;
}

// Find a base object from it's id
BASE_OBJECT *getBaseObjFromId(UDWORD id)
{
	unsigned int i;
	UDWORD			player;
	BASE_OBJECT		*psObj;
	DROID			*psTrans;

	for (i = 0; i < 7; ++i)
	{
		for (player = 0; player < MAX_PLAYERS; ++player)
		{
			switch (i)
			{
			case 0:
				psObj = (BASE_OBJECT *)apsDroidLists[player];
				break;
			case 1:
				psObj = (BASE_OBJECT *)apsStructLists[player];
				break;
			case 2:
				if (player == 0)
				{
					psObj = (BASE_OBJECT *)apsFeatureLists[0];
				}
				else
				{
					psObj = nullptr;
				}
				break;
			case 3:
				psObj = (BASE_OBJECT *)mission.apsDroidLists[player];
				break;
			case 4:
				psObj = (BASE_OBJECT *)mission.apsStructLists[player];
				break;
			case 5:
				if (player == 0)
				{
					psObj = (BASE_OBJECT *)mission.apsFeatureLists[0];
				}
				else
				{
					psObj = nullptr;
				}
				break;
			case 6:
				if (player == 0)
				{
					psObj = (BASE_OBJECT *)apsLimboDroids[0];
				}
				else
				{
					psObj = nullptr;
				}
				break;
			default:
				psObj = nullptr;
				break;
			}

			while (psObj)
			{
				if (psObj->id == id)
				{
					return psObj;
				}
				// if transporter check any droids in the grp
				if ((psObj->type == OBJ_DROID) && isTransporter((DROID *)psObj))
				{
					for (psTrans = ((DROID *)psObj)->psGroup->psList; psTrans != nullptr; psTrans = psTrans->psGrpNext)
					{
						if (psTrans->id == id)
						{
							return (BASE_OBJECT *)psTrans;
						}
					}
				}
				psObj = psObj->psNext;
			}
		}
	}
	ASSERT(!"couldn't find a BASE_OBJ with ID", "getBaseObjFromId() failed for id %d", id);

	return nullptr;
}

UDWORD getRepairIdFromFlag(FLAG_POSITION *psFlag)
{
	unsigned int i;
	UDWORD			player;
	STRUCTURE		*psObj;
	REPAIR_FACILITY	*psRepair;


	player = psFlag->player;

	//probably dont need to check mission list
	for (i = 0; i < 2; ++i)
	{
		switch (i)
		{
		case 0:
			psObj = (STRUCTURE *)apsStructLists[player];
			break;
		case 1:
			psObj = (STRUCTURE *)mission.apsStructLists[player];
			break;
		default:
			psObj = nullptr;
			break;
		}

		while (psObj)
		{
			if (psObj->pFunctionality)
			{
				if	(psObj->pStructureType->type == REF_REPAIR_FACILITY)
				{
					//check for matching delivery point
					psRepair = ((REPAIR_FACILITY *)psObj->pFunctionality);
					if (psRepair->psDeliveryPoint == psFlag)
					{
						return psObj->id;
					}
				}
			}
			psObj = psObj->psNext;
		}
	}
	ASSERT(!"unable to find repair id for FLAG_POSITION", "getRepairIdFromFlag() failed");

	return UDWORD_MAX;
}


// check a base object exists for an ID
bool checkValidId(UDWORD id)
{
	return getBaseObjFromId(id) != nullptr;
}


// integrity check the lists
#ifdef DEBUG
static void objListIntegCheck(void)
{
	SDWORD			player;
	BASE_OBJECT		*psCurr;

	for (player = 0; player < MAX_PLAYERS; player += 1)
	{
		for (psCurr = (BASE_OBJECT *)apsDroidLists[player]; psCurr; psCurr = psCurr->psNext)
		{
			ASSERT(psCurr->type == OBJ_DROID &&
			       (SDWORD)psCurr->player == player,
			       "objListIntegCheck: misplaced object in the droid list for player %d",
			       player);
		}
	}
	for (player = 0; player < MAX_PLAYERS; player += 1)
	{
		for (psCurr = (BASE_OBJECT *)apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
		{
			ASSERT(psCurr->type == OBJ_STRUCTURE &&
			       (SDWORD)psCurr->player == player,
			       "objListIntegCheck: misplaced %s(%p) in the structure list for player %d, is owned by %d",
			       objInfo(psCurr), psCurr, player, (int)psCurr->player);
		}
	}
	for (psCurr = (BASE_OBJECT *)apsFeatureLists[0]; psCurr; psCurr = psCurr->psNext)
	{
		ASSERT(psCurr->type == OBJ_FEATURE,
		       "objListIntegCheck: misplaced object in the feature list");
	}
	for (psCurr = (BASE_OBJECT *)psDestroyedObj; psCurr; psCurr = psCurr->psNext)
	{
		ASSERT(psCurr->died > 0, "objListIntegCheck: Object in destroyed list but not dead!");
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
		for (DROID *psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			(*droids)++;
			if (isTransporter(psDroid))
			{
				DROID *psTrans = psDroid->psGroup->psList;

				for (psTrans = psTrans->psGrpNext; psTrans != nullptr; psTrans = psTrans->psGrpNext)
				{
					(*droids)++;
				}
			}
		}

		for (STRUCTURE *psStruct = apsStructLists[i]; psStruct; psStruct = psStruct->psNext)
		{
			(*structures)++;
		}
	}

	for (FEATURE *psFeat = apsFeatureLists[0]; psFeat; psFeat = psFeat->psNext)
	{
		(*features)++;
	}
}
