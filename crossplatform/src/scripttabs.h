/*
 * ScriptTabs.h
 *
 * All the tables for the script compiler
 *
 */
#ifndef _scripttabs_h
#define _scripttabs_h

// How many game ticks for one event tick
#define SCR_TICKRATE	100


#define BARB1		6	
#define BARB2		7


// ID numbers for each user type
typedef enum _scr_user_types
{
	ST_INTMESSAGE = VAL_USERTYPESTART,		// Intelligence message ?? (6)
	ST_BASEOBJECT,							// Base object
	ST_DROID,								// Droid object
	ST_STRUCTURE,							// Structure object
	ST_FEATURE,								// Feature object
	ST_BASESTATS,							// General stats type
	ST_COMPONENT,							// General component
	ST_BODY,								// Component types
	ST_PROPULSION,
	ST_ECM,
	ST_SENSOR,
	ST_CONSTRUCT,
	ST_WEAPON,
	ST_REPAIR,
	ST_BRAIN,
	ST_TEMPLATE,							// Template object
	ST_STRUCTUREID,							/* A structure ID number (don't really 
											   need this since just a number?)*/
	ST_STRUCTURESTAT,						// structure stat type
	ST_FEATURESTAT,							// feature stat type
	ST_DROIDID,								// ID of a droid
	ST_TEXTSTRING,							// text string for display messages in tutorial
	ST_SOUND,
	ST_LEVEL,								// The name of a game level
	ST_GROUP,								// A group of droids
	ST_RESEARCH,							// A research topic

	//private types for game code - not for use in script
	ST_POINTER_O,								//used so we can check for NULL objects etc
	ST_POINTER_T,								//used so we can check for NULL templates
	ST_POINTER_S,								// NULL stats

	ST_MAXTYPE,									// maximum possible type - should always be last
} SCR_USER_TYPES;

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

