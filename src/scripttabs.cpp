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
#include "lib/netplay/netplay.h"
#include "lib/script/chat_processing.h"


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
 *      <number of parameters>,  <parameter types>,  false
 */
FUNC_SYMBOL asFuncTable[] =
{
	// These functions are part of the script library
	{ "traceOn",			interpTraceOn,			VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "traceOff",			interpTraceOff,			VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "setEventTrigger",	eventSetTrigger,		VAL_VOID,
		2, { VAL_EVENT, VAL_TRIGGER },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "eventTraceLevel",	eventSetTraceLevel,		VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	// Trigger functions
	{ "objectInRange",		scrObjectInRange,		VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "droidInRange",		scrDroidInRange,		VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "structInRange",		scrStructInRange,		VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "playerPower",		scrPlayerPower,			VAL_INT,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "objectInArea",		scrObjectInArea,		VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "droidInArea",		scrDroidInArea,			VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "structInArea",		scrStructInArea,		VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "seenStructInArea",	scrSeenStructInArea,	VAL_BOOL,
		7, { VAL_INT, VAL_INT, VAL_BOOL,VAL_INT,VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "structButNoWallsInArea",scrStructButNoWallsInArea,VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numObjectsInArea",	scrNumObjectsInArea,	VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numDroidsInArea",	scrNumDroidsInArea,		VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numStructsInArea",	scrNumStructsInArea,	VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numStructsButNotWallsInArea",scrNumStructsButNotWallsInArea,VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numStructsByTypeInArea",scrNumStructsByTypeInArea,VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },


	{ "droidHasSeen",		scrDroidHasSeen,		VAL_BOOL,
		2, { (INTERP_TYPE)ST_BASEOBJECT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "buildingDestroyed",	scrBuildingDestroyed,	VAL_BOOL,
		2, { (INTERP_TYPE)ST_STRUCTUREID, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "structureIdle",		scrStructureIdle,		VAL_BOOL,
		1, { (INTERP_TYPE)ST_STRUCTURE },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "initEnumStruct",		scrInitEnumStruct,		VAL_VOID,
		4, { VAL_BOOL, (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT,VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "enumStruct",			scrEnumStruct,			(INTERP_TYPE)ST_STRUCTURE,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "initEnumStructB",	scrInitEnumStructB,		VAL_VOID,
		5, { VAL_BOOL, (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT,VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "enumStructB",		scrEnumStructB,			(INTERP_TYPE)ST_STRUCTURE,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "structureBeingBuilt",scrStructureBeingBuilt,	VAL_BOOL,
		2,	{ (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "structureComplete",	scrStructureComplete,	VAL_BOOL,		// pc multiplayer only
		1,	{ (INTERP_TYPE)ST_STRUCTURE },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "structureBuilt",		scrStructureBuilt,		VAL_BOOL,
		2,	{ (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "anyDroidsLeft",		scrAnyDroidsLeft,		VAL_BOOL,
		1,	{ VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "anyStructButWallsLeft",scrAnyStructButWallsLeft,VAL_BOOL,
		1,	{ VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "anyFactoriesLeft",	scrAnyFactoriesLeft,	VAL_BOOL,
		1,	{ VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	// event functions
	{ "enableComponent",	scrEnableComponent,		VAL_VOID,
		2, { (INTERP_TYPE)ST_COMPONENT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "makeComponentAvailable",scrMakeComponentAvailable,VAL_VOID,
		2, { (INTERP_TYPE)ST_COMPONENT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "enableStructure",	scrEnableStructure,		VAL_VOID,
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "isStructureAvailable",scrIsStructureAvailable,VAL_BOOL,	// pc multiplay only
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "addDroid",			scrAddDroid,			(INTERP_TYPE)ST_DROID,
		4, { (INTERP_TYPE)ST_TEMPLATE, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "addDroidToMissionList",scrAddDroidToMissionList,(INTERP_TYPE)ST_DROID,
		2, { (INTERP_TYPE)ST_TEMPLATE, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "buildDroid",			scrBuildDroid,			VAL_VOID,
		4, { (INTERP_TYPE)ST_TEMPLATE, (INTERP_TYPE)ST_STRUCTURE, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "addTemplate",		scrAddTemplate,			VAL_BOOL,
		2, { (INTERP_TYPE)ST_TEMPLATE, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "addReticuleButton",	scrAddReticuleButton,	VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "removeReticuleButton",scrRemoveReticuleButton,VAL_VOID,
		2, { VAL_INT, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "addMessage",			scrAddMessage,			VAL_VOID,
		4, { (INTERP_TYPE)ST_INTMESSAGE, VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "removeMessage",		scrRemoveMessage,		VAL_VOID,
		3, { (INTERP_TYPE)ST_INTMESSAGE, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "selectDroidByID",	scrSelectDroidByID,		VAL_BOOL,
		2, { (INTERP_TYPE)ST_DROIDID, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setAssemblyPoint",	scrSetAssemblyPoint,	VAL_VOID,
		3, { (INTERP_TYPE)ST_STRUCTURE, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "attackLocation",		scrAttackLocation,		VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "initGetFeature",		scrInitGetFeature,		VAL_VOID,
		3, { (INTERP_TYPE)ST_FEATURESTAT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getFeature",			scrGetFeature,			(INTERP_TYPE)ST_FEATURE,
		1, { VAL_INT},
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getFeatureB",		scrGetFeatureB,			(INTERP_TYPE)ST_FEATURE,
		1, { VAL_INT},
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "addFeature",			scrAddFeature,			(INTERP_TYPE)ST_FEATURE,
		3, { (INTERP_TYPE)ST_FEATURESTAT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "destroyFeature",		scrDestroyFeature,		VAL_VOID,
		1, { (INTERP_TYPE)ST_FEATURE },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "addStructure",		scrAddStructure,		(INTERP_TYPE)ST_STRUCTURE,
		4, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "destroyStructure",	scrDestroyStructure,	VAL_VOID,
		1, { (INTERP_TYPE)ST_STRUCTURE },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "centreView",			scrCentreView,			VAL_VOID,
		1, { (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "centreViewPos",		scrCentreViewPos,		VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getStructure",		scrGetStructure,		(INTERP_TYPE)ST_STRUCTURE,
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getStructureVis",		scrGetStructureVis,		(INTERP_TYPE)ST_STRUCTURE,
		3, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getTemplate",		scrGetTemplate,			(INTERP_TYPE)ST_TEMPLATE,
		2, { (INTERP_TYPE)ST_COMPONENT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getDroid",			scrGetDroid,			(INTERP_TYPE)ST_DROID,
		2, { (INTERP_TYPE)ST_COMPONENT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setScrollParams",	scrSetScrollParams,		VAL_VOID,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setScrollMinX",		scrSetScrollMinX,		VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setScrollMinY",		scrSetScrollMinY,		VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setScrollMaxX",		scrSetScrollMaxX,		VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setScrollMaxY",		scrSetScrollMaxY,		VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setDefaultSensor",	scrSetDefaultSensor,	VAL_VOID,
		2, { (INTERP_TYPE)ST_SENSOR, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setDefaultECM",		scrSetDefaultECM,		VAL_VOID,
		2, { (INTERP_TYPE)ST_ECM, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setDefaultRepair",	scrSetDefaultRepair,	VAL_VOID,
		2, { (INTERP_TYPE)ST_REPAIR, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setStructureLimits",	scrSetStructureLimits,	VAL_VOID,
		3, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setAllStructureLimits",scrSetAllStructureLimits,VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "applyLimitSet",		scrApplyLimitSet,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "playSound",			scrPlaySound,			VAL_VOID,
		2, { (INTERP_TYPE)ST_SOUND, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "playSoundPos",		scrPlaySoundPos,		VAL_VOID,
		5, { (INTERP_TYPE)ST_SOUND, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "addConsoleText",		scrAddConsoleText,		VAL_VOID,
		2, { (INTERP_TYPE)ST_TEXTSTRING, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "showConsoleText",	scrShowConsoleText,		VAL_VOID,
		2, { (INTERP_TYPE)ST_TEXTSTRING, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "tagConsoleText",		scrTagConsoleText,		VAL_VOID,
		2, { (INTERP_TYPE)ST_TEXTSTRING, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "turnPowerOn",		scrTurnPowerOn,			VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "turnPowerOff",		scrTurnPowerOff,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "tutorialEnd",		scrTutorialEnd,			VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "clearConsole",		scrClearConsole,		VAL_VOID,
		0, { VAL_VOID},
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "playVideo",			scrPlayVideo,			VAL_VOID,
		2, { (INTERP_TYPE)ST_TEXTSTRING, (INTERP_TYPE)ST_TEXTSTRING },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "gameOverMessage",	scrGameOverMessage,		VAL_VOID,
		4, { (INTERP_TYPE)ST_INTMESSAGE, VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "gameOver",			scrGameOver,			VAL_VOID,
		1, { VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "playBackgroundAudio",scrPlayBackgroundAudio,	VAL_VOID,
		2, { (INTERP_TYPE)ST_TEXTSTRING, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "playIngameCDAudio",		scrPlayIngameCDAudio,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "stopCDAudio",		scrStopCDAudio,			VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "pauseCDAudio",		scrPauseCDAudio,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "resumeCDAudio",		scrResumeCDAudio,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setRetreatPoint",	scrSetRetreatPoint,		VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setRetreatForce",	scrSetRetreatForce,		VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setRetreatLeadership",scrSetRetreatLeadership,VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setGroupRetreatForce",scrSetGroupRetreatForce,VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setGroupRetreatLeadership",scrSetGroupRetreatLeadership,VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setGroupRetreatPoint",scrSetGroupRetreatPoint,VAL_VOID,
		3, { (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setRetreatHealth",	scrSetRetreatHealth,	VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setGroupRetreatHealth",scrSetGroupRetreatHealth,VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

    { "startMission",		scrStartMission,		VAL_VOID,
		2, { VAL_INT, (INTERP_TYPE)ST_LEVEL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setSnow",			scrSetSnow,				VAL_VOID,
		1, { VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setRain",			scrSetRain,				VAL_VOID,
		1, { VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setBackgroundFog",	scrSetBackgroundFog,	VAL_VOID,
		1, { VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setDepthFog",		scrSetDepthFog,			VAL_VOID,
		1, { VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setFogColour",		scrSetFogColour,		VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setTransporterExit",	scrSetTransporterExit,	VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "flyTransporterIn",	scrFlyTransporterIn,	VAL_VOID,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "addDroidToTransporter",scrAddDroidToTransporter,VAL_VOID,
		2, { (INTERP_TYPE)ST_DROID, (INTERP_TYPE)ST_DROID },
		0, 0, NULL, 0, 0, NULL, NULL },

	/*{ "endMission",		scrEndMission,			VAL_VOID,
		1, { VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },*/

	{ "structureBuiltInRange",scrStructureBuiltInRange,(INTERP_TYPE)ST_STRUCTURE,
		5, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "random",				scrRandom,				VAL_INT,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "randomiseSeed",		scrRandomiseSeed,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "enableResearch",		scrEnableResearch,		VAL_VOID,
		2, { (INTERP_TYPE)ST_RESEARCH, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "completeResearch",	scrCompleteResearch,	VAL_VOID,
		2, { (INTERP_TYPE)ST_RESEARCH, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "flashOn",			scrFlashOn,				VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "flashOff",			scrFlashOff,			VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setPowerLevel",		scrSetPowerLevel,		VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "addPower",		    scrAddPower,		    VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setLandingZone",		scrSetLandingZone,		VAL_VOID,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setLimboLanding",	scrSetLimboLanding,		VAL_VOID,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setNoGoArea",		scrSetNoGoArea,			VAL_VOID,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "initAllNoGoAreas",	scrInitAllNoGoAreas,	VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setRadarZoom",		scrSetRadarZoom,		VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setReinforcementTime",scrSetReinforcementTime,VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setMissionTime",		scrSetMissionTime,		VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "missionTimeRemaining",scrMissionTimeRemaining,VAL_INT,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

    { "flushConsoleMessages",scrFlushConsoleMessages,VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "pickStructLocation",	scrPickStructLocation,	VAL_BOOL,
		4, {(INTERP_TYPE)ST_STRUCTURESTAT, VAL_REF | VAL_INT, VAL_REF | VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "pickStructLocationB",	scrPickStructLocationB,	VAL_BOOL,
		5, {(INTERP_TYPE)ST_STRUCTURESTAT, VAL_REF | VAL_INT, VAL_REF | VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	// AI functions
	{ "groupAddDroid",		scrGroupAddDroid,		VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, (INTERP_TYPE)ST_DROID },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "groupAddArea",		scrGroupAddArea,		VAL_VOID,
		6, { (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT,VAL_INT, VAL_INT,VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "groupAddAreaNoGroup",	scrGroupAddAreaNoGroup,		VAL_VOID,
		6, { (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT,VAL_INT, VAL_INT,VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "groupAddGroup",		scrGroupAddGroup,		VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, (INTERP_TYPE)ST_GROUP },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "groupMember",		scrGroupMember,			VAL_BOOL,
		2, { (INTERP_TYPE)ST_GROUP, (INTERP_TYPE)ST_DROID },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "idleGroup",			scrIdleGroup,			VAL_INT,
		1, { (INTERP_TYPE)ST_GROUP },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "initIterateGroup",	scrInitIterateGroup,	VAL_VOID,
		1, { (INTERP_TYPE)ST_GROUP },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "iterateGroup",		scrIterateGroup,		(INTERP_TYPE)ST_DROID,
		1, { (INTERP_TYPE)ST_GROUP },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "droidLeaveGroup",	scrDroidLeaveGroup,		VAL_VOID,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "orderDroid",			scrOrderDroid,			VAL_VOID,
		2, { (INTERP_TYPE)ST_DROID, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "orderDroidLoc",		scrOrderDroidLoc,		VAL_VOID,
		4, { (INTERP_TYPE)ST_DROID, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "orderDroidObj",		scrOrderDroidObj,		VAL_VOID,
		3, { (INTERP_TYPE)ST_DROID, VAL_INT, (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "orderDroidStatsLoc",	scrOrderDroidStatsLoc,	VAL_VOID,
		5, { (INTERP_TYPE)ST_DROID, VAL_INT, (INTERP_TYPE)ST_BASESTATS, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "orderGroup",			scrOrderGroup,			VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "orderGroupLoc",		scrOrderGroupLoc,		VAL_VOID,
		4, { (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "orderGroupObj",		scrOrderGroupObj,		VAL_VOID,
		3, { (INTERP_TYPE)ST_GROUP, VAL_INT, (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setDroidSecondary",	scrSetDroidSecondary,	VAL_VOID,
		3, { (INTERP_TYPE)ST_DROID, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "setGroupSecondary",	scrSetGroupSecondary,	VAL_VOID,
		3, { (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "initIterateCluster", scrInitIterateCluster,	VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "iterateCluster",		scrIterateCluster,		(INTERP_TYPE)ST_BASEOBJECT,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	//Commander functions
	{ "cmdDroidAddDroid",	scrCmdDroidAddDroid,	VAL_VOID,
		2, { (INTERP_TYPE)ST_DROID, (INTERP_TYPE)ST_DROID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "cmdDroidMaxGroup",	scrCmdDroidMaxGroup,	VAL_INT,
		1, { (INTERP_TYPE)ST_DROID},
		0, 0, NULL, 0, 0, NULL, NULL },

	// a couple of example functions
	{ "debugBox",			scrNumMB,				VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "approxRoot",			scrApproxRoot,			VAL_INT,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "refTest",			scrRefTest,				VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	// geo funcs
	{ "distBetweenTwoPoints",scrDistanceTwoPts,		VAL_INT,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "losTwoObjects",		scrLOSTwoBaseObjects,	VAL_BOOL,
		3, { (INTERP_TYPE)ST_BASEOBJECT, (INTERP_TYPE)ST_BASEOBJECT, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "killStructsInArea",	scrDestroyStructuresInArea,	VAL_VOID,
		8, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getThreatInArea",	scrThreatInArea,		VAL_INT,
		10, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getNearestGateway",	scrGetNearestGateway,	VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_REF|VAL_INT, VAL_REF|VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setWaterTile",		scrSetWaterTile,		VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setRubbleTile",		scrSetRubbleTile,		VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setCampaignNumber",	scrSetCampaignNumber,	VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "testStructureModule",scrTestStructureModule, VAL_BOOL,
		3, {VAL_INT,(INTERP_TYPE)ST_STRUCTURE,VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "killDroidsInArea",	scrDestroyUnitsInArea,	VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "vanishUnit",			scrRemoveDroid,			VAL_VOID,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "forceDamageObject",	scrForceDamage,			VAL_VOID,
		2, { (INTERP_TYPE)ST_BASEOBJECT, VAL_INT},
		0, 0, NULL, 0, 0, NULL, NULL },

	//multiplayer stuff.
	{ "isHumanPlayer",		scrIsHumanPlayer,		VAL_BOOL,
	    1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "offerAlliance",		scrOfferAlliance,		VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "createAlliance",		scrCreateAlliance,		VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "breakAlliance",		scrBreakAlliance,		VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "allianceExists",		scrAllianceExists,		VAL_BOOL,
		0, {VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "allianceExistsBetween",scrAllianceExistsBetween,VAL_BOOL,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "playerInAlliance",	scrPlayerInAlliance,	VAL_BOOL,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "dominatingAlliance",	scrDominatingAlliance,	VAL_BOOL,
		0, { VAL_VOID } ,
		0, 0, NULL, 0, 0, NULL, NULL},

	{ "myResponsibility",	scrMyResponsibility,	VAL_BOOL,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	// object conversion routines
	{ "objToDroid",			scrObjToDroid,			(INTERP_TYPE)ST_DROID,
		1, { (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "objToStructure",		scrObjToStructure,		(INTERP_TYPE)ST_STRUCTURE,
		1, { (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "objToFeature",		scrObjToFeature,		(INTERP_TYPE)ST_FEATURE,
		1, { (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, NULL, 0, 0, NULL, NULL },


	{ "getGameStatus", 		scrGetGameStatus,		VAL_BOOL,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

    //player colour access functions
	{ "getPlayerColour",	scrGetPlayerColour,		VAL_INT,
        1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getPlayerColourName",scrGetPlayerColourName,	VAL_STRING,
        1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

    { "setPlayerColour",	scrSetPlayerColour,		VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

    { "takeOverDroidsInArea",scrTakeOverDroidsInArea,VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

    { "takeOverDroidsInAreaExp",scrTakeOverDroidsInAreaExp,VAL_INT,
		8, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

    { "takeOverStructsInArea",scrTakeOverStructsInArea,VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

    { "takeOverSingleDroid",scrTakeOverSingleDroid,	(INTERP_TYPE)ST_DROID,
		2, { (INTERP_TYPE)ST_DROID, VAL_INT},
		0, 0, NULL, 0, 0, NULL, NULL },

    { "takeOverSingleStructure",scrTakeOverSingleStructure,(INTERP_TYPE)ST_STRUCTURE,
		2, { (INTERP_TYPE)ST_STRUCTURE, VAL_INT},
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "resetStructTargets",	scrResetStructTargets,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "resetDroidTargets",	scrResetDroidTargets,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "setStructTarPref",	scrSetStructTarPref,		VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "setStructTarIgnore",	scrSetStructTarIgnore,		VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "setDroidTarPref",	scrSetDroidTarPref,			VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "setDroidTarIgnore",	scrSetDroidTarIgnore,		VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "structTargetInArea",	scrStructTargetInArea,		(INTERP_TYPE)ST_STRUCTURE,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "structTargetOnMap",	scrStructTargetOnMap,		(INTERP_TYPE)ST_STRUCTURE,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "droidTargetInArea",	scrDroidTargetInArea,		(INTERP_TYPE)ST_DROID,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "droidTargetOnMap",	scrDroidTargetOnMap,		(INTERP_TYPE)ST_DROID,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "targetInCluster",	scrTargetInCluster,			(INTERP_TYPE)ST_DROID,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setDroidsToSafetyFlag",scrSetDroidsToSafetyFlag,	VAL_VOID,
		1, { VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setPlayCountDown",	scrSetPlayCountDown,	    VAL_VOID,
		1, { VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getDroidCount",		scrGetDroidCount,		    VAL_INT,
        1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "fireWeaponAtObj",	scrFireWeaponAtObj,			VAL_VOID,
		2, { (INTERP_TYPE)ST_WEAPON, (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "fireWeaponAtLoc",	scrFireWeaponAtLoc,			VAL_VOID,
		3, { (INTERP_TYPE)ST_WEAPON, VAL_INT,VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setDroidKills",		scrSetDroidKills,			VAL_VOID,
		2, { (INTERP_TYPE)ST_DROID, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "getDroidKills",		scrGetDroidKills,			VAL_INT,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "resetPlayerVisibility",scrResetPlayerVisibility,	VAL_VOID,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setVTOLReturnPos",	scrSetVTOLReturnPos,		VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "isVtol",				scrIsVtol,					VAL_BOOL,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "tutorialTemplates",	scrTutorialTemplates,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "resetLimboMission",	scrResetLimboMission,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

//ajl skirmish funcs
	{ "skDoResearch",		scrSkDoResearch,			VAL_VOID,
		3, { (INTERP_TYPE)ST_STRUCTURE, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "skLocateEnemy",		scrSkLocateEnemy,			(INTERP_TYPE)ST_BASEOBJECT,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "skCanBuildTemplate",	scrSkCanBuildTemplate,		VAL_BOOL,
		3, { VAL_INT, (INTERP_TYPE)ST_STRUCTURE, (INTERP_TYPE)ST_TEMPLATE },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "skVtolEnableCheck",	scrSkVtolEnableCheck,		VAL_BOOL,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "skGetFactoryCapacity",scrSkGetFactoryCapacity,	VAL_INT,
	1, { (INTERP_TYPE)ST_STRUCTURE },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "skDifficultyModifier",scrSkDifficultyModifier,	VAL_VOID,
	1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "skDefenseLocation",	scrSkDefenseLocation,		VAL_BOOL,
	6, {VAL_REF|VAL_INT, VAL_REF|VAL_INT, (INTERP_TYPE)ST_STRUCTURESTAT, (INTERP_TYPE)ST_STRUCTURESTAT, (INTERP_TYPE)ST_DROID,VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "skFireLassat",		scrSkFireLassat,			VAL_VOID,
	2, {VAL_INT, (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, NULL, 0, 0, NULL, NULL },

	/* New functions */

	{ "strcmp",				scrStrcmp,					VAL_BOOL,
		2, { VAL_STRING, VAL_STRING },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "console",			scrConsole,					VAL_VOID,
		1, { VAL_STRING },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "MsgBox",				scrMsgBox,					VAL_VOID,
		1, { VAL_STRING },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "dbgMsgOn",			scrDbgMsgOn,				VAL_VOID,
		2, { VAL_INT, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "dbg",				scrDbg,						VAL_VOID,
		2, { VAL_STRING, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },
	{ "msg",				scrMsg,						VAL_VOID, //multiplayer msg
		3, { VAL_STRING, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "debug",				scrDebugFile,				VAL_VOID,
		1, { VAL_STRING },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getBit",				scrGetBit,					VAL_BOOL,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setBit",				scrSetBit,					VAL_INT,
		3, { VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "dropBeacon",			scrDropBeacon,				VAL_VOID,
		6, { VAL_STRING, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "removeBeacon",	scrRemoveBeacon,		VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	//droid functions
	//-----------------------------------
	{ "actionDroidObj",		scrActionDroidObj,			VAL_VOID,
		3, { (INTERP_TYPE)ST_DROID, VAL_INT, (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "InitEnumDroids",		scrInitEnumDroids,			VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "EnumDroid",			scrEnumDroid,				(INTERP_TYPE)ST_DROID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "initIterateGroupB",	scrInitIterateGroupB,		VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "iterateGroupB",		scrIterateGroupB,			(INTERP_TYPE)ST_DROID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "closestDamagedGroupDroid",scrClosestDamagedGroupDroid,(INTERP_TYPE)ST_DROID,
		6, { VAL_INT, (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },


	{ "factoryGetTemplate",	scrFactoryGetTemplate,		(INTERP_TYPE)ST_TEMPLATE,
		1, { (INTERP_TYPE)ST_STRUCTURE },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numTemplatesInProduction",scrNumTemplatesInProduction,VAL_INT,
		2, { (INTERP_TYPE)ST_TEMPLATE, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numDroidsByComponent",scrNumDroidsByComponent,	VAL_INT,
		3, { (INTERP_TYPE)ST_COMPONENT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getStructureLimit",	scrGetStructureLimit,		VAL_INT,
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "structureLimitReached",scrStructureLimitReached,	VAL_BOOL,
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getNumStructures",	scrGetNumStructures,		VAL_INT,
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getUnitLimit",		scrGetUnitLimit,			VAL_INT,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "min",				scrMin,						VAL_INT,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "max",				scrMax,						VAL_INT,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	//float equivalence for min()
	{ "fmin",				scrFMin,					VAL_FLOAT,
		2, { VAL_FLOAT, VAL_FLOAT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "fmax",				scrFMax,					VAL_FLOAT,
		2, { VAL_FLOAT, VAL_FLOAT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "fogTileInRange",		scrFogTileInRange,			VAL_BOOL,
		9, { VAL_REF|VAL_INT, VAL_REF|VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },


	{ "mapRevealedInRange",	scrMapRevealedInRange,		VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "mapTileVisible",		scrMapTileVisible,			VAL_BOOL,
		3, { VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "pursueResearch",		scrPursueResearch,			VAL_BOOL,
		3, { (INTERP_TYPE)ST_STRUCTURE, VAL_INT, (INTERP_TYPE)ST_RESEARCH },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numResearchLeft",	scrNumResearchLeft,			VAL_INT,
		2, { VAL_INT, (INTERP_TYPE)ST_RESEARCH },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "researchFinished",	scrResearchCompleted,		VAL_BOOL,
		2, { (INTERP_TYPE)ST_RESEARCH, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "researchStarted",	scrResearchStarted,			VAL_BOOL,
		2, { (INTERP_TYPE)ST_RESEARCH, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "threatInRange",		scrThreatInRange,			VAL_BOOL,
		5, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numEnemyWeapObjInRange",scrNumEnemyWeapObjInRange,VAL_INT,
		6, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numEnemyWeapDroidsInRange",scrNumEnemyWeapDroidsInRange,VAL_INT,
		5, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numEnemyWeapStructsInRange",scrNumEnemyWeapStructsInRange,VAL_INT,
		5, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numFriendlyWeapObjInRange",scrNumFriendlyWeapObjInRange,VAL_INT,
		6, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numFriendlyWeapDroidsInRange",scrNumFriendlyWeapDroidsInRange,VAL_INT,
		5, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numFriendlyWeapStructsInRange",scrNumFriendlyWeapStructsInRange,VAL_INT,
		5, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numPlayerWeapObjInRange",scrNumPlayerWeapObjInRange,VAL_INT,
		7, { VAL_INT, VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numPlayerWeapDroidsInRange",	scrNumPlayerWeapDroidsInRange,		VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numPlayerWeapStructsInRange",scrNumPlayerWeapStructsInRange,		VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numEnemyObjInRange",		scrNumEnemyObjInRange,			VAL_INT,
		6, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "enemyWeapObjCostInRange",scrEnemyWeapObjCostInRange,VAL_INT,
		6, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "friendlyWeapObjCostInRange",scrFriendlyWeapObjCostInRange,VAL_INT,
		6, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "alliancesLocked",		scrAlliancesLocked,		VAL_BOOL,
		0, {VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

//num structures
	{ "numStructsByStatInRange",scrNumStructsByStatInRange,VAL_INT,
		6, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT , VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numStructsByStatInArea",	scrNumStructsByStatInArea,VAL_INT,
		7, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT , VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numStructsByTypeInRange",scrNumStructsByTypeInRange,VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numFeatByTypeInRange",	scrNumFeatByTypeInRange,VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numStructsButNotWallsInRangeVis",scrNumStructsButNotWallsInRangeVis,VAL_INT,
		5, { VAL_INT , VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "chooseValidLoc",			scrChooseValidLoc,		VAL_BOOL,
		6, { VAL_REF|VAL_INT, VAL_REF|VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "transporterCapacity",	scrTransporterCapacity,	VAL_INT,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "transporterFlying",		scrTransporterFlying,	VAL_BOOL,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "unloadTransporter",		scrUnloadTransporter,	VAL_VOID,
		3, { (INTERP_TYPE)ST_DROID, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "hasGroup",				scrHasGroup,			VAL_BOOL,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "objWeaponMaxRange",		scrObjWeaponMaxRange,	VAL_INT,
		1, { (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "objHasWeapon",			scrObjHasWeapon,		VAL_BOOL,
		1, { (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "objectHasIndirectWeapon",scrObjectHasIndirectWeapon,VAL_BOOL,
		1, { (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getClosestEnemyDroidByType",scrGetClosestEnemyDroidByType,(INTERP_TYPE)ST_DROID,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_BOOL, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getClosestEnemyStructByType",scrGetClosestEnemyStructByType,(INTERP_TYPE)ST_STRUCTURE,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getClosestEnemy",		scrGetClosestEnemy,		(INTERP_TYPE)ST_BASEOBJECT,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "skDefenseLocationB",		scrSkDefenseLocationB,		VAL_BOOL,
	6, {VAL_REF|VAL_INT, VAL_REF|VAL_INT, (INTERP_TYPE)ST_STRUCTURESTAT, (INTERP_TYPE)ST_STRUCTURESTAT, (INTERP_TYPE)ST_DROID, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "circlePerimPoint",		scrCirclePerimPoint,		VAL_VOID,
		5, { VAL_INT, VAL_INT, VAL_REF|VAL_INT, VAL_REF|VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "structInRangeVis",		scrStructInRangeVis,		VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "droidInRangeVis",		scrDroidInRangeVis,			VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "giftRadar",				scrGiftRadar,				VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numAllies",				scrNumAllies,				VAL_INT,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "numAAinRange",			scrNumAAinRange,			VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "selectDroid",			scrSelectDroid,				VAL_VOID,
		2, { (INTERP_TYPE)ST_DROID, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "selectGroup",			scrSelectGroup,				VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "modulo",					scrModulo,					VAL_INT,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "playerLoaded",			scrPlayerLoaded,			VAL_BOOL,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getStructureType",		scrGetStructureType,		VAL_INT,
		1, { (INTERP_TYPE)ST_STRUCTURE },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getPlayerName",			scrGetPlayerName,			VAL_STRING,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setPlayerName",			scrSetPlayerName,			VAL_BOOL,
		2, { VAL_INT, VAL_STRING },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "ASSERT",					scrASSERT,					VAL_VOID,
		3, { VAL_BOOL, VAL_STRING, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "showRangeAtPos",			scrShowRangeAtPos,			VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "toPow",					scrToPow,					VAL_FLOAT,
		2, { VAL_FLOAT, VAL_FLOAT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "exp",				scrExp,							VAL_FLOAT,
		1, { VAL_FLOAT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "sqrt",				scrSqrt,						VAL_FLOAT,
		1, { VAL_FLOAT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "log",				scrLog,						VAL_FLOAT,
		1, { VAL_FLOAT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "debugMenu",				scrDebugMenu,				VAL_VOID,
		1, { VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setDebugMenuEntry",		scrSetDebugMenuEntry,		VAL_VOID,
		2, { VAL_STRING, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	//scripting engine chat interface functions
	{ "processChatMsg",			scrProcessChatMsg,			VAL_INT,
		1, { VAL_STRING },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getChatCmdDescription",	scrGetChatCmdDescription,	VAL_STRING,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getNumArgsInCmd",		scrGetNumArgsInCmd,			VAL_INT,
		1, { VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getChatCmdParam",		scrGetChatCmdParam,			VAL_BOOL,
		3, { VAL_REF | VAL_VOID, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "chatCmdIsPlayerAddressed",	scrChatCmdIsPlayerAddressed,	VAL_BOOL,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "setTileHeight",			scrSetTileHeight,			VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getTileStructure",		scrGetTileStructure,		(INTERP_TYPE)ST_STRUCTURE,
		2, { VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "printCallStack",		scrPrintCallStack,				VAL_VOID,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "debugModeEnabled",		scrDebugModeEnabled,		VAL_BOOL,
		0, { VAL_VOID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "calcDroidPower",			scrCalcDroidPower,			VAL_INT,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getDroidRank",			scrGetDroidLevel,			VAL_INT,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "assembleWeaponTemplate",		scrAssembleWeaponTemplate,		(INTERP_TYPE)ST_TEMPLATE,
		4, { VAL_INT, (INTERP_TYPE)ST_BODY, (INTERP_TYPE)ST_PROPULSION, (INTERP_TYPE)ST_WEAPON },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "weaponShortHitUpgrade",	scrWeaponShortHitUpgrade,	VAL_INT,
		2, { VAL_INT, (INTERP_TYPE)ST_WEAPON },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "weaponLongHitUpgrade",	scrWeaponLongHitUpgrade,	VAL_INT,
		2, { VAL_INT, (INTERP_TYPE)ST_WEAPON },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "weaponDamageUpgrade",	scrWeaponDamageUpgrade,		VAL_INT,
		2, { VAL_INT, (INTERP_TYPE)ST_WEAPON },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "weaponFirePauseUpgrade",	scrWeaponFirePauseUpgrade,	VAL_INT,
		2, { VAL_INT, (INTERP_TYPE)ST_WEAPON },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "isComponentAvailable",	scrIsComponentAvailable,	VAL_BOOL,
		2, { VAL_INT, (INTERP_TYPE)ST_COMPONENT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "getBodySize",			scrGetBodySize,				VAL_INT,
		1, { (INTERP_TYPE)ST_BODY },
		0, 0, NULL, 0, 0, NULL, NULL },

	// Gettext functions
	{ "_", scrGettext, (INTERP_TYPE)ST_TEXTSTRING,
		1, { VAL_STRING },
		false, 0, NULL, 0, 0, NULL, NULL },

	{ "gettext", scrGettext, (INTERP_TYPE)ST_TEXTSTRING,
		1, { VAL_STRING },
		false, 0, NULL, 0, 0, NULL, NULL },

	{ "N_", scrGettext_noop, VAL_STRING,
		1, { VAL_STRING },
		false, 0, NULL, 0, 0, NULL, NULL },

	{ "gettext_noop", scrGettext_noop, VAL_STRING,
		1, { VAL_STRING },
		false, 0, NULL, 0, 0, NULL, NULL },

	{ "P_", scrPgettext, (INTERP_TYPE)ST_TEXTSTRING,
		2, { VAL_STRING, VAL_STRING },
		false, 0, NULL, 0, 0, NULL, NULL },

	{ "pgettext", scrPgettext, (INTERP_TYPE)ST_TEXTSTRING,
		2, { VAL_STRING, VAL_STRING },
		false, 0, NULL, 0, 0, NULL, NULL },

	{ "PE_", scrPgettext_expr, (INTERP_TYPE)ST_TEXTSTRING,
		2, { VAL_STRING, VAL_STRING },
		false, 0, NULL, 0, 0, NULL, NULL },

	{ "NP_", scrPgettext_noop, VAL_STRING,
		2, { VAL_STRING, VAL_STRING },
		false, 0, NULL, 0, 0, NULL, NULL },

	{ "droidCanReach", scrDroidCanReach, VAL_BOOL,
		3, { (INTERP_TYPE)ST_DROID, VAL_INT, VAL_INT },
		false, 0, NULL, 0, 0, NULL, NULL },

	{ "getPlayer", scrGetPlayer, VAL_INT,
		1, { VAL_STRING },
		false, 0, NULL, 0, 0, NULL, NULL },

	{ "getPlayerStartPosition", scrGetPlayerStartPosition, VAL_BOOL,
		3, { VAL_INT, VAL_INT|VAL_REF, VAL_INT|VAL_REF },
		false, 0, NULL, 0, 0, NULL, NULL },

	{ "scavengersActive", scrScavengersActive, VAL_INT,
		0, { VAL_VOID },
		false, 0, NULL, 0, 0, NULL, NULL },

	{ "initEnumUnbuilt", scrEnumUnbuilt, VAL_VOID, 1, { VAL_INT }, false, 0, NULL, 0, 0, NULL, NULL },
	{ "enumUnbuilt", scrIterateUnbuilt, (INTERP_TYPE)ST_STRUCTURE, 0, { VAL_VOID }, false, 0, NULL, 0, 0, NULL, NULL },

	{ "pickDroidStructLocation",	scrPickStructLocationC,	VAL_BOOL,
		6, {(INTERP_TYPE)ST_DROID, (INTERP_TYPE)ST_STRUCTURESTAT, VAL_REF | VAL_INT, VAL_REF | VAL_INT, VAL_INT, VAL_INT },
		0, 0, NULL, 0, 0, NULL, NULL },

	{ "safeDest", scrSafeDest, VAL_BOOL,
		3, { VAL_INT, VAL_INT, VAL_INT },
		false, 0, NULL, 0, 0, NULL, NULL },

	{ "threatAt", scrThreatAt, VAL_BOOL,
		3, { VAL_INT, VAL_INT, VAL_INT },
		false, 0, NULL, 0, 0, NULL, NULL },

	{ "setSunPosition", scrSetSunPosition, VAL_VOID,
		3, { VAL_FLOAT, VAL_FLOAT, VAL_FLOAT },
		false, 0, NULL, 0, 0, NULL, NULL },

	{ "setSunIntensity", scrSetSunIntensity, VAL_VOID,
		9, { VAL_FLOAT, VAL_FLOAT, VAL_FLOAT, VAL_FLOAT, VAL_FLOAT, VAL_FLOAT, VAL_FLOAT, VAL_FLOAT, VAL_FLOAT },
		false, 0, NULL, 0, 0, NULL, NULL },

	{ "getDifficulty", scrGetDifficulty, VAL_INT,
		1, { VAL_INT },
		false, 0, NULL, 0, 0, NULL, NULL },

	{ "getDerrick", scrGetDerrick, (INTERP_TYPE)ST_BASEOBJECT,
		1, { VAL_INT },
		false, 0, NULL, 0, 0, NULL, NULL },

	/* This final entry marks the end of the function list */
	{ "FUNCTION LIST END", NULL, VAL_VOID, 0, { VAL_VOID }, 0, 0, NULL, 0, 0, NULL, NULL }
};


/*
 * The table of external variables and their access functions.
 * The table is laid out as follows :
 *
 *   "variable_identifier", <variable type>, (INTERP_TYPE)ST_EXTERN, 0, <index>,
 *		<get_function>, <set_function>
 *
 * The Storage type for an external variable is always ST_EXTERN.
 * The index is not used by the compiler but is passed to the access function
 * to allow one function to deal with a number of variables
 */
VAR_SYMBOL asExternTable[] =
{
	{ "trackTransporter",VAL_BOOL,	ST_EXTERN,	0,
		EXTID_TRACKTRANSPORTER,	scrGenExternGet,	NULL, 0, {0}, NULL },

	{ "mapWidth",		VAL_INT,	ST_EXTERN,	0,
		EXTID_MAPWIDTH,		scrGenExternGet,		NULL, 0, {0}, NULL },
	{ "mapHeight",		VAL_INT,	ST_EXTERN,	0,
		EXTID_MAPHEIGHT,	scrGenExternGet,		NULL, 0, {0}, NULL },

	{ "gameInitialised",VAL_BOOL,	ST_EXTERN,	0,
		EXTID_GAMEINIT,		scrGenExternGet,		NULL, 0, {0}, NULL },

	{ "selectedPlayer",	VAL_INT,	ST_EXTERN,	0,
		EXTID_SELECTEDPLAYER,	scrGenExternGet,	NULL, 0, {0}, NULL },
	{ "gameTime",		VAL_INT,	ST_EXTERN,	0,
		EXTID_GAMETIME,		scrGenExternGet,		NULL, 0, {0}, NULL },

	{ "gameLevel",		VAL_INT,	ST_EXTERN,	0,
		EXTID_GAMELEVEL,	scrGenExternGet,		scrGenExternSet, 0, {0}, NULL },
	{ "inTutorial",		VAL_BOOL,	ST_EXTERN,	0,
		EXTID_TUTORIAL,		scrGenExternGet,		scrGenExternSet, 0, {0}, NULL },
	{ "cursorType",		VAL_INT,	ST_EXTERN,	0,
		EXTID_CURSOR,		scrGenExternGet,		NULL, 0, {0}, NULL },


	{ "intMode",		VAL_INT,	ST_EXTERN,	0,
		EXTID_INTMODE,		scrGenExternGet,		NULL, 0, {0}, NULL },

	{ "targetedObjectType",	VAL_INT,ST_EXTERN,	0,
		EXTID_TARGETTYPE,	scrGenExternGet,		NULL, 0, {0}, NULL },
	{ "extraVictoryFlag",VAL_BOOL,	ST_EXTERN,	0,
		EXTID_EXTRAVICTORYFLAG,	scrGenExternGet,	scrGenExternSet, 0, {0}, NULL },
	{ "extraFailFlag",	VAL_BOOL,	ST_EXTERN,	0,
		EXTID_EXTRAFAILFLAG,scrGenExternGet,		scrGenExternSet, 0, {0}, NULL },

	{ "multiPlayerGameType",VAL_INT,ST_EXTERN,	0,
		EXTID_MULTIGAMETYPE,scrGenExternGet,		NULL, 0, {0}, NULL },

	{ "multiPlayerMaxPlayers",VAL_INT,ST_EXTERN,0,
		EXTID_MULTIGAMEHUMANMAX,scrGenExternGet,	NULL, 0, {0}, NULL },

	{ "multiPlayerBaseType",VAL_INT,ST_EXTERN,	0,
		EXTID_MULTIGAMEBASETYPE,scrGenExternGet,	NULL, 0, {0}, NULL },

	{ "multiPlayerAlliancesType",VAL_INT,ST_EXTERN,0,
		EXTID_MULTIGAMEALLIANCESTYPE,scrGenExternGet,NULL, 0, {0}, NULL },

	/* This entry marks the end of the variable list */
	{ NULL, VAL_VOID, ST_EXTERN, 0, 0, NULL, NULL, 0, {0}, NULL }
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
	{ "x",			VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_POSX,		scrBaseObjGet,	NULL, 0, {0}, NULL },
	{ "y",			VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_POSY,		scrBaseObjGet,	NULL, 0, {0}, NULL },
	{ "z",			VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_POSZ,		scrBaseObjGet,	NULL, 0, {0}, NULL },
	{ "id",			VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_ID,		scrBaseObjGet,	NULL, 0, {0}, NULL },
	{ "player",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_PLAYER,	scrBaseObjGet,	NULL, 0, {0}, NULL },
	{ "type",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_TYPE,		scrBaseObjGet,	NULL, 0, {0}, NULL },
	{ "clusterID",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_CLUSTERID,scrBaseObjGet,	NULL, 0, {0}, NULL },
	{ "health",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_HEALTH,	scrBaseObjGet,	NULL, 0, {0}, NULL },
	{ "weapon",		(INTERP_TYPE)ST_WEAPON,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,		OBJID_WEAPON,	scrBaseObjGet,	NULL, 0, {0}, NULL },

	// object's hit points
	{ "hitPoints",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_HITPOINTS,	scrBaseObjGet,	NULL, 0, {0}, NULL },

	// object's original hit points (when not damaged)
	{ "origHitPoints",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_ORIG_HITPOINTS,	scrBaseObjGet,	NULL, 0, {0}, NULL },

	// droid variables
	{ "order",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_ORDER,	scrBaseObjGet,	NULL, 0, {0}, NULL },
	{ "droidType",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_DROIDTYPE,scrBaseObjGet,	NULL, 0, {0}, NULL },
	{ "body",		(INTERP_TYPE)ST_BODY,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_BODY,		scrBaseObjGet,	NULL, 0, {0}, NULL },
	{ "propulsion",	(INTERP_TYPE)ST_PROPULSION,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_PROPULSION,scrBaseObjGet,	NULL, 0, {0}, NULL },
	{ "orderx",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_ORDERX,	scrBaseObjGet,	NULL, 0, {0}, NULL },
	{ "ordery",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_ORDERY,	scrBaseObjGet,	NULL, 0, {0}, NULL },
	{ "group",		(INTERP_TYPE)ST_GROUP,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_GROUP,	scrBaseObjGet,	NULL, 0, {0}, NULL },

	// structure variables
	//{ "stat",			(INTERP_TYPE)ST_STRUCTURESTAT,	ST_OBJECT,	(INTERP_TYPE)ST_STRUCTURE,	OBJID_STRUCTSTAT,
	//	scrBaseObjGet,			NULL, 0, {0}, NULL },


	// group variables
	{ "x",			VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_GROUP,		GROUPID_POSX,	scrGroupObjGet,	NULL, 0, {0}, NULL },
	{ "y",			VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_GROUP,		GROUPID_POSY,	scrGroupObjGet,	NULL, 0, {0}, NULL },
	{ "members",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_GROUP,		GROUPID_MEMBERS,scrGroupObjGet,	NULL, 0, {0}, NULL },
	{ "health",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_GROUP,		GROUPID_HEALTH,	scrGroupObjGet,	NULL, 0, {0}, NULL },
	{ "type",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_GROUP,		GROUPID_TYPE,	scrGroupObjGet,	NULL, 0, {0}, NULL },
	{ "commander",	(INTERP_TYPE)ST_DROID,	ST_OBJECT,
		(INTERP_TYPE)ST_GROUP,		GROUPID_CMD,	scrGroupObjGet,	NULL, 0, {0}, NULL },

	/* new member variables */
	//similar to .order
	{ "action",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_ACTION,	scrBaseObjGet,	NULL, 0, {0}, NULL },

	//.stat - now supports droids, ST_STRUCTURE became ST_BASEOBJECT
	{ "stat",		(INTERP_TYPE)ST_STRUCTURESTAT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_STRUCTSTAT,scrBaseObjGet,	NULL, 0, {0}, NULL },

	//object->psTarget
	{ "target",		(INTERP_TYPE)ST_BASEOBJECT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_TARGET,	scrBaseObjGet,	NULL, 0, {0}, NULL },

	//returns psStruct->pStructureType->type
	{ "stattype",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_STRUCTURE,	OBJID_STRUCTSTATTYPE,scrBaseObjGet,NULL, 0, {0}, NULL },

	//returns if this unit is currently selected by a player (usually human)
	{ "selected",	VAL_BOOL,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_SELECTED,	scrBaseObjGet,	NULL, 0, {0}, NULL },

	/* Weapon Stats */

	//weapon short range
	{ "shortRange",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_WEAPON,	WEAPID_SHORT_RANGE,	scrWeaponObjGet,	NULL, 0, {0}, NULL },

	//weapon long range
	{ "longRange",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_WEAPON,	WEAPID_LONG_RANGE,	scrWeaponObjGet,	NULL, 0, {0}, NULL },

	//weapon short hit chance
	{ "shortHit",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_WEAPON,	WEAPID_SHORT_HIT,	scrWeaponObjGet,	NULL, 0, {0}, NULL },

	//weapon long hit chance
	{ "longHit",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_WEAPON,	WEAPID_LONG_HIT,	scrWeaponObjGet,	NULL, 0, {0}, NULL },

	//weapon damage
	{ "damage",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_WEAPON,	WEAPID_DAMAGE,		scrWeaponObjGet,	NULL, 0, {0}, NULL },

	//weapon fire pause
	{ "firePause",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_WEAPON,	WEAPID_FIRE_PAUSE,	scrWeaponObjGet,	NULL, 0, {0}, NULL },

	//weapon reload time
	{ "reloadTime",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_WEAPON,	WEAPID_RELOAD_TIME,	scrWeaponObjGet,	NULL, 0, {0}, NULL },

	//num of weapon's rounds (salvo fire)
	{ "numRounds",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_WEAPON,	WEAPID_NUM_ROUNDS,	scrWeaponObjGet,	NULL, 0, {0}, NULL },

	/* This entry marks the end of the variable list */
	{ NULL, VAL_VOID, (INTERP_TYPE)ST_OBJECT, VAL_VOID, 0, NULL, NULL, 0, {0}, NULL }
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
	//reticule button IDs	- for scrFlashOn & Off
	// original annette styley
	{ "OPTIONS",	VAL_INT,	false,	IDRET_OPTIONS,		NULL, NULL, 0.0f },
	{ "CANCEL",		VAL_INT,	false,	IDRET_CANCEL,		NULL, NULL, 0.0f },
	{ "BUILD",		VAL_INT,	false,	IDRET_BUILD,		NULL, NULL, 0.0f },
	{ "MANUFACTURE",VAL_INT,	false,	IDRET_MANUFACTURE,	NULL, NULL, 0.0f },
	{ "RESEARCH",	VAL_INT,	false,	IDRET_RESEARCH,		NULL, NULL, 0.0f },
	{ "INTELMAP",	VAL_INT,	false,	IDRET_INTEL_MAP,	NULL, NULL, 0.0f },
	{ "DESIGN",		VAL_INT,	false,	IDRET_DESIGN,		NULL, NULL, 0.0f },
	{ "COMMAND",	VAL_INT,	false,	IDRET_COMMAND,		NULL, NULL, 0.0f },

	// new styley that supports many other buttons
	{ "IDRET_OPTIONS",		VAL_INT,	false,	IDRET_OPTIONS,		NULL, NULL, 0.0f },
	{ "IDRET_CANCEL",		VAL_INT,	false,	IDRET_CANCEL,		NULL, NULL, 0.0f },
	{ "IDRET_BUILD",		VAL_INT,	false,	IDRET_BUILD,		NULL, NULL, 0.0f },
	{ "IDRET_MANUFACTURE",	VAL_INT,	false,	IDRET_MANUFACTURE,	NULL, NULL, 0.0f },
	{ "IDRET_RESEARCH",		VAL_INT,	false,	IDRET_RESEARCH,		NULL, NULL, 0.0f },
	{ "IDRET_INTELMAP",		VAL_INT,	false,	IDRET_INTEL_MAP,	NULL, NULL, 0.0f },
	{ "IDRET_DESIGN",		VAL_INT,	false,	IDRET_DESIGN,		NULL, NULL, 0.0f },
	{ "IDRET_COMMAND",		VAL_INT,	false,	IDRET_COMMAND,		NULL, NULL, 0.0f },
	{ "IDRET_ORDER",		VAL_INT,	false,	IDRET_ORDER,		NULL, NULL, 0.0f },
	{ "IDRET_TRANSPORTER",	VAL_INT,	false,	IDRET_TRANSPORTER,	NULL, NULL, 0.0f },

	    // Design screen buttons
	{ "IDDES_TEMPLSTART",	VAL_INT,	false,	IDDES_TEMPLSTART,	NULL, NULL, 0.0f },
	{ "IDDES_SYSTEMBUTTON",	VAL_INT,	false,	IDDES_SYSTEMBUTTON,	NULL, NULL, 0.0f },
	{ "IDDES_BODYBUTTON",	VAL_INT,	false,	IDDES_BODYBUTTON,	NULL, NULL, 0.0f },
	{ "IDDES_PROPBUTTON",	VAL_INT,	false,	IDDES_PROPBUTTON,	NULL, NULL, 0.0f },

	// the first (top-left)  button on the object window (right of the reticule window)
	{ "IDOBJ_STATSTART",	VAL_INT,	false,	IDOBJ_STATSTART,	NULL, NULL, 0.0f },

	// one below obj_statstart
	{ "IDOBJ_OBJSTART",		VAL_INT,	false,	IDOBJ_OBJSTART,		NULL, NULL, 0.0f },

	// the first (top-left) button on the list window (up from the reticule)
	{ "IDSTAT_START",		VAL_INT,	false,	IDSTAT_START,		NULL, NULL, 0.0f },

	//message Types
	{ "RES_MSG",	VAL_INT,	false,	MSG_RESEARCH,	NULL, NULL, 0.0f },
	{ "CAMP_MSG",	VAL_INT,	false,	MSG_CAMPAIGN,	NULL, NULL, 0.0f },
	{ "MISS_MSG",	VAL_INT,	false,	MSG_MISSION,	NULL, NULL, 0.0f },
	{ "PROX_MSG",	VAL_INT,	false,	MSG_PROXIMITY,	NULL, NULL, 0.0f },

	//used for null pointers
	{ "NULLTEMPLATE",	(INTERP_TYPE)ST_POINTER_T,	false,	0,	NULL, NULL, 0.0f },
	{ "NULLOBJECT",		(INTERP_TYPE)ST_POINTER_O,	false,	0,	NULL, NULL, 0.0f },
	{ "NULLSTAT",		(INTERP_TYPE)ST_POINTER_S,	false,	0,	NULL, NULL, 0.0f },
	{ "NULLSTRING",		(INTERP_TYPE)ST_TEXTSTRING,	false,	0,	NULL, NULL, 0.0f },
	{ "NULLSTRUCTURESTAT",(INTERP_TYPE)ST_POINTER_STRUCTSTAT,	false,	0,	NULL, NULL, 0.0f },	//for NULLSTRUCTURESTAT

    //#define used to set the reinforcement timer with
	{ "LZ_COMPROMISED_TIME",VAL_INT,	false,	SCR_LZ_COMPROMISED_TIME,	NULL, NULL, 0.0f },

    // BASEOBJ types
	{ "OBJ_DROID",		VAL_INT,	false,	OBJ_DROID,		NULL, NULL, 0.0f },
	{ "OBJ_STRUCTURE",	VAL_INT,	false,	OBJ_STRUCTURE,	NULL, NULL, 0.0f },
	{ "OBJ_FEATURE",	VAL_INT,	false,	OBJ_FEATURE,	NULL, NULL, 0.0f },
	//mission Types
	{ "CAMP_START",	VAL_INT,	false,	LDS_CAMSTART,		NULL, NULL, 0.0f },
	{ "CAMP_EXPAND",VAL_INT,	false,	LDS_EXPAND,			NULL, NULL, 0.0f },
	{ "OFF_KEEP",	VAL_INT,	false,	LDS_MKEEP,			NULL, NULL, 0.0f },
	{ "OFF_CLEAR",	VAL_INT,	false,	LDS_MCLEAR,			NULL, NULL, 0.0f },
	{ "BETWEEN",	VAL_INT,	false,	LDS_BETWEEN,		NULL, NULL, 0.0f },
	// droid types
	{ "DROID_WEAPON",			VAL_INT,	false,	DROID_WEAPON,			NULL, NULL, 0.0f },
	{ "DROID_SENSOR",			VAL_INT,	false,	DROID_SENSOR,			NULL, NULL, 0.0f },
	{ "DROID_ECM",				VAL_INT,	false,	DROID_ECM,				NULL, NULL, 0.0f },
	{ "DROID_CONSTRUCT",		VAL_INT,	false,	DROID_CONSTRUCT,		NULL, NULL, 0.0f },
	{ "DROID_PERSON",			VAL_INT,	false,	DROID_PERSON,			NULL, NULL, 0.0f },
	{ "DROID_CYBORG",			VAL_INT,	false,	DROID_CYBORG,			NULL, NULL, 0.0f },
	{ "DROID_CYBORG_SUPER",		VAL_INT,	false,	DROID_CYBORG_SUPER,		NULL, NULL, 0.0f },
	{ "DROID_SUPERTRANSPORTER",	VAL_INT,	false,	DROID_SUPERTRANSPORTER,	NULL, NULL, 0.0f },
	{ "DROID_TRANSPORTER",		VAL_INT,	false,	DROID_TRANSPORTER,		NULL, NULL, 0.0f },
	{ "DROID_COMMAND",			VAL_INT,	false,	DROID_COMMAND,			NULL, NULL, 0.0f },
	{ "DROID_REPAIR",			VAL_INT,	false,	DROID_REPAIR,			NULL, NULL, 0.0f },
	{ "DROID_CYBORG_REPAIR",	VAL_INT,	false,	DROID_CYBORG_REPAIR,	NULL, NULL, 0.0f },
	{ "DROID_CYBORG_CONSTRUCT",	VAL_INT,	false,	DROID_CYBORG_CONSTRUCT,	NULL, NULL, 0.0f },

	// structure types
	{ "REF_HQ",				VAL_INT,	false,	REF_HQ,					NULL, NULL, 0.0f },
	{ "REF_FACTORY",		VAL_INT,	false,	REF_FACTORY,			NULL, NULL, 0.0f },
	{ "REF_FACTORY_MODULE",	VAL_INT,	false,	REF_FACTORY_MODULE,		NULL, NULL, 0.0f },
	{ "REF_POWER_GEN",		VAL_INT,	false,	REF_POWER_GEN,			NULL, NULL, 0.0f },
	{ "REF_POWER_MODULE",	VAL_INT,	false,	REF_POWER_MODULE,		NULL, NULL, 0.0f },
	{ "REF_RESOURCE_EXTRACTOR",VAL_INT,	false,	REF_RESOURCE_EXTRACTOR,	NULL, NULL, 0.0f },
	{ "REF_DEFENSE",		VAL_INT,	false,	REF_DEFENSE,			NULL, NULL, 0.0f },
	{ "REF_WALL",			VAL_INT,	false,	REF_WALL,				NULL, NULL, 0.0f },
	{ "REF_WALLCORNER",		VAL_INT,	false,	REF_WALLCORNER,			NULL, NULL, 0.0f },
	{ "REF_RESEARCH",		VAL_INT,	false,	REF_RESEARCH,			NULL, NULL, 0.0f },
	{ "REF_RESEARCH_MODULE",VAL_INT,	false,	REF_RESEARCH_MODULE,	NULL, NULL, 0.0f },
	{ "REF_REPAIR_FACILITY",VAL_INT,	false,	REF_REPAIR_FACILITY,	NULL, NULL, 0.0f },
	{ "REF_COMMAND_CONTROL",VAL_INT,	false,	REF_COMMAND_CONTROL,	NULL, NULL, 0.0f },
	{ "REF_CYBORG_FACTORY",	VAL_INT,	false,	REF_CYBORG_FACTORY,		NULL, NULL, 0.0f },
	{ "REF_VTOL_FACTORY",	VAL_INT,	false,	REF_VTOL_FACTORY,		NULL, NULL, 0.0f },
	{ "REF_REARM_PAD",		VAL_INT,	false,	REF_REARM_PAD,		    NULL, NULL, 0.0f },
	{ "REF_MISSILE_SILO",	VAL_INT,	false,	REF_MISSILE_SILO,		NULL, NULL, 0.0f },


	// primary orders
	{ "DORDER_NONE",		VAL_INT,	false,	DORDER_NONE,		NULL, NULL, 0.0f },
	{ "DORDER_STOP",		VAL_INT,	false,	DORDER_STOP,		NULL, NULL, 0.0f },
	{ "DORDER_MOVE",		VAL_INT,	false,	DORDER_MOVE,		NULL, NULL, 0.0f },
	{ "DORDER_ATTACK",		VAL_INT,	false,	DORDER_ATTACK,		NULL, NULL, 0.0f },
	{ "DORDER_BUILD",		VAL_INT,	false,	DORDER_BUILD,		NULL, NULL, 0.0f },
	{ "DORDER_HELPBUILD",	VAL_INT,	false,	DORDER_HELPBUILD,	NULL, NULL, 0.0f },
	{ "DORDER_LINEBUILD",	VAL_INT,	false,	DORDER_LINEBUILD,	NULL, NULL, 0.0f },
	{ "DORDER_DEMOLISH",	VAL_INT,	false,	DORDER_DEMOLISH,	NULL, NULL, 0.0f },
	{ "DORDER_REPAIR",		VAL_INT,	false,	DORDER_REPAIR,		NULL, NULL, 0.0f },
	{ "DORDER_OBSERVE",		VAL_INT,	false,	DORDER_OBSERVE,		NULL, NULL, 0.0f },
	{ "DORDER_FIRESUPPORT",	VAL_INT,	false,	DORDER_FIRESUPPORT,	NULL, NULL, 0.0f },
	{ "DORDER_RETREAT",		VAL_INT,	false,	DORDER_RETREAT,		NULL, NULL, 0.0f },
	{ "DORDER_DESTRUCT",	VAL_INT,	false,	DORDER_DESTRUCT,	NULL, NULL, 0.0f },
	{ "DORDER_RTB",			VAL_INT,	false,	DORDER_RTB,			NULL, NULL, 0.0f },
	{ "DORDER_RTR",			VAL_INT,	false,	DORDER_RTR,			NULL, NULL, 0.0f },
	{ "DORDER_RUN",			VAL_INT,	false,	DORDER_RUN,			NULL, NULL, 0.0f },
	{ "DORDER_EMBARK",		VAL_INT,	false,	DORDER_EMBARK,		NULL, NULL, 0.0f },
	{ "DORDER_DISEMBARK",	VAL_INT,	false,	DORDER_DISEMBARK,	NULL, NULL, 0.0f },
	{ "DORDER_SCOUT",		VAL_INT,	false,	DORDER_SCOUT,		NULL, NULL, 0.0f },
	{ "DORDER_DROIDREPAIR",	VAL_INT,	false,	DORDER_DROIDREPAIR,	NULL, NULL, 0.0f },
	{ "DORDER_GUARD",		VAL_INT,	false,	DORDER_GUARD,		NULL, NULL, 0.0f },


	//new member varialbe - constants for .action
	//-----------------------------------------------------------
	{ "DACTION_NONE",				VAL_INT,	false,	DACTION_NONE,				NULL, NULL, 0.0f },
	{ "DACTION_MOVE",				VAL_INT,	false,	DACTION_MOVE,				NULL, NULL, 0.0f },
	{ "DACTION_BUILD",				VAL_INT,	false,	DACTION_BUILD,				NULL, NULL, 0.0f },
	{ "DACTION_BUILD_FOUNDATION",	VAL_INT,	false,	DACTION_BUILD_FOUNDATION,	NULL, NULL, 0.0f },
	{ "DACTION_DEMOLISH",			VAL_INT,	false,	DACTION_DEMOLISH,			NULL, NULL, 0.0f },
	{ "DACTION_REPAIR",				VAL_INT,	false,	DACTION_REPAIR,				NULL, NULL, 0.0f },
	{ "DACTION_ATTACK",				VAL_INT,	false,	DACTION_ATTACK,				NULL, NULL, 0.0f },
	{ "DACTION_OBSERVE",			VAL_INT,	false,	DACTION_OBSERVE,			NULL, NULL, 0.0f },
	{ "DACTION_FIRESUPPORT",		VAL_INT,	false,	DACTION_FIRESUPPORT,		NULL, NULL, 0.0f },
	{ "DACTION_SULK",				VAL_INT,	false,	DACTION_SULK,				NULL, NULL, 0.0f },

	{ "DACTION_DESTRUCT",			VAL_INT,	false,	DACTION_DESTRUCT,			NULL, NULL, 0.0f },
	{ "DACTION_TRANSPORTOUT",		VAL_INT,	false,	DACTION_TRANSPORTOUT,		NULL, NULL, 0.0f },
	{ "DACTION_TRANSPORTWAITTOFLYIN",VAL_INT,	false,	DACTION_TRANSPORTWAITTOFLYIN,NULL, NULL, 0.0f },
	{ "DACTION_TRANSPORTIN",		VAL_INT,	false,	DACTION_TRANSPORTIN,		NULL, NULL, 0.0f },
	{ "DACTION_DROIDREPAIR",		VAL_INT,	false,	DACTION_DROIDREPAIR,		NULL, NULL, 0.0f },
	{ "DACTION_RESTORE",			VAL_INT,	false,	DACTION_RESTORE,			NULL, NULL, 0.0f },
	{ "DACTION_MOVEFIRE",			VAL_INT,	false,	DACTION_MOVEFIRE,			NULL, NULL, 0.0f },
	{ "DACTION_MOVETOBUILD",		VAL_INT,	false,	DACTION_MOVETOBUILD,		NULL, NULL, 0.0f },
	{ "DACTION_MOVETODEMOLISH",		VAL_INT,	false,	DACTION_MOVETODEMOLISH,		NULL, NULL, 0.0f },

	{ "DACTION_MOVETOREPAIR",		VAL_INT,	false,	DACTION_MOVETOREPAIR,		NULL, NULL, 0.0f },
	{ "DACTION_BUILDWANDER",		VAL_INT,	false,	DACTION_BUILDWANDER,		NULL, NULL, 0.0f },
	{ "DACTION_FOUNDATION_WANDER",	VAL_INT,	false,	DACTION_FOUNDATION_WANDER,	NULL, NULL, 0.0f },
	{ "DACTION_MOVETOATTACK",		VAL_INT,	false,	DACTION_MOVETOATTACK,		NULL, NULL, 0.0f },
	{ "DACTION_ROTATETOATTACK",		VAL_INT,	false,	DACTION_ROTATETOATTACK,		NULL, NULL, 0.0f },
	{ "DACTION_MOVETOOBSERVE",		VAL_INT,	false,	DACTION_MOVETOOBSERVE,		NULL, NULL, 0.0f },
	{ "DACTION_WAITFORREPAIR",		VAL_INT,	false,	DACTION_WAITFORREPAIR,		NULL, NULL, 0.0f },
	{ "DACTION_MOVETOREPAIRPOINT",	VAL_INT,	false,	DACTION_MOVETOREPAIRPOINT,	NULL, NULL, 0.0f },
	{ "DACTION_WAITDURINGREPAIR",	VAL_INT,	false,	DACTION_WAITDURINGREPAIR,	NULL, NULL, 0.0f },
	{ "DACTION_MOVETODROIDREPAIR",	VAL_INT,	false,	DACTION_MOVETODROIDREPAIR,	NULL, NULL, 0.0f },

	{ "DACTION_MOVETORESTORE",		VAL_INT,	false,	DACTION_MOVETORESTORE,		NULL, NULL, 0.0f },
	{ "DACTION_MOVETOREARM",		VAL_INT,	false,	DACTION_MOVETOREARM,		NULL, NULL, 0.0f },
	{ "DACTION_WAITFORREARM",		VAL_INT,	false,	DACTION_WAITFORREARM,		NULL, NULL, 0.0f },
	{ "DACTION_MOVETOREARMPOINT",	VAL_INT,	false,	DACTION_MOVETOREARMPOINT,	NULL, NULL, 0.0f },
	{ "DACTION_WAITDURINGREARM",	VAL_INT,	false,	DACTION_WAITDURINGREARM,	NULL, NULL, 0.0f },
	{ "DACTION_VTOLATTACK",			VAL_INT,	false,	DACTION_VTOLATTACK,			NULL, NULL, 0.0f },
	{ "DACTION_CLEARREARMPAD",		VAL_INT,	false,	DACTION_CLEARREARMPAD,		NULL, NULL, 0.0f },
	{ "DACTION_RETURNTOPOS",		VAL_INT,	false,	DACTION_RETURNTOPOS,		NULL, NULL, 0.0f },
	{ "DACTION_FIRESUPPORT_RETREAT",VAL_INT,	false,	DACTION_FIRESUPPORT_RETREAT,NULL, NULL, 0.0f },

	// secondary orders
	{ "DSO_ATTACK_RANGE",	VAL_INT,	false,	DSO_ATTACK_RANGE,	NULL, NULL, 0.0f },
	{ "DSO_REPAIR_LEVEL",	VAL_INT,	false,	DSO_REPAIR_LEVEL,	NULL, NULL, 0.0f },
	{ "DSO_ATTACK_LEVEL",	VAL_INT,	false,	DSO_ATTACK_LEVEL,	NULL, NULL, 0.0f },
	{ "DSO_RECYCLE",		VAL_INT,	false,	DSO_RECYCLE,		NULL, NULL, 0.0f },
	{ "DSO_PATROL",			VAL_INT,	false,	DSO_PATROL,			NULL, NULL, 0.0f },
	{ "DSO_HALTTYPE",		VAL_INT,	false,	DSO_HALTTYPE,		NULL, NULL, 0.0f },
	{ "DSO_RETURN_TO_LOC",	VAL_INT,	false,	DSO_RETURN_TO_LOC,	NULL, NULL, 0.0f },

	// secondary order stats
	{ "DSS_ARANGE_SHORT",	VAL_INT,	false,	DSS_ARANGE_SHORT,	NULL, NULL, 0.0f },
	{ "DSS_ARANGE_LONG",	VAL_INT,	false,	DSS_ARANGE_LONG,	NULL, NULL, 0.0f },
	{ "DSS_ARANGE_DEFAULT",	VAL_INT,	false,	DSS_ARANGE_DEFAULT,	NULL, NULL, 0.0f },
	{ "DSS_REPLEV_LOW",		VAL_INT,	false,	DSS_REPLEV_LOW,		NULL, NULL, 0.0f },
	{ "DSS_REPLEV_HIGH",	VAL_INT,	false,	DSS_REPLEV_HIGH,	NULL, NULL, 0.0f },
	{ "DSS_REPLEV_NEVER",	VAL_INT,	false,	DSS_REPLEV_NEVER,	NULL, NULL, 0.0f },
	{ "DSS_ALEV_ALWAYS",	VAL_INT,	false,	DSS_ALEV_ALWAYS,	NULL, NULL, 0.0f },
	{ "DSS_ALEV_ATTACKED",	VAL_INT,	false,	DSS_ALEV_ATTACKED,	NULL, NULL, 0.0f },
	{ "DSS_ALEV_NEVER",		VAL_INT,	false,	DSS_ALEV_NEVER,		NULL, NULL, 0.0f },
	{ "DSS_HALT_HOLD",		VAL_INT,	false,	DSS_HALT_HOLD,		NULL, NULL, 0.0f },
	{ "DSS_HALT_GUARD",		VAL_INT,	false,	DSS_HALT_GUARD,		NULL, NULL, 0.0f },
	{ "DSS_HALT_PERSUE",	VAL_INT,	false,	DSS_HALT_PURSUE,	NULL, NULL, 0.0f },
	{ "DSS_RECYCLE_SET",	VAL_INT,	false,	DSS_RECYCLE_SET,	NULL, NULL, 0.0f },
	{ "DSS_ASSPROD_START",	VAL_INT,	false,	DSS_ASSPROD_START,	NULL, NULL, 0.0f },
	{ "DSS_ASSPROD_END ",	VAL_INT,	false,	DSS_ASSPROD_END ,	NULL, NULL, 0.0f },
	{ "DSS_RTL_REPAIR",		VAL_INT,	false,	DSS_RTL_REPAIR,		NULL, NULL, 0.0f },
	{ "DSS_RTL_BASE",		VAL_INT,	false,	DSS_RTL_BASE,		NULL, NULL, 0.0f },
	{ "DSS_RTL_TRANSPORT",	VAL_INT,	false,	DSS_RTL_TRANSPORT,	NULL, NULL, 0.0f },
	{ "DSS_PATROL_SET",		VAL_INT,	false,	DSS_PATROL_SET,		NULL, NULL, 0.0f },

	// button id's
	{ "IDRET_OPTIONS",		VAL_INT,	false,	IDRET_OPTIONS,		NULL, NULL, 0.0f },
	{ "IDRET_BUILD",		VAL_INT,	false,	IDRET_BUILD,		NULL, NULL, 0.0f },
	{ "IDRET_MANUFACTURE",	VAL_INT,	false,	IDRET_MANUFACTURE,	NULL, NULL, 0.0f },
	{ "IDRET_RESEARCH",		VAL_INT,	false,	IDRET_RESEARCH,		NULL, NULL, 0.0f },
	{ "IDRET_INTELMAP",		VAL_INT,	false,	IDRET_INTEL_MAP,	NULL, NULL, 0.0f },
	{ "IDRET_DESIGN",		VAL_INT,	false,	IDRET_DESIGN,		NULL, NULL, 0.0f },
	{ "IDRET_CANCEL",		VAL_INT,	false,	IDRET_CANCEL,		NULL, NULL, 0.0f },
	{ "IDRET_COMMAND",		VAL_INT,	false,	IDRET_COMMAND,		NULL, NULL, 0.0f },

	// Cursor types
	{ "IMAGE_CURSOR_SELECT", VAL_INT,	false,	IMAGE_CURSOR_SELECT,	NULL, NULL, 0.0f },
	{ "IMAGE_CURSOR_ATTACK", VAL_INT,	false,	IMAGE_CURSOR_ATTACK,	NULL, NULL, 0.0f },
	{ "IMAGE_CURSOR_MOVE",	 VAL_INT,	false,	IMAGE_CURSOR_MOVE,		NULL, NULL, 0.0f },

	{ "IMAGE_CURSOR_ECM",	 VAL_INT,	false,	IMAGE_CURSOR_ECM,		NULL, NULL, 0.0f },
	{ "IMAGE_CURSOR_REPAIR", VAL_INT,	false,	IMAGE_CURSOR_REPAIR,	NULL, NULL, 0.0f },

	{ "IMAGE_CURSOR_PICKUP",  VAL_INT,	false,	IMAGE_CURSOR_PICKUP,	NULL, NULL, 0.0f },
	{ "IMAGE_CURSOR_DEFAULT", VAL_INT,	false,	IMAGE_CURSOR_DEFAULT,	NULL, NULL, 0.0f },

	{ "IMAGE_CURSOR_BUILD",	  VAL_INT,	false,	IMAGE_CURSOR_BUILD,		NULL, NULL, 0.0f },
	{ "IMAGE_CURSOR_GUARD",	  VAL_INT,	false,	IMAGE_CURSOR_GUARD,		NULL, NULL, 0.0f },
	{ "IMAGE_CURSOR_SCOUT",   VAL_INT,	false,	IMAGE_CURSOR_SCOUT,		NULL, NULL, 0.0f },
	{ "IMAGE_CURSOR_DISEMBARK",VAL_INT,	false,	IMAGE_CURSOR_DISEMBARK,	NULL, NULL, 0.0f },
	{ "IMAGE_CURSOR_ATTACH",  VAL_INT,	false,	IMAGE_CURSOR_ATTACH,	NULL, NULL, 0.0f },
	{ "IMAGE_CURSOR_LOCKON",  VAL_INT,	false,	IMAGE_CURSOR_LOCKON,	NULL, NULL, 0.0f },
	{ "IMAGE_CURSOR_FIX",	  VAL_INT,	false,	IMAGE_CURSOR_FIX,	    NULL, NULL, 0.0f },
	{ "IMAGE_CURSOR_EMBARK",  VAL_INT,	false,	IMAGE_CURSOR_EMBARK, 	NULL, NULL, 0.0f },

	{ "IMAGE_CURSOR_BRIDGE",  VAL_INT,	false,	IMAGE_CURSOR_BRIDGE,	NULL, NULL, 0.0f },
	{ "IMAGE_CURSOR_BOMB",    VAL_INT,	false,	IMAGE_CURSOR_BOMB,		NULL, NULL, 0.0f },

	// game mode types  (possible values for intMode)
	{ "INT_NORMAL",			VAL_INT,	false,	INT_NORMAL,		NULL, NULL, 0.0f }, // Standard mode (just the reticule)

	{ "INT_OPTION",			VAL_INT,	false,	INT_OPTION,		NULL, NULL, 0.0f }, // Option screen

	{ "INT_EDITSTAT",		VAL_INT,	false,	INT_EDITSTAT,	NULL, NULL, 0.0f }, // Stat screen up for placing objects

	{ "INT_EDIT",			VAL_INT,	false,	INT_EDIT,		NULL, NULL, 0.0f }, // Edit mode

	{ "INT_OBJECT",			VAL_INT,	false,	INT_OBJECT,		NULL, NULL, 0.0f }, // Object screen
	{ "INT_STAT",			VAL_INT,	false,	INT_STAT,		NULL, NULL, 0.0f }, // Object screen with stat screen
	{ "INT_CMDORDER",		VAL_INT,	false,	INT_CMDORDER,	NULL, NULL, 0.0f }, // Object screen with command droids and orders screen
	{ "INT_DESIGN",			VAL_INT,	false,	INT_DESIGN,		NULL, NULL, 0.0f }, // Design screen
	{ "INT_INTELMAP",		VAL_INT,	false,	INT_INTELMAP,	NULL, NULL, 0.0f }, // Intelligence Map
	{ "INT_ORDER",			VAL_INT,	false,	INT_ORDER,		NULL, NULL, 0.0f },
	{ "INT_INGAMEOP",		VAL_INT,	false,	INT_INGAMEOP,	NULL, NULL, 0.0f }, // in game options.
	{ "INT_TRANSPORTER",	VAL_INT,	false,	INT_TRANSPORTER,NULL, NULL, 0.0f }, //Loading/unloading a Transporter
	{ "INT_MISSIONRES",		VAL_INT,	false,	INT_MISSIONRES,	NULL, NULL, 0.0f }, // Results of a mission display.
	{ "INT_MULTIMENU",		VAL_INT,	false,	INT_MULTIMENU,	NULL, NULL, 0.0f }, // multiplayer only, player stats etc...

	// parameters for getGameStatus
	{ "STATUS_ReticuleIsOpen",		VAL_INT,	false, STATUS_ReticuleIsOpen,		NULL, NULL, 0.0f },		// is the reticule current being displayed (true=yes)
	{ "STATUS_BattleMapViewEnabled",VAL_INT,	false, STATUS_BattleMapViewEnabled,	NULL, NULL, 0.0f },	// Are we currently in the battlemap view (tactical display) true=yes
	{ "STATUS_DeliveryReposInProgress",VAL_INT,false, STATUS_DeliveryReposInProgress,	NULL, NULL, 0.0f },	// Are we currently in the delivery repos mode

	//possible values for externed 	targetedObjectType
	{ "MT_TERRAIN",		VAL_INT,	false,	MT_TERRAIN,			NULL, NULL, 0.0f },
	{ "MT_RESOURCE",	VAL_INT,	false,	MT_RESOURCE,		NULL, NULL, 0.0f },
	{ "MT_BLOCKING",	VAL_INT,	false,	MT_BLOCKING,		NULL, NULL, 0.0f },
	{ "MT_RIVER",		VAL_INT,	false,	MT_RIVER,			NULL, NULL, 0.0f },
	{ "MT_TRENCH",		VAL_INT,	false,	MT_TRENCH,			NULL, NULL, 0.0f },
	{ "MT_OWNSTRDAM",	VAL_INT,	false,	MT_OWNSTRDAM,		NULL, NULL, 0.0f },
	{ "MT_OWNSTROK",	VAL_INT,	false,	MT_OWNSTROK,		NULL, NULL, 0.0f },
	{ "MT_OWNSTRINCOMP",VAL_INT,	false,	MT_OWNSTRINCOMP,	NULL, NULL, 0.0f },
	{ "MT_REPAIR",		VAL_INT,	false,	MT_REPAIR,			NULL, NULL, 0.0f },
	{ "MT_REPAIRDAM",	VAL_INT,	false,	MT_REPAIRDAM,		NULL, NULL, 0.0f },
	{ "MT_ENEMYSTR",	VAL_INT,	false,	MT_ENEMYSTR,		NULL, NULL, 0.0f },
	{ "MT_TRANDROID",	VAL_INT,	false,	MT_TRANDROID,		NULL, NULL, 0.0f },
	{ "MT_OWNDROID",	VAL_INT,	false,	MT_OWNDROID,		NULL, NULL, 0.0f },
	{ "MT_OWNDROIDDAM",	VAL_INT,	false,	MT_OWNDROIDDAM,		NULL, NULL, 0.0f },
	{ "MT_ENEMYDROID",	VAL_INT,	false,	MT_ENEMYDROID,		NULL, NULL, 0.0f },
	{ "MT_COMMAND",		VAL_INT,	false,	MT_COMMAND,			NULL, NULL, 0.0f },
	{ "MT_ARTIFACT",	VAL_INT,	false,	MT_ARTIFACT,		NULL, NULL, 0.0f },
	{ "MT_DAMFEATURE",	VAL_INT,	false,	MT_DAMFEATURE,		NULL, NULL, 0.0f },
	{ "MT_SENSOR",		VAL_INT,	false,	MT_SENSOR,			NULL, NULL, 0.0f },

	// structure target types
	{ "ST_HQ",					VAL_INT,	false,	SCR_ST_HQ,					NULL, NULL, 0.0f },
	{ "ST_FACTORY",				VAL_INT,	false,	SCR_ST_FACTORY,				NULL, NULL, 0.0f },
	{ "ST_POWER_GEN",			VAL_INT,	false,	SCR_ST_POWER_GEN,			NULL, NULL, 0.0f },
	{ "ST_RESOURCE_EXTRACTOR",	VAL_INT,	false,	SCR_ST_RESOURCE_EXTRACTOR,	NULL, NULL, 0.0f },
	{ "ST_WALL",				VAL_INT,	false,	SCR_ST_WALL,				NULL, NULL, 0.0f },
	{ "ST_RESEARCH",			VAL_INT,	false,	SCR_ST_RESEARCH,			NULL, NULL, 0.0f },
	{ "ST_REPAIR_FACILITY",		VAL_INT,	false,	SCR_ST_REPAIR_FACILITY,		NULL, NULL, 0.0f },
	{ "ST_COMMAND_CONTROL",		VAL_INT,	false,	SCR_ST_COMMAND_CONTROL,		NULL, NULL, 0.0f },
	{ "ST_CYBORG_FACTORY",		VAL_INT,	false,	SCR_ST_CYBORG_FACTORY,		NULL, NULL, 0.0f },
	{ "ST_VTOL_FACTORY",		VAL_INT,	false,	SCR_ST_VTOL_FACTORY,		NULL, NULL, 0.0f },
	{ "ST_REARM_PAD",			VAL_INT,	false,	SCR_ST_REARM_PAD,			NULL, NULL, 0.0f },
	{ "ST_SENSOR",				VAL_INT,	false,	SCR_ST_SENSOR,				NULL, NULL, 0.0f },
	{ "ST_DEF_GROUND",			VAL_INT,	false,	SCR_ST_DEF_GROUND,			NULL, NULL, 0.0f },
	{ "ST_DEF_AIR",				VAL_INT,	false,	SCR_ST_DEF_AIR,				NULL, NULL, 0.0f },
	{ "ST_DEF_IDF",				VAL_INT,	false,	SCR_ST_DEF_IDF,				NULL, NULL, 0.0f },
	{ "ST_DEF_ALL",				VAL_INT,	false,	SCR_ST_DEF_ALL,				NULL, NULL, 0.0f },

	// droid target types
	{ "DT_COMMAND",		VAL_INT,	false,	SCR_DT_COMMAND,		NULL, NULL, 0.0f },
	{ "DT_SENSOR",		VAL_INT,	false,	SCR_DT_SENSOR,		NULL, NULL, 0.0f },
	{ "DT_CONSTRUCT",	VAL_INT,	false,	SCR_DT_CONSTRUCT,	NULL, NULL, 0.0f },
	{ "DT_REPAIR",		VAL_INT,	false,	SCR_DT_REPAIR,		NULL, NULL, 0.0f },
	{ "DT_WEAP_GROUND",	VAL_INT,	false,	SCR_DT_WEAP_GROUND,	NULL, NULL, 0.0f },
	{ "DT_WEAP_AIR",	VAL_INT,	false,	SCR_DT_WEAP_AIR,	NULL, NULL, 0.0f },
	{ "DT_WEAP_IDF",	VAL_INT,	false,	SCR_DT_WEAP_IDF,	NULL, NULL, 0.0f },
	{ "DT_WEAP_ALL",	VAL_INT,	false,	SCR_DT_WEAP_ALL,	NULL, NULL, 0.0f },
	{ "DT_LIGHT",		VAL_INT,	false,	SCR_DT_LIGHT,		NULL, NULL, 0.0f },
	{ "DT_MEDIUM",		VAL_INT,	false,	SCR_DT_MEDIUM,		NULL, NULL, 0.0f },
	{ "DT_HEAVY",		VAL_INT,	false,	SCR_DT_HEAVY,		NULL, NULL, 0.0f },
	{ "DT_SUPER_HEAVY",	VAL_INT,	false,	SCR_DT_SUPER_HEAVY,	NULL, NULL, 0.0f },
	{ "DT_TRACK",		VAL_INT,	false,	SCR_DT_TRACK,		NULL, NULL, 0.0f },
	{ "DT_HTRACK",		VAL_INT,	false,	SCR_DT_HTRACK,		NULL, NULL, 0.0f },
	{ "DT_WHEEL",		VAL_INT,	false,	SCR_DT_WHEEL,		NULL, NULL, 0.0f },
	{ "DT_LEGS",		VAL_INT,	false,	SCR_DT_LEGS,		NULL, NULL, 0.0f },
	{ "DT_GROUND",		VAL_INT,	false,	SCR_DT_GROUND,		NULL, NULL, 0.0f },
	{ "DT_VTOL",		VAL_INT,	false,	SCR_DT_VTOL,		NULL, NULL, 0.0f },
	{ "DT_HOVER",		VAL_INT,	false,	SCR_DT_HOVER,		NULL, NULL, 0.0f },

	// multiplayer

	{ "MAX_PLAYERS",        VAL_INT,        false,  MAX_PLAYERS,                    NULL, NULL, 0.0f },

	{ "CAMPAIGN",		VAL_INT,	false,	CAMPAIGN,			NULL, NULL, 0.0f },
	{ "SKIRMISH",		VAL_INT,	false,	SKIRMISH,			NULL, NULL, 0.0f },

	{ "CAMP_CLEAN",		VAL_INT,	false,	CAMP_CLEAN,			NULL, NULL, 0.0f },
	{ "CAMP_BASE",		VAL_INT,	false,	CAMP_BASE,			NULL, NULL, 0.0f },
	{ "CAMP_WALLS",		VAL_INT,	false,	CAMP_WALLS,			NULL, NULL, 0.0f },

	{ "NO_ALLIANCES",	VAL_INT,	false,	NO_ALLIANCES,		NULL, NULL, 0.0f },
	{ "ALLIANCES",		VAL_INT,	false,	ALLIANCES,			NULL, NULL, 0.0f },
	{ "ALLIANCES_TEAMS",VAL_INT,	false,	ALLIANCES_TEAMS,	NULL, NULL, 0.0f },

	//Group types
	{ "GT_NORMAL",		VAL_INT,	false,	GT_NORMAL,			NULL, NULL, 0.0f },
	{ "GT_COMMAND",		VAL_INT,	false,	GT_COMMAND,			NULL, NULL, 0.0f },
	{ "GT_TRANSPORTER",	VAL_INT,	false,	GT_TRANSPORTER,		NULL, NULL, 0.0f },

	/* some key constants */
	{ "KEY_LCTRL",		VAL_INT,	false,		KEY_LCTRL,		NULL, NULL, 0.0f },
	{ "KEY_RCTRL",		VAL_INT,	false,		KEY_RCTRL,		NULL, NULL, 0.0f },
	{ "KEY_LSHIFT",		VAL_INT,	false,		KEY_LSHIFT,		NULL, NULL, 0.0f },
	{ "KEY_RSHIFT",		VAL_INT,	false,		KEY_RSHIFT,		NULL, NULL, 0.0f },
	{ "KEY_LALT",		VAL_INT,	false,		KEY_LALT,		NULL, NULL, 0.0f },
	{ "KEY_RALT",		VAL_INT,	false,		KEY_RALT,		NULL, NULL, 0.0f },
	{ "KEY_SPACE",		VAL_INT,	false,		KEY_SPACE,		NULL, NULL, 0.0f },
	{ "KEY_RETURN",		VAL_INT,	false,		KEY_RETURN,		NULL, NULL, 0.0f },
	{ "KEY_ESC",		VAL_INT,	false,		KEY_ESC,		NULL, NULL, 0.0f },
	{ "KEY_A",			VAL_INT,	false,		KEY_A,			NULL, NULL, 0.0f },
	{ "KEY_B",			VAL_INT,	false,		KEY_B,			NULL, NULL, 0.0f },
	{ "KEY_C",			VAL_INT,	false,		KEY_C,			NULL, NULL, 0.0f },
	{ "KEY_D",			VAL_INT,	false,		KEY_D,			NULL, NULL, 0.0f },
	{ "KEY_E",			VAL_INT,	false,		KEY_E,			NULL, NULL, 0.0f },
	{ "KEY_F",			VAL_INT,	false,		KEY_F,			NULL, NULL, 0.0f },
	{ "KEY_G",			VAL_INT,	false,		KEY_G,			NULL, NULL, 0.0f },
	{ "KEY_H",			VAL_INT,	false,		KEY_H,			NULL, NULL, 0.0f },
	{ "KEY_I",			VAL_INT,	false,		KEY_I,			NULL, NULL, 0.0f },
	{ "KEY_J",			VAL_INT,	false,		KEY_J,			NULL, NULL, 0.0f },
	{ "KEY_K",			VAL_INT,	false,		KEY_K,			NULL, NULL, 0.0f },
	{ "KEY_L",			VAL_INT,	false,		KEY_L,			NULL, NULL, 0.0f },
	{ "KEY_M",			VAL_INT,	false,		KEY_M,			NULL, NULL, 0.0f },
	{ "KEY_N",			VAL_INT,	false,		KEY_N,			NULL, NULL, 0.0f },
	{ "KEY_O",			VAL_INT,	false,		KEY_O,			NULL, NULL, 0.0f },
	{ "KEY_P",			VAL_INT,	false,		KEY_P,			NULL, NULL, 0.0f },
	{ "KEY_Q",			VAL_INT,	false,		KEY_Q,			NULL, NULL, 0.0f },
	{ "KEY_R",			VAL_INT,	false,		KEY_R,			NULL, NULL, 0.0f },
	{ "KEY_S",			VAL_INT,	false,		KEY_S,			NULL, NULL, 0.0f },
	{ "KEY_T",			VAL_INT,	false,		KEY_T,			NULL, NULL, 0.0f },
	{ "KEY_U",			VAL_INT,	false,		KEY_U,			NULL, NULL, 0.0f },
	{ "KEY_V",			VAL_INT,	false,		KEY_V,			NULL, NULL, 0.0f },
	{ "KEY_W",			VAL_INT,	false,		KEY_W,			NULL, NULL, 0.0f },
	{ "KEY_X",			VAL_INT,	false,		KEY_X,			NULL, NULL, 0.0f },
	{ "KEY_Y",			VAL_INT,	false,		KEY_Y,			NULL, NULL, 0.0f },
	{ "KEY_Z",			VAL_INT,	false,		KEY_Z,			NULL, NULL, 0.0f },

	{ "KEY_KP_0",		VAL_INT,	false,		KEY_KP_0,		NULL, NULL, 0.0f },
	{ "KEY_KP_1",		VAL_INT,	false,		KEY_KP_1,		NULL, NULL, 0.0f },
	{ "KEY_KP_2",		VAL_INT,	false,		KEY_KP_2,		NULL, NULL, 0.0f },
	{ "KEY_KP_3",		VAL_INT,	false,		KEY_KP_3,		NULL, NULL, 0.0f },
	{ "KEY_KP_4",		VAL_INT,	false,		KEY_KP_4,		NULL, NULL, 0.0f },
	{ "KEY_KP_5",		VAL_INT,	false,		KEY_KP_5,		NULL, NULL, 0.0f },
	{ "KEY_KP_6",		VAL_INT,	false,		KEY_KP_6,		NULL, NULL, 0.0f },
	{ "KEY_KP_7",		VAL_INT,	false,		KEY_KP_7,		NULL, NULL, 0.0f },
	{ "KEY_KP_8",		VAL_INT,	false,		KEY_KP_8,		NULL, NULL, 0.0f },
	{ "KEY_KP_9",		VAL_INT,	false,		KEY_KP_9,		NULL, NULL, 0.0f },

	{ "EASY",               VAL_INT,        false,  DIFFICULTY_EASY,                NULL, NULL, 0.0f },
	{ "MEDIUM",             VAL_INT,        false,  DIFFICULTY_MEDIUM,              NULL, NULL, 0.0f },
	{ "HARD",               VAL_INT,        false,  DIFFICULTY_HARD,                NULL, NULL, 0.0f },
	{ "INSANE",             VAL_INT,        false,  DIFFICULTY_INSANE,              NULL, NULL, 0.0f },

	/* This entry marks the end of the constant list */
	{ "CONSTANT LIST END",VAL_VOID,	false,		0,	NULL, NULL, 0.0f }
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
	{ "CALL_GAMEINIT",			(TRIGGER_TYPE)CALL_GAMEINIT,			NULL, 0, {VAL_VOID} },
	{ "CALL_DELIVPOINTMOVED",	(TRIGGER_TYPE)CALL_DELIVPOINTMOVED,		NULL, 0, {VAL_VOID} },
	{ "CALL_DROIDDESIGNED",		(TRIGGER_TYPE)CALL_DROIDDESIGNED,		NULL, 0, {VAL_VOID} },
	{ "CALL_DROIDBUILT",		(TRIGGER_TYPE)CALL_DROIDBUILT,			NULL, 0, {VAL_VOID} },
	{ "CALL_POWERGEN_BUILT",	(TRIGGER_TYPE)CALL_POWERGEN_BUILT,		NULL, 0, {VAL_VOID} },
	{ "CALL_RESEX_BUILT",		(TRIGGER_TYPE)CALL_RESEX_BUILT,			NULL, 0, {VAL_VOID} },
	{ "CALL_RESEARCH_BUILT",	(TRIGGER_TYPE)CALL_RESEARCH_BUILT,		NULL, 0, {VAL_VOID} },
	{ "CALL_FACTORY_BUILT",		(TRIGGER_TYPE)CALL_FACTORY_BUILT,		NULL, 0, {VAL_VOID} },

	{ "CALL_MISSION_START",		(TRIGGER_TYPE)CALL_MISSION_START,		NULL, 0, {VAL_VOID} },
	{ "CALL_MISSION_END",		(TRIGGER_TYPE)CALL_MISSION_END,			NULL, 0, {VAL_VOID} },
	{ "CALL_VIDEO_QUIT",		(TRIGGER_TYPE)CALL_VIDEO_QUIT,			NULL, 0, {VAL_VOID} },
	{ "CALL_LAUNCH_TRANSPORTER",(TRIGGER_TYPE)CALL_LAUNCH_TRANSPORTER,	NULL, 0, {VAL_VOID} },
	{ "CALL_START_NEXT_LEVEL",	(TRIGGER_TYPE)CALL_START_NEXT_LEVEL,	NULL, 0, {VAL_VOID} },
	{ "CALL_TRANSPORTER_REINFORCE",(TRIGGER_TYPE)CALL_TRANSPORTER_REINFORCE,NULL, 0, {VAL_VOID} },
	{ "CALL_MISSION_TIME",		(TRIGGER_TYPE)CALL_MISSION_TIME,		NULL, 0, {VAL_VOID} },
	{ "CALL_ELECTRONIC_TAKEOVER",(TRIGGER_TYPE)CALL_ELECTRONIC_TAKEOVER,NULL, 0, {VAL_VOID} },

	// tutorial only callbacks
	{ "CALL_BUILDLIST",			(TRIGGER_TYPE)CALL_BUILDLIST,			NULL, 0, {VAL_VOID} },
	{ "CALL_BUILDGRID",			(TRIGGER_TYPE)CALL_BUILDGRID,			NULL, 0, {VAL_VOID} },
	{ "CALL_RESEARCHLIST",		(TRIGGER_TYPE)CALL_RESEARCHLIST,		NULL, 0, {VAL_VOID} },
	{ "CALL_MANURUN",			(TRIGGER_TYPE)CALL_MANURUN,				NULL, 0, {VAL_VOID} },
	{ "CALL_MANULIST",			(TRIGGER_TYPE)CALL_MANULIST,			NULL, 0, {VAL_VOID} },
	{ "CALL_BUTTON_PRESSED",	(TRIGGER_TYPE)CALL_BUTTON_PRESSED,		scrCBButtonPressed,	1,	{ VAL_INT } },
	{ "CALL_DROID_SELECTED",	(TRIGGER_TYPE)CALL_DROID_SELECTED,		scrCBDroidSelected,	1,	{ VAL_REF|(INTERP_TYPE)ST_DROID } },
	{ "CALL_DESIGN_QUIT",		(TRIGGER_TYPE)CALL_DESIGN_QUIT,			NULL, 0, {VAL_VOID} },
	{ "CALL_DESIGN_WEAPON",		(TRIGGER_TYPE)CALL_DESIGN_WEAPON,		NULL, 0, {VAL_VOID} },
	{ "CALL_DESIGN_SYSTEM",		(TRIGGER_TYPE)CALL_DESIGN_SYSTEM,		NULL, 0, {VAL_VOID} },
	{ "CALL_DESIGN_COMMAND",	(TRIGGER_TYPE)CALL_DESIGN_COMMAND,		NULL, 0, {VAL_VOID} },
	{ "CALL_DESIGN_BODY",		(TRIGGER_TYPE)CALL_DESIGN_BODY,			NULL, 0, {VAL_VOID} },
	{ "CALL_DESIGN_PROPULSION",	(TRIGGER_TYPE)CALL_DESIGN_PROPULSION,	NULL, 0, {VAL_VOID} },

	// callback triggers with parameters
	{ "CALL_RESEARCHCOMPLETED",	(TRIGGER_TYPE)CALL_RESEARCHCOMPLETED,
		scrCBResCompleted,	3,	{ VAL_REF|(INTERP_TYPE)ST_RESEARCH, VAL_REF|(INTERP_TYPE)ST_STRUCTURE, VAL_INT } },

	{ "CALL_NEWDROID",			(TRIGGER_TYPE)CALL_NEWDROID,
		scrCBNewDroid,		3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_DROID, VAL_REF|(INTERP_TYPE)ST_STRUCTURE } },

	{ "CALL_STRUCT_ATTACKED",	(TRIGGER_TYPE)CALL_STRUCT_ATTACKED,
		scrCBStructAttacked,3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_STRUCTURE, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT } },

	{ "CALL_DROID_ATTACKED",	(TRIGGER_TYPE)CALL_DROID_ATTACKED,
		scrCBDroidAttacked,	3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_DROID, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT } },

	{ "CALL_ATTACKED",			(TRIGGER_TYPE)CALL_ATTACKED,
		scrCBAttacked,		3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT } },

	{ "CALL_STRUCT_SEEN",		(TRIGGER_TYPE)CALL_STRUCT_SEEN,
		scrCBStructSeen,	3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_STRUCTURE, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT } },

	{ "CALL_DROID_SEEN",		(TRIGGER_TYPE)CALL_DROID_SEEN,
		scrCBDroidSeen,		3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_DROID, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT } },

	{ "CALL_FEATURE_SEEN",		(TRIGGER_TYPE)CALL_FEATURE_SEEN,
		scrCBFeatureSeen,	3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_FEATURE, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT } },

	{ "CALL_OBJ_SEEN",			(TRIGGER_TYPE)CALL_OBJ_SEEN,
		scrCBObjSeen,		3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT } },

	{ "CALL_OBJ_DESTROYED",		(TRIGGER_TYPE)CALL_OBJ_DESTROYED,
		scrCBObjDestroyed,	2,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_BASEOBJECT } },

	{ "CALL_STRUCT_DESTROYED",	(TRIGGER_TYPE)CALL_STRUCT_DESTROYED,
		scrCBStructDestroyed,2,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_STRUCTURE } },

	{ "CALL_DROID_DESTROYED",	(TRIGGER_TYPE)CALL_DROID_DESTROYED,
		scrCBDroidDestroyed,2,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_DROID } },

	{ "CALL_FEATURE_DESTROYED",	(TRIGGER_TYPE)CALL_FEATURE_DESTROYED,
		scrCBFeatureDestroyed,1,	{ VAL_REF|(INTERP_TYPE)ST_FEATURE } },

	{ "CALL_OBJECTOPEN",		(TRIGGER_TYPE)CALL_OBJECTOPEN, 		NULL, 0, {VAL_VOID} },
	{ "CALL_OBJECTCLOSE",		(TRIGGER_TYPE)CALL_OBJECTCLOSE, 	NULL, 0, {VAL_VOID} },

	{ "CALL_TRANSPORTER_OFFMAP",(TRIGGER_TYPE)CALL_TRANSPORTER_OFFMAP,
		scrCBTransporterOffMap,1,	{ VAL_INT } },
	{ "CALL_TRANSPORTER_LANDED",(TRIGGER_TYPE)CALL_TRANSPORTER_LANDED,
		scrCBTransporterLanded,2,	{ (INTERP_TYPE)ST_GROUP, VAL_INT } },

	{ "CALL_ALL_ONSCREEN_DROIDS_SELECTED",(TRIGGER_TYPE)CALL_ALL_ONSCREEN_DROIDS_SELECTED,	NULL, 0, {VAL_VOID} },

    { "CALL_NO_REINFORCEMENTS_LEFT",(TRIGGER_TYPE)CALL_NO_REINFORCEMENTS_LEFT,	NULL, 0, {VAL_VOID} },

	{ "CALL_CLUSTER_EMPTY",		(TRIGGER_TYPE)CALL_CLUSTER_EMPTY,
		scrCBClusterEmpty,	1,	{ VAL_REF|VAL_INT } },

	{ "CALL_VTOL_OFF_MAP",		(TRIGGER_TYPE)CALL_VTOL_OFF_MAP,
		scrCBVtolOffMap,	2,  { VAL_INT, VAL_REF|(INTERP_TYPE)ST_DROID } },

	{ "CALL_UNITTAKEOVER",		(TRIGGER_TYPE)CALL_UNITTAKEOVER,
		scrCBDroidTaken,	1,	{VAL_REF|(INTERP_TYPE)ST_DROID} },

	{ "CALL_PLAYERLEFT",		(TRIGGER_TYPE)CALL_PLAYERLEFT,
		scrCBPlayerLeft,	1,	{ VAL_REF|VAL_INT }},

	{ "CALL_ALLIANCEOFFER",		(TRIGGER_TYPE)CALL_ALLIANCEOFFER,
		scrCBAllianceOffer,	2,	{ VAL_REF|VAL_INT,VAL_REF|VAL_INT }},

	// new callbacks
	//-------------------------------------------------------------------------------

	//console callback
	{ "CALL_CONSOLE",			(TRIGGER_TYPE)CALL_CONSOLE,
		scrCallConsole,		2,	{ VAL_REF|VAL_INT,VAL_REF|VAL_STRING }},

	//59
	{ "CALL_AI_MSG",			(TRIGGER_TYPE)CALL_AI_MSG,
		scrCallMultiMsg,	3,	{ VAL_INT, VAL_REF|VAL_INT,VAL_REF|VAL_STRING }},

	//59
	{ "CALL_BEACON",			(TRIGGER_TYPE)CALL_BEACON,
		scrCallBeacon,		5,	{ VAL_INT, VAL_REF|VAL_INT, VAL_REF|VAL_INT, VAL_REF|VAL_INT, VAL_REF|VAL_STRING }},

	{ "CALL_STRUCTBUILT",		(TRIGGER_TYPE)CALL_STRUCTBUILT,
		scrCBStructBuilt,	3,	{ VAL_INT, VAL_REF|ST_DROID, VAL_REF|ST_STRUCTURE } },

	// new transporter landed callback
	{ "CALL_TRANSPORTER_LANDED_B",(TRIGGER_TYPE)CALL_TRANSPORTER_LANDED_B,
		scrCBTransporterLandedB,3,	{ (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_REF|ST_DROID } },

	// fired when droid received DORDER_STOP order
	{ "CALL_DORDER_STOP",	(TRIGGER_TYPE)CALL_DORDER_STOP,
		scrCBDorderStop,	2,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_DROID } },

	// fired when droid reached the destination and stopped on its own
	{ "CALL_DROID_REACH_LOCATION",(TRIGGER_TYPE)CALL_DROID_REACH_LOCATION,
		scrCBDorderReachedLocation,3,	{ VAL_INT, VAL_REF|(INTERP_TYPE)ST_DROID, VAL_REF | VAL_INT } },

	{ "CALL_KEY_PRESSED",			(TRIGGER_TYPE)CALL_KEY_PRESSED,
		scrCBProcessKeyPress,	2,	{ VAL_REF | VAL_INT, VAL_REF| VAL_INT} },

	{ "CALL_VTOL_RETARGET",			(TRIGGER_TYPE)CALL_VTOL_RETARGET,
		scrCBVTOLRetarget,	2, 	{ VAL_INT, VAL_REF| (INTERP_TYPE)ST_DROID } },

	/* This entry marks the end of the callback list */
	{ "CALLBACK LIST END", (TRIGGER_TYPE)0, NULL, 0, {VAL_VOID} }
};


/* The table of type equivalence (only user types allowed)
 * The format is :
 *
 *       <base type>  <num equivalents>  <eqivalent types>
 *
 */
TYPE_EQUIV asEquivTable[] =
{
	{ (INTERP_TYPE)ST_BASEOBJECT,	3,	{ (INTERP_TYPE)ST_DROID, (INTERP_TYPE)ST_STRUCTURE, (INTERP_TYPE)ST_FEATURE, } },
	{ (INTERP_TYPE)ST_COMPONENT,	8,	{ (INTERP_TYPE)ST_BODY, (INTERP_TYPE)ST_PROPULSION, (INTERP_TYPE)ST_ECM, (INTERP_TYPE)ST_SENSOR, (INTERP_TYPE)ST_CONSTRUCT, (INTERP_TYPE)ST_WEAPON, (INTERP_TYPE)ST_REPAIR, (INTERP_TYPE)ST_BRAIN } },
	{ (INTERP_TYPE)ST_BASESTATS,	2,	{ (INTERP_TYPE)ST_STRUCTURESTAT, (INTERP_TYPE)ST_FEATURESTAT } },

	{ (INTERP_TYPE)ST_DROID,		1,	{ (INTERP_TYPE)ST_POINTER_O, } },
	{ (INTERP_TYPE)ST_STRUCTURE,	1,	{ (INTERP_TYPE)ST_POINTER_O, } },
	{ (INTERP_TYPE)ST_FEATURE,		1,	{ (INTERP_TYPE)ST_POINTER_O, } },
	{ (INTERP_TYPE)ST_BASEOBJECT,	1,	{ (INTERP_TYPE)ST_POINTER_O, } },
	{ (INTERP_TYPE)ST_GROUP,		1,	{ (INTERP_TYPE)ST_POINTER_O, } },
	{ (INTERP_TYPE)ST_TEMPLATE,		1,	{ (INTERP_TYPE)ST_POINTER_T, } },
	{ (INTERP_TYPE)ST_BODY,			1,  { (INTERP_TYPE)ST_POINTER_S, } },
	{ (INTERP_TYPE)ST_PROPULSION,	1,	{ (INTERP_TYPE)ST_POINTER_S, } },
	{ (INTERP_TYPE)ST_WEAPON,		1,	{ (INTERP_TYPE)ST_POINTER_S, } },
	{ (INTERP_TYPE)ST_ECM,			1,	{ (INTERP_TYPE)ST_POINTER_S, } },
	{ (INTERP_TYPE)ST_SENSOR,		1,	{ (INTERP_TYPE)ST_POINTER_S, } },
	{ (INTERP_TYPE)ST_CONSTRUCT,	1,	{ (INTERP_TYPE)ST_POINTER_S, } },
	{ (INTERP_TYPE)ST_REPAIR,		1,	{ (INTERP_TYPE)ST_POINTER_S, } },
	{ (INTERP_TYPE)ST_BRAIN,		1,	{ (INTERP_TYPE)ST_POINTER_S, } },
	{ (INTERP_TYPE)ST_STRUCTURESTAT,1,	{ (INTERP_TYPE)ST_POINTER_STRUCTSTAT, } },

	/* This marks the end of the equivalence list */
	{ (INTERP_TYPE)0, 0, {(INTERP_TYPE)0} }
};


// Initialise the script system
bool scrTabInitialise(void)
{
	unsigned int i;

	if (!scriptInitialise())
	{
		return false;
	}

	if (!eventInitValueFuncs((INTERP_TYPE)ST_MAXTYPE))
	{
		return false;
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
		return false;
	}
	if (!eventAddValueRelease((INTERP_TYPE)ST_BASEOBJECT, scrvReleaseBasePointer))
	{
		return false;
	}

	if (!eventAddValueCreate((INTERP_TYPE)ST_DROID, scrvAddBasePointer))
	{
		return false;
	}
	if (!eventAddValueRelease((INTERP_TYPE)ST_DROID, scrvReleaseBasePointer))
	{
		return false;
	}

	if (!eventAddValueCreate((INTERP_TYPE)ST_STRUCTURE, scrvAddBasePointer))
	{
		return false;
	}
	if (!eventAddValueRelease((INTERP_TYPE)ST_STRUCTURE, scrvReleaseBasePointer))
	{
		return false;
	}

	if (!eventAddValueCreate((INTERP_TYPE)ST_FEATURE, scrvAddBasePointer))
	{
		return false;
	}
	if (!eventAddValueRelease((INTERP_TYPE)ST_FEATURE, scrvReleaseBasePointer))
	{
		return false;
	}

	if (!eventAddValueCreate((INTERP_TYPE)ST_GROUP, scrvNewGroup))
	{
		return false;
	}
	if (!eventAddValueRelease((INTERP_TYPE)ST_GROUP, scrvReleaseGroup))
	{
		return false;
	}

	// initialise various variables
	scrGameLevel = 0;
	bInTutorial = false;

	/* Initialize debug output */
	for(i=0; i<DEBUGMENU_MAX_ENTRIES; i++)
		debugMenuEntry[i][0] = '\0';

	/* Initialize chat message struct */
	chat_msg.numCommands = 0;
	strcpy(chat_msg.lastMessage, "");

	return true;
}

// Shut down the script system
void scrShutDown(void)
{
	scrvShutDown();
	scriptShutDown();
}
