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
BOOL bMultiExpBoost = FALSE;


// Initialise the command droids
BOOL cmdDroidInit(void)
{
	memset(apsCmdDesignator, 0, sizeof(DROID *)*MAX_PLAYERS);

	return TRUE;
}


// ShutDown the command droids
void cmdDroidShutDown(void)
{
}


// Make new command droids available
void cmdDroidAvailable(BRAIN_STATS *psBrainStats, SDWORD player)
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
		DROID_OACTION_INFO oaInfo = {{(BASE_OBJECT*)psCommander}};

		grpJoin(psCommander->psGroup, psDroid);
		psDroid->group = UBYTE_MAX;

		// set the secondary states for the unit
		secondarySetState(psDroid, DSO_ATTACK_RANGE, (SECONDARY_STATE)(psCommander->secondaryOrder & DSS_ARANGE_MASK));
		secondarySetState(psDroid, DSO_REPAIR_LEVEL, (SECONDARY_STATE)(psCommander->secondaryOrder & DSS_REPLEV_MASK));
		secondarySetState(psDroid, DSO_ATTACK_LEVEL, (SECONDARY_STATE)(psCommander->secondaryOrder & DSS_ALEV_MASK));
		secondarySetState(psDroid, DSO_HALTTYPE, (SECONDARY_STATE)(psCommander->secondaryOrder & DSS_HALT_MASK));

		orderDroidObj(psDroid, DORDER_GUARD, &oaInfo);
	}

//	if(bMultiPlayer && myResponsibility(psDroid->player) )
//	{
//		sendCommandDroid(psCommander,psDroid);
//	}
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


// get the experience level of a command droid
SDWORD cmdDroidGetLevel(DROID *psCommander)
{
	SDWORD	numKills = psCommander->numKills;

	// commanders do not need as much experience in multiplayer
	if (bMultiExpBoost)
	{
		numKills *= 2;
	}
	if (numKills > 2047)
	{
		return 8;
	}
	else if (numKills > 1023)
	{
		return 7;
	}
	else if (numKills > 511)
	{
		return 6;
	}
	else if (numKills > 255)
	{
		return 5;
	}
	else if (numKills > 127)
	{
		return 4;
	}
	else if (numKills > 63)
	{
		return 3;
	}
	else if (numKills > 32)
	{
		return 2;
	}
	else if (numKills > 15)
	{
		return 1;
	}

	return 0;
}

// get the maximum group size for a command droid
SDWORD cmdDroidMaxGroup(DROID *psCommander)
{
	return cmdDroidGetLevel(psCommander) * 2 + 6;
}

// update the kills of a command droid if psKiller is in a command group
void cmdDroidUpdateKills(DROID *psKiller)
{
	DROID	*psCommander;

	ASSERT( PTRVALID(psKiller, sizeof(DROID)),
		"cmdUnitUpdateKills: invalid Unit pointer" );

	if ( (psKiller->psGroup != NULL) &&
		 (psKiller->psGroup->type == GT_COMMAND) )
	{
		psCommander = psKiller->psGroup->psCommander;
		psCommander->numKills += 1;
	}
}

// get the level of a droids commander, if any
SDWORD cmdGetCommanderLevel(DROID *psDroid)
{
	DROID	*psCommander;

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"cmdGetCommanderLevel: invalid droid pointer" );

	if ( (psDroid->psGroup != NULL) &&
		 (psDroid->psGroup->type == GT_COMMAND) )
	{
		psCommander = psDroid->psGroup->psCommander;

		return cmdDroidGetLevel(psCommander);
	}

	return 0;
}

// Selects all droids for a given commander
void	cmdSelectSubDroids(DROID *psDroid)
{
DROID	*psCurr;

	for (psCurr = apsDroidLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
	{
		if( (psCurr->psGroup!=NULL) && (psCurr->psGroup->type == GT_COMMAND) )
		{
			if(psCurr->psGroup->psCommander == psDroid)
			{
//				psCurr->selected = TRUE;
				SelectDroid(psCurr);
			}
		}
	}
}

// set the number of command droids for a player
static void cmdDroidSetAvailable(SDWORD player, SDWORD num)
{
	ASSERT( (player >= 0) && (player < MAX_PLAYERS),
		"cmdUnitSetAvailable: invalid player number" );
	ASSERT( (num > 0) && (num < MAX_CMDDROIDS),
		"cmdUnitSetAvailable: invalid player number" );
}






