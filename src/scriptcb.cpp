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

// vtol target
DROID		*psScrVtolRetarget = NULL;

// alliance details
UDWORD	CBallFrom, CBallTo;

// player number that left the game
UDWORD	CBPlayerLeft;

//console callback stuff
//---------------------------
SDWORD ConsolePlayer = -2;
SDWORD MultiMsgPlayerTo = -2;
SDWORD beaconX = -1, beaconY = -1;
SDWORD MultiMsgPlayerFrom = -2;
char ConsoleMsg[MAXSTRLEN] = "ERROR!!!\0";	//Last console message
char MultiplayMsg[MAXSTRLEN];	//Last multiplayer message

bool scrCBDroidTaken(void)
{
	DROID		**ppsDroid;
	bool	triggered = false;

	if (!stackPopParams(1, VAL_REF | ST_DROID, &ppsDroid))
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

// Deal with a CALL_NEWDROID
bool scrCBNewDroid(void)
{
	SDWORD		player;
	DROID		**ppsDroid;
	STRUCTURE	**ppsStructure;
	bool	triggered = false;

	if (!stackPopParams(3, VAL_INT, &player, VAL_REF | ST_DROID, &ppsDroid, VAL_REF | ST_STRUCTURE, &ppsStructure))
	{
		return false;
	}

	if (psScrCBNewDroid == NULL)
	{
		// eh? got called without setting the new droid
		ASSERT(false, "scrCBNewUnit: no unit has been set");
		triggered = false;
		*ppsDroid = NULL;
		*ppsStructure  = NULL;
	}
	else if (psScrCBNewDroid->player == (UDWORD)player)
	{
		triggered = true;
		*ppsDroid = psScrCBNewDroid;
		*ppsStructure  = psScrCBNewDroidFact;
	}

	scrFunctionResult.v.bval = triggered;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// Deal with a CALL_STRUCT_ATTACKED
bool scrCBStructAttacked(void)
{
	SDWORD			player;
	STRUCTURE		**ppsTarget;
	BASE_OBJECT		**ppsAttacker;//, **ppsTarget;
	bool			triggered = false;

	if (!stackPopParams(3, VAL_INT, &player,
	                    VAL_REF | ST_STRUCTURE, &ppsTarget,
	                    VAL_REF | ST_BASEOBJECT, &ppsAttacker))
	{
		return false;
	}

	if (psLastStructHit == NULL)
	{
		ASSERT(false, "scrCBStructAttacked: no target has been set");
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

bool scrCBVTOLRetarget(void)
{
	SDWORD			player;
	DROID			**ppsDroid;

	if (!stackPopParams(2, VAL_INT, &player, VAL_REF | ST_DROID, &ppsDroid))
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
bool scrCBDroidAttacked(void)
{
	SDWORD			player;
	DROID			**ppsTarget;
	BASE_OBJECT		**ppsAttacker;//, **ppsTarget;
	bool			triggered = false;

	if (!stackPopParams(3, VAL_INT, &player,
	                    VAL_REF | ST_DROID, &ppsTarget,
	                    VAL_REF | ST_BASEOBJECT, &ppsAttacker))
	{
		return false;
	}

	if (psLastDroidHit == NULL)
	{
		ASSERT(false, "scrCBUnitAttacked: no target has been set");
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
bool scrCBAttacked(void)
{
	SDWORD			player;
	BASE_OBJECT		**ppsTarget;
	BASE_OBJECT		**ppsAttacker;//, **ppsTarget;
	bool			triggered = false;

	if (!stackPopParams(3, VAL_INT, &player,
	                    VAL_REF | ST_BASEOBJECT, &ppsTarget,
	                    VAL_REF | ST_BASEOBJECT, &ppsAttacker))
	{
		return false;
	}

	if (psScrCBTarget == NULL)
	{
		ASSERT(false, "scrCBAttacked: no target has been set");
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
bool scrCBButtonPressed(void)
{
	UDWORD	button;
	bool	triggered = false;

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
bool scrCBDroidSelected(void)
{
	DROID	**ppsDroid;

	if (!stackPopParams(1, VAL_REF | ST_DROID, &ppsDroid))
	{
		return false;
	}

	ASSERT(psCBSelectedDroid != NULL,
	       "scrSCUnitSelected: invalid unit pointer");

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
bool scrCBObjDestroyed(void)
{
	SDWORD			player;
	BASE_OBJECT		**ppsObj;
	bool			retval;

	if (!stackPopParams(2, VAL_INT, &player, VAL_REF | ST_BASEOBJECT, &ppsObj))
	{
		return false;
	}

	if ((psCBObjDestroyed != NULL) &&
	    (psCBObjDestroyed->player == (UDWORD)player) &&
	    (psCBObjDestroyed->type != OBJ_FEATURE))
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
bool scrCBStructDestroyed(void)
{
	SDWORD			player;
	BASE_OBJECT		**ppsObj;
	bool			retval;

	if (!stackPopParams(2, VAL_INT, &player, VAL_REF | ST_STRUCTURE, &ppsObj))
	{
		return false;
	}

	if ((psCBObjDestroyed != NULL) &&
	    (psCBObjDestroyed->player == (UDWORD)player) &&
	    (psCBObjDestroyed->type == OBJ_STRUCTURE))
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
bool scrCBDroidDestroyed(void)
{
	SDWORD			player;
	BASE_OBJECT		**ppsObj;
	bool			retval;

	if (!stackPopParams(2, VAL_INT, &player, VAL_REF | ST_DROID, &ppsObj))
	{
		return false;
	}

	if ((psCBObjDestroyed != NULL) &&
	    (psCBObjDestroyed->player == (UDWORD)player) &&
	    (psCBObjDestroyed->type == OBJ_DROID))
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
bool scrCBFeatureDestroyed(void)
{
	BASE_OBJECT		**ppsObj;
	bool			retval;

	if (!stackPopParams(1, VAL_REF | ST_FEATURE, &ppsObj))
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
static bool scrCBObjectSeen(SDWORD callback)
{
	BASE_OBJECT		**ppsObj;
	BASE_OBJECT		**ppsViewer;
	SDWORD			player;
	bool			retval;

	if (!stackPopParams(3, VAL_INT, &player, VAL_REF | ST_BASEOBJECT, &ppsObj, VAL_REF | ST_BASEOBJECT, &ppsViewer))
	{
		return false;
	}

	if (psScrCBObjSeen == NULL)
	{
		ASSERT(false, "scrCBObjectSeen: no object set");
		return false;
	}

	*ppsObj = NULL;
	if (psScrCBObjViewer != NULL &&  psScrCBObjViewer->player != player)
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
bool scrCBObjSeen(void)
{
	return scrCBObjectSeen(CALL_OBJ_SEEN);
}

// deal with a CALL_DROID_SEEN
bool scrCBDroidSeen(void)
{
	return scrCBObjectSeen(CALL_DROID_SEEN);
}

// deal with a CALL_STRUCT_SEEN
bool scrCBStructSeen(void)
{
	return scrCBObjectSeen(CALL_STRUCT_SEEN);
}

// deal with a CALL_FEATURE_SEEN
bool scrCBFeatureSeen(void)
{
	return scrCBObjectSeen(CALL_FEATURE_SEEN);
}

bool scrCBTransporterOffMap(void)
{
	SDWORD	player;
	bool	retval;
	DROID	*psTransporter;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	psTransporter = transporterGetScriptCurrent();

	if ((psTransporter != NULL) &&
	    (psTransporter->player == (UDWORD)player))
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

bool scrCBTransporterLanded(void)
{
	SDWORD			player;
	DROID_GROUP		*psGroup;
	DROID			*psTransporter, *psDroid, *psNext;
	bool			retval;

	if (!stackPopParams(2, ST_GROUP, &psGroup, VAL_INT, &player))
	{
		return false;
	}

	psTransporter = transporterGetScriptCurrent();

	if ((psTransporter == NULL) ||
	    (psTransporter->player != (UDWORD)player))
	{
		retval = false;
	}
	else
	{
		/* if not selectedPlayer unload droids */
		if ((UDWORD)player != selectedPlayer)
		{
			/* transfer droids from transporter group to current group */
			for (psDroid = psTransporter->psGroup->psList; psDroid; psDroid = psNext)
			{
				psNext = psDroid->psGrpNext;
				if (psDroid != psTransporter)
				{
					psGroup->add(psDroid);
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

bool scrCBTransporterLandedB(void)
{
	SDWORD			player;
	DROID_GROUP		*psGroup;
	DROID			*psTransporter, *psDroid, *psNext;
	bool			retval;
	DROID			**ppsTransp;

	if (!stackPopParams(3, ST_GROUP, &psGroup, VAL_INT, &player,
	                    VAL_REF | ST_DROID, &ppsTransp))
	{
		debug(LOG_ERROR, "scrCBTransporterLandedB(): stack failed");
		return false;
	}

	psTransporter = transporterGetScriptCurrent();

	if (psTransporter == NULL || psTransporter->player != (UDWORD)player)
	{
		retval = false;
	}
	else
	{
		*ppsTransp = psTransporter;		//return landed transporter

		/* transfer droids from transporter group to current group */
		for (psDroid = psTransporter->psGroup->psList; psDroid; psDroid = psNext)
		{
			psNext = psDroid->psGrpNext;
			if (psDroid != psTransporter)
			{
				psGroup->add(psDroid);
			}
		}

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
bool scrCBClusterEmpty(void)
{
	SDWORD		*pClusterID;

	if (!stackPopParams(1, VAL_REF | VAL_INT, &pClusterID))
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
bool scrCBVtolOffMap(void)
{
	SDWORD	player;
	DROID	**ppsVtol;
	bool	retval;

	if (!stackPopParams(2, VAL_INT, &player, VAL_REF | ST_DROID, &ppsVtol))
	{
		return false;
	}

	if (psScrCBVtolOffMap == NULL)
	{
		ASSERT(false, "scrCBVtolAtBase: NULL vtol pointer");
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
bool scrCBResCompleted(void)
{
	RESEARCH	**ppsResearch;
	STRUCTURE	**ppsResFac;
	bool	    retVal;
	SDWORD		resFacOwner;

	if (!stackPopParams(3, VAL_REF | ST_RESEARCH, &ppsResearch,
	                    VAL_REF | ST_STRUCTURE, &ppsResFac , VAL_INT, &resFacOwner))
	{
		return false;
	}

	retVal = false;
	*ppsResearch = NULL;
	*ppsResFac = NULL;

	if (resFacOwner == -1 || resFacOwner == CBResFacilityOwner)
	{
		if (psCBLastResearch != NULL)
		{
			retVal = true;
			*ppsResearch = psCBLastResearch;
			*ppsResFac = psCBLastResStructure;
		}
		else
		{
			ASSERT(false, "scrCBResCompleted: no research has been set");
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
bool scrCBPlayerLeft(void)
{
	SDWORD	*player;
	if (!stackPopParams(1, VAL_REF | VAL_INT, &player))
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
bool scrCBAllianceOffer(void)
{
	SDWORD	*from, *to;

	if (!stackPopParams(2, VAL_REF | VAL_INT, &from, VAL_REF | VAL_INT, &to))
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
bool scrCallConsole(void)
{
	SDWORD	*player;
	char	**ConsoleText = NULL;

	if (!stackPopParams(2, VAL_REF | VAL_INT, &player, VAL_REF | VAL_STRING, &ConsoleText))
	{
		debug(LOG_ERROR, "scrCallConsole(): stack failed");
		return false;
	}

	if (*ConsoleText == NULL)
	{
		debug(LOG_ERROR, "scrCallConsole(): passed string was not initialized");
		return false;
	}

	strcpy(*ConsoleText, ConsoleMsg);

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
bool scrCallBeacon(void)
{
	SDWORD	*playerFrom, playerTo;
	char	**BeaconText = NULL;
	SDWORD	*locX, *locY;

	if (!stackPopParams(5, VAL_INT, &playerTo, VAL_REF | VAL_INT, &playerFrom,
	                    VAL_REF | VAL_INT, &locX, VAL_REF | VAL_INT, &locY,
	                    VAL_REF | VAL_STRING, &BeaconText))
	{
		debug(LOG_ERROR, "scrCallBeacon() - failed to pop parameters.");
		return false;
	}

	debug(LOG_SCRIPT, "scrCallBeacon: to: %d (%d), text: %s ",
	      playerTo, MultiMsgPlayerTo, *BeaconText);

	if (*BeaconText == NULL)
	{
		debug(LOG_ERROR, "scrCallBeacon(): passed string was not initialized");
		return false;
	}

	if (MultiMsgPlayerTo >= 0 && MultiMsgPlayerFrom >= 0 && MultiMsgPlayerTo < MAX_PLAYERS && MultiMsgPlayerFrom < MAX_PLAYERS)
	{

		if (MultiMsgPlayerTo == playerTo)
		{
			strcpy(*BeaconText, MultiplayMsg);

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
bool scrCallMultiMsg(void)
{
	SDWORD	*player, playerTo;
	char	**ConsoleText = NULL;

	if (!stackPopParams(3, VAL_INT, &playerTo, VAL_REF | VAL_INT, &player, VAL_REF | VAL_STRING, &ConsoleText))
	{
		debug(LOG_ERROR, "scrCallMultiMsg() failed to pop parameters.");
		return false;
	}

	if (*ConsoleText == NULL)
	{
		debug(LOG_ERROR, "scrCallMultiMsg(): passed string was not initialized");
		return false;
	}

	if (MultiMsgPlayerTo >= 0 && MultiMsgPlayerFrom >= 0 && MultiMsgPlayerTo < MAX_PLAYERS && MultiMsgPlayerFrom < MAX_PLAYERS)
	{
		if (MultiMsgPlayerTo == playerTo)
		{
			strcpy(*ConsoleText, MultiplayMsg);

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
bool scrCBStructBuilt(void)
{
	SDWORD		player;
	STRUCTURE	**ppsStructure;
	bool		triggered = false;
	DROID		**ppsDroid;

	if (!stackPopParams(3, VAL_INT, &player, VAL_REF | ST_DROID, &ppsDroid, VAL_REF | ST_STRUCTURE, &ppsStructure))
	{
		debug(LOG_ERROR, "scrCBStructBuilt() failed to pop parameters.");
		return false;
	}

	if (psScrCBNewStruct == NULL)
	{
		debug(LOG_ERROR, "scrCBStructBuilt: no structure has been set");
		ASSERT(false, "scrCBStructBuilt: no structure has been set");
		triggered = false;
		*ppsStructure  = NULL;
		*ppsDroid = NULL;
	}
	else if (psScrCBNewStructTruck == NULL)
	{
		debug(LOG_ERROR, "scrCBStructBuilt: no builder has been set");
		ASSERT(false, "scrCBStructBuilt: no builder has been set");
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
bool scrCBDorderStop(void)
{
	SDWORD		player;
	DROID		**ppsDroid;
	bool	triggered = false;

	if (!stackPopParams(2, VAL_INT, &player, VAL_REF | ST_DROID, &ppsDroid))
	{
		debug(LOG_ERROR, "scrCBDorderStop: failed to pop");
		return false;
	}

	if (psScrCBOrderDroid == NULL)	//if droid that received stop order was destroyed
	{
		ASSERT(false, "scrCBDorderStop: psScrCBOrderDroid is NULL");
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
bool scrCBDorderReachedLocation(void)
{
	SDWORD		player;
	SDWORD		*Order = NULL;
	DROID		**ppsDroid;
	bool	triggered = false;

	if (!stackPopParams(3, VAL_INT, &player, VAL_REF | ST_DROID, &ppsDroid, VAL_REF | VAL_INT, &Order))
	{
		debug(LOG_ERROR, "scrCBDorderReachedLocation: failed to pop");
		return false;
	}

	if (psScrCBOrderDroid == NULL)	//if droid was destroyed
	{
		ASSERT(false, "psScrCBOrderDroid is NULL");
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
bool scrCBProcessKeyPress(void)
{
	SDWORD		*key = NULL, *metaKey = NULL;

	if (!stackPopParams(2, VAL_REF | VAL_INT, &key, VAL_REF | VAL_INT, &metaKey))
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
