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
 * Mechanics.c
 *
 * Game world mechanics.
 *
 */

#include "lib/framework/frame.h"
#include "mechanics.h"
#include "stats.h"
#include "structure.h"
#include "game.h"
#include "power.h"
#include "projectile.h"
#include "move.h"
#include "message.h"
#include "deliverance.h"
#include "astar.h"
#include "visibility.h"

/* Shutdown the mechanics system */
BOOL mechShutdown(void)
{
	BASE_OBJECT *psObj, *psNext;

	for(psObj = psDestroyedObj; psObj; psObj = psNext)
	{
		psNext = psObj->psNext;
		if (psObj->type == OBJ_DROID)
		{
			droidRelease((DROID *)psObj);
			heapFree(psDroidHeap, (DROID *)psObj);
		}
		if (psObj->type == OBJ_STRUCTURE)
		{
			structureRelease((STRUCTURE *)psObj);
			heapFree(psStructHeap, (STRUCTURE *)psObj);
		}
		if (psObj->type == OBJ_FEATURE)
		{
			featureRelease((FEATURE *)psObj);
			heapFree(psFeatureHeap, (FEATURE *)psObj);
		}
	}
	psDestroyedObj = NULL;

	return TRUE;
}


// Allocate the list for a component
BOOL allocComponentList(COMPONENT_TYPE	type, SDWORD number)
{
	SDWORD	inc, comp;

	//allocate the space for the Players' component lists
	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		apCompLists[inc][type] = (UBYTE *) MALLOC(sizeof(UBYTE) * number);
		if (apCompLists[inc][type] == NULL)
		{
			debug( LOG_ERROR, "Out of memory assigning Player Component Lists" );
			abort();
			return FALSE;
		}

		//initialise the players' lists
		for (comp=0; comp <number; comp++)
		{
			apCompLists[inc][type][comp] = UNAVAILABLE;
		}
	}

	return TRUE;
}

// release all the component lists
void freeComponentLists(void)
{
	UDWORD	inc;

	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		//free the component lists
		FREE(apCompLists[inc][COMP_BODY]);
		FREE(apCompLists[inc][COMP_BRAIN]);
		FREE(apCompLists[inc][COMP_PROPULSION]);
		FREE(apCompLists[inc][COMP_SENSOR]);
		FREE(apCompLists[inc][COMP_ECM]);
		FREE(apCompLists[inc][COMP_REPAIRUNIT]);
		FREE(apCompLists[inc][COMP_CONSTRUCT]);
		FREE(apCompLists[inc][COMP_WEAPON]);
		//FREE(apCompLists[inc][COMP_PROGRAM]);
	}
}

//allocate the space for the Players' structure lists
BOOL allocStructLists(void)
{
	SDWORD	inc, stat;

	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		if(numStructureStats)
		{
			apStructTypeLists[inc] = (UBYTE *) MALLOC(sizeof(UBYTE) *
								numStructureStats);
			if (apStructTypeLists[inc] == NULL)
			{
				debug( LOG_ERROR, "Out of memory assigning Player Structure Lists" );
				abort();
				return FALSE;
			}
			for (stat = 0; stat < (SDWORD)numStructureStats; stat++)
			{
				apStructTypeLists[inc][stat] = UNAVAILABLE;
			}
		}
		else
		{
			apStructTypeLists[inc] = NULL;
		}
	}

	return TRUE;
}


// release the structure lists
void freeStructureLists(void)
{
	UDWORD	inc;

	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		//free the structure lists
		if(apStructTypeLists[inc]) {
			FREE(apStructTypeLists[inc]);
		}
	}
}


//TEST FUNCTION - MAKE EVERYTHING AVAILABLE
void makeAllAvailable(void)
{
	UDWORD	comp,i;

	for(i=0;i<MAX_PLAYERS;i++)
	{
		for (comp=0; comp <numWeaponStats; comp++)
		{
			apCompLists[i][COMP_WEAPON][comp] = AVAILABLE;
		}
		for (comp=0; comp <numBodyStats; comp++)
		{
			apCompLists[i][COMP_BODY][comp] = AVAILABLE;
		}
		for (comp=0; comp <numPropulsionStats; comp++)
		{
			apCompLists[i][COMP_PROPULSION][comp] = AVAILABLE;
		}
		for (comp=0; comp <numSensorStats; comp++)
		{
			apCompLists[i][COMP_SENSOR][comp] = AVAILABLE;
		}
		for (comp=0; comp <numECMStats; comp++)
		{
			apCompLists[i][COMP_ECM][comp] = AVAILABLE;
		}
		for (comp=0; comp <numConstructStats; comp++)
		{
			apCompLists[i][COMP_CONSTRUCT][comp] = AVAILABLE;
		}
		for (comp=0; comp <numBrainStats; comp++)
		{
			apCompLists[i][COMP_BRAIN][comp] = AVAILABLE;
		}
		for (comp=0; comp <numRepairStats; comp++)
		{
			apCompLists[i][COMP_REPAIRUNIT][comp] = AVAILABLE;
		}
		/*for (comp=i; comp <numProgramStats; comp++)
		{
			apCompLists[i][COMP_PROGRAM][comp] = AVAILABLE;
		}*/

		//make all the structures available
		for (comp=0; comp < numStructureStats; comp++)
		{
			apStructTypeLists[i][comp] = AVAILABLE;
		}
		//make all research availble to be performed
		for (comp = 0; comp < numResearch; comp++)
		{
			enableResearch(&asResearch[comp], i);
		}
	}
}

