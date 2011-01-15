/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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

// Lua
#include "lib/lua/warzone.h"

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
typedef struct _CVAR {
	const char *name;
	int id;
	BOOL readOnly;
} CVAR;

CVAR CVarTable[] = {
	{"trackTransporter", EXTID_TRACKTRANSPORTER, true},
	{"mapWidth", EXTID_MAPWIDTH, true},
	{"mapHeight", EXTID_MAPHEIGHT, true},
	{"gameInitialised", EXTID_GAMEINIT, true},
	{"selectedPlayer", EXTID_SELECTEDPLAYER, true},
	{"gameTime", EXTID_GAMETIME, true},
	{"gameLevel", EXTID_GAMELEVEL, false},
	{"inTutorial", EXTID_TUTORIAL, false},
	{"cursorType", EXTID_CURSOR, true},
	{"intMode", EXTID_INTMODE, true},
	{"targetedObjectType", EXTID_TARGETTYPE, true},
	{"extraVictoryFlag", EXTID_EXTRAVICTORYFLAG, false},
	{"extraFailFlag", EXTID_EXTRAFAILFLAG, false},
	{"multiPlayerGameType", EXTID_MULTIGAMETYPE, true},
	{"multiPlayerMaxPlayers", EXTID_MULTIGAMEHUMANMAX, true},
	{"multiPlayerBaseType", EXTID_MULTIGAMEBASETYPE, true},
	{"multiPlayerAlliancesType", EXTID_MULTIGAMEALLIANCESTYPE, true},
	{NULL, 0, false}
};

static void registerCVariable(lua_State *L, const char *name, int id, BOOL readOnly)
{
	lua_getglobal(L, "registerCVariable");  /* function to be called */
	lua_pushstring(L, name);
	lua_pushinteger(L, id);
	lua_pushboolean(L, readOnly);
	luaWZ_pcall_backtrace(L, 3, 0);
}

void scrRegisterCVariables(lua_State *L)
{
	int i = 0;
	luaWZ_dofile(L, "script/cvars.lua");
	while (CVarTable[i].name)
	{
		registerCVariable(L, CVarTable[i].name,
		                     CVarTable[i].id,
		                     CVarTable[i].readOnly);
		i++;
	}
	lua_register(L, "getCVar", scrGetCVar);
	lua_register(L, "setCVar", scrSetCVar);
}


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

	// structure states
	{ "SS_BEING_BUILT",			VAL_INT,	false,	SS_BEING_BUILT,			NULL, NULL, 0.0f },
	{ "SS_BUILT",				VAL_INT,	false,	SS_BUILT,				NULL, NULL, 0.0f },
	{ "SS_BEING_DEMOLISHED",	VAL_INT,	false,	SS_BEING_DEMOLISHED,	NULL, NULL, 0.0f },
	{ "SS_BLUEPRINT_VALID",		VAL_INT,	false,	SS_BLUEPRINT_VALID,		NULL, NULL, 0.0f },
	{ "SS_BLUEPRINT_INVALID",	VAL_INT,	false,	SS_BLUEPRINT_INVALID,	NULL, NULL, 0.0f },
	{ "SS_BLUEPRINT_PLANNED",	VAL_INT,	false,	SS_BLUEPRINT_PLANNED,	NULL, NULL, 0.0f },

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

	{ "CALL_DROID_SELECTED",		VAL_INT,	false,		CALL_DROID_SELECTED,		NULL, NULL, 0.0f },
	{ "CALL_MANURUN",		VAL_INT,	false,		CALL_MANURUN,		NULL, NULL, 0.0f },
	{ "CALL_MANULIST",		VAL_INT,	false,		CALL_MANULIST,		NULL, NULL, 0.0f },
	{ "CALL_BUILDLIST",		VAL_INT,	false,		CALL_BUILDLIST,		NULL, NULL, 0.0f },
	{ "CALL_BUILDGRID",		VAL_INT,	false,		CALL_BUILDGRID,		NULL, NULL, 0.0f },
	{ "CALL_RESEARCHLIST",		VAL_INT,	false,		CALL_RESEARCHLIST,		NULL, NULL, 0.0f },
	{ "CALL_BUTTON_PRESSED",		VAL_INT,	false,		CALL_BUTTON_PRESSED,		NULL, NULL, 0.0f },
	{ "CALL_DESIGN_WEAPON",		VAL_INT,	false,		CALL_DESIGN_WEAPON,		NULL, NULL, 0.0f },
	{ "CALL_DESIGN_BODY",		VAL_INT,	false,		CALL_DESIGN_BODY,		NULL, NULL, 0.0f },
	{ "CALL_DESIGN_PROPULSION",		VAL_INT,	false,		CALL_DESIGN_PROPULSION,		NULL, NULL, 0.0f },
	{ "CALL_DELIVPOINTMOVED",		VAL_INT,	false,		CALL_DELIVPOINTMOVED,		NULL, NULL, 0.0f },
	{ "CALL_DESIGN_QUIT",		VAL_INT,	false,		CALL_DESIGN_QUIT,		NULL, NULL, 0.0f },
	{ "CALL_OBJ_SEEN",		VAL_INT,	false,		CALL_OBJ_SEEN,		NULL, NULL, 0.0f },
	{ "CALL_GAMEINIT",		VAL_INT,	false,		CALL_GAMEINIT,		NULL, NULL, 0.0f },
	{ "CALL_DROIDDESIGNED",		VAL_INT,	false,		CALL_DROIDDESIGNED,		NULL, NULL, 0.0f },
	{ "CALL_RESEARCHCOMPLETED",		VAL_INT,	false,		CALL_RESEARCHCOMPLETED,		NULL, NULL, 0.0f },
	{ "CALL_NEWDROID",		VAL_INT,	false,		CALL_NEWDROID,		NULL, NULL, 0.0f },
	{ "CALL_DROIDBUILT",		VAL_INT,	false,		CALL_DROIDBUILT,		NULL, NULL, 0.0f },
	{ "CALL_POWERGEN_BUILT",		VAL_INT,	false,		CALL_POWERGEN_BUILT,		NULL, NULL, 0.0f },
	{ "CALL_RESEX_BUILT",		VAL_INT,	false,		CALL_RESEX_BUILT,		NULL, NULL, 0.0f },
	{ "CALL_RESEARCH_BUILT",		VAL_INT,	false,		CALL_RESEARCH_BUILT,		NULL, NULL, 0.0f },
	{ "CALL_FACTORY_BUILT",		VAL_INT,	false,		CALL_FACTORY_BUILT,		NULL, NULL, 0.0f },
	{ "CALL_MISSION_START",		VAL_INT,	false,		CALL_MISSION_START,		NULL, NULL, 0.0f },
	{ "CALL_MISSION_END",		VAL_INT,	false,		CALL_MISSION_END,		NULL, NULL, 0.0f },
	{ "CALL_VIDEO_QUIT",		VAL_INT,	false,		CALL_VIDEO_QUIT,		NULL, NULL, 0.0f },
	{ "CALL_TRANSPORTER_OFFMAP",		VAL_INT,	false,		CALL_TRANSPORTER_OFFMAP,		NULL, NULL, 0.0f },
	{ "CALL_TRANSPORTER_LANDED",		VAL_INT,	false,		CALL_TRANSPORTER_LANDED,		NULL, NULL, 0.0f },
	{ "CALL_TRANSPORTER_LANDED_B",		VAL_INT,	false,		CALL_TRANSPORTER_LANDED_B,		NULL, NULL, 0.0f },
	{ "CALL_LAUNCH_TRANSPORTER",		VAL_INT,	false,		CALL_LAUNCH_TRANSPORTER,		NULL, NULL, 0.0f },
	{ "CALL_START_NEXT_LEVEL",		VAL_INT,	false,		CALL_START_NEXT_LEVEL,		NULL, NULL, 0.0f },
	{ "CALL_TRANSPORTER_REINFORCE",		VAL_INT,	false,		CALL_TRANSPORTER_REINFORCE,		NULL, NULL, 0.0f },
	{ "CALL_MISSION_TIME",		VAL_INT,	false,		CALL_MISSION_TIME,		NULL, NULL, 0.0f },
	{ "CALL_PLAYERLEFT",		VAL_INT,	false,		CALL_PLAYERLEFT,		NULL, NULL, 0.0f },
	{ "CALL_STRUCT_ATTACKED",		VAL_INT,	false,		CALL_STRUCT_ATTACKED,		NULL, NULL, 0.0f },
	{ "CALL_ATTACKED",		VAL_INT,	false,		CALL_ATTACKED,		NULL, NULL, 0.0f },
	{ "CALL_STRUCT_DESTROYED",		VAL_INT,	false,		CALL_STRUCT_DESTROYED,		NULL, NULL, 0.0f },
	{ "CALL_STRUCTBUILT",		VAL_INT,	false,		CALL_STRUCTBUILT,		NULL, NULL, 0.0f },
	{ "CALL_DROID_ATTACKED",		VAL_INT,	false,		CALL_DROID_ATTACKED,		NULL, NULL, 0.0f },
	{ "CALL_DROID_DESTROYED",		VAL_INT,	false,		CALL_DROID_DESTROYED,		NULL, NULL, 0.0f },
	{ "CALL_DROID_SEEN",		VAL_INT,	false,		CALL_DROID_SEEN,		NULL, NULL, 0.0f },
	{ "CALL_OBJ_SEEN",		VAL_INT,	false,		CALL_OBJ_SEEN,		NULL, NULL, 0.0f },
	{ "CALL_ALLIANCEOFFER",		VAL_INT,	false,		CALL_ALLIANCEOFFER,		NULL, NULL, 0.0f },
	{ "CALL_AI_MSG",		VAL_INT,	false,		CALL_AI_MSG,		NULL, NULL, 0.0f },
	{ "CALL_BEACON",		VAL_INT,	false,		CALL_BEACON,		NULL, NULL, 0.0f },
	{ "CALL_CONSOLE",		VAL_INT,	false,		CALL_CONSOLE,		NULL, NULL, 0.0f },
	{ "CALL_ELECTRONIC_TAKEOVER",		VAL_INT,	false,		CALL_ELECTRONIC_TAKEOVER,		NULL, NULL, 0.0f },
	{ "CALL_UNITTAKEOVER",		VAL_INT,	false,		CALL_UNITTAKEOVER,		NULL, NULL, 0.0f },
	{ "CALL_CLUSTER_EMPTY",		VAL_INT,	false,		CALL_CLUSTER_EMPTY,		NULL, NULL, 0.0f },
	{ "CALL_NO_REINFORCEMENTS_LEFT",		VAL_INT,	false,		CALL_NO_REINFORCEMENTS_LEFT,		NULL, NULL, 0.0f },
	{ "CALL_VTOL_OFF_MAP",		VAL_INT,	false,		CALL_VTOL_OFF_MAP,		NULL, NULL, 0.0f },
	{ "CALL_KEY_PRESSED",		VAL_INT,	false,		CALL_KEY_PRESSED,		NULL, NULL, 0.0f },
	{ "CALL_DROID_REACH_LOCATION",		VAL_INT,	false,		CALL_DROID_REACH_LOCATION,		NULL, NULL, 0.0f },
	{ "CALL_VTOL_RETARGET",		VAL_INT,	false,		CALL_VTOL_RETARGET,		NULL, NULL, 0.0f },

	{ "CALL_EVERY_FRAME",		VAL_INT,	false,		CALL_EVERY_FRAME,		NULL, NULL, 0.0f },

	/* This entry marks the end of the constant list */
	{ "CONSTANT LIST END",VAL_VOID,	false,		0,	NULL, NULL, 0.0f }
};

/// register all constants as globals in the Lua state
void scrRegisterConstants(lua_State *L)
{
	CONST_SYMBOL *pos;
	pos = asConstantTable;
	while (pos->type != VAL_VOID)
	{
		if (pos->type == VAL_INT)
		{
			lua_pushinteger(L, pos->ival);
		}
		else
		{
			lua_pushnil(L);
		}
		lua_setglobal(L, pos->pIdent);
		pos++;
	}
}

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
BOOL scrTabInitialise(void)
{
	unsigned int i;

	if (!scriptInitialise())
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

	// Set the object variable table
	scriptSetObjectTab(asObjTable);

	// Set the type equivalence table
	scriptSetTypeEquiv(asEquivTable);

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
