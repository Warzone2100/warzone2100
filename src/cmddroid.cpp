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
 * CmdDroid.c
 *
 * Code for command droids
 *
 */
#include <string.h>
#include "lib/framework/frame.h"
#include "objects.h"
#include "cmddroiddef.h"
#include "cmddroid.h"
#include "lib/gamelib/gtime.h"
#include "group.h"
#include "order.h"
#include "multiplay.h"

extern UDWORD selectedPlayer;

// the current command droid target designator for IDF structures
DROID	*apsCmdDesignator[MAX_PLAYERS];

// whether experience should be boosted due to a multi game
static BOOL bMultiExpBoost = false;

// Initialise the command droids
BOOL cmdDroidInit(void)
{
	memset(apsCmdDesignator, 0, sizeof(DROID *)*MAX_PLAYERS);

	return true;
}


// ShutDown the command droids
void cmdDroidShutDown(void)
{
}


// Make new command droids available
void cmdDroidAvailable(WZ_DECL_UNUSED BRAIN_STATS *psBrainStats, WZ_DECL_UNUSED SDWORD player)
{
}


// update the command droids
void cmdDroidUpdate(void)
{
	SDWORD	i;

	for(i=0; i<MAX_PLAYERS; i++)
	{
		if (apsCmdDesignator[i] && apsCmdDesignator[i]->died)
		{
			apsCmdDesignator[i] = NULL;
		}
	}
}


// add a droid to a command group
void cmdDroidAddDroid(DROID *psCommander, DROID *psDroid)
{
	DROID_GROUP	*psGroup;

	if (psCommander->psGroup == NULL)
	{
		if (!grpCreate(&psGroup))
		{
			return;
		}
		grpJoin(psGroup, psCommander);
		psDroid->group = UBYTE_MAX;
	}

	if (grpNumMembers(psCommander->psGroup) < cmdDroidMaxGroup(psCommander))
	{
		grpJoin(psCommander->psGroup, psDroid);
		psDroid->group = UBYTE_MAX;

		// set the secondary states for the unit
		secondarySetState(psDroid, DSO_ATTACK_RANGE, (SECONDARY_STATE)(psCommander->secondaryOrder & DSS_ARANGE_MASK));
		secondarySetState(psDroid, DSO_REPAIR_LEVEL, (SECONDARY_STATE)(psCommander->secondaryOrder & DSS_REPLEV_MASK));
		secondarySetState(psDroid, DSO_ATTACK_LEVEL, (SECONDARY_STATE)(psCommander->secondaryOrder & DSS_ALEV_MASK));
		secondarySetState(psDroid, DSO_HALTTYPE, (SECONDARY_STATE)(psCommander->secondaryOrder & DSS_HALT_MASK));

		orderDroidObj(psDroid, DORDER_GUARD, (BASE_OBJECT *)psCommander, ModeQueue);
	}
}

// return the current target designator for a player
DROID *cmdDroidGetDesignator(UDWORD player)
{
	return apsCmdDesignator[player];
}

// set the current target designator for a player
void cmdDroidSetDesignator(DROID *psDroid)
{
	if (psDroid->droidType != DROID_COMMAND)
	{
		return;
	}

	apsCmdDesignator[psDroid->player] = psDroid;
}

// set the current target designator for a player
void cmdDroidClearDesignator(UDWORD player)
{
	apsCmdDesignator[player] = NULL;
}

// get the index of the command droid
SDWORD cmdDroidGetIndex(DROID *psCommander)
{
	SDWORD	index = 1;
	DROID	*psCurr;

	if (psCommander->droidType != DROID_COMMAND)
	{
		return 0;
	}

	for(psCurr=apsDroidLists[psCommander->player]; psCurr; psCurr=psCurr->psNext)
	{
		if (psCurr->droidType == DROID_COMMAND &&
			psCurr->id < psCommander->id)
		{
			index += 1;
		}
	}

	return index;
}


// note that commander experience should be increased
void cmdDroidMultiExpBoost(BOOL bDoit)
{
	bMultiExpBoost = bDoit;
}

BOOL cmdGetDroidMultiExpBoost()
{
	return bMultiExpBoost;
}

// get the maximum group size for a command droid
unsigned int cmdDroidMaxGroup(const DROID* psCommander)
{
	return getDroidLevel(psCommander) * 2 + MIN_CMD_GROUP_DROIDS;
}

// update the kills of a command droid if psKiller is in a command group
void cmdDroidUpdateKills(DROID *psKiller, uint32_t experienceInc)
{
	ASSERT_OR_RETURN( , psKiller != NULL, "invalid Unit pointer" );

	if (hasCommander(psKiller))
	{
		DROID *psCommander = psKiller->psGroup->psCommander;
		psCommander->experience += MIN(experienceInc, UINT32_MAX - psCommander->experience);
	}
}

// returns true if a droid in question is assigned to a commander
bool hasCommander(const DROID* psDroid)
{
	ASSERT_OR_RETURN(false, psDroid != NULL, "invalid droid pointer");

	if (psDroid->droidType != DROID_COMMAND &&
		psDroid->psGroup != NULL &&
	    psDroid->psGroup->type == GT_COMMAND)
	{
		return true;
	}

	return false;
}

// get the level of a droids commander, if any
unsigned int cmdGetCommanderLevel(const DROID* psDroid)
{
	const DROID* psCommander;

	ASSERT(psDroid != NULL, "invalid droid pointer");

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

// Selects all droids for a given commander
void cmdSelectSubDroids(DROID *psDroid)
{
	DROID *psCurr;

	for (psCurr = apsDroidLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
	{
		if (hasCommander(psCurr)
		 && psCurr->psGroup->psCommander == psDroid)
		{
			SelectDroid(psCurr);
		}
	}
}
