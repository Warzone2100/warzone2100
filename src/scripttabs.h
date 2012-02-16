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
 *  All the tables for the script compiler
 */

#ifndef __INCLUDED_SRC_SCRIPTTABS_H__
#define __INCLUDED_SRC_SCRIPTTABS_H__

#include "lib/script/event.h" // needed for _scr_user_types
#include "lib/script/parse.h"

// How many game ticks for one event tick
#define SCR_TICKRATE	100


enum SCR_CALLBACK_TYPES
{
	CALL_GAMEINIT = TR_CALLBACKSTART,
	CALL_DELIVPOINTMOVED,
	CALL_DROIDDESIGNED,
	CALL_DROIDBUILT,
	CALL_POWERGEN_BUILT,
	CALL_RESEX_BUILT,
	CALL_RESEARCH_BUILT,
	CALL_FACTORY_BUILT,
	CALL_MISSION_START,
	CALL_MISSION_END,
	CALL_VIDEO_QUIT,
	CALL_LAUNCH_TRANSPORTER,
	CALL_START_NEXT_LEVEL,
	CALL_TRANSPORTER_REINFORCE,
	CALL_MISSION_TIME,
	CALL_ELECTRONIC_TAKEOVER,
	CALL_BUILDLIST,
	CALL_BUILDGRID,
	CALL_RESEARCHLIST,
	CALL_MANURUN,
	CALL_MANULIST,
	CALL_BUTTON_PRESSED,
	CALL_DROID_SELECTED,
	CALL_DESIGN_QUIT,
	CALL_DESIGN_WEAPON,
	CALL_DESIGN_SYSTEM,
	CALL_DESIGN_COMMAND,
	CALL_DESIGN_BODY,
	CALL_DESIGN_PROPULSION,
	CALL_RESEARCHCOMPLETED,
	CALL_NEWDROID,
	CALL_STRUCT_ATTACKED,
	CALL_DROID_ATTACKED,
	CALL_ATTACKED,
	CALL_STRUCT_SEEN,
	CALL_DROID_SEEN,
	CALL_FEATURE_SEEN,
	CALL_OBJ_SEEN,
	CALL_OBJ_DESTROYED,
	CALL_STRUCT_DESTROYED,
	CALL_DROID_DESTROYED,
	CALL_FEATURE_DESTROYED,
	CALL_OBJECTOPEN,
	CALL_OBJECTCLOSE,
	CALL_TRANSPORTER_OFFMAP,
	CALL_TRANSPORTER_LANDED,
	CALL_ALL_ONSCREEN_DROIDS_SELECTED,
	CALL_NO_REINFORCEMENTS_LEFT,
	CALL_CLUSTER_EMPTY,
	CALL_VTOL_OFF_MAP,
	CALL_UNITTAKEOVER,
	CALL_PLAYERLEFT,
	CALL_ALLIANCEOFFER,
	CALL_CONSOLE,			// Gets fired when user types something in the console and presses enter
	CALL_AI_MSG,			// Player received msg from another player
	CALL_BEACON,			// Beacon help (blip) msg received
	CALL_STRUCTBUILT,		// Gets fired when a structure is built for a certain player, returns structure
	CALL_TRANSPORTER_LANDED_B,
	CALL_DORDER_STOP,		// Fired when droid is forced to stop via user interface
	CALL_DROID_REACH_LOCATION,	// Fired when droid reached the destination and stopped on its own
	CALL_KEY_PRESSED,		// Allows to process key presses, mainly for debug purposes
	CALL_VTOL_RETARGET,		// VTOL is out of targets
};

// The table of user types for the compiler
extern TYPE_SYMBOL asTypeTable[];

// The table of script callable functions
extern FUNC_SYMBOL asFuncTable[];

// The table of external variables
extern VAR_SYMBOL asExternTable[];

// The table of object variables
extern VAR_SYMBOL asObjTable[];

// The table of constant values
extern CONST_SYMBOL asConstantTable[];

// Initialise the script system
extern bool scrTabInitialise(void);

// Shut down the script system
extern void scrShutDown(void);

#endif // __INCLUDED_SRC_SCRIPTTABS_H__
