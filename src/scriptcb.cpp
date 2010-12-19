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
#include "scriptobj.h"

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

// vtol target
DROID		*psScrVtolRetarget = NULL;

// alliance details
UDWORD	CBallFrom,CBallTo;

// player number that left the game
UDWORD	CBPlayerLeft;

//console callback stuff
//---------------------------
SDWORD ConsolePlayer = -2;
SDWORD MultiMsgPlayerTo = -2;
SDWORD beaconX = -1, beaconY = -1;
SDWORD MultiMsgPlayerFrom = -2;
char ConsoleMsg[MAXSTRLEN]="ERROR!!!\0";	//Last console message
char MultiplayMsg[MAXSTRLEN];	//Last multiplayer message


///////////////////////////////////////////

typedef struct
{
    DROID *droid;
    STRUCTURE *factory;
} NEWDROID;

typedef struct
{
    TRIGGER_TYPE type;
    union p_t
    {
        NEWDROID newDroid;
    } p;
} EVENT;

WZ_DECL_UNUSED static BOOL scrCBDroidTaken(void)
{
	DROID		**ppsDroid;
	BOOL	triggered = false;

	if (!stackPopParams(1, VAL_REF|ST_DROID, &ppsDroid))
	{
		return false;
	}

	if (psScrCBDroidTaken == NULL)
	{
		triggered = false;
		*ppsDroid = NULL;
	}
	else
	{
		triggered = true;
		*ppsDroid = psScrCBDroidTaken;
	}

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// Deal with a CALL_STRUCT_ATTACKED
WZ_DECL_UNUSED static BOOL scrCBStructAttacked(void)
{
	SDWORD			player;
	STRUCTURE		**ppsTarget;
	BASE_OBJECT		**ppsAttacker;//, **ppsTarget;
	BOOL			triggered = false;

	if (!stackPopParams(3, VAL_INT, &player,
						VAL_REF|ST_STRUCTURE, &ppsTarget,
						VAL_REF|ST_BASEOBJECT, &ppsAttacker))
	{
		return false;
	}

	if (psLastStructHit == NULL)
	{
		ASSERT( false, "scrCBStructAttacked: no target has been set" );
		triggered = false;
		*ppsAttacker = NULL;
		*ppsTarget = NULL;
	}
	else if (psLastStructHit->player == (UDWORD)player)
	{
		triggered = true;
		*ppsAttacker = g_pProjLastAttacker;
		*ppsTarget = psLastStructHit;
	}
	else
	{
		triggered = false;
		*ppsAttacker = NULL;
		*ppsTarget = NULL;
	}

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

BOOL scrCBVTOLRetarget(void)
{
	SDWORD			player;
	DROID			**ppsDroid;

	if (!stackPopParams(2, VAL_INT, &player, VAL_REF|ST_DROID, &ppsDroid))
	{
		return false;
	}
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS && player >= 0, "Invalid player %d", player);

	if (player == psScrVtolRetarget->player)
	{
		*ppsDroid = psScrVtolRetarget;
		scrFunctionResult.v.bval = true;
	}
	else
	{
		*ppsDroid = NULL;
		scrFunctionResult.v.bval = false;
	}
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// Deal with a CALL_DROID_ATTACKED
WZ_DECL_UNUSED static BOOL scrCBDroidAttacked(void)
{
	SDWORD			player;
	DROID			**ppsTarget;
	BASE_OBJECT		**ppsAttacker;//, **ppsTarget;
	BOOL			triggered = false;

	if (!stackPopParams(3, VAL_INT, &player,
						VAL_REF|ST_DROID, &ppsTarget,
						VAL_REF|ST_BASEOBJECT, &ppsAttacker))
	{
		return false;
	}

	if (psLastDroidHit == NULL)
	{
		ASSERT( false, "scrCBUnitAttacked: no target has been set" );
		triggered = false;
		*ppsAttacker = NULL;
		*ppsTarget = NULL;
	}
	else if (psLastDroidHit->player == (UDWORD)player)
	{
		triggered = true;
		*ppsAttacker = g_pProjLastAttacker;
		*ppsTarget = psLastDroidHit;
	}
	else
	{
		triggered = false;
		*ppsAttacker = NULL;
		*ppsTarget = NULL;
	}

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// Deal with a CALL_ATTACKED
WZ_DECL_UNUSED static BOOL scrCBAttacked(void)
{
	SDWORD			player;
	BASE_OBJECT		**ppsTarget;
	BASE_OBJECT		**ppsAttacker;//, **ppsTarget;
	BOOL			triggered = false;

	if (!stackPopParams(3, VAL_INT, &player,
						VAL_REF|ST_BASEOBJECT, &ppsTarget,
						VAL_REF|ST_BASEOBJECT, &ppsAttacker))
	{
		return false;
	}

	if (psScrCBTarget == NULL)
	{
		ASSERT( false, "scrCBAttacked: no target has been set" );
		triggered = false;
		*ppsAttacker = NULL;
		*ppsTarget = NULL;
	}
	else if (psScrCBTarget->player == (UDWORD)player)
	{
		triggered = true;
		*ppsAttacker = g_pProjLastAttacker;
		*ppsTarget = psScrCBTarget;
	}
	else
	{
		triggered = false;
		*ppsAttacker = NULL;
		*ppsTarget = NULL;
	}

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// The button id

// deal with CALL_BUTTON_PRESSED
WZ_DECL_UNUSED static BOOL scrCBButtonPressed(void)
{
	UDWORD	button;
	BOOL	triggered = false;

	if (!stackPopParams(1, VAL_INT, &button))
	{
		return false;
	}

	if (button == intLastWidget)
	{
		triggered = true;
	}

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// the Droid that was selected for a CALL_DROID_SELECTED
DROID	*psCBSelectedDroid;

// deal with CALL_DROID_SELECTED
WZ_DECL_UNUSED static BOOL scrCBDroidSelected(void)
{
	DROID	**ppsDroid;

	if (!stackPopParams(1, VAL_REF|ST_DROID, &ppsDroid))
	{
		return false;
	}

	ASSERT( psCBSelectedDroid != NULL,
		"scrSCUnitSelected: invalid unit pointer" );

	*ppsDroid = psCBSelectedDroid;

	scrFunctionResult.v.bval = true;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// the object that was last killed for a CALL_OBJ_DESTROYED
BASE_OBJECT *psCBObjDestroyed;

// deal with a CALL_OBJ_DESTROYED
WZ_DECL_UNUSED static BOOL scrCBObjDestroyed(void)
{
	SDWORD			player;
	BASE_OBJECT		**ppsObj;
	BOOL			retval;

	if (!stackPopParams(2, VAL_INT, &player, VAL_REF|ST_BASEOBJECT, &ppsObj))
	{
		return false;
	}

	if ( (psCBObjDestroyed != NULL) &&
		 (psCBObjDestroyed->player == (UDWORD)player) &&
		 (psCBObjDestroyed->type != OBJ_FEATURE) )
	{
		retval = true;
		*ppsObj = psCBObjDestroyed;
	}
	else
	{
		retval = false;
		*ppsObj = NULL;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// deal with a CALL_STRUCT_DESTROYED
WZ_DECL_UNUSED static BOOL scrCBStructDestroyed(void)
{
	SDWORD			player;
	BASE_OBJECT		**ppsObj;
	BOOL			retval;

	if (!stackPopParams(2, VAL_INT, &player, VAL_REF|ST_STRUCTURE, &ppsObj))
	{
		return false;
	}

	if ( (psCBObjDestroyed != NULL) &&
		 (psCBObjDestroyed->player == (UDWORD)player) &&
		 (psCBObjDestroyed->type == OBJ_STRUCTURE) )
	{
		retval = true;
		*ppsObj = psCBObjDestroyed;
	}
	else
	{
		retval = false;
		*ppsObj = NULL;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// deal with a CALL_DROID_DESTROYED
WZ_DECL_UNUSED static BOOL scrCBDroidDestroyed(void)
{
	SDWORD			player;
	BASE_OBJECT		**ppsObj;
	BOOL			retval;

	if (!stackPopParams(2, VAL_INT, &player, VAL_REF|ST_DROID, &ppsObj))
	{
		return false;
	}

	if ( (psCBObjDestroyed != NULL) &&
		 (psCBObjDestroyed->player == (UDWORD)player) &&
		 (psCBObjDestroyed->type == OBJ_DROID) )
	{
		retval = true;
		*ppsObj = psCBObjDestroyed;
	}
	else
	{
		retval = false;
		*ppsObj = NULL;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// deal with a CALL_FEATURE_DESTROYED
WZ_DECL_UNUSED static BOOL scrCBFeatureDestroyed(void)
{
	BASE_OBJECT		**ppsObj;
	BOOL			retval;

	if (!stackPopParams(1, VAL_REF|ST_FEATURE, &ppsObj))
	{
		return false;
	}

	if (psCBObjDestroyed != NULL)
	{
		retval = true;
		*ppsObj = psCBObjDestroyed;
	}
	else
	{
		retval = false;
		*ppsObj = NULL;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// the last object to be seen for a CALL_OBJ_SEEN
BASE_OBJECT		*psScrCBObjSeen;
// the object that saw psScrCBObjSeen for a CALL_OBJ_SEEN
BASE_OBJECT		*psScrCBObjViewer;

// deal with all the object seen functions
WZ_DECL_UNUSED static BOOL scrCBObjectSeen(SDWORD callback)
{
	BASE_OBJECT		**ppsObj;
	BASE_OBJECT		**ppsViewer;
	SDWORD			player;
	BOOL			retval;

	if (!stackPopParams(3, VAL_INT, &player, VAL_REF|ST_BASEOBJECT, &ppsObj, VAL_REF|ST_BASEOBJECT, &ppsViewer))
	{
		return false;
	}

	if (psScrCBObjSeen == NULL)
	{
		ASSERT( false,"scrCBObjectSeen: no object set" );
		return false;
	}

	*ppsObj = NULL;
	if (((psScrCBObjViewer != NULL) &&
		 (psScrCBObjViewer->player != player)) ||
		!psScrCBObjSeen->visible[player])
	{
		retval = false;
	}
	else if ((callback == CALL_DROID_SEEN) &&
			 (psScrCBObjSeen->type != OBJ_DROID))
	{
		retval = false;
	}
	else if ((callback == CALL_STRUCT_SEEN) &&
			 (psScrCBObjSeen->type != OBJ_STRUCTURE))
	{
		retval = false;
	}
	else if ((callback == CALL_FEATURE_SEEN) &&
			 (psScrCBObjSeen->type != OBJ_FEATURE))
	{
		retval = false;
	}
	else
	{
		retval = true;
		*ppsObj = psScrCBObjSeen;
		*ppsViewer = psScrCBObjViewer;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// deal with a CALL_OBJ_SEEN
WZ_DECL_UNUSED static BOOL scrCBObjSeen(void)
{
	return scrCBObjectSeen(CALL_OBJ_SEEN);
}

// deal with a CALL_DROID_SEEN
WZ_DECL_UNUSED static BOOL scrCBDroidSeen(void)
{
	return scrCBObjectSeen(CALL_DROID_SEEN);
}

// deal with a CALL_STRUCT_SEEN
WZ_DECL_UNUSED static BOOL scrCBStructSeen(void)
{
	return scrCBObjectSeen(CALL_STRUCT_SEEN);
}

// deal with a CALL_FEATURE_SEEN
WZ_DECL_UNUSED static BOOL scrCBFeatureSeen(void)
{
	return scrCBObjectSeen(CALL_FEATURE_SEEN);
}

WZ_DECL_UNUSED static BOOL scrCBTransporterOffMap( void )
{
	SDWORD	player;
	BOOL	retval;
	DROID	*psTransporter;

	if (!stackPopParams(1, VAL_INT, &player) )
	{
		return false;
	}

	psTransporter = transporterGetScriptCurrent();

	if ( (psTransporter != NULL) &&
		 (psTransporter->player == (UDWORD)player) )
	{
		retval = true;
	}
	else
	{
		retval = false;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

WZ_DECL_UNUSED static BOOL scrCBTransporterLanded( void )
{
	SDWORD			player;
	DROID_GROUP		*psGroup;
	DROID			*psTransporter, *psDroid, *psNext;
	BOOL			retval;

	if (!stackPopParams(2, ST_GROUP, &psGroup, VAL_INT, &player))
	{
		return false;
	}

	psTransporter = transporterGetScriptCurrent();

	if ( (psTransporter == NULL) ||
		 (psTransporter->player != (UDWORD)player) )
	{
		retval = false;
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

		retval = true;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

WZ_DECL_UNUSED static BOOL scrCBTransporterLandedB( void )
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
		return false;
	}

	psTransporter = transporterGetScriptCurrent();

	if ( (psTransporter == NULL) ||
		 (psTransporter->player != (UDWORD)player) )
	{
		retval = false;
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

		retval = true;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrCBTransporterLandedB: push landed");
		return false;
	}

	return true;
}


// tell the scripts when a cluster is no longer valid
SDWORD	scrCBEmptyClusterID;
WZ_DECL_UNUSED static BOOL scrCBClusterEmpty( void )
{
	SDWORD		*pClusterID;

	if (!stackPopParams(1, VAL_REF|VAL_INT, &pClusterID))
	{
		return false;
	}

	*pClusterID = scrCBEmptyClusterID;

	scrFunctionResult.v.bval = true;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// note when a vtol has finished returning to base - used to vanish
// vtols when they are attacking from off map
DROID *psScrCBVtolOffMap;
WZ_DECL_UNUSED static BOOL scrCBVtolOffMap(void)
{
	SDWORD	player;
	DROID	**ppsVtol;
	BOOL	retval;

	if (!stackPopParams(2, VAL_INT, &player, VAL_REF|ST_DROID, &ppsVtol))
	{
		return false;
	}

	if (psScrCBVtolOffMap == NULL)
	{
		ASSERT( false, "scrCBVtolAtBase: NULL vtol pointer" );
		return false;
	}

	retval = false;
	if (psScrCBVtolOffMap->player == player)
	{
		retval = true;
		*ppsVtol = psScrCBVtolOffMap;
	}
	psScrCBVtolOffMap = NULL;

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/*called when selectedPlayer completes some research*/
WZ_DECL_UNUSED static BOOL scrCBResCompleted(void)
{
	RESEARCH	**ppsResearch;
	STRUCTURE	**ppsResFac;
	BOOL	    retVal;
	SDWORD		resFacOwner;

	if (!stackPopParams(3, VAL_REF|ST_RESEARCH, &ppsResearch,
		VAL_REF|ST_STRUCTURE, &ppsResFac ,VAL_INT, &resFacOwner))
	{
		return false;
	}

	retVal = false;
	*ppsResearch = NULL;
	*ppsResFac = NULL;

	if(resFacOwner == -1 || resFacOwner == CBResFacilityOwner)
	{
		if (psCBLastResearch != NULL)
		{
			retVal = true;
			*ppsResearch = psCBLastResearch;
			*ppsResFac = psCBLastResStructure;
		}
		else
		{
			ASSERT( false, "scrCBResCompleted: no research has been set" );
		}
	}

	scrFunctionResult.v.bval = retVal;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


/* when a humna player leaves a game*/
WZ_DECL_UNUSED static BOOL scrCBPlayerLeft(void)
{
	SDWORD	*player;
	if (!stackPopParams(1, VAL_REF | VAL_INT, &player) )
	{
		return false;
	}

	*player = CBallFrom;

	scrFunctionResult.v.bval = true;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// alliance has been offered.
WZ_DECL_UNUSED static BOOL scrCBAllianceOffer(void)
{
	SDWORD	*from,*to;

	if (!stackPopParams(2, VAL_REF | VAL_INT, &from, VAL_REF | VAL_INT,&to) )
	{
		return false;
	}

	*from = CBallFrom;
	*to = CBallTo;

	scrFunctionResult.v.bval = true;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------------------------
										/* New callbacks */


//console callback
//---------------------------
WZ_DECL_UNUSED static BOOL scrCallConsole(void)
{
	SDWORD	*player;
	char	**ConsoleText = NULL;

	if (!stackPopParams(2, VAL_REF | VAL_INT, &player, VAL_REF | VAL_STRING, &ConsoleText) )
	{
		debug(LOG_ERROR, "scrCallConsole(): stack failed");
		return false;
	}

	if(*ConsoleText == NULL)
	{
		debug(LOG_ERROR, "scrCallConsole(): passed string was not initialized");
		return false;
	}

	strcpy(*ConsoleText,ConsoleMsg);

	*player = ConsolePlayer;

	scrFunctionResult.v.bval = true;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrCallConsole(): stackPushResult failed");
		return false;
	}

	return true;
}

//multiplayer beacon
//---------------------------
WZ_DECL_UNUSED static BOOL scrCallBeacon(void)
{
	SDWORD	*playerFrom, playerTo;
	char	**BeaconText = NULL;
	SDWORD	*locX,*locY;

	if (!stackPopParams(5, VAL_INT, &playerTo, VAL_REF | VAL_INT, &playerFrom,
		VAL_REF | VAL_INT, &locX, VAL_REF | VAL_INT, &locY,
		VAL_REF | VAL_STRING, &BeaconText))
	{
		debug(LOG_ERROR, "scrCallBeacon() - failed to pop parameters.");
		return false;
	}

	debug(LOG_SCRIPT, "scrCallBeacon: to: %d (%d), text: %s ",
		playerTo, MultiMsgPlayerTo, *BeaconText);

	if(*BeaconText == NULL)
	{
		debug(LOG_ERROR, "scrCallBeacon(): passed string was not initialized");
		return false;
	}

	if(MultiMsgPlayerTo >= 0 && MultiMsgPlayerFrom >= 0 && MultiMsgPlayerTo < MAX_PLAYERS && MultiMsgPlayerFrom < MAX_PLAYERS)
	{

		if(MultiMsgPlayerTo == playerTo)
		{
			strcpy(*BeaconText,MultiplayMsg);

			*playerFrom = MultiMsgPlayerFrom;
			*locX = beaconX;
			*locY = beaconY;

			scrFunctionResult.v.bval = true;
			if (!stackPushResult(VAL_BOOL, &scrFunctionResult))	//triggered
			{
				debug(LOG_ERROR, "scrCallBeacon - failed to push");
				return false;
			}

			return true;
		}
	}
	else
	{
		debug(LOG_ERROR, "scrCallBeacon() - player indexes failed: %d - %d", MultiMsgPlayerFrom, MultiMsgPlayerTo);
		scrFunctionResult.v.bval = false;
		if (!stackPushResult(VAL_BOOL, &scrFunctionResult))	//not triggered
		{
			return false;
		}

		return true;
	}

	//return "not triggered"
	scrFunctionResult.v.bval = false;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

//multiplayer message callback
//----------------------------
WZ_DECL_UNUSED static BOOL scrCallMultiMsg(void)
{
	SDWORD	*player, playerTo;
	char	**ConsoleText = NULL;

	if (!stackPopParams(3, VAL_INT, &playerTo, VAL_REF | VAL_INT, &player, VAL_REF | VAL_STRING, &ConsoleText) )
	{
		debug(LOG_ERROR, "scrCallMultiMsg() failed to pop parameters.");
		return false;
	}

	if(*ConsoleText == NULL)
	{
		debug(LOG_ERROR, "scrCallMultiMsg(): passed string was not initialized");
		return false;
	}

	if(MultiMsgPlayerTo >= 0 && MultiMsgPlayerFrom >= 0 && MultiMsgPlayerTo < MAX_PLAYERS && MultiMsgPlayerFrom < MAX_PLAYERS)
	{
		if(MultiMsgPlayerTo == playerTo)
		{
			strcpy(*ConsoleText,MultiplayMsg);

			*player = MultiMsgPlayerFrom;

			scrFunctionResult.v.bval = true;
			if (!stackPushResult(VAL_BOOL, &scrFunctionResult))	//triggered
			{
				debug(LOG_ERROR, "scrCallMultiMsg(): stackPushResult failed");
				return false;
			}

			return true;
		}
	}
	else
	{
		debug(LOG_ERROR, "scrCallMultiMsg() - player indexes failed: %d - %d", MultiMsgPlayerFrom, MultiMsgPlayerTo);
		scrFunctionResult.v.bval = false;
		if (!stackPushResult(VAL_BOOL, &scrFunctionResult))	//not triggered
		{
			return false;
		}

		return true;
	}

	//return "not triggered"
	scrFunctionResult.v.bval = false;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrCallMultiMsg: stackPushResult failed");
		return false;
	}

	return true;
}

STRUCTURE	*psScrCBNewStruct = NULL;	//for scrCBStructBuilt callback
DROID		*psScrCBNewStructTruck = NULL;
//structure built callback
//------------------------------
WZ_DECL_UNUSED static BOOL scrCBStructBuilt(void)
{
	SDWORD		player;
	STRUCTURE	**ppsStructure;
	BOOL		triggered = false;
	DROID		**ppsDroid;

	if (!stackPopParams(3, VAL_INT, &player, VAL_REF|ST_DROID, &ppsDroid, VAL_REF|ST_STRUCTURE, &ppsStructure) )
	{
		debug(LOG_ERROR, "scrCBStructBuilt() failed to pop parameters.");
		return false;
	}

	if (psScrCBNewStruct == NULL)
	{
		debug(LOG_ERROR, "scrCBStructBuilt: no structure has been set");
		ASSERT( false, "scrCBStructBuilt: no structure has been set" );
		triggered = false;
		*ppsStructure  = NULL;
		*ppsDroid = NULL;
	}
	else if(psScrCBNewStructTruck == NULL)
	{
		debug(LOG_ERROR, "scrCBStructBuilt: no builder has been set");
		ASSERT( false, "scrCBStructBuilt: no builder has been set" );
		triggered = false;
		*ppsStructure  = NULL;
		*ppsDroid = NULL;
	}
	else if (psScrCBNewStruct->player == (UDWORD)player)
	{
		triggered = true;
		*ppsStructure  = psScrCBNewStruct;		//pass to script
		*ppsDroid = psScrCBNewStructTruck;
	}

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrCBStructBuilt: push failed");
		return false;
	}

	return true;
}

/* Droid received stop order */
WZ_DECL_UNUSED static BOOL scrCBDorderStop(void)
{
	SDWORD		player;
	DROID		**ppsDroid;
	BOOL	triggered = false;

	if (!stackPopParams(2, VAL_INT, &player, VAL_REF|ST_DROID, &ppsDroid))
	{
		debug(LOG_ERROR, "scrCBDorderStop: failed to pop");
		return false;
	}

	if (psScrCBOrderDroid == NULL)	//if droid that received stop order was destroyed
	{
		ASSERT( false, "scrCBDorderStop: psScrCBOrderDroid is NULL" );
		triggered = false;
		*ppsDroid = NULL;
	}
	else if (psScrCBOrderDroid->player == (UDWORD)player)
	{
		triggered = true;
		*ppsDroid = psScrCBOrderDroid;
	}

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/* Droid reached destination point and stopped on its own */
WZ_DECL_UNUSED static BOOL scrCBDorderReachedLocation(void)
{
	SDWORD		player;
	SDWORD		*Order = NULL;
	DROID		**ppsDroid;
	BOOL	triggered = false;

	if (!stackPopParams(3, VAL_INT, &player, VAL_REF|ST_DROID, &ppsDroid
		,VAL_REF | VAL_INT, &Order))
	{
		debug(LOG_ERROR, "scrCBDorderReachedLocation: failed to pop");
		return false;
	}

	if (psScrCBOrderDroid == NULL)	//if droid was destroyed
	{
		ASSERT( false, "scrCBDorderReachedLocation: psScrCBOrderDroid is NULL" );
		triggered = false;
		*ppsDroid = NULL;
	}
	else if (psScrCBOrderDroid->player == (UDWORD)player)
	{
		triggered = true;
		*ppsDroid = psScrCBOrderDroid;
		*Order = psScrCBOrder;
	}

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

/* Process key-combo */
WZ_DECL_UNUSED static BOOL scrCBProcessKeyPress(void)
{
	SDWORD		*key = NULL, *metaKey = NULL;

	if (!stackPopParams(2,VAL_REF | VAL_INT, &key, VAL_REF | VAL_INT, &metaKey))
	{
		debug(LOG_ERROR, "scrCBProcessKeyPress: failed to pop");
		return false;
	}

	*key = cbPressedKey;
	*metaKey = cbPressedMetaKey;

	scrFunctionResult.v.bval = true;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))		//triggered
	{
		return false;
	}

	return true;
}

// FIXME
extern lua_State *lua_states[100];

void eventFireCallbackTrigger(TRIGGER_TYPE trigger_)
{
	SCR_CALLBACK_TYPES trigger = (SCR_CALLBACK_TYPES)trigger_;  // Why does it take the wrong enum type?

	int i, args;
	lua_State *L;
	// reverse loop because some scripts expect scripts that are loaded later
	// to be the first to get the callback
	for(i=99;i>=0;i--)
	{
		L = lua_states[i];
		if (!L)
		{
			continue;
		}
		lua_getglobal(L, "doCallbacksFor");
		lua_pushinteger(L, trigger);
		args = 1;
		switch (trigger)
		{
			case CALL_NEWDROID:
				luaWZObj_pushdroid(L, psScrCBNewDroid);
				luaWZObj_pushstructure(L, psScrCBNewDroidFact);
				args += 2;
				break;
			case CALL_ATTACKED:
			case CALL_STRUCT_ATTACKED:
			case CALL_DROID_ATTACKED:
				luaWZObj_pushobject(L, psScrCBTarget);
				if (g_pProjLastAttacker)
				{
					luaWZObj_pushobject(L, g_pProjLastAttacker);
				}
				else
				{
					// we don't know who shot it (not in LOS)
					lua_pushnil(L);
				}
				args += 2;
				break;
			case CALL_STRUCTBUILT:
				luaWZObj_pushdroid(L, psScrCBNewStructTruck);
				luaWZObj_pushstructure(L, psScrCBNewStruct);
				args += 2;
				break;
			case CALL_KEY_PRESSED:
				lua_pushinteger(L, cbPressedKey);
				args += 1;
				break;
			case CALL_CONSOLE:
				lua_pushinteger(L, ConsolePlayer);
				lua_pushstring(L, ConsoleMsg);
				args += 2;
				break;
			case CALL_AI_MSG:
				lua_pushinteger(L, MultiMsgPlayerTo);
				lua_pushinteger(L, MultiMsgPlayerFrom);
				lua_pushstring(L, MultiplayMsg);
				args += 3;
				break;
			case CALL_DROID_REACH_LOCATION:
				luaWZObj_pushdroid(L, psScrCBOrderDroid);
				lua_pushinteger(L, psScrCBOrder);
				args += 2;
				break;
			case CALL_RESEARCHCOMPLETED:
				lua_pushstring(L, psCBLastResearch->pName);
				luaWZObj_pushstructure(L, psCBLastResStructure);
				lua_pushinteger(L, CBResFacilityOwner);
				args += 3;
				break;
			case CALL_OBJ_SEEN:
				luaWZObj_pushobject(L, psScrCBObjSeen);
				luaWZObj_pushobject(L, psScrCBObjViewer);
				args += 2;
				break;
			case CALL_OBJ_DESTROYED:
			case CALL_DROID_DESTROYED:
			case CALL_STRUCT_DESTROYED:
			case CALL_FEATURE_DESTROYED:
				luaWZObj_pushobject(L, psCBObjDestroyed);
				if (g_pProjLastAttacker)
				{
					luaWZObj_pushobject(L, g_pProjLastAttacker);
				}
				else
				{
					// we don't know who shot it (not in LOS)
					lua_pushnil(L);
				}
				args += 2;
				break;
			case CALL_CLUSTER_EMPTY:
				lua_pushinteger(L, scrCBEmptyClusterID);
				args += 1;
				break;
			case CALL_DROID_SEEN:
			case CALL_STRUCT_SEEN:
			case CALL_FEATURE_SEEN:
				luaWZObj_pushobject(L, psScrCBObjSeen);
				luaWZObj_pushobject(L, psScrCBObjViewer);
				args += 2;
				break;
			case CALL_BUILDGRID:
			case CALL_BUILDLIST:
			case CALL_DELIVPOINTMOVED:
			case CALL_DESIGN_BODY:
			case CALL_DESIGN_PROPULSION:
			case CALL_DESIGN_QUIT:
			case CALL_DESIGN_WEAPON:
			case CALL_DROIDBUILT:
			case CALL_DROIDDESIGNED:
			case CALL_ELECTRONIC_TAKEOVER:
			case CALL_FACTORY_BUILT:
			case CALL_GAMEINIT:
			case CALL_LAUNCH_TRANSPORTER:
			case CALL_MANULIST:
			case CALL_MANURUN:
			case CALL_MISSION_END:
			case CALL_MISSION_START:
			case CALL_MISSION_TIME:
			case CALL_NO_REINFORCEMENTS_LEFT:
			case CALL_POWERGEN_BUILT:
			case CALL_RESEARCH_BUILT:
			case CALL_RESEARCHLIST:
			case CALL_RESEX_BUILT:
			case CALL_START_NEXT_LEVEL:
			case CALL_TRANSPORTER_REINFORCE:
			case CALL_VIDEO_QUIT:
			case CALL_EVERY_FRAME:
				// no args
				break;
			default:
				debug(LOG_SCRIPT, "unknown callback %i", trigger);
				break;
		}
		luaWZ_pcall_backtrace(L, args, 0);
	}
}
