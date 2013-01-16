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
/*
 * ScriptVals.c
 *
 * Script data value functions
 *
 */

#include <string.h>
#include <list>

#include "lib/framework/frame.h"
#include "lib/script/script.h"
#include "objects.h"
#include "basedef.h"
#include "scripttabs.h"
#include "scriptvals.h"
#include "lib/gamelib/gtime.h"
#include "group.h"

// Keep all the loaded script contexts
struct SCRV_STORE
{
	SCRV_TYPE		type;
	char			*pIDString;
	SCRIPT_CONTEXT	*psContext;

	SCRV_STORE *     psNext;
};

// The list of script contexts
static SCRV_STORE	*psContextStore=NULL;

// Copy of all references to game objects, so that we can NULL them on death...
static std::list<INTERP_VAL *>basePointers;

// Initialise the script value module
bool scrvInitialise(void)
{
	psContextStore = NULL;
	return true;
}

// Shut down the script value module
void scrvShutDown(void)
{
	SCRV_STORE	*psCurr;
	while (psContextStore)
	{
		psCurr = psContextStore;
		psContextStore = psContextStore->psNext;

		free(psCurr->pIDString);
		free(psCurr);
	}
}

// reset the script value module
void scrvReset(void)
{
	SCRV_STORE	*psCurr;
	while (psContextStore)
	{
		psCurr = psContextStore;
		psContextStore = psContextStore->psNext;

		free(psCurr->pIDString);
		free(psCurr);
	}

	psContextStore = NULL;
	basePointers.clear();
}

// Add a new context to the list
bool scrvAddContext(char *pID, SCRIPT_CONTEXT *psContext, SCRV_TYPE type)
{
	SCRV_STORE		*psNew;

	psNew = (SCRV_STORE*)malloc(sizeof(SCRV_STORE));
	if (!psNew)
	{
		debug( LOG_FATAL, "scrvAddContext: Out of memory" );
		abort();
		return false;
	}
	psNew->pIDString = (char*)malloc(strlen(pID) + 1);
	if (!psNew->pIDString)
	{
		debug( LOG_FATAL, "scrvAddContext: Out of memory" );
		abort();
		return false;
	}
	strcpy(psNew->pIDString, pID);
	psNew->type = type;
	psNew->psContext = psContext;

	psNew->psNext = psContextStore;
	psContextStore = psNew;

	return true;
}

// Add a new base pointer variable
bool scrvAddBasePointer(INTERP_VAL *psVal)
{
	basePointers.push_back(psVal);
	return true;
}

// remove a base pointer from the list
void scrvReleaseBasePointer(INTERP_VAL *psVal)
{
	basePointers.remove(psVal);
}

static bool baseObjDead(INTERP_VAL *psVal)
{
	BASE_OBJECT *psObj = (BASE_OBJECT *)psVal->v.oval;
	if (psObj && isDead(psObj))
	{
		debug(LOG_DEATH, "Removing %p (%s) from the wzscript system", psObj, objInfo(psObj));
		psVal->v.oval = NULL;
		return true;
	}
	return false;
}

// Check all the base pointers to see if they have died
void scrvUpdateBasePointers(void)
{
	std::for_each(basePointers.begin(), basePointers.end(), baseObjDead);
}

// create a group structure for a ST_GROUP variable
bool scrvNewGroup(INTERP_VAL *psVal)
{
	DROID_GROUP		*psGroup;

	psGroup = grpCreate();

	// increment the refcount so the group doesn't get automatically freed when empty
	psGroup->add(NULL);

	psVal->v.oval = psGroup;

	return true;
}

// release a ST_GROUP variable
void scrvReleaseGroup(INTERP_VAL *psVal)
{
	DROID_GROUP		*psGroup;

	psGroup = (DROID_GROUP*)psVal->v.oval;
	psGroup->removeAll();

	ASSERT( psGroup->refCount == 1,
		"scrvReleaseGroup: ref count is wrong" );

	// do a final Remove to free the group
	psGroup->remove(NULL);
}

// Get a context from the list
bool scrvGetContext(char *pID, SCRIPT_CONTEXT **ppsContext)
{
	SCRV_STORE	*psCurr;

	for(psCurr=psContextStore; psCurr; psCurr=psCurr->psNext)
	{
		if (strcmp(psCurr->pIDString, pID) == 0)
		{
			*ppsContext = psCurr->psContext;
			return true;
		}
	}

	debug( LOG_FATAL, "scrvGetContext: couldn't find context for id: %s", pID );
	abort();
	return false;
}
