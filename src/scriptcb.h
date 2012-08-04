/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
extern bool scrCBDroidTaken(void);

// Deal with a CALL_NEWDROID
extern bool scrCBNewDroid(void);

// the attacker and target for a CALL_ATTACKED
extern BASE_OBJECT	*psScrCBAttacker, *psScrCBTarget;

// Deal with a CALL_STRUCT_ATTACKED
extern bool scrCBStructAttacked(void);

// Deal with a CALL_DROID_ATTACKED
extern bool scrCBDroidAttacked(void);

// Deal with a CALL_ATTACKED
extern bool scrCBAttacked(void);

// deal with CALL_BUTTON_PRESSED
extern bool scrCBButtonPressed(void);

// the Droid that was selected for a CALL_DROID_SELECTED
extern DROID	*psCBSelectedDroid;

// deal with CALL_DROID_SELECTED
extern bool scrCBDroidSelected(void);

// the object that was last killed for a CALL_OBJ_DESTROYED
extern BASE_OBJECT *psCBObjDestroyed;

// deal with a CALL_OBJ_DESTROYED
extern bool scrCBObjDestroyed(void);

// deal with a CALL_STRUCT_DESTROYED
extern bool scrCBStructDestroyed(void);

// deal with a CALL_DROID_DESTROYED
extern bool scrCBDroidDestroyed(void);

// deal with a CALL_FEATURE_DESTROYED
extern bool scrCBFeatureDestroyed(void);

// the last object to be seen for a CALL_OBJ_SEEN
extern BASE_OBJECT		*psScrCBObjSeen;

// the object that saw psScrCBObjSeen for a CALL_OBJ_SEEN
extern BASE_OBJECT		*psScrCBObjViewer;

// deal with a CALL_OBJ_SEEN
extern bool scrCBObjSeen(void);

// deal with a CALL_DROID_SEEN
extern bool scrCBDroidSeen(void);

// deal with a CALL_STRUCT_SEEN
extern bool scrCBStructSeen(void);

// deal with a CALL_FEATURE_SEEN
extern bool scrCBFeatureSeen(void);

// deal with a CALL_TRANSPORTER_OFFMAP
extern bool scrCBTransporterOffMap(void);

// deal with a CALL_TRANSPORTER_LANDED
extern bool scrCBTransporterLanded(void);

// tell the scripts when a cluster is no longer valid
extern SDWORD	scrCBEmptyClusterID;
extern bool scrCBClusterEmpty( void );

// note when a vtol has finished returning to base - used to vanish
// vtols when they are attacking from off map
extern DROID *psScrCBVtolOffMap;
extern bool scrCBVtolOffMap(void);

/*called when selectedPlayer completes some research*/
extern bool scrCBResCompleted(void);

/* when a player leaves the game*/
extern bool scrCBPlayerLeft(void);

/* when a VTOL runs out of things to do while mid-air */
extern bool scrCBVTOLRetarget(void);

// alliance offered.
extern bool scrCBAllianceOffer(void);
extern UDWORD	CBallFrom,CBallTo;

// player number that left the game
extern UDWORD	CBPlayerLeft;

//Console callback
extern bool scrCallConsole(void);
extern bool scrCBStructBuilt(void);
extern bool scrCallMultiMsg(void);
extern bool scrCallBeacon(void);
extern bool scrCBTransporterLandedB(void);

extern bool scrCBDorderStop(void);
extern bool scrCBDorderReachedLocation(void);
extern bool scrCBProcessKeyPress(void);

#endif // __INCLUDED_SRC_SCRIPTCB_H__
