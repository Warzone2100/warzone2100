/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
/***************************************************************************/
/*
 * Animobj.c
 *
 * Anim object functions
 *
 * Gareth Jones 14/11/97
 */
/***************************************************************************/

#include <string.h>

#include "lib/framework/frame.h"
#include "lib/gamelib/hashtable.h"
#include "lib/gamelib/gtime.h"
#include "animobj.h"
#include "basedef.h"

/***************************************************************************/

/* allocation sizes for anim object table */
#define	ANIM_OBJ_INIT		100
#define	ANIM_OBJ_EXT		20

/* max number of slots in hash table - prime numbers are best because hash
 * function used here is modulous of object pointer with table size -
 * prime number nearest 500 is 499.
 * Table of nearest primes in Binstock+Rex, "Practical Algorithms" p 91.
 */
#define	ANIM_HASH_TABLE_SIZE	499

/***************************************************************************/
/* global variables */

static	HASHTABLE *g_pAnimObjTable = NULL;

/***************************************************************************/
/* local functions */

static UDWORD		animObj_HashFunction(intptr_t iKey1, intptr_t iKey2);
static void		animObj_HashFreeElementFunc(void *psElement);

/***************************************************************************/
/*
 * Anim functions
 */
/***************************************************************************/

bool animObj_Init()
{
	SDWORD	iSize = sizeof(ANIM_OBJECT);

	/* allocate hashtable */
	hashTable_Create(&g_pAnimObjTable, ANIM_HASH_TABLE_SIZE,
	                 ANIM_OBJ_INIT, ANIM_OBJ_EXT, iSize);

	/* set local hash table functions */
	hashTable_SetHashFunction(g_pAnimObjTable, animObj_HashFunction);
	hashTable_SetFreeElementFunction(g_pAnimObjTable, animObj_HashFreeElementFunc);

	return true;
}

bool animObj_Shutdown()
{
	/* destroy hash table */
	hashTable_Destroy(g_pAnimObjTable);
	g_pAnimObjTable = NULL;

	return true;
}

void animObj_SetDoneFunc(ANIM_OBJECT *psObj, ANIMOBJDONEFUNC pDoneFunc)
{
	psObj->pDoneFunc = pDoneFunc;
}

/// Takes object pointer and id and returns hashed index
static UDWORD animObj_HashFunction(intptr_t iKey1, intptr_t iKey2)
{
	return (iKey1 + iKey2) % ANIM_HASH_TABLE_SIZE;
}

static void animObj_HashFreeElementFunc(void *psElement)
{
#ifdef DEBUG
	ANIM_OBJECT *psObj = (ANIM_OBJECT *) psElement;
	ASSERT(psObj != NULL, "object pointer invalid");
#endif
}

void animObj_Update()
{
	ANIM_OBJECT *psObj = (ANIM_OBJECT *)hashTable_GetFirst(g_pAnimObjTable);

	while (psObj != NULL)
	{
		bool bRemove = false;

		/* test whether parent object has died */
		if (psObj->psParent->died == true)
		{
			bRemove = true;
			psObj->psParent->psCurAnim = NULL;
		}

		/* remove any expired (non-looping) animations */
		if ((bRemove == false) && (psObj->uwCycles != 0))
		{
			int dwTime = graphicsTime - psObj->udwStartTime;

			if (dwTime > (psObj->psAnim->uwAnimTime * psObj->uwCycles))
			{
				/* fire callback if set */
				if (psObj->pDoneFunc != NULL)
				{
					(psObj->pDoneFunc)(psObj);
				}

				bRemove = true;
			}
		}

		/* remove object if flagged */
		if (bRemove == true)
		{
			if (hashTable_RemoveElement(g_pAnimObjTable, psObj,
			                            (intptr_t) psObj->psParent, psObj->psAnim->uwID) == false)
			{
				debug(LOG_FATAL, "animObj_Update: couldn't remove anim obj\n");
				abort();
			}
		}

		psObj = (ANIM_OBJECT *)hashTable_GetNext(g_pAnimObjTable);
	}
}

// uwCycles=0 for infinite looping
ANIM_OBJECT *animObj_Add(BASE_OBJECT *pParentObj, int iAnimID, UWORD uwCycles)
{
	ANIM_OBJECT *psObj;
	BASEANIM *psAnim = anim_GetAnim(iAnimID);
	UWORD uwObj;

	ASSERT(psAnim != NULL, "anim id %i not found\n", iAnimID);

	/* get object from table */
	psObj = (ANIM_OBJECT *)hashTable_GetElement(g_pAnimObjTable);

	if (psObj == NULL)
	{
		debug(LOG_ERROR, "No room in hash table");
		return (NULL);
	}

	/* init object */
	psObj->uwID           = (UWORD) iAnimID;
	psObj->psAnim         = (ANIM3D *) psAnim;
	psObj->udwStartTime   = gameTime - deltaGameTime + 1;  // Start animation at beginning of update period.
	psObj->uwCycles       = uwCycles;
	psObj->bVisible       = true;
	psObj->psParent       = pParentObj;
	psObj->pDoneFunc	  = NULL;

	/* allocate component objects */
	if (psAnim->animType == ANIM_3D_TRANS)
	{
		uwObj = psAnim->uwObj;
	}
	else
	{
		uwObj = psAnim->uwStates;
	}

	if (uwObj > ANIM_MAX_COMPONENTS)
	{
		debug(LOG_FATAL, "Number of components too small");
		abort();
	}

	/* set parent pointer and shape pointer */
	for (int i = 0; i < uwObj; i++)
	{
		psObj->apComponents[i].psParent = pParentObj;
		psObj->apComponents[i].psShape  = psObj->psAnim->apFrame[i];
	}

	/* insert object in table by parent */
	hashTable_InsertElement(g_pAnimObjTable, psObj, (intptr_t) pParentObj, iAnimID);

	return psObj;
}

ANIM_OBJECT *animObj_GetFirst()
{
	return (ANIM_OBJECT *)hashTable_GetFirst(g_pAnimObjTable);
}

ANIM_OBJECT *animObj_GetNext()
{
	return (ANIM_OBJECT *)hashTable_GetNext(g_pAnimObjTable);
}

ANIM_OBJECT *animObj_Find(BASE_OBJECT *pParentObj, int iAnimID)
{
	return (ANIM_OBJECT *)hashTable_FindElement(g_pAnimObjTable, (intptr_t) pParentObj, iAnimID);
}

bool animObj_Remove(ANIM_OBJECT *psObj, int iAnimID)
{
	return hashTable_RemoveElement(g_pAnimObjTable, psObj, (intptr_t)psObj->psParent, iAnimID);
}
