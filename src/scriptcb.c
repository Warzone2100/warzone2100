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
 * ScriptCB.c
 *
 * functions to deal with parameterised script callback triggers.
 *
 */

#include <string.h>

#include "lib/framework/frame.h"
#include "objects.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "scriptcb.h"
#include "projectile.h"
#include "hci.h"
#include "group.h"
#include "transporter.h"
#include "mission.h"
#include "research.h"

static INTERP_VAL	scrFunctionResult;	//function return value to be pushed to stack

// unit taken over..
DROID		*psScrCBDroidTaken;

DROID		*psScrCBOrderDroid = NULL;		//Callback droid that have received an order
SDWORD		psScrCBOrder = DORDER_NONE;			//Order of the droid

//Script key event callback
SDWORD		cbPressedMetaKey;
SDWORD		cbPressedKey;

// The pointer to the droid that was just built for a CALL_NEWDROID
DROID		*psScrCBNewDroid;
STRUCTURE	*psScrCBNewDroidFact;	// id of factory that built it.

// the attacker and target for a CALL_ATTACKED
BASE_OBJECT	*psScrCBAttacker, *psScrCBTarget;


// alliance details
UDWORD	CBallFrom,CBallTo;

//console callback stuff
//---------------------------
SDWORD ConsolePlayer = -2;
SDWORD MultiMsgPlayerTo = -2;
SDWORD beaconX = -1, beaconY = -1;
SDWORD MultiMsgPlayerFrom = -2;
char ConsoleMsg[MAXSTRLEN]="ERROR!!!\0";	//Last console message
char MultiplayMsg[MAXSTRLEN];	//Last multiplayer message

BOOL scrCBDroidTaken(void)
{
	DROID		**ppsDroid;
	BOOL	triggered = FALSE;

	if (!stackPopParams(1, VAL_REF|ST_DROID, &ppsDroid))
	{
		return FALSE;
	}

	if (psScrCBDroidTaken == NULL)
	{
		triggered = FALSE;
		*ppsDroid = NULL;
	}
	else
	{
		triggered = TRUE;
		*ppsDroid = psScrCBDroidTaken;
	}

	psScrCBDroidTaken = NULL;

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

// Deal with a CALL_NEWDROID
BOOL scrCBNewDroid(void)
{
	SDWORD		player;
	DROID		**ppsDroid;
	STRUCTURE	**ppsStructure;
	BOOL	triggered = FALSE;

	if (!stackPopParams(3, VAL_INT, &player, VAL_REF|ST_DROID, &ppsDroid, VAL_REF|ST_STRUCTURE, &ppsStructure))
	{
		return FALSE;
	}

	if (psScrCBNewDroid == NULL)
	{
		// eh? got called without setting the new droid
		ASSERT( FALSE, "scrCBNewUnit: no unit has been set" );
		triggered = FALSE;
		*ppsDroid = NULL;
		*ppsStructure  = NULL;
	}
	else if (psScrCBNewDroid->player == (UDWORD)player)
	{
		triggered = TRUE;
		*ppsDroid = psScrCBNewDroid;
		*ppsStructure  = psScrCBNewDroidFact;
	}

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

// Deal with a CALL_STRUCT_ATTACKED
BOOL scrCBStructAttacked(void)
{
	SDWORD			player;
	STRUCTURE		**ppsTarget;
	BASE_OBJECT		**ppsAttacker;//, **ppsTarget;
	BOOL			triggered = FALSE;

	if (!stackPopParams(3, VAL_INT, &player,
						VAL_REF|ST_STRUCTURE, &ppsTarget,
						VAL_REF|ST_BASEOBJECT, &ppsAttacker))
	{
		return FALSE;
	}

	if (psLastStructHit == NULL)
	{
		ASSERT( FALSE, "scrCBStructAttacked: no target has been set" );
		triggered = FALSE;
		*ppsAttacker = NULL;
		*ppsTarget = NULL;
	}
	else if (psLastStructHit->player == (UDWORD)player)
	{
		triggered = TRUE;
		*ppsAttacker = g_pProjLastAttacker;
		*ppsTarget = psLastStructHit;
	}
	else
	{
		triggered = FALSE;
		*ppsAttacker = NULL;
		*ppsTarget = NULL;
	}

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

// Deal with a CALL_DROID_ATTACKED
BOOL scrCBDroidAttacked(void)
{
	SDWORD			player;
	DROID			**ppsTarget;
	BASE_OBJECT		**ppsAttacker;//, **ppsTarget;
	BOOL			triggered = FALSE;

	if (!stackPopParams(3, VAL_INT, &player,
						VAL_REF|ST_DROID, &ppsTarget,
						VAL_REF|ST_BASEOBJECT, &ppsAttacker))
	{
		return FALSE;
	}

	if (psLastDroidHit == NULL)
	{
		ASSERT( FALSE, "scrCBUnitAttacked: no target has been set" );
		triggered = FALSE;
		*ppsAttacker = NULL;
		*ppsTarget = NULL;
	}
	else if (psLastDroidHit->player == (UDWORD)player)
	{
		triggered = TRUE;
		*ppsAttacker = g_pProjLastAttacker;
		*ppsTarget = psLastDroidHit;
	}
	else
	{
		triggered = FALSE;
		*ppsAttacker = NULL;
		*ppsTarget = NULL;
	}

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

// Deal with a CALL_ATTACKED
BOOL scrCBAttacked(void)
{
	SDWORD			player;
	BASE_OBJECT		**ppsTarget;
	BASE_OBJECT		**ppsAttacker;//, **ppsTarget;
	BOOL			triggered = FALSE;

	if (!stackPopParams(3, VAL_INT, &player,
						VAL_REF|ST_BASEOBJECT, &ppsTarget,
						VAL_REF|ST_BASEOBJECT, &ppsAttacker))
	{
		return FALSE;
	}

	if (psScrCBTarget == NULL)
	{
		ASSERT( FALSE, "scrCBAttacked: no target has been set" );
		triggered = FALSE;
		*ppsAttacker = NULL;
		*ppsTarget = NULL;
	}
	else if (psScrCBTarget->player == (UDWORD)player)
	{
		triggered = TRUE;
		*ppsAttacker = g_pProjLastAttacker;
		*ppsTarget = psScrCBTarget;
	}
	else
	{
		triggered = FALSE;
		*ppsAttacker = NULL;
		*ppsTarget = NULL;
	}

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

// The button id

// deal with CALL_BUTTON_PRESSED
BOOL scrCBButtonPressed(void)
{
	UDWORD	button;
	BOOL	triggered = FALSE;

	if (!stackPopParams(1, VAL_INT, &button))
	{
		return FALSE;
	}

	if (button == intLastWidget)
	{
		triggered = TRUE;
	}

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

// the Droid that was selected for a CALL_DROID_SELECTED
DROID	*psCBSelectedDroid;

// deal with CALL_DROID_SELECTED
BOOL scrCBDroidSelected(void)
{
	DROID	**ppsDroid;

	if (!stackPopParams(1, VAL_REF|ST_DROID, &ppsDroid))
	{
		return FALSE;
	}

	ASSERT( psCBSelectedDroid != NULL,
		"scrSCUnitSelected: invalid unit pointer" );

	*ppsDroid = psCBSelectedDroid;

	scrFunctionResult.v.bval = TRUE;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}


// the object that was last killed for a CALL_OBJ_DESTROYED
BASE_OBJECT *psCBObjDestroyed;

// deal with a CALL_OBJ_DESTROYED
BOOL scrCBObjDestroyed(void)
{
	SDWORD			player;
	BASE_OBJECT		**ppsObj;
	BOOL			retval;

	if (!stackPopParams(2, VAL_INT, &player, VAL_REF|ST_BASEOBJECT, &ppsObj))
	{
		return FALSE;
	}

	if ( (psCBObjDestroyed != NULL) &&
		 (psCBObjDestroyed->player == (UDWORD)player) &&
		 (psCBObjDestroyed->type != OBJ_FEATURE) )
	{
		retval = TRUE;
		*ppsObj = psCBObjDestroyed;
	}
	else
	{
		retval = FALSE;
		*ppsObj = NULL;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}


// deal with a CALL_STRUCT_DESTROYED
BOOL scrCBStructDestroyed(void)
{
	SDWORD			player;
	BASE_OBJECT		**ppsObj;
	BOOL			retval;

	if (!stackPopParams(2, VAL_INT, &player, VAL_REF|ST_STRUCTURE, &ppsObj))
	{
		return FALSE;
	}

	if ( (psCBObjDestroyed != NULL) &&
		 (psCBObjDestroyed->player == (UDWORD)player) &&
		 (psCBObjDestroyed->type == OBJ_STRUCTURE) )
	{
		retval = TRUE;
		*ppsObj = psCBObjDestroyed;
	}
	else
	{
		retval = FALSE;
		*ppsObj = NULL;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}


// deal with a CALL_DROID_DESTROYED
BOOL scrCBDroidDestroyed(void)
{
	SDWORD			player;
	BASE_OBJECT		**ppsObj;
	BOOL			retval;

	if (!stackPopParams(2, VAL_INT, &player, VAL_REF|ST_DROID, &ppsObj))
	{
		return FALSE;
	}

	if ( (psCBObjDestroyed != NULL) &&
		 (psCBObjDestroyed->player == (UDWORD)player) &&
		 (psCBObjDestroyed->type == OBJ_DROID) )
	{
		retval = TRUE;
		*ppsObj = psCBObjDestroyed;
	}
	else
	{
		retval = FALSE;
		*ppsObj = NULL;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}


// deal with a CALL_FEATURE_DESTROYED
BOOL scrCBFeatureDestroyed(void)
{
	BASE_OBJECT		**ppsObj;
	BOOL			retval;

	if (!stackPopParams(1, VAL_REF|ST_FEATURE, &ppsObj))
	{
		return FALSE;
	}

	if (psCBObjDestroyed != NULL)
	{
		retval = TRUE;
		*ppsObj = psCBObjDestroyed;
	}
	else
	{
		retval = FALSE;
		*ppsObj = NULL;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}


// the last object to be seen for a CALL_OBJ_SEEN
BASE_OBJECT		*psScrCBObjSeen;
// the object that saw psScrCBObjSeen for a CALL_OBJ_SEEN
BASE_OBJECT		*psScrCBObjViewer;

// deal with all the object seen functions
static BOOL scrCBObjectSeen(SDWORD callback)
{
	BASE_OBJECT		**ppsObj;
	BASE_OBJECT		**ppsViewer;
	SDWORD			player;
	BOOL			retval;

	if (!stackPopParams(3, VAL_INT, &player, VAL_REF|ST_BASEOBJECT, &ppsObj, VAL_REF|ST_BASEOBJECT, &ppsViewer))
	{
		return FALSE;
	}

	if (psScrCBObjSeen == NULL)
	{
		ASSERT( FALSE,"scrCBObjectSeen: no object set" );
		return FALSE;
	}

	*ppsObj = NULL;
	if (((psScrCBObjViewer != NULL) &&
		 (psScrCBObjViewer->player != player)) ||
		!psScrCBObjSeen->visible[player])
	{
		retval = FALSE;
	}
	else if ((callback == CALL_DROID_SEEN) &&
			 (psScrCBObjSeen->type != OBJ_DROID))
	{
		retval = FALSE;
	}
	else if ((callback == CALL_STRUCT_SEEN) &&
			 (psScrCBObjSeen->type != OBJ_STRUCTURE))
	{
		retval = FALSE;
	}
	else if ((callback == CALL_FEATURE_SEEN) &&
			 (psScrCBObjSeen->type != OBJ_FEATURE))
	{
		retval = FALSE;
	}
	else
	{
		retval = TRUE;
		*ppsObj = psScrCBObjSeen;
		*ppsViewer = psScrCBObjViewer;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

// deal with a CALL_OBJ_SEEN
BOOL scrCBObjSeen(void)
{
	return scrCBObjectSeen(CALL_OBJ_SEEN);
}

// deal with a CALL_DROID_SEEN
BOOL scrCBDroidSeen(void)
{
	return scrCBObjectSeen(CALL_DROID_SEEN);
}

// deal with a CALL_STRUCT_SEEN
BOOL scrCBStructSeen(void)
{
	return scrCBObjectSeen(CALL_STRUCT_SEEN);
}

// deal with a CALL_FEATURE_SEEN
BOOL scrCBFeatureSeen(void)
{
	return scrCBObjectSeen(CALL_FEATURE_SEEN);
}

BOOL scrCBTransporterOffMap( void )
{
	SDWORD	player;
	BOOL	retval;
	DROID	*psTransporter;

	if (!stackPopParams(1, VAL_INT, &player) )
	{
		return FALSE;
	}

	psTransporter = transporterGetScriptCurrent();

	if ( (psTransporter != NULL) &&
		 (psTransporter->player == (UDWORD)player) )
	{
		retval = TRUE;
	}
	else
	{
		retval = FALSE;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL scrCBTransporterLanded( void )
{
	SDWORD			player;
	DROID_GROUP		*psGroup;
	DROID			*psTransporter, *psDroid, *psNext;
	BOOL			retval;

	if (!stackPopParams(2, ST_GROUP, &psGroup, VAL_INT, &player))
	{
		return FALSE;
	}

	psTransporter = transporterGetScriptCurrent();

	if ( (psTransporter == NULL) ||
		 (psTransporter->player != (UDWORD)player) )
	{
		retval = FALSE;
	}
	else
	{
		/* if not selectedPlayer unload droids */
		if ( (UDWORD)player != selectedPlayer )
		{
			/* transfer droids from transporter group to current group */
			for(psDroid=psTransporter->psGroup->psList; psDroid; psDroid=psNext)
			{
				psNext = psDroid->psGrpNext;
				if ( psDroid != psTransporter )
				{
					grpLeave( psTransporter->psGroup, psDroid );
					grpJoin(psGroup, psDroid);
				}
			}
		}

		retval = TRUE;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL scrCBTransporterLandedB( void )
{
	SDWORD			player;
	DROID_GROUP		*psGroup;
	DROID			*psTransporter, *psDroid, *psNext;
	BOOL			retval;
	DROID			**ppsTransp;

	if (!stackPopParams(3, ST_GROUP, &psGroup, VAL_INT, &player,
		VAL_REF|ST_DROID, &ppsTransp))
	{
		debug(LOG_ERROR, "scrCBTransporterLandedB(): stack failed");
		return FALSE;
	}

	psTransporter = transporterGetScriptCurrent();

	if ( (psTransporter == NULL) ||
		 (psTransporter->player != (UDWORD)player) )
	{
		retval = FALSE;
	}
	else
	{
		*ppsTransp = psTransporter;		//return landed transporter

		/* if not selectedPlayer unload droids */
		//if ( (UDWORD)player != selectedPlayer )
		//{
			/* transfer droids from transporter group to current group */
			for(psDroid=psTransporter->psGroup->psList; psDroid; psDroid=psNext)
			{
				psNext = psDroid->psGrpNext;
				if ( psDroid != psTransporter )
				{
					grpLeave( psTransporter->psGroup, psDroid );
					grpJoin(psGroup, psDroid);
				}
			}
		//}

		retval = TRUE;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrCBTransporterLandedB: push landed");
		return FALSE;
	}

	return TRUE;
}


// tell the scripts when a cluster is no longer valid
SDWORD	scrCBEmptyClusterID;
BOOL scrCBClusterEmpty( void )
{
	SDWORD		*pClusterID;

	if (!stackPopParams(1, VAL_REF|VAL_INT, &pClusterID))
	{
		return FALSE;
	}

	*pClusterID = scrCBEmptyClusterID;

	scrFunctionResult.v.bval = TRUE;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

// note when a vtol has finished returning to base - used to vanish
// vtols when they are attacking from off map
DROID *psScrCBVtolOffMap;
BOOL scrCBVtolOffMap(void)
{
	SDWORD	player;
	DROID	**ppsVtol;
	BOOL	retval;

	if (!stackPopParams(2, VAL_INT, &player, VAL_REF|ST_DROID, &ppsVtol))
	{
		return FALSE;
	}

	if (psScrCBVtolOffMap == NULL)
	{
		ASSERT( FALSE, "scrCBVtolAtBase: NULL vtol pointer" );
		return FALSE;
	}

	retval = FALSE;
	if (psScrCBVtolOffMap->player == player)
	{
		retval = TRUE;
		*ppsVtol = psScrCBVtolOffMap;
	}
	psScrCBVtolOffMap = NULL;

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

/*called when selectedPlayer completes some research*/
BOOL scrCBResCompleted(void)
{
	RESEARCH	**ppsResearch;
	STRUCTURE	**ppsResFac;
	BOOL	    retVal;
	SDWORD		resFacOwner;

	if (!stackPopParams(3, VAL_REF|ST_RESEARCH, &ppsResearch,
		VAL_REF|ST_STRUCTURE, &ppsResFac ,VAL_INT, &resFacOwner))
	{
		return FALSE;
	}

	retVal = FALSE;
	*ppsResearch = NULL;
	*ppsResFac = NULL;

	if(resFacOwner == -1 || resFacOwner == CBResFacilityOwner)
	{
		if (psCBLastResearch != NULL)
		{
			retVal = TRUE;
			*ppsResearch = psCBLastResearch;
			*ppsResFac = psCBLastResStructure;
		}
		else
		{
			ASSERT( FALSE, "scrCBResCompleted: no research has been set" );
		}
	}

	scrFunctionResult.v.bval = retVal;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}


/* when a humna player leaves a game*/
BOOL scrCBPlayerLeft(void)
{
	SDWORD	player;
	if (!stackPopParams(1, VAL_INT, &player) )
	{
		return FALSE;
	}

	scrFunctionResult.v.bval = TRUE;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}


// alliance has been offered.
BOOL scrCBAllianceOffer(void)
{
	SDWORD	*from,*to;

	if (!stackPopParams(2, VAL_REF | VAL_INT, &from, VAL_REF | VAL_INT,&to) )
	{
		return FALSE;
	}

	*from = CBallFrom;
	*to = CBallTo;

	scrFunctionResult.v.bval = TRUE;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

//------------------------------------------------------------------------------------------------
										/* New callbacks */


//console callback
//---------------------------
BOOL scrCallConsole(void)
{
	SDWORD	*player;
	char	**ConsoleText = NULL;

	if (!stackPopParams(2, VAL_REF | VAL_INT, &player, VAL_REF | VAL_STRING, &ConsoleText) ) 
	{
		debug(LOG_ERROR, "scrCallConsole(): stack failed");
		return FALSE;
	}

	if(*ConsoleText == NULL)
	{
		debug(LOG_ERROR, "scrCallConsole(): passed string was not initialized");
		return FALSE;
	}

	strcpy(*ConsoleText,ConsoleMsg);

	*player = ConsolePlayer;

	scrFunctionResult.v.bval = TRUE;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrCallConsole(): stackPushResult failed");
		return FALSE;
	}

	return TRUE;
}

//multiplayer beacon
//---------------------------
BOOL scrCallBeacon(void)
{
	SDWORD	*playerFrom, playerTo;
	char	**BeaconText = NULL;
	SDWORD	*locX,*locY;

	if (!stackPopParams(5, VAL_INT, &playerTo, VAL_REF | VAL_INT, &playerFrom,
		VAL_REF | VAL_INT, &locX, VAL_REF | VAL_INT, &locY,
		VAL_REF | VAL_STRING, &BeaconText)) 
	{
		debug(LOG_ERROR, "scrCallBeacon() - failed to pop parameters.");
		return FALSE;
	}

	
	if(*BeaconText == NULL)
	{
		debug(LOG_ERROR, "scrCallBeacon(): passed string was not initialized");
		return FALSE;
	}

	if(MultiMsgPlayerTo >= 0 && MultiMsgPlayerFrom >= 0 && MultiMsgPlayerTo < MAX_PLAYERS && MultiMsgPlayerFrom < MAX_PLAYERS)
	{

		if(MultiMsgPlayerTo == playerTo)
		{
			strcpy(*BeaconText,MultiplayMsg);
	 
			*playerFrom = MultiMsgPlayerFrom;
			*locX = beaconX;
			*locY = beaconY;

			scrFunctionResult.v.bval = TRUE;
			if (!stackPushResult(VAL_BOOL, &scrFunctionResult))	//triggered
			{
				debug(LOG_ERROR, "scrCallBeacon - failed to push");
				return FALSE;
			}

			return TRUE;
		}
	}
	else
	{
		debug(LOG_ERROR, "scrCallBeacon() - player indexes failed: %d - %d", MultiMsgPlayerFrom, MultiMsgPlayerTo);
		scrFunctionResult.v.bval = FALSE;
		if (!stackPushResult(VAL_BOOL, &scrFunctionResult))	//not triggered
		{
			return FALSE;
		}

		return TRUE;
	}

	//return "not triggered"
	scrFunctionResult.v.bval = FALSE;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}
	
	return TRUE;
}

//multiplayer message callback
//----------------------------
BOOL scrCallMultiMsg(void)
{
	SDWORD	*player, playerTo;
	char	**ConsoleText = NULL;

	if (!stackPopParams(3, VAL_INT, &playerTo, VAL_REF | VAL_INT, &player, VAL_REF | VAL_STRING, &ConsoleText) ) 
	{
		debug(LOG_ERROR, "scrCallMultiMsg() failed to pop parameters.");
		return FALSE;
	}

	if(*ConsoleText == NULL)
	{
		debug(LOG_ERROR, "scrCallMultiMsg(): passed string was not initialized");
		return FALSE;
	}

	if(MultiMsgPlayerTo >= 0 && MultiMsgPlayerFrom >= 0 && MultiMsgPlayerTo < MAX_PLAYERS && MultiMsgPlayerFrom < MAX_PLAYERS)
	{
		if(MultiMsgPlayerTo == playerTo)
		{
			strcpy(*ConsoleText,MultiplayMsg);
	 
			*player = MultiMsgPlayerFrom;

			scrFunctionResult.v.bval = TRUE;
			if (!stackPushResult(VAL_BOOL, &scrFunctionResult))	//triggered
			{
				debug(LOG_ERROR, "scrCallMultiMsg(): stackPushResult failed");
				return FALSE;
			}

			return TRUE;
		}
	}
	else
	{
		debug(LOG_ERROR, "scrCallMultiMsg() - player indexes failed: %d - %d", MultiMsgPlayerFrom, MultiMsgPlayerTo);
		scrFunctionResult.v.bval = FALSE;
		if (!stackPushResult(VAL_BOOL, &scrFunctionResult))	//not triggered
		{
			return FALSE;
		}

		return TRUE;
	}

	//return "not triggered"
	scrFunctionResult.v.bval = FALSE;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrCallMultiMsg: stackPushResult failed");
		return FALSE;
	}
	
	return TRUE;
}

STRUCTURE	*psScrCBNewStruct = NULL;	//for scrCBStructBuilt callback
DROID		*psScrCBNewStructTruck = NULL;
//structure built callback
//------------------------------
BOOL scrCBStructBuilt(void)
{
	SDWORD		player;
	STRUCTURE	**ppsStructure;
	BOOL		triggered = FALSE;
	DROID		**ppsDroid;

	if (!stackPopParams(3, VAL_INT, &player, VAL_REF|ST_DROID, &ppsDroid, VAL_REF|ST_STRUCTURE, &ppsStructure) ) 
	{
		debug(LOG_ERROR, "scrCBStructBuilt() failed to pop parameters.");
		return FALSE;
	}

	if (psScrCBNewStruct == NULL)
	{
		debug(LOG_ERROR, "scrCBStructBuilt: no structure has been set");
		ASSERT( FALSE, "scrCBStructBuilt: no structure has been set" );
		triggered = FALSE;
		*ppsStructure  = NULL;
		*ppsDroid = NULL;
	}
	else if(psScrCBNewStructTruck == NULL)
	{
		debug(LOG_ERROR, "scrCBStructBuilt: no builder has been set");
		ASSERT( FALSE, "scrCBStructBuilt: no builder has been set" );
		triggered = FALSE;
		*ppsStructure  = NULL;
		*ppsDroid = NULL;
	}
	else if (psScrCBNewStruct->player == (UDWORD)player)
	{
		triggered = TRUE;
		*ppsStructure  = psScrCBNewStruct;		//pass to script
		*ppsDroid = psScrCBNewStructTruck;
	}

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrCBStructBuilt: push failed");
		return FALSE;
	}

	return TRUE;
}

/* Droid received stop order */
BOOL scrCBDorderStop(void)
{
	SDWORD		player;
	DROID		**ppsDroid;
	BOOL	triggered = FALSE;

	if (!stackPopParams(2, VAL_INT, &player, VAL_REF|ST_DROID, &ppsDroid))
	{
		debug(LOG_ERROR, "scrCBDorderStop: failed to pop");
		return FALSE;
	}

	if (psScrCBOrderDroid == NULL)	//if droid that received stop order was destroyed
	{
		ASSERT( FALSE, "scrCBDorderStop: psScrCBOrderDroid is NULL" );
		triggered = FALSE;
		*ppsDroid = NULL;
	}
	else if (psScrCBOrderDroid->player == (UDWORD)player)
	{
		triggered = TRUE;
		*ppsDroid = psScrCBOrderDroid;
	}

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

/* Droid reached destination point and stopped on its own */
BOOL scrCBDorderReachedLocation(void)
{
	SDWORD		player;
	SDWORD		*Order = NULL;
	DROID		**ppsDroid;
	BOOL	triggered = FALSE;

	if (!stackPopParams(3, VAL_INT, &player, VAL_REF|ST_DROID, &ppsDroid
		,VAL_REF | VAL_INT, &Order))
	{
		debug(LOG_ERROR, "scrCBDorderReachedLocation: failed to pop");
		return FALSE;
	}

	if (psScrCBOrderDroid == NULL)	//if droid was destroyed
	{
		ASSERT( FALSE, "scrCBDorderReachedLocation: psScrCBOrderDroid is NULL" );
		triggered = FALSE;
		*ppsDroid = NULL;
	}
	else if (psScrCBOrderDroid->player == (UDWORD)player)
	{
		triggered = TRUE;
		*ppsDroid = psScrCBOrderDroid;
		*Order = psScrCBOrder;
	}

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

/* Process key-combo */
BOOL scrCBProcessKeyPress(void)
{
	SDWORD		*key = NULL, *metaKey = NULL;

	if (!stackPopParams(2,VAL_REF | VAL_INT, &key, VAL_REF | VAL_INT, &metaKey))
	{
		debug(LOG_ERROR, "scrCBProcessKeyPress: failed to pop");
		return FALSE;
	}

	*key = cbPressedKey;
	*metaKey = cbPressedMetaKey;

	scrFunctionResult.v.bval = TRUE;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))		//triggered
	{
		return FALSE;
	}

	return TRUE;
}
