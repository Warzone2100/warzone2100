/*
 * ScriptTabs.h
 *
 * All the tables for the script compiler
 *
 */
#ifndef _scripttabs_h
#define _scripttabs_h

#include "lib/script/event.h" // needed for _scr_user_types

// How many game ticks for one event tick
#define SCR_TICKRATE	100


#define BARB1		6	
#define BARB2		7


typedef enum _scr_callback_types
{
	CALL_GAMEINIT = TR_CALLBACKSTART,
	CALL_DELIVPOINTMOVED,
	CALL_DROIDDESIGNED,
	//CALL_RESEARCHCOMPLETED,       callback function added
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
} SCR_CALLBACK_TYPES;

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
extern BOOL scrTabInitialise(void);

// Shut down the script system
extern void scrShutDown(void);

#endif

