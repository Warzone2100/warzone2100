/*
 * ScriptTabs.c
 *
 * All the tables for the script compiler
 *
 */


#include "Frame.h"
#include "widget.h"
#include "Objects.h"
#include "Script.h"
#include "ScriptTabs.h"

// Get all the function prototypes
#include "ScriptFuncs.h"
#include "ScriptExtern.h"
#include "ScriptObj.h"
#include "ScriptVals.h"
#include "ScriptCB.h"
#include "ScriptAI.h"

#include "droid.h"
#include "HCI.h"
#include "message.h"
//#include "mission.h"
#include "levels.h"
#include "Order.h"
#include "GTime.h"
#include "mission.h"

#include "design.h"			// for the iddes_...
#include "display.h"		// for the MT_...  
#ifdef WIN32
#include "multiplay.h"
#include "intfac.h"
#else
#include "intpsx.h"

#endif

/* The table of user defined types
 * The format is :
 *         <type id no.>,  "Type name",   <access type>
 *
 * The type id no. should start at VAL_USERTYPESTART and increase by one per type.
 * The access type controls whether the type is a simple data type or an object type.
 */
TYPE_SYMBOL asTypeTable[] =
{
	{ ST_INTMESSAGE,	AT_SIMPLE, "INTMESSAGE",	scrValDefSave, scrValDefLoad },
	{ ST_BASEOBJECT,	AT_OBJECT, "BASEOBJ",		scrValDefSave, scrValDefLoad },
	{ ST_DROID,			AT_OBJECT, "DROID",			scrValDefSave, scrValDefLoad },
	{ ST_STRUCTURE,		AT_OBJECT, "STRUCTURE",		scrValDefSave, scrValDefLoad },
	{ ST_FEATURE,		AT_OBJECT, "FEATURE",		scrValDefSave, scrValDefLoad },

	{ ST_BASESTATS,		AT_SIMPLE, "BASESTATS",		scrValDefSave, scrValDefLoad },

	// Component types
	{ ST_COMPONENT,		AT_SIMPLE, "COMPONENT",		scrValDefSave, scrValDefLoad },
	{ ST_BODY,			AT_SIMPLE, "BODY",			scrValDefSave, scrValDefLoad },
	{ ST_PROPULSION,	AT_SIMPLE, "PROPULSION",	scrValDefSave, scrValDefLoad },
	{ ST_ECM,			AT_SIMPLE, "ECM",			scrValDefSave, scrValDefLoad },
	{ ST_SENSOR,		AT_SIMPLE, "SENSOR",		scrValDefSave, scrValDefLoad },
	{ ST_CONSTRUCT,		AT_SIMPLE, "CONSTRUCT",		scrValDefSave, scrValDefLoad },
	{ ST_WEAPON,		AT_SIMPLE, "WEAPON",		scrValDefSave, scrValDefLoad },
	{ ST_REPAIR,		AT_SIMPLE, "REPAIR",		scrValDefSave, scrValDefLoad },
	{ ST_BRAIN,			AT_SIMPLE, "BRAIN",			scrValDefSave, scrValDefLoad },

	{ ST_TEMPLATE,		AT_SIMPLE, "TEMPLATE",		scrValDefSave, scrValDefLoad },

	{ ST_STRUCTUREID,	AT_SIMPLE, "STRUCTUREID",	scrValDefSave, scrValDefLoad },
	{ ST_STRUCTURESTAT,	AT_SIMPLE, "STRUCTURESTAT",	scrValDefSave, scrValDefLoad },
	{ ST_FEATURESTAT,	AT_SIMPLE, "FEATURESTAT",	scrValDefSave, scrValDefLoad },
	{ ST_DROIDID,		AT_SIMPLE, "DROIDID",		scrValDefSave, scrValDefLoad },
	{ ST_TEXTSTRING,	AT_SIMPLE, "TEXTSTRING",	scrValDefSave, scrValDefLoad },
	{ ST_SOUND,			AT_SIMPLE, "SOUND",			scrValDefSave, scrValDefLoad },
	{ ST_LEVEL,			AT_SIMPLE, "LEVEL",			scrValDefSave, scrValDefLoad },
	{ ST_GROUP,			AT_OBJECT, "GROUP",			scrValDefSave, scrValDefLoad },
	{ ST_RESEARCH,		AT_SIMPLE, "RESEARCHSTAT",	scrValDefSave, scrValDefLoad },

	//private types for code - NOT used in the scripts - hence the ""
	{ ST_POINTER_O,		AT_OBJECT,	"" },
	{ ST_POINTER_T,		AT_SIMPLE,	"" },
	{ ST_POINTER_S,		AT_SIMPLE,	"" },

	/* This final entry marks the end of the type list */
	{ 0,				AT_SIMPLE,	"END OF TYPE LIST" },
};

#ifdef PSX

// if we are just running binary scripts we don't want any of all the bollox that comes with the function table 
// ... all we want is a table of pointers to functions - THIS ALONE TAKES 24k OFF THE OBJECT CODE !!!

// If these defines don't compile on the PC you had better go and see Tim... it's his fault

#ifdef NOSCRIPT
#define FUNC(a,b,c,d,fmt...) { b }	
#else
#define FUNC(a,b,c,d,fmt...) { a,b,c,d, ## fmt }	
#endif

/* The table of script callable C functions
 * This is laid out :
 *     "ScriptName",   function_Pointer,  <function return type>
 *      <number of parameters>,  <parameter types>,  FALSE
 */
FUNC_SYMBOL asFuncTable[] =
{
	// These functions are part of the script library


	FUNC("traceOn",					interpTraceOn,				VAL_VOID,	0, { VAL_VOID } ),
	FUNC("traceOff",				interpTraceOff,				VAL_VOID,	0, { VAL_VOID } ),
	FUNC("setEventTrigger",			eventSetTrigger,			VAL_VOID,	2, { VAL_EVENT, VAL_TRIGGER } ),
	FUNC("eventTraceLevel",			eventSetTraceLevel,			VAL_VOID,	1, { VAL_INT } ),

	// Trigger functions
	FUNC("objectInRange",			scrObjectInRange,			VAL_BOOL,	4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("droidInRange",			scrDroidInRange,			VAL_BOOL,	4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("structInRange",			scrStructInRange,			VAL_BOOL,	4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),

	FUNC("playerPower",				scrPlayerPower,				VAL_INT,	1, { VAL_INT } ),

	FUNC("objectInArea",			scrObjectInArea,			VAL_BOOL,	5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("droidInArea",				scrDroidInArea,				VAL_BOOL,	5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("structInArea",			scrStructInArea,			VAL_BOOL,	5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("seenStructInArea",		scrSeenStructInArea,		VAL_BOOL,	7, { VAL_INT, VAL_INT, VAL_BOOL,VAL_INT,VAL_INT, VAL_INT, VAL_INT } ),

	FUNC("structButNoWallsInArea",	scrStructButNoWallsInArea,	VAL_BOOL,	5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("numObjectsInArea",		scrNumObjectsInArea,		VAL_INT,   	5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
   	FUNC("numDroidsInArea",			scrNumDroidsInArea,			VAL_INT, 	5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("numStructsInArea",		scrNumStructsInArea,		VAL_INT, 	5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("numStructsButNotWallsInArea",scrNumStructsButNotWallsInArea,VAL_INT,5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("numStructsByTypeInArea",	scrNumStructsByTypeInArea,	VAL_INT,	6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	


	FUNC("droidHasSeen",			scrDroidHasSeen,			VAL_BOOL,	2, { ST_BASEOBJECT, VAL_INT } ),
	FUNC("buildingDestroyed",		scrBuildingDestroyed,		VAL_BOOL,	2, { ST_STRUCTUREID, VAL_INT } ),
	FUNC("structureIdle",			scrStructureIdle,  			VAL_BOOL,	1, { ST_STRUCTURE } ),

	FUNC( "initEnumStruct",			scrInitEnumStruct,			VAL_VOID,	4, { VAL_BOOL, ST_STRUCTURESTAT, VAL_INT, VAL_INT} ),
	FUNC( "enumStruct",				scrEnumStruct,				ST_STRUCTURE,0, { VAL_VOID} ),

	FUNC("structureBeingBuilt",		scrStructureBeingBuilt,		VAL_BOOL,	2,	{ ST_STRUCTURESTAT, VAL_INT } ),
	FUNC("structureBuilt",			scrStructureBuilt,			VAL_BOOL,	2,	{ ST_STRUCTURESTAT, VAL_INT } ),

	FUNC("anyDroidsLeft",			scrAnyDroidsLeft,			VAL_BOOL,	1,	{ VAL_INT } ),
	FUNC("anyStructButWallsLeft",	scrAnyStructButWallsLeft,	VAL_BOOL,	1,	{ VAL_INT } ),
	FUNC("anyFactoriesLeft",		scrAnyFactoriesLeft,		VAL_BOOL,	1,	{ VAL_INT } ),

	// event functions
	FUNC("enableComponent",			scrEnableComponent,			VAL_VOID, 	2, { ST_COMPONENT, VAL_INT } ),
	FUNC("makeComponentAvailable",	scrMakeComponentAvailable,	VAL_VOID,2, { ST_COMPONENT, VAL_INT } ),
	FUNC("enableStructure",			scrEnableStructure,			VAL_VOID, 	2, { ST_STRUCTURESTAT, VAL_INT } ),

	FUNC("addDroid",				scrAddDroid,  				ST_DROID, 	4, { ST_TEMPLATE, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("addDroidToMissionList",	scrAddDroidToMissionList,  	ST_DROID, 	2, { ST_TEMPLATE, VAL_INT } ),
	FUNC("buildDroid",				scrBuildDroid,				VAL_VOID,  	4, { ST_TEMPLATE, ST_STRUCTURE, VAL_INT, VAL_INT } ),
	FUNC("addTemplate",				scrAddTemplate,  			VAL_BOOL, 	2, { ST_TEMPLATE, VAL_INT } ),

	FUNC("addReticuleButton",		scrAddReticuleButton,		VAL_VOID, 	1, { VAL_INT } ),
	FUNC("removeReticuleButton",	scrRemoveReticuleButton,	VAL_VOID,	1, { VAL_INT } ),
	FUNC("addMessage",				scrAddMessage,				VAL_VOID, 	4, { ST_INTMESSAGE, VAL_INT, VAL_INT, VAL_BOOL } ),
	FUNC("removeMessage",			scrRemoveMessage,			VAL_VOID, 	3, { ST_INTMESSAGE, VAL_INT, VAL_INT } ),
 
/*	{ "addTutorialMessage",		scrAddTutorialMessage,		VAL_VOID, 	2, { ST_INTMESSAGE, VAL_INT } },*/
 
	FUNC( "selectDroidByID",		scrSelectDroidByID,			VAL_BOOL,	2, { ST_DROIDID, VAL_INT } ),
	FUNC( "setAssemblyPoint",		scrSetAssemblyPoint,		VAL_VOID,	3, { ST_STRUCTURE, VAL_INT, VAL_INT } ),
	FUNC( "attackLocation",			scrAttackLocation,			VAL_VOID,	3, { VAL_INT, VAL_INT, VAL_INT } ),

	FUNC( "initGetFeature",			scrInitGetFeature,			VAL_VOID,	3, { ST_FEATURESTAT, VAL_INT,VAL_INT} ),
	FUNC( "getFeature",				scrGetFeature,				ST_FEATURE,	1, { VAL_INT} ),

	FUNC( "addFeature",				scrAddFeature,				ST_FEATURE,	3, { ST_FEATURESTAT, VAL_INT, VAL_INT } ),
	FUNC( "destroyFeature",			scrDestroyFeature,			VAL_VOID, 	1, { ST_FEATURE } ),
	FUNC( "addStructure",			scrAddStructure,			ST_STRUCTURE,4, { ST_STRUCTURESTAT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC( "destroyStructure",		scrDestroyStructure,		VAL_VOID,  	1, { ST_STRUCTURE } ),
	FUNC( "centreView",				scrCentreView,				VAL_VOID,  	1, { ST_BASEOBJECT } ),
	FUNC( "centreViewPos",			scrCentreViewPos,			VAL_VOID,	2, { VAL_INT, VAL_INT } ),
	FUNC( "getStructure",			scrGetStructure,			ST_STRUCTURE,2, { ST_STRUCTURESTAT, VAL_INT } ),
	FUNC( "getTemplate",			scrGetTemplate,				ST_TEMPLATE, 2, { ST_COMPONENT, VAL_INT } ),
	FUNC("getDroid",				scrGetDroid,				ST_DROID,	2, { ST_COMPONENT, VAL_INT } ),
	FUNC("setScrollParams",			scrSetScrollParams,			VAL_VOID,	4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT} ),
	FUNC("setScrollMinX",			scrSetScrollMinX,			VAL_VOID,	1, { VAL_INT } ),
	FUNC("setScrollMinY",			scrSetScrollMinY,			VAL_VOID,  	1, { VAL_INT } ),
	FUNC("setScrollMaxX",			scrSetScrollMaxX,			VAL_VOID,	1, { VAL_INT } ),
	FUNC("setScrollMaxY",			scrSetScrollMaxY,			VAL_VOID,	1, { VAL_INT } ),
	FUNC("setDefaultSensor",		scrSetDefaultSensor,		VAL_VOID,  	2, { ST_SENSOR, VAL_INT } ),
	FUNC("setDefaultECM",			scrSetDefaultECM,			VAL_VOID,  	2, { ST_ECM, VAL_INT } ),
	FUNC("setDefaultRepair",		scrSetDefaultRepair,		VAL_VOID,  	2, { ST_REPAIR, VAL_INT } ),
	FUNC("setStructureLimits",		scrSetStructureLimits,		VAL_VOID,	3, { ST_STRUCTURESTAT, VAL_INT, VAL_INT } ),
	FUNC("setAllStructureLimits",	scrSetAllStructureLimits,	VAL_VOID,	2, { VAL_INT, VAL_INT } ),
	FUNC("playSound",				scrPlaySound,				VAL_VOID,	2, { ST_SOUND, VAL_INT } ),
	FUNC("playSoundPos",			scrPlaySoundPos,			VAL_VOID,	5, { ST_SOUND, VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("addConsoleText",			scrAddConsoleText,			VAL_VOID,	2, { ST_TEXTSTRING, VAL_INT } ),
	FUNC("showConsoleText",			scrShowConsoleText,			VAL_VOID,	2, { ST_TEXTSTRING, VAL_INT } ),
	FUNC("turnPowerOn",				scrTurnPowerOn,				VAL_VOID,	0, { VAL_VOID } ),
	FUNC("turnPowerOff",			scrTurnPowerOff,			VAL_VOID,	0, { VAL_VOID } ),
	FUNC("tutorialEnd",				scrTutorialEnd,				VAL_VOID,	0, { VAL_VOID } ),
	FUNC("playVideo",				scrPlayVideo,				VAL_VOID,	2, { ST_TEXTSTRING, ST_TEXTSTRING } ),
	FUNC("gameOverMessage",			scrGameOverMessage,			VAL_VOID,   4, { ST_INTMESSAGE, VAL_INT, VAL_INT, VAL_BOOL } ),
	FUNC("gameOver",				scrGameOver,				VAL_VOID,   1, { VAL_BOOL } ),
	FUNC("playBackgroundAudio",		scrPlayBackgroundAudio,		VAL_VOID,	2, { ST_TEXTSTRING, VAL_INT } ),
	FUNC("playCDAudio",				scrPlayCDAudio,				VAL_VOID,	1, { VAL_INT } ),
	FUNC("stopCDAudio",				scrStopCDAudio,				VAL_VOID,   0, { VAL_VOID } ),
	FUNC("pauseCDAudio",			scrPauseCDAudio,			VAL_VOID,	0, { VAL_VOID } ),
	FUNC("resumeCDAudio",			scrResumeCDAudio,			VAL_VOID,	0, { VAL_VOID } ),
	FUNC("setRetreatPoint",			scrSetRetreatPoint,			VAL_VOID,   3, { VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("setRetreatForce",			scrSetRetreatForce,			VAL_VOID,	2, { VAL_INT, VAL_INT } ),
	FUNC("setRetreatLeadership",	scrSetRetreatLeadership,	VAL_VOID,2, { VAL_INT, VAL_INT } ),
	FUNC("setGroupRetreatForce",	scrSetGroupRetreatForce,   	VAL_VOID,2, { ST_GROUP, VAL_INT } ),
	FUNC("setGroupRetreatLeadership",	scrSetGroupRetreatLeadership,	VAL_VOID,2, { ST_GROUP, VAL_INT } ),
	FUNC("setGroupRetreatPoint",	scrSetGroupRetreatPoint,	VAL_VOID,3, { ST_GROUP, VAL_INT, VAL_INT } ),
	FUNC("setRetreatHealth",		scrSetRetreatHealth,		VAL_VOID,	2, { VAL_INT, VAL_INT } ),
	FUNC("setGroupRetreatHealth",	scrSetGroupRetreatHealth,   VAL_VOID,2, { ST_GROUP, VAL_INT } ),
	FUNC("startMission",			scrStartMission,			VAL_VOID,	2, { VAL_INT, ST_LEVEL } ),
	FUNC("setSnow",					scrSetSnow,					VAL_VOID,	1, { VAL_BOOL } ),
	FUNC("setRain",					scrSetRain,					VAL_VOID,	1, { VAL_BOOL } ),
	FUNC("setBackgroundFog",		scrSetBackgroundFog,		VAL_VOID,	1, { VAL_BOOL } ),
	FUNC("setDepthFog",				scrSetDepthFog,				VAL_VOID,	1, { VAL_BOOL } ),
	FUNC("setFogColour",			scrSetFogColour,			VAL_VOID,	3, { VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("setTransporterExit",		scrSetTransporterExit,		VAL_VOID,	3, { VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("flyTransporterIn",		scrFlyTransporterIn,		VAL_VOID,	4, { VAL_INT, VAL_INT, VAL_INT, VAL_BOOL } ),
	FUNC("addDroidToTransporter",	scrAddDroidToTransporter,	VAL_VOID,	2, { ST_DROID, ST_DROID } ),

	/*{ "endMission",			scrEndMission,				VAL_VOID,1, { VAL_BOOL } },*/

	FUNC("structureBuiltInRange",	scrStructureBuiltInRange,	ST_STRUCTURE, 5, { ST_STRUCTURESTAT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("random",					scrRandom,					VAL_INT,	1, { VAL_INT } ),
	FUNC("randomiseSeed", 			scrRandomiseSeed,			VAL_VOID,	0, { VAL_VOID } ),
	FUNC("enableResearch",			scrEnableResearch,			VAL_VOID,		2, { ST_RESEARCH, VAL_INT } ),
	FUNC("completeResearch",		scrCompleteResearch,		VAL_VOID,		2, { ST_RESEARCH, VAL_INT } ),
	FUNC("flashOn",					scrFlashOn,					VAL_VOID,		1, { VAL_INT } ),
	FUNC("flashOff",				scrFlashOff,				VAL_VOID, 		1, { VAL_INT } ),
	FUNC("setPowerLevel",			scrSetPowerLevel,			VAL_VOID, 		2, { VAL_INT, VAL_INT } ),
	FUNC("addPower",			    scrAddPower,			    VAL_VOID, 		2, { VAL_INT, VAL_INT } ),
	FUNC("setLandingZone",			scrSetLandingZone,			VAL_VOID,		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("setLimboLanding",			scrSetLimboLanding,			VAL_VOID,		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
    FUNC("setNoGoArea",				scrSetNoGoArea,				VAL_VOID,		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("initAllNoGoAreas",		scrInitAllNoGoAreas,		VAL_VOID,		0, { VAL_VOID } ),
	FUNC("setRadarZoom",			scrSetRadarZoom,			VAL_VOID,		1, { VAL_INT } ),
	FUNC("setReinforcementTime",	scrSetReinforcementTime,	VAL_VOID,		1, { VAL_INT } ),
	FUNC("setMissionTime",			scrSetMissionTime,			VAL_VOID,		1, { VAL_INT } ),
	FUNC("missionTimeRemaining",	scrMissionTimeRemaining,	VAL_INT,		0, { VAL_VOID } ),
    FUNC("flushConsoleMessages",	scrFlushConsoleMessages,   	VAL_VOID,		0, { VAL_VOID} ),
	FUNC("pickStructLocation",		scrPickStructLocation,		VAL_BOOL,		4, {ST_STRUCTURESTAT, VAL_REF | VAL_INT, VAL_REF | VAL_INT, VAL_INT  } ),

	// AI functions
	FUNC("groupAddDroid",			scrGroupAddDroid,			VAL_VOID,	2, { ST_GROUP, ST_DROID } ),
	FUNC("groupAddArea",			scrGroupAddArea,			VAL_VOID,	6, { ST_GROUP, VAL_INT, VAL_INT,VAL_INT, VAL_INT,VAL_INT } ),
	FUNC("groupAddAreaNoGroup",		scrGroupAddArea,			VAL_VOID,	6, { ST_GROUP, VAL_INT, VAL_INT,VAL_INT, VAL_INT,VAL_INT } ),
	FUNC("groupAddGroup",			scrGroupAddGroup,			VAL_VOID,	2, { ST_GROUP, ST_GROUP } ),
	FUNC("groupMember",				scrGroupMember,				VAL_BOOL,	2, { ST_GROUP, ST_DROID } ),
	FUNC("idleGroup",				scrIdleGroup,				VAL_INT,	1, { ST_GROUP } ),
	FUNC("initIterateGroup",		scrInitIterateGroup,		VAL_VOID,	1, { ST_GROUP } ),
	FUNC("iterateGroup",			scrIterateGroup,			ST_DROID,	1, { ST_GROUP } ),
	FUNC("droidLeaveGroup",			scrDroidLeaveGroup,			VAL_VOID,	1, { ST_DROID } ),


	FUNC("orderDroid",				scrOrderDroid,				VAL_VOID,	2, { ST_DROID, VAL_INT } ),
	FUNC("orderDroidLoc",			scrOrderDroidLoc,			VAL_VOID,	4, { ST_DROID, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("orderDroidObj",			scrOrderDroidObj,			VAL_VOID, 	3, { ST_DROID, VAL_INT, ST_BASEOBJECT } ),
	FUNC("orderDroidStatsLoc",		scrOrderDroidStatsLoc,		VAL_VOID, 	5, { ST_DROID, VAL_INT, ST_BASESTATS, VAL_INT, VAL_INT } ),

	FUNC("orderGroup",				scrOrderGroup,				VAL_VOID, 	2, { ST_GROUP, VAL_INT } ),
	FUNC("orderGroupLoc",			scrOrderGroupLoc,			VAL_VOID, 	4, { ST_GROUP, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("orderGroupObj",			scrOrderGroupObj,			VAL_VOID,	3, { ST_GROUP, VAL_INT, ST_BASEOBJECT } ),
	FUNC("setDroidSecondary",		scrSetDroidSecondary,		VAL_VOID, 	3, { ST_DROID, VAL_INT, VAL_INT } ),
	FUNC("setGroupSecondary",		scrSetGroupSecondary,		VAL_VOID,	3, { ST_GROUP, VAL_INT, VAL_INT } ),

	FUNC("initIterateCluster",		scrInitIterateCluster,		VAL_VOID,	1, { VAL_INT } ),
	FUNC("iterateCluster",			scrIterateCluster,			ST_BASEOBJECT,	0, { VAL_VOID } ),
	FUNC("cmdDroidAddDroid",		scrCmdDroidAddDroid,		VAL_VOID,	2, { ST_DROID, ST_DROID } ),

	// a couple of example functions
	FUNC("numMessageBox",			scrNumMB,					VAL_VOID, 	1, { VAL_INT } ),
	FUNC("debugBox",				scrNumMB,					VAL_VOID, 	1, { VAL_INT } ),
	FUNC("approxRoot",				scrApproxRoot,				VAL_INT,  	2, { VAL_INT, VAL_INT } ),
	FUNC("refTest",					scrRefTest,					VAL_VOID, 	1, {VAL_INT} ), 

	// geo functions
	FUNC("distBetweenTwoPoints",	scrDistanceTwoPts,			VAL_INT, 	4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("losTwoObjects",			scrLOSTwoBaseObjects,		VAL_BOOL,	3, { ST_BASEOBJECT, ST_BASEOBJECT, VAL_BOOL } ),
	FUNC("killStructsInArea",		scrDestroyStructuresInArea,	VAL_VOID,	8, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL } ),
	FUNC("getThreatInArea",			scrThreatInArea,			VAL_INT,	10, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_BOOL} ),
   	FUNC("getNearestGateway",		scrGetNearestGateway,		VAL_BOOL,	4, { VAL_INT, VAL_INT, VAL_REF|VAL_INT, VAL_REF|VAL_INT } ),
	FUNC("setWaterTile",			scrSetWaterTile,			VAL_VOID,	1, { VAL_INT } ),
	FUNC("setRubbleTile",			scrSetRubbleTile,			VAL_VOID,	1, { VAL_INT } ),
	FUNC("setCampaignNumber",		scrSetCampaignNumber,		VAL_VOID,	1, { VAL_INT } ),
	FUNC("testStructureModule",		scrTestStructureModule,		VAL_BOOL,	3, {VAL_INT,ST_STRUCTURE,VAL_INT} ),
	FUNC("killDroidsInArea",		scrDestroyUnitsInArea,		VAL_VOID,	5, {VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT} ),
	FUNC("vanishUnit",				scrRemoveDroid,				VAL_VOID,   1, {ST_DROID} ),
   	FUNC("forceDamageObject",		scrForceDamage,				VAL_VOID,	2, {ST_BASEOBJECT,VAL_INT} ),

	//multiplayer stuff.
	FUNC("createAlliance",			scrCreateAlliance,	 		VAL_VOID, 	2, { VAL_INT, VAL_INT } ),
	FUNC("breakAlliance",			scrBreakAlliance,	 		VAL_VOID,	2, { VAL_INT, VAL_INT } ),
	FUNC("allianceExists",			scrAllianceExists,	 		VAL_BOOL,	0, {VAL_VOID } ),
	FUNC("allianceExistsBetween",	scrAllianceExistsBetween,	VAL_BOOL,	2, { VAL_INT, VAL_INT } ),
	FUNC("playerInAlliance",		scrPlayerInAlliance,	 	VAL_BOOL,	1, {VAL_INT } ),
//m	FUNC("dominatingAlliance",		scrDominatingAlliance,		VAL_BOOL,	0, {VAL_VOID } ),
//m	FUNC("myResponsibility",		scrMyResponsibility,		VAL_BOOL,	1, {VAL_INT } ),

	// object conversion routines
	FUNC("objToDroid",				scrObjToDroid,		  		ST_DROID, 	1, { ST_BASEOBJECT } ),
	FUNC("objToStructure",			scrObjToStructure,	  		ST_STRUCTURE,1, { ST_BASEOBJECT } ),
	FUNC("objToFeature",			scrObjToFeature,	  		ST_FEATURE,	1, { ST_BASEOBJECT } ),

	FUNC("getGameStatus", 			scrGetGameStatus,	  		VAL_BOOL,	1, {VAL_INT} ),

    //player colour access functions
	FUNC("getPlayerColour",			scrGetPlayerColour,			VAL_INT, 	1, { VAL_INT } ),
	FUNC("setPlayerColour",			scrSetPlayerColour,			VAL_VOID, 	2, { VAL_INT, VAL_INT } ),

	FUNC("takeOverDroidsInArea",	scrTakeOverDroidsInArea,	VAL_INT, 	    6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("takeOverDroidsInAreaExp",	scrTakeOverDroidsInAreaExp,	VAL_INT, 	    8, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("takeOverStructsInArea",	scrTakeOverStructsInArea,	VAL_INT, 	    6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } ),
	FUNC("takeOverSingleDroid",	    scrTakeOverSingleDroid,	    ST_DROID, 	    2, { ST_DROID, VAL_INT} ),
	FUNC("takeOverSingleStructure",	scrTakeOverSingleStructure,	ST_STRUCTURE,   2, { ST_STRUCTURE, VAL_INT} ),

	FUNC("resetStructTargets",		scrResetStructTargets,		VAL_VOID,	    0, { VAL_VOID }, ),
	FUNC("resetDroidTargets",		scrResetDroidTargets,		VAL_VOID,		0, { VAL_VOID }, ),
	FUNC("setStructTarPref",		scrSetStructTarPref,		VAL_VOID,		1, { VAL_INT }, ),
	FUNC("setStructTarIgnore",		scrSetStructTarIgnore,		VAL_VOID,		1, { VAL_INT }, ),
	FUNC("setDroidTarPref",			scrSetDroidTarPref,			VAL_VOID,		1, { VAL_INT }, ),
	FUNC("setDroidTarIgnore",		scrSetDroidTarIgnore,		VAL_VOID,		1, { VAL_INT }, ),
	FUNC("structTargetInArea",		scrStructTargetInArea,		ST_STRUCTURE,	6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, }, ),
	FUNC("structTargetOnMap",		scrStructTargetOnMap,		ST_STRUCTURE,	2, { VAL_INT, VAL_INT }, ),
	FUNC("droidTargetInArea",		scrDroidTargetInArea,		ST_DROID,		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, }, ),
	FUNC("droidTargetOnMap",		scrDroidTargetOnMap,		ST_DROID,		2, { VAL_INT, VAL_INT }, ),
	FUNC("targetInCluster",			scrTargetInCluster,			ST_DROID,		2, { VAL_INT, VAL_INT }, ),

    FUNC("setDroidsToSafetyFlag",   scrSetDroidsToSafetyFlag,   VAL_VOID,	    1, { VAL_BOOL } ),

    FUNC("setPlayCountDown",        scrSetPlayCountDown,        VAL_VOID,	    1, { VAL_BOOL } ),

    FUNC("getDroidCount",			scrGetDroidCount,			VAL_INT, 	    1, { VAL_INT } ),

	FUNC("fireWeaponAtObj",		scrFireWeaponAtObj,			VAL_VOID,	2, { ST_WEAPON, ST_BASEOBJECT } ),
	FUNC("fireWeaponAtLoc",		scrFireWeaponAtLoc,			VAL_VOID,	3, { ST_WEAPON, VAL_INT,VAL_INT } ),

	FUNC("setDroidKills",			scrSetDroidKills,		VAL_VOID,	2, { ST_DROID, VAL_INT } ),
	FUNC("resetPlayerVisibility",	scrResetPlayerVisibility,	VAL_VOID,	1, { VAL_INT } ),
	FUNC("setVTOLReturnPos",		scrSetVTOLReturnPos,		VAL_VOID,	3, { VAL_INT, VAL_INT, VAL_INT } ),

	FUNC("resetLimboMission",		scrResetLimboMission,		VAL_VOID,	0, { VAL_VOID } ),
	

//PD	FUNC("tutorialTemplates",		scrTutorialTemplates,		VAL_VOID,	0, { VAL_VOID } ),

	/* This final entry marks the end of the function list */
	FUNC("FUNCTION LIST END",		NULL,						VAL_VOID,	0, { VAL_VOID })
};



#else


/* The table of script callable C functions
 * This is laid out :
 *     "ScriptName",   function_Pointer,  <function return type>
 *      <number of parameters>,  <parameter types>,  FALSE
 */
FUNC_SYMBOL asFuncTable[] =
{
	{ "InitEnumDroids",			scrInitEnumDroids,		VAL_VOID,
		2, { VAL_INT, VAL_INT } },
	{ "EnumDroid",			scrEnumDroid,		ST_DROID,
		0, { VAL_VOID } },

	// These functions are part of the script library
	{ "traceOn",				interpTraceOn,			VAL_VOID,
		0, { VAL_VOID } },
	{ "traceOff",				interpTraceOff,			VAL_VOID,
		0, { VAL_VOID } },
	{ "setEventTrigger",		eventSetTrigger,		VAL_VOID,
		2, { VAL_EVENT, VAL_TRIGGER } },
	{ "eventTraceLevel",		eventSetTraceLevel,		VAL_VOID,
		1, { VAL_INT } },

	// Trigger functions
	{ "objectInRange",			scrObjectInRange,		VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "droidInRange",			scrDroidInRange,		VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "structInRange",			scrStructInRange,		VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "playerPower",			scrPlayerPower,			VAL_INT,
		1, { VAL_INT } },

	{ "objectInArea",			scrObjectInArea,		VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "droidInArea",			scrDroidInArea,			VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "structInArea",			scrStructInArea,		VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "seenStructInArea",		scrSeenStructInArea,	VAL_BOOL,
		7, { VAL_INT, VAL_INT, VAL_BOOL,VAL_INT,VAL_INT, VAL_INT, VAL_INT } },

	{ "structButNoWallsInArea",	scrStructButNoWallsInArea,	VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "numObjectsInArea",		scrNumObjectsInArea,	VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "numDroidsInArea",		scrNumDroidsInArea,		VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "numStructsInArea",		scrNumStructsInArea,	VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "numStructsButNotWallsInArea",	scrNumStructsButNotWallsInArea,	VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "numStructsByTypeInArea",		scrNumStructsByTypeInArea,	VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },


	{ "droidHasSeen",			scrDroidHasSeen,		VAL_BOOL,
		2, { ST_BASEOBJECT, VAL_INT } },

	{ "buildingDestroyed",		scrBuildingDestroyed,	VAL_BOOL,
		2, { ST_STRUCTUREID, VAL_INT } },

	{ "structureIdle",			scrStructureIdle,		VAL_BOOL,
		1, { ST_STRUCTURE } },
	
	{ "initEnumStruct",			scrInitEnumStruct,		VAL_VOID,
		4, { VAL_BOOL, ST_STRUCTURESTAT, VAL_INT,VAL_INT } },

	{ "enumStruct",				scrEnumStruct,			ST_STRUCTURE,
		0, { VAL_VOID} },

	{ "structureBeingBuilt",	scrStructureBeingBuilt,	VAL_BOOL,
		2,	{ ST_STRUCTURESTAT, VAL_INT } },

	{ "structureComplete",	scrStructureComplete,		VAL_BOOL,		// pc multiplayer only
		1,	{ ST_STRUCTURE } },
	
	{ "structureBuilt",			scrStructureBuilt,		VAL_BOOL,
		2,	{ ST_STRUCTURESTAT, VAL_INT } },

	{ "anyDroidsLeft",			scrAnyDroidsLeft,		VAL_BOOL,
		1,	{ VAL_INT } },

	{ "anyStructButWallsLeft",	scrAnyStructButWallsLeft, VAL_BOOL,
		1,	{ VAL_INT } },
	
	{ "anyFactoriesLeft",		scrAnyFactoriesLeft,	VAL_BOOL,
		1,	{ VAL_INT } },

	// event functions
	{ "enableComponent",		scrEnableComponent,		VAL_VOID,
		2, { ST_COMPONENT, VAL_INT } },

	{ "makeComponentAvailable",	scrMakeComponentAvailable,VAL_VOID,
		2, { ST_COMPONENT, VAL_INT } },

	{ "enableStructure",		scrEnableStructure,		VAL_VOID,
		2, { ST_STRUCTURESTAT, VAL_INT } },

	{ "isStructureAvailable",		scrIsStructureAvailable,VAL_BOOL,	// pc multiplay only
		2, { ST_STRUCTURESTAT, VAL_INT } },

	{ "addDroid",		scrAddDroid,					ST_DROID,
		4, { ST_TEMPLATE, VAL_INT, VAL_INT, VAL_INT } },

	{ "addDroidToMissionList",	scrAddDroidToMissionList,ST_DROID,
		2, { ST_TEMPLATE, VAL_INT } },

	{ "buildDroid",		scrBuildDroid,					VAL_VOID,
		4, { ST_TEMPLATE, ST_STRUCTURE, VAL_INT, VAL_INT } },

	{ "addTemplate",		scrAddTemplate,				VAL_BOOL,
		2, { ST_TEMPLATE, VAL_INT } },

	{ "addReticuleButton",		scrAddReticuleButton,	VAL_VOID,
		1, { VAL_INT } },

	{ "removeReticuleButton",	scrRemoveReticuleButton,VAL_VOID,
		2, { VAL_INT, VAL_BOOL } },

	{ "addMessage",				scrAddMessage,			VAL_VOID,
		4, { ST_INTMESSAGE, VAL_INT, VAL_INT, VAL_BOOL } },

	{ "removeMessage",			scrRemoveMessage,		VAL_VOID,
		3, { ST_INTMESSAGE, VAL_INT, VAL_INT } },

/*	{ "addTutorialMessage",		scrAddTutorialMessage,	VAL_VOID,
		2, { ST_INTMESSAGE, VAL_INT } },*/

	{ "selectDroidByID",		scrSelectDroidByID,		VAL_BOOL,
		2, { ST_DROIDID, VAL_INT } },

	{ "setAssemblyPoint",		scrSetAssemblyPoint,	VAL_VOID,
		3, { ST_STRUCTURE, VAL_INT, VAL_INT } },

	{ "attackLocation",			scrAttackLocation,		VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT } },

	{ "initGetFeature",			scrInitGetFeature,		VAL_VOID,
		3, { ST_FEATURESTAT, VAL_INT, VAL_INT } },

	{ "getFeature",				scrGetFeature,			ST_FEATURE,
		1, { VAL_INT} },

	{ "addFeature",				scrAddFeature,			ST_FEATURE,
		3, { ST_FEATURESTAT, VAL_INT, VAL_INT } },

	{ "destroyFeature",			scrDestroyFeature,		VAL_VOID,
		1, { ST_FEATURE } },

	{ "addStructure",			scrAddStructure,		ST_STRUCTURE,
		4, { ST_STRUCTURESTAT, VAL_INT, VAL_INT, VAL_INT } },

	{ "destroyStructure",		scrDestroyStructure,	VAL_VOID,
		1, { ST_STRUCTURE } },

	{ "centreView",				scrCentreView,			VAL_VOID,
		1, { ST_BASEOBJECT } },

	{ "centreViewPos",			scrCentreViewPos,		VAL_VOID,
		2, { VAL_INT, VAL_INT } },

	{ "getStructure",			scrGetStructure,		ST_STRUCTURE,
		2, { ST_STRUCTURESTAT, VAL_INT } },

	{ "getTemplate",			scrGetTemplate,			ST_TEMPLATE,
		2, { ST_COMPONENT, VAL_INT } },

	{ "getDroid",				scrGetDroid,			ST_DROID,
		2, { ST_COMPONENT, VAL_INT } },

	{ "setScrollParams",		scrSetScrollParams,		VAL_VOID,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT} },

	{ "setScrollMinX",		scrSetScrollMinX,			VAL_VOID,
		1, { VAL_INT } },

	{ "setScrollMinY",		scrSetScrollMinY,			VAL_VOID,
		1, { VAL_INT } },

	{ "setScrollMaxX",		scrSetScrollMaxX,			VAL_VOID,
		1, { VAL_INT } },

	{ "setScrollMaxY",		scrSetScrollMaxY,			VAL_VOID,
		1, { VAL_INT } },

	{ "setDefaultSensor",	scrSetDefaultSensor,		VAL_VOID,
		2, { ST_SENSOR, VAL_INT } },

	{ "setDefaultECM",		scrSetDefaultECM,			VAL_VOID,
		2, { ST_ECM, VAL_INT } },

	{ "setDefaultRepair",	scrSetDefaultRepair,		VAL_VOID,
		2, { ST_REPAIR, VAL_INT } },

	{ "setStructureLimits",	scrSetStructureLimits,		VAL_VOID,
		3, { ST_STRUCTURESTAT, VAL_INT, VAL_INT } },

	{ "setAllStructureLimits",	scrSetAllStructureLimits,	VAL_VOID,
		2, { VAL_INT, VAL_INT } },

	{ "applyLimitSet",		scrApplyLimitSet,			VAL_VOID,
		0, { VAL_VOID } },

	{ "playSound",			scrPlaySound,				VAL_VOID,
		2, { ST_SOUND, VAL_INT } },

	{ "playSoundPos",		scrPlaySoundPos,			VAL_VOID,
		5, { ST_SOUND, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "addConsoleText",		scrAddConsoleText,			VAL_VOID,
		2, { ST_TEXTSTRING, VAL_INT } },

	{ "showConsoleText",		scrShowConsoleText,			VAL_VOID,
		2, { ST_TEXTSTRING, VAL_INT } },

	{ "tagConsoleText",		scrTagConsoleText,			VAL_VOID,
		2, { ST_TEXTSTRING, VAL_INT } },

	{ "turnPowerOn",		scrTurnPowerOn,				VAL_VOID,
		0, { VAL_VOID } },

	{ "turnPowerOff",		scrTurnPowerOff,			VAL_VOID,
		0, { VAL_VOID } },

	{ "tutorialEnd",		scrTutorialEnd,				VAL_VOID,
		0, { VAL_VOID } },
	{ "clearConsole",		scrClearConsole,			VAL_VOID,
	0, { VAL_VOID} },

	{ "playVideo",			scrPlayVideo,				VAL_VOID,
		2, { ST_TEXTSTRING, ST_TEXTSTRING } },

	{ "gameOverMessage",	scrGameOverMessage,			VAL_VOID,
		4, { ST_INTMESSAGE, VAL_INT, VAL_INT, VAL_BOOL } },

	{ "gameOver",			scrGameOver,				VAL_VOID,
		1, { VAL_BOOL } },

	{ "playBackgroundAudio", scrPlayBackgroundAudio,	VAL_VOID,
		2, { ST_TEXTSTRING, VAL_INT } },

	{ "playCDAudio",		scrPlayCDAudio,				VAL_VOID,
		1, { VAL_INT } },

	{ "stopCDAudio",		scrStopCDAudio,				VAL_VOID,
		0, { VAL_VOID } },

	{ "pauseCDAudio",		scrPauseCDAudio,			VAL_VOID,
		0, { VAL_VOID } },

	{ "resumeCDAudio",		scrResumeCDAudio,			VAL_VOID,
		0, { VAL_VOID } },

	{ "setRetreatPoint",	scrSetRetreatPoint,			VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT } },

	{ "setRetreatForce",	scrSetRetreatForce,			VAL_VOID,
		2, { VAL_INT, VAL_INT } },

	{ "setRetreatLeadership",	scrSetRetreatLeadership,	VAL_VOID,
		2, { VAL_INT, VAL_INT } },

	{ "setGroupRetreatForce",	scrSetGroupRetreatForce,			VAL_VOID,
		2, { ST_GROUP, VAL_INT } },

	{ "setGroupRetreatLeadership",	scrSetGroupRetreatLeadership,	VAL_VOID,
		2, { ST_GROUP, VAL_INT } },

	{ "setGroupRetreatPoint",	scrSetGroupRetreatPoint,			VAL_VOID,
		3, { ST_GROUP, VAL_INT, VAL_INT } },

	{ "setRetreatHealth",	    scrSetRetreatHealth,		VAL_VOID,
		2, { VAL_INT, VAL_INT } },

	{ "setGroupRetreatHealth",	scrSetGroupRetreatHealth,		VAL_VOID,
		2, { ST_GROUP, VAL_INT } },

    { "startMission",		scrStartMission,			VAL_VOID,
		2, { VAL_INT, ST_LEVEL } },

	{ "setSnow",			scrSetSnow,					VAL_VOID,
		1, { VAL_BOOL } },

	{ "setRain",			scrSetRain,					VAL_VOID,
		1, { VAL_BOOL } },

	{ "setBackgroundFog",	scrSetBackgroundFog,		VAL_VOID,
		1, { VAL_BOOL } },

	{ "setDepthFog",		scrSetDepthFog,				VAL_VOID,
		1, { VAL_BOOL } },

	{ "setFogColour",		scrSetFogColour,			VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT } },

	{ "setTransporterExit",	scrSetTransporterExit,		VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT } },

	{ "flyTransporterIn",	scrFlyTransporterIn,		VAL_VOID,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_BOOL } },

	{ "addDroidToTransporter",	scrAddDroidToTransporter,	VAL_VOID,
		2, { ST_DROID, ST_DROID } },

	/*{ "endMission",			scrEndMission,				VAL_VOID,
		1, { VAL_BOOL } },*/

	{ "structureBuiltInRange",	scrStructureBuiltInRange,	ST_STRUCTURE,
		5, { ST_STRUCTURESTAT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "random",				scrRandom,				VAL_INT,
		1, { VAL_INT } },

	{ "randomiseSeed",		scrRandomiseSeed,		VAL_VOID,
		0, { VAL_VOID } },

	{ "enableResearch",		scrEnableResearch,		VAL_VOID,
		2, { ST_RESEARCH, VAL_INT } },

	{ "completeResearch",	scrCompleteResearch,	VAL_VOID,
		2, { ST_RESEARCH, VAL_INT } },

	{ "flashOn",			scrFlashOn,				VAL_VOID,
		1, { VAL_INT } },
	{ "flashOff",			scrFlashOff,			VAL_VOID,
		1, { VAL_INT } },

	{ "setPowerLevel",		scrSetPowerLevel,		VAL_VOID,
		2, { VAL_INT, VAL_INT } },

	{ "addPower",		    scrAddPower,		    VAL_VOID,
		2, { VAL_INT, VAL_INT } },

	{ "setLandingZone",		scrSetLandingZone,		VAL_VOID,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "setLimboLanding",	scrSetLimboLanding,		VAL_VOID,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "setNoGoArea",		scrSetNoGoArea,			VAL_VOID,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "initAllNoGoAreas",	scrInitAllNoGoAreas,	VAL_VOID,
		0, { VAL_VOID } },

	{ "setRadarZoom",		scrSetRadarZoom,		VAL_VOID,
		1, { VAL_INT } },

	{ "setReinforcementTime", scrSetReinforcementTime, VAL_VOID,
		1, { VAL_INT } },

	{ "setMissionTime",		scrSetMissionTime,		VAL_VOID,
		1, { VAL_INT } },

	{ "missionTimeRemaining",	scrMissionTimeRemaining,    VAL_INT,
		0, { VAL_VOID } },

    { "flushConsoleMessages",	scrFlushConsoleMessages,		VAL_VOID,
		0, { VAL_VOID } },

	{ "pickStructLocation",		scrPickStructLocation,	VAL_BOOL,
		4, {ST_STRUCTURESTAT, VAL_REF | VAL_INT, VAL_REF | VAL_INT, VAL_INT } },

	// AI functions
	{ "groupAddDroid",		scrGroupAddDroid,			VAL_VOID,
		2, { ST_GROUP, ST_DROID } },
	{ "groupAddArea",		scrGroupAddArea,			VAL_VOID,
		6, { ST_GROUP, VAL_INT, VAL_INT,VAL_INT, VAL_INT,VAL_INT } },
	{ "groupAddAreaNoGroup",scrGroupAddArea,			VAL_VOID,
		6, { ST_GROUP, VAL_INT, VAL_INT,VAL_INT, VAL_INT,VAL_INT } },
	{ "groupAddGroup",		scrGroupAddGroup,			VAL_VOID,
		2, { ST_GROUP, ST_GROUP } },
	{ "groupMember",		scrGroupMember,				VAL_BOOL,
		2, { ST_GROUP, ST_DROID } },
	{ "idleGroup",			scrIdleGroup,				VAL_INT,
		1, { ST_GROUP } },
	{ "initIterateGroup",	scrInitIterateGroup,		VAL_VOID,
		1, { ST_GROUP } },
	{ "iterateGroup",		scrIterateGroup,			ST_DROID,
		1, { ST_GROUP } },
	{ "droidLeaveGroup",	scrDroidLeaveGroup,			VAL_VOID,
		1, { ST_DROID } },

	{ "orderDroid",			scrOrderDroid,				VAL_VOID,
		2, { ST_DROID, VAL_INT } },
	{ "orderDroidLoc",		scrOrderDroidLoc,			VAL_VOID,
		4, { ST_DROID, VAL_INT, VAL_INT, VAL_INT } },
	{ "orderDroidObj",		scrOrderDroidObj,			VAL_VOID,
		3, { ST_DROID, VAL_INT, ST_BASEOBJECT } },
	{ "orderDroidStatsLoc",	scrOrderDroidStatsLoc,		VAL_VOID,
		5, { ST_DROID, VAL_INT, ST_BASESTATS, VAL_INT, VAL_INT } },

	{ "orderGroup",			scrOrderGroup,				VAL_VOID,
		2, { ST_GROUP, VAL_INT } },
	{ "orderGroupLoc",		scrOrderGroupLoc,			VAL_VOID,
		4, { ST_GROUP, VAL_INT, VAL_INT, VAL_INT } },
	{ "orderGroupObj",		scrOrderGroupObj,			VAL_VOID,
		3, { ST_GROUP, VAL_INT, ST_BASEOBJECT } },

	{ "setDroidSecondary",	scrSetDroidSecondary,		VAL_VOID,
		3, { ST_DROID, VAL_INT, VAL_INT } },
	{ "setGroupSecondary",	scrSetGroupSecondary,		VAL_VOID,
		3, { ST_GROUP, VAL_INT, VAL_INT } },

	{ "initIterateCluster", scrInitIterateCluster,		VAL_VOID,
		1, { VAL_INT } },
	{ "iterateCluster",		scrIterateCluster,			ST_BASEOBJECT,
		0, { VAL_VOID } },
	{ "cmdDroidAddDroid",	scrCmdDroidAddDroid,		VAL_VOID,
		2, { ST_DROID, ST_DROID } },

	// a couple of example functions
	{ "numMessageBox",			scrNumMB,				VAL_VOID,
		1, { VAL_INT } },
	{ "debugBox",				scrNumMB,				VAL_VOID,
		1, { VAL_INT } },
	{ "approxRoot",				scrApproxRoot,			VAL_INT,
		2, { VAL_INT, VAL_INT } },
	{ "refTest",				scrRefTest,				VAL_VOID,
		1, { VAL_INT} },

	// geo funcs
	{ "distBetweenTwoPoints",	scrDistanceTwoPts,		VAL_INT,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "losTwoObjects",			scrLOSTwoBaseObjects,	VAL_BOOL,
		3, { ST_BASEOBJECT, ST_BASEOBJECT, VAL_BOOL } },

	{ "killStructsInArea",		scrDestroyStructuresInArea,	VAL_VOID,	
		8, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL} },

	{ "getThreatInArea",		scrThreatInArea,		VAL_INT,
		10, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_BOOL} },

	{ "getNearestGateway",		scrGetNearestGateway,	VAL_BOOL,	
		4, { VAL_INT, VAL_INT, VAL_REF|VAL_INT, VAL_REF|VAL_INT } },

	{ "setWaterTile",			scrSetWaterTile,		VAL_VOID,
		1, { VAL_INT } },

	{ "setRubbleTile",			scrSetRubbleTile,		VAL_VOID,
		1, { VAL_INT } },

	{ "setCampaignNumber",		scrSetCampaignNumber,	VAL_VOID,
		1, { VAL_INT } },

	{ "testStructureModule",	scrTestStructureModule, VAL_BOOL,
		3, {VAL_INT,ST_STRUCTURE,VAL_INT} },

	{ "killDroidsInArea",		scrDestroyUnitsInArea,	VAL_INT,
	5, {VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT} }, 

	{ "vanishUnit",				scrRemoveDroid,			VAL_VOID,
		1, {ST_DROID} },

	{ "forceDamageObject",		scrForceDamage,			VAL_VOID,
		2, {ST_BASEOBJECT,VAL_INT}},
	
	//multiplayer stuff.
	{ "isHumanPlayer",		scrIsHumanPlayer,			VAL_BOOL,	
	    1, { VAL_INT } },

	{ "offerAlliance",		scrOfferAlliance,			VAL_VOID,
		2, { VAL_INT, VAL_INT }},

	{ "createAlliance",		scrCreateAlliance,			VAL_VOID,
		2, { VAL_INT, VAL_INT }},

	{ "breakAlliance",		scrBreakAlliance,			VAL_VOID,
		2, { VAL_INT, VAL_INT }},

	{ "allianceExists",		scrAllianceExists,			VAL_BOOL,
		0, {VAL_VOID } },

	{ "allianceExistsBetween",scrAllianceExistsBetween,	VAL_BOOL,
		2, { VAL_INT, VAL_INT }},

	{ "playerInAlliance",	scrPlayerInAlliance,		VAL_BOOL,
		1, {VAL_INT } },

	{ "dominatingAlliance",	scrDominatingAlliance,		VAL_BOOL,
		0, {VAL_VOID } },

	{ "myResponsibility",	scrMyResponsibility,		VAL_BOOL,
		1, {VAL_INT } },

	// object conversion routines
	{ "objToDroid",			scrObjToDroid,				ST_DROID,
		1, { ST_BASEOBJECT } },
	{ "objToStructure",		scrObjToStructure,			ST_STRUCTURE,
		1, { ST_BASEOBJECT } },
	{ "objToFeature",		scrObjToFeature,			ST_FEATURE,
		1, { ST_BASEOBJECT } },


	{"getGameStatus", 		scrGetGameStatus,	  		VAL_BOOL,	1, {VAL_INT} },

    //player colour access functions
	{ "getPlayerColour",	scrGetPlayerColour,		    VAL_INT,	
        1, { VAL_INT } },
	
    { "setPlayerColour",	scrSetPlayerColour,		    VAL_VOID,
		2, { VAL_INT, VAL_INT } },

    { "takeOverDroidsInArea",	scrTakeOverDroidsInArea,	VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

    { "takeOverDroidsInAreaExp",scrTakeOverDroidsInAreaExp,	VAL_INT,
		8, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

    { "takeOverStructsInArea",	scrTakeOverStructsInArea,	VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

    { "takeOverSingleDroid",	scrTakeOverSingleDroid,	    ST_DROID,
		2, { ST_DROID, VAL_INT} },

    { "takeOverSingleStructure",    scrTakeOverSingleStructure,	    ST_STRUCTURE,
		2, { ST_STRUCTURE, VAL_INT} },

	{ "resetStructTargets",	scrResetStructTargets,			VAL_VOID,
		0, { VAL_VOID }, },
	{ "resetDroidTargets",	scrResetDroidTargets,			VAL_VOID,
		0, { VAL_VOID }, },
	{ "setStructTarPref",	scrSetStructTarPref,			VAL_VOID,
		1, { VAL_INT }, },
	{ "setStructTarIgnore",	scrSetStructTarIgnore,			VAL_VOID,
		1, { VAL_INT }, },
	{ "setDroidTarPref",	scrSetDroidTarPref,				VAL_VOID,
		1, { VAL_INT }, },
	{ "setDroidTarIgnore",	scrSetDroidTarIgnore,			VAL_VOID,
		1, { VAL_INT }, },
	{ "structTargetInArea",	scrStructTargetInArea,			ST_STRUCTURE,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, }, },
	{ "structTargetOnMap",	scrStructTargetOnMap,			ST_STRUCTURE,
		2, { VAL_INT, VAL_INT }, },
	{ "droidTargetInArea",	scrDroidTargetInArea,			ST_DROID,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, }, },
	{ "droidTargetOnMap",	scrDroidTargetOnMap,			ST_DROID,
		2, { VAL_INT, VAL_INT }, },
	{ "targetInCluster",	scrTargetInCluster,				ST_DROID,
		2, { VAL_INT, VAL_INT }, },
        
	{ "setDroidsToSafetyFlag",	scrSetDroidsToSafetyFlag,	VAL_VOID,
		1, { VAL_BOOL } },

	{ "setPlayCountDown",       scrSetPlayCountDown,	    VAL_VOID,
		1, { VAL_BOOL } },

	{ "getDroidCount",	        scrGetDroidCount,		    VAL_INT,	
        1, { VAL_INT } },

	{ "fireWeaponAtObj",		scrFireWeaponAtObj,			VAL_VOID,
		2, { ST_WEAPON, ST_BASEOBJECT } },
	{ "fireWeaponAtLoc",		scrFireWeaponAtLoc,			VAL_VOID,
		3, { ST_WEAPON, VAL_INT,VAL_INT } },

	{ "setDroidKills",			scrSetDroidKills,			VAL_VOID,
		2, { ST_DROID, VAL_INT } },
	{ "resetPlayerVisibility",	scrResetPlayerVisibility,	VAL_VOID,
		1, { VAL_INT } },

	{ "setVTOLReturnPos",		scrSetVTOLReturnPos,		VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT } },
		
	{ "isVtol",					scrIsVtol,					VAL_BOOL,
		1, { ST_DROID } },

	{ "tutorialTemplates",		scrTutorialTemplates,		VAL_VOID,
		0, { VAL_VOID } },

	{ "resetLimboMission",		scrResetLimboMission,		VAL_VOID,
		0, { VAL_VOID } },

//ajl skirmish funcs
	{ "skDoResearch",			scrSkDoResearch,			VAL_VOID,
		3, { ST_STRUCTURE, VAL_INT, VAL_INT } },

	{ "skLocateEnemy",			scrSkLocateEnemy,			ST_BASEOBJECT,
		1, { VAL_INT } },

	{ "skCanBuildTemplate",		scrSkCanBuildTemplate,		VAL_BOOL,
		3, { VAL_INT, ST_STRUCTURE, ST_TEMPLATE } },

	{ "skVtolEnableCheck",		scrSkVtolEnableCheck,		VAL_BOOL,
		1, { VAL_INT } },

	{ "skGetFactoryCapacity",	scrSkGetFactoryCapacity,	VAL_INT,
	1, { ST_STRUCTURE } },

	{ "skDifficultyModifier",	scrSkDifficultyModifier,	VAL_VOID,
	1, { VAL_INT } },

	{ "skDefenseLocation",		scrSkDefenseLocation,		VAL_BOOL,
	6, {VAL_REF|VAL_INT, VAL_REF|VAL_INT, ST_STRUCTURESTAT, ST_STRUCTURESTAT, ST_DROID,VAL_INT } },

	{ "skFireLassat",			scrSkFireLassat,			VAL_VOID,
	2, {VAL_INT, ST_BASEOBJECT } },


//	{ "skOrderDroidLineBuild",	scrSkOrderDroidLineBuild,	VAL_VOID,
//	6, { ST_DROID, ST_STRUCTURESTAT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

    /* This final entry marks the end of the function list */
	{ "FUNCTION LIST END",		NULL,					VAL_VOID,
		0, { VAL_VOID } }
};





#endif



/*
 * The table of external variables and their access functions.
 * The table is laid out as follows :
 *
 *   "variable_identifier", <variable type>, ST_EXTERN, 0, <index>,
 *		<get_function>, <set_function>
 *
 * The Storage type for an external variable is always ST_EXTERN.
 * The index is not used by the compiler but is passed to the access function
 * to allow one function to deal with a number of variables
 */
VAR_SYMBOL asExternTable[] =
{
	{ "isPSX",			VAL_BOOL,	ST_EXTERN,	0,		EXTID_ISPSX,
		scrGenExternGet,		NULL },

	{ "trackTransporter",VAL_BOOL,	ST_EXTERN,	0,		EXTID_TRACKTRANSPORTER,
		scrGenExternGet,		NULL },

	{ "mapWidth",		VAL_INT,	ST_EXTERN,	0,		EXTID_MAPWIDTH,
		scrGenExternGet,		NULL },
	{ "mapHeight",		VAL_INT,	ST_EXTERN,	0,		EXTID_MAPHEIGHT,
		scrGenExternGet,		NULL },

	{ "gameInitialised",VAL_BOOL,	ST_EXTERN,	0,		EXTID_GAMEINIT,
		scrGenExternGet,		NULL },

	{ "selectedPlayer",	VAL_INT,	ST_EXTERN,	0,		EXTID_SELECTEDPLAYER,
		scrGenExternGet,		NULL },
	{ "gameTime",		VAL_INT,	ST_EXTERN,	0,		EXTID_GAMETIME,
		scrGenExternGet,		NULL },

	{ "gameLevel",		VAL_INT,	ST_EXTERN,	0,		EXTID_GAMELEVEL,
		scrGenExternGet,		scrGenExternSet },
	{ "inTutorial",		VAL_BOOL,	ST_EXTERN,	0,		EXTID_TUTORIAL,
		scrGenExternGet,		scrGenExternSet },
	{ "cursorType",		VAL_INT,	ST_EXTERN,	0,		EXTID_CURSOR,
		scrGenExternGet, NULL },


	{ "intMode",		VAL_INT,	ST_EXTERN,	0,		EXTID_INTMODE,
		scrGenExternGet, NULL },

	{ "targetedObjectType",	VAL_INT,	ST_EXTERN,	0,		EXTID_TARGETTYPE,
		scrGenExternGet, NULL },
	{ "extraVictoryFlag",VAL_BOOL,	ST_EXTERN,	0,		EXTID_EXTRAVICTORYFLAG,
		scrGenExternGet,		scrGenExternSet },
	{ "extraFailFlag",	VAL_BOOL,	ST_EXTERN,	0,		EXTID_EXTRAFAILFLAG,
		scrGenExternGet,		scrGenExternSet },



#ifdef WIN32	
	{ "multiPlayerGameType",VAL_INT,	ST_EXTERN,	0,	EXTID_MULTIGAMETYPE,
		scrGenExternGet,			NULL },

	{ "multiPlayerMaxPlayers",VAL_INT,	ST_EXTERN,	0,EXTID_MULTIGAMEHUMANMAX,
		scrGenExternGet,			NULL },

	{ "multiPlayerBaseType",VAL_INT,	ST_EXTERN,	0,EXTID_MULTIGAMEBASETYPE,
		scrGenExternGet,			NULL },
#endif

		

	/* This entry marks the end of the variable list */
	{ NULL, VAL_VOID, ST_EXTERN, 0, 0, NULL, NULL }
};



/*
 * The table of object variables and their access functions.
 * The table is laid out as follows :
 *
 *   "variable_identifier", <variable type>, ST_OBJECT, <object type>, <index>,
 *		<get_function>, <set_function>
 *
 * The Storage type for an object variable is always ST_OBJECT.
 * The object type is the type of the object this is a member of.
 * The index is not used by the compiler but is passed to the access function
 * to allow one function to deal with a number of variables
 */
VAR_SYMBOL asObjTable[] =
{
	// base object variables
	{ "x",				VAL_INT,	ST_OBJECT,	ST_BASEOBJECT,	OBJID_POSX,
		scrBaseObjGet,			NULL },
	{ "y",				VAL_INT,	ST_OBJECT,	ST_BASEOBJECT,	OBJID_POSY,
		scrBaseObjGet,			NULL },
	{ "z",				VAL_INT,	ST_OBJECT,	ST_BASEOBJECT,	OBJID_POSZ,
		scrBaseObjGet,			NULL },
	{ "id",				VAL_INT,	ST_OBJECT,	ST_BASEOBJECT,	OBJID_ID,
		scrBaseObjGet,			NULL },
	{ "player",			VAL_INT,	ST_OBJECT,	ST_BASEOBJECT,	OBJID_PLAYER,
		scrBaseObjGet,			NULL },
	{ "type",			VAL_INT,	ST_OBJECT,	ST_BASEOBJECT,	OBJID_TYPE,
		scrBaseObjGet,			NULL },
	{ "clusterID",		VAL_INT,	ST_OBJECT,	ST_BASEOBJECT,	OBJID_CLUSTERID,
		scrBaseObjGet,			NULL },
	{ "health",			VAL_INT,	ST_OBJECT,	ST_BASEOBJECT,	OBJID_HEALTH,
		scrBaseObjGet,			NULL },

	// droid variables
	{ "order",			VAL_INT,	ST_OBJECT,	ST_DROID,		OBJID_ORDER,
		scrBaseObjGet,			NULL },
	{ "droidType",		VAL_INT,	ST_OBJECT,	ST_DROID,		OBJID_DROIDTYPE,
		scrBaseObjGet,			NULL },
	{ "body",			ST_BODY,	ST_OBJECT,	ST_DROID,		OBJID_BODY,
		scrBaseObjGet,			NULL },
	{ "propulsion",		ST_PROPULSION,	ST_OBJECT,	ST_DROID,		OBJID_PROPULSION,
		scrBaseObjGet,			NULL },
	{ "weapon",			ST_WEAPON,	ST_OBJECT,	ST_DROID,		OBJID_WEAPON,
		scrBaseObjGet,			NULL },
	{ "orderx",			VAL_INT,	ST_OBJECT,	ST_DROID,		OBJID_ORDERX,
		scrBaseObjGet,			NULL },
	{ "ordery",			VAL_INT,	ST_OBJECT,	ST_DROID,		OBJID_ORDERY,
		scrBaseObjGet,			NULL },

	// structure variables
	{ "stat",			ST_STRUCTURESTAT,	ST_OBJECT,	ST_STRUCTURE,	OBJID_STRUCTSTAT,
		scrBaseObjGet,			NULL },

	// group variables
	{ "x",				VAL_INT,	ST_OBJECT,	ST_GROUP,		GROUPID_POSX,
		scrGroupObjGet,			NULL },
	{ "y",				VAL_INT,	ST_OBJECT,	ST_GROUP,		GROUPID_POSY,
		scrGroupObjGet,			NULL },
	{ "members",		VAL_INT,	ST_OBJECT,	ST_GROUP,		GROUPID_MEMBERS,
		scrGroupObjGet,			NULL },
	{ "health",			VAL_INT,	ST_OBJECT,	ST_GROUP,		GROUPID_HEALTH,
		scrGroupObjGet,			NULL },



	/* This entry marks the end of the variable list */
	{ NULL, VAL_VOID, ST_OBJECT, VAL_VOID, 0, NULL, NULL }
};

#ifndef NOSCRIPT
/* The table of constant variables
 * The format is :
 *
 *	"variable name", <variable type>, <bool value>, <int value>,
 *						<object pointer value>
 * 
 * Only the value corresponding to the type should be set, all other values
 * should be 0.
 *
 * Any user-type constants should use the object pointer value.
 */
CONST_SYMBOL asConstantTable[] =
{
/*	{ "TEST_BOOL_CONST",	VAL_BOOL, TRUE,		0,		0 },
	{ "TEST_INT_CONST",		VAL_INT,	0,		10,		0 },*/

	//reticule button IDs	- for scrFlashOn & Off
	// original annette styley
	{ "OPTIONS",	VAL_INT,	0,		IDRET_OPTIONS,		0 },
	{ "CANCEL",		VAL_INT,	0,		IDRET_CANCEL,		0 },
	{ "BUILD",		VAL_INT,	0,		IDRET_BUILD,		0 },
	{ "MANUFACTURE",VAL_INT,	0,		IDRET_MANUFACTURE,	0 },
	{ "RESEARCH",	VAL_INT,	0,		IDRET_RESEARCH,		0 },
	{ "INTELMAP",	VAL_INT,	0,		IDRET_INTEL_MAP,	0 },
	{ "DESIGN",		VAL_INT,	0,		IDRET_DESIGN,		0 },
	{ "COMMAND",	VAL_INT,	0,		IDRET_COMMAND,		0 },

	// new styley that supports many other buttons
	{ "IDRET_OPTIONS",		VAL_INT,	0,		IDRET_OPTIONS,		0 },
	{ "IDRET_CANCEL",		VAL_INT,	0,		IDRET_CANCEL,		0 },
	{ "IDRET_BUILD",		VAL_INT,	0,		IDRET_BUILD,		0 },
	{ "IDRET_MANUFACTURE",	VAL_INT,	0,		IDRET_MANUFACTURE,	0 },
	{ "IDRET_RESEARCH",		VAL_INT,	0,		IDRET_RESEARCH,		0 },
	{ "IDRET_INTELMAP",		VAL_INT,	0,		IDRET_INTEL_MAP,	0 },
	{ "IDRET_DESIGN",		VAL_INT,	0,		IDRET_DESIGN,		0 },
	{ "IDRET_COMMAND",		VAL_INT,	0,		IDRET_COMMAND,		0 },
	{ "IDRET_ORDER",		VAL_INT,	0,		IDRET_ORDER,		0 },
	{ "IDRET_TRANSPORTER",	VAL_INT,	0,		IDRET_TRANSPORTER,	0 },

	    // Design screen buttons
	{ "IDDES_TEMPLSTART",	VAL_INT,	0,		IDDES_TEMPLSTART,	0 },
	{ "IDDES_SYSTEMBUTTON",	VAL_INT,	0,		IDDES_SYSTEMBUTTON,	0 },
	{ "IDDES_BODYBUTTON",	VAL_INT,	0,		IDDES_BODYBUTTON,	0 },
	{ "IDDES_PROPBUTTON",	VAL_INT,	0,		IDDES_PROPBUTTON,	0 },

	// the first (top-left)  button on the object window (right of the reticule window)
	{ "IDOBJ_STATSTART",	VAL_INT,	0,		IDOBJ_STATSTART,	0 },

	// one below obj_statstart
	{ "IDOBJ_OBJSTART",		VAL_INT,	0,		IDOBJ_OBJSTART,	0 },

	// the first (top-left) button on the list window (up from the reticule)
	{ "IDSTAT_START",		VAL_INT,	0,		IDSTAT_START,	0 },


	//message Types
	{ "RES_MSG",	VAL_INT,	0,		MSG_RESEARCH,		0},
	{ "CAMP_MSG",	VAL_INT,	0,		MSG_CAMPAIGN,		0},
	{ "MISS_MSG",	VAL_INT,	0,		MSG_MISSION,		0},
	{ "PROX_MSG",	VAL_INT,	0,		MSG_PROXIMITY,		0},
	//{ "TUT_MSG",	VAL_INT,	0,		MSG_TUTORIAL,		0}, NOT NEEDED

	//used for null pointers
	{ "NULLTEMPLATE",	ST_POINTER_T,	0,		0,					NULL},
	{ "NULLOBJECT",		ST_POINTER_O,	0,		0,					NULL},
	{ "NULLSTAT",		ST_POINTER_S,	0,		0,					NULL},
	{ "NULLSTRING",		ST_TEXTSTRING,	0,		0,					NULL},

	//barbarian player ids
	{ "BARBARIAN1",	VAL_INT,	0,		BARB1,				0},
	{ "BARBARIAN2",	VAL_INT,	0,		BARB2,				0},

    //#define used to set the reinforcement timer with
	{ "LZ_COMPROMISED_TIME",	VAL_INT,    0,	SCR_LZ_COMPROMISED_TIME,		0 },

    // BASEOBJ types
	{ "OBJ_DROID",		VAL_INT,	0,		OBJ_DROID,				0},
	{ "OBJ_STRUCTURE",	VAL_INT,	0,		OBJ_STRUCTURE,			0},
	{ "OBJ_FEATURE",	VAL_INT,	0,		OBJ_FEATURE,			0},
	//mission Types
	{ "CAMP_START",	VAL_INT,	0,		LDS_CAMSTART,		0},
#ifndef COVERMOUNT
	{ "CAMP_EXPAND",VAL_INT,	0,		LDS_EXPAND,		0},
#endif
	{ "OFF_KEEP",	VAL_INT,	0,		LDS_MKEEP,			0},
#ifndef COVERMOUNT
	{ "OFF_CLEAR",	VAL_INT,	0,		LDS_MCLEAR,			0},
#endif
	{ "BETWEEN",	VAL_INT,	0,		LDS_BETWEEN,		0},
	// droid types
	{ "DROID_WEAPON",		VAL_INT,	0,		DROID_WEAPON,			0 },
	{ "DROID_SENSOR",		VAL_INT,	0,		DROID_SENSOR,			0 },
	{ "DROID_ECM",			VAL_INT,	0,		DROID_ECM,				0 },
	{ "DROID_CONSTRUCT",	VAL_INT,	0,		DROID_CONSTRUCT,		0 },
	{ "DROID_PERSON",		VAL_INT,	0,		DROID_PERSON,			0 },
	{ "DROID_CYBORG",		VAL_INT,	0,		DROID_CYBORG,			0 },
	{ "DROID_TRANSPORTER",	VAL_INT,	0,		DROID_TRANSPORTER,		0 },
	{ "DROID_COMMAND",		VAL_INT,	0,		DROID_COMMAND,			0 },
	{ "DROID_REPAIR",		VAL_INT,	0,		DROID_REPAIR,			0 },

	// structure types
	{ "REF_HQ",				VAL_INT,	0,		REF_HQ,					0 },
	{ "REF_FACTORY",		VAL_INT,	0,		REF_FACTORY,			0 },
	{ "REF_FACTORY_MODULE",	VAL_INT,	0,		REF_FACTORY_MODULE,		0 },
	{ "REF_POWER_GEN",		VAL_INT,	0,		REF_POWER_GEN,			0 },
	{ "REF_POWER_MODULE",	VAL_INT,	0,		REF_POWER_MODULE,		0 },
	{ "REF_RESOURCE_EXTRACTOR",	VAL_INT,	0,		REF_RESOURCE_EXTRACTOR,	0 },
	{ "REF_DEFENSE",		VAL_INT,	0,		REF_DEFENSE,			0 },
	{ "REF_WALL",			VAL_INT,	0,		REF_WALL,				0 },
	{ "REF_WALLCORNER",		VAL_INT,	0,		REF_WALLCORNER,			0 },
	{ "REF_RESEARCH",		VAL_INT,	0,		REF_RESEARCH,			0 },
	{ "REF_RESEARCH_MODULE",	VAL_INT,	0,		REF_RESEARCH_MODULE,	0 },
	{ "REF_REPAIR_FACILITY",	VAL_INT,	0,		REF_REPAIR_FACILITY,	0 },
	{ "REF_COMMAND_CONTROL",	VAL_INT,	0,		REF_COMMAND_CONTROL,	0 },
	{ "REF_CYBORG_FACTORY",		VAL_INT,	0,		REF_CYBORG_FACTORY,		0 },
	{ "REF_VTOL_FACTORY",		VAL_INT,	0,		REF_VTOL_FACTORY,		0 },
	{ "REF_REARM_PAD",		    VAL_INT,	0,		REF_REARM_PAD,		    0 },
	{ "REF_MISSILE_SILO",		VAL_INT,	0,		REF_MISSILE_SILO,		0 },


	// primary orders
	{ "DORDER_NONE",		VAL_INT,	0,		DORDER_NONE,			0 },
	{ "DORDER_STOP",		VAL_INT,	0,		DORDER_STOP,			0 },
	{ "DORDER_MOVE",		VAL_INT,	0,		DORDER_MOVE,			0 },
	{ "DORDER_ATTACK",		VAL_INT,	0,		DORDER_ATTACK,			0 },
	{ "DORDER_BUILD",		VAL_INT,	0,		DORDER_BUILD,			0 },
	{ "DORDER_HELPBUILD",	VAL_INT,	0,		DORDER_HELPBUILD,		0 },
	{ "DORDER_LINEBUILD",	VAL_INT,	0,		DORDER_LINEBUILD,		0 },
	{ "DORDER_DEMOLISH",	VAL_INT,	0,		DORDER_DEMOLISH,		0 },
	{ "DORDER_REPAIR",		VAL_INT,	0,		DORDER_REPAIR,			0 },
	{ "DORDER_OBSERVE",		VAL_INT,	0,		DORDER_OBSERVE,			0 },
	{ "DORDER_FIRESUPPORT",	VAL_INT,	0,		DORDER_FIRESUPPORT,		0 },
	{ "DORDER_RETREAT",		VAL_INT,	0,		DORDER_RETREAT,			0 },
	{ "DORDER_DESTRUCT",	VAL_INT,	0,		DORDER_DESTRUCT,		0 },
	{ "DORDER_RTB",			VAL_INT,	0,		DORDER_RTB,				0 },
	{ "DORDER_RTR",			VAL_INT,	0,		DORDER_RTR,				0 },
	{ "DORDER_RUN",			VAL_INT,	0,		DORDER_RUN,				0 },
	{ "DORDER_EMBARK",		VAL_INT,	0,		DORDER_EMBARK,			0 },
	{ "DORDER_DISEMBARK",	VAL_INT,	0,		DORDER_DISEMBARK,		0 },
	{ "DORDER_SCOUT",		VAL_INT,	0,		DORDER_SCOUT,			0 },

	// secondary orders
	{ "DSO_ATTACK_RANGE",	VAL_INT,	0,		DSO_ATTACK_RANGE,		0 },
	{ "DSO_REPAIR_LEVEL",	VAL_INT,	0,		DSO_REPAIR_LEVEL,		0 },
	{ "DSO_ATTACK_LEVEL",	VAL_INT,	0,		DSO_ATTACK_LEVEL,		0 },
	{ "DSO_RECYCLE",		VAL_INT,	0,		DSO_RECYCLE,			0 },
	{ "DSO_PATROL",			VAL_INT,	0,		DSO_PATROL,				0 },
	{ "DSO_HALTTYPE",		VAL_INT,	0,		DSO_HALTTYPE,			0 },
	{ "DSO_RETURN_TO_LOC",	VAL_INT,	0,		DSO_RETURN_TO_LOC,		0 },

	// secondary order stats
	{ "DSS_ARANGE_SHORT",	VAL_INT,	0,		DSS_ARANGE_SHORT,		0 },
	{ "DSS_ARANGE_LONG",	VAL_INT,	0,		DSS_ARANGE_LONG,		0 },
	{ "DSS_ARANGE_DEFAULT",	VAL_INT,	0,		DSS_ARANGE_DEFAULT,		0 },
	{ "DSS_REPLEV_LOW",		VAL_INT,	0,		DSS_REPLEV_LOW,			0 },
	{ "DSS_REPLEV_HIGH",	VAL_INT,	0,		DSS_REPLEV_HIGH,		0 },
	{ "DSS_REPLEV_NEVER",	VAL_INT,	0,		DSS_REPLEV_NEVER,		0 },
	{ "DSS_ALEV_ALWAYS",	VAL_INT,	0,		DSS_ALEV_ALWAYS,		0 },
	{ "DSS_ALEV_ATTACKED",	VAL_INT,	0,		DSS_ALEV_ATTACKED,		0 },
	{ "DSS_ALEV_NEVER",		VAL_INT,	0,		DSS_ALEV_NEVER,			0 },
	{ "DSS_HALT_HOLD",		VAL_INT,	0,		DSS_HALT_HOLD,			0 },
	{ "DSS_HALT_GUARD",		VAL_INT,	0,		DSS_HALT_GUARD,			0 },
	{ "DSS_HALT_PERSUE",	VAL_INT,	0,		DSS_HALT_PERSUE,		0 },
	{ "DSS_RECYCLE_SET",	VAL_INT,	0,		DSS_RECYCLE_SET,		0 },
	{ "DSS_ASSPROD_START",	VAL_INT,	0,		DSS_ASSPROD_START,		0 },
	{ "DSS_ASSPROD_END ",	VAL_INT,	0,		DSS_ASSPROD_END ,		0 },
	{ "DSS_RTL_REPAIR",		VAL_INT,	0,		DSS_RTL_REPAIR,			0 },
	{ "DSS_RTL_BASE",		VAL_INT,	0,		DSS_RTL_BASE,			0 },
	{ "DSS_RTL_TRANSPORT",	VAL_INT,	0,		DSS_RTL_TRANSPORT,		0 },
	{ "DSS_PATROL_SET",		VAL_INT,	0,		DSS_PATROL_SET,			0 },

	// button id's
	{ "IDRET_OPTIONS",		VAL_INT,	0,		IDRET_OPTIONS,			0 },
	{ "IDRET_BUILD",		VAL_INT,	0,		IDRET_BUILD,			0 },
	{ "IDRET_MANUFACTURE",	VAL_INT,	0,		IDRET_MANUFACTURE,		0 },
	{ "IDRET_RESEARCH",		VAL_INT,	0,		IDRET_RESEARCH,			0 },
	{ "IDRET_INTELMAP",		VAL_INT,	0,		IDRET_INTEL_MAP,		0 },
	{ "IDRET_DESIGN",		VAL_INT,	0,		IDRET_DESIGN,			0 },
	{ "IDRET_CANCEL",		VAL_INT,	0,		IDRET_CANCEL,			0 },
	{ "IDRET_COMMAND",		VAL_INT,	0,		IDRET_COMMAND,			0 },

	// Cursor types
	{ "IMAGE_CURSOR_SELECT", VAL_INT,	0,		IMAGE_CURSOR_SELECT,	0 },
	{ "IMAGE_CURSOR_ATTACK", VAL_INT,	0,  	IMAGE_CURSOR_ATTACK,	0 },
	{ "IMAGE_CURSOR_MOVE",	 VAL_INT,	0,		IMAGE_CURSOR_MOVE,		0 },
#ifdef WIN32
	{ "IMAGE_CURSOR_ECM",	 VAL_INT,	0, 		IMAGE_CURSOR_ECM,		0 },
	{ "IMAGE_CURSOR_REPAIR", VAL_INT,	0,		IMAGE_CURSOR_REPAIR, 0 },
#else
	{ "IMAGE_CURSOR_ECM",	 VAL_INT,	0, 		IMAGE_CURSOR_JAM,		0 },
	{ "IMAGE_CURSOR_REPAIR", VAL_INT,	0,		IMAGE_CURSOR_SEEKREPAIR, 0 },
#endif
	{ "IMAGE_CURSOR_PICKUP",  VAL_INT,	0, 		IMAGE_CURSOR_PICKUP,	0 },
	{ "IMAGE_CURSOR_DEFAULT", VAL_INT,	0, 		IMAGE_CURSOR_DEFAULT,	0 },

	{ "IMAGE_CURSOR_BUILD",	   VAL_INT,	0,		IMAGE_CURSOR_BUILD,		0 },
	{ "IMAGE_CURSOR_GUARD",	  VAL_INT,	0,	 	IMAGE_CURSOR_GUARD,		0 },
	{ "IMAGE_CURSOR_BRIDGE",  VAL_INT,	0,	 	IMAGE_CURSOR_BRIDGE,	0 },
	{ "IMAGE_CURSOR_ATTACH",  VAL_INT,	0,	 	IMAGE_CURSOR_ATTACH,	0 },
	{ "IMAGE_CURSOR_LOCKON",  VAL_INT,	0,	 	IMAGE_CURSOR_LOCKON,	0 },
	{ "IMAGE_CURSOR_FIX",	  VAL_INT,	0,		IMAGE_CURSOR_FIX,	    0 },
	{ "IMAGE_CURSOR_EMBARK",  VAL_INT,	0,	 	IMAGE_CURSOR_EMBARK, 	0 },

	// game mode types  (possible values for intMode)
	{ "INT_NORMAL",			VAL_INT,	0,		INT_NORMAL,	0 },		// Standard mode (just the reticule)
#ifdef WIN32
	{ "INT_OPTION",			VAL_INT,	0,		INT_OPTION,0 },	// Option screen
#endif
	{ "INT_EDITSTAT",		VAL_INT,	0,		INT_EDITSTAT,0 },	// Stat screen up for placing objects
#ifdef WIN32
	{ "INT_EDIT",			VAL_INT,	0,		INT_EDIT,0 },		// Edit mode
#endif
	{ "INT_OBJECT",			VAL_INT,	0,		INT_OBJECT,0 },	// Object screen
	{ "INT_STAT",			VAL_INT,	0,		INT_STAT,0 },		// Object screen with stat screen
	{ "INT_CMDORDER",		VAL_INT,	0,		INT_CMDORDER,0 },	// Object screen with command droids and orders screen
	{ "INT_DESIGN",			VAL_INT,	0,		INT_DESIGN,0 },		// Design screen
	{ "INT_INTELMAP",		VAL_INT,	0,		INT_INTELMAP,0 },	// Intelligence Map
	{ "INT_ORDER",			VAL_INT,	0,		INT_ORDER,0 },
	{ "INT_INGAMEOP",		VAL_INT,	0,		INT_INGAMEOP,0 },	// in game options.
	{ "INT_TRANSPORTER",	VAL_INT,	0,		INT_TRANSPORTER,0 }, //Loading/unloading a Transporter
	{ "INT_MISSIONRES",		VAL_INT,	0,		INT_MISSIONRES,0 },	// Results of a mission display.
	{ "INT_MULTIMENU",		VAL_INT,	0,		INT_MULTIMENU,0 },	// multiplayer only, player stats etc...

	// parameters for getGameStatus
	{ "STATUS_ReticuleIsOpen",		VAL_INT,	0, STATUS_ReticuleIsOpen,0 },		// is the reticule current being displayed (TRUE=yes)
	{ "STATUS_BattleMapViewEnabled",VAL_INT,	0, STATUS_BattleMapViewEnabled,0 },	// Are we currently in the battlemap view (tactical display) true=yes
	{ "STATUS_DeliveryReposInProgress",VAL_INT,0, STATUS_DeliveryReposInProgress,0 },	// Are we currently in the delivery repos mode


	//possible values for externed 	targetedObjectType
	{"MT_TERRAIN",VAL_INT,0,MT_TERRAIN,0},
	{"MT_RESOURCE",VAL_INT,0,MT_RESOURCE,0},
	{"MT_BLOCKING",VAL_INT,0,MT_BLOCKING,0},
	{"MT_RIVER",VAL_INT,0,MT_RIVER,0},
	{"MT_TRENCH",VAL_INT,0,MT_TRENCH,0},
	{"MT_OWNSTRDAM",VAL_INT,0,MT_OWNSTRDAM,0},
	{"MT_OWNSTROK",VAL_INT,0,MT_OWNSTROK,0},
	{"MT_OWNSTRINCOMP",VAL_INT,0,MT_OWNSTRINCOMP,0},
	{"MT_REPAIR",VAL_INT,0,MT_REPAIR,0},
	{"MT_REPAIRDAM",VAL_INT,0,MT_REPAIRDAM,0},
	{"MT_ENEMYSTR",VAL_INT,0,MT_ENEMYSTR,0},
	{"MT_TRANDROID",VAL_INT,0,MT_TRANDROID,0},
	{"MT_OWNDROID",VAL_INT,0,MT_OWNDROID,0},
	{"MT_OWNDROIDDAM",VAL_INT,0,MT_OWNDROIDDAM,0},
	{"MT_ENEMYDROID",VAL_INT,0,MT_ENEMYDROID,0},
	{"MT_COMMAND",VAL_INT,0,MT_COMMAND,0},
	{"MT_ARTIFACT",VAL_INT,0,MT_ARTIFACT,0},
	{"MT_DAMFEATURE",VAL_INT,0,MT_DAMFEATURE,0},
	{"MT_SENSOR",VAL_INT,0,MT_SENSOR,0},

	// structure target types
	{ "ST_HQ",					VAL_INT,	0,	SCR_ST_HQ,					0 },
	{ "ST_FACTORY",				VAL_INT,	0,	SCR_ST_FACTORY,				0 },
	{ "ST_POWER_GEN",			VAL_INT,	0,	SCR_ST_POWER_GEN,			0 },
	{ "ST_RESOURCE_EXTRACTOR",	VAL_INT,	0,	SCR_ST_RESOURCE_EXTRACTOR,	0 },
	{ "ST_WALL",				VAL_INT,	0,	SCR_ST_WALL,				0 },
	{ "ST_RESEARCH",			VAL_INT,	0,	SCR_ST_RESEARCH,			0 },
	{ "ST_REPAIR_FACILITY",		VAL_INT,	0,	SCR_ST_REPAIR_FACILITY,		0 },
	{ "ST_COMMAND_CONTROL",		VAL_INT,	0,	SCR_ST_COMMAND_CONTROL,		0 },
	{ "ST_CYBORG_FACTORY",		VAL_INT,	0,	SCR_ST_CYBORG_FACTORY,		0 },
	{ "ST_VTOL_FACTORY",		VAL_INT,	0,	SCR_ST_VTOL_FACTORY,		0 },
	{ "ST_REARM_PAD",			VAL_INT,	0,	SCR_ST_REARM_PAD,			0 },
	{ "ST_SENSOR",				VAL_INT,	0,	SCR_ST_SENSOR,				0 },
	{ "ST_DEF_GROUND",			VAL_INT,	0,	SCR_ST_DEF_GROUND,			0 },
	{ "ST_DEF_AIR",				VAL_INT,	0,	SCR_ST_DEF_AIR,				0 },
	{ "ST_DEF_IDF",				VAL_INT,	0,	SCR_ST_DEF_IDF,				0 },
	{ "ST_DEF_ALL",				VAL_INT,	0,	SCR_ST_DEF_ALL,				0 },

	// droid target types
	{ "DT_COMMAND",		VAL_INT,	0,	SCR_DT_COMMAND,		0 },
	{ "DT_SENSOR",		VAL_INT,	0,	SCR_DT_SENSOR,		0 },
	{ "DT_CONSTRUCT",	VAL_INT,	0,	SCR_DT_CONSTRUCT,	0 },
	{ "DT_REPAIR",		VAL_INT,	0,	SCR_DT_REPAIR,		0 },
	{ "DT_WEAP_GROUND",	VAL_INT,	0,	SCR_DT_WEAP_GROUND,	0 },
	{ "DT_WEAP_AIR",	VAL_INT,	0,	SCR_DT_WEAP_AIR,	0 },
	{ "DT_WEAP_IDF",	VAL_INT,	0,	SCR_DT_WEAP_IDF,	0 },
	{ "DT_WEAP_ALL",	VAL_INT,	0,	SCR_DT_WEAP_ALL,	0 },
	{ "DT_LIGHT",		VAL_INT,	0,	SCR_DT_LIGHT,		0 },
	{ "DT_MEDIUM",		VAL_INT,	0,	SCR_DT_MEDIUM,		0 },
	{ "DT_HEAVY",		VAL_INT,	0,	SCR_DT_HEAVY,		0 },
	{ "DT_SUPER_HEAVY",	VAL_INT,	0,	SCR_DT_SUPER_HEAVY,	0 },
	{ "DT_TRACK",		VAL_INT,	0,	SCR_DT_TRACK,		0 },
	{ "DT_HTRACK",		VAL_INT,	0,	SCR_DT_HTRACK,		0 },
	{ "DT_WHEEL",		VAL_INT,	0,	SCR_DT_WHEEL,		0 },
	{ "DT_LEGS",		VAL_INT,	0,	SCR_DT_LEGS,		0 },
	{ "DT_GROUND",		VAL_INT,	0,	SCR_DT_GROUND,		0 },
	{ "DT_VTOL",		VAL_INT,	0,	SCR_DT_VTOL,		0 },
	{ "DT_HOVER",		VAL_INT,	0,	SCR_DT_HOVER,		0 },

	// multiplayer
#ifdef WIN32		
//	{ "DMATCH",				VAL_INT,	0,		DMATCH,					0 },
	{ "CAMPAIGN",			VAL_INT,	0,		CAMPAIGN,				0 },
	{ "TEAMPLAY",			VAL_INT,	0,		TEAMPLAY,				0 },
	{ "SKIRMISH",			VAL_INT,	0,		SKIRMISH,				0 },

	{ "CAMP_CLEAN",			VAL_INT,	0,		CAMP_CLEAN,				0 },
	{ "CAMP_BASE",			VAL_INT,	0,		CAMP_BASE,				0 },
	{ "CAMP_WALLS",			VAL_INT,	0,		CAMP_WALLS,				0 },
#endif


	/* This entry marks the end of the constant list */
	{ "CONSTANT LIST END",	VAL_VOID }
};
#endif

/* The Table of callback triggers
 * The format is :
 *
 * "callback name", <callback id>
 *
 * The callback id should be a unique id number starting at TR_CALLBACKSTART
 * and increasing sequentially by 1
 */
CALLBACK_SYMBOL asCallbackTable[] =
{
	{ "CALL_GAMEINIT",				CALL_GAMEINIT,				NULL, 0 },
	{ "CALL_DELIVPOINTMOVED",		CALL_DELIVPOINTMOVED,		NULL, 0 },
	{ "CALL_DROIDDESIGNED",			CALL_DROIDDESIGNED,			NULL, 0 },
	//{ "CALL_RESEARCHCOMPLETED",		CALL_RESEARCHCOMPLETED,		NULL, 0 }, - callback function added
	{ "CALL_DROIDBUILT",			CALL_DROIDBUILT,			NULL, 0 },
	{ "CALL_POWERGEN_BUILT",		CALL_POWERGEN_BUILT,		NULL, 0 },
	{ "CALL_RESEX_BUILT",			CALL_RESEX_BUILT,			NULL, 0 },
	{ "CALL_RESEARCH_BUILT",		CALL_RESEARCH_BUILT,		NULL, 0 },
	{ "CALL_FACTORY_BUILT",			CALL_FACTORY_BUILT,			NULL, 0 },
	{ "CALL_MISSION_START",			CALL_MISSION_START,			NULL, 0 },
	{ "CALL_MISSION_END",			CALL_MISSION_END,			NULL, 0 },
	{ "CALL_VIDEO_QUIT",			CALL_VIDEO_QUIT,			NULL, 0 },
	{ "CALL_LAUNCH_TRANSPORTER",	CALL_LAUNCH_TRANSPORTER,	NULL, 0 },
	{ "CALL_START_NEXT_LEVEL",		CALL_START_NEXT_LEVEL,		NULL, 0 },
	{ "CALL_TRANSPORTER_REINFORCE",	CALL_TRANSPORTER_REINFORCE,	NULL, 0 },
	{ "CALL_MISSION_TIME",			CALL_MISSION_TIME,			NULL, 0 },
	{ "CALL_ELECTRONIC_TAKEOVER",	CALL_ELECTRONIC_TAKEOVER,	NULL, 0 },

	// tutorial only callbacks
	{ "CALL_BUILDLIST",				CALL_BUILDLIST,				NULL, 0 },
	{ "CALL_BUILDGRID",				CALL_BUILDGRID,				NULL, 0 },
	{ "CALL_RESEARCHLIST",			CALL_RESEARCHLIST,			NULL, 0 },
	{ "CALL_MANURUN",				CALL_MANURUN,				NULL, 0 },
	{ "CALL_MANULIST",				CALL_MANULIST,				NULL, 0 },
	{ "CALL_BUTTON_PRESSED",		CALL_BUTTON_PRESSED,		scrCBButtonPressed,
		1,	{ VAL_INT } },
	{ "CALL_DROID_SELECTED",		CALL_DROID_SELECTED,		scrCBDroidSelected,
		1,	{ VAL_REF|ST_DROID } },
	{ "CALL_DESIGN_QUIT",			CALL_DESIGN_QUIT,			NULL, 0 },
	{ "CALL_DESIGN_WEAPON",			CALL_DESIGN_WEAPON,			NULL, 0 },
	{ "CALL_DESIGN_SYSTEM",			CALL_DESIGN_SYSTEM,			NULL, 0 },
	{ "CALL_DESIGN_COMMAND",		CALL_DESIGN_COMMAND,		NULL, 0 },
	{ "CALL_DESIGN_BODY",			CALL_DESIGN_BODY,			NULL, 0 },
	{ "CALL_DESIGN_PROPULSION",		CALL_DESIGN_PROPULSION,		NULL, 0 },

	// callback triggers with parameters
    { "CALL_RESEARCHCOMPLETED",     CALL_RESEARCHCOMPLETED, scrCBResCompleted,
		1,	{ VAL_REF|ST_RESEARCH } },

	{ "CALL_NEWDROID",				CALL_NEWDROID,			scrCBNewDroid,
		3,	{ VAL_INT, VAL_REF|ST_DROID,VAL_REF|ST_STRUCTURE	   } },

	{ "CALL_STRUCT_ATTACKED",		CALL_STRUCT_ATTACKED,	scrCBStructAttacked,
		3,	{ VAL_INT, VAL_REF|ST_STRUCTURE, VAL_REF|ST_BASEOBJECT } },

	{ "CALL_DROID_ATTACKED",		CALL_DROID_ATTACKED,	scrCBDroidAttacked,
		3,	{ VAL_INT, VAL_REF|ST_DROID, VAL_REF|ST_BASEOBJECT } },

	{ "CALL_ATTACKED",				CALL_ATTACKED,			scrCBAttacked,
		3,	{ VAL_INT, VAL_REF|ST_BASEOBJECT, VAL_REF|ST_BASEOBJECT } },

	{ "CALL_STRUCT_SEEN",			CALL_STRUCT_SEEN,		scrCBStructSeen,
		3,	{ VAL_INT, VAL_REF|ST_STRUCTURE, VAL_REF|ST_BASEOBJECT } },

	{ "CALL_DROID_SEEN",			CALL_DROID_SEEN,		scrCBDroidSeen,
		3,	{ VAL_INT, VAL_REF|ST_DROID, VAL_REF|ST_BASEOBJECT } },

	{ "CALL_FEATURE_SEEN",			CALL_FEATURE_SEEN,		scrCBFeatureSeen,
		3,	{ VAL_INT, VAL_REF|ST_FEATURE, VAL_REF|ST_BASEOBJECT } },

	{ "CALL_OBJ_SEEN",				CALL_OBJ_SEEN,			scrCBObjSeen,
		3,	{ VAL_INT, VAL_REF|ST_BASEOBJECT, VAL_REF|ST_BASEOBJECT } },

	{ "CALL_OBJ_DESTROYED",			CALL_OBJ_DESTROYED,		scrCBObjDestroyed,
		2,	{ VAL_INT, VAL_REF|ST_BASEOBJECT } },

	{ "CALL_STRUCT_DESTROYED",		CALL_STRUCT_DESTROYED,	scrCBStructDestroyed,
		2,	{ VAL_INT, VAL_REF|ST_STRUCTURE } },

	{ "CALL_DROID_DESTROYED",		CALL_DROID_DESTROYED,	scrCBDroidDestroyed,
		2,	{ VAL_INT, VAL_REF|ST_DROID } },

	{ "CALL_FEATURE_DESTROYED",		CALL_FEATURE_DESTROYED,	scrCBFeatureDestroyed,
		1,	{ VAL_REF|ST_FEATURE } },

	{ "CALL_OBJECTOPEN",		CALL_OBJECTOPEN, 	NULL, 0 },
	{ "CALL_OBJECTCLOSE",		CALL_OBJECTCLOSE, 	NULL, 0 },

	{ "CALL_TRANSPORTER_OFFMAP",	CALL_TRANSPORTER_OFFMAP,	scrCBTransporterOffMap,
		1,	{ VAL_INT } },
	{ "CALL_TRANSPORTER_LANDED",	CALL_TRANSPORTER_LANDED,	scrCBTransporterLanded,
		2,	{ ST_GROUP, VAL_INT } },

	{ "CALL_ALL_ONSCREEN_DROIDS_SELECTED",		CALL_ALL_ONSCREEN_DROIDS_SELECTED, 	NULL, 0 },

    { "CALL_NO_REINFORCEMENTS_LEFT",    CALL_NO_REINFORCEMENTS_LEFT,    NULL, 0 },

	{ "CALL_CLUSTER_EMPTY",				CALL_CLUSTER_EMPTY,		scrCBClusterEmpty,
		1,	{ VAL_REF|VAL_INT } },

	{ "CALL_VTOL_OFF_MAP",				CALL_VTOL_OFF_MAP,		scrCBVtolOffMap,
		2,  { VAL_INT, VAL_REF|ST_DROID } },

	{ "CALL_UNITTAKEOVER",			CALL_UNITTAKEOVER,		scrCBDroidTaken,
		1,	{VAL_REF|ST_DROID} },

	{ "CALL_PLAYERLEFT",			CALL_PLAYERLEFT,		scrCBPlayerLeft,
		1,	{ VAL_INT }},

	{ "CALL_ALLIANCEOFFER",			CALL_ALLIANCEOFFER,		scrCBAllianceOffer,
		2,	{ VAL_REF|VAL_INT,VAL_REF|VAL_INT }},

	/* This entry marks the end of the callback list */
	{ "CALLBACK LIST END", 0 }
};


/* The table of type equivalence
 * The format is :
 *
 *       <base type>  <num equivalents>  <eqivalent types>
 *
 */
TYPE_EQUIV asEquivTable[] =
{
	{ ST_BASEOBJECT,	3,	{ ST_DROID, ST_STRUCTURE, ST_FEATURE, } },
	{ ST_COMPONENT,		8,	{ ST_BODY, ST_PROPULSION, ST_ECM, ST_SENSOR, ST_CONSTRUCT, ST_WEAPON, ST_REPAIR, ST_BRAIN } },
	{ ST_BASESTATS,		2,	{ ST_STRUCTURESTAT, ST_FEATURESTAT } },
	
	{ ST_DROID,			1,	{ ST_POINTER_O, } },
	{ ST_STRUCTURE,		1,	{ ST_POINTER_O, } },
	{ ST_FEATURE,		1,	{ ST_POINTER_O, } },
	{ ST_BASEOBJECT,	1,	{ ST_POINTER_O, } },
	{ ST_TEMPLATE,		1,	{ ST_POINTER_T, } },
	{ ST_BODY,			1,  { ST_POINTER_S, } },
	{ ST_PROPULSION,	1,	{ ST_POINTER_S, } },
	{ ST_WEAPON,		1,	{ ST_POINTER_S, } },
	{ ST_ECM,			1,	{ ST_POINTER_S, } },
	{ ST_SENSOR,		1,	{ ST_POINTER_S, } },
	{ ST_CONSTRUCT,		1,	{ ST_POINTER_S, } },
	{ ST_REPAIR,		1,	{ ST_POINTER_S, } },
	{ ST_BRAIN,			1,	{ ST_POINTER_S, } },

	/* This marks the end of the equivalence list */
	{ 0,0 }
};


// Initialise the script system
BOOL scrTabInitialise(void)
{
	EVENT_INIT	sInit;

	sInit.valInit = 50;
	sInit.valExt = 5;
	sInit.trigInit = 35;		// was 20 ... not enough
	sInit.trigExt = 5;
	sInit.contInit = 50;
	sInit.contExt = 5;
	if (!scriptInitialise(&sInit))
	{
		return FALSE;
	}

	if (!eventInitValueFuncs(ST_MAXTYPE))
	{
		return FALSE;
	}

	scrvInitialise();
#ifndef NOSCRIPT

	// Set the constant table
	scriptSetConstTab(asConstantTable);
#endif
	// Set the function table
	scriptSetFuncTab(asFuncTable);

	// Set the type table
	scriptSetTypeTab(asTypeTable);


	// Set the external variable table
	scriptSetExternalTab(asExternTable);

	// Set the object variable table
	scriptSetObjectTab(asObjTable);




	// Set the callback table
	scriptSetCallbackTab(asCallbackTable);

	// Set the type equivalence table
	scriptSetTypeEquiv(asEquivTable);


	// Set the create and release functions
	if (!eventAddValueCreate(ST_BASEOBJECT, scrvAddBasePointer))
	{
		return FALSE;
	}
	if (!eventAddValueRelease(ST_BASEOBJECT, scrvReleaseBasePointer))
	{
		return FALSE;
	}

	if (!eventAddValueCreate(ST_DROID, scrvAddBasePointer))
	{
		return FALSE;
	}
	if (!eventAddValueRelease(ST_DROID, scrvReleaseBasePointer))
	{
		return FALSE;
	}

	if (!eventAddValueCreate(ST_STRUCTURE, scrvAddBasePointer))
	{
		return FALSE;
	}
	if (!eventAddValueRelease(ST_STRUCTURE, scrvReleaseBasePointer))
	{
		return FALSE;
	}

	if (!eventAddValueCreate(ST_FEATURE, scrvAddBasePointer))
	{
		return FALSE;
	}
	if (!eventAddValueRelease(ST_FEATURE, scrvReleaseBasePointer))
	{
		return FALSE;
	}

	if (!eventAddValueCreate(ST_GROUP, scrvNewGroup))
	{
		return FALSE;
	}
	if (!eventAddValueRelease(ST_GROUP, scrvReleaseGroup))
	{
		return FALSE;
	}

	// initialise various variables
	scrGameLevel = 0;
	bInTutorial = FALSE;

	return TRUE;
}


// Shut down the script system
void scrShutDown(void)
{
	scrvShutDown();
	scriptShutDown();
}


