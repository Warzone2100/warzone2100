/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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

struct BASE_OBJECT;
struct DROID;
struct STRUCTURE;

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
extern DROID		*psScrVtolRetarget;

//Script key event callback
extern SDWORD		cbPressedMetaKey;
extern SDWORD		cbPressedKey;

// deal with unit takover(2)
bool scrCBDroidTaken();

// Deal with a CALL_NEWDROID
bool scrCBNewDroid();

// the attacker and target for a CALL_ATTACKED
extern BASE_OBJECT	*psScrCBAttacker, *psScrCBTarget;

// Deal with a CALL_STRUCT_ATTACKED
bool scrCBStructAttacked();

// Deal with a CALL_DROID_ATTACKED
bool scrCBDroidAttacked();

// Deal with a CALL_ATTACKED
bool scrCBAttacked();

// deal with CALL_BUTTON_PRESSED
bool scrCBButtonPressed();

// the Droid that was selected for a CALL_DROID_SELECTED
extern DROID	*psCBSelectedDroid;

// deal with CALL_DROID_SELECTED
bool scrCBDroidSelected();

// the object that was last killed for a CALL_OBJ_DESTROYED
extern BASE_OBJECT *psCBObjDestroyed;

// deal with a CALL_OBJ_DESTROYED
bool scrCBObjDestroyed();

// deal with a CALL_STRUCT_DESTROYED
bool scrCBStructDestroyed();

// deal with a CALL_DROID_DESTROYED
bool scrCBDroidDestroyed();

// deal with a CALL_FEATURE_DESTROYED
bool scrCBFeatureDestroyed();

// the last object to be seen for a CALL_OBJ_SEEN
extern BASE_OBJECT		*psScrCBObjSeen;

// the object that saw psScrCBObjSeen for a CALL_OBJ_SEEN
extern BASE_OBJECT		*psScrCBObjViewer;

// deal with a CALL_OBJ_SEEN
bool scrCBObjSeen();

// deal with a CALL_DROID_SEEN
bool scrCBDroidSeen();

// deal with a CALL_STRUCT_SEEN
bool scrCBStructSeen();

// deal with a CALL_FEATURE_SEEN
bool scrCBFeatureSeen();

// deal with a CALL_TRANSPORTER_OFFMAP
bool scrCBTransporterOffMap();

// deal with a CALL_TRANSPORTER_LANDED
bool scrCBTransporterLanded();

// tell the scripts when a cluster is no longer valid
extern SDWORD	scrCBEmptyClusterID;
bool scrCBClusterEmpty();

// note when a vtol has finished returning to base - used to vanish
// vtols when they are attacking from off map
extern DROID *psScrCBVtolOffMap;
bool scrCBVtolOffMap();

/*called when selectedPlayer completes some research*/
bool scrCBResCompleted();

/* when a player leaves the game*/
bool scrCBPlayerLeft();

/* when a VTOL runs out of things to do while mid-air */
bool scrCBVTOLRetarget();

// alliance offered.
bool scrCBAllianceOffer();
extern UDWORD	CBallFrom, CBallTo;

// player number that left the game
extern UDWORD	CBPlayerLeft;

//Console callback
bool scrCallConsole();
bool scrCBStructBuilt();
bool scrCallMultiMsg();
bool scrCallBeacon();
bool scrCBTransporterLandedB();

bool scrCBDorderStop();
bool scrCBDorderReachedLocation();
bool scrCBProcessKeyPress();

#endif // __INCLUDED_SRC_SCRIPTCB_H__
