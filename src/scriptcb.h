/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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
/** @file
 *  functions to deal with parameterised script callback triggers.
 */

#ifndef __INCLUDED_SRC_SCRIPTCB_H__
#define __INCLUDED_SRC_SCRIPTCB_H__

#include "lib/script/interpreter.h"

// Lua
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "lib/lua/warzone.h"

//console callback stuff
//---------------------------
#ifndef MAXSTRLEN
#define MAXSTRLEN 255
#endif

extern SDWORD ConsolePlayer;
extern SDWORD MultiMsgPlayerTo;
extern SDWORD MultiMsgPlayerFrom;
extern SDWORD beaconX;
extern SDWORD beaconY;
extern char ConsoleMsg[MAXSTRLEN];	//Last console message
extern char MultiplayMsg[MAXSTRLEN];	//Last multiplayer message
extern STRUCTURE	*psScrCBNewStruct;		//for scrCBStructBuilt callback
extern DROID		*psScrCBNewStructTruck;	//for scrCBStructBuilt callback


// The pointer to the droid that was just built for a CALL_NEWDROID
extern DROID		*psScrCBDroidTaken;
extern DROID		*psScrCBNewDroid;
extern STRUCTURE	*psScrCBNewDroidFact;
extern DROID		*psScrCBOrderDroid;
extern SDWORD		psScrCBOrder;

//Script key event callback
extern SDWORD		cbPressedMetaKey;
extern SDWORD		cbPressedKey;

// the attacker and target for a CALL_ATTACKED
extern BASE_OBJECT	*psScrCBAttacker, *psScrCBTarget;

// the Droid that was selected for a CALL_DROID_SELECTED
extern DROID	*psCBSelectedDroid;

// the object that was last killed for a CALL_OBJ_DESTROYED
extern BASE_OBJECT *psCBObjDestroyed;

// the last object to be seen for a CALL_OBJ_SEEN
extern BASE_OBJECT		*psScrCBObjSeen;

// the object that saw psScrCBObjSeen for a CALL_OBJ_SEEN
extern BASE_OBJECT		*psScrCBObjViewer;

// tell the scripts when a cluster is no longer valid
extern SDWORD	scrCBEmptyClusterID;

// note when a vtol has finished returning to base - used to vanish
// vtols when they are attacking from off map
extern DROID *psScrCBVtolOffMap;

extern UDWORD	CBallFrom,CBallTo;

extern void eventFireCallbackTrigger(TRIGGER_TYPE trigger);

// player number that left the game
extern UDWORD	CBPlayerLeft;

#endif // __INCLUDED_SRC_SCRIPTCB_H__
