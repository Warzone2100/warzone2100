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
/**
 * @file group.c
 *
 * Link droids together into a group for AI etc.
 *
 */

#include "lib/framework/frame.h"
#include "lib/netplay/netplay.h"

#include "multiplay.h"
#include <QtCore/QMap>

// Group system variables: grpGlobalManager enables to remove all the groups to Shutdown the system
static QMap<int, DROID_GROUP *> grpGlobalManager;
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
	QMap<int, DROID_GROUP *>::iterator iter;

	for(iter = grpGlobalManager.begin(); iter != grpGlobalManager.end(); iter++)
	{
		delete (*iter);
	}
	grpGlobalManager.clear();
	grpInitialized = false;
}

// Constructor
DROID_GROUP::DROID_GROUP()
{
	type = GT_NORMAL;
	refCount = 0;
	psList = NULL;
	psCommander = NULL;
	memset(&sRunData, 0, sizeof(sRunData));
}

// create a new group
DROID_GROUP *grpCreate(int id)
{
	ASSERT(grpInitialized, "Group code not initialized yet");
	DROID_GROUP *psGroup = new DROID_GROUP;
	if (id == -1)
	{
		int i;
		for (i = 0; grpGlobalManager.contains(i); i++) {}	// surly hack
		psGroup->id = i;
	}
	else
	{
		ASSERT(!grpGlobalManager.contains(id), "Group %d is already created!", id);
		psGroup->id = id;
	}
	grpGlobalManager.insert(psGroup->id, psGroup);
	return psGroup;
}

DROID_GROUP *grpFind(int id)
{
	DROID_GROUP *psGroup = grpGlobalManager.value(id, NULL);
	if (!psGroup)
	{
		psGroup = grpCreate(id);
	}
	return psGroup;
}

// add a droid to a group
void DROID_GROUP::add(DROID *psDroid)
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
			psDroid->psGroup->remove(psDroid);
		}
		psDroid->psGroup = this;

		if (isTransporter(psDroid))
		{
			ASSERT_OR_RETURN(, (type == GT_NORMAL), "grpJoin: Cannot have two transporters in a group" );
			ASSERT_OR_RETURN(, psList == NULL, "Adding transporter to non-empty list.");
			type = GT_TRANSPORTER;
			psDroid->psGrpNext = psList;
			psList = psDroid;
		}
		else if ((psDroid->droidType == DROID_COMMAND) && (type != GT_TRANSPORTER))
		{
			ASSERT_OR_RETURN(, (type == GT_NORMAL) && (psCommander == NULL), "grpJoin: Cannot have two command droids in a group" );
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
void DROID_GROUP::remove(DROID *psDroid)
{
	DROID	*psPrev, *psCurr;

	ASSERT_OR_RETURN(, grpInitialized, "Group code not initialized yet");

	if (psDroid != NULL && psDroid->psGroup != this)
	{
		ASSERT( false, "grpLeave: droid group does not match" );
		return;
	}

	// SyncDebug
	if (psDroid != NULL && type == GT_COMMAND)
	{
		syncDebug("Droid %d leaving command group %d", psDroid->id, psCommander != NULL? psCommander->id : 0);
	}

	refCount -= 1;
	// if psDroid == NULL just decrease the refcount don't remove anything from the list
	if (psDroid != NULL)
	{
		// update group list of droids and droids' psGrpNext
		if (psDroid->droidType != DROID_COMMAND || type != GT_COMMAND)
		{
			psPrev = NULL;
			for(psCurr = psList; psCurr; psCurr = psCurr->psGrpNext)
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
					psList = psList->psGrpNext;
				}
			}
		}

		psDroid->psGroup = NULL;
		psDroid->psGrpNext = NULL;

		// update group's type
		if ((psDroid->droidType == DROID_COMMAND) && (type == GT_COMMAND))
		{
			type = GT_NORMAL;
			psCommander = NULL;
		}
		else if (isTransporter(psDroid) && (type == GT_TRANSPORTER))
		{
			type = GT_NORMAL;
		}
	}

	// free the group if necessary
	if (refCount <= 0)
	{
		grpGlobalManager.remove(id);
		delete this;
	}
}

// count the members of a group
unsigned int DROID_GROUP::getNumMembers()
{
	const DROID* psCurr;
	unsigned int num;

	ASSERT(grpInitialized, "Group code not initialized yet");

	num = 0;
	for (psCurr = psList; psCurr; psCurr = psCurr->psGrpNext)
	{
		num += 1;
	}

	return num;
}

// remove all droids from a group
void DROID_GROUP::removeAll()
{
	DROID	*psCurr, *psNext;

	ASSERT(grpInitialized, "Group code not initialized yet");

	for(psCurr = psList; psCurr; psCurr = psNext)
	{
		psNext = psCurr->psGrpNext;
		remove(psCurr);
	}
}

// Give a group of droids an order
void DROID_GROUP::orderGroup(DROID_ORDER order)
{
	DROID *psCurr;

	ASSERT(grpInitialized, "Group code not initialized yet");

	for (psCurr = psList; psCurr; psCurr=psCurr->psGrpNext)
	{
		orderDroid(psCurr, order, ModeQueue);
	}
}

// Give a group of droids an order (using a Location)
void DROID_GROUP::orderGroup(DROID_ORDER order, UDWORD x, UDWORD y)
{
	DROID	*psCurr;

	ASSERT(grpInitialized, "Group code not initialized yet");
	ASSERT_OR_RETURN(, validOrderForLoc(order), "orderGroup: Bad order");

	for (psCurr = psList; psCurr != NULL; psCurr = psCurr->psGrpNext)
	{
		orderDroidLoc(psCurr, order, x, y, bMultiMessages? ModeQueue : ModeImmediate);
	}
}

// Give a group of droids an order (using an Object)
void DROID_GROUP::orderGroup(DROID_ORDER order, BASE_OBJECT *psObj)
{
	DROID	*psCurr;

	ASSERT_OR_RETURN(, validOrderForObj(order), "orderGroup: Bad order");

	for (psCurr = psList; psCurr != NULL; psCurr = psCurr->psGrpNext)
	{
		orderDroidObj(psCurr, order, (BASE_OBJECT *)psObj, bMultiMessages? ModeQueue : ModeImmediate);
	}
}

// Set the secondary state for a group of droids
void DROID_GROUP::setSecondary(SECONDARY_ORDER sec, SECONDARY_STATE state)
{
	DROID	*psCurr;

	ASSERT(grpInitialized, "Group code not initialized yet");

	for(psCurr = psList; psCurr; psCurr = psCurr->psGrpNext)
	{
		secondarySetState(psCurr, sec, state);
	}
}
