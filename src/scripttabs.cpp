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
 * ScriptTabs.c
 *
 * All the tables for the script compiler
 *
 */


#include "lib/framework/frame.h"
#include "lib/script/event.h"
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
	{ "DROID",	(INTERP_TYPE)ST_DROID,		AT_OBJECT,	scrValDefSave,	scrValDefLoad },
	{ "STRUCTURE",	(INTERP_TYPE)ST_STRUCTURE,	AT_OBJECT,	scrValDefSave,	scrValDefLoad },
	{ "FEATURE",	(INTERP_TYPE)ST_FEATURE,	AT_OBJECT,	scrValDefSave,	scrValDefLoad },

	{ "BASESTATS",	(INTERP_TYPE)ST_BASESTATS,	AT_SIMPLE,	scrValDefSave,	scrValDefLoad },

	// Component types
	{ "COMPONENT",	(INTERP_TYPE)ST_COMPONENT,	AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "BODY",	(INTERP_TYPE)ST_BODY,		AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "PROPULSION",	(INTERP_TYPE)ST_PROPULSION,	AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "ECM",	(INTERP_TYPE)ST_ECM,		AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "SENSOR",	(INTERP_TYPE)ST_SENSOR,		AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "CONSTRUCT",	(INTERP_TYPE)ST_CONSTRUCT,	AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "WEAPON",	(INTERP_TYPE)ST_WEAPON,		AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "REPAIR",	(INTERP_TYPE)ST_REPAIR,		AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "BRAIN",	(INTERP_TYPE)ST_BRAIN,		AT_SIMPLE,	scrValDefSave,	scrValDefLoad },

	{ "TEMPLATE",	(INTERP_TYPE)ST_TEMPLATE,	AT_SIMPLE,	scrValDefSave,	scrValDefLoad },

	{ "STRUCTUREID", (INTERP_TYPE)ST_STRUCTUREID, AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "STRUCTURESTAT", (INTERP_TYPE)ST_STRUCTURESTAT, AT_SIMPLE, scrValDefSave,	scrValDefLoad },
	{ "FEATURESTAT", (INTERP_TYPE)ST_FEATURESTAT, AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "DROIDID",	(INTERP_TYPE)ST_DROIDID,	AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "TEXTSTRING",	(INTERP_TYPE)ST_TEXTSTRING,	AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "SOUND",	(INTERP_TYPE)ST_SOUND,		AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "LEVEL",	(INTERP_TYPE)ST_LEVEL,		AT_SIMPLE,	scrValDefSave,	scrValDefLoad },
	{ "GROUP",	(INTERP_TYPE)ST_GROUP,		AT_OBJECT,	scrValDefSave,	scrValDefLoad },
	{ "RESEARCHSTAT", (INTERP_TYPE)ST_RESEARCH,	AT_SIMPLE,	scrValDefSave,	scrValDefLoad },

	//private types for code - NOT used in the scripts - hence the ""
	{ "",	(INTERP_TYPE)ST_POINTER_O,	AT_OBJECT,	nullptr,	nullptr },
	{ "",	(INTERP_TYPE)ST_POINTER_T,	AT_SIMPLE,	nullptr,	nullptr },
	{ "",	(INTERP_TYPE)ST_POINTER_S,	AT_SIMPLE,	nullptr,	nullptr },

	{ "",	(INTERP_TYPE)ST_POINTER_STRUCTSTAT, AT_SIMPLE, nullptr, nullptr },	//for NULLSTRUCTURESTAT

	/* This final entry marks the end of the type list */
	{ "END OF TYPE LIST", (INTERP_TYPE)0,		AT_SIMPLE,	nullptr,	nullptr },
};





/* The table of script callable C functions
 * This is laid out :
 *     "ScriptName",   function_Pointer,  <function return type>
 *      <number of parameters>,  <parameter types>,  false
 */
FUNC_SYMBOL asFuncTable[] =
{
	// These functions are part of the script library
	{
		"traceOn",			interpTraceOn,			VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"traceOff",			interpTraceOff,			VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"setEventTrigger",	eventSetTrigger,		VAL_VOID,
		2, { VAL_EVENT, VAL_TRIGGER },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"eventTraceLevel",	eventSetTraceLevel,		VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	// Trigger functions
	{
		"objectInRange",		scrObjectInRange,		VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"droidInRange",		scrDroidInRange,		VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"structInRange",		scrStructInRange,		VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"playerPower",		scrPlayerPower,			VAL_INT,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"objectInArea",		scrObjectInArea,		VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"droidInArea",		scrDroidInArea,			VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"structInArea",		scrStructInArea,		VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"seenStructInArea",	scrSeenStructInArea,	VAL_BOOL,
		7, { VAL_INT, VAL_INT, VAL_BOOL, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"structButNoWallsInArea", scrStructButNoWallsInArea, VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numObjectsInArea",	scrNumObjectsInArea,	VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numDroidsInArea",	scrNumDroidsInArea,		VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numStructsInArea",	scrNumStructsInArea,	VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numStructsButNotWallsInArea", scrNumStructsButNotWallsInArea, VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numStructsByTypeInArea", scrNumStructsByTypeInArea, VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},


	{
		"droidHasSeen",		scrDroidHasSeen,		VAL_BOOL,
		2, { (INTERP_TYPE)ST_BASEOBJECT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"buildingDestroyed",	scrBuildingDestroyed,	VAL_BOOL,
		2, { (INTERP_TYPE)ST_STRUCTUREID, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"structureIdle",		scrStructureIdle,		VAL_BOOL,
		1, { (INTERP_TYPE)ST_STRUCTURE },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"initEnumStruct",		scrInitEnumStruct,		VAL_VOID,
		4, { VAL_BOOL, (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"enumStruct",			scrEnumStruct,	(INTERP_TYPE)ST_STRUCTURE,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"initEnumStructB",	scrInitEnumStructB,		VAL_VOID,
		5, { VAL_BOOL, (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"enumStructB",		scrEnumStructB,	(INTERP_TYPE)ST_STRUCTURE,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"structureBeingBuilt", scrStructureBeingBuilt,	VAL_BOOL,
		2,	{ (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"structureComplete",	scrStructureComplete,	VAL_BOOL,		// pc multiplayer only
		1,	{ (INTERP_TYPE)ST_STRUCTURE },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"structureBuilt",		scrStructureBuilt,		VAL_BOOL,
		2,	{ (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"anyDroidsLeft",		scrAnyDroidsLeft,		VAL_BOOL,
		1,	{ VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"anyStructButWallsLeft", scrAnyStructButWallsLeft, VAL_BOOL,
		1,	{ VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"anyFactoriesLeft",	scrAnyFactoriesLeft,	VAL_BOOL,
		1,	{ VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	// event functions
	{
		"enableComponent",	scrEnableComponent,		VAL_VOID,
		2, { (INTERP_TYPE)ST_COMPONENT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"makeComponentAvailable", scrMakeComponentAvailable, VAL_VOID,
		2, { (INTERP_TYPE)ST_COMPONENT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"enableStructure",	scrEnableStructure,		VAL_VOID,
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"isStructureAvailable", scrIsStructureAvailable, VAL_BOOL,	// pc multiplay only
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"addDroid",			scrAddDroid,	(INTERP_TYPE)ST_DROID,
		4, { (INTERP_TYPE)ST_TEMPLATE, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"addDroidToMissionList", scrAddDroidToMissionList, (INTERP_TYPE)ST_DROID,
		2, { (INTERP_TYPE)ST_TEMPLATE, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"buildDroid",			scrBuildDroid,			VAL_VOID,
		4, { (INTERP_TYPE)ST_TEMPLATE, (INTERP_TYPE)ST_STRUCTURE, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"addTemplate",		scrAddTemplate,			VAL_BOOL,
		2, { (INTERP_TYPE)ST_TEMPLATE, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"addReticuleButton",	scrAddReticuleButton,	VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"removeReticuleButton", scrRemoveReticuleButton, VAL_VOID,
		2, { VAL_INT, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"addMessage",			scrAddMessage,			VAL_VOID,
		4, { (INTERP_TYPE)ST_INTMESSAGE, VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"removeMessage",		scrRemoveMessage,		VAL_VOID,
		3, { (INTERP_TYPE)ST_INTMESSAGE, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"selectDroidByID",	scrSelectDroidByID,		VAL_BOOL,
		2, { (INTERP_TYPE)ST_DROIDID, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setAssemblyPoint",	scrSetAssemblyPoint,	VAL_VOID,
		3, { (INTERP_TYPE)ST_STRUCTURE, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"attackLocation",		scrAttackLocation,		VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"initGetFeature",		scrInitGetFeature,		VAL_VOID,
		3, { (INTERP_TYPE)ST_FEATURESTAT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getFeature",			scrGetFeature,	(INTERP_TYPE)ST_FEATURE,
		1, { VAL_INT},
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getFeatureB",		scrGetFeatureB,	(INTERP_TYPE)ST_FEATURE,
		1, { VAL_INT},
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"addFeature",			scrAddFeature,	(INTERP_TYPE)ST_FEATURE,
		3, { (INTERP_TYPE)ST_FEATURESTAT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"destroyFeature",		scrDestroyFeature,		VAL_VOID,
		1, { (INTERP_TYPE)ST_FEATURE },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"addStructure",		scrAddStructure,	(INTERP_TYPE)ST_STRUCTURE,
		4, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"destroyStructure",	scrDestroyStructure,	VAL_VOID,
		1, { (INTERP_TYPE)ST_STRUCTURE },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"centreView",			scrCentreView,			VAL_VOID,
		1, { (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"centreViewPos",		scrCentreViewPos,		VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getStructure",		scrGetStructure,	(INTERP_TYPE)ST_STRUCTURE,
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getStructureVis",		scrGetStructureVis,	(INTERP_TYPE)ST_STRUCTURE,
		3, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getTemplate",		scrGetTemplate,	(INTERP_TYPE)ST_TEMPLATE,
		2, { (INTERP_TYPE)ST_COMPONENT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getDroid",			scrGetDroid,	(INTERP_TYPE)ST_DROID,
		2, { (INTERP_TYPE)ST_COMPONENT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setScrollParams",	scrSetScrollParams,		VAL_VOID,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setScrollMinX",		scrSetScrollMinX,		VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setScrollMinY",		scrSetScrollMinY,		VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setScrollMaxX",		scrSetScrollMaxX,		VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setScrollMaxY",		scrSetScrollMaxY,		VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setDefaultSensor",	scrSetDefaultSensor,	VAL_VOID,
		2, { (INTERP_TYPE)ST_SENSOR, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setDefaultECM",		scrSetDefaultECM,		VAL_VOID,
		2, { (INTERP_TYPE)ST_ECM, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setDefaultRepair",	scrSetDefaultRepair,	VAL_VOID,
		2, { (INTERP_TYPE)ST_REPAIR, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setStructureLimits",	scrSetStructureLimits,	VAL_VOID,
		3, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setAllStructureLimits", scrSetAllStructureLimits, VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"applyLimitSet",		scrApplyLimitSet,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"playSound",			scrPlaySound,			VAL_VOID,
		2, { (INTERP_TYPE)ST_SOUND, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"playSoundPos",		scrPlaySoundPos,		VAL_VOID,
		5, { (INTERP_TYPE)ST_SOUND, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"addConsoleText",		scrAddConsoleText,		VAL_VOID,
		2, { (INTERP_TYPE)ST_TEXTSTRING, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"showConsoleText",	scrShowConsoleText,		VAL_VOID,
		2, { (INTERP_TYPE)ST_TEXTSTRING, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"tagConsoleText",		scrTagConsoleText,		VAL_VOID,
		2, { (INTERP_TYPE)ST_TEXTSTRING, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"turnPowerOn",		scrTurnPowerOn,			VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"turnPowerOff",		scrTurnPowerOff,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"tutorialEnd",		scrTutorialEnd,			VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"clearConsole",		scrClearConsole,		VAL_VOID,
		0, { VAL_VOID},
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"playVideo",			scrPlayVideo,			VAL_VOID,
		2, { (INTERP_TYPE)ST_TEXTSTRING, (INTERP_TYPE)ST_TEXTSTRING },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"gameOverMessage",	scrGameOverMessage,		VAL_VOID,
		4, { (INTERP_TYPE)ST_INTMESSAGE, VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"gameOver",			scrGameOver,			VAL_VOID,
		1, { VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"playBackgroundAudio", scrPlayBackgroundAudio,	VAL_VOID,
		2, { (INTERP_TYPE)ST_TEXTSTRING, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"playIngameCDAudio",		scrPlayIngameCDAudio,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"stopCDAudio",		scrStopCDAudio,			VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"pauseCDAudio",		scrPauseCDAudio,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"resumeCDAudio",		scrResumeCDAudio,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setRetreatPoint",	scrSetRetreatPoint,		VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setRetreatForce",	scrSetRetreatForce,		VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setRetreatLeadership", scrSetRetreatLeadership, VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setGroupRetreatForce", scrSetGroupRetreatForce, VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setGroupRetreatLeadership", scrSetGroupRetreatLeadership, VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setGroupRetreatPoint", scrSetGroupRetreatPoint, VAL_VOID,
		3, { (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setRetreatHealth",	scrSetRetreatHealth,	VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setGroupRetreatHealth", scrSetGroupRetreatHealth, VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"startMission",		scrStartMission,		VAL_VOID,
		2, { VAL_INT, (INTERP_TYPE)ST_LEVEL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setSnow",			scrSetSnow,				VAL_VOID,
		1, { VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setRain",			scrSetRain,				VAL_VOID,
		1, { VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setBackgroundFog",	scrSetBackgroundFog,	VAL_VOID,
		1, { VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setDepthFog",		scrSetDepthFog,			VAL_VOID,
		1, { VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setFogColour",		scrSetFogColour,		VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setTransporterExit",	scrSetTransporterExit,	VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"flyTransporterIn",	scrFlyTransporterIn,	VAL_VOID,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"addDroidToTransporter", scrAddDroidToTransporter, VAL_VOID,
		2, { (INTERP_TYPE)ST_DROID, (INTERP_TYPE)ST_DROID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	/*{ "endMission",		scrEndMission,			VAL_VOID,
		1, { VAL_BOOL },
		0, 0, NULL, 0, 0, NULL, NULL },*/

	{
		"structureBuiltInRange", scrStructureBuiltInRange, (INTERP_TYPE)ST_STRUCTURE,
		5, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"random",				scrRandom,				VAL_INT,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"randomiseSeed",		scrRandomiseSeed,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"enableResearch",		scrEnableResearch,		VAL_VOID,
		2, { (INTERP_TYPE)ST_RESEARCH, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"completeResearch",	scrCompleteResearch,	VAL_VOID,
		2, { (INTERP_TYPE)ST_RESEARCH, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"flashOn",			scrFlashOn,				VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"flashOff",			scrFlashOff,			VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setPowerLevel",		scrSetPowerLevel,		VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"addPower",		    scrAddPower,		    VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setLandingZone",		scrSetLandingZone,		VAL_VOID,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setLimboLanding",	scrSetLimboLanding,		VAL_VOID,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setNoGoArea",		scrSetNoGoArea,			VAL_VOID,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"initAllNoGoAreas",	scrInitAllNoGoAreas,	VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setRadarZoom",		scrSetRadarZoom,		VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setReinforcementTime", scrSetReinforcementTime, VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setMissionTime",		scrSetMissionTime,		VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"missionTimeRemaining", scrMissionTimeRemaining, VAL_INT,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"flushConsoleMessages", scrFlushConsoleMessages, VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"pickStructLocation",	scrPickStructLocation,	VAL_BOOL,
		4, {(INTERP_TYPE)ST_STRUCTURESTAT, VAL_REF | VAL_INT, VAL_REF | VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"pickStructLocationB",	scrPickStructLocationB,	VAL_BOOL,
		5, {(INTERP_TYPE)ST_STRUCTURESTAT, VAL_REF | VAL_INT, VAL_REF | VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	// AI functions
	{
		"groupAddDroid",		scrGroupAddDroid,		VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, (INTERP_TYPE)ST_DROID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"groupAddArea",		scrGroupAddArea,		VAL_VOID,
		6, { (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"groupAddAreaNoGroup",	scrGroupAddAreaNoGroup,		VAL_VOID,
		6, { (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"groupAddGroup",		scrGroupAddGroup,		VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, (INTERP_TYPE)ST_GROUP },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"groupMember",		scrGroupMember,			VAL_BOOL,
		2, { (INTERP_TYPE)ST_GROUP, (INTERP_TYPE)ST_DROID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"idleGroup",			scrIdleGroup,			VAL_INT,
		1, { (INTERP_TYPE)ST_GROUP },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"initIterateGroup",	scrInitIterateGroup,	VAL_VOID,
		1, { (INTERP_TYPE)ST_GROUP },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"iterateGroup",		scrIterateGroup,	(INTERP_TYPE)ST_DROID,
		1, { (INTERP_TYPE)ST_GROUP },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"droidLeaveGroup",	scrDroidLeaveGroup,		VAL_VOID,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"orderDroid",			scrOrderDroid,			VAL_VOID,
		2, { (INTERP_TYPE)ST_DROID, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"orderDroidLoc",		scrOrderDroidLoc,		VAL_VOID,
		4, { (INTERP_TYPE)ST_DROID, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"orderDroidObj",		scrOrderDroidObj,		VAL_VOID,
		3, { (INTERP_TYPE)ST_DROID, VAL_INT, (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"orderDroidStatsLoc",	scrOrderDroidStatsLoc,	VAL_VOID,
		5, { (INTERP_TYPE)ST_DROID, VAL_INT, (INTERP_TYPE)ST_BASESTATS, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"orderGroup",			scrOrderGroup,			VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"orderGroupLoc",		scrOrderGroupLoc,		VAL_VOID,
		4, { (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"orderGroupObj",		scrOrderGroupObj,		VAL_VOID,
		3, { (INTERP_TYPE)ST_GROUP, VAL_INT, (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setDroidSecondary",	scrSetDroidSecondary,	VAL_VOID,
		3, { (INTERP_TYPE)ST_DROID, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"setGroupSecondary",	scrSetGroupSecondary,	VAL_VOID,
		3, { (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	//Commander functions
	{
		"cmdDroidAddDroid",	scrCmdDroidAddDroid,	VAL_VOID,
		2, { (INTERP_TYPE)ST_DROID, (INTERP_TYPE)ST_DROID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"cmdDroidMaxGroup",	scrCmdDroidMaxGroup,	VAL_INT,
		1, { (INTERP_TYPE)ST_DROID},
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	// a couple of example functions
	{
		"debugBox",			scrNumMB,				VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"approxRoot",			scrApproxRoot,			VAL_INT,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"refTest",			scrRefTest,				VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	// geo funcs
	{
		"distBetweenTwoPoints", scrDistanceTwoPts,		VAL_INT,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"losTwoObjects",		scrLOSTwoBaseObjects,	VAL_BOOL,
		3, { (INTERP_TYPE)ST_BASEOBJECT, (INTERP_TYPE)ST_BASEOBJECT, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"killStructsInArea",	scrDestroyStructuresInArea,	VAL_VOID,
		8, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getThreatInArea",	scrThreatInArea,		VAL_INT,
		10, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getNearestGateway",	scrGetNearestGateway,	VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_REF | VAL_INT, VAL_REF | VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setWaterTile",		scrSetWaterTile,		VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setRubbleTile",		scrSetRubbleTile,		VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setCampaignNumber",	scrSetCampaignNumber,	VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"testStructureModule", scrTestStructureModule, VAL_BOOL,
		3, {VAL_INT, (INTERP_TYPE)ST_STRUCTURE, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"killDroidsInArea",	scrDestroyUnitsInArea,	VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"vanishUnit",			scrRemoveDroid,			VAL_VOID,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"forceDamageObject",	scrForceDamage,			VAL_VOID,
		2, { (INTERP_TYPE)ST_BASEOBJECT, VAL_INT},
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	//multiplayer stuff.
	{
		"isHumanPlayer",		scrIsHumanPlayer,		VAL_BOOL,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"offerAlliance",		scrOfferAlliance,		VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"createAlliance",		scrCreateAlliance,		VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"breakAlliance",		scrBreakAlliance,		VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"allianceExists",		scrAllianceExists,		VAL_BOOL,
		0, {VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"allianceExistsBetween", scrAllianceExistsBetween, VAL_BOOL,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"playerInAlliance",	scrPlayerInAlliance,	VAL_BOOL,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"dominatingAlliance",	scrDominatingAlliance,	VAL_BOOL,
		0, { VAL_VOID } ,
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"myResponsibility",	scrMyResponsibility,	VAL_BOOL,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	// object conversion routines
	{
		"objToDroid",			scrObjToDroid,	(INTERP_TYPE)ST_DROID,
		1, { (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"objToStructure",		scrObjToStructure,	(INTERP_TYPE)ST_STRUCTURE,
		1, { (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"objToFeature",		scrObjToFeature,	(INTERP_TYPE)ST_FEATURE,
		1, { (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},


	{
		"getGameStatus", 		scrGetGameStatus,		VAL_BOOL,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	//player colour access functions
	{
		"getPlayerColour",	scrGetPlayerColour,		VAL_INT,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getPlayerColourName", scrGetPlayerColourName,	VAL_STRING,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setPlayerColour",	scrSetPlayerColour,		VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"takeOverDroidsInArea", scrTakeOverDroidsInArea, VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"takeOverDroidsInAreaExp", scrTakeOverDroidsInAreaExp, VAL_INT,
		8, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"takeOverStructsInArea", scrTakeOverStructsInArea, VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"takeOverSingleDroid", scrTakeOverSingleDroid,	(INTERP_TYPE)ST_DROID,
		2, { (INTERP_TYPE)ST_DROID, VAL_INT},
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"takeOverSingleStructure", scrTakeOverSingleStructure, (INTERP_TYPE)ST_STRUCTURE,
		2, { (INTERP_TYPE)ST_STRUCTURE, VAL_INT},
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"resetStructTargets",	scrResetStructTargets,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"resetDroidTargets",	scrResetDroidTargets,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"setStructTarPref",	scrSetStructTarPref,		VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"setStructTarIgnore",	scrSetStructTarIgnore,		VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"setDroidTarPref",	scrSetDroidTarPref,			VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"setDroidTarIgnore",	scrSetDroidTarIgnore,		VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"structTargetInArea",	scrStructTargetInArea,	(INTERP_TYPE)ST_STRUCTURE,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"structTargetOnMap",	scrStructTargetOnMap,	(INTERP_TYPE)ST_STRUCTURE,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"droidTargetInArea",	scrDroidTargetInArea,	(INTERP_TYPE)ST_DROID,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"droidTargetOnMap",	scrDroidTargetOnMap,	(INTERP_TYPE)ST_DROID,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"targetInCluster",	scrTargetInCluster,	(INTERP_TYPE)ST_DROID,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setDroidsToSafetyFlag", scrSetDroidsToSafetyFlag,	VAL_VOID,
		1, { VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setPlayCountDown",	scrSetPlayCountDown,	    VAL_VOID,
		1, { VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getDroidCount",		scrGetDroidCount,		    VAL_INT,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"fireWeaponAtObj",	scrFireWeaponAtObj,			VAL_VOID,
		2, { (INTERP_TYPE)ST_WEAPON, (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"fireWeaponAtLoc",	scrFireWeaponAtLoc,			VAL_VOID,
		3, { (INTERP_TYPE)ST_WEAPON, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setDroidKills",		scrSetDroidKills,			VAL_VOID,
		2, { (INTERP_TYPE)ST_DROID, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"getDroidKills",		scrGetDroidKills,			VAL_INT,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"resetPlayerVisibility", scrResetPlayerVisibility,	VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setVTOLReturnPos",	scrSetVTOLReturnPos,		VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"isVtol",				scrIsVtol,					VAL_BOOL,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"tutorialTemplates",	scrTutorialTemplates,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"resetLimboMission",	scrResetLimboMission,		VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

//ajl skirmish funcs
	{
		"skDoResearch",		scrSkDoResearch,			VAL_VOID,
		3, { (INTERP_TYPE)ST_STRUCTURE, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"skLocateEnemy",		scrSkLocateEnemy,	(INTERP_TYPE)ST_BASEOBJECT,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"skCanBuildTemplate",	scrSkCanBuildTemplate,		VAL_BOOL,
		3, { VAL_INT, (INTERP_TYPE)ST_STRUCTURE, (INTERP_TYPE)ST_TEMPLATE },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"skVtolEnableCheck",	scrSkVtolEnableCheck,		VAL_BOOL,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"skGetFactoryCapacity", scrSkGetFactoryCapacity,	VAL_INT,
		1, { (INTERP_TYPE)ST_STRUCTURE },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"skDifficultyModifier", scrSkDifficultyModifier,	VAL_VOID,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"skDefenseLocation",	scrSkDefenseLocation,		VAL_BOOL,
		6, {VAL_REF | VAL_INT, VAL_REF | VAL_INT, (INTERP_TYPE)ST_STRUCTURESTAT, (INTERP_TYPE)ST_STRUCTURESTAT, (INTERP_TYPE)ST_DROID, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"skFireLassat",		scrSkFireLassat,			VAL_VOID,
		2, {VAL_INT, (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	/* New functions */

	{
		"strcmp",				scrStrcmp,					VAL_BOOL,
		2, { VAL_STRING, VAL_STRING },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"console",			scrConsole,					VAL_VOID,
		1, { VAL_STRING },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"MsgBox",				scrMsgBox,					VAL_VOID,
		1, { VAL_STRING },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"dbgMsgOn",			scrDbgMsgOn,				VAL_VOID,
		2, { VAL_INT, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"dbg",				scrDbg,						VAL_VOID,
		2, { VAL_STRING, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},
	{
		"msg",				scrMsg,						VAL_VOID, //multiplayer msg
		3, { VAL_STRING, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"debug",				scrDebugFile,				VAL_VOID,
		1, { VAL_STRING },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getBit",				scrGetBit,					VAL_BOOL,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setBit",				scrSetBit,					VAL_INT,
		3, { VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"dropBeacon",			scrDropBeacon,				VAL_VOID,
		6, { VAL_STRING, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"removeBeacon",	scrRemoveBeacon,		VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	//droid functions
	//-----------------------------------
	{
		"actionDroidObj",		scrActionDroidObj,			VAL_VOID,
		3, { (INTERP_TYPE)ST_DROID, VAL_INT, (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"InitEnumDroids",		scrInitEnumDroids,			VAL_VOID,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"EnumDroid",			scrEnumDroid,	(INTERP_TYPE)ST_DROID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"initIterateGroupB",	scrInitIterateGroupB,		VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"iterateGroupB",		scrIterateGroupB,	(INTERP_TYPE)ST_DROID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"closestDamagedGroupDroid", scrClosestDamagedGroupDroid, (INTERP_TYPE)ST_DROID,
		6, { VAL_INT, (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},


	{
		"factoryGetTemplate",	scrFactoryGetTemplate,	(INTERP_TYPE)ST_TEMPLATE,
		1, { (INTERP_TYPE)ST_STRUCTURE },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numTemplatesInProduction", scrNumTemplatesInProduction, VAL_INT,
		2, { (INTERP_TYPE)ST_TEMPLATE, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numDroidsByComponent", scrNumDroidsByComponent,	VAL_INT,
		3, { (INTERP_TYPE)ST_COMPONENT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getStructureLimit",	scrGetStructureLimit,		VAL_INT,
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"structureLimitReached", scrStructureLimitReached,	VAL_BOOL,
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getNumStructures",	scrGetNumStructures,		VAL_INT,
		2, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getUnitLimit",		scrGetUnitLimit,			VAL_INT,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"min",				scrMin,						VAL_INT,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"max",				scrMax,						VAL_INT,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	//float equivalence for min()
	{
		"fmin",				scrFMin,					VAL_FLOAT,
		2, { VAL_FLOAT, VAL_FLOAT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"fmax",				scrFMax,					VAL_FLOAT,
		2, { VAL_FLOAT, VAL_FLOAT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"fogTileInRange",		scrFogTileInRange,			VAL_BOOL,
		9, { VAL_REF | VAL_INT, VAL_REF | VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},


	{
		"mapRevealedInRange",	scrMapRevealedInRange,		VAL_BOOL,
		4, { VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"mapTileVisible",		scrMapTileVisible,			VAL_BOOL,
		3, { VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"pursueResearch",		scrPursueResearch,			VAL_BOOL,
		3, { (INTERP_TYPE)ST_STRUCTURE, VAL_INT, (INTERP_TYPE)ST_RESEARCH },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numResearchLeft",	scrNumResearchLeft,			VAL_INT,
		2, { VAL_INT, (INTERP_TYPE)ST_RESEARCH },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"researchFinished",	scrResearchCompleted,		VAL_BOOL,
		2, { (INTERP_TYPE)ST_RESEARCH, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"researchStarted",	scrResearchStarted,			VAL_BOOL,
		2, { (INTERP_TYPE)ST_RESEARCH, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"threatInRange",		scrThreatInRange,			VAL_BOOL,
		5, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numEnemyWeapObjInRange", scrNumEnemyWeapObjInRange, VAL_INT,
		6, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numEnemyWeapDroidsInRange", scrNumEnemyWeapDroidsInRange, VAL_INT,
		5, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numEnemyWeapStructsInRange", scrNumEnemyWeapStructsInRange, VAL_INT,
		5, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numFriendlyWeapObjInRange", scrNumFriendlyWeapObjInRange, VAL_INT,
		6, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numFriendlyWeapDroidsInRange", scrNumFriendlyWeapDroidsInRange, VAL_INT,
		5, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numFriendlyWeapStructsInRange", scrNumFriendlyWeapStructsInRange, VAL_INT,
		5, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numPlayerWeapObjInRange", scrNumPlayerWeapObjInRange, VAL_INT,
		7, { VAL_INT, VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numPlayerWeapDroidsInRange",	scrNumPlayerWeapDroidsInRange,		VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numPlayerWeapStructsInRange", scrNumPlayerWeapStructsInRange,		VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numEnemyObjInRange",		scrNumEnemyObjInRange,			VAL_INT,
		6, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"enemyWeapObjCostInRange", scrEnemyWeapObjCostInRange, VAL_INT,
		6, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"friendlyWeapObjCostInRange", scrFriendlyWeapObjCostInRange, VAL_INT,
		6, { VAL_INT, VAL_INT , VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"alliancesLocked",		scrAlliancesLocked,		VAL_BOOL,
		0, {VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

//num structures
	{
		"numStructsByStatInRange", scrNumStructsByStatInRange, VAL_INT,
		6, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT , VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numStructsByStatInArea",	scrNumStructsByStatInArea, VAL_INT,
		7, { (INTERP_TYPE)ST_STRUCTURESTAT, VAL_INT , VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numStructsByTypeInRange", scrNumStructsByTypeInRange, VAL_INT,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numFeatByTypeInRange",	scrNumFeatByTypeInRange, VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numStructsButNotWallsInRangeVis", scrNumStructsButNotWallsInRangeVis, VAL_INT,
		5, { VAL_INT , VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"chooseValidLoc",			scrChooseValidLoc,		VAL_BOOL,
		6, { VAL_REF | VAL_INT, VAL_REF | VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"transporterCapacity",	scrTransporterCapacity,	VAL_INT,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"transporterFlying",		scrTransporterFlying,	VAL_BOOL,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"unloadTransporter",		scrUnloadTransporter,	VAL_VOID,
		3, { (INTERP_TYPE)ST_DROID, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"hasGroup",				scrHasGroup,			VAL_BOOL,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"objWeaponMaxRange",		scrObjWeaponMaxRange,	VAL_INT,
		1, { (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"objHasWeapon",			scrObjHasWeapon,		VAL_BOOL,
		1, { (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"objectHasIndirectWeapon", scrObjectHasIndirectWeapon, VAL_BOOL,
		1, { (INTERP_TYPE)ST_BASEOBJECT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getClosestEnemyDroidByType", scrGetClosestEnemyDroidByType, (INTERP_TYPE)ST_DROID,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_BOOL, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getClosestEnemyStructByType", scrGetClosestEnemyStructByType, (INTERP_TYPE)ST_STRUCTURE,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getClosestEnemy",		scrGetClosestEnemy,	(INTERP_TYPE)ST_BASEOBJECT,
		6, { VAL_INT, VAL_INT, VAL_INT, VAL_BOOL, VAL_BOOL, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"skDefenseLocationB",		scrSkDefenseLocationB,		VAL_BOOL,
		6, {VAL_REF | VAL_INT, VAL_REF | VAL_INT, (INTERP_TYPE)ST_STRUCTURESTAT, (INTERP_TYPE)ST_STRUCTURESTAT, (INTERP_TYPE)ST_DROID, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"circlePerimPoint",		scrCirclePerimPoint,		VAL_VOID,
		5, { VAL_INT, VAL_INT, VAL_REF | VAL_INT, VAL_REF | VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"structInRangeVis",		scrStructInRangeVis,		VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"droidInRangeVis",		scrDroidInRangeVis,			VAL_BOOL,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"giftRadar",				scrGiftRadar,				VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numAllies",				scrNumAllies,				VAL_INT,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"numAAinRange",			scrNumAAinRange,			VAL_INT,
		5, { VAL_INT, VAL_INT, VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"selectDroid",			scrSelectDroid,				VAL_VOID,
		2, { (INTERP_TYPE)ST_DROID, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"selectGroup",			scrSelectGroup,				VAL_VOID,
		2, { (INTERP_TYPE)ST_GROUP, VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"modulo",					scrModulo,					VAL_INT,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"playerLoaded",			scrPlayerLoaded,			VAL_BOOL,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getStructureType",		scrGetStructureType,		VAL_INT,
		1, { (INTERP_TYPE)ST_STRUCTURE },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getPlayerName",			scrGetPlayerName,			VAL_STRING,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setPlayerName",			scrSetPlayerName,			VAL_BOOL,
		2, { VAL_INT, VAL_STRING },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"ASSERT",					scrASSERT,					VAL_VOID,
		3, { VAL_BOOL, VAL_STRING, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"showRangeAtPos",			scrShowRangeAtPos,			VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"toPow",					scrToPow,					VAL_FLOAT,
		2, { VAL_FLOAT, VAL_FLOAT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"exp",				scrExp,							VAL_FLOAT,
		1, { VAL_FLOAT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"sqrt",				scrSqrt,						VAL_FLOAT,
		1, { VAL_FLOAT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"log",				scrLog,						VAL_FLOAT,
		1, { VAL_FLOAT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"debugMenu",				scrDebugMenu,				VAL_VOID,
		1, { VAL_BOOL },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setDebugMenuEntry",		scrSetDebugMenuEntry,		VAL_VOID,
		2, { VAL_STRING, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	//scripting engine chat interface functions
	{
		"processChatMsg",			scrProcessChatMsg,			VAL_INT,
		1, { VAL_STRING },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getChatCmdDescription",	scrGetChatCmdDescription,	VAL_STRING,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getNumArgsInCmd",		scrGetNumArgsInCmd,			VAL_INT,
		1, { VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getChatCmdParam",		scrGetChatCmdParam,			VAL_BOOL,
		3, { VAL_REF | VAL_VOID, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"chatCmdIsPlayerAddressed",	scrChatCmdIsPlayerAddressed,	VAL_BOOL,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setTileHeight",			scrSetTileHeight,			VAL_VOID,
		3, { VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getTileStructure",		scrGetTileStructure,	(INTERP_TYPE)ST_STRUCTURE,
		2, { VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"printCallStack",		scrPrintCallStack,				VAL_VOID,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"debugModeEnabled",		scrDebugModeEnabled,		VAL_BOOL,
		0, { VAL_VOID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"calcDroidPower",			scrCalcDroidPower,			VAL_INT,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getDroidRank",			scrGetDroidLevel,			VAL_INT,
		1, { (INTERP_TYPE)ST_DROID },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"assembleWeaponTemplate",		scrAssembleWeaponTemplate,	(INTERP_TYPE)ST_TEMPLATE,
		4, { VAL_INT, (INTERP_TYPE)ST_BODY, (INTERP_TYPE)ST_PROPULSION, (INTERP_TYPE)ST_WEAPON },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"weaponLongHitUpgrade",	scrWeaponLongHitUpgrade,	VAL_INT,
		2, { VAL_INT, (INTERP_TYPE)ST_WEAPON },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"weaponDamageUpgrade",	scrWeaponDamageUpgrade,		VAL_INT,
		2, { VAL_INT, (INTERP_TYPE)ST_WEAPON },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"weaponFirePauseUpgrade",	scrWeaponFirePauseUpgrade,	VAL_INT,
		2, { VAL_INT, (INTERP_TYPE)ST_WEAPON },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"isComponentAvailable",	scrIsComponentAvailable,	VAL_BOOL,
		2, { VAL_INT, (INTERP_TYPE)ST_COMPONENT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getBodySize",			scrGetBodySize,				VAL_INT,
		1, { (INTERP_TYPE)ST_BODY },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	// Gettext functions
	{
		"_", scrGettext, (INTERP_TYPE)ST_TEXTSTRING,
		1, { VAL_STRING },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"gettext", scrGettext, (INTERP_TYPE)ST_TEXTSTRING,
		1, { VAL_STRING },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"N_", scrGettext_noop, VAL_STRING,
		1, { VAL_STRING },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"gettext_noop", scrGettext_noop, VAL_STRING,
		1, { VAL_STRING },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"P_", scrPgettext, (INTERP_TYPE)ST_TEXTSTRING,
		2, { VAL_STRING, VAL_STRING },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"pgettext", scrPgettext, (INTERP_TYPE)ST_TEXTSTRING,
		2, { VAL_STRING, VAL_STRING },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"PE_", scrPgettext_expr, (INTERP_TYPE)ST_TEXTSTRING,
		2, { VAL_STRING, VAL_STRING },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"NP_", scrPgettext_noop, VAL_STRING,
		2, { VAL_STRING, VAL_STRING },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"droidCanReach", scrDroidCanReach, VAL_BOOL,
		3, { (INTERP_TYPE)ST_DROID, VAL_INT, VAL_INT },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getPlayer", scrGetPlayer, VAL_INT,
		1, { VAL_STRING },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getPlayerStartPosition", scrGetPlayerStartPosition, VAL_BOOL,
		3, { VAL_INT, VAL_INT | VAL_REF, VAL_INT | VAL_REF },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"scavengersActive", scrScavengersActive, VAL_INT,
		0, { VAL_VOID },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{ "initEnumUnbuilt", scrEnumUnbuilt, VAL_VOID, 1, { VAL_INT }, false, 0, nullptr, 0, 0, nullptr, nullptr },
	{ "enumUnbuilt", scrIterateUnbuilt, (INTERP_TYPE)ST_STRUCTURE, 0, { VAL_VOID }, false, 0, nullptr, 0, 0, nullptr, nullptr },

	{
		"pickDroidStructLocation",	scrPickStructLocationC,	VAL_BOOL,
		6, {(INTERP_TYPE)ST_DROID, (INTERP_TYPE)ST_STRUCTURESTAT, VAL_REF | VAL_INT, VAL_REF | VAL_INT, VAL_INT, VAL_INT },
		0, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"safeDest", scrSafeDest, VAL_BOOL,
		3, { VAL_INT, VAL_INT, VAL_INT },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"threatAt", scrThreatAt, VAL_BOOL,
		3, { VAL_INT, VAL_INT, VAL_INT },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setSunPosition", scrSetSunPosition, VAL_VOID,
		3, { VAL_FLOAT, VAL_FLOAT, VAL_FLOAT },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"setSunIntensity", scrSetSunIntensity, VAL_VOID,
		9, { VAL_FLOAT, VAL_FLOAT, VAL_FLOAT, VAL_FLOAT, VAL_FLOAT, VAL_FLOAT, VAL_FLOAT, VAL_FLOAT, VAL_FLOAT },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getDifficulty", scrGetDifficulty, VAL_INT,
		1, { VAL_INT },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	{
		"getDerrick", scrGetDerrick, (INTERP_TYPE)ST_BASEOBJECT,
		1, { VAL_INT },
		false, 0, nullptr, 0, 0, nullptr, nullptr
	},

	/* This final entry marks the end of the function list */
	{ "FUNCTION LIST END", nullptr, VAL_VOID, 0, { VAL_VOID }, 0, 0, nullptr, 0, 0, nullptr, nullptr }
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
	{
		"trackTransporter", VAL_BOOL,	ST_EXTERN,	0,
		EXTID_TRACKTRANSPORTER,	scrGenExternGet,	nullptr, 0, {0}, nullptr
	},

	{
		"mapWidth",		VAL_INT,	ST_EXTERN,	0,
		EXTID_MAPWIDTH,		scrGenExternGet,		nullptr, 0, {0}, nullptr
	},
	{
		"mapHeight",		VAL_INT,	ST_EXTERN,	0,
		EXTID_MAPHEIGHT,	scrGenExternGet,		nullptr, 0, {0}, nullptr
	},

	{
		"gameInitialised", VAL_BOOL,	ST_EXTERN,	0,
		EXTID_GAMEINIT,		scrGenExternGet,		nullptr, 0, {0}, nullptr
	},

	{
		"selectedPlayer",	VAL_INT,	ST_EXTERN,	0,
		EXTID_SELECTEDPLAYER,	scrGenExternGet,	nullptr, 0, {0}, nullptr
	},
	{
		"gameTime",		VAL_INT,	ST_EXTERN,	0,
		EXTID_GAMETIME,		scrGenExternGet,		nullptr, 0, {0}, nullptr
	},

	{
		"gameLevel",		VAL_INT,	ST_EXTERN,	0,
		EXTID_GAMELEVEL,	scrGenExternGet,		scrGenExternSet, 0, {0}, nullptr
	},
	{
		"inTutorial",		VAL_BOOL,	ST_EXTERN,	0,
		EXTID_TUTORIAL,		scrGenExternGet,		scrGenExternSet, 0, {0}, nullptr
	},
	{
		"cursorType",		VAL_INT,	ST_EXTERN,	0,
		EXTID_CURSOR,		scrGenExternGet,		nullptr, 0, {0}, nullptr
	},


	{
		"intMode",		VAL_INT,	ST_EXTERN,	0,
		EXTID_INTMODE,		scrGenExternGet,		nullptr, 0, {0}, nullptr
	},

	{
		"targetedObjectType",	VAL_INT, ST_EXTERN,	0,
		EXTID_TARGETTYPE,	scrGenExternGet,		nullptr, 0, {0}, nullptr
	},
	{
		"extraVictoryFlag", VAL_BOOL,	ST_EXTERN,	0,
		EXTID_EXTRAVICTORYFLAG,	scrGenExternGet,	scrGenExternSet, 0, {0}, nullptr
	},
	{
		"extraFailFlag",	VAL_BOOL,	ST_EXTERN,	0,
		EXTID_EXTRAFAILFLAG, scrGenExternGet,		scrGenExternSet, 0, {0}, nullptr
	},

	{
		"multiPlayerGameType", VAL_INT, ST_EXTERN,	0,
		EXTID_MULTIGAMETYPE, scrGenExternGet,		nullptr, 0, {0}, nullptr
	},

	{
		"multiPlayerMaxPlayers", VAL_INT, ST_EXTERN, 0,
		EXTID_MULTIGAMEHUMANMAX, scrGenExternGet,	nullptr, 0, {0}, nullptr
	},

	{
		"multiPlayerBaseType", VAL_INT, ST_EXTERN,	0,
		EXTID_MULTIGAMEBASETYPE, scrGenExternGet,	nullptr, 0, {0}, nullptr
	},

	{
		"multiPlayerAlliancesType", VAL_INT, ST_EXTERN, 0,
		EXTID_MULTIGAMEALLIANCESTYPE, scrGenExternGet, nullptr, 0, {0}, nullptr
	},

	/* This entry marks the end of the variable list */
	{ nullptr, VAL_VOID, ST_EXTERN, 0, 0, nullptr, nullptr, 0, {0}, nullptr }
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
	{
		"x",			VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_POSX,		scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"y",			VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_POSY,		scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"z",			VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_POSZ,		scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"id",			VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_ID,		scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"player",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_PLAYER,	scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"type",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_TYPE,		scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"clusterID",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_CLUSTERID, scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"health",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_HEALTH,	scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"weapon",	(INTERP_TYPE)ST_WEAPON,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,		OBJID_WEAPON,	scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},

	// object's hit points
	{
		"hitPoints",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_HITPOINTS,	scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},

	// object's original hit points (when not damaged)
	{
		"origHitPoints",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_ORIG_HITPOINTS,	scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},

	// droid variables
	{
		"order",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_ORDER,	scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"droidType",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_DROIDTYPE, scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"body",	(INTERP_TYPE)ST_BODY,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_BODY,		scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"propulsion",	(INTERP_TYPE)ST_PROPULSION,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_PROPULSION, scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"orderx",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_ORDERX,	scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"ordery",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_ORDERY,	scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"group",	(INTERP_TYPE)ST_GROUP,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_GROUP,	scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},

	// structure variables
	//{ "stat",			(INTERP_TYPE)ST_STRUCTURESTAT,	ST_OBJECT,	(INTERP_TYPE)ST_STRUCTURE,	OBJID_STRUCTSTAT,
	//	scrBaseObjGet,			NULL, 0, {0}, NULL },


	// group variables
	{
		"x",			VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_GROUP,		GROUPID_POSX,	scrGroupObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"y",			VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_GROUP,		GROUPID_POSY,	scrGroupObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"members",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_GROUP,		GROUPID_MEMBERS, scrGroupObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"health",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_GROUP,		GROUPID_HEALTH,	scrGroupObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"type",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_GROUP,		GROUPID_TYPE,	scrGroupObjGet,	nullptr, 0, {0}, nullptr
	},
	{
		"commander",	(INTERP_TYPE)ST_DROID,	ST_OBJECT,
		(INTERP_TYPE)ST_GROUP,		GROUPID_CMD,	scrGroupObjGet,	nullptr, 0, {0}, nullptr
	},

	/* new member variables */
	//similar to .order
	{
		"action",		VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_ACTION,	scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},

	//.stat - now supports droids, ST_STRUCTURE became ST_BASEOBJECT
	{
		"stat",	(INTERP_TYPE)ST_STRUCTURESTAT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_STRUCTSTAT, scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},

	//object->psTarget
	{
		"target",	(INTERP_TYPE)ST_BASEOBJECT,	ST_OBJECT,
		(INTERP_TYPE)ST_BASEOBJECT,	OBJID_TARGET,	scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},

	//returns psStruct->pStructureType->type
	{
		"stattype",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_STRUCTURE,	OBJID_STRUCTSTATTYPE, scrBaseObjGet, nullptr, 0, {0}, nullptr
	},

	//returns if this unit is currently selected by a player (usually human)
	{
		"selected",	VAL_BOOL,	ST_OBJECT,
		(INTERP_TYPE)ST_DROID,		OBJID_SELECTED,	scrBaseObjGet,	nullptr, 0, {0}, nullptr
	},

	/* Weapon Stats */

	//weapon long range
	{
		"longRange",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_WEAPON,	WEAPID_LONG_RANGE,	scrWeaponObjGet,	nullptr, 0, {0}, nullptr
	},

	//weapon long hit chance
	{
		"longHit",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_WEAPON,	WEAPID_LONG_HIT,	scrWeaponObjGet,	nullptr, 0, {0}, nullptr
	},

	//weapon damage
	{
		"damage",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_WEAPON,	WEAPID_DAMAGE,		scrWeaponObjGet,	nullptr, 0, {0}, nullptr
	},

	//weapon fire pause
	{
		"firePause",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_WEAPON,	WEAPID_FIRE_PAUSE,	scrWeaponObjGet,	nullptr, 0, {0}, nullptr
	},

	//weapon reload time
	{
		"reloadTime",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_WEAPON,	WEAPID_RELOAD_TIME,	scrWeaponObjGet,	nullptr, 0, {0}, nullptr
	},

	//num of weapon's rounds (salvo fire)
	{
		"numRounds",	VAL_INT,	ST_OBJECT,
		(INTERP_TYPE)ST_WEAPON,	WEAPID_NUM_ROUNDS,	scrWeaponObjGet,	nullptr, 0, {0}, nullptr
	},

	/* This entry marks the end of the variable list */
	{ nullptr, VAL_VOID, (INTERP_TYPE)ST_OBJECT, VAL_VOID, 0, nullptr, nullptr, 0, {0}, nullptr }
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
	{ "OPTIONS",	VAL_INT,	false,	IDRET_OPTIONS,		nullptr, nullptr, 0.0f },
	{ "CANCEL",		VAL_INT,	false,	IDRET_CANCEL,		nullptr, nullptr, 0.0f },
	{ "BUILD",		VAL_INT,	false,	IDRET_BUILD,		nullptr, nullptr, 0.0f },
	{ "MANUFACTURE", VAL_INT,	false,	IDRET_MANUFACTURE,	nullptr, nullptr, 0.0f },
	{ "RESEARCH",	VAL_INT,	false,	IDRET_RESEARCH,		nullptr, nullptr, 0.0f },
	{ "INTELMAP",	VAL_INT,	false,	IDRET_INTEL_MAP,	nullptr, nullptr, 0.0f },
	{ "DESIGN",		VAL_INT,	false,	IDRET_DESIGN,		nullptr, nullptr, 0.0f },
	{ "COMMAND",	VAL_INT,	false,	IDRET_COMMAND,		nullptr, nullptr, 0.0f },

	// new styley that supports many other buttons
	{ "IDRET_OPTIONS",		VAL_INT,	false,	IDRET_OPTIONS,		nullptr, nullptr, 0.0f },
	{ "IDRET_CANCEL",		VAL_INT,	false,	IDRET_CANCEL,		nullptr, nullptr, 0.0f },
	{ "IDRET_BUILD",		VAL_INT,	false,	IDRET_BUILD,		nullptr, nullptr, 0.0f },
	{ "IDRET_MANUFACTURE",	VAL_INT,	false,	IDRET_MANUFACTURE,	nullptr, nullptr, 0.0f },
	{ "IDRET_RESEARCH",		VAL_INT,	false,	IDRET_RESEARCH,		nullptr, nullptr, 0.0f },
	{ "IDRET_INTELMAP",		VAL_INT,	false,	IDRET_INTEL_MAP,	nullptr, nullptr, 0.0f },
	{ "IDRET_DESIGN",		VAL_INT,	false,	IDRET_DESIGN,		nullptr, nullptr, 0.0f },
	{ "IDRET_COMMAND",		VAL_INT,	false,	IDRET_COMMAND,		nullptr, nullptr, 0.0f },
	{ "IDRET_TRANSPORTER",	VAL_INT,	false,	IDRET_TRANSPORTER,	nullptr, nullptr, 0.0f },

	// Design screen buttons
	{ "IDDES_TEMPLSTART",	VAL_INT,	false,	IDDES_TEMPLSTART,	nullptr, nullptr, 0.0f },
	{ "IDDES_SYSTEMBUTTON",	VAL_INT,	false,	IDDES_SYSTEMBUTTON,	nullptr, nullptr, 0.0f },
	{ "IDDES_BODYBUTTON",	VAL_INT,	false,	IDDES_BODYBUTTON,	nullptr, nullptr, 0.0f },
	{ "IDDES_PROPBUTTON",	VAL_INT,	false,	IDDES_PROPBUTTON,	nullptr, nullptr, 0.0f },

	// the first (top-left)  button on the object window (right of the reticule window)
	{ "IDOBJ_STATSTART",	VAL_INT,	false,	IDOBJ_STATSTART,	nullptr, nullptr, 0.0f },

	// one below obj_statstart
	{ "IDOBJ_OBJSTART",		VAL_INT,	false,	IDOBJ_OBJSTART,		nullptr, nullptr, 0.0f },

	// the first (top-left) button on the list window (up from the reticule)
	{ "IDSTAT_START",		VAL_INT,	false,	IDSTAT_START,		nullptr, nullptr, 0.0f },

	//message Types
	{ "RES_MSG",	VAL_INT,	false,	MSG_RESEARCH,	nullptr, nullptr, 0.0f },
	{ "CAMP_MSG",	VAL_INT,	false,	MSG_CAMPAIGN,	nullptr, nullptr, 0.0f },
	{ "MISS_MSG",	VAL_INT,	false,	MSG_MISSION,	nullptr, nullptr, 0.0f },
	{ "PROX_MSG",	VAL_INT,	false,	MSG_PROXIMITY,	nullptr, nullptr, 0.0f },

	//used for null pointers
	{ "NULLTEMPLATE",	(INTERP_TYPE)ST_POINTER_T,	false,	0,	nullptr, nullptr, 0.0f },
	{ "NULLOBJECT",	(INTERP_TYPE)ST_POINTER_O,	false,	0,	nullptr, nullptr, 0.0f },
	{ "NULLSTAT",	(INTERP_TYPE)ST_POINTER_S,	false,	0,	nullptr, nullptr, 0.0f },
	{ "NULLSTRING",	(INTERP_TYPE)ST_TEXTSTRING,	false,	0,	nullptr, nullptr, 0.0f },
	{ "NULLSTRUCTURESTAT", (INTERP_TYPE)ST_POINTER_STRUCTSTAT,	false,	0,	nullptr, nullptr, 0.0f },	//for NULLSTRUCTURESTAT

	//#define used to set the reinforcement timer with
	{ "LZ_COMPROMISED_TIME", VAL_INT,	false,	SCR_LZ_COMPROMISED_TIME,	nullptr, nullptr, 0.0f },

	// BASEOBJ types
	{ "OBJ_DROID",		VAL_INT,	false,	OBJ_DROID,		nullptr, nullptr, 0.0f },
	{ "OBJ_STRUCTURE",	VAL_INT,	false,	OBJ_STRUCTURE,	nullptr, nullptr, 0.0f },
	{ "OBJ_FEATURE",	VAL_INT,	false,	OBJ_FEATURE,	nullptr, nullptr, 0.0f },
	//mission Types
	{ "CAMP_START",	VAL_INT,	false,	LDS_CAMSTART,		nullptr, nullptr, 0.0f },
	{ "CAMP_EXPAND", VAL_INT,	false,	LDS_EXPAND,			nullptr, nullptr, 0.0f },
	{ "OFF_KEEP",	VAL_INT,	false,	LDS_MKEEP,			nullptr, nullptr, 0.0f },
	{ "OFF_CLEAR",	VAL_INT,	false,	LDS_MCLEAR,			nullptr, nullptr, 0.0f },
	{ "BETWEEN",	VAL_INT,	false,	LDS_BETWEEN,		nullptr, nullptr, 0.0f },
	// droid types
	{ "DROID_WEAPON",			VAL_INT,	false,	DROID_WEAPON,			nullptr, nullptr, 0.0f },
	{ "DROID_SENSOR",			VAL_INT,	false,	DROID_SENSOR,			nullptr, nullptr, 0.0f },
	{ "DROID_ECM",				VAL_INT,	false,	DROID_ECM,				nullptr, nullptr, 0.0f },
	{ "DROID_CONSTRUCT",		VAL_INT,	false,	DROID_CONSTRUCT,		nullptr, nullptr, 0.0f },
	{ "DROID_PERSON",			VAL_INT,	false,	DROID_PERSON,			nullptr, nullptr, 0.0f },
	{ "DROID_CYBORG",			VAL_INT,	false,	DROID_CYBORG,			nullptr, nullptr, 0.0f },
	{ "DROID_CYBORG_SUPER",		VAL_INT,	false,	DROID_CYBORG_SUPER,		nullptr, nullptr, 0.0f },
	{ "DROID_SUPERTRANSPORTER",	VAL_INT,	false,	DROID_SUPERTRANSPORTER,	nullptr, nullptr, 0.0f },
	{ "DROID_TRANSPORTER",		VAL_INT,	false,	DROID_TRANSPORTER,		nullptr, nullptr, 0.0f },
	{ "DROID_COMMAND",			VAL_INT,	false,	DROID_COMMAND,			nullptr, nullptr, 0.0f },
	{ "DROID_REPAIR",			VAL_INT,	false,	DROID_REPAIR,			nullptr, nullptr, 0.0f },
	{ "DROID_CYBORG_REPAIR",	VAL_INT,	false,	DROID_CYBORG_REPAIR,	nullptr, nullptr, 0.0f },
	{ "DROID_CYBORG_CONSTRUCT",	VAL_INT,	false,	DROID_CYBORG_CONSTRUCT,	nullptr, nullptr, 0.0f },

	// structure types
	{ "REF_HQ",				VAL_INT,	false,	REF_HQ,					nullptr, nullptr, 0.0f },
	{ "REF_FACTORY",		VAL_INT,	false,	REF_FACTORY,			nullptr, nullptr, 0.0f },
	{ "REF_FACTORY_MODULE",	VAL_INT,	false,	REF_FACTORY_MODULE,		nullptr, nullptr, 0.0f },
	{ "REF_POWER_GEN",		VAL_INT,	false,	REF_POWER_GEN,			nullptr, nullptr, 0.0f },
	{ "REF_POWER_MODULE",	VAL_INT,	false,	REF_POWER_MODULE,		nullptr, nullptr, 0.0f },
	{ "REF_RESOURCE_EXTRACTOR", VAL_INT,	false,	REF_RESOURCE_EXTRACTOR,	nullptr, nullptr, 0.0f },
	{ "REF_DEFENSE",		VAL_INT,	false,	REF_DEFENSE,			nullptr, nullptr, 0.0f },
	{ "REF_WALL",			VAL_INT,	false,	REF_WALL,				nullptr, nullptr, 0.0f },
	{ "REF_WALLCORNER",		VAL_INT,	false,	REF_WALLCORNER,			nullptr, nullptr, 0.0f },
	{ "REF_RESEARCH",		VAL_INT,	false,	REF_RESEARCH,			nullptr, nullptr, 0.0f },
	{ "REF_RESEARCH_MODULE", VAL_INT,	false,	REF_RESEARCH_MODULE,	nullptr, nullptr, 0.0f },
	{ "REF_REPAIR_FACILITY", VAL_INT,	false,	REF_REPAIR_FACILITY,	nullptr, nullptr, 0.0f },
	{ "REF_COMMAND_CONTROL", VAL_INT,	false,	REF_COMMAND_CONTROL,	nullptr, nullptr, 0.0f },
	{ "REF_CYBORG_FACTORY",	VAL_INT,	false,	REF_CYBORG_FACTORY,		nullptr, nullptr, 0.0f },
	{ "REF_VTOL_FACTORY",	VAL_INT,	false,	REF_VTOL_FACTORY,		nullptr, nullptr, 0.0f },
	{ "REF_REARM_PAD",		VAL_INT,	false,	REF_REARM_PAD,		    nullptr, nullptr, 0.0f },
	{ "REF_MISSILE_SILO",	VAL_INT,	false,	REF_MISSILE_SILO,		nullptr, nullptr, 0.0f },


	// primary orders
	{ "DORDER_NONE",		VAL_INT,	false,	DORDER_NONE,		nullptr, nullptr, 0.0f },
	{ "DORDER_STOP",		VAL_INT,	false,	DORDER_STOP,		nullptr, nullptr, 0.0f },
	{ "DORDER_MOVE",		VAL_INT,	false,	DORDER_MOVE,		nullptr, nullptr, 0.0f },
	{ "DORDER_ATTACK",		VAL_INT,	false,	DORDER_ATTACK,		nullptr, nullptr, 0.0f },
	{ "DORDER_BUILD",		VAL_INT,	false,	DORDER_BUILD,		nullptr, nullptr, 0.0f },
	{ "DORDER_HELPBUILD",	VAL_INT,	false,	DORDER_HELPBUILD,	nullptr, nullptr, 0.0f },
	{ "DORDER_LINEBUILD",	VAL_INT,	false,	DORDER_LINEBUILD,	nullptr, nullptr, 0.0f },
	{ "DORDER_DEMOLISH",	VAL_INT,	false,	DORDER_DEMOLISH,	nullptr, nullptr, 0.0f },
	{ "DORDER_REPAIR",		VAL_INT,	false,	DORDER_REPAIR,		nullptr, nullptr, 0.0f },
	{ "DORDER_OBSERVE",		VAL_INT,	false,	DORDER_OBSERVE,		nullptr, nullptr, 0.0f },
	{ "DORDER_FIRESUPPORT",	VAL_INT,	false,	DORDER_FIRESUPPORT,	nullptr, nullptr, 0.0f },
	{ "DORDER_RETREAT",		VAL_INT,	false,	DORDER_RETREAT,		nullptr, nullptr, 0.0f },
	{ "DORDER_RTB",			VAL_INT,	false,	DORDER_RTB,			nullptr, nullptr, 0.0f },
	{ "DORDER_RTR",			VAL_INT,	false,	DORDER_RTR,			nullptr, nullptr, 0.0f },
	{ "DORDER_RUN",			VAL_INT,	false,	DORDER_RUN,			nullptr, nullptr, 0.0f },
	{ "DORDER_EMBARK",		VAL_INT,	false,	DORDER_EMBARK,		nullptr, nullptr, 0.0f },
	{ "DORDER_DISEMBARK",	VAL_INT,	false,	DORDER_DISEMBARK,	nullptr, nullptr, 0.0f },
	{ "DORDER_SCOUT",		VAL_INT,	false,	DORDER_SCOUT,		nullptr, nullptr, 0.0f },
	{ "DORDER_DROIDREPAIR",	VAL_INT,	false,	DORDER_DROIDREPAIR,	nullptr, nullptr, 0.0f },
	{ "DORDER_GUARD",		VAL_INT,	false,	DORDER_GUARD,		nullptr, nullptr, 0.0f },


	//new member varialbe - constants for .action
	//-----------------------------------------------------------
	{ "DACTION_NONE",				VAL_INT,	false,	DACTION_NONE,				nullptr, nullptr, 0.0f },
	{ "DACTION_MOVE",				VAL_INT,	false,	DACTION_MOVE,				nullptr, nullptr, 0.0f },
	{ "DACTION_BUILD",				VAL_INT,	false,	DACTION_BUILD,				nullptr, nullptr, 0.0f },
	{ "DACTION_DEMOLISH",			VAL_INT,	false,	DACTION_DEMOLISH,			nullptr, nullptr, 0.0f },
	{ "DACTION_REPAIR",				VAL_INT,	false,	DACTION_REPAIR,				nullptr, nullptr, 0.0f },
	{ "DACTION_ATTACK",				VAL_INT,	false,	DACTION_ATTACK,				nullptr, nullptr, 0.0f },
	{ "DACTION_OBSERVE",			VAL_INT,	false,	DACTION_OBSERVE,			nullptr, nullptr, 0.0f },
	{ "DACTION_FIRESUPPORT",		VAL_INT,	false,	DACTION_FIRESUPPORT,		nullptr, nullptr, 0.0f },
	{ "DACTION_SULK",				VAL_INT,	false,	DACTION_SULK,				nullptr, nullptr, 0.0f },

	{ "DACTION_TRANSPORTOUT",		VAL_INT,	false,	DACTION_TRANSPORTOUT,		nullptr, nullptr, 0.0f },
	{ "DACTION_TRANSPORTWAITTOFLYIN", VAL_INT,	false,	DACTION_TRANSPORTWAITTOFLYIN, nullptr, nullptr, 0.0f },
	{ "DACTION_TRANSPORTIN",		VAL_INT,	false,	DACTION_TRANSPORTIN,		nullptr, nullptr, 0.0f },
	{ "DACTION_DROIDREPAIR",		VAL_INT,	false,	DACTION_DROIDREPAIR,		nullptr, nullptr, 0.0f },
	{ "DACTION_RESTORE",			VAL_INT,	false,	DACTION_RESTORE,			nullptr, nullptr, 0.0f },
	{ "DACTION_MOVEFIRE",			VAL_INT,	false,	DACTION_MOVEFIRE,			nullptr, nullptr, 0.0f },
	{ "DACTION_MOVETOBUILD",		VAL_INT,	false,	DACTION_MOVETOBUILD,		nullptr, nullptr, 0.0f },
	{ "DACTION_MOVETODEMOLISH",		VAL_INT,	false,	DACTION_MOVETODEMOLISH,		nullptr, nullptr, 0.0f },

	{ "DACTION_MOVETOREPAIR",		VAL_INT,	false,	DACTION_MOVETOREPAIR,		nullptr, nullptr, 0.0f },
	{ "DACTION_BUILDWANDER",		VAL_INT,	false,	DACTION_BUILDWANDER,		nullptr, nullptr, 0.0f },
	{ "DACTION_MOVETOATTACK",		VAL_INT,	false,	DACTION_MOVETOATTACK,		nullptr, nullptr, 0.0f },
	{ "DACTION_ROTATETOATTACK",		VAL_INT,	false,	DACTION_ROTATETOATTACK,		nullptr, nullptr, 0.0f },
	{ "DACTION_MOVETOOBSERVE",		VAL_INT,	false,	DACTION_MOVETOOBSERVE,		nullptr, nullptr, 0.0f },
	{ "DACTION_WAITFORREPAIR",		VAL_INT,	false,	DACTION_WAITFORREPAIR,		nullptr, nullptr, 0.0f },
	{ "DACTION_MOVETOREPAIRPOINT",	VAL_INT,	false,	DACTION_MOVETOREPAIRPOINT,	nullptr, nullptr, 0.0f },
	{ "DACTION_WAITDURINGREPAIR",	VAL_INT,	false,	DACTION_WAITDURINGREPAIR,	nullptr, nullptr, 0.0f },
	{ "DACTION_MOVETODROIDREPAIR",	VAL_INT,	false,	DACTION_MOVETODROIDREPAIR,	nullptr, nullptr, 0.0f },

	{ "DACTION_MOVETORESTORE",		VAL_INT,	false,	DACTION_MOVETORESTORE,		nullptr, nullptr, 0.0f },
	{ "DACTION_MOVETOREARM",		VAL_INT,	false,	DACTION_MOVETOREARM,		nullptr, nullptr, 0.0f },
	{ "DACTION_WAITFORREARM",		VAL_INT,	false,	DACTION_WAITFORREARM,		nullptr, nullptr, 0.0f },
	{ "DACTION_MOVETOREARMPOINT",	VAL_INT,	false,	DACTION_MOVETOREARMPOINT,	nullptr, nullptr, 0.0f },
	{ "DACTION_WAITDURINGREARM",	VAL_INT,	false,	DACTION_WAITDURINGREARM,	nullptr, nullptr, 0.0f },
	{ "DACTION_VTOLATTACK",			VAL_INT,	false,	DACTION_VTOLATTACK,			nullptr, nullptr, 0.0f },
	{ "DACTION_CLEARREARMPAD",		VAL_INT,	false,	DACTION_CLEARREARMPAD,		nullptr, nullptr, 0.0f },
	{ "DACTION_RETURNTOPOS",		VAL_INT,	false,	DACTION_RETURNTOPOS,		nullptr, nullptr, 0.0f },
	{ "DACTION_FIRESUPPORT_RETREAT", VAL_INT,	false,	DACTION_FIRESUPPORT_RETREAT, nullptr, nullptr, 0.0f },

	// secondary orders
	{ "DSO_REPAIR_LEVEL",	VAL_INT,	false,	DSO_REPAIR_LEVEL,	nullptr, nullptr, 0.0f },
	{ "DSO_ATTACK_LEVEL",	VAL_INT,	false,	DSO_ATTACK_LEVEL,	nullptr, nullptr, 0.0f },
	{ "DSO_RECYCLE",		VAL_INT,	false,	DSO_RECYCLE,		nullptr, nullptr, 0.0f },
	{ "DSO_PATROL",			VAL_INT,	false,	DSO_PATROL,			nullptr, nullptr, 0.0f },
	{ "DSO_RETURN_TO_LOC",	VAL_INT,	false,	DSO_RETURN_TO_LOC,	nullptr, nullptr, 0.0f },

	// secondary order stats
	{ "DSS_REPLEV_LOW",		VAL_INT,	false,	DSS_REPLEV_LOW,		nullptr, nullptr, 0.0f },
	{ "DSS_REPLEV_HIGH",	VAL_INT,	false,	DSS_REPLEV_HIGH,	nullptr, nullptr, 0.0f },
	{ "DSS_REPLEV_NEVER",	VAL_INT,	false,	DSS_REPLEV_NEVER,	nullptr, nullptr, 0.0f },
	{ "DSS_ALEV_ALWAYS",	VAL_INT,	false,	DSS_ALEV_ALWAYS,	nullptr, nullptr, 0.0f },
	{ "DSS_ALEV_ATTACKED",	VAL_INT,	false,	DSS_ALEV_ATTACKED,	nullptr, nullptr, 0.0f },
	{ "DSS_ALEV_NEVER",		VAL_INT,	false,	DSS_ALEV_NEVER,		nullptr, nullptr, 0.0f },
	{ "DSS_RECYCLE_SET",	VAL_INT,	false,	DSS_RECYCLE_SET,	nullptr, nullptr, 0.0f },
	{ "DSS_ASSPROD_START",	VAL_INT,	false,	DSS_ASSPROD_START,	nullptr, nullptr, 0.0f },
	{ "DSS_ASSPROD_END ",	VAL_INT,	false,	DSS_ASSPROD_END ,	nullptr, nullptr, 0.0f },
	{ "DSS_RTL_REPAIR",		VAL_INT,	false,	DSS_RTL_REPAIR,		nullptr, nullptr, 0.0f },
	{ "DSS_RTL_BASE",		VAL_INT,	false,	DSS_RTL_BASE,		nullptr, nullptr, 0.0f },
	{ "DSS_RTL_TRANSPORT",	VAL_INT,	false,	DSS_RTL_TRANSPORT,	nullptr, nullptr, 0.0f },
	{ "DSS_PATROL_SET",		VAL_INT,	false,	DSS_PATROL_SET,		nullptr, nullptr, 0.0f },

	// button id's
	{ "IDRET_OPTIONS",		VAL_INT,	false,	IDRET_OPTIONS,		nullptr, nullptr, 0.0f },
	{ "IDRET_BUILD",		VAL_INT,	false,	IDRET_BUILD,		nullptr, nullptr, 0.0f },
	{ "IDRET_MANUFACTURE",	VAL_INT,	false,	IDRET_MANUFACTURE,	nullptr, nullptr, 0.0f },
	{ "IDRET_RESEARCH",		VAL_INT,	false,	IDRET_RESEARCH,		nullptr, nullptr, 0.0f },
	{ "IDRET_INTELMAP",		VAL_INT,	false,	IDRET_INTEL_MAP,	nullptr, nullptr, 0.0f },
	{ "IDRET_DESIGN",		VAL_INT,	false,	IDRET_DESIGN,		nullptr, nullptr, 0.0f },
	{ "IDRET_CANCEL",		VAL_INT,	false,	IDRET_CANCEL,		nullptr, nullptr, 0.0f },
	{ "IDRET_COMMAND",		VAL_INT,	false,	IDRET_COMMAND,		nullptr, nullptr, 0.0f },

	// Cursor types
	{ "IMAGE_CURSOR_SELECT", VAL_INT,	false,	IMAGE_CURSOR_SELECT,	nullptr, nullptr, 0.0f },
	{ "IMAGE_CURSOR_ATTACK", VAL_INT,	false,	IMAGE_CURSOR_ATTACK,	nullptr, nullptr, 0.0f },
	{ "IMAGE_CURSOR_MOVE",	 VAL_INT,	false,	IMAGE_CURSOR_MOVE,		nullptr, nullptr, 0.0f },

	{ "IMAGE_CURSOR_ECM",	 VAL_INT,	false,	IMAGE_CURSOR_ECM,		nullptr, nullptr, 0.0f },
	{ "IMAGE_CURSOR_REPAIR", VAL_INT,	false,	IMAGE_CURSOR_REPAIR,	nullptr, nullptr, 0.0f },

	{ "IMAGE_CURSOR_PICKUP",  VAL_INT,	false,	IMAGE_CURSOR_PICKUP,	nullptr, nullptr, 0.0f },
	{ "IMAGE_CURSOR_DEFAULT", VAL_INT,	false,	IMAGE_CURSOR_DEFAULT,	nullptr, nullptr, 0.0f },

	{ "IMAGE_CURSOR_BUILD",	  VAL_INT,	false,	IMAGE_CURSOR_BUILD,		nullptr, nullptr, 0.0f },
	{ "IMAGE_CURSOR_GUARD",	  VAL_INT,	false,	IMAGE_CURSOR_GUARD,		nullptr, nullptr, 0.0f },
	{ "IMAGE_CURSOR_SCOUT",   VAL_INT,	false,	IMAGE_CURSOR_SCOUT,		nullptr, nullptr, 0.0f },
	{ "IMAGE_CURSOR_DISEMBARK", VAL_INT,	false,	IMAGE_CURSOR_DISEMBARK,	nullptr, nullptr, 0.0f },
	{ "IMAGE_CURSOR_ATTACH",  VAL_INT,	false,	IMAGE_CURSOR_ATTACH,	nullptr, nullptr, 0.0f },
	{ "IMAGE_CURSOR_LOCKON",  VAL_INT,	false,	IMAGE_CURSOR_LOCKON,	nullptr, nullptr, 0.0f },
	{ "IMAGE_CURSOR_FIX",	  VAL_INT,	false,	IMAGE_CURSOR_FIX,	    nullptr, nullptr, 0.0f },
	{ "IMAGE_CURSOR_EMBARK",  VAL_INT,	false,	IMAGE_CURSOR_EMBARK, 	nullptr, nullptr, 0.0f },

	{ "IMAGE_CURSOR_BRIDGE",  VAL_INT,	false,	IMAGE_CURSOR_BRIDGE,	nullptr, nullptr, 0.0f },
	{ "IMAGE_CURSOR_BOMB",    VAL_INT,	false,	IMAGE_CURSOR_BOMB,		nullptr, nullptr, 0.0f },

	// game mode types  (possible values for intMode)
	{ "INT_NORMAL",			VAL_INT,	false,	INT_NORMAL,		nullptr, nullptr, 0.0f }, // Standard mode (just the reticule)

	{ "INT_OPTION",			VAL_INT,	false,	INT_OPTION,		nullptr, nullptr, 0.0f }, // Option screen

	{ "INT_EDITSTAT",		VAL_INT,	false,	INT_EDITSTAT,	nullptr, nullptr, 0.0f }, // Stat screen up for placing objects

	{ "INT_EDIT",			VAL_INT,	false,	INT_EDIT,		nullptr, nullptr, 0.0f }, // Edit mode

	{ "INT_OBJECT",			VAL_INT,	false,	INT_OBJECT,		nullptr, nullptr, 0.0f }, // Object screen
	{ "INT_STAT",			VAL_INT,	false,	INT_STAT,		nullptr, nullptr, 0.0f }, // Object screen with stat screen
	{ "INT_CMDORDER",		VAL_INT,	false,	INT_CMDORDER,	nullptr, nullptr, 0.0f }, // Object screen with command droids and orders screen
	{ "INT_DESIGN",			VAL_INT,	false,	INT_DESIGN,		nullptr, nullptr, 0.0f }, // Design screen
	{ "INT_INTELMAP",		VAL_INT,	false,	INT_INTELMAP,	nullptr, nullptr, 0.0f }, // Intelligence Map
	{ "INT_ORDER",			VAL_INT,	false,	INT_ORDER,		nullptr, nullptr, 0.0f },
	{ "INT_INGAMEOP",		VAL_INT,	false,	INT_INGAMEOP,	nullptr, nullptr, 0.0f }, // in game options.
	{ "INT_TRANSPORTER",	VAL_INT,	false,	INT_TRANSPORTER, nullptr, nullptr, 0.0f }, //Loading/unloading a Transporter
	{ "INT_MISSIONRES",		VAL_INT,	false,	INT_MISSIONRES,	nullptr, nullptr, 0.0f }, // Results of a mission display.
	{ "INT_MULTIMENU",		VAL_INT,	false,	INT_MULTIMENU,	nullptr, nullptr, 0.0f }, // multiplayer only, player stats etc...

	// parameters for getGameStatus
	{ "STATUS_ReticuleIsOpen",		VAL_INT,	false, STATUS_ReticuleIsOpen,		nullptr, nullptr, 0.0f },		// is the reticule current being displayed (true=yes)
	{ "STATUS_BattleMapViewEnabled", VAL_INT,	false, STATUS_BattleMapViewEnabled,	nullptr, nullptr, 0.0f },	// Are we currently in the battlemap view (tactical display) true=yes
	{ "STATUS_DeliveryReposInProgress", VAL_INT, false, STATUS_DeliveryReposInProgress,	nullptr, nullptr, 0.0f },	// Are we currently in the delivery repos mode

	//possible values for externed 	targetedObjectType
	{ "MT_TERRAIN",		VAL_INT,	false,	MT_TERRAIN,			nullptr, nullptr, 0.0f },
	{ "MT_RESOURCE",	VAL_INT,	false,	MT_RESOURCE,		nullptr, nullptr, 0.0f },
	{ "MT_BLOCKING",	VAL_INT,	false,	MT_BLOCKING,		nullptr, nullptr, 0.0f },
	{ "MT_RIVER",		VAL_INT,	false,	MT_RIVER,			nullptr, nullptr, 0.0f },
	{ "MT_TRENCH",		VAL_INT,	false,	MT_TRENCH,			nullptr, nullptr, 0.0f },
	{ "MT_OWNSTRDAM",	VAL_INT,	false,	MT_OWNSTRDAM,		nullptr, nullptr, 0.0f },
	{ "MT_OWNSTROK",	VAL_INT,	false,	MT_OWNSTROK,		nullptr, nullptr, 0.0f },
	{ "MT_OWNSTRINCOMP", VAL_INT,	false,	MT_OWNSTRINCOMP,	nullptr, nullptr, 0.0f },
	{ "MT_REPAIR",		VAL_INT,	false,	MT_REPAIR,			nullptr, nullptr, 0.0f },
	{ "MT_REPAIRDAM",	VAL_INT,	false,	MT_REPAIRDAM,		nullptr, nullptr, 0.0f },
	{ "MT_ENEMYSTR",	VAL_INT,	false,	MT_ENEMYSTR,		nullptr, nullptr, 0.0f },
	{ "MT_TRANDROID",	VAL_INT,	false,	MT_TRANDROID,		nullptr, nullptr, 0.0f },
	{ "MT_OWNDROID",	VAL_INT,	false,	MT_OWNDROID,		nullptr, nullptr, 0.0f },
	{ "MT_OWNDROIDDAM",	VAL_INT,	false,	MT_OWNDROIDDAM,		nullptr, nullptr, 0.0f },
	{ "MT_ENEMYDROID",	VAL_INT,	false,	MT_ENEMYDROID,		nullptr, nullptr, 0.0f },
	{ "MT_COMMAND",		VAL_INT,	false,	MT_COMMAND,			nullptr, nullptr, 0.0f },
	{ "MT_ARTIFACT",	VAL_INT,	false,	MT_ARTIFACT,		nullptr, nullptr, 0.0f },
	{ "MT_DAMFEATURE",	VAL_INT,	false,	MT_DAMFEATURE,		nullptr, nullptr, 0.0f },
	{ "MT_SENSOR",		VAL_INT,	false,	MT_SENSOR,			nullptr, nullptr, 0.0f },

	// structure target types
	{ "ST_HQ",					VAL_INT,	false,	SCR_ST_HQ,					nullptr, nullptr, 0.0f },
	{ "ST_FACTORY",				VAL_INT,	false,	SCR_ST_FACTORY,				nullptr, nullptr, 0.0f },
	{ "ST_POWER_GEN",			VAL_INT,	false,	SCR_ST_POWER_GEN,			nullptr, nullptr, 0.0f },
	{ "ST_RESOURCE_EXTRACTOR",	VAL_INT,	false,	SCR_ST_RESOURCE_EXTRACTOR,	nullptr, nullptr, 0.0f },
	{ "ST_WALL",				VAL_INT,	false,	SCR_ST_WALL,				nullptr, nullptr, 0.0f },
	{ "ST_RESEARCH",			VAL_INT,	false,	SCR_ST_RESEARCH,			nullptr, nullptr, 0.0f },
	{ "ST_REPAIR_FACILITY",		VAL_INT,	false,	SCR_ST_REPAIR_FACILITY,		nullptr, nullptr, 0.0f },
	{ "ST_COMMAND_CONTROL",		VAL_INT,	false,	SCR_ST_COMMAND_CONTROL,		nullptr, nullptr, 0.0f },
	{ "ST_CYBORG_FACTORY",		VAL_INT,	false,	SCR_ST_CYBORG_FACTORY,		nullptr, nullptr, 0.0f },
	{ "ST_VTOL_FACTORY",		VAL_INT,	false,	SCR_ST_VTOL_FACTORY,		nullptr, nullptr, 0.0f },
	{ "ST_REARM_PAD",			VAL_INT,	false,	SCR_ST_REARM_PAD,			nullptr, nullptr, 0.0f },
	{ "ST_SENSOR",				VAL_INT,	false,	SCR_ST_SENSOR,				nullptr, nullptr, 0.0f },
	{ "ST_DEF_GROUND",			VAL_INT,	false,	SCR_ST_DEF_GROUND,			nullptr, nullptr, 0.0f },
	{ "ST_DEF_AIR",				VAL_INT,	false,	SCR_ST_DEF_AIR,				nullptr, nullptr, 0.0f },
	{ "ST_DEF_IDF",				VAL_INT,	false,	SCR_ST_DEF_IDF,				nullptr, nullptr, 0.0f },
	{ "ST_DEF_ALL",				VAL_INT,	false,	SCR_ST_DEF_ALL,				nullptr, nullptr, 0.0f },

	// droid target types
	{ "DT_COMMAND",		VAL_INT,	false,	SCR_DT_COMMAND,		nullptr, nullptr, 0.0f },
	{ "DT_SENSOR",		VAL_INT,	false,	SCR_DT_SENSOR,		nullptr, nullptr, 0.0f },
	{ "DT_CONSTRUCT",	VAL_INT,	false,	SCR_DT_CONSTRUCT,	nullptr, nullptr, 0.0f },
	{ "DT_REPAIR",		VAL_INT,	false,	SCR_DT_REPAIR,		nullptr, nullptr, 0.0f },
	{ "DT_WEAP_GROUND",	VAL_INT,	false,	SCR_DT_WEAP_GROUND,	nullptr, nullptr, 0.0f },
	{ "DT_WEAP_AIR",	VAL_INT,	false,	SCR_DT_WEAP_AIR,	nullptr, nullptr, 0.0f },
	{ "DT_WEAP_IDF",	VAL_INT,	false,	SCR_DT_WEAP_IDF,	nullptr, nullptr, 0.0f },
	{ "DT_WEAP_ALL",	VAL_INT,	false,	SCR_DT_WEAP_ALL,	nullptr, nullptr, 0.0f },
	{ "DT_LIGHT",		VAL_INT,	false,	SCR_DT_LIGHT,		nullptr, nullptr, 0.0f },
	{ "DT_MEDIUM",		VAL_INT,	false,	SCR_DT_MEDIUM,		nullptr, nullptr, 0.0f },
	{ "DT_HEAVY",		VAL_INT,	false,	SCR_DT_HEAVY,		nullptr, nullptr, 0.0f },
	{ "DT_SUPER_HEAVY",	VAL_INT,	false,	SCR_DT_SUPER_HEAVY,	nullptr, nullptr, 0.0f },
	{ "DT_TRACK",		VAL_INT,	false,	SCR_DT_TRACK,		nullptr, nullptr, 0.0f },
	{ "DT_HTRACK",		VAL_INT,	false,	SCR_DT_HTRACK,		nullptr, nullptr, 0.0f },
	{ "DT_WHEEL",		VAL_INT,	false,	SCR_DT_WHEEL,		nullptr, nullptr, 0.0f },
	{ "DT_LEGS",		VAL_INT,	false,	SCR_DT_LEGS,		nullptr, nullptr, 0.0f },
	{ "DT_GROUND",		VAL_INT,	false,	SCR_DT_GROUND,		nullptr, nullptr, 0.0f },
	{ "DT_VTOL",		VAL_INT,	false,	SCR_DT_VTOL,		nullptr, nullptr, 0.0f },
	{ "DT_HOVER",		VAL_INT,	false,	SCR_DT_HOVER,		nullptr, nullptr, 0.0f },

	// multiplayer

	{ "MAX_PLAYERS",        VAL_INT,        false,  MAX_PLAYERS,                    nullptr, nullptr, 0.0f },

	{ "CAMPAIGN",		VAL_INT,	false,	CAMPAIGN,			nullptr, nullptr, 0.0f },
	{ "SKIRMISH",		VAL_INT,	false,	SKIRMISH,			nullptr, nullptr, 0.0f },

	{ "CAMP_CLEAN",		VAL_INT,	false,	CAMP_CLEAN,			nullptr, nullptr, 0.0f },
	{ "CAMP_BASE",		VAL_INT,	false,	CAMP_BASE,			nullptr, nullptr, 0.0f },
	{ "CAMP_WALLS",		VAL_INT,	false,	CAMP_WALLS,			nullptr, nullptr, 0.0f },

	{ "NO_ALLIANCES",	VAL_INT,	false,	NO_ALLIANCES,		nullptr, nullptr, 0.0f },
	{ "ALLIANCES",		VAL_INT,	false,	ALLIANCES,			nullptr, nullptr, 0.0f },
	{ "ALLIANCES_TEAMS", VAL_INT,	false,	ALLIANCES_TEAMS,	nullptr, nullptr, 0.0f },
	{ "ALLIANCES_UNSHARED", VAL_INT,        false,  ALLIANCES_UNSHARED,             nullptr, nullptr, 0.0f },

	//Group types
	{ "GT_NORMAL",		VAL_INT,	false,	GT_NORMAL,			nullptr, nullptr, 0.0f },
	{ "GT_COMMAND",		VAL_INT,	false,	GT_COMMAND,			nullptr, nullptr, 0.0f },
	{ "GT_TRANSPORTER",	VAL_INT,	false,	GT_TRANSPORTER,		nullptr, nullptr, 0.0f },

	/* some key constants */
	{ "KEY_LCTRL",		VAL_INT,	false,		KEY_LCTRL,		nullptr, nullptr, 0.0f },
	{ "KEY_RCTRL",		VAL_INT,	false,		KEY_RCTRL,		nullptr, nullptr, 0.0f },
	{ "KEY_LSHIFT",		VAL_INT,	false,		KEY_LSHIFT,		nullptr, nullptr, 0.0f },
	{ "KEY_RSHIFT",		VAL_INT,	false,		KEY_RSHIFT,		nullptr, nullptr, 0.0f },
	{ "KEY_LALT",		VAL_INT,	false,		KEY_LALT,		nullptr, nullptr, 0.0f },
	{ "KEY_RALT",		VAL_INT,	false,		KEY_RALT,		nullptr, nullptr, 0.0f },
	{ "KEY_SPACE",		VAL_INT,	false,		KEY_SPACE,		nullptr, nullptr, 0.0f },
	{ "KEY_RETURN",		VAL_INT,	false,		KEY_RETURN,		nullptr, nullptr, 0.0f },
	{ "KEY_ESC",		VAL_INT,	false,		KEY_ESC,		nullptr, nullptr, 0.0f },
	{ "KEY_A",			VAL_INT,	false,		KEY_A,			nullptr, nullptr, 0.0f },
	{ "KEY_B",			VAL_INT,	false,		KEY_B,			nullptr, nullptr, 0.0f },
	{ "KEY_C",			VAL_INT,	false,		KEY_C,			nullptr, nullptr, 0.0f },
	{ "KEY_D",			VAL_INT,	false,		KEY_D,			nullptr, nullptr, 0.0f },
	{ "KEY_E",			VAL_INT,	false,		KEY_E,			nullptr, nullptr, 0.0f },
	{ "KEY_F",			VAL_INT,	false,		KEY_F,			nullptr, nullptr, 0.0f },
	{ "KEY_G",			VAL_INT,	false,		KEY_G,			nullptr, nullptr, 0.0f },
	{ "KEY_H",			VAL_INT,	false,		KEY_H,			nullptr, nullptr, 0.0f },
	{ "KEY_I",			VAL_INT,	false,		KEY_I,			nullptr, nullptr, 0.0f },
	{ "KEY_J",			VAL_INT,	false,		KEY_J,			nullptr, nullptr, 0.0f },
	{ "KEY_K",			VAL_INT,	false,		KEY_K,			nullptr, nullptr, 0.0f },
	{ "KEY_L",			VAL_INT,	false,		KEY_L,			nullptr, nullptr, 0.0f },
	{ "KEY_M",			VAL_INT,	false,		KEY_M,			nullptr, nullptr, 0.0f },
	{ "KEY_N",			VAL_INT,	false,		KEY_N,			nullptr, nullptr, 0.0f },
	{ "KEY_O",			VAL_INT,	false,		KEY_O,			nullptr, nullptr, 0.0f },
	{ "KEY_P",			VAL_INT,	false,		KEY_P,			nullptr, nullptr, 0.0f },
	{ "KEY_Q",			VAL_INT,	false,		KEY_Q,			nullptr, nullptr, 0.0f },
	{ "KEY_R",			VAL_INT,	false,		KEY_R,			nullptr, nullptr, 0.0f },
	{ "KEY_S",			VAL_INT,	false,		KEY_S,			nullptr, nullptr, 0.0f },
	{ "KEY_T",			VAL_INT,	false,		KEY_T,			nullptr, nullptr, 0.0f },
	{ "KEY_U",			VAL_INT,	false,		KEY_U,			nullptr, nullptr, 0.0f },
	{ "KEY_V",			VAL_INT,	false,		KEY_V,			nullptr, nullptr, 0.0f },
	{ "KEY_W",			VAL_INT,	false,		KEY_W,			nullptr, nullptr, 0.0f },
	{ "KEY_X",			VAL_INT,	false,		KEY_X,			nullptr, nullptr, 0.0f },
	{ "KEY_Y",			VAL_INT,	false,		KEY_Y,			nullptr, nullptr, 0.0f },
	{ "KEY_Z",			VAL_INT,	false,		KEY_Z,			nullptr, nullptr, 0.0f },

	{ "KEY_KP_0",		VAL_INT,	false,		KEY_KP_0,		nullptr, nullptr, 0.0f },
	{ "KEY_KP_1",		VAL_INT,	false,		KEY_KP_1,		nullptr, nullptr, 0.0f },
	{ "KEY_KP_2",		VAL_INT,	false,		KEY_KP_2,		nullptr, nullptr, 0.0f },
	{ "KEY_KP_3",		VAL_INT,	false,		KEY_KP_3,		nullptr, nullptr, 0.0f },
	{ "KEY_KP_4",		VAL_INT,	false,		KEY_KP_4,		nullptr, nullptr, 0.0f },
	{ "KEY_KP_5",		VAL_INT,	false,		KEY_KP_5,		nullptr, nullptr, 0.0f },
	{ "KEY_KP_6",		VAL_INT,	false,		KEY_KP_6,		nullptr, nullptr, 0.0f },
	{ "KEY_KP_7",		VAL_INT,	false,		KEY_KP_7,		nullptr, nullptr, 0.0f },
	{ "KEY_KP_8",		VAL_INT,	false,		KEY_KP_8,		nullptr, nullptr, 0.0f },
	{ "KEY_KP_9",		VAL_INT,	false,		KEY_KP_9,		nullptr, nullptr, 0.0f },

	{ "EASY",               VAL_INT,        false,  DIFFICULTY_EASY,                nullptr, nullptr, 0.0f },
	{ "MEDIUM",             VAL_INT,        false,  DIFFICULTY_MEDIUM,              nullptr, nullptr, 0.0f },
	{ "HARD",               VAL_INT,        false,  DIFFICULTY_HARD,                nullptr, nullptr, 0.0f },
	{ "INSANE",             VAL_INT,        false,  DIFFICULTY_INSANE,              nullptr, nullptr, 0.0f },

	/* This entry marks the end of the constant list */
	{ "CONSTANT LIST END", VAL_VOID,	false,		0,	nullptr, nullptr, 0.0f }
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
	{ "CALL_GAMEINIT",	(TRIGGER_TYPE)CALL_GAMEINIT,			nullptr, 0, {VAL_VOID} },
	{ "CALL_DELIVPOINTMOVED",	(TRIGGER_TYPE)CALL_DELIVPOINTMOVED,		nullptr, 0, {VAL_VOID} },
	{ "CALL_DROIDDESIGNED",	(TRIGGER_TYPE)CALL_DROIDDESIGNED,		nullptr, 0, {VAL_VOID} },
	{ "CALL_DROIDBUILT",	(TRIGGER_TYPE)CALL_DROIDBUILT,			nullptr, 0, {VAL_VOID} },
	{ "CALL_POWERGEN_BUILT",	(TRIGGER_TYPE)CALL_POWERGEN_BUILT,		nullptr, 0, {VAL_VOID} },
	{ "CALL_RESEX_BUILT",	(TRIGGER_TYPE)CALL_RESEX_BUILT,			nullptr, 0, {VAL_VOID} },
	{ "CALL_RESEARCH_BUILT",	(TRIGGER_TYPE)CALL_RESEARCH_BUILT,		nullptr, 0, {VAL_VOID} },
	{ "CALL_FACTORY_BUILT",	(TRIGGER_TYPE)CALL_FACTORY_BUILT,		nullptr, 0, {VAL_VOID} },

	{ "CALL_MISSION_START",	(TRIGGER_TYPE)CALL_MISSION_START,		nullptr, 0, {VAL_VOID} },
	{ "CALL_MISSION_END",	(TRIGGER_TYPE)CALL_MISSION_END,			nullptr, 0, {VAL_VOID} },
	{ "CALL_VIDEO_QUIT",	(TRIGGER_TYPE)CALL_VIDEO_QUIT,			nullptr, 0, {VAL_VOID} },
	{ "CALL_LAUNCH_TRANSPORTER", (TRIGGER_TYPE)CALL_LAUNCH_TRANSPORTER,	nullptr, 0, {VAL_VOID} },
	{ "CALL_START_NEXT_LEVEL",	(TRIGGER_TYPE)CALL_START_NEXT_LEVEL,	nullptr, 0, {VAL_VOID} },
	{ "CALL_TRANSPORTER_REINFORCE", (TRIGGER_TYPE)CALL_TRANSPORTER_REINFORCE, nullptr, 0, {VAL_VOID} },
	{ "CALL_MISSION_TIME",	(TRIGGER_TYPE)CALL_MISSION_TIME,		nullptr, 0, {VAL_VOID} },
	{ "CALL_ELECTRONIC_TAKEOVER", (TRIGGER_TYPE)CALL_ELECTRONIC_TAKEOVER, nullptr, 0, {VAL_VOID} },

	// tutorial only callbacks
	{ "CALL_BUILDLIST",	(TRIGGER_TYPE)CALL_BUILDLIST,			nullptr, 0, {VAL_VOID} },
	{ "CALL_BUILDGRID",	(TRIGGER_TYPE)CALL_BUILDGRID,			nullptr, 0, {VAL_VOID} },
	{ "CALL_RESEARCHLIST",	(TRIGGER_TYPE)CALL_RESEARCHLIST,		nullptr, 0, {VAL_VOID} },
	{ "CALL_MANURUN",	(TRIGGER_TYPE)CALL_MANURUN,				nullptr, 0, {VAL_VOID} },
	{ "CALL_MANULIST",	(TRIGGER_TYPE)CALL_MANULIST,			nullptr, 0, {VAL_VOID} },
	{ "CALL_BUTTON_PRESSED",	(TRIGGER_TYPE)CALL_BUTTON_PRESSED,		scrCBButtonPressed,	1,	{ VAL_INT } },
	{ "CALL_DROID_SELECTED",	(TRIGGER_TYPE)CALL_DROID_SELECTED,		scrCBDroidSelected,	1,	{ VAL_REF | (INTERP_TYPE)ST_DROID } },
	{ "CALL_DESIGN_QUIT",	(TRIGGER_TYPE)CALL_DESIGN_QUIT,			nullptr, 0, {VAL_VOID} },
	{ "CALL_DESIGN_WEAPON",	(TRIGGER_TYPE)CALL_DESIGN_WEAPON,		nullptr, 0, {VAL_VOID} },
	{ "CALL_DESIGN_SYSTEM",	(TRIGGER_TYPE)CALL_DESIGN_SYSTEM,		nullptr, 0, {VAL_VOID} },
	{ "CALL_DESIGN_COMMAND",	(TRIGGER_TYPE)CALL_DESIGN_COMMAND,		nullptr, 0, {VAL_VOID} },
	{ "CALL_DESIGN_BODY",	(TRIGGER_TYPE)CALL_DESIGN_BODY,			nullptr, 0, {VAL_VOID} },
	{ "CALL_DESIGN_PROPULSION",	(TRIGGER_TYPE)CALL_DESIGN_PROPULSION,	nullptr, 0, {VAL_VOID} },

	// callback triggers with parameters
	{
		"CALL_RESEARCHCOMPLETED",	(TRIGGER_TYPE)CALL_RESEARCHCOMPLETED,
		scrCBResCompleted,	3,	{ VAL_REF | (INTERP_TYPE)ST_RESEARCH, VAL_REF | (INTERP_TYPE)ST_STRUCTURE, VAL_INT }
	},

	{
		"CALL_NEWDROID",	(TRIGGER_TYPE)CALL_NEWDROID,
		scrCBNewDroid,		3,	{ VAL_INT, VAL_REF | (INTERP_TYPE)ST_DROID, VAL_REF | (INTERP_TYPE)ST_STRUCTURE }
	},

	{
		"CALL_STRUCT_ATTACKED",	(TRIGGER_TYPE)CALL_STRUCT_ATTACKED,
		scrCBStructAttacked, 3,	{ VAL_INT, VAL_REF | (INTERP_TYPE)ST_STRUCTURE, VAL_REF | (INTERP_TYPE)ST_BASEOBJECT }
	},

	{
		"CALL_DROID_ATTACKED",	(TRIGGER_TYPE)CALL_DROID_ATTACKED,
		scrCBDroidAttacked,	3,	{ VAL_INT, VAL_REF | (INTERP_TYPE)ST_DROID, VAL_REF | (INTERP_TYPE)ST_BASEOBJECT }
	},

	{
		"CALL_ATTACKED",	(TRIGGER_TYPE)CALL_ATTACKED,
		scrCBAttacked,		3,	{ VAL_INT, VAL_REF | (INTERP_TYPE)ST_BASEOBJECT, VAL_REF | (INTERP_TYPE)ST_BASEOBJECT }
	},

	{
		"CALL_STRUCT_SEEN",	(TRIGGER_TYPE)CALL_STRUCT_SEEN,
		scrCBStructSeen,	3,	{ VAL_INT, VAL_REF | (INTERP_TYPE)ST_STRUCTURE, VAL_REF | (INTERP_TYPE)ST_BASEOBJECT }
	},

	{
		"CALL_DROID_SEEN",	(TRIGGER_TYPE)CALL_DROID_SEEN,
		scrCBDroidSeen,		3,	{ VAL_INT, VAL_REF | (INTERP_TYPE)ST_DROID, VAL_REF | (INTERP_TYPE)ST_BASEOBJECT }
	},

	{
		"CALL_FEATURE_SEEN",	(TRIGGER_TYPE)CALL_FEATURE_SEEN,
		scrCBFeatureSeen,	3,	{ VAL_INT, VAL_REF | (INTERP_TYPE)ST_FEATURE, VAL_REF | (INTERP_TYPE)ST_BASEOBJECT }
	},

	{
		"CALL_OBJ_SEEN",	(TRIGGER_TYPE)CALL_OBJ_SEEN,
		scrCBObjSeen,		3,	{ VAL_INT, VAL_REF | (INTERP_TYPE)ST_BASEOBJECT, VAL_REF | (INTERP_TYPE)ST_BASEOBJECT }
	},

	{
		"CALL_OBJ_DESTROYED",	(TRIGGER_TYPE)CALL_OBJ_DESTROYED,
		scrCBObjDestroyed,	2,	{ VAL_INT, VAL_REF | (INTERP_TYPE)ST_BASEOBJECT }
	},

	{
		"CALL_STRUCT_DESTROYED",	(TRIGGER_TYPE)CALL_STRUCT_DESTROYED,
		scrCBStructDestroyed, 2,	{ VAL_INT, VAL_REF | (INTERP_TYPE)ST_STRUCTURE }
	},

	{
		"CALL_DROID_DESTROYED",	(TRIGGER_TYPE)CALL_DROID_DESTROYED,
		scrCBDroidDestroyed, 2,	{ VAL_INT, VAL_REF | (INTERP_TYPE)ST_DROID }
	},

	{
		"CALL_FEATURE_DESTROYED",	(TRIGGER_TYPE)CALL_FEATURE_DESTROYED,
		scrCBFeatureDestroyed, 1,	{ VAL_REF | (INTERP_TYPE)ST_FEATURE }
	},

	{ "CALL_OBJECTOPEN",	(TRIGGER_TYPE)CALL_OBJECTOPEN, 		nullptr, 0, {VAL_VOID} },
	{ "CALL_OBJECTCLOSE",	(TRIGGER_TYPE)CALL_OBJECTCLOSE, 	nullptr, 0, {VAL_VOID} },

	{
		"CALL_TRANSPORTER_OFFMAP", (TRIGGER_TYPE)CALL_TRANSPORTER_OFFMAP,
		scrCBTransporterOffMap, 1,	{ VAL_INT }
	},
	{
		"CALL_TRANSPORTER_LANDED", (TRIGGER_TYPE)CALL_TRANSPORTER_LANDED,
		scrCBTransporterLanded, 2,	{ (INTERP_TYPE)ST_GROUP, VAL_INT }
	},

	{ "CALL_ALL_ONSCREEN_DROIDS_SELECTED", (TRIGGER_TYPE)CALL_ALL_ONSCREEN_DROIDS_SELECTED,	nullptr, 0, {VAL_VOID} },

	{ "CALL_NO_REINFORCEMENTS_LEFT", (TRIGGER_TYPE)CALL_NO_REINFORCEMENTS_LEFT,	nullptr, 0, {VAL_VOID} },

	{
		"CALL_CLUSTER_EMPTY",	(TRIGGER_TYPE)CALL_CLUSTER_EMPTY,
		scrCBClusterEmpty,	1,	{ VAL_REF | VAL_INT }
	},

	{
		"CALL_VTOL_OFF_MAP",	(TRIGGER_TYPE)CALL_VTOL_OFF_MAP,
		scrCBVtolOffMap,	2,  { VAL_INT, VAL_REF | (INTERP_TYPE)ST_DROID }
	},

	{
		"CALL_UNITTAKEOVER",	(TRIGGER_TYPE)CALL_UNITTAKEOVER,
		scrCBDroidTaken,	1,	{VAL_REF | (INTERP_TYPE)ST_DROID}
	},

	{
		"CALL_PLAYERLEFT",	(TRIGGER_TYPE)CALL_PLAYERLEFT,
		scrCBPlayerLeft,	1,	{ VAL_REF | VAL_INT }
	},

	{
		"CALL_ALLIANCEOFFER",	(TRIGGER_TYPE)CALL_ALLIANCEOFFER,
		scrCBAllianceOffer,	2,	{ VAL_REF | VAL_INT, VAL_REF | VAL_INT }
	},

	// new callbacks
	//-------------------------------------------------------------------------------

	//console callback
	{
		"CALL_CONSOLE",	(TRIGGER_TYPE)CALL_CONSOLE,
		scrCallConsole,		2,	{ VAL_REF | VAL_INT, VAL_REF | VAL_STRING }
	},

	//59
	{
		"CALL_AI_MSG",	(TRIGGER_TYPE)CALL_AI_MSG,
		scrCallMultiMsg,	3,	{ VAL_INT, VAL_REF | VAL_INT, VAL_REF | VAL_STRING }
	},

	//59
	{
		"CALL_BEACON",	(TRIGGER_TYPE)CALL_BEACON,
		scrCallBeacon,		5,	{ VAL_INT, VAL_REF | VAL_INT, VAL_REF | VAL_INT, VAL_REF | VAL_INT, VAL_REF | VAL_STRING }
	},

	{
		"CALL_STRUCTBUILT",	(TRIGGER_TYPE)CALL_STRUCTBUILT,
		scrCBStructBuilt,	3,	{ VAL_INT, VAL_REF | ST_DROID, VAL_REF | ST_STRUCTURE }
	},

	// new transporter landed callback
	{
		"CALL_TRANSPORTER_LANDED_B", (TRIGGER_TYPE)CALL_TRANSPORTER_LANDED_B,
		scrCBTransporterLandedB, 3,	{ (INTERP_TYPE)ST_GROUP, VAL_INT, VAL_REF | ST_DROID }
	},

	// fired when droid received DORDER_STOP order
	{
		"CALL_DORDER_STOP",	(TRIGGER_TYPE)CALL_DORDER_STOP,
		scrCBDorderStop,	2,	{ VAL_INT, VAL_REF | (INTERP_TYPE)ST_DROID }
	},

	// fired when droid reached the destination and stopped on its own
	{
		"CALL_DROID_REACH_LOCATION", (TRIGGER_TYPE)CALL_DROID_REACH_LOCATION,
		scrCBDorderReachedLocation, 3,	{ VAL_INT, VAL_REF | (INTERP_TYPE)ST_DROID, VAL_REF | VAL_INT }
	},

	{
		"CALL_VTOL_RETARGET",	(TRIGGER_TYPE)CALL_VTOL_RETARGET,
		scrCBVTOLRetarget,	2, 	{ VAL_INT, VAL_REF | (INTERP_TYPE)ST_DROID }
	},

	/* This entry marks the end of the callback list */
	{ "CALLBACK LIST END", (TRIGGER_TYPE)0, nullptr, 0, {VAL_VOID} }
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
	{ (INTERP_TYPE)ST_STRUCTURESTAT, 1,	{ (INTERP_TYPE)ST_POINTER_STRUCTSTAT, } },

	/* This marks the end of the equivalence list */
	{ (INTERP_TYPE)0, 0, {(INTERP_TYPE)0} }
};


// Initialise the script system
bool scrTabInitialise()
{
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

	/* Initialize chat message struct */
	chat_msg.numCommands = 0;
	sstrcpy(chat_msg.lastMessage, "");

	return true;
}

// Shut down the script system
void scrShutDown()
{
	scrvShutDown();
	scriptShutDown();
}
