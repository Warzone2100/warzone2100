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
/*
 * ObjMem.c
 *
 * Object memory management functions.
 *
 */
#include <string.h>

//#define DEBUG_GROUP1
#include "lib/framework/frame.h"
#include "objects.h"
#include "deliverance.h"
#include "lib/gamelib/gtime.h"
#include "hci.h"
#include "map.h"
#include "power.h"
#include "lib/script/script.h"
#include "scriptvals.h"
#include "scripttabs.h"
#include "scriptcb.h"
#include "mission.h"

/* Allocation sizes for the droid, structure and feature heaps */


#define DROID_INIT		400
#define STRUCTURE_INIT	200

#define DROID_EXT		15
#define STRUCTURE_EXT	15
#define STRUCTFUNC_INIT	50
#define STRUCTFUNC_EXT	5
#define FEATURE_INIT	145		// Surely this can be reduced.
#define FEATURE_EXT		15
#define FLAGPOS_INIT	20
#define FLAGPOS_EXT		5
#define TEMPLATE_INIT	120		// was 40 but this there is 116 templates in template.txt alone ... Arse ... 84 bytes each as well ... arse ...
#define TEMPLATE_EXT	10




/* The memory heaps for the different object types */
OBJ_HEAP		*psDroidHeap, *psStructHeap, *psFeatureHeap;
/*Heap for structure functionality*/
OBJ_HEAP		*psStructFuncHeap;
/* The memory heap for the Flag Postions */
OBJ_HEAP		*psFlagPosHeap;
// the memory heap for templates
OBJ_HEAP		*psTemplateHeap;


//SDWORD	factoryDeliveryPointCheck[MAX_PLAYERS][NUM_FACTORY_TYPES][MAX_FACTORY];
SDWORD	factoryDeliveryPointCheck[MAX_PLAYERS][NUM_FLAG_TYPES][MAX_FACTORY];
// the initial value for the object ID
#define OBJ_ID_INIT		20000


/* The id number for the next object allocated
 * Each object will have a unique id number irrespective of type
 */
UDWORD	objID;


/* The lists of objects allocated */
DROID			*apsDroidLists[MAX_PLAYERS];
STRUCTURE		*apsStructLists[MAX_PLAYERS];
FEATURE			*apsFeatureLists[MAX_PLAYERS];		// Only player zero is valid for
													// features
/* The list of structure functionality's required*/
FUNCTIONALITY	*apsStructFuncLists[MAX_PLAYERS];
/*The list of Flag Positions allocated */
FLAG_POSITION	*apsFlagPosLists[MAX_PLAYERS];

/* The list of destroyed objects */
BASE_OBJECT		*psDestroyedObj=NULL;


#if defined(DEBUG)
// store a record of units that recently died
typedef struct _morgue
{
	BASE_OBJECT		*pMem;
	SDWORD			type;
	UDWORD			id;
	UDWORD			player;
	char			aName[50];
	UDWORD			died;
} MORGUE;

#define MAX_MORGUE		150
MORGUE	asMorgue[MAX_MORGUE];

SDWORD		morgueEnd;

void initMorgue(void)
{
	memset(asMorgue, 0, sizeof(asMorgue));
	morgueEnd = 0;
}

void embalm(BASE_OBJECT *psDead)
{
	MORGUE	*psSlot;
	char	*pName;

	psSlot = asMorgue + morgueEnd;

	memset(psSlot, 0, sizeof(MORGUE));

	psSlot->pMem = psDead;
	psSlot->type = psDead->type;
	psSlot->id = psDead->id;
	psSlot->player = psDead->player;
	psSlot->died = psDead->died;

	pName = NULL;
	switch (psDead->type)
	{
	case OBJ_BULLET:
	case OBJ_TARGET:
		/* Was originally not handled */
		debug(LOG_ERROR, "src/objmem.c:embalm(): Unhandled dead object type");
		break;
	case OBJ_DROID:
		pName = ((DROID *)psDead)->aName;
		break;
	case OBJ_STRUCTURE:
		pName = ((STRUCTURE *)psDead)->pStructureType->pName;
		break;
	case OBJ_FEATURE:
		pName = ((FEATURE *)psDead)->psStats->pName;
		break;
	}

	if (pName != NULL)
	{
		strncpy(psSlot->aName, pName, 50);
		psSlot->aName[50] = 0;
	}
	else
	{
		psSlot->aName[0] = 0;
	}

	morgueEnd += 1;
	if (morgueEnd >= MAX_MORGUE)
	{
		morgueEnd = 0;
	}
}


#define INIT_MORGUE()	initMorgue()
#define EMBALM(x)		embalm(x)


#else
#define INIT_MORGUE()
#define EMBALM(x)
#endif

void objListIntegCheck(void);

/* Initialise the object heaps */
BOOL objmemInitialise(void)
{
	if (!HEAP_CREATE(&psDroidHeap, sizeof(DROID), DROID_INIT, DROID_EXT))
	{
		return FALSE;
	}

	if (!HEAP_CREATE(&psStructHeap, sizeof(STRUCTURE), STRUCTURE_INIT, STRUCTURE_EXT))
	{
		return FALSE;
	}

	if (!HEAP_CREATE(&psStructFuncHeap, sizeof(FUNCTIONALITY), STRUCTFUNC_INIT,
		STRUCTFUNC_EXT))
	{
		return FALSE;
	}

	if (!HEAP_CREATE(&psFeatureHeap, sizeof(FEATURE), FEATURE_INIT, FEATURE_EXT))
	{
		return FALSE;
	}

	if (!HEAP_CREATE(&psFlagPosHeap, sizeof(FLAG_POSITION), FLAGPOS_INIT, FLAGPOS_EXT))
	{
		return FALSE;
	}

	if (!HEAP_CREATE(&psTemplateHeap, sizeof(DROID_TEMPLATE), TEMPLATE_INIT, TEMPLATE_EXT))
	{
		return FALSE;
	}

	// reset the object ID number
	objID = OBJ_ID_INIT;

	INIT_MORGUE();

	return TRUE;
}

/* Release the object heaps */
void objmemShutdown(void)
{
	HEAP_DESTROY(psDroidHeap);
	HEAP_DESTROY(psStructHeap);
	HEAP_DESTROY(psStructFuncHeap);
	HEAP_DESTROY(psFeatureHeap);
	HEAP_DESTROY(psFlagPosHeap);
	HEAP_DESTROY(psTemplateHeap);
}

/* General housekeeping for the object system */
void objmemUpdate(void)
{
	BASE_OBJECT		*psCurr, *psNext, *psPrev;

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

		EMBALM(psDestroyedObj);

		switch (psDestroyedObj->type)
		{
		case OBJ_DROID:
			debug( LOG_MEMORY, "objmemUpdate: freeing droid\n");
			droidRelease((DROID *)psDestroyedObj);
			HEAP_FREE(psDroidHeap, psDestroyedObj);
			break;
		case OBJ_STRUCTURE:
			debug( LOG_MEMORY, "objmemUpdate: freeing structure\n");
			structureRelease((STRUCTURE *)psDestroyedObj);
			HEAP_FREE(psStructHeap, psDestroyedObj);
			break;
		case OBJ_FEATURE:
			featureRelease((FEATURE *)psDestroyedObj);
			HEAP_FREE(psFeatureHeap, psDestroyedObj);
			break;
		default:
			ASSERT( FALSE, "objmemUpdate: unknown object type in destroyed list" );
		}
		psDestroyedObj = psNext;
	}

	/* Now see if there are any further down the list
	Keep track of the previous object to set its Next pointer*/
	for(psCurr = psPrev = psDestroyedObj; psCurr != NULL; psCurr = psNext)
	{
		psNext = psCurr->psNext;
		if (psCurr->died != gameTime)
		{
			EMBALM(psCurr);

			switch (psCurr->type)
			{
			case OBJ_DROID:
				droidRelease((DROID *)psCurr);
				HEAP_FREE(psDroidHeap, psCurr);
				break;
			case OBJ_STRUCTURE:
				structureRelease((STRUCTURE *)psCurr);
				HEAP_FREE(psStructHeap, psCurr);
				break;
			case OBJ_FEATURE:
				featureRelease((FEATURE *)psDestroyedObj);
				HEAP_FREE(psFeatureHeap, psCurr);
				break;
			default:
				ASSERT( FALSE, "objmemUpdate: unknown object type in destroyed list" );
			}
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

/**************************************************************************************
 *
 * Inlines for the object memory functions
 * The code is the same for the different object types only the pointer types
 * change.
 */

/* Creating a new object
 */
static inline BASE_OBJECT* createObject(UDWORD player, OBJ_HEAP *heap, OBJECT_TYPE objType)
{
	BASE_OBJECT* newObject;

	ASSERT(player < MAX_PLAYERS,
	       "createObject: invalid player number");

	if (!HEAP_ALLOC(heap, ((void**) &newObject)))
		return NULL;

	newObject->type = objType;
	newObject->id = (objID << 3) | player;
	objID++;
	newObject->player = (UBYTE)player;
	newObject->died = 0;

	return newObject;
}

/* Add the object to its list
 * \param list is a pointer to the object list
 */
static inline void addObjectToList(BASE_OBJECT *list[], BASE_OBJECT *object)
{
	ASSERT(object != NULL,
	       "addObjectToList: Invalid pointer");

	// Prepend the object to the top of the list
	object->psNext = list[object->player];
	list[object->player] = object;
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
static inline void removeObjectFromList(BASE_OBJECT *list[], BASE_OBJECT *object)
{
	BASE_OBJECT *psPrev = NULL, *psCurr;

	ASSERT( object != NULL,
		"removeObjectFromList: Invalid pointer" );

	// If the message to remove is the first one in the list then mark the next one as the first
	if (list[object->player] == object)
	{
		list[object->player] = list[object->player]->psNext;
		
		return;
	}
	
	// Iterate through the list and find the item before the object to delete
	for(psCurr = list[object->player]; (psCurr != object) && (psCurr != NULL); psCurr = psCurr->psNext)
	{
		psPrev = psCurr;
	}

	ASSERT( psCurr != NULL,
		"removeObjectFromList: object not found in list" );

	if (psCurr != NULL)
	{
		// Modify the "next" pointer of the previous item to
		// point to the "next" item of the item to delete.
		psPrev->psNext = psCurr->psNext;
	}
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

static inline void releaseAllObjectsInList(BASE_OBJECT *list[], OBJ_HEAP *heap, OBJECT_DESTRUCTOR objectDestructor)
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

			// Release object's memory
			HEAP_FREE(heap, psCurr);
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
BOOL createDroid(UDWORD player, DROID **ppsNew)
{
	*ppsNew = (DROID*)createObject(player, psDroidHeap, OBJ_DROID);

	if (*ppsNew == NULL)
		return FALSE;

	return TRUE;
}

/* add the droid to the Droid Lists */
 void addDroid(DROID *psDroidToAdd, DROID *pList[MAX_PLAYERS])
 {
	 DROID_GROUP	*psGroup;

	 addObjectToList((BASE_OBJECT**)pList, (BASE_OBJECT*)psDroidToAdd);
     /*whenever a droid gets added to a list other than the current list
     its died flag is set to NOT_CURRENT_LIST so that anything targetting
     it will cancel itself - HACK?!*/
     if (pList[psDroidToAdd->player] == apsDroidLists[psDroidToAdd->player])
     {
         psDroidToAdd->died = FALSE;

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
 }

 /*destroy a droid */
void killDroid(DROID *psDel)
{
	ASSERT( psDel->type == OBJ_DROID,
		"killUnit: pointer is not a unit" );
	ASSERT( psDel->player < MAX_PLAYERS,
		"killUnit: invalid player for unit" );
	destroyObject((BASE_OBJECT**)apsDroidLists, (BASE_OBJECT*)psDel);
}

/* Remove all droids */
void freeAllDroids(void)
{
	releaseAllObjectsInList((BASE_OBJECT**)apsDroidLists, psDroidHeap, (OBJECT_DESTRUCTOR)droidRelease);
}

/*Remove a single Droid from a list*/
void removeDroid(DROID *psDroidToRemove, DROID *pList[MAX_PLAYERS])
{
	ASSERT( psDroidToRemove->type == OBJ_DROID,
		"removeUnit: pointer is not a unit" );
	ASSERT( psDroidToRemove->player < MAX_PLAYERS,
		"removeUnit: invalid player for unit" );
	removeObjectFromList((BASE_OBJECT**)pList, (BASE_OBJECT*)psDroidToRemove);

    /*whenever a droid is removed from the current list its died
    flag is set to NOT_CURRENT_LIST so that anything targetting
    it will cancel itself - HACK?!*/
    if (pList[psDroidToRemove->player] == apsDroidLists[psDroidToRemove->player])
    {
        psDroidToRemove->died = NOT_CURRENT_LIST;
    }
}

/*Removes all droids that may be stored in the mission lists*/
void freeAllMissionDroids(void)
{
	releaseAllObjectsInList((BASE_OBJECT**)mission.apsDroidLists, psDroidHeap, (OBJECT_DESTRUCTOR)droidRelease);
}

/*Removes all droids that may be stored in the limbo lists*/
void freeAllLimboDroids(void)
{
	releaseAllObjectsInList((BASE_OBJECT**)apsLimboDroids, psDroidHeap, (OBJECT_DESTRUCTOR)droidRelease);
}

/**************************  STRUCTURE  *******************************/

/* Create a new structure */
BOOL createStruct(UDWORD player, STRUCTURE **ppsNew)
{
	*ppsNew = (STRUCTURE*)createObject(player, psStructHeap, OBJ_STRUCTURE);

	if (*ppsNew == NULL)
		return FALSE;

	return TRUE;
}

/* add the structure to the Structure Lists */
void addStructure(STRUCTURE *psStructToAdd)
{
    addObjectToList((BASE_OBJECT**)apsStructLists, (BASE_OBJECT*)psStructToAdd);
}

/* Destroy a structure */
void killStruct(STRUCTURE *psDel)
{
	ASSERT( psDel->type == OBJ_STRUCTURE,
		"killStruct: pointer is not a droid" );
	ASSERT( psDel->player < MAX_PLAYERS,
		"killStruct: invalid player for stucture" );
	destroyObject((BASE_OBJECT**)apsStructLists, (BASE_OBJECT*)psDel);
}

/* Remove heapall structures */
void freeAllStructs(void)
{
	releaseAllObjectsInList((BASE_OBJECT**)apsStructLists, psStructHeap, (OBJECT_DESTRUCTOR)structureRelease);
}

/*Remove a single Structure from a list*/
void removeStructureFromList(STRUCTURE *psStructToRemove, STRUCTURE *pList[MAX_PLAYERS])
{
	ASSERT( psStructToRemove->type == OBJ_STRUCTURE,
		"removeStructureFromList: pointer is not a structure" );
	ASSERT( psStructToRemove->player < MAX_PLAYERS,
		"removeStructureFromList: invalid player for structure" );
	removeObjectFromList((BASE_OBJECT**)pList, (BASE_OBJECT*)psStructToRemove);
}

/**************************  FEATURE  *********************************/

/* Create a new Feature */
BOOL createFeature(FEATURE **ppsNew)
{
	*ppsNew = (FEATURE*)createObject(0, psFeatureHeap, OBJ_FEATURE);

	if (*ppsNew == NULL)
		return FALSE;

	return TRUE;
}

/* add the feature to the Feature Lists */
 void addFeature(FEATURE *psFeatureToAdd)
 {
	 addObjectToList((BASE_OBJECT**)apsFeatureLists, (BASE_OBJECT*)psFeatureToAdd);
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
}

/* Remove all features */
void freeAllFeatures(void)
{
	releaseAllObjectsInList((BASE_OBJECT**)apsFeatureLists, psFeatureHeap, (OBJECT_DESTRUCTOR)featureRelease);
}

/**************************  FLAG_POSITION ********************************/

/* Create a new Flag Position */
BOOL createFlagPosition(FLAG_POSITION **ppsNew, UDWORD player)
{
	ASSERT( player<MAX_PLAYERS, "createFlagPosition: invalid player number" );

	if (!HEAP_ALLOC(psFlagPosHeap, (void**) ppsNew))
	{
		return FALSE;
	}
	(*ppsNew)->type = POS_DELIVERY;
	(*ppsNew)->player = player;
	(*ppsNew)->frameNumber = 0;
	(*ppsNew)->selected = FALSE;
	return TRUE;
}

/* add the Flag Position to the Flag Position Lists */
 void addFlagPosition(FLAG_POSITION *psFlagPosToAdd)
 {
	ASSERT( psFlagPosToAdd != NULL,
		"addFlagPosition: Invalid FlagPosition pointer" );

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
		HEAP_FREE(psFlagPosHeap, psDel);
	}
	else
	{
		for(psCurr = apsFlagPosLists[psDel->player]; (psCurr != psDel) &&
			(psCurr != NULL); psCurr = psCurr->psNext)
		{
			psPrev = psCurr;
		}
		ASSERT( psCurr != NULL,
			"removeFlagPosition:object not found" );
		if (psCurr != NULL)
		{
			psPrev->psNext = psCurr->psNext;
			HEAP_FREE(psFlagPosHeap, psCurr);
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
			HEAP_FREE(psFlagPosHeap, apsFlagPosLists[player]);
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


/**************************  STRUC FUNCTIONALITY ********************************/

/* Create a new Structure Functionality*/
BOOL createStructFunc(FUNCTIONALITY **ppsNew)
{
	if (!HEAP_ALLOC(psStructFuncHeap, (void**)ppsNew))
	{
		return FALSE;
	}
	return TRUE;
}

/*remove a structure Functionality from the heap*/
void removeStructFunc(FUNCTIONALITY *psDel)
{
	HEAP_FREE(psStructFuncHeap, psDel);
}

/**************************  OBJECT ACCESS FUNCTIONALITY ********************************/

// Find a base object from it's id
//this function is similar to BOOL scrvGetBaseObj(UDWORD id, BASE_OBJECT **ppsObj)
BASE_OBJECT *getBaseObjFromId(UDWORD id)
{
	UDWORD			i;
	UDWORD			player;
	BASE_OBJECT		*psObj;
	DROID			*psTrans;

	for(i=0; i<7; i++)
	{
		for(player=0; player<MAX_PLAYERS; player++)
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
	ASSERT( FALSE,"getBaseObjFromId() failed for id %d", id );

	return NULL;
}

UDWORD getRepairIdFromFlag(FLAG_POSITION *psFlag)
{
	UDWORD			i;
	UDWORD			player;
	STRUCTURE		*psObj;
	REPAIR_FACILITY	*psRepair;


	player = psFlag->player;

	//probably dont need to check mission list
	for(i=0; i<2; i++)
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
	ASSERT( FALSE,"getRepairIdFromFlag() failed" );

	return UDWORD_MAX;
}


// check a base object exists for an ID
BOOL checkValidId(UDWORD id)
{
	UDWORD			i;
	UDWORD			player;
	BASE_OBJECT		*psObj;
	DROID			*psTrans;

	for(i=0; i<7; i++)
	{
		for(player=0; player<MAX_PLAYERS; player++)
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
					return TRUE;
				}
			 	// if transporter check any droids in the grp
				if ((psObj->type == OBJ_DROID) && (((DROID*)psObj)->droidType == DROID_TRANSPORTER))
				{
					for(psTrans = ((DROID*)psObj)->psGroup->psList; psTrans != NULL; psTrans = psTrans->psGrpNext)
					{
						if (psTrans->id == id)
						{
							return TRUE;
						}
					}
				}
				psObj = psObj->psNext;
			}
		}
	}
	ASSERT( FALSE,"checkValidId() failed for id %d", id );

	return FALSE;
}


// integrity check the lists
void objListIntegCheck(void)
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
					"objListIntegCheck: misplaced object in the structure list for player %d",
					player );
		}
	}
	for(psCurr = (BASE_OBJECT*)apsFeatureLists[0]; psCurr; psCurr=psCurr->psNext)
	{
		ASSERT( psCurr->type == OBJ_FEATURE,
				"objListIntegCheck: misplaced object in the feature list" );
	}
}
