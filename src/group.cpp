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
/**
 * @file group.c
 *
 * Link droids together into a group for AI etc.
 *
 */

#include "lib/framework/frame.h"
#include "lib/netplay/netplay.h"

#include "multiplay.h"
#include <list>

static std::list<DROID_GROUP *> grpGlobalManager;
static bool grpInitialized = false;

// initialise the group system
bool grpInitialise(void)
{
	grpGlobalManager.clear();
	grpInitialized = true;
	return true;
}

// shutdown the group system
void grpShutDown(void)
{
	/* Since we are not very diligent removing groups after we have
	 * created them; we need this hack to remove them on level end. */
	std::list<DROID_GROUP *>::iterator iter;

	for(iter = grpGlobalManager.begin(); iter != grpGlobalManager.end(); iter++)
	{
		free(*iter);
	}
	grpGlobalManager.clear();
	grpInitialized = false;
}

// Initialize new group
void DROID_GROUP::Init()
{
	type = GT_NORMAL;
	refCount = 0;
	psList = NULL;
	psCommander = NULL;
}

// create a new group
bool grpCreate(DROID_GROUP	**ppsGroup)
{
	ASSERT(grpInitialized, "Group code not initialized yet");
	*ppsGroup = (DROID_GROUP *)calloc(1, sizeof(DROID_GROUP));
	if (*ppsGroup == NULL)
	{
		debug(LOG_ERROR, "grpCreate: Out of memory");
		return false;
	}

	grpGlobalManager.push_back(*ppsGroup);
	(*ppsGroup)->Init();

	return true;
}

// add a droid to a group
void DROID_GROUP::Add(DROID *psDroid)
{
	ASSERT(grpInitialized, "Group code not initialized yet");
	
	refCount += 1;
	// if psDroid == NULL just increase the refcount don't add anything to the list
	if (psDroid != NULL)
	{
		if (psList && psDroid->player != psList->player)
		{
			ASSERT( false,"grpJoin: Cannot have more than one players droids in a group" );
			return;
		}
		
		if (psDroid->psGroup != NULL)
		{
			grpLeave(psDroid->psGroup, psDroid);
		}
		
		psDroid->psGroup = this;
		
		if (psDroid->droidType == DROID_TRANSPORTER)
		{
			ASSERT_OR_RETURN(, (type == GT_NORMAL),
											 "grpJoin: Cannot have two transporters in a group" );
			type = GT_TRANSPORTER;
			psDroid->psGrpNext = psList;
			psList = psDroid;
		}
		else if ((psDroid->droidType == DROID_COMMAND) &&
						 (type != GT_TRANSPORTER))
		{
			ASSERT_OR_RETURN(, (type == GT_NORMAL) && (psCommander == NULL),
											 "grpJoin: Cannot have two command droids in a group" );
			type = GT_COMMAND;
			psCommander = psDroid;
		}
		else
		{
			psDroid->psGrpNext = psList;
			psList = psDroid;
		}
		
		if (type == GT_COMMAND)
		{
			syncDebug("Droid %d joining command group %d", psDroid->id, psCommander != NULL? psCommander->id : 0);
		}
	}
}

// remove a droid from a group
void grpLeave(DROID_GROUP *psGroup, DROID *psDroid)
{
	DROID	*psPrev, *psCurr;

	ASSERT_OR_RETURN(, grpInitialized, "Group code not initialized yet");
	ASSERT_OR_RETURN(, psGroup != NULL,
		"grpLeave: invalid group pointer" );

	if (psDroid != NULL && psDroid->psGroup != psGroup)
	{
		ASSERT( false, "grpLeave: droid group does not match" );
		return;
	}

	if (psDroid != NULL && psGroup->type == GT_COMMAND)
	{
		syncDebug("Droid %d leaving command group %d", psDroid->id, psGroup->psCommander != NULL? psGroup->psCommander->id : 0);
	}

	psGroup->refCount -= 1;
	// if psDroid == NULL just decrease the refcount don't remove anything from the list
	if (psDroid != NULL &&
		(psDroid->droidType != DROID_COMMAND ||
		 psGroup->type != GT_COMMAND))
	{
		psPrev = NULL;
		for(psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			if (psCurr == psDroid)
			{
				break;
			}
			psPrev = psCurr;
		}
		ASSERT( psCurr != NULL, "grpLeave: droid not found" );
		if (psCurr != NULL)
		{
			if (psPrev)
			{
				psPrev->psGrpNext = psCurr->psGrpNext;
			}
			else
			{
				psGroup->psList = psGroup->psList->psGrpNext;
			}
		}
	}

	if (psDroid)
	{
		psDroid->psGroup = NULL;
		psDroid->psGrpNext = NULL;

		if ((psDroid->droidType == DROID_COMMAND) &&
			(psGroup->type == GT_COMMAND))
		{
			psGroup->type = GT_NORMAL;
			psGroup->psCommander = NULL;
		}
		else if ((psDroid->droidType == DROID_TRANSPORTER) &&
				 (psGroup->type == GT_TRANSPORTER))
		{
			psGroup->type = GT_NORMAL;
		}
	}

	// free the group structure if necessary
	if (psGroup->refCount <= 0)
	{
		grpGlobalManager.remove(psGroup);
		free(psGroup);
	}
}

// count the members of a group
unsigned int grpNumMembers(const DROID_GROUP* psGroup)
{
	const DROID* psCurr;
	unsigned int num;

	ASSERT(grpInitialized, "Group code not initialized yet");
	ASSERT_OR_RETURN(0, psGroup != NULL, "invalid droid group");

	num = 0;
	for (psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
	{
		num += 1;
	}

	return num;
}

// remove all droids from a group
void grpReset(DROID_GROUP *psGroup)
{
	DROID	*psCurr, *psNext;

	ASSERT(grpInitialized, "Group code not initialized yet");
	ASSERT_OR_RETURN(, psGroup != NULL,
		"grpReset: invalid droid group" );

	for(psCurr = psGroup->psList; psCurr; psCurr = psNext)
	{
		psNext = psCurr->psGrpNext;
		grpLeave(psGroup, psCurr);
	}
}

// Give a group of droids an order
void orderGroup(DROID_GROUP *psGroup, DROID_ORDER order)
{
	DROID *psCurr;

	ASSERT(grpInitialized, "Group code not initialized yet");
	ASSERT_OR_RETURN(, psGroup != NULL,
		"orderGroup: invalid droid group" );

	for (psCurr = psGroup->psList; psCurr; psCurr=psCurr->psGrpNext)
	{
		orderDroid(psCurr, order, ModeQueue);
	}
}

// Give a group of droids an order (using a Location)
void orderGroup(DROID_GROUP *psGroup, DROID_ORDER order, UDWORD x, UDWORD y)
{
	DROID	*psCurr;

	ASSERT(grpInitialized, "Group code not initialized yet");
	ASSERT_OR_RETURN(, psGroup != NULL,
		"orderGroupLoc: invalid droid group" );
	ASSERT_OR_RETURN(, validOrderForLoc(order), "orderGroup: Bad order");

	for (psCurr = psGroup->psList; psCurr != NULL; psCurr = psCurr->psGrpNext)
	{
		orderDroidLoc(psCurr, order, x, y, bMultiMessages? ModeQueue : ModeImmediate);
	}
}

// Give a group of droids an order (using an Object)
void orderGroup(DROID_GROUP *psGroup, DROID_ORDER order, BASE_OBJECT *psObj)
{
	DROID	*psCurr;

	ASSERT_OR_RETURN(, psGroup != NULL,
		"orderGroup: invalid droid group" );
	ASSERT_OR_RETURN(, validOrderForObj(order), "orderGroup: Bad order");

	for (psCurr = psGroup->psList; psCurr != NULL; psCurr = psCurr->psGrpNext)
	{
		orderDroidObj(psCurr, order, (BASE_OBJECT *)psObj, bMultiMessages? ModeQueue : ModeImmediate);
	}
}

/* set the secondary state for a group of droids */
void grpSetSecondary(DROID_GROUP *psGroup, SECONDARY_ORDER sec, SECONDARY_STATE state)
{
	DROID	*psCurr;

	ASSERT(grpInitialized, "Group code not initialized yet");
	ASSERT_OR_RETURN(, psGroup != NULL,
		"grpSetSecondary: invalid droid group" );

	for(psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
	{
		secondarySetState(psCurr, sec, state);
	}
}
