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
/**
 * @file group.c
 *
 * Link droids together into a group for AI etc.
 *
 */

#include "lib/framework/frame.h"
#include "lib/netplay/sync_debug.h"

#include "multiplay.h"
#include "group.h"
#include "droid.h"
#include "order.h"
#include "hci.h"
#include <map>

// Group system variables: grpGlobalManager enables to remove all the groups to Shutdown the system
static std::map<int, std::unique_ptr<DROID_GROUP>> grpGlobalManager;
static bool grpInitialized = false;

// initialise the group system
bool grpInitialise()
{
	grpGlobalManager.clear();
	grpInitialized = true;
	return true;
}

// shutdown the group system
void grpShutDown()
{
	/* Since we are not very diligent removing groups after we have
	 * created them; we need this hack to remove them on level end. */
	grpGlobalManager.clear();
	grpInitialized = false;
}

// Constructor
DROID_GROUP::DROID_GROUP()
{
	type = GT_NORMAL;
	refCount = 0;
	psCommander = nullptr;
}

// create a new group
DROID_GROUP *grpCreate()
{
	ASSERT(grpInitialized, "Group code not initialized yet");
	auto psGroup = std::make_unique<DROID_GROUP>();
	{
		int i;
		for (i = 0; grpGlobalManager.find(i) != grpGlobalManager.end(); i++) {}	// surly hack
		psGroup->id = i;
	}
	DROID_GROUP* rawPsGroup = psGroup.get();
	grpGlobalManager.emplace(psGroup->id, std::move(psGroup));
	return rawPsGroup;
}

// add a droid to a group
void DROID_GROUP::add(DROID *psDroid)
{
	ASSERT(grpInitialized, "Group code not initialized yet");

	refCount += 1;
	// if psDroid == NULL just increase the refcount don't add anything to the list
	if (psDroid != nullptr)
	{
		if (!psList.empty() && psDroid->player != psList.front()->player)
		{
			ASSERT(false, "grpJoin: Cannot have more than one players droids in a group");
			return;
		}

		if (psDroid->psGroup != nullptr)
		{
			psDroid->psGroup->remove(psDroid);
		}
		psDroid->psGroup = this;

		if (psDroid->isTransporter())
		{
			ASSERT_OR_RETURN(, (type == GT_NORMAL), "grpJoin: Cannot have two transporters in a group");
			ASSERT_OR_RETURN(, psList.empty(), "Adding transporter to non-empty list.");
			type = GT_TRANSPORTER;
			psList.push_front(psDroid);
		}
		else if ((psDroid->droidType == DROID_COMMAND) && (type != GT_TRANSPORTER))
		{
			ASSERT_OR_RETURN(, (type == GT_NORMAL) && (psCommander == nullptr), "grpJoin: Cannot have two command droids in a group");
			type = GT_COMMAND;
			psCommander = psDroid;
		}
		else
		{
			psList.push_front(psDroid);
		}

		if (type == GT_COMMAND)
		{
			syncDebug("Droid %d joining command group %d", psDroid->id, psCommander != nullptr ? psCommander->id : 0);
		}
	}

	intCommanderGroupChanged((type == GT_COMMAND && psCommander) ? psCommander : nullptr);
}

// remove a droid from a group
void DROID_GROUP::remove(DROID *psDroid)
{
	ASSERT_OR_RETURN(, grpInitialized, "Group code not initialized yet");

	if (psDroid != nullptr && psDroid->psGroup != this)
	{
		ASSERT(false, "grpLeave: droid group does not match");
		return;
	}

	// SyncDebug
	if (psDroid != nullptr && type == GT_COMMAND)
	{
		syncDebug("Droid %d leaving command group %d", psDroid->id, psCommander != nullptr ? psCommander->id : 0);
	}

	refCount -= 1;
	// if psDroid == NULL just decrease the refcount don't remove anything from the list
	if (psDroid != nullptr)
	{
		// update group list of droids
		if (psDroid->droidType != DROID_COMMAND || type != GT_COMMAND)
		{
			auto it = std::find(psList.begin(), psList.end(), psDroid);
			if (it != psList.end())
			{
				psList.erase(it);
			}
			else
			{
				ASSERT(false, "grpLeave: droid not found");
			}
		}

		psDroid->psGroup = nullptr;

		// update group's type
		if ((psDroid->droidType == DROID_COMMAND) && (type == GT_COMMAND))
		{
			type = GT_NORMAL;
			psCommander = nullptr;
		}
		else if (psDroid->isTransporter() && (type == GT_TRANSPORTER))
		{
			type = GT_NORMAL;
		}
	}

	intCommanderGroupChanged((type == GT_COMMAND && psCommander) ? psCommander : nullptr);

	// free the group if necessary
	if (refCount <= 0)
	{
		grpGlobalManager.erase(id);
	}
}

// count the members of a group
unsigned int DROID_GROUP::getNumMembers() const
{
	ASSERT(grpInitialized, "Group code not initialized yet");
	return psList.size();
}

// Give a group of droids an order
void DROID_GROUP::orderGroup(DROID_ORDER order)
{
	ASSERT(grpInitialized, "Group code not initialized yet");

	for (DROID* psCurr : psList)
	{
		orderDroid(psCurr, order, ModeQueue);
	}
}

// Give a group of droids an order (using a Location)
void DROID_GROUP::orderGroup(DROID_ORDER order, UDWORD x, UDWORD y)
{
	ASSERT(grpInitialized, "Group code not initialized yet");
	ASSERT_OR_RETURN(, validOrderForLoc(order), "orderGroup: Bad order");

	for (DROID* psCurr : psList)
	{
		orderDroidLoc(psCurr, order, x, y, bMultiMessages ? ModeQueue : ModeImmediate);
	}
}

// Give a group of droids an order (using an Object)
void DROID_GROUP::orderGroup(DROID_ORDER order, BASE_OBJECT *psObj)
{
	ASSERT_OR_RETURN(, validOrderForObj(order), "orderGroup: Bad order");

	for (DROID* psCurr : psList)
	{
		orderDroidObj(psCurr, order, (BASE_OBJECT *)psObj, bMultiMessages ? ModeQueue : ModeImmediate);
	}
}

// Set the secondary state for a group of droids
void DROID_GROUP::setSecondary(SECONDARY_ORDER sec, SECONDARY_STATE state)
{
	ASSERT(grpInitialized, "Group code not initialized yet");

	for (DROID* psCurr : psList)
	{
		secondarySetState(psCurr, sec, state);
	}
}
