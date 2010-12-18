/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
#include "lib/sound/audio.h"
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

#ifdef DEBUG
static SDWORD factoryDeliveryPointCheck[MAX_PLAYERS][NUM_FLAG_TYPES][MAX_FACTORY];
#endif

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
BASE_OBJECT		*psDestroyedObj=NULL;

/* Forward function declarations */
#ifdef DEBUG
static void objListIntegCheck(void);
#endif


/* Initialise the object heaps */
BOOL objmemInitialise(void)
{
	// reset the object ID number
	unsynchObjID = OBJ_ID_INIT/2;  // /2 so that object IDs start around OBJ_ID_INIT*8, in case that's important when loading maps.
	synchObjID   = OBJ_ID_INIT*4;  // *4 so that object IDs start around OBJ_ID_INIT*8, in case that's important when loading maps.

	return true;
}

/* Release the object heaps */
void objmemShutdown(void)
{
}

/* Remove an object from the destroyed list, finally freeing its memory
 * Hopefully by this time, no pointers still refer to it! */
static void objmemDestroy(BASE_OBJECT *psObj)
{
	switch (psObj->type)
	{
		case OBJ_DROID:
			debug(LOG_MEMORY, "freeing droid at %p", psObj);
			if (!droidCheckReferences((DROID *)psObj))
			{
				return;
			}
			droidRelease((DROID *)psObj);
			break;

		case OBJ_STRUCTURE:
			debug(LOG_MEMORY, "freeing structure at %p", psObj);
			if (!structureCheckReferences((STRUCTURE *)psObj))
			{
				return;
			}
			structureRelease((STRUCTURE *)psObj);
			break;

		case OBJ_FEATURE:
			debug(LOG_MEMORY, "freeing feature at %p", psObj);
			featureRelease((FEATURE *)psObj);
			break;

		default:
			ASSERT(!"unknown object type", "objmemDestroy: unknown object type in destroyed list at 0x%p", psObj);
	}
	// Make sure to get rid of some final references in the sound code to this object first
	audio_RemoveObj(psObj);

	visRemoveVisibility(psObj);
	free(psObj->watchedTiles);
#ifdef DEBUG
	psObj->type = (OBJECT_TYPE)(psObj->type + 1000000000);  // Hopefully this will trigger an assert              if someone uses the freed object.
	psObj->player += 1000000000;                            // Hopefully this will trigger an assert and/or crash if someone uses the freed object.
	psObj->psNext = psObj;                                  // Hopefully this will trigger an infinite loop       if someone uses the freed object.
#endif //DEBUG
	free(psObj);
	debug(LOG_MEMORY, "BASE_OBJECT* 0x%p is freed.", psObj);
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
	if (psDestroyedObj != NULL)
	{
		scrvUpdateBasePointers();
	}

	/* Go through the destroyed objects list looking for objects that
	   were destroyed before this turn */

	/* First remove the objects from the start of the list */
	while (psDestroyedObj != NULL && psDestroyedObj->died != gameTime)
	{
		psNext = psDestroyedObj->psNext;
		objmemDestroy(psDestroyedObj);
		psDestroyedObj = psNext;
	}

	/* Now see if there are any further down the list
	Keep track of the previous object to set its Next pointer*/
	for(psCurr = psPrev = psDestroyedObj; psCurr != NULL; psCurr = psNext)
	{
		psNext = psCurr->psNext;
		if (psCurr->died != gameTime)
		{
			objmemDestroy(psCurr);

			/*set the linked list up - you will never be deleting the top
			of the list, so don't have to check*/
			psPrev->psNext = psNext;
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
			psCBObjDestroyed = NULL;

			psPrev = psCurr;
		}
	}
}

uint32_t generateNewObjectId(void)
{
	return unsynchObjID++*MAX_PLAYERS*2 + selectedPlayer*2;  // Was taken from createObject, where 'player' was used instead of 'selectedPlayer'. Hope there are no stupid hacks that try to recover 'player' from the last 3 bits.
}

uint32_t generateSynchronisedObjectId(void)
{
	uint32_t ret = synchObjID++*2 + 1;
	syncDebug("New objectId = %u", ret);
	return ret;
}

/**************************************************************************************
 *
 * Inlines for the object memory functions
 * The code is the same for the different object types only the pointer types
 * change.
 */

/* Creating a new object
 */
static inline BASE_OBJECT* createObject(UDWORD player, OBJECT_TYPE objType)
{
	BASE_OBJECT* newObject;

	ASSERT(player < MAX_PLAYERS,
	       "createObject: invalid player number");

	switch (objType)
	{
		case OBJ_FEATURE:
			newObject = malloc(sizeof(FEATURE));
			break;

		case OBJ_STRUCTURE:
			newObject = malloc(sizeof(STRUCTURE));
			break;

		case OBJ_DROID:
			newObject = malloc(sizeof(DROID));
			break;

		default:
			ASSERT(!"unknown object type", "createObject: unknown object type");
			return NULL;
	}

	if (newObject == NULL)
	{
		debug(LOG_ERROR, "Out of memory");
		return NULL;
	}

	newObject->type = objType;
	newObject->id = generateSynchronisedObjectId();
	newObject->player = (UBYTE)player;
	newObject->died = 0;
	newObject->psNextFunc = NULL;
	newObject->numWatchedTiles = 0;
	newObject->watchedTiles = NULL;
	newObject->born = gameTime;

	return newObject;
}

/* Add the object to its list
 * \param list is a pointer to the object list
 */
static inline void addObjectToList(BASE_OBJECT *list[], BASE_OBJECT *object, int player)
{
	ASSERT(object != NULL, "Invalid pointer");

	// Prepend the object to the top of the list
	//object->psNext = list[player];
	//list[player] = object;
	// Do the above two lines without breaking strict-aliasing rules. (Otherwise things may break when compiled with optimisations.)
	memcpy(&object->psNext, &list[player], sizeof(BASE_OBJECT *));
	memcpy(&list[player], &object, sizeof(BASE_OBJECT *));
}

/* Add the object to its list
 * \param list is a pointer to the object list
 */
static inline void addObjectToFuncList(BASE_OBJECT *list[], BASE_OBJECT *object, int player)
{
	ASSERT(object != NULL, "Invalid pointer");
	ASSERT_OR_RETURN(, object->psNextFunc == NULL, "%s(%p) is already in a function list!", objInfo(object), object);

	// Prepend the object to the top of the list
	// memcpy is to avoid strict-aliasing error. Will probably break if using multiple inheritance.
	memcpy(&object->psNextFunc, &list[player], sizeof(void *));  // object->psNextFunc = list[player];
	memcpy(&list[player], &object, sizeof(void *));              // list[player] = object;
}

/* Move an object from the active list to the destroyed list.
 * \param list is a pointer to the object list
 * \param del is a pointer to the object to remove
 */
static inline void destroyObject(BASE_OBJECT* list[], BASE_OBJECT* object)
{
	BASE_OBJECT *psPrev = NULL, *psCurr;

	ASSERT(object != NULL,
	       "destroyObject: Invalid pointer");

	// If the message to remove is the first one in the list then mark the next one as the first
	if (list[object->player] == object)
	{
		list[object->player] = list[object->player]->psNext;
		((BASE_OBJECT *)object)->psNext = psDestroyedObj;
		psDestroyedObj = (BASE_OBJECT *)object;
		object->died = gameTime;
		return;
	}

	// Iterate through the list and find the item before the object to delete
	for(psCurr = list[object->player]; (psCurr != object) && (psCurr != NULL); psCurr = psCurr->psNext)
	{
		psPrev = psCurr;
	}

	ASSERT(psCurr != NULL,
	       "destroyObject: object not found in list");

	if (psCurr != NULL)
	{
		// Modify the "next" pointer of the previous item to
		// point to the "next" item of the item to delete.
		psPrev->psNext = psCurr->psNext;

		// Prepend the object to the destruction list
		((BASE_OBJECT *)object)->psNext = psDestroyedObj;
		psDestroyedObj = (BASE_OBJECT *)object;

		// Set destruction time
		object->died = gameTime;
	}
}

/* Remove an object from the active list
 * \param list is a pointer to the object list
 * \param remove is a pointer to the object to remove
 * \param type is the type of the object
 */
static inline void removeObjectFromList(BASE_OBJECT *list[], BASE_OBJECT *object, int player)
{
	BASE_OBJECT *psPrev = NULL, *psCurr;

	ASSERT_OR_RETURN(, object != NULL, "Invalid pointer");

	// If the message to remove is the first one in the list then mark the next one as the first
	if (list[player] == object)
	{
		list[player] = list[player]->psNext;
		return;
	}
	
	// Iterate through the list and find the item before the object to delete
	for(psCurr = list[player]; (psCurr != object) && (psCurr != NULL); psCurr = psCurr->psNext)
	{
		psPrev = psCurr;
	}

	ASSERT_OR_RETURN(, psCurr != NULL, "Object %p not found in list", object);

	// Modify the "next" pointer of the previous item to
	// point to the "next" item of the item to delete.
	psPrev->psNext = psCurr->psNext;
}

/* Remove an object from the relevant function list. An object can only be in one function list at a time!
 * \param list is a pointer to the object list
 * \param remove is a pointer to the object to remove
 * \param type is the type of the object
 */
static inline void removeObjectFromFuncList(BASE_OBJECT *list[], BASE_OBJECT *object, int player)
{
	BASE_OBJECT *psPrev = NULL, *psCurr;
	BASE_OBJECT *listPlayer;

	ASSERT_OR_RETURN(, object != NULL, "Invalid pointer");

	memcpy(&listPlayer, &list[player], sizeof(void *));

	// If the message to remove is the first one in the list then mark the next one as the first
	if (listPlayer == object)
	{
		memcpy(&list[player], &listPlayer->psNextFunc, sizeof(void *));  // list[player] = list[player]->psNextFunc;
		object->psNextFunc = NULL;
		return;
	}
	
	// Iterate through the list and find the item before the object to delete
	for(psCurr = listPlayer; psCurr != object && psCurr != NULL; psCurr = psCurr->psNextFunc)
	{
		psPrev = psCurr;
	}

	ASSERT_OR_RETURN(, psCurr != NULL, "Object %p not found in list", object);

	// Modify the "next" pointer of the previous item to
	// point to the "next" item of the item to delete.
	psPrev->psNextFunc = psCurr->psNextFunc;
	object->psNextFunc = NULL;
}

static inline BASE_OBJECT* findObjectInList(BASE_OBJECT list[], UDWORD idNum)
{
	BASE_OBJECT *psCurr;
	for(psCurr = list; psCurr != NULL; psCurr = psCurr->psNext)
	{
		if (psCurr->id == (idNum))
		{
			return psCurr;
		}
	}

	return NULL;
}

// Necessary for a nice looking cast in calls to releaseAllObjectsInList
typedef void (*OBJECT_DESTRUCTOR)(BASE_OBJECT*);

static inline void releaseAllObjectsInList(BASE_OBJECT *list[], OBJECT_DESTRUCTOR objectDestructor)
{
	UDWORD i;
	BASE_OBJECT *psCurr, *psNext;

	// Iterate through all players' object lists
	for(i=0; i<MAX_PLAYERS; i++)
	{
		// Iterate through all objects in list
		for(psCurr = list[i]; psCurr != NULL; psCurr = psNext)
		{
	 		psNext = psCurr->psNext;

			// Call a specialized destruction function
			// (will do all cleanup except for releasing memory of object)
			objectDestructor(psCurr);
			// FIXME: the next call is disabled for now, yes, it will leak memory again.
			// issue is with campaign games, and the swapping pointers 'trick' Pumpkin uses.
			//	visRemoveVisibility(psCurr);
			// Release object's memory
			free(psCurr);
		}
		list[i] = NULL;
	}
}

/***************************************************************************************
 *
 * The actual object memory management functions for the different object types
 */

/***************************  DROID  *********************************/

/* Create a new droid */
DROID* createDroid(UDWORD player)
{
	return (DROID*)createObject(player, OBJ_DROID);
}

/* add the droid to the Droid Lists */
void addDroid(DROID *psDroidToAdd, DROID *pList[MAX_PLAYERS])
{
	DROID_GROUP	*psGroup;

	addObjectToList((BASE_OBJECT**)pList, (BASE_OBJECT*)psDroidToAdd, psDroidToAdd->player);

	/* Whenever a droid gets added to a list other than the current list
	 * its died flag is set to NOT_CURRENT_LIST so that anything targetting
	 * it will cancel itself - HACK?! */
	if (pList[psDroidToAdd->player] == apsDroidLists[psDroidToAdd->player])
	{
		psDroidToAdd->died = false;
		if (psDroidToAdd->droidType == DROID_SENSOR)
		{
			addObjectToFuncList(apsSensorList, (BASE_OBJECT*)psDroidToAdd, 0);
		}

		// commanders have to get their group back
		if (psDroidToAdd->droidType == DROID_COMMAND)
		{
			grpCreate(&psGroup);
			if (psGroup)
			{
				grpJoin(psGroup, psDroidToAdd);
			}
		}
	}
	else if (pList[psDroidToAdd->player] == mission.apsDroidLists[psDroidToAdd->player])
	{
		if (psDroidToAdd->droidType == DROID_SENSOR)
		{
			addObjectToFuncList(mission.apsSensorList, (BASE_OBJECT*)psDroidToAdd, 0);
		}
	}
}

/* Destroy a droid */
void killDroid(DROID *psDel)
{
	int i;

	ASSERT( psDel->type == OBJ_DROID,
		"killUnit: pointer is not a unit" );
	ASSERT( psDel->player < MAX_PLAYERS,
		"killUnit: invalid player for unit" );

	setDroidTarget(psDel, NULL);
	for (i = 0; i < DROID_MAXWEAPS; i++)
	{
		setDroidActionTarget(psDel, NULL, i);
	}
	setDroidBase(psDel, NULL);
	if (psDel->droidType == DROID_SENSOR)
	{
		removeObjectFromFuncList(apsSensorList, (BASE_OBJECT*)psDel, 0);
	}
	free(psDel->gameCheckDroid);

	destroyObject((BASE_OBJECT**)apsDroidLists, (BASE_OBJECT*)psDel);
}

/* Remove all droids */
void freeAllDroids(void)
{
	releaseAllObjectsInList((BASE_OBJECT**)apsDroidLists, (OBJECT_DESTRUCTOR)droidRelease);
}

/*Remove a single Droid from a list*/
void removeDroid(DROID *psDroidToRemove, DROID *pList[MAX_PLAYERS])
{
	ASSERT( psDroidToRemove->type == OBJ_DROID,
		"removeUnit: pointer is not a unit" );
	ASSERT( psDroidToRemove->player < MAX_PLAYERS,
		"removeUnit: invalid player for unit" );
	removeObjectFromList((BASE_OBJECT**)pList, (BASE_OBJECT*)psDroidToRemove, psDroidToRemove->player);

	/* Whenever a droid is removed from the current list its died
	 * flag is set to NOT_CURRENT_LIST so that anything targetting
	 * it will cancel itself, and we know it is not really on the map. */
	if (pList[psDroidToRemove->player] == apsDroidLists[psDroidToRemove->player])
	{
		if (psDroidToRemove->droidType == DROID_SENSOR)
		{
			removeObjectFromFuncList(apsSensorList, (BASE_OBJECT*)psDroidToRemove, 0);
		}
		psDroidToRemove->died = NOT_CURRENT_LIST;
	}
	else if (pList[psDroidToRemove->player] == mission.apsDroidLists[psDroidToRemove->player])
	{
		if (psDroidToRemove->droidType == DROID_SENSOR)
		{
			removeObjectFromFuncList(mission.apsSensorList, (BASE_OBJECT*)psDroidToRemove, 0);
		}
	}
}

/*Removes all droids that may be stored in the mission lists*/
void freeAllMissionDroids(void)
{
	releaseAllObjectsInList((BASE_OBJECT**)mission.apsDroidLists, (OBJECT_DESTRUCTOR)droidRelease);
}

/*Removes all droids that may be stored in the limbo lists*/
void freeAllLimboDroids(void)
{
	releaseAllObjectsInList((BASE_OBJECT**)apsLimboDroids, (OBJECT_DESTRUCTOR)droidRelease);
}

/**************************  STRUCTURE  *******************************/

/* Create a new structure */
STRUCTURE* createStruct(UDWORD player)
{
	return (STRUCTURE*)createObject(player, OBJ_STRUCTURE);
}

/* add the structure to the Structure Lists */
void addStructure(STRUCTURE *psStructToAdd)
{
	addObjectToList((BASE_OBJECT**)apsStructLists, (BASE_OBJECT*)psStructToAdd, psStructToAdd->player);
	if (psStructToAdd->pStructureType->pSensor
	    && psStructToAdd->pStructureType->pSensor->location == LOC_TURRET)
	{
		addObjectToFuncList(apsSensorList, (BASE_OBJECT*)psStructToAdd, 0);
	}
	else if (psStructToAdd->pStructureType->type == REF_RESOURCE_EXTRACTOR)
	{
		addObjectToFuncList((BASE_OBJECT **)apsExtractorLists, (BASE_OBJECT *)psStructToAdd, psStructToAdd->player);
	}
}

/* Destroy a structure */
void killStruct(STRUCTURE *psBuilding)
{
	int i;

	ASSERT( psBuilding->type == OBJ_STRUCTURE,
		"killStruct: pointer is not a droid" );
	ASSERT( psBuilding->player < MAX_PLAYERS,
		"killStruct: invalid player for stucture" );

	if (psBuilding->pStructureType->pSensor
	    && psBuilding->pStructureType->pSensor->location == LOC_TURRET)
	{
		removeObjectFromFuncList(apsSensorList, (BASE_OBJECT*)psBuilding, 0);
	}
	else if (psBuilding->pStructureType->type == REF_RESOURCE_EXTRACTOR)
	{
		removeObjectFromFuncList((BASE_OBJECT **)apsExtractorLists, (BASE_OBJECT *)psBuilding, psBuilding->player);
	}

	for (i = 0; i < STRUCT_MAXWEAPS; i++)
	{
		setStructureTarget(psBuilding, NULL, i, ORIGIN_UNKNOWN);
	}

	if (psBuilding->pFunctionality != NULL)
	{
		if (StructIsFactory(psBuilding))
		{
			FACTORY *psFactory = (FACTORY *)psBuilding->pFunctionality;

			// remove any commander from the factory
			if (psFactory->psCommander != NULL)
			{
				assignFactoryCommandDroid(psBuilding, NULL);
			}

			// remove any assembly points
			if (psFactory->psAssemblyPoint != NULL)
			{
				removeFlagPosition(psFactory->psAssemblyPoint);
				psFactory->psAssemblyPoint = NULL;
			}
		}
		else if (psBuilding->pStructureType->type == REF_REPAIR_FACILITY)
		{
			REPAIR_FACILITY *psRepair = (REPAIR_FACILITY *)psBuilding->pFunctionality;

			if (psRepair->psDeliveryPoint)
			{
				// free up repair fac stuff
				removeFlagPosition(psRepair->psDeliveryPoint);
				psRepair->psDeliveryPoint = NULL;
			}
		}
	}

	destroyObject((BASE_OBJECT**)apsStructLists, (BASE_OBJECT*)psBuilding);
}

/* Remove heapall structures */
void freeAllStructs(void)
{
	releaseAllObjectsInList((BASE_OBJECT**)apsStructLists, (OBJECT_DESTRUCTOR)structureRelease);
}

/*Remove a single Structure from a list*/
void removeStructureFromList(STRUCTURE *psStructToRemove, STRUCTURE *pList[MAX_PLAYERS])
{
	ASSERT( psStructToRemove->type == OBJ_STRUCTURE,
		"removeStructureFromList: pointer is not a structure" );
	ASSERT( psStructToRemove->player < MAX_PLAYERS,
		"removeStructureFromList: invalid player for structure" );
	removeObjectFromList((BASE_OBJECT**)pList, (BASE_OBJECT*)psStructToRemove, psStructToRemove->player);
	if (psStructToRemove->pStructureType->pSensor
	    && psStructToRemove->pStructureType->pSensor->location == LOC_TURRET)
	{
		removeObjectFromFuncList(apsSensorList, (BASE_OBJECT*)psStructToRemove, 0);
	}
	else if (psStructToRemove->pStructureType->type == REF_RESOURCE_EXTRACTOR)
	{
		removeObjectFromFuncList((BASE_OBJECT **)apsExtractorLists, (BASE_OBJECT *)psStructToRemove, psStructToRemove->player);
	}
}

/**************************  FEATURE  *********************************/

/* Create a new Feature */
FEATURE* createFeature()
{
	return (FEATURE*)createObject(0, OBJ_FEATURE);
}

/* add the feature to the Feature Lists */
void addFeature(FEATURE *psFeatureToAdd)
{
	addObjectToList((BASE_OBJECT**)apsFeatureLists, (BASE_OBJECT*)psFeatureToAdd, 0);
	if (psFeatureToAdd->psStats->subType == FEAT_OIL_RESOURCE)
	{
		addObjectToFuncList((BASE_OBJECT **)apsOilList, (BASE_OBJECT *)psFeatureToAdd, 0);
	}
}

/* Destroy a feature */
// set the player to 0 since features have player = maxplayers+1. This screws up destroyObject
// it's a bit of a hack, but hey, it works
void killFeature(FEATURE *psDel)
{
	ASSERT( psDel->type == OBJ_FEATURE,
		"killFeature: pointer is not a feature" );
	psDel->player = 0;
	destroyObject((BASE_OBJECT**)apsFeatureLists, (BASE_OBJECT*)psDel);

	if (psDel->psStats->subType == FEAT_OIL_RESOURCE)
	{
		removeObjectFromFuncList((BASE_OBJECT **)apsOilList, (BASE_OBJECT *)psDel, 0);
	}
}

/* Remove all features */
void freeAllFeatures(void)
{
	releaseAllObjectsInList((BASE_OBJECT**)apsFeatureLists, (OBJECT_DESTRUCTOR)featureRelease);
}

/**************************  FLAG_POSITION ********************************/

/* Create a new Flag Position */
BOOL createFlagPosition(FLAG_POSITION **ppsNew, UDWORD player)
{
	ASSERT( player<MAX_PLAYERS, "createFlagPosition: invalid player number" );

	*ppsNew = malloc(sizeof(FLAG_POSITION));
	if (*ppsNew == NULL)
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
	ASSERT( psFlagPosToAdd != NULL,
		"addFlagPosition: Invalid FlagPosition pointer" );
	ASSERT(psFlagPosToAdd->coords.x != ~0, "flag has invalid position");

	psFlagPosToAdd->psNext = apsFlagPosLists[psFlagPosToAdd->player];
	apsFlagPosLists[psFlagPosToAdd->player] = psFlagPosToAdd;
 }

/* Remove a Flag Position from the Lists */
void removeFlagPosition(FLAG_POSITION *psDel)
{
	FLAG_POSITION		*psPrev=NULL, *psCurr;

	ASSERT( psDel != NULL,
		"removeFlagPosition: Invalid Flag Positionpointer" );

	if (apsFlagPosLists[psDel->player] == psDel)
	{
		apsFlagPosLists[psDel->player] = apsFlagPosLists[psDel->player]->psNext;
		free(psDel);
	}
	else
	{
		for(psCurr = apsFlagPosLists[psDel->player]; (psCurr != psDel) &&
			(psCurr != NULL); psCurr = psCurr->psNext)
		{
			psPrev = psCurr;
		}
		if (psCurr != NULL)
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

	for(player=0; player<MAX_PLAYERS; player++)
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
	FLAG_POSITION	*psFlag;
	SDWORD			player, type, factory;

	//clear the check array
	for(player=0; player<MAX_PLAYERS; player++)
	{
		//for(type=0; type<NUM_FACTORY_TYPES; type++)
        for(type=0; type<NUM_FLAG_TYPES; type++)
		{
			for(factory=0; factory<MAX_FACTORY; factory++)
			{
				factoryDeliveryPointCheck[player][type][factory] = 0;
			}
		}
	}

	//check the flags
	for(player=0; player<MAX_PLAYERS; player++)
	{
		psFlag = apsFlagPosLists[player];
		while (psFlag)
		{
			if ((psFlag->type == POS_DELIVERY) &&//check this is attached to a unique factory
				(psFlag->factoryType != REPAIR_FLAG))
			{
				type = psFlag->factoryType;
				factory = psFlag->factoryInc;
				ASSERT( factoryDeliveryPointCheck[player][type][factory] == 0,"DUPLICATE FACTORY DELIVERY POINT FOUND" );
				factoryDeliveryPointCheck[player][type][factory] = 1;
			}
			psFlag = psFlag->psNext;
		}
	}
}
#endif


/**************************  OBJECT ACCESS FUNCTIONALITY ********************************/

// Find a base object from it's id
BASE_OBJECT *getBaseObjFromId(UDWORD id)
{
	unsigned int i;
	UDWORD			player;
	BASE_OBJECT		*psObj;
	DROID			*psTrans;

	for(i = 0; i < 7; ++i)
	{
		for(player = 0; player < MAX_PLAYERS; ++player)
		{
			switch (i)
			{
				case 0:
					psObj=(BASE_OBJECT *)apsDroidLists[player];
					break;
				case 1:
					psObj=(BASE_OBJECT *)apsStructLists[player];
					break;
				case 2:
					if (player == 0)
					{
						psObj=(BASE_OBJECT *)apsFeatureLists[0];
					}
					else
					{
						psObj = NULL;
					}
					break;
				case 3:
					psObj=(BASE_OBJECT *)mission.apsDroidLists[player];
					break;
				case 4:
					psObj=(BASE_OBJECT *)mission.apsStructLists[player];
					break;
				case 5:
					if (player == 0)
					{
						psObj=(BASE_OBJECT *)mission.apsFeatureLists[0];
					}
					else
					{
						psObj = NULL;
					}
					break;
				case 6:
					if (player == 0)
					{
						psObj=(BASE_OBJECT *)apsLimboDroids[0];
					}
					else
					{
						psObj = NULL;
					}
					break;
				default:
					psObj = NULL;
					break;
			}

			while (psObj)
			{
				if (psObj->id == id)
				{
					return psObj;
				}
				// if transporter check any droids in the grp
				if ((psObj->type == OBJ_DROID) && (((DROID*)psObj)->droidType == DROID_TRANSPORTER))
				{
					for(psTrans = ((DROID*)psObj)->psGroup->psList; psTrans != NULL; psTrans = psTrans->psGrpNext)
					{
						if (psTrans->id == id)
						{
							return (BASE_OBJECT*)psTrans;
						}
					}
				}
				psObj = psObj->psNext;
			}
		}
	}
	ASSERT(!"couldn't find a BASE_OBJ with ID", "getBaseObjFromId() failed for id %d", id);

	return NULL;
}

UDWORD getRepairIdFromFlag(FLAG_POSITION *psFlag)
{
	unsigned int i;
	UDWORD			player;
	STRUCTURE		*psObj;
	REPAIR_FACILITY	*psRepair;


	player = psFlag->player;

	//probably dont need to check mission list
	for(i = 0; i < 2; ++i)
	{
		switch (i)
		{
			case 0:
				psObj=(STRUCTURE *)apsStructLists[player];
				break;
			case 1:
				psObj=(STRUCTURE *)mission.apsStructLists[player];
				break;
			default:
				psObj = NULL;
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
BOOL checkValidId(UDWORD id)
{
	return getBaseObjFromId(id) != NULL;
}


// integrity check the lists
#ifdef DEBUG
static void objListIntegCheck(void)
{
	SDWORD			player;
	BASE_OBJECT		*psCurr;

	for(player = 0; player <MAX_PLAYERS; player += 1)
	{
		for(psCurr = (BASE_OBJECT*)apsDroidLists[player]; psCurr; psCurr=psCurr->psNext)
		{
			ASSERT( psCurr->type == OBJ_DROID &&
					(SDWORD)psCurr->player == player,
					"objListIntegCheck: misplaced object in the droid list for player %d",
					player );
		}
	}
	for(player = 0; player <MAX_PLAYERS; player += 1)
	{
		for(psCurr = (BASE_OBJECT*)apsStructLists[player]; psCurr; psCurr=psCurr->psNext)
		{
			ASSERT( psCurr->type == OBJ_STRUCTURE &&
					(SDWORD)psCurr->player == player,
					"objListIntegCheck: misplaced %s(%p) in the structure list for player %d, is owned by %d",
					objInfo(psCurr), psCurr, player, (int)psCurr->player);
		}
	}
	for(psCurr = (BASE_OBJECT*)apsFeatureLists[0]; psCurr; psCurr=psCurr->psNext)
	{
		ASSERT( psCurr->type == OBJ_FEATURE,
				"objListIntegCheck: misplaced object in the feature list" );
	}
	for (psCurr = (BASE_OBJECT*)psDestroyedObj; psCurr; psCurr = psCurr->psNext)
	{
		ASSERT( psCurr->died > 0, "objListIntegCheck: Object in destroyed list but not dead!" );
	}
}
#endif

void objCount(int *droids, int *structures, int *features)
{
	int 		i;
	DROID		*psDroid;
	STRUCTURE	*psStruct;
	FEATURE		*psFeat;

	*droids = 0;
	*structures = 0;
	*features = 0;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		for (psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			(*droids)++;
			if (psDroid->droidType == DROID_TRANSPORTER)
			{
				DROID *psTrans = psDroid->psGroup->psList;

				for(psTrans = psTrans->psGrpNext; psTrans != NULL; psTrans = psTrans->psGrpNext)
				{
					(*droids)++;
				}
			}
		}

		for (psStruct = apsStructLists[i]; psStruct; psStruct = psStruct->psNext)
		{
			(*structures)++;
		}
	}

	for (psFeat = apsFeatureLists[0]; psFeat; psFeat = psFeat->psNext)
	{
		(*features)++;
	}
}
