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
 * Mechanics.c
 *
 * Game world mechanics.
 *
 */

#include "lib/framework/frame.h"

#include "basedef.h"
#include "droid.h"
#include "feature.h"
#include "mechanics.h"
#include "objmem.h"
#include "research.h"
#include "structure.h"

/* Shutdown the mechanics system */
bool mechanicsShutdown(void)
{
	BASE_OBJECT *psObj, *psNext;

	for(psObj = psDestroyedObj; psObj != NULL; psObj = psNext)
	{
		psNext = psObj->psNext;
		delete psObj;
	}
	psDestroyedObj = NULL;

	return true;
}


// Allocate the list for a component
bool allocComponentList(COMPONENT_TYPE	type, SDWORD number)
{
	SDWORD	inc, comp;

	//allocate the space for the Players' component lists
	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		if (apCompLists[inc][type])
		{
			free(apCompLists[inc][type]);
		}

		apCompLists[inc][type] = (UBYTE *) malloc(sizeof(UBYTE) * number);
		if (apCompLists[inc][type] == NULL)
		{
			debug( LOG_FATAL, "Out of memory assigning Player Component Lists" );
			abort();
			return false;
		}

		//initialise the players' lists
		for (comp=0; comp < number; comp++)
		{
			apCompLists[inc][type][comp] = UNAVAILABLE;
		}
	}

	return true;
}

// release all the component lists
void freeComponentLists(void)
{
	UDWORD	inc;

	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		//free the component lists
		if (apCompLists[inc][COMP_BODY])
		{
			free(apCompLists[inc][COMP_BODY]);
			apCompLists[inc][COMP_BODY] = NULL;
		}
		if (apCompLists[inc][COMP_BRAIN])
		{
			free(apCompLists[inc][COMP_BRAIN]);
			apCompLists[inc][COMP_BRAIN] = NULL;
		}
		if (apCompLists[inc][COMP_PROPULSION])
		{
			free(apCompLists[inc][COMP_PROPULSION]);
			apCompLists[inc][COMP_PROPULSION] = NULL;
		}
		if (apCompLists[inc][COMP_SENSOR])
		{
			free(apCompLists[inc][COMP_SENSOR]);
			apCompLists[inc][COMP_SENSOR] = NULL;
		}
		if (apCompLists[inc][COMP_ECM])
		{
			free(apCompLists[inc][COMP_ECM]);
			apCompLists[inc][COMP_ECM] = NULL;
		}
		if (apCompLists[inc][COMP_REPAIRUNIT])
		{
			free(apCompLists[inc][COMP_REPAIRUNIT]);
			apCompLists[inc][COMP_REPAIRUNIT] = NULL;
		}
		if (apCompLists[inc][COMP_CONSTRUCT])
		{
			free(apCompLists[inc][COMP_CONSTRUCT]);
			apCompLists[inc][COMP_CONSTRUCT] = NULL;
		}
		if (apCompLists[inc][COMP_WEAPON])
		{
			free(apCompLists[inc][COMP_WEAPON]);
			apCompLists[inc][COMP_WEAPON] = NULL;
		}
	}
}

//allocate the space for the Players' structure lists
bool allocStructLists(void)
{
	SDWORD	inc, stat;

	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		if(numStructureStats)
		{
			apStructTypeLists[inc] = (UBYTE *) malloc(sizeof(UBYTE) *
								numStructureStats);
			if (apStructTypeLists[inc] == NULL)
			{
				debug( LOG_FATAL, "Out of memory assigning Player Structure Lists" );
				abort();
				return false;
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

	return true;
}


// release the structure lists
void freeStructureLists(void)
{
	UDWORD	inc;

	for (inc=0; inc < MAX_PLAYERS; inc++)
	{
		//free the structure lists
		if(apStructTypeLists[inc]) {
			free(apStructTypeLists[inc]);
			apStructTypeLists[inc] = NULL;
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

		//make all the structures available
		for (comp=0; comp < numStructureStats; comp++)
		{
			apStructTypeLists[i][comp] = AVAILABLE;
		}
		//make all research availble to be performed
		for (comp = 0; comp < asResearch.size(); comp++)
		{
			enableResearch(&asResearch[comp], i);
		}
	}
}

