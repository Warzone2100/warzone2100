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
 * ScriptVals.c
 *
 * Script data value functions
 *
 */

#include <string.h>

#include "lib/framework/frame.h"
#include "lib/script/script.h"
#include "objects.h"
#include "basedef.h"
#include "scripttabs.h"
#include "scriptvals.h"
#include "lib/gamelib/gtime.h"
#include "group.h"

// Keep all the loaded script contexts
typedef struct _scrv_store
{
	SCRV_TYPE		type;
	char			*pIDString;
	SCRIPT_CONTEXT	*psContext;

	struct _scrv_store *psNext;
} SCRV_STORE;

// The list of script contexts
static SCRV_STORE	*psContextStore=NULL;

// keep a note of all base object pointers
#define MAX_BASEPOINTER		2000		//200 - local variables require more of these ("run" error)
static INTERP_VAL	*asBasePointers[MAX_BASEPOINTER];

// Initialise the script value module
BOOL scrvInitialise(void)
{
	psContextStore = NULL;
	memset(asBasePointers, 0, sizeof(asBasePointers));

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
	memset(asBasePointers, 0, sizeof(asBasePointers));
}


// Add a new context to the list
BOOL scrvAddContext(char *pID, SCRIPT_CONTEXT *psContext, SCRV_TYPE type)
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
BOOL scrvAddBasePointer(INTERP_VAL *psVal)
{
	SDWORD	i;

	for(i=0; i<MAX_BASEPOINTER; i++)
	{
		if (asBasePointers[i] == NULL)
		{
			asBasePointers[i] = psVal;
			return true;
		}
	}

	ASSERT(false, "scrvAddBasePointer: not enough base pointers left (total :%d)", MAX_BASEPOINTER);
	return false;
}


// remove a base pointer from the list
void scrvReleaseBasePointer(INTERP_VAL *psVal)
{
	SDWORD	i;

	for(i=0; i<MAX_BASEPOINTER; i++)
	{
		if (asBasePointers[i] == psVal)
		{
			asBasePointers[i] = NULL;
			return;
		}
	}
}


// Check all the base pointers to see if they have died
void scrvUpdateBasePointers(void)
{
	unsigned int i;

	for(i = 0; i < MAX_BASEPOINTER; i++)
	{
		if (asBasePointers[i] != NULL)
		{
			INTERP_VAL *psVal = asBasePointers[i];
			BASE_OBJECT *psObj = (BASE_OBJECT *)psVal->v.oval;

			if (psObj && isDead(psObj))
			{
				psVal->v.oval = NULL;
			}
		}
	}
}


// create a group structure for a ST_GROUP variable
BOOL scrvNewGroup(INTERP_VAL *psVal)
{
	DROID_GROUP		*psGroup;

	if (!grpCreate(&psGroup))
	{
		return false;
	}

	// increment the refcount so the group doesn't get automatically freed when empty
	grpJoin(psGroup, NULL);

	psVal->v.oval = psGroup;

	return true;
}


// release a ST_GROUP variable
void scrvReleaseGroup(INTERP_VAL *psVal)
{
	DROID_GROUP		*psGroup;

	psGroup = (DROID_GROUP*)psVal->v.oval;
	grpReset(psGroup);

	ASSERT( psGroup->refCount == 1,
		"scrvReleaseGroup: ref count is wrong" );

	// do a final grpLeave to free the group
	grpLeave(psGroup, NULL);
}


// Get a context from the list
BOOL scrvGetContext(char *pID, SCRIPT_CONTEXT **ppsContext)
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
