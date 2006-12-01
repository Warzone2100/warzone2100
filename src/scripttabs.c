/*
 * ScriptTabs.c
 *
 * All the tables for the script compiler
 *
 */


#include "lib/framework/frame.h"
#include "lib/widget/widget.h"
#include "objects.h"
#include "lib/script/script.h"
#include "scripttabs.h"

// Get all the function prototypes
#include "scriptfuncs.h"
#include "scriptextern.h"
#include "scriptobj.h"
#include "scriptvals.h"
#include "scriptcb.h"
#include "scriptai.h"

#include "droid.h"
#include "hci.h"
#include "message.h"
//#include "mission.h"
#include "levels.h"
#include "order.h"
#include "action.h"		//new member variable - .action
#include "lib/gamelib/gtime.h"
#include "mission.h"

#include "design.h"			// for the iddes_...
#include "display.h"		// for the MT_...

#include "multiplay.h"
#include "intfac.h"
#include "multimenu.h"
#include "lib/framework/input.h"		//for key constants


/* The table of user defined types
 * The format is :
 *         <type id no.>,  "Type name",   <access type>
 *
 * The type id no. should start at VAL_USERTYPESTART and increase by one per type.
 * The access type controls whether the type is a simple data type or an object type.
 */
TYPE_SYMBOL asTypeTable[] =
{
	{ "INTMESSAGE",	(INTERP_TYPE)ST_INTMESSAGE,	AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "BASEOBJ",	(INTERP_TYPE)ST_BASEOBJECT,	AT_OBJECT,	scrValDefSave,	scrValDefLoad },
	{ "DROID",		(INTERP_TYPE)ST_DROID,		AT_OBJECT,	scrValDefSave,	scrValDefLoad },
	{ "STRUCTURE",	(INTERP_TYPE)ST_STRUCTURE,	AT_OBJECT,	scrValDefSave,	scrValDefLoad },
	{ "FEATURE",	(INTERP_TYPE)ST_FEATURE,	AT_OBJECT,	scrValDefSave,	scrValDefLoad },

	{ "BASESTATS",	(INTERP_TYPE)ST_BASESTATS,	AT_SIMPLE,	scrValDefSave,	scrValDefLoad },

	// Component types
	{ "COMPONENT",	(INTERP_TYPE)ST_COMPONENT,	AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "BODY",		(INTERP_TYPE)ST_BODY,		AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "PROPULSION",	(INTERP_TYPE)ST_PROPULSION,	AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "ECM",		(INTERP_TYPE)ST_ECM,		AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "SENSOR",		(INTERP_TYPE)ST_SENSOR,		AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "CONSTRUCT",	(INTERP_TYPE)ST_CONSTRUCT,	AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "WEAPON",		(INTERP_TYPE)ST_WEAPON,		AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "REPAIR",		(INTERP_TYPE)ST_REPAIR,		AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "BRAIN",		(INTERP_TYPE)ST_BRAIN,		AT_SIMPLE,	scrValDefSave,	scrValDefLoad },

	{ "TEMPLATE",	(INTERP_TYPE)ST_TEMPLATE,	AT_SIMPLE,	scrValDefSave,	scrValDefLoad },

	{ "STRUCTUREID",(INTERP_TYPE)ST_STRUCTUREID,AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "STRUCTURESTAT",(INTERP_TYPE)ST_STRUCTURESTAT,AT_SIMPLE,scrValDefSave,	scrValDefLoad },
	{ "FEATURESTAT",(INTERP_TYPE)ST_FEATURESTAT,AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "DROIDID",	(INTERP_TYPE)ST_DROIDID,	AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "TEXTSTRING",	(INTERP_TYPE)ST_TEXTSTRING,	AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "SOUND",		(INTERP_TYPE)ST_SOUND,		AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "LEVEL",		(INTERP_TYPE)ST_LEVEL,		AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "GROUP",		(INTERP_TYPE)ST_GROUP,		AT_OBJECT,	scrValDefSave,	scrValDefLoad },
	{ "RESEARCHSTAT",(INTERP_TYPE)ST_RESEARCH,	AT_SIMPLE,	scrValDefSave,	scrValDefLoad },

	//private types for code - NOT used in the scripts - hence the ""
	{ "",			(INTERP_TYPE)ST_POINTER_O,	AT_OBJECT,	NULL,	NULL },
	{ "",			(INTERP_TYPE)ST_POINTER_T,	AT_SIMPLE,	NULL,	NULL },
	{ "",			(INTERP_TYPE)ST_POINTER_S,	AT_SIMPLE,	NULL,	NULL },

	{ "",			(INTERP_TYPE)ST_POINTER_STRUCTSTAT,AT_SIMPLE,NULL,NULL },	//for NULLSTRUCTURESTAT

	/* This final entry marks the end of the type list */
	{ "END OF TYPE LIST",(INTERP_TYPE)NULL,		AT_SIMPLE,	NULL,	NULL },
};





/* The table of script callable C functions
 * This is laid out :
 *     "ScriptName",   function_Pointer,  <function return type>
 *      <number of parameters>,  <parameter types>,  FALSE
 */
FUNC_SYMBOL asFuncTable[] =
{
	{ "InitEnumDroids",			scrInitEnumDroids,		VAL_VOID,
		2, { VAL_INT, VAL_INT } },
	{ "EnumDroid",			scrEnumDroid,		(INTERP_TYPE)ST_DROID,
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
		2, { (INTERP_TYPE)ST_BASEOBJECT, VAL_INT } },

	{ "buildingDestroyed",		scrBuildingDestroyed,	VAL_BOOL,
		2, { (INTERP_TYPE)ST_STRUCTUREID, VAL_INT } },

	{ "structureIdle",			scrStructureIdle,		VAL_BOOL,
		1, { (INTERP_TYPE)ST_STRUCTURE } },

	{ "initEnumStruct",			scrInitEnumStruct,		VAL_VOID,
		4, { VAL_BOOL, (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT,VAL_INT } },

	{ "enumStruct",				scrEnumStruct,			(INTERP_TYPE)ST_STRUCTURE,
		0, { VAL_VOID} },

	{ "structureBeingBuilt",	scrStructureBeingBuilt,	VAL_BOOL,
		2,	{ (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT } },

	{ "structureComplete",	scrStructureComplete,		VAL_BOOL,		// pc multiplayer only
		1,	{ (INTERP_TYPE)ST_STRUCTURE } },

	{ "structureBuilt",			scrStructureBuilt,		VAL_BOOL,
		2,	{ (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT } },

	{ "anyDroidsLeft",			scrAnyDroidsLeft,		VAL_BOOL,
		1,	{ VAL_INT } },

	{ "anyStructButWallsLeft",	scrAnyStructButWallsLeft, VAL_BOOL,
		1,	{ VAL_INT } },

	{ "anyFactoriesLeft",		scrAnyFactoriesLeft,	VAL_BOOL,
		1,	{ VAL_INT } },

	// event functions
	{ "enableComponent",		scrEnableComponent,		VAL_VOID,
		2, { (INTERP_TYPE)ST_COMPONENT, VAL_INT } },

	{ "makeComponentAvailable",	scrMakeComponentAvailable,VAL_VOID,
		2, { (INTERP_TYPE)ST_COMPONENT, VAL_INT } },

	{ "enableStructure",		scrEnableStructure,		VAL_VOID,
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT } },

	{ "isStructureAvailable",		scrIsStructureAvailable,VAL_BOOL,	// pc multiplay only
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT } },

	{ "addDroid",		scrAddDroid,					(INTERP_TYPE)ST_DROID,
		4, { (INTERP_TYPE)ST_TEMPLATE, VAL_INT, VAL_INT, VAL_INT } },

	{ "addDroidToMissionList",	scrAddDroidToMissionList,(INTERP_TYPE)ST_DROID,
		2, { (INTERP_TYPE)ST_TEMPLATE, VAL_INT } },

	{ "buildDroid",		scrBuildDroid,					VAL_VOID,
		4, { (INTERP_TYPE)ST_TEMPLATE, (INTERP_TYPE)ST_STRUCTURE, VAL_INT, VAL_INT } },

	{ "addTemplate",		scrAddTemplate,				VAL_BOOL,
		2, { (INTERP_TYPE)ST_TEMPLATE, VAL_INT } },

	{ "addReticuleButton",		scrAddReticuleButton,	VAL_VOID,
		1, { VAL_INT } },

	{ "removeReticuleButton",	scrRemoveReticuleButton,VAL_VOID,
		2, { VAL_INT, VAL_BOOL } },

	{ "addMessage",				scrAddMessage,			VAL_VOID,
		4, { (INTERP_TYPE)ST_INTMESSAGE, VAL_INT, VAL_INT, VAL_BOOL } },

	{ "removeMessage",			scrRemoveMessage,		VAL_VOID,
		3, { (INTERP_TYPE)ST_INTMESSAGE, VAL_INT, VAL_INT } },

/*	{ "addTutorialMessage",		scrAddTutorialMessage,	VAL_VOID,
		2, { (INTERP_TYPE)ST_INTMESSAGE, VAL_INT } },*/

	{ "selectDroidByID",		scrSelectDroidByID,		VAL_BOOL,
		2, { (INTERP_TYPE)ST_DROIDID, VAL_INT } },

	{ "setAssemblyPoint",		scrSetAssemblyPoint,	VAL_VOID,
		3, { (INTERP_TYPE)ST_STRUCTURE, VAL_INT, VAL_INT } },

	{ "attackLocation",			scrAttackLocation,		VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT } },

	{ "initGetFeature",			scrInitGetFeature,		VAL_VOID,
		3, { (INTERP_TYPE)ST_FEATURESTAT, VAL_INT, VAL_INT } },

	{ "getFeature",				scrGetFeature,			(INTERP_TYPE)ST_FEATURE,
		1, { VAL_INT} },

	{ "addFeature",				scrAddFeature,			(INTERP_TYPE)ST_FEATURE,
		3, { (INTERP_TYPE)ST_FEATURESTAT, VAL_INT, VAL_INT } },

	{ "destroyFeature",			scrDestroyFeature,		VAL_VOID,
		1, { (INTERP_TYPE)ST_FEATURE } },

	{ "addStructure",			scrAddStructure,		(INTERP_TYPE)ST_STRUCTURE,
		4, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT, VAL_INT, VAL_INT } },

	{ "destroyStructure",		scrDestroyStructure,	VAL_VOID,
		1, { (INTERP_TYPE)ST_STRUCTURE } },

	{ "centreView",				scrCentreView,			VAL_VOID,
		1, { (INTERP_TYPE)ST_BASEOBJECT } },

	{ "centreViewPos",			scrCentreViewPos,		VAL_VOID,
		2, { VAL_INT, VAL_INT } },

	{ "getStructure",			scrGetStructure,		(INTERP_TYPE)ST_STRUCTURE,
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT } },

	{ "getTemplate",			scrGetTemplate,			(INTERP_TYPE)ST_TEMPLATE,
		2, { (INTERP_TYPE)ST_COMPONENT, VAL_INT } },

	{ "getDroid",				scrGetDroid,			(INTERP_TYPE)ST_DROID,
		2, { (INTERP_TYPE)ST_COMPONENT, VAL_INT } },

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
		2, { (INTERP_TYPE)ST_SENSOR, VAL_INT } },

	{ "setDefaultECM",		scrSetDefaultECM,			VAL_VOID,
		2, { (INTERP_TYPE)ST_ECM, VAL_INT } },

	{ "setDefaultRepair",	scrSetDefaultRepair,		VAL_VOID,
		2, { (INTERP_TYPE)ST_REPAIR, VAL_INT } },

	{ "setStructureLimits",	scrSetStructureLimits,		VAL_VOID,
		3, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT, VAL_INT } },

	{ "setAllStructureLimits",	scrSetAllStructureLimits,	VAL_VOID,
		2, { VAL_INT, VAL_INT } },

	{ "applyLimitSet",		scrApplyLimitSet,			VAL_VOID,
		0, { VAL_VOID } },

	{ "playSound",			scrPlaySound,				VAL_VOID,
		2, { (INTERP_TYPE)ST_SOUND, VAL_INT } },

	{ "playSoundPos",		scrPlaySoundPos,			VAL_VOID,
		5, { (INTERP_TYPE)ST_SOUND, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "addConsoleText",		scrAddConsoleText,			VAL_VOID,
		2, { (INTERP_TYPE)ST_TEXTSTRING, VAL_INT } },

	{ "showConsoleText",		scrShowConsoleText,			VAL_VOID,
		2, { (INTERP_TYPE)ST_TEXTSTRING, VAL_INT } },

	{ "tagConsoleText",		scrTagConsoleText,			VAL_VOID,
		2, { (INTERP_TYPE)ST_TEXTSTRING, VAL_INT } },

	{ "turnPowerOn",		scrTurnPowerOn,				VAL_VOID,
		0, { VAL_VOID } },

	{ "turnPowerOff",		scrTurnPowerOff,			VAL_VOID,
		0, { VAL_VOID } },

	{ "tutorialEnd",		scrTutorialEnd,				VAL_VOID,
		0, { VAL_VOID } },
	{ "clearConsole",		scrClearConsole,			VAL_VOID,
	0, { VAL_VOID} },

	{ "playVideo",			scrPlayVideo,				VAL_VOID,
		2, { (INTERP_TYPE)ST_TEXTSTRING, (INTERP_TYPE)ST_TEXTSTRING } },

	{ "gameOverMessage",	scrGameOverMessage,			VAL_VOID,
		4, { (INTERP_TYPE)ST_INTMESSAGE, VAL_INT, VAL_INT, VAL_BOOL } },

	{ "gameOver",			scrGameOver,				VAL_VOID,
		1, { VAL_BOOL } },

	{ "playBackgroundAudio", scrPlayBackgroundAudio,	VAL_VOID,
		2, { (INTERP_TYPE)ST_TEXTSTRING, VAL_INT } },

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
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT } },

	{ "setGroupRetreatLeadership",	scrSetGroupRetreatLeadership,	VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT } },

	{ "setGroupRetreatPoint",	scrSetGroupRetreatPoint,			VAL_VOID,
		3, { (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT } },

	{ "setRetreatHealth",	    scrSetRetreatHealth,		VAL_VOID,
		2, { VAL_INT, VAL_INT } },

	{ "setGroupRetreatHealth",	scrSetGroupRetreatHealth,		VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT } },

    { "startMission",		scrStartMission,			VAL_VOID,
		2, { VAL_INT, (INTERP_TYPE)ST_LEVEL } },

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
		2, { (INTERP_TYPE)ST_DROID, (INTERP_TYPE)ST_DROID } },

	/*{ "endMission",			scrEndMission,				VAL_VOID,
		1, { VAL_BOOL } },*/

	{ "structureBuiltInRange",	scrStructureBuiltInRange,	(INTERP_TYPE)ST_STRUCTURE,
		5, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "random",				scrRandom,				VAL_INT,
		1, { VAL_INT } },

	{ "randomiseSeed",		scrRandomiseSeed,		VAL_VOID,
		0, { VAL_VOID } },

	{ "enableResearch",		scrEnableResearch,		VAL_VOID,
		2, { (INTERP_TYPE)ST_RESEARCH, VAL_INT } },

	{ "completeResearch",	scrCompleteResearch,	VAL_VOID,
		2, { (INTERP_TYPE)ST_RESEARCH, VAL_INT } },

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
		4, {(INTERP_TYPE)ST_STRUCTURESTAT, VAL_REF | VAL_INT, VAL_REF | VAL_INT, VAL_INT } },

	// AI functions
	{ "groupAddDroid",		scrGroupAddDroid,			VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, (INTERP_TYPE)ST_DROID } },
	{ "groupAddArea",		scrGroupAddArea,			VAL_VOID,
		6, { (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT,VAL_INT, VAL_INT,VAL_INT } },
	{ "groupAddAreaNoGroup",scrGroupAddArea,			VAL_VOID,
		6, { (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT,VAL_INT, VAL_INT,VAL_INT } },
	{ "groupAddGroup",		scrGroupAddGroup,			VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, (INTERP_TYPE)ST_GROUP } },
	{ "groupMember",		scrGroupMember,				VAL_BOOL,
		2, { (INTERP_TYPE)ST_GROUP, (INTERP_TYPE)ST_DROID } },
	{ "idleGroup",			scrIdleGroup,				VAL_INT,
		1, { (INTERP_TYPE)ST_GROUP } },
	{ "initIterateGroup",	scrInitIterateGroup,		VAL_VOID,
		1, { (INTERP_TYPE)ST_GROUP } },
	{ "iterateGroup",		scrIterateGroup,			(INTERP_TYPE)ST_DROID,
		1, { (INTERP_TYPE)ST_GROUP } },
	{ "droidLeaveGroup",	scrDroidLeaveGroup,			VAL_VOID,
		1, { (INTERP_TYPE)ST_DROID } },

	{ "orderDroid",			scrOrderDroid,				VAL_VOID,
		2, { (INTERP_TYPE)ST_DROID, VAL_INT } },
	{ "orderDroidLoc",		scrOrderDroidLoc,			VAL_VOID,
		4, { (INTERP_TYPE)ST_DROID, VAL_INT, VAL_INT, VAL_INT } },
	{ "orderDroidObj",		scrOrderDroidObj,			VAL_VOID,
		3, { (INTERP_TYPE)ST_DROID, VAL_INT, (INTERP_TYPE)ST_BASEOBJECT } },
	{ "orderDroidStatsLoc",	scrOrderDroidStatsLoc,		VAL_VOID,
		5, { (INTERP_TYPE)ST_DROID, VAL_INT, (INTERP_TYPE)ST_BASESTATS, VAL_INT, VAL_INT } },

	{ "orderGroup",			scrOrderGroup,				VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT } },
	{ "orderGroupLoc",		scrOrderGroupLoc,			VAL_VOID,
		4, { (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT, VAL_INT } },
	{ "orderGroupObj",		scrOrderGroupObj,			VAL_VOID,
		3, { (INTERP_TYPE)ST_GROUP, VAL_INT, (INTERP_TYPE)ST_BASEOBJECT } },

	{ "setDroidSecondary",	scrSetDroidSecondary,		VAL_VOID,
		3, { (INTERP_TYPE)ST_DROID, VAL_INT, VAL_INT } },
	{ "setGroupSecondary",	scrSetGroupSecondary,		VAL_VOID,
		3, { (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT } },

	{ "initIterateCluster", scrInitIterateCluster,		VAL_VOID,
		1, { VAL_INT } },
	{ "iterateCluster",		scrIterateCluster,			(INTERP_TYPE)ST_BASEOBJECT,
		0, { VAL_VOID } },
	{ "cmdDroidAddDroid",	scrCmdDroidAddDroid,		VAL_VOID,
		2, { (INTERP_TYPE)ST_DROID, (INTERP_TYPE)ST_DROID } },

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
		3, { (INTERP_TYPE)ST_BASEOBJECT, (INTERP_TYPE)ST_BASEOBJECT, VAL_BOOL } },

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
		3, {VAL_INT,(INTERP_TYPE)ST_STRUCTURE,VAL_INT} },

	{ "killDroidsInArea",		scrDestroyUnitsInArea,	VAL_INT,
	5, {VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT} },

	{ "vanishUnit",				scrRemoveDroid,			VAL_VOID,
		1, {(INTERP_TYPE)ST_DROID} },

	{ "forceDamageObject",		scrForceDamage,			VAL_VOID,
		2, {(INTERP_TYPE)ST_BASEOBJECT,VAL_INT}},

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
	{ "objToDroid",			scrObjToDroid,				(INTERP_TYPE)ST_DROID,
		1, { (INTERP_TYPE)ST_BASEOBJECT } },
	{ "objToStructure",		scrObjToStructure,			(INTERP_TYPE)ST_STRUCTURE,
		1, { (INTERP_TYPE)ST_BASEOBJECT } },
	{ "objToFeature",		scrObjToFeature,			(INTERP_TYPE)ST_FEATURE,
		1, { (INTERP_TYPE)ST_BASEOBJECT } },


	{"getGameStatus", 		scrGetGameStatus,	  		VAL_BOOL,	1, {VAL_INT} },

    //player colour access functions
	{ "getPlayerColour",	scrGetPlayerColour,		    VAL_INT,
        1, { VAL_INT } },

	{ "getPlayerColourName",	scrGetPlayerColourName,		    VAL_STRING,
        1, { VAL_INT } },

    { "setPlayerColour",	scrSetPlayerColour,		    VAL_VOID,
		2, { VAL_INT, VAL_INT } },

    { "takeOverDroidsInArea",	scrTakeOverDroidsInArea,	VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

    { "takeOverDroidsInAreaExp",scrTakeOverDroidsInAreaExp,	VAL_INT,
		8, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

    { "takeOverStructsInArea",	scrTakeOverStructsInArea,	VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

    { "takeOverSingleDroid",	scrTakeOverSingleDroid,	    (INTERP_TYPE)ST_DROID,
		2, { (INTERP_TYPE)ST_DROID, VAL_INT} },

    { "takeOverSingleStructure",    scrTakeOverSingleStructure,	    (INTERP_TYPE)ST_STRUCTURE,
		2, { (INTERP_TYPE)ST_STRUCTURE, VAL_INT} },

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
	{ "structTargetInArea",	scrStructTargetInArea,			(INTERP_TYPE)ST_STRUCTURE,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, }, },
	{ "structTargetOnMap",	scrStructTargetOnMap,			(INTERP_TYPE)ST_STRUCTURE,
		2, { VAL_INT, VAL_INT }, },
	{ "droidTargetInArea",	scrDroidTargetInArea,			(INTERP_TYPE)ST_DROID,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, }, },
	{ "droidTargetOnMap",	scrDroidTargetOnMap,			(INTERP_TYPE)ST_DROID,
		2, { VAL_INT, VAL_INT }, },
	{ "targetInCluster",	scrTargetInCluster,				(INTERP_TYPE)ST_DROID,
		2, { VAL_INT, VAL_INT }, },

	{ "setDroidsToSafetyFlag",	scrSetDroidsToSafetyFlag,	VAL_VOID,
		1, { VAL_BOOL } },

	{ "setPlayCountDown",       scrSetPlayCountDown,	    VAL_VOID,
		1, { VAL_BOOL } },

	{ "getDroidCount",	        scrGetDroidCount,		    VAL_INT,
        1, { VAL_INT } },

	{ "fireWeaponAtObj",		scrFireWeaponAtObj,			VAL_VOID,
		2, { (INTERP_TYPE)ST_WEAPON, (INTERP_TYPE)ST_BASEOBJECT } },
	{ "fireWeaponAtLoc",		scrFireWeaponAtLoc,			VAL_VOID,
		3, { (INTERP_TYPE)ST_WEAPON, VAL_INT,VAL_INT } },

	{ "setDroidKills",			scrSetDroidKills,			VAL_VOID,
		2, { (INTERP_TYPE)ST_DROID, VAL_INT } },
	{ "resetPlayerVisibility",	scrResetPlayerVisibility,	VAL_VOID,
		1, { VAL_INT } },

	{ "setVTOLReturnPos",		scrSetVTOLReturnPos,		VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT } },

	{ "isVtol",					scrIsVtol,					VAL_BOOL,
		1, { (INTERP_TYPE)ST_DROID } },

	{ "tutorialTemplates",		scrTutorialTemplates,		VAL_VOID,
		0, { VAL_VOID } },

	{ "resetLimboMission",		scrResetLimboMission,		VAL_VOID,
		0, { VAL_VOID } },

//ajl skirmish funcs
	{ "skDoResearch",			scrSkDoResearch,			VAL_VOID,
		3, { (INTERP_TYPE)ST_STRUCTURE, VAL_INT, VAL_INT } },

	{ "skLocateEnemy",			scrSkLocateEnemy,			(INTERP_TYPE)ST_BASEOBJECT,
		1, { VAL_INT } },

	{ "skCanBuildTemplate",		scrSkCanBuildTemplate,		VAL_BOOL,
		3, { VAL_INT, (INTERP_TYPE)ST_STRUCTURE, (INTERP_TYPE)ST_TEMPLATE } },

	{ "skVtolEnableCheck",		scrSkVtolEnableCheck,		VAL_BOOL,
		1, { VAL_INT } },

	{ "skGetFactoryCapacity",	scrSkGetFactoryCapacity,	VAL_INT,
	1, { (INTERP_TYPE)ST_STRUCTURE } },

	{ "skDifficultyModifier",	scrSkDifficultyModifier,	VAL_VOID,
	1, { VAL_INT } },

	{ "skDefenseLocation",		scrSkDefenseLocation,		VAL_BOOL,
	6, {VAL_REF|VAL_INT, VAL_REF|VAL_INT, (INTERP_TYPE)ST_STRUCTURESTAT, (INTERP_TYPE)ST_STRUCTURESTAT, (INTERP_TYPE)ST_DROID,VAL_INT } },

	{ "skFireLassat",			scrSkFireLassat,			VAL_VOID,
	2, {VAL_INT, (INTERP_TYPE)ST_BASEOBJECT } },

	/* New functions */

	{ "strcmp",			scrStrcmp,		VAL_BOOL,
		2, { VAL_STRING, VAL_STRING} },

	{ "console",			scrConsole,				VAL_VOID,
		1, { VAL_STRING } },

	{ "MsgBox",			scrMsgBox,				VAL_VOID,
		1, { VAL_STRING } },

	{ "dbgMsgOn",			scrDbgMsgOn,				VAL_VOID,
		2, { VAL_INT, VAL_BOOL } },
	{ "dbg",			scrDbg,				VAL_VOID,
		2, { VAL_STRING, VAL_INT } },
	{ "msg",			scrMsg,				VAL_VOID,		//multiplayer msg
		3, { VAL_STRING, VAL_INT, VAL_INT } },

	{ "debug",			scrDebugFile,				VAL_VOID,
		1, { VAL_STRING } },


	{ "getTargetPlayers",	scrGetTargetPlayers,	VAL_INT,
		1, { VAL_REF|VAL_STRING } },

	{ "match",	scrMatch,	VAL_BOOL,
		3, { VAL_STRING, VAL_STRING, VAL_REF|VAL_INT } },

	{ "bitSet",	scrBitSet,	VAL_BOOL,
		2, { VAL_INT, VAL_INT } },

	{ "dropBeacon",				scrDropBeacon,			VAL_VOID,
		6, { VAL_STRING, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "removeHelpMessage",				scrRemoveHelpMessage,			VAL_VOID,
		2, { VAL_INT, VAL_INT } },

	//droid functions
	//-----------------------------------
	{ "actionDroidObj",		scrActionDroidObj,			VAL_VOID,
		3, { (INTERP_TYPE)ST_DROID, VAL_INT, (INTERP_TYPE)ST_BASEOBJECT } },

	{ "InitEnumDroids",			scrInitEnumDroids,		VAL_VOID,
		2, { VAL_INT, VAL_INT } },
	{ "EnumDroid",			scrEnumDroid,		(INTERP_TYPE)ST_DROID,
		0, { VAL_VOID } },

	{ "initIterateGroupB",	scrInitIterateGroupB,		VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT } },

	{ "iterateGroupB",		scrIterateGroupB,			(INTERP_TYPE)ST_DROID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT } },

	{ "closestDamagedGroupDroid",		scrClosestDamagedGroupDroid,			(INTERP_TYPE)ST_DROID,
		6, { VAL_INT, (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },


	{ "factoryGetTemplate",			scrFactoryGetTemplate,		(INTERP_TYPE)ST_TEMPLATE,
		1, { (INTERP_TYPE)ST_STRUCTURE } },

	{ "numTemplatesInProduction",	scrNumTemplatesInProduction,		VAL_INT,
		2, { (INTERP_TYPE)ST_TEMPLATE, VAL_INT } },

	{ "numDroidsByComponent",			scrNumDroidsByComponent,		VAL_INT,
		3, { (INTERP_TYPE)ST_COMPONENT, VAL_INT, VAL_INT } },

	{ "getStructureLimit",	scrGetStructureLimit,		VAL_INT,
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT } },

	{ "structureLimitReached",	scrStructureLimitReached,		VAL_BOOL,
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT } },

	{ "getNumStructures",	scrGetNumStructures,		VAL_INT,
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT } },

	{ "getUnitLimit",	scrGetUnitLimit,		VAL_INT,
		1, { VAL_INT } },

	{ "min",	scrMin,		VAL_INT,
		2, { VAL_INT, VAL_INT } },

	{ "max",	scrMax,		VAL_INT,
		2, { VAL_INT, VAL_INT } },

	{ "fogTileInRange",	scrFogTileInRange,		VAL_BOOL,
		9, { VAL_REF|VAL_INT, VAL_REF|VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "mapRevealedInRange",	scrMapRevealedInRange,		VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT} },

	{ "pursueResearch",			scrPursueResearch,			VAL_BOOL,
		3, { (INTERP_TYPE)ST_STRUCTURE, VAL_INT, (INTERP_TYPE)ST_RESEARCH } },

	{ "numResearchLeft",			scrNumResearchLeft,			VAL_INT,
		2, { VAL_INT, (INTERP_TYPE)ST_RESEARCH } },

	{ "researchFinished",			scrResearchCompleted,			VAL_BOOL,
		2, { (INTERP_TYPE)ST_RESEARCH, VAL_INT } },

	{ "researchStarted",			scrResearchStarted,			VAL_BOOL,
		2, { (INTERP_TYPE)ST_RESEARCH, VAL_INT } },

	{ "threatInRange",			scrThreatInRange,			VAL_BOOL,
		5, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL} },

	{ "numEnemyWeapObjInRange",			scrNumEnemyWeapObjInRange,			VAL_INT,
		5, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL} },

	{ "numEnemyWeapDroidsInRange",			scrNumEnemyWeapDroidsInRange,			VAL_INT,
		5, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL} },

	{ "numEnemyWeapStructsInRange",			scrNumEnemyWeapStructsInRange,			VAL_INT,
		4, { VAL_INT, VAL_INT , VAL_INT, VAL_INT} },

	{ "numFriendlyWeapObjInRange",			scrNumFriendlyWeapObjInRange,			VAL_INT,
		5, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL} },

	{ "numFriendlyWeapDroidsInRange",			scrNumFriendlyWeapDroidsInRange,			VAL_INT,
		5, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL} },

	{ "numFriendlyWeapStructsInRange",			scrNumFriendlyWeapStructsInRange,			VAL_INT,
		4, { VAL_INT, VAL_INT , VAL_INT, VAL_INT} },

	{ "numPlayerWeapObjInRange",			scrNumPlayerWeapObjInRange,			VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL} },

	{ "numEnemyObjInRange",			scrNumEnemyObjInRange,			VAL_INT,
		5, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL} },

	{ "alliancesLocked",			scrAlliancesLocked,			VAL_BOOL,
		0, {VAL_VOID} },

//num structures
	{ "numStructsByStatInRange",			scrNumStructsByStatInRange,		VAL_INT,
		6, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT , VAL_INT, VAL_INT, VAL_INT, VAL_INT} },

	{ "numStructsByStatInArea",			scrNumStructsByStatInArea,		VAL_INT,
		7, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT , VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT} },

	{ "numStructsByTypeInRange",		scrNumStructsByTypeInRange,	VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "numFeatByTypeInRange",		scrNumFeatByTypeInRange,	VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "numStructsButNotWallsInRangeVis",			scrNumStructsButNotWallsInRangeVis,		VAL_INT,
		5, { VAL_INT , VAL_INT, VAL_INT, VAL_INT, VAL_INT} },

	{ "getStructureVis",			scrGetStructureVis,		(INTERP_TYPE)ST_STRUCTURE,
		3, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT, VAL_INT } },

	{ "chooseValidLoc",			scrChooseValidLoc,		VAL_BOOL,
		6, { VAL_REF|VAL_INT, VAL_REF|VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

	{ "getClosestEnemy",			scrGetClosestEnemy,		(INTERP_TYPE)ST_BASEOBJECT,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL, VAL_INT } },

	{ "transporterCapacity",		scrTransporterCapacity,		VAL_INT,
		1, { (INTERP_TYPE)ST_DROID} },

	{ "transporterFlying",		scrTransporterFlying,		VAL_BOOL,
		1, { (INTERP_TYPE)ST_DROID} },

	{ "unloadTransporter",		scrUnloadTransporter,		VAL_VOID,
		3, { (INTERP_TYPE)ST_DROID, VAL_INT, VAL_INT} },

	{ "hasGroup",		scrHasGroup,		VAL_BOOL,
		1, { (INTERP_TYPE)ST_DROID} },

	{ "objWeaponMaxRange",		scrObjWeaponMaxRange,		VAL_INT,
		1, { (INTERP_TYPE)ST_BASEOBJECT} },

	{ "objHasWeapon",			scrObjHasWeapon,		VAL_BOOL,
		1, { (INTERP_TYPE)ST_BASEOBJECT} },

	{ "objectHasIndirectWeapon",		scrObjectHasIndirectWeapon,		VAL_BOOL,
		1, { (INTERP_TYPE)ST_BASEOBJECT} },

	{ "getClosestEnemyDroidByType",			scrGetClosestEnemyDroidByType,		(INTERP_TYPE)ST_DROID,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_BOOL, VAL_INT } },

	{ "getClosestEnemyStructByType",		scrGetClosestEnemyStructByType,	(INTERP_TYPE)ST_STRUCTURE,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },


	{ "skDefenseLocationB",		scrSkDefenseLocationB,		VAL_BOOL,
	6, {VAL_REF|VAL_INT, VAL_REF|VAL_INT, (INTERP_TYPE)ST_STRUCTURESTAT, (INTERP_TYPE)ST_STRUCTURESTAT, (INTERP_TYPE)ST_DROID,VAL_INT } },

	{ "circlePerimPoint",			scrCirclePerimPoint,		VAL_VOID,
		5, { VAL_INT, VAL_INT, VAL_REF|VAL_INT, VAL_REF|VAL_INT, VAL_INT} },


	/* learn functions */

	{ "learnPlayerBaseLoc",			scrLearnPlayerBaseLoc,		VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT} },

	{ "recallPlayerBaseLoc",			scrRecallPlayerBaseLoc,		VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_REF|VAL_INT, VAL_REF|VAL_INT} },

	{ "canRememberPlayerBaseLoc",			scrCanRememberPlayerBaseLoc,		VAL_BOOL,
		2, { VAL_INT, VAL_INT} },

	{ "learnBaseDefendLoc",			scrLearnBaseDefendLoc,		VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT} },

	{ "learnOilDefendLoc",			scrLearnOilDefendLoc,		VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT} },

	{ "getBaseDefendLocIndex",			scrGetBaseDefendLocIndex,		VAL_INT,
		3, { VAL_INT, VAL_INT, VAL_INT} },

	{ "getOilDefendLocIndex",			scrGetOilDefendLocIndex,		VAL_INT,
		3, { VAL_INT, VAL_INT, VAL_INT} },

	{ "getBaseDefendLocCount",			scrGetBaseDefendLocCount,		VAL_INT,
		0, { VAL_VOID} },

	{ "getOilDefendLocCount",			scrGetOilDefendLocCount,		VAL_INT,
		0, { VAL_VOID} },

	{ "recallBaseDefendLoc",			scrRecallBaseDefendLoc,		VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_REF|VAL_INT, VAL_REF|VAL_INT, VAL_REF|VAL_INT} },

	{ "recallOilDefendLoc",			scrRecallOilDefendLoc,		VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_REF|VAL_INT, VAL_REF|VAL_INT, VAL_REF|VAL_INT} },

	{ "savePlayerAIExperience",			scrSavePlayerAIExperience,		VAL_BOOL,
		2, { VAL_INT, VAL_BOOL} },

	{ "loadPlayerAIExperience",			scrLoadPlayerAIExperience,		VAL_BOOL,
	2, { VAL_INT, VAL_BOOL} },

	/* end of learn functions */

	{ "structInRangeVis",			scrStructInRangeVis,		VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },


	{ "giftRadar",			scrGiftRadar,		VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_BOOL} },

	{ "numAllies",			scrNumAllies,		VAL_INT,
		1, { VAL_INT} },

	{ "numAAinRange",			scrNumAAinRange,		VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT} },

	{ "selectDroid",			scrSelectDroid,		VAL_VOID,
		2, { (INTERP_TYPE)ST_DROID, VAL_BOOL} },

	{ "selectGroup",			scrSelectGroup,		VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_BOOL} },

	{ "modulo",			scrModulo,		VAL_INT,
		2, { VAL_INT, VAL_INT} },

	{ "playerLoaded",			scrPlayerLoaded,		VAL_BOOL,
		1, { VAL_INT} },

	{ "getStructureType",			scrGetStructureType,		VAL_INT,
		1, { (INTERP_TYPE)ST_STRUCTURE} },

	{ "playerName",			scrGetPlayerName,		VAL_STRING,
		1, { VAL_INT} },

	{ "getPlayerName",			scrGetPlayerName,		VAL_STRING,
		1, { VAL_INT} },

	{ "setPlayerName",			scrSetPlayerName,		VAL_BOOL,
		2, { VAL_INT, VAL_STRING} },

	{ "ASSERT",			scrASSERT,		VAL_VOID,
		3, { VAL_BOOL, VAL_STRING, VAL_INT} },

	{ "showRangeAtPos",		scrShowRangeAtPos,		VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT} },

	{ "toPow",		scrToPow,		VAL_FLOAT,
		2, { VAL_FLOAT, VAL_FLOAT,} },

	{ "debugMenu",		scrDebugMenu,		VAL_VOID,
		1, { VAL_BOOL} },

	{ "setDebugMenuEntry",		scrSetDebugMenuEntry,		VAL_VOID,
		2, { VAL_STRING, VAL_INT} },

	/* END new functions */


//	{ "skOrderDroidLineBuild",	scrSkOrderDroidLineBuild,	VAL_VOID,
//	6, { (INTERP_TYPE)ST_DROID, (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT, VAL_INT, VAL_INT, VAL_INT } },

    /* This final entry marks the end of the function list */
	{ "FUNCTION LIST END",		NULL,					VAL_VOID,
		0, { VAL_VOID } }
};
/*
 * The table of external variables and their access functions.
 * The table is laid out as follows :
 *
 *   "variable_identifier", <variable type>, (INTERP_TYPE)ST_EXTERN, 0, <index>,
 *		<get_function>, <set_function>
 *
 * The Storage type for an external variable is always (INTERP_TYPE)ST_EXTERN.
 * The index is not used by the compiler but is passed to the access function
 * to allow one function to deal with a number of variables
 */
VAR_SYMBOL asExternTable[] =
{
	{ "trackTransporter",VAL_BOOL,	(INTERP_TYPE)ST_EXTERN,	0,		EXTID_TRACKTRANSPORTER,
		scrGenExternGet,		NULL },

	{ "mapWidth",		VAL_INT,	(INTERP_TYPE)ST_EXTERN,	0,		EXTID_MAPWIDTH,
		scrGenExternGet,		NULL },
	{ "mapHeight",		VAL_INT,	(INTERP_TYPE)ST_EXTERN,	0,		EXTID_MAPHEIGHT,
		scrGenExternGet,		NULL },

	{ "gameInitialised",VAL_BOOL,	(INTERP_TYPE)ST_EXTERN,	0,		EXTID_GAMEINIT,
		scrGenExternGet,		NULL },

	{ "selectedPlayer",	VAL_INT,	(INTERP_TYPE)ST_EXTERN,	0,		EXTID_SELECTEDPLAYER,
		scrGenExternGet,		NULL },
	{ "gameTime",		VAL_INT,	(INTERP_TYPE)ST_EXTERN,	0,		EXTID_GAMETIME,
		scrGenExternGet,		NULL },

	{ "gameLevel",		VAL_INT,	(INTERP_TYPE)ST_EXTERN,	0,		EXTID_GAMELEVEL,
		scrGenExternGet,		scrGenExternSet },
	{ "inTutorial",		VAL_BOOL,	(INTERP_TYPE)ST_EXTERN,	0,		EXTID_TUTORIAL,
		scrGenExternGet,		scrGenExternSet },
	{ "cursorType",		VAL_INT,	(INTERP_TYPE)ST_EXTERN,	0,		EXTID_CURSOR,
		scrGenExternGet, NULL },


	{ "intMode",		VAL_INT,	(INTERP_TYPE)ST_EXTERN,	0,		EXTID_INTMODE,
		scrGenExternGet, NULL },

	{ "targetedObjectType",	VAL_INT,	(INTERP_TYPE)ST_EXTERN,	0,		EXTID_TARGETTYPE,
		scrGenExternGet, NULL },
	{ "extraVictoryFlag",VAL_BOOL,	(INTERP_TYPE)ST_EXTERN,	0,		EXTID_EXTRAVICTORYFLAG,
		scrGenExternGet,		scrGenExternSet },
	{ "extraFailFlag",	VAL_BOOL,	(INTERP_TYPE)ST_EXTERN,	0,		EXTID_EXTRAFAILFLAG,
		scrGenExternGet,		scrGenExternSet },

	{ "multiPlayerGameType",VAL_INT,	(INTERP_TYPE)ST_EXTERN,	0,	EXTID_MULTIGAMETYPE,
		scrGenExternGet,			NULL },

	{ "multiPlayerMaxPlayers",VAL_INT,	(INTERP_TYPE)ST_EXTERN,	0,EXTID_MULTIGAMEHUMANMAX,
		scrGenExternGet,			NULL },

	{ "multiPlayerBaseType",VAL_INT,	(INTERP_TYPE)ST_EXTERN,	0,EXTID_MULTIGAMEBASETYPE,
		scrGenExternGet,			NULL },

	{ "multiPlayerAlliancesType",VAL_INT,	(INTERP_TYPE)ST_EXTERN,	0,	EXTID_MULTIGAMEALLIANCESTYPE,
		scrGenExternGet,			NULL },



	/* This entry marks the end of the variable list */
	{ NULL, VAL_VOID, (INTERP_TYPE)ST_EXTERN, 0, 0, NULL, NULL }
};



/*
 * The table of object variables and their access functions.
 * The table is laid out as follows :
 *
 *   "variable_identifier", <variable type>, (INTERP_TYPE)ST_OBJECT, <object type>, <index>,
 *		<get_function>, <set_function>
 *
 * The Storage type for an object variable is always (INTERP_TYPE)ST_OBJECT.
 * The object type is the type of the object this is a member of.
 * The index is not used by the compiler but is passed to the access function
 * to allow one function to deal with a number of variables
 */
VAR_SYMBOL asObjTable[] =
{
	// base object variables
	{ "x",				VAL_INT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_BASEOBJECT,	OBJID_POSX,
		scrBaseObjGet,			NULL },
	{ "y",				VAL_INT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_BASEOBJECT,	OBJID_POSY,
		scrBaseObjGet,			NULL },
	{ "z",				VAL_INT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_BASEOBJECT,	OBJID_POSZ,
		scrBaseObjGet,			NULL },
	{ "id",				VAL_INT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_BASEOBJECT,	OBJID_ID,
		scrBaseObjGet,			NULL },
	{ "player",			VAL_INT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_BASEOBJECT,	OBJID_PLAYER,
		scrBaseObjGet,			NULL },
	{ "type",			VAL_INT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_BASEOBJECT,	OBJID_TYPE,
		scrBaseObjGet,			NULL },
	{ "clusterID",		VAL_INT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_BASEOBJECT,	OBJID_CLUSTERID,
		scrBaseObjGet,			NULL },
	{ "health",			VAL_INT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_BASEOBJECT,	OBJID_HEALTH,
		scrBaseObjGet,			NULL },

	// droid variables
	{ "order",			VAL_INT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_DROID,		OBJID_ORDER,
		scrBaseObjGet,			NULL },
	{ "droidType",		VAL_INT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_DROID,		OBJID_DROIDTYPE,
		scrBaseObjGet,			NULL },
	{ "body",			(INTERP_TYPE)ST_BODY,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_DROID,		OBJID_BODY,
		scrBaseObjGet,			NULL },
	{ "propulsion",		(INTERP_TYPE)ST_PROPULSION,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_DROID,		OBJID_PROPULSION,
		scrBaseObjGet,			NULL },
	{ "weapon",			(INTERP_TYPE)ST_WEAPON,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_DROID,		OBJID_WEAPON,
		scrBaseObjGet,			NULL },
	{ "orderx",			VAL_INT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_DROID,		OBJID_ORDERX,
		scrBaseObjGet,			NULL },
	{ "ordery",			VAL_INT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_DROID,		OBJID_ORDERY,
		scrBaseObjGet,			NULL },

	// structure variables
	//{ "stat",			(INTERP_TYPE)ST_STRUCTURESTAT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_STRUCTURE,	OBJID_STRUCTSTAT,
	//	scrBaseObjGet,			NULL },


	// group variables
	{ "x",				VAL_INT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_GROUP,		GROUPID_POSX,
		scrGroupObjGet,			NULL },
	{ "y",				VAL_INT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_GROUP,		GROUPID_POSY,
		scrGroupObjGet,			NULL },
	{ "members",		VAL_INT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_GROUP,		GROUPID_MEMBERS,
		scrGroupObjGet,			NULL },
	{ "health",			VAL_INT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_GROUP,		GROUPID_HEALTH,
		scrGroupObjGet,			NULL },

	/* new member variables */
	//similiar to .order
	{ "action",			VAL_INT,			(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_DROID,		OBJID_ACTION,
		scrBaseObjGet,			NULL },

	//.stat - now supports droids, ST_STRUCTURE became ST_BASEOBJECT
	{ "stat",			(INTERP_TYPE)ST_STRUCTURESTAT,	(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_BASEOBJECT,	OBJID_STRUCTSTAT,
		scrBaseObjGet,			NULL },

	//object->psTarget
	{ "target",			(INTERP_TYPE)ST_BASEOBJECT,		(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_BASEOBJECT,	OBJID_TARGET,
		scrBaseObjGet,			NULL },

	//returns psStruct->pStructureType->type
	{ "stattype",			VAL_INT,		(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_STRUCTURE,	OBJID_STRUCTSTATTYPE,
		scrBaseObjGet,			NULL },

	//returns if this unit is currently selected by a player (usually human)
	{ "selected",			VAL_BOOL,		(INTERP_TYPE)ST_OBJECT,	(INTERP_TYPE)ST_DROID,		OBJID_SELECTED,
		scrBaseObjGet,			NULL },

	/* This entry marks the end of the variable list */
	{ NULL, VAL_VOID, (INTERP_TYPE)ST_OBJECT, VAL_VOID, 0, NULL, NULL }
};

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
	{ "OPTIONS",	VAL_INT,	FALSE,	IDRET_OPTIONS,		NULL, NULL, 0.0f },
	{ "CANCEL",		VAL_INT,	FALSE,	IDRET_CANCEL,		NULL, NULL, 0.0f },
	{ "BUILD",		VAL_INT,	FALSE,	IDRET_BUILD,		NULL, NULL, 0.0f },
	{ "MANUFACTURE",VAL_INT,	FALSE,	IDRET_MANUFACTURE,	NULL, NULL, 0.0f },
	{ "RESEARCH",	VAL_INT,	FALSE,	IDRET_RESEARCH,		NULL, NULL, 0.0f },
	{ "INTELMAP",	VAL_INT,	FALSE,	IDRET_INTEL_MAP,	NULL, NULL, 0.0f },
	{ "DESIGN",		VAL_INT,	FALSE,	IDRET_DESIGN,		NULL, NULL, 0.0f },
	{ "COMMAND",	VAL_INT,	FALSE,	IDRET_COMMAND,		NULL, NULL, 0.0f },


	// new styley that supports many other buttons
	{ "IDRET_OPTIONS",		VAL_INT,	FALSE,		IDRET_OPTIONS,		0 },
	{ "IDRET_CANCEL",		VAL_INT,	FALSE,		IDRET_CANCEL,		0 },
	{ "IDRET_BUILD",		VAL_INT,	FALSE,		IDRET_BUILD,		0 },
	{ "IDRET_MANUFACTURE",	VAL_INT,	FALSE,		IDRET_MANUFACTURE,	0 },
	{ "IDRET_RESEARCH",		VAL_INT,	FALSE,		IDRET_RESEARCH,		0 },
	{ "IDRET_INTELMAP",		VAL_INT,	FALSE,		IDRET_INTEL_MAP,	0 },
	{ "IDRET_DESIGN",		VAL_INT,	FALSE,		IDRET_DESIGN,		0 },
	{ "IDRET_COMMAND",		VAL_INT,	FALSE,		IDRET_COMMAND,		0 },
	{ "IDRET_ORDER",		VAL_INT,	FALSE,		IDRET_ORDER,		0 },
	{ "IDRET_TRANSPORTER",	VAL_INT,	FALSE,		IDRET_TRANSPORTER,	0 },

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
	{ "NULLTEMPLATE",	(INTERP_TYPE)ST_POINTER_T,	0,		0,					NULL},
	{ "NULLOBJECT",		(INTERP_TYPE)ST_POINTER_O,	0,		0,					NULL},
	{ "NULLSTAT",		(INTERP_TYPE)ST_POINTER_S,	0,		0,					NULL},
	{ "NULLSTRING",		(INTERP_TYPE)ST_TEXTSTRING,	0,		0,					NULL},

	{ "NULLSTRUCTURESTAT",		(INTERP_TYPE)ST_POINTER_STRUCTSTAT,	0,	0,		NULL},	//for NULLSTRUCTURESTAT

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
	{ "CAMP_EXPAND",VAL_INT,	0,		LDS_EXPAND,		0},
	{ "OFF_KEEP",	VAL_INT,	0,		LDS_MKEEP,			0},
	{ "OFF_CLEAR",	VAL_INT,	0,		LDS_MCLEAR,			0},
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
	{ "DORDER_DROIDREPAIR",		VAL_INT,	0,		DORDER_DROIDREPAIR,			0 },	//new one


	//new member varialbe - constants for .action
	//-----------------------------------------------------------
	{ "DACTION_NONE",				VAL_INT,	0,		DACTION_NONE,				0 },
	{ "DACTION_MOVE",				VAL_INT,	0,		DACTION_MOVE,				0 },
	{ "DACTION_BUILD",				VAL_INT,	0,		DACTION_BUILD,				0 },
	{ "DACTION_BUILD_FOUNDATION",	VAL_INT,	0,		DACTION_BUILD_FOUNDATION,	0 },
	{ "DACTION_DEMOLISH",			VAL_INT,	0,		DACTION_DEMOLISH,			0 },
	{ "DACTION_REPAIR",				VAL_INT,	0,		DACTION_REPAIR,				0 },
	{ "DACTION_ATTACK",				VAL_INT,	0,		DACTION_ATTACK,				0 },
	{ "DACTION_OBSERVE",			VAL_INT,	0,		DACTION_OBSERVE,			0 },
	{ "DACTION_FIRESUPPORT",		VAL_INT,	0,		DACTION_FIRESUPPORT,		0 },
	{ "DACTION_SULK",				VAL_INT,	0,		DACTION_SULK,				0 },

	{ "DACTION_DESTRUCT",				VAL_INT,	0,		DACTION_DESTRUCT,				0 },
	{ "DACTION_TRANSPORTOUT",			VAL_INT,	0,		DACTION_TRANSPORTOUT,				0 },
	{ "DACTION_TRANSPORTWAITTOFLYIN",	VAL_INT,	0,		DACTION_TRANSPORTWAITTOFLYIN,				0 },
	{ "DACTION_TRANSPORTIN",			VAL_INT,	0,		DACTION_TRANSPORTIN,				0 },
	{ "DACTION_DROIDREPAIR",			VAL_INT,	0,		DACTION_DROIDREPAIR,				0 },
	{ "DACTION_RESTORE",				VAL_INT,	0,		DACTION_RESTORE,				0 },
	{ "DACTION_MOVEFIRE",				VAL_INT,	0,		DACTION_MOVEFIRE,				0 },
	{ "DACTION_MOVETOBUILD",			VAL_INT,	0,		DACTION_MOVETOBUILD,				0 },
	{ "DACTION_MOVETODEMOLISH",			VAL_INT,	0,		DACTION_MOVETODEMOLISH,				0 },

	{ "DACTION_MOVETOREPAIR",			VAL_INT,	0,		DACTION_MOVETOREPAIR,				0 },
	{ "DACTION_BUILDWANDER",			VAL_INT,	0,		DACTION_BUILDWANDER,				0 },
	{ "DACTION_FOUNDATION_WANDER",		VAL_INT,	0,		DACTION_FOUNDATION_WANDER,				0 },
	{ "DACTION_MOVETOATTACK",			VAL_INT,	0,		DACTION_MOVETOATTACK,				0 },
	{ "DACTION_ROTATETOATTACK",			VAL_INT,	0,		DACTION_ROTATETOATTACK,				0 },
	{ "DACTION_MOVETOOBSERVE",			VAL_INT,	0,		DACTION_MOVETOOBSERVE,				0 },
	{ "DACTION_WAITFORREPAIR",			VAL_INT,	0,		DACTION_WAITFORREPAIR,				0 },
	{ "DACTION_MOVETOREPAIRPOINT",		VAL_INT,	0,		DACTION_MOVETOREPAIRPOINT,				0 },
	{ "DACTION_WAITDURINGREPAIR",		VAL_INT,	0,		DACTION_WAITDURINGREPAIR,				0 },
	{ "DACTION_MOVETODROIDREPAIR",		VAL_INT,	0,		DACTION_MOVETODROIDREPAIR,				0 },

	{ "DACTION_MOVETORESTORE",			VAL_INT,	0,		DACTION_MOVETORESTORE,				0 },
	{ "DACTION_MOVETOREARM",			VAL_INT,	0,		DACTION_MOVETOREARM,				0 },
	{ "DACTION_WAITFORREARM",			VAL_INT,	0,		DACTION_WAITFORREARM,				0 },
	{ "DACTION_MOVETOREARMPOINT",		VAL_INT,	0,		DACTION_MOVETOREARMPOINT,				0 },
	{ "DACTION_WAITDURINGREARM",		VAL_INT,	0,		DACTION_WAITDURINGREARM,				0 },
	{ "DACTION_VTOLATTACK",				VAL_INT,	0,		DACTION_VTOLATTACK,				0 },
	{ "DACTION_CLEARREARMPAD",			VAL_INT,	0,		DACTION_CLEARREARMPAD,				0 },
	{ "DACTION_RETURNTOPOS",			VAL_INT,	0,		DACTION_RETURNTOPOS,				0 },
	{ "DACTION_FIRESUPPORT_RETREAT",	VAL_INT,	0,		DACTION_FIRESUPPORT_RETREAT,				0 },
	//Watermelon:scriptable variable string for DACTION_ATTACK_M,DACTION_MOVE_ENGAGE
	{ "DACTION_ATTACK_M",				VAL_INT,	0,		DACTION_ATTACK_M,				0 },


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

	{ "IMAGE_CURSOR_ECM",	 VAL_INT,	0, 		IMAGE_CURSOR_ECM,		0 },
	{ "IMAGE_CURSOR_REPAIR", VAL_INT,	0,		IMAGE_CURSOR_REPAIR, 0 },

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

	{ "INT_OPTION",			VAL_INT,	0,		INT_OPTION,0 },	// Option screen

	{ "INT_EDITSTAT",		VAL_INT,	0,		INT_EDITSTAT,0 },	// Stat screen up for placing objects

	{ "INT_EDIT",			VAL_INT,	0,		INT_EDIT,0 },		// Edit mode

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

//	{ "DMATCH",				VAL_INT,	0,		DMATCH,					0 },
	{ "CAMPAIGN",			VAL_INT,	0,		CAMPAIGN,				0 },
	{ "TEAMPLAY",			VAL_INT,	0,		TEAMPLAY,				0 },
	{ "SKIRMISH",			VAL_INT,	0,		SKIRMISH,				0 },

	{ "CAMP_CLEAN",			VAL_INT,	0,		CAMP_CLEAN,				0 },
	{ "CAMP_BASE",			VAL_INT,	0,		CAMP_BASE,				0 },
	{ "CAMP_WALLS",			VAL_INT,	0,		CAMP_WALLS,				0 },

	{ "NO_ALLIANCES",			VAL_INT,	0,		NO_ALLIANCES,				0 },
	{ "ALLIANCES",			VAL_INT,	0,		ALLIANCES,				0 },
	{ "ALLIANCES_TEAMS",			VAL_INT,	0,		ALLIANCES_TEAMS,				0 },

	/* some key constants */
	{ "KEY_LCTRL",			VAL_INT,	0,		KEY_LCTRL,				0 },
	{ "KEY_RCTRL",			VAL_INT,	0,		KEY_RCTRL,				0 },
	{ "KEY_LSHIFT",			VAL_INT,	0,		KEY_LSHIFT,				0 },
	{ "KEY_RSHIFT",			VAL_INT,	0,		KEY_RSHIFT,				0 },
	{ "KEY_LALT",				VAL_INT,	0,		KEY_LALT,					0 },
	{ "KEY_RALT",			VAL_INT,	0,		KEY_RALT,					0 },
	{ "KEY_SPACE",			VAL_INT,	0,		KEY_SPACE,				0 },
	{ "KEY_RETURN",		VAL_INT,	0,		KEY_RETURN,			0 },
	{ "KEY_ESC",				VAL_INT,	0,		KEY_ESC,					0 },
	{ "KEY_A",					VAL_INT,	0,		KEY_A,						0 },
	{ "KEY_B",					VAL_INT,	0,		KEY_B,						0 },
	{ "KEY_C",					VAL_INT,	0,		KEY_C,						0 },
	{ "KEY_D",					VAL_INT,	0,		KEY_D,						0 },
	{ "KEY_E",					VAL_INT,	0,		KEY_E,						0 },
	{ "KEY_F",					VAL_INT,	0,		KEY_F,						0 },
	{ "KEY_G",					VAL_INT,	0,		KEY_G,						0 },
	{ "KEY_H",					VAL_INT,	0,		KEY_H,						0 },
	{ "KEY_I",					VAL_INT,	0,		KEY_I,						0 },
	{ "KEY_J",					VAL_INT,	0,		KEY_J,						0 },
	{ "KEY_K",					VAL_INT,	0,		KEY_K,						0 },
	{ "KEY_L",					VAL_INT,	0,		KEY_L,						0 },
	{ "KEY_M",					VAL_INT,	0,		KEY_M,						0 },
	{ "KEY_N",					VAL_INT,	0,		KEY_N,						0 },
	{ "KEY_O",					VAL_INT,	0,		KEY_O,						0 },
	{ "KEY_P",					VAL_INT,	0,		KEY_P,						0 },
	{ "KEY_Q",					VAL_INT,	0,		KEY_Q,						0 },
	{ "KEY_R",					VAL_INT,	0,		KEY_R,						0 },
	{ "KEY_S",					VAL_INT,	0,		KEY_S,						0 },
	{ "KEY_T",					VAL_INT,	0,		KEY_T,						0 },
	{ "KEY_U",					VAL_INT,	0,		KEY_U,						0 },
	{ "KEY_V",					VAL_INT,	0,		KEY_V,						0 },
	{ "KEY_W",					VAL_INT,	0,		KEY_W,						0 },
	{ "KEY_X",					VAL_INT,	0,		KEY_X,						0 },
	{ "KEY_Y",					VAL_INT,	0,		KEY_Y,						0 },
	{ "KEY_Z",					VAL_INT,	0,		KEY_Z,						0 },

	/* This entry marks the end of the constant list */
	{ "CONSTANT LIST END",	VAL_VOID }
};

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
	{ "CALL_GAMEINIT",			(TRIGGER_TYPE)CALL_GAMEINIT,				NULL, 0 },
	{ "CALL_DELIVPOINTMOVED",		(TRIGGER_TYPE)CALL_DELIVPOINTMOVED,		NULL, 0 },
	{ "CALL_DROIDDESIGNED",			(TRIGGER_TYPE)CALL_DROIDDESIGNED,			NULL, 0 },
	//{ "CALL_RESEARCHCOMPLETED",		(TRIGGER_TYPE)CALL_RESEARCHCOMPLETED,		NULL, 0 }, - callback function added
	{ "CALL_DROIDBUILT",			(TRIGGER_TYPE)CALL_DROIDBUILT,			NULL, 0 },
	{ "CALL_POWERGEN_BUILT",		(TRIGGER_TYPE)CALL_POWERGEN_BUILT,		NULL, 0 },
	{ "CALL_RESEX_BUILT",			(TRIGGER_TYPE)CALL_RESEX_BUILT,			NULL, 0 },
	{ "CALL_RESEARCH_BUILT",		(TRIGGER_TYPE)CALL_RESEARCH_BUILT,		NULL, 0 },
	{ "CALL_FACTORY_BUILT",			(TRIGGER_TYPE)CALL_FACTORY_BUILT,			NULL, 0 },
	{ "CALL_MISSION_START",			(TRIGGER_TYPE)CALL_MISSION_START,			NULL, 0 },
	{ "CALL_MISSION_END",			(TRIGGER_TYPE)CALL_MISSION_END,			NULL, 0 },
	{ "CALL_VIDEO_QUIT",			(TRIGGER_TYPE)CALL_VIDEO_QUIT,			NULL, 0 },
	{ "CALL_LAUNCH_TRANSPORTER",	(TRIGGER_TYPE)CALL_LAUNCH_TRANSPORTER,	NULL, 0 },
	{ "CALL_START_NEXT_LEVEL",		(TRIGGER_TYPE)CALL_START_NEXT_LEVEL,		NULL, 0 },
	{ "CALL_TRANSPORTER_REINFORCE",	(TRIGGER_TYPE)CALL_TRANSPORTER_REINFORCE,	NULL, 0 },
	{ "CALL_MISSION_TIME",			(TRIGGER_TYPE)CALL_MISSION_TIME,			NULL, 0 },
	{ "CALL_ELECTRONIC_TAKEOVER",	(TRIGGER_TYPE)CALL_ELECTRONIC_TAKEOVER,	NULL, 0 },

	// tutorial only callbacks
	{ "CALL_BUILDLIST",				(TRIGGER_TYPE)CALL_BUILDLIST,				NULL, 0 },
	{ "CALL_BUILDGRID",				(TRIGGER_TYPE)CALL_BUILDGRID,				NULL, 0 },
	{ "CALL_RESEARCHLIST",			(TRIGGER_TYPE)CALL_RESEARCHLIST,			NULL, 0 },
	{ "CALL_MANURUN",				(TRIGGER_TYPE)CALL_MANURUN,				NULL, 0 },
	{ "CALL_MANULIST",				(TRIGGER_TYPE)CALL_MANULIST,				NULL, 0 },
	{ "CALL_BUTTON_PRESSED",		(TRIGGER_TYPE)CALL_BUTTON_PRESSED,		scrCBButtonPressed,
		1,	{ VAL_INT } },
	{ "CALL_DROID_SELECTED",		(TRIGGER_TYPE)CALL_DROID_SELECTED,		scrCBDroidSelected,
		1,	{ VAL_REF|(INTERP_TYPE)ST_DROID } },
	{ "CALL_DESIGN_QUIT",			(TRIGGER_TYPE)CALL_DESIGN_QUIT,			NULL, 0 },
	{ "CALL_DESIGN_WEAPON",			(TRIGGER_TYPE)CALL_DESIGN_WEAPON,			NULL, 0 },
	{ "CALL_DESIGN_SYSTEM",			(TRIGGER_TYPE)CALL_DESIGN_SYSTEM,			NULL, 0 },
	{ "CALL_DESIGN_COMMAND",		(TRIGGER_TYPE)CALL_DESIGN_COMMAND,		NULL, 0 },
	{ "CALL_DESIGN_BODY",			(TRIGGER_TYPE)CALL_DESIGN_BODY,			NULL, 0 },
	{ "CALL_DESIGN_PROPULSION",		(TRIGGER_TYPE)CALL_DESIGN_PROPULSION,		NULL, 0 },

	// callback triggers with parameters
    { "CALL_RESEARCHCOMPLETED",     (TRIGGER_TYPE)CALL_RESEARCHCOMPLETED, scrCBResCompleted,
		1,	{ VAL_REF|(INTERP_TYPE)ST_RESEARCH } },

	{ "CALL_NEWDROID",				(TRIGGER_TYPE)CALL_NEWDROID,			scrCBNewDroid,
		3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_DROID,VAL_REF|(INTERP_TYPE)ST_STRUCTURE	   } },

	{ "CALL_STRUCT_ATTACKED",		(TRIGGER_TYPE)CALL_STRUCT_ATTACKED,	scrCBStructAttacked,
		3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_STRUCTURE, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT } },

	{ "CALL_DROID_ATTACKED",		(TRIGGER_TYPE)CALL_DROID_ATTACKED,	scrCBDroidAttacked,
		3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_DROID, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT } },

	{ "CALL_ATTACKED",				(TRIGGER_TYPE)CALL_ATTACKED,			scrCBAttacked,
		3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT } },

	{ "CALL_STRUCT_SEEN",			(TRIGGER_TYPE)CALL_STRUCT_SEEN,		scrCBStructSeen,
		3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_STRUCTURE, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT } },

	{ "CALL_DROID_SEEN",			(TRIGGER_TYPE)CALL_DROID_SEEN,		scrCBDroidSeen,
		3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_DROID, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT } },

	{ "CALL_FEATURE_SEEN",			(TRIGGER_TYPE)CALL_FEATURE_SEEN,		scrCBFeatureSeen,
		3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_FEATURE, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT } },

	{ "CALL_OBJ_SEEN",				(TRIGGER_TYPE)CALL_OBJ_SEEN,			scrCBObjSeen,
		3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT } },

	{ "CALL_OBJ_DESTROYED",			(TRIGGER_TYPE)CALL_OBJ_DESTROYED,		scrCBObjDestroyed,
		2,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT } },

	{ "CALL_STRUCT_DESTROYED",		(TRIGGER_TYPE)CALL_STRUCT_DESTROYED,	scrCBStructDestroyed,
		2,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_STRUCTURE } },

	{ "CALL_DROID_DESTROYED",		(TRIGGER_TYPE)CALL_DROID_DESTROYED,	scrCBDroidDestroyed,
		2,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_DROID } },

	{ "CALL_FEATURE_DESTROYED",		(TRIGGER_TYPE)CALL_FEATURE_DESTROYED,	scrCBFeatureDestroyed,
		1,	{ VAL_REF|(INTERP_TYPE)ST_FEATURE } },

	{ "CALL_OBJECTOPEN",		(TRIGGER_TYPE)CALL_OBJECTOPEN, 	NULL, 0 },
	{ "CALL_OBJECTCLOSE",		(TRIGGER_TYPE)CALL_OBJECTCLOSE, 	NULL, 0 },

	{ "CALL_TRANSPORTER_OFFMAP",	(TRIGGER_TYPE)CALL_TRANSPORTER_OFFMAP,	scrCBTransporterOffMap,
		1,	{ VAL_INT } },
	{ "CALL_TRANSPORTER_LANDED",	(TRIGGER_TYPE)CALL_TRANSPORTER_LANDED,	scrCBTransporterLanded,
		2,	{ (INTERP_TYPE)ST_GROUP, VAL_INT } },

	{ "CALL_ALL_ONSCREEN_DROIDS_SELECTED",		(TRIGGER_TYPE)CALL_ALL_ONSCREEN_DROIDS_SELECTED, 	NULL, 0 },

    { "CALL_NO_REINFORCEMENTS_LEFT",    (TRIGGER_TYPE)CALL_NO_REINFORCEMENTS_LEFT,    NULL, 0 },

	{ "CALL_CLUSTER_EMPTY",				(TRIGGER_TYPE)CALL_CLUSTER_EMPTY,		scrCBClusterEmpty,
		1,	{ VAL_REF|VAL_INT } },

	{ "CALL_VTOL_OFF_MAP",				(TRIGGER_TYPE)CALL_VTOL_OFF_MAP,		scrCBVtolOffMap,
		2,  { VAL_INT, VAL_REF|(INTERP_TYPE)ST_DROID } },

	{ "CALL_UNITTAKEOVER",			(TRIGGER_TYPE)CALL_UNITTAKEOVER,		scrCBDroidTaken,
		1,	{VAL_REF|(INTERP_TYPE)ST_DROID} },

	{ "CALL_PLAYERLEFT",			(TRIGGER_TYPE)CALL_PLAYERLEFT,		scrCBPlayerLeft,
		1,	{ VAL_INT }},

	{ "CALL_ALLIANCEOFFER",			(TRIGGER_TYPE)CALL_ALLIANCEOFFER,		scrCBAllianceOffer,
		2,	{ VAL_REF|VAL_INT,VAL_REF|VAL_INT }},

		// new callbacks
		//-------------------------------------------------------------------------------

			//console callback
	{ "CALL_CONSOLE",			(TRIGGER_TYPE)CALL_CONSOLE,		scrCallConsole,
		2,	{ VAL_REF|VAL_INT,VAL_REF|VAL_STRING }},

		//59
	{ "CALL_AI_MSG",			(TRIGGER_TYPE)CALL_AI_MSG,		scrCallMultiMsg,
		3,	{ VAL_INT, VAL_REF|VAL_INT,VAL_REF|VAL_STRING }},

		//59
	{ "CALL_BEACON",			(TRIGGER_TYPE)CALL_BEACON,		scrCallBeacon,
		5,	{ VAL_INT, VAL_REF|VAL_INT, VAL_REF|VAL_INT,
		VAL_REF|VAL_INT, VAL_REF|VAL_STRING }},

	{ "CALL_STRUCTBUILT",				(TRIGGER_TYPE)CALL_STRUCTBUILT,			scrCBStructBuilt,
		3,	{ VAL_INT, VAL_REF|ST_DROID, VAL_REF|ST_STRUCTURE	   } },

		//new transporter landed callback
	{ "CALL_TRANSPORTER_LANDED_B",	(TRIGGER_TYPE)CALL_TRANSPORTER_LANDED_B,	scrCBTransporterLandedB,
		3,	{ (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_REF|ST_DROID } },

		//fired when droid received DORDER_STOP order
	{ "CALL_DORDER_STOP",	(TRIGGER_TYPE)CALL_DORDER_STOP,	scrCBDorderStop,
		2,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_DROID	   } },

		//fired when droid reached the destination and stopped on its own
	{ "CALL_DROID_REACH_LOCATION",	(TRIGGER_TYPE)CALL_DROID_REACH_LOCATION,	scrCBDorderReachedLocation,
		3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_DROID, VAL_REF | VAL_INT	   } },

	{ "CALL_KEY_PRESSED",	(TRIGGER_TYPE)CALL_KEY_PRESSED,	scrCBProcessKeyPress,
		2,	{ VAL_REF | VAL_INT, VAL_REF| VAL_INT} },

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
	{ (INTERP_TYPE)ST_BASEOBJECT,	3,	{ (INTERP_TYPE)ST_DROID, (INTERP_TYPE)ST_STRUCTURE, (INTERP_TYPE)ST_FEATURE, } },
	{ (INTERP_TYPE)ST_COMPONENT,		8,	{ (INTERP_TYPE)ST_BODY, (INTERP_TYPE)ST_PROPULSION, (INTERP_TYPE)ST_ECM, (INTERP_TYPE)ST_SENSOR, (INTERP_TYPE)ST_CONSTRUCT, (INTERP_TYPE)ST_WEAPON, (INTERP_TYPE)ST_REPAIR, (INTERP_TYPE)ST_BRAIN } },
	{ (INTERP_TYPE)ST_BASESTATS,		2,	{ (INTERP_TYPE)ST_STRUCTURESTAT, (INTERP_TYPE)ST_FEATURESTAT } },

	{ (INTERP_TYPE)ST_DROID,			1,	{ (INTERP_TYPE)ST_POINTER_O, } },
	{ (INTERP_TYPE)ST_STRUCTURE,		1,	{ (INTERP_TYPE)ST_POINTER_O, } },
	{ (INTERP_TYPE)ST_FEATURE,		1,	{ (INTERP_TYPE)ST_POINTER_O, } },
	{ (INTERP_TYPE)ST_BASEOBJECT,	1,	{ (INTERP_TYPE)ST_POINTER_O, } },
	{ (INTERP_TYPE)ST_TEMPLATE,		1,	{ (INTERP_TYPE)ST_POINTER_T, } },
	{ (INTERP_TYPE)ST_BODY,			1,  { (INTERP_TYPE)ST_POINTER_S, } },
	{ (INTERP_TYPE)ST_PROPULSION,	1,	{ (INTERP_TYPE)ST_POINTER_S, } },
	{ (INTERP_TYPE)ST_WEAPON,		1,	{ (INTERP_TYPE)ST_POINTER_S, } },
	{ (INTERP_TYPE)ST_ECM,			1,	{ (INTERP_TYPE)ST_POINTER_S, } },
	{ (INTERP_TYPE)ST_SENSOR,		1,	{ (INTERP_TYPE)ST_POINTER_S, } },
	{ (INTERP_TYPE)ST_CONSTRUCT,		1,	{ (INTERP_TYPE)ST_POINTER_S, } },
	{ (INTERP_TYPE)ST_REPAIR,		1,	{ (INTERP_TYPE)ST_POINTER_S, } },
	{ (INTERP_TYPE)ST_BRAIN,			1,	{ (INTERP_TYPE)ST_POINTER_S, } },
	{ (INTERP_TYPE)ST_STRUCTURESTAT,		1,	{ (INTERP_TYPE)ST_POINTER_STRUCTSTAT, } },


	/* This marks the end of the equivalence list */
	{ (INTERP_TYPE)0,0 }
};


// Initialise the script system
BOOL scrTabInitialise(void)
{
	EVENT_INIT	sInit;
	UDWORD	i;

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

	if (!eventInitValueFuncs((INTERP_TYPE)ST_MAXTYPE))
	{
		return FALSE;
	}

	scrvInitialise();

	// Set the constant table
	scriptSetConstTab(asConstantTable);

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
	if (!eventAddValueCreate((INTERP_TYPE)ST_BASEOBJECT, scrvAddBasePointer))
	{
		return FALSE;
	}
	if (!eventAddValueRelease((INTERP_TYPE)ST_BASEOBJECT, scrvReleaseBasePointer))
	{
		return FALSE;
	}

	if (!eventAddValueCreate((INTERP_TYPE)ST_DROID, scrvAddBasePointer))
	{
		return FALSE;
	}
	if (!eventAddValueRelease((INTERP_TYPE)ST_DROID, scrvReleaseBasePointer))
	{
		return FALSE;
	}

	if (!eventAddValueCreate((INTERP_TYPE)ST_STRUCTURE, scrvAddBasePointer))
	{
		return FALSE;
	}
	if (!eventAddValueRelease((INTERP_TYPE)ST_STRUCTURE, scrvReleaseBasePointer))
	{
		return FALSE;
	}

	if (!eventAddValueCreate((INTERP_TYPE)ST_FEATURE, scrvAddBasePointer))
	{
		return FALSE;
	}
	if (!eventAddValueRelease((INTERP_TYPE)ST_FEATURE, scrvReleaseBasePointer))
	{
		return FALSE;
	}

	if (!eventAddValueCreate((INTERP_TYPE)ST_GROUP, scrvNewGroup))
	{
		return FALSE;
	}
	if (!eventAddValueRelease((INTERP_TYPE)ST_GROUP, scrvReleaseGroup))
	{
		return FALSE;
	}

	// initialise various variables
	scrGameLevel = 0;
	bInTutorial = FALSE;

	/* Initialize debug output */
	for(i=0; i<DEBUGMENU_MAX_ENTRIES; i++)
		debugMenuEntry[i][0] = '\0';

	return TRUE;
}


// Shut down the script system
void scrShutDown(void)
{
	scrvShutDown();
	scriptShutDown();
}




