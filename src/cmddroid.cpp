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
/**
 *
 * @file
 * Code for command droids
 *
 */

#include <string.h>
#include "lib/framework/frame.h"
#include "objects.h"
#include "cmddroiddef.h"
#include "cmddroid.h"
#include "group.h"
#include "order.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "console.h"
#include "objmem.h"
#include "droid.h"

/**This represents the current selected player, which is the client's player.*/
extern UDWORD selectedPlayer;

/** This global instance is responsible for dealing with the each player's target designator.*/
DROID	*apsCmdDesignator[MAX_PLAYERS];


/** This function allocs the global instance apsCmdDesignator.*/
bool cmdDroidInit()
{
	memset(apsCmdDesignator, 0, sizeof(DROID *)*MAX_PLAYERS);
	return true;
}

// ShutDown the command droids
void cmdDroidShutDown()
{
}

/** This function runs on all players to check if the player's current target designator as died.
 * If it does, sets the target designator to NULL.
 */
void cmdDroidUpdate()
{
	for (auto &i : apsCmdDesignator)
	{
		if (i && i->died)
		{
			i = nullptr;
		}
	}
}

/** This function adds the droid to the command group commanded by psCommander.
 * It creates a group if it doesn't exist.
 * If the group is not full, it adds the droid to it and sets all the droid's states and orders to the group's.
 */
void cmdDroidAddDroid(DROID *psCommander, DROID *psDroid)
{
	DROID_GROUP	*psGroup;

	if (psCommander->psGroup == nullptr)
	{
		psGroup = grpCreate();
		psGroup->add(psCommander);
		psDroid->group = UBYTE_MAX;
	}

	if (psCommander->psGroup->getNumMembers() < cmdDroidMaxGroup(psCommander))
	{
		psCommander->psGroup->add(psDroid);
		psDroid->group = UBYTE_MAX;

		// set the secondary states for the unit
		secondarySetState(psDroid, DSO_REPAIR_LEVEL, (SECONDARY_STATE)(psCommander->secondaryOrder & DSS_REPLEV_MASK), ModeImmediate);
		secondarySetState(psDroid, DSO_ATTACK_LEVEL, (SECONDARY_STATE)(psCommander->secondaryOrder & DSS_ALEV_MASK), ModeImmediate);

		orderDroidObj(psDroid, DORDER_GUARD, (BASE_OBJECT *)psCommander, ModeImmediate);
	}
	else
	{
		audio_PlayTrack(ID_SOUND_BUILD_FAIL);
		addConsoleMessage(_("Commander needs a higher level to command more units"), DEFAULT_JUSTIFY,  SYSTEM_MESSAGE);
	}
}

DROID *cmdDroidGetDesignator(UDWORD player)
{
	return apsCmdDesignator[player];
}

void cmdDroidSetDesignator(DROID *psDroid)
{
	ASSERT_OR_RETURN(, psDroid != nullptr, "Invalid droid!");
	if (psDroid->droidType != DROID_COMMAND)
	{
		return;
	}

	apsCmdDesignator[psDroid->player] = psDroid;
}

void cmdDroidClearDesignator(UDWORD player)
{
	apsCmdDesignator[player] = nullptr;
}

/** This function returns the index of the command droid.
 * It does this by searching throughout all the player's droids.
 * @todo try to find something more efficient, has this function is of O(TotalNumberOfDroidsOfPlayer).
 */
SDWORD cmdDroidGetIndex(DROID *psCommander)
{
	SDWORD	index = 1;
	DROID	*psCurr;

	if (psCommander->droidType != DROID_COMMAND)
	{
		return 0;
	}

	for (psCurr = apsDroidLists[psCommander->player]; psCurr; psCurr = psCurr->psNext)
	{
		if (psCurr->droidType == DROID_COMMAND &&
		    psCurr->id < psCommander->id)
		{
			index += 1;
		}
	}

	return index;
}

/** This function returns the maximum group size of the command droid.*/
unsigned int cmdDroidMaxGroup(const DROID *psCommander)
{
	const BRAIN_STATS *psStats = getBrainStats(psCommander);
	return getDroidLevel(psCommander) * psStats->upgrade[psCommander->player].maxDroidsMult + psStats->upgrade[psCommander->player].maxDroids;
}

/** This function adds experience to the command droid of the psKiller's command group.*/
void cmdDroidUpdateKills(DROID *psKiller, uint32_t experienceInc)
{
	ASSERT_OR_RETURN(, psKiller != nullptr, "invalid Unit pointer");

	if (hasCommander(psKiller))
	{
		DROID *psCommander = psKiller->psGroup->psCommander;
		psCommander->experience += MIN(experienceInc, UINT32_MAX - psCommander->experience);
	}
}

/** This function returns true if the droid is assigned to a commander group and it is not the commander.*/
bool hasCommander(const DROID *psDroid)
{
	ASSERT_OR_RETURN(false, psDroid != nullptr, "invalid droid pointer");

	if (psDroid->droidType != DROID_COMMAND &&
	    psDroid->psGroup != nullptr &&
	    psDroid->psGroup->type == GT_COMMAND)
	{
		return true;
	}

	return false;
}

/** This function returns the level of a droids commander. If the droid doesn't have commander, it returns 0.*/
unsigned int cmdGetCommanderLevel(const DROID *psDroid)
{
	const DROID *psCommander;

	ASSERT(psDroid != nullptr, "invalid droid pointer");

	// If this droid is not the member of a Commander's group
	// Return an experience level of 0
	if (!hasCommander(psDroid))
	{
		return 0;
	}

	// Retrieve this group's commander
	psCommander = psDroid->psGroup->psCommander;

	// Return the experience level of this commander
	return getDroidLevel(psCommander);
}
