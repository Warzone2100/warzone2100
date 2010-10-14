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
 * ScriptAI.c
 *
 * Script functions to support the AI system
 *
 */
 
#include "lib/framework/frame.h"
#include "objects.h"
#include "group.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "scriptai.h"
#include "order.h"
#include "map.h"
#include "cluster.h"
#include "lib/netplay/netplay.h"
#include "cmddroid.h"
#include "projectile.h"
#include "research.h"
#include "gateway.h"
#include "multiplay.h"
#include "action.h"		//because of .action
#include "power.h"
#include "geometry.h"
#include "src/scriptfuncs.h"
#include "fpath.h"
#include "multigifts.h"

// Lua
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "lib/lua/warzone.h"

#include "scriptobj.h"

static INTERP_VAL	scrFunctionResult;	//function return value to be pushed to stack

static int scrNewGroup(lua_State *L)
{
	DROID_GROUP *group;
	if (!grpCreate(&group))
	{
		return luaL_error(L, "could not create new group");
	}
	// increase the reference count
	group->refCount += 1;
	luaWZObj_pushgroup(L, group);
	return 1;
}

/// Add a droid to a group
static int scrGroupAddDroid(lua_State *L)
{
	DROID_GROUP *psGroup = luaWZObj_checkgroup(L, 1);
	DROID *psDroid = (DROID*)luaWZObj_checkobject(L, 2, OBJ_DROID);

	if (psDroid->droidType == DROID_COMMAND)
	{
		return luaL_error(L, "scrGroupAdd: cannot add a command droid to a group" );
	}
	if (psDroid->droidType == DROID_TRANSPORTER)
	{
		return luaL_error(L, "scrGroupAdd: cannot add a transporter to a group" );
	}

	grpJoin(psGroup, psDroid);

	return 0;
}


/// Add droids in an area to a group
static int scrGroupAddArea(lua_State *L)
{
	DROID *psDroid;
	DROID_GROUP *psGroup = luaWZObj_checkgroup(L, 1);
	int player = luaL_checkint(L, 2);
	int x1 = luaL_checkint(L, 3);
	int y1 = luaL_checkint(L, 4);
	int x2 = luaL_checkint(L, 5);
	int y2 = luaL_checkint(L, 6);
	
// 	debug( LOG_SCRIPT, "groupAddArea: player %d (%d,%d) -> (%d,%d)\n", player, x1, y1, x2, y2 );

	for(psDroid=apsDroidLists[player]; psDroid; psDroid=psDroid->psNext)
	{
		if (((SDWORD)psDroid->pos.x >= x1) && ((SDWORD)psDroid->pos.x <= x2) &&
			((SDWORD)psDroid->pos.y >= y1) && ((SDWORD)psDroid->pos.y <= y2) &&
			psDroid->droidType != DROID_COMMAND &&
			psDroid->droidType != DROID_TRANSPORTER )

		{
			grpJoin(psGroup, psDroid);
		}
	}

	return 0;
}


// Add groupless droids in an area to a group
WZ_DECL_UNUSED static BOOL scrGroupAddAreaNoGroup(void)
{
	DROID_GROUP		*psGroup;
	DROID			*psDroid;
	SDWORD			x1,y1,x2,y2, player;

	if (!stackPopParams(6, ST_GROUP, &psGroup, VAL_INT, &player,
							VAL_INT,&x1,VAL_INT,&y1, VAL_INT,&x2,VAL_INT,&y2))
	{
		return false;
	}

	ASSERT( psGroup != NULL,
		"scrGroupAddNoGroup: Invalid group pointer" );

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( false, "scrGroupAddAreaNoGroup: invalid player" );
		return false;
	}

	for(psDroid=apsDroidLists[player]; psDroid; psDroid=psDroid->psNext)
	{
		if (((SDWORD)psDroid->pos.x >= x1) && ((SDWORD)psDroid->pos.x <= x2) &&
			((SDWORD)psDroid->pos.y >= y1) && ((SDWORD)psDroid->pos.y <= y2) &&
			psDroid->droidType != DROID_COMMAND &&
			psDroid->droidType != DROID_TRANSPORTER &&
			psDroid->psGroup   == NULL)
		{
			grpJoin(psGroup, psDroid);
		}
	}

	return true;
}


/// Move the droids from one group to another
static int scrGroupAddGroup(lua_State *L)
{
	DROID_GROUP *psTo = luaWZObj_checkgroup(L, 1);
	DROID_GROUP *psFrom = luaWZObj_checkgroup(L, 2);
	DROID *psDroid, *psNext;

	for(psDroid=psFrom->psList; psDroid; psDroid=psNext)
	{
		psNext = psDroid->psGrpNext;
		grpJoin(psTo, psDroid);
	}

	return 0;
}


// check if a droid is a member of a group
static int scrGroupMember(lua_State *L)
{
	DROID_GROUP *psGroup = luaWZObj_checkgroup(L, 1);
	DROID *psDroid = (DROID*)luaWZObj_checkobject(L, 2, OBJ_DROID);

	lua_pushboolean(L, (psDroid->psGroup == psGroup));
	return 1;
}


/// returns number of idle droids in a group.
static int scrIdleGroup(lua_State *L)
{
	DROID_GROUP *psGroup = luaWZObj_checkgroup(L, 1);
	DROID		*psDroid;
	UDWORD		count=0;

	for(psDroid = psGroup->psList;psDroid; psDroid = psDroid->psGrpNext)
	{
		if (psDroid->order == DORDER_NONE || (psDroid->order == DORDER_GUARD && psDroid->psTarget == NULL))
		{
			count++;
		}
	}
	lua_pushinteger(L, count);
	return 1;
}

// variables for the group iterator
static DROID_GROUP		*psScrIterateGroup;
static DROID			*psScrIterateGroupDroid;

/// initialise iterating a groups members
static int scrInitIterateGroup(lua_State *L)
{
	DROID_GROUP *psGroup = luaWZObj_checkgroup(L, 1);

	psScrIterateGroup = psGroup;
	psScrIterateGroupDroid = psGroup->psList;
	
	//debug(LOG_WARNING, "iterating over group with %i members", grpNumMembers(psGroup));

	return 0;
}

/// iterate through a groups members
static int scrIterateGroup(lua_State *L)
{
	DROID_GROUP *psGroup = luaWZObj_checkgroup(L, 1);
	DROID *psDroid;

	if (psGroup != psScrIterateGroup)
	{
		debug(LOG_ERROR, "scrIterateGroup: invalid group, InitGroupIterate not called?" );
		return false;
	}

	if (psScrIterateGroupDroid != NULL)
	{
		psDroid = psScrIterateGroupDroid;
		psScrIterateGroupDroid = psScrIterateGroupDroid->psGrpNext;
		luaWZObj_pushdroid(L, psDroid);
	}
	else
	{
		lua_pushnil(L);
	}
	
	return 1;
}


// initialise iterating a cluster
WZ_DECL_UNUSED static BOOL scrInitIterateCluster(void)
{
	SDWORD	clusterID;

	if (!stackPopParams(1, VAL_INT, &clusterID))
	{
		return false;
	}

	clustInitIterate(clusterID);

	return true;
}


// iterate a cluster
WZ_DECL_UNUSED static BOOL scrIterateCluster(void)
{
	BASE_OBJECT		*psObj;

	psObj = clustIterate();

	scrFunctionResult.v.oval = psObj;
	if (!stackPushResult((INTERP_TYPE)ST_BASEOBJECT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


/// remove a droid from a group
static int scrDroidLeaveGroup(lua_State *L)
{
	DROID *psDroid = (DROID*)luaWZObj_checkobject(L, 1, OBJ_DROID);

	if (psDroid->psGroup != NULL)
	{
		grpLeave(psDroid->psGroup, psDroid);
	}

	return 0;
}


// Give a group an order
WZ_DECL_UNUSED static BOOL scrOrderGroup(void)
{
	DROID_GROUP		*psGroup;
	DROID_ORDER		order;

	if (!stackPopParams(2, ST_GROUP, &psGroup, VAL_INT, &order))
	{
		return false;
	}

	ASSERT( psGroup != NULL,
		"scrOrderGroup: Invalid group pointer" );

	if (order != DORDER_STOP &&
		order != DORDER_RETREAT &&
		order != DORDER_DESTRUCT &&
		order != DORDER_RTR &&
		order != DORDER_RTB &&
		order != DORDER_RUN)
	{
		ASSERT( false,
			"scrOrderGroup: Invalid order" );
		return false;
	}

	debug(LOG_NEVER, "group %p (%u) order %d", psGroup, grpNumMembers(psGroup), order);
	orderGroup(psGroup, order);

	return true;
}

/// Give a group an order to a location
static int scrOrderGroupLoc(lua_State *L)
{
	DROID_GROUP *psGroup = luaWZObj_checkgroup(L, 1);
	DROID_ORDER order = (DROID_ORDER)luaL_checkinteger(L, 2);
	int x = luaL_checkint(L, 3);
	int y = luaL_checkint(L, 4);

	if (order != DORDER_MOVE &&
		order != DORDER_SCOUT)
	{
		return luaL_error(L, "scrOrderGroupLoc: Invalid order" );
	}
	if (x < 0
	 || x > world_coord(mapWidth)
	 || y < 0
	 || y > world_coord(mapHeight))
	{
		return luaL_error(L, "Invalid map location (%d, %d), max is (%d, %d)", x, y, world_coord(mapWidth), world_coord(mapHeight));
	}

	debug(LOG_NEVER, "group %p (%u) order %d (%d,%d)",
		psGroup, grpNumMembers(psGroup), order, x,y);
	orderGroupLoc(psGroup, order, (UDWORD)x,(UDWORD)y);

	return 0;
}

// Give a group an order to an object
static int scrOrderGroupObj(lua_State *L)
{
	DROID_GROUP *psGroup = luaWZObj_checkgroup(L, 1);
	DROID_ORDER order = (DROID_ORDER)luaL_checkinteger(L, 2);
	BASE_OBJECT *psObj = luaWZObj_checkbaseobject(L, 3);

	if (order != DORDER_ATTACK &&
		order != DORDER_HELPBUILD &&
		order != DORDER_DEMOLISH &&
		order != DORDER_REPAIR &&
		order != DORDER_OBSERVE &&
		order != DORDER_EMBARK &&
		order != DORDER_FIRESUPPORT &&
		order != DORDER_DROIDREPAIR)
	{
		luaL_error(L, "Invalid order" );
	}

	debug(LOG_NEVER, "group %p (%u) order %d,  obj type %d player %d id %d",
		psGroup, grpNumMembers(psGroup), order, psObj->type, psObj->player, psObj->id);

	orderGroupObj(psGroup, order, psObj);

	return 0;
}

/// Give a droid an order
static int scrOrderDroid(lua_State *L)
{
	DROID *psDroid = (DROID*)luaWZObj_checkobject(L, 1, OBJ_DROID);
	DROID_ORDER order = luaL_checkint(L, 2);

	if (order != DORDER_STOP &&
		order != DORDER_RETREAT &&
		order != DORDER_DESTRUCT &&
		order != DORDER_RTR &&
		order != DORDER_RTB &&
		order != DORDER_RUN &&
		order != DORDER_NONE )
	{
		return luaL_error(L, "Invalid order %d", order);
	}

	orderDroid(psDroid, order);

	return 0;
}


/// Give a Droid an order to a location
static int scrOrderDroidLoc(lua_State *L)
{
	DROID *psDroid = (DROID*)luaWZObj_checkobject(L, 1, OBJ_DROID);
	int order = luaL_checkinteger(L, 2);
	int x = luaL_checkinteger(L, 3);
	int y = luaL_checkinteger(L, 4);

	if (order != DORDER_MOVE &&
		order != DORDER_SCOUT)
	{
		return luaL_error(L, "order (arg 2) should be MOVE or SCOUT (you passed %i)", order);
	}
	if (x < 0
	 || x > world_coord(mapWidth)
	 || y < 0
	 || y > world_coord(mapHeight))
	{
		return luaL_error(L, "invalid location (%i,%i)", x, y);
	}

	orderDroidLoc(psDroid, order, (UDWORD)x,(UDWORD)y);

	return 0;
}


/// Give a Droid an order to an object
static int scrOrderDroidObj(lua_State *L)
{
	DROID *psDroid = (DROID*)luaWZObj_checkobject(L, 1, OBJ_DROID);
	DROID_ORDER order = luaL_checkint(L, 2);
	BASE_OBJECT *psObj = luaWZObj_checkbaseobject(L, 3);

	if (order != DORDER_ATTACK &&
		order != DORDER_HELPBUILD &&
		order != DORDER_DEMOLISH &&
		order != DORDER_REPAIR &&
		order != DORDER_OBSERVE &&
		order != DORDER_EMBARK &&
		order != DORDER_FIRESUPPORT &&
		order != DORDER_DROIDREPAIR)
	{
		return luaL_error(L, "Invalid order");
	}

	orderDroidObj(psDroid, order, psObj);
	return 0;
}

/// Give a Droid an order with a stat
static int scrOrderDroidStatsLoc(lua_State *L)
{
	BASE_STATS *psStats;
	
	DROID *psDroid = (DROID*)luaWZObj_checkobject(L, 1, OBJ_DROID);
	DROID_ORDER order = (DROID_ORDER)luaL_checkint(L, 2);
	int statIndex = luaWZObj_checkstructurestat(L, 3);
	int x = luaL_checkint(L, 4);
	int y = luaL_checkint(L, 5);

	psStats = (BASE_STATS *)(asStructureStats + statIndex);

	ASSERT( psStats != NULL,
		"scrOrderUnitStatsLoc: Invalid object pointer" );

	if ((x < 0) || (x > (SDWORD)mapWidth*TILE_UNITS) ||
		(y < 0) || (y > (SDWORD)mapHeight*TILE_UNITS))
	{
		return luaL_error(L, "scrOrderUnitStatsLoc: Invalid location: %i,%i", x, y);
	}

	if (order != DORDER_BUILD)
	{
		return luaL_error(L, "scrOrderUnitStatsLoc: Invalid order" );
	}

	// Don't allow scripts to order structure builds if players structure
	// limit has been reached.
	if (!IsPlayerStructureLimitReached(psDroid->player))
	{
		// HACK: FIXME: Looks like a script error in the player*.slo files
		// buildOnExactLocation() which references previously destroyed buildings from
		// _stat = rebuildStructStat[_count]  causes this.
		if (strcmp(psStats->pName, "A0ADemolishStructure") == 0)
		{
			// I don't feel like spamming a ASSERT here, we *know* it is a issue. 
			return true;
		}

		orderDroidStatsLocDir(psDroid, order, psStats, (UDWORD)x, (UDWORD)y, 0);
	}

	return 0;
}


/// set the secondary state for a droid
static int scrSetDroidSecondary(lua_State *L)
{
	DROID *psDroid = (DROID*)luaWZObj_checkobject(L, 1, OBJ_DROID);
	SECONDARY_ORDER sec = (SECONDARY_ORDER)luaL_checkint(L, 2);
	SECONDARY_STATE state = (SECONDARY_STATE)luaL_checkint(L, 3);

	secondarySetState(psDroid, sec, state);
	return 0;
}

// set the secondary state for a droid
static int scrSetGroupSecondary(lua_State *L)
{
	DROID_GROUP *psGroup = luaWZObj_checkgroup(L, 1);
	SECONDARY_ORDER sec = luaL_checkint(L, 2);
	SECONDARY_STATE state = luaL_checkint(L, 3);

	if (psGroup)
	{
		grpSetSecondary(psGroup, sec, state);
	}
	return 0;
}


// add a droid to a commander
WZ_DECL_UNUSED static BOOL scrCmdDroidAddDroid(void)
{
	DROID		*psDroid, *psCommander;

	if (!stackPopParams(2, ST_DROID, &psCommander, ST_DROID, &psDroid))
	{
		return false;
	}

	cmdDroidAddDroid(psCommander, psDroid);

	return true;
}

// returns max number of droids in a commander group
WZ_DECL_UNUSED static BOOL scrCmdDroidMaxGroup(void)
{
	DROID		*psCommander;

	if (!stackPopParams(1, ST_DROID, &psCommander))
	{
		return false;
	}

	ASSERT(psCommander != NULL,
		"scrCmdDroidMaxGroup: NULL pointer passed");

	scrFunctionResult.v.ival = cmdDroidMaxGroup(psCommander);
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// store the prefered and ignore targets for the target functions
UDWORD	scrStructPref, scrStructIgnore;
UDWORD	scrDroidPref, scrDroidIgnore;


// reset the structure preferences
WZ_DECL_UNUSED static BOOL scrResetStructTargets(void)
{
	scrStructPref = 0;
	scrStructIgnore = 0;

	return true;
}


// reset the droid preferences
WZ_DECL_UNUSED static BOOL scrResetDroidTargets(void)
{
	scrDroidPref = 0;
	scrDroidIgnore = 0;

	return true;
}


// set prefered structure target types
WZ_DECL_UNUSED static BOOL scrSetStructTarPref(void)
{
	UDWORD	pref;

	if (!stackPopParams(1, VAL_INT, &pref))
	{
		return false;
	}

	ASSERT( (SCR_ST_HQ					== pref) ||
			 (SCR_ST_FACTORY			== pref) ||
			 (SCR_ST_POWER_GEN			== pref) ||
			 (SCR_ST_RESOURCE_EXTRACTOR	== pref) ||
			 (SCR_ST_WALL				== pref) ||
			 (SCR_ST_RESEARCH			== pref) ||
			 (SCR_ST_REPAIR_FACILITY	== pref) ||
			 (SCR_ST_COMMAND_CONTROL	== pref) ||
			 (SCR_ST_CYBORG_FACTORY		== pref) ||
			 (SCR_ST_VTOL_FACTORY		== pref) ||
			 (SCR_ST_REARM_PAD			== pref) ||
			 (SCR_ST_SENSOR				== pref) ||
			 (SCR_ST_DEF_GROUND			== pref) ||
			 (SCR_ST_DEF_AIR			== pref) ||
			 (SCR_ST_DEF_IDF			== pref) ||
			 (SCR_ST_DEF_ALL			== pref) ,
		"scrSetStructTarPref: unknown target preference" );

	scrStructPref |= pref;

	return true;
}


// set structure target ignore types
WZ_DECL_UNUSED static BOOL scrSetStructTarIgnore(void)
{
	UDWORD	pref;

	if (!stackPopParams(1, VAL_INT, &pref))
	{
		return false;
	}

	ASSERT( (SCR_ST_HQ					== pref) ||
			 (SCR_ST_FACTORY			== pref) ||
			 (SCR_ST_POWER_GEN			== pref) ||
			 (SCR_ST_RESOURCE_EXTRACTOR	== pref) ||
			 (SCR_ST_WALL				== pref) ||
			 (SCR_ST_RESEARCH			== pref) ||
			 (SCR_ST_REPAIR_FACILITY	== pref) ||
			 (SCR_ST_COMMAND_CONTROL	== pref) ||
			 (SCR_ST_CYBORG_FACTORY		== pref) ||
			 (SCR_ST_VTOL_FACTORY		== pref) ||
			 (SCR_ST_REARM_PAD			== pref) ||
			 (SCR_ST_SENSOR				== pref) ||
			 (SCR_ST_DEF_GROUND			== pref) ||
			 (SCR_ST_DEF_AIR			== pref) ||
			 (SCR_ST_DEF_IDF			== pref) ||
			 (SCR_ST_DEF_ALL			== pref) ,
		"scrSetStructTarIgnore: unknown ignore target" );

	scrStructIgnore |= pref;

	return true;
}


// set prefered droid target types
WZ_DECL_UNUSED static BOOL scrSetDroidTarPref(void)
{
	UDWORD	pref;

	if (!stackPopParams(1, VAL_INT, &pref))
	{
		return false;
	}

	ASSERT( (SCR_DT_COMMAND		== pref) ||
			 (SCR_DT_SENSOR			== pref) ||
			 (SCR_DT_CONSTRUCT		== pref) ||
			 (SCR_DT_REPAIR			== pref) ||
			 (SCR_DT_WEAP_GROUND	== pref) ||
			 (SCR_DT_WEAP_AIR		== pref) ||
			 (SCR_DT_WEAP_IDF		== pref) ||
			 (SCR_DT_WEAP_ALL		== pref) ||
			 (SCR_DT_LIGHT			== pref) ||
			 (SCR_DT_MEDIUM			== pref) ||
			 (SCR_DT_HEAVY			== pref) ||
			 (SCR_DT_SUPER_HEAVY	== pref) ||
			 (SCR_DT_TRACK			== pref) ||
			 (SCR_DT_HTRACK			== pref) ||
			 (SCR_DT_WHEEL			== pref) ||
			 (SCR_DT_LEGS			== pref) ||
			 (SCR_DT_GROUND			== pref) ||
			 (SCR_DT_VTOL			== pref) ||
			 (SCR_DT_HOVER			== pref) ,
		"scrSetUnitTarPref: unknown target preference" );


	scrDroidPref |= pref;

	return true;
}

// set droid target ignore types
WZ_DECL_UNUSED static BOOL scrSetDroidTarIgnore(void)
{
	UDWORD	pref;

	if (!stackPopParams(1, VAL_INT, &pref))
	{
		return false;
	}

	ASSERT( (SCR_DT_COMMAND		== pref) ||
			 (SCR_DT_SENSOR			== pref) ||
			 (SCR_DT_CONSTRUCT		== pref) ||
			 (SCR_DT_REPAIR			== pref) ||
			 (SCR_DT_WEAP_GROUND	== pref) ||
			 (SCR_DT_WEAP_AIR		== pref) ||
			 (SCR_DT_WEAP_IDF		== pref) ||
			 (SCR_DT_WEAP_ALL		== pref) ||
			 (SCR_DT_LIGHT			== pref) ||
			 (SCR_DT_MEDIUM			== pref) ||
			 (SCR_DT_HEAVY			== pref) ||
			 (SCR_DT_SUPER_HEAVY	== pref) ||
			 (SCR_DT_TRACK			== pref) ||
			 (SCR_DT_HTRACK			== pref) ||
			 (SCR_DT_WHEEL			== pref) ||
			 (SCR_DT_LEGS			== pref) ||
			 (SCR_DT_GROUND			== pref) ||
			 (SCR_DT_VTOL			== pref) ||
			 (SCR_DT_HOVER			== pref) ,
		"scrSetUnitTarIgnore: unknown ignore target" );

	scrDroidIgnore |= pref;

	return true;
}


// get the correct type mask for a structure target
static UDWORD scrStructTargetMask(STRUCTURE *psStruct)
{
	UDWORD				mask = 0;
	STRUCTURE_STATS		*psStats;
	WEAPON_STATS		*psWStats;

	psStats = psStruct->pStructureType;
	switch (psStats->type)
	{
	case REF_HQ:
		mask = SCR_ST_HQ;
		break;
	case REF_FACTORY:
	case REF_FACTORY_MODULE:
		mask = SCR_ST_FACTORY;
		break;
	case REF_POWER_GEN:
	case REF_POWER_MODULE:
		mask = SCR_ST_POWER_GEN;
		break;
	case REF_RESOURCE_EXTRACTOR:
		mask = SCR_ST_RESOURCE_EXTRACTOR;
		break;
	case REF_DEFENSE:
		//if (psStats->numWeaps == 0 && psStats->pSensor != NULL)
		if (psStats->psWeapStat[0] == NULL && psStats->pSensor != NULL)
		{
			mask = SCR_ST_SENSOR;
		}
		//else if (psStats->numWeaps > 0)
		else if (psStats->psWeapStat[0] != NULL)
		{
			psWStats = psStats->psWeapStat[0];
			if (!proj_Direct(psWStats))
			{
				mask = SCR_ST_DEF_IDF;
			}
			else if (psWStats->surfaceToAir & SHOOT_IN_AIR)
			{
				mask = SCR_ST_DEF_AIR;
			}
			else
			{
				mask = SCR_ST_DEF_GROUND;
			}
		}
		break;
	case REF_WALL:
	case REF_WALLCORNER:
		mask = 0;	// ignore walls for now
		break;
	case REF_RESEARCH:
	case REF_RESEARCH_MODULE:
		mask = SCR_ST_RESEARCH;
		break;
	case REF_REPAIR_FACILITY:
		mask = SCR_ST_REPAIR_FACILITY;
		break;
	case REF_COMMAND_CONTROL:
		mask = SCR_ST_COMMAND_CONTROL;
		break;
	case REF_CYBORG_FACTORY:
		mask = SCR_ST_CYBORG_FACTORY;
		break;
	case REF_VTOL_FACTORY:
		mask = SCR_ST_VTOL_FACTORY;
		break;
	case REF_REARM_PAD:
		mask = SCR_ST_REARM_PAD;
		break;
    case REF_MISSILE_SILO:
        //don't want the assert!
        mask = 0;
        break;
	case REF_LAB:
	case REF_BRIDGE:
	case REF_DEMOLISH:
	case REF_BLASTDOOR:
	case REF_GATE:
	default:
		ASSERT( false,
			"scrStructTargetMask: unknown or invalid target structure type" );
		mask = 0;
		break;
	}

	return mask;
}


// prioritise structure targets
static void scrStructTargetPriority(STRUCTURE **ppsTarget, STRUCTURE *psCurr)
{
	// skip walls unless they are explicitly asked for
	if ((scrStructPref & SCR_ST_WALL) ||
		((psCurr->pStructureType->type != REF_WALL) &&
		 (psCurr->pStructureType->type != REF_WALLCORNER)) )
	{
		*ppsTarget = psCurr;
	}
}


static UDWORD scrDroidTargetMask(DROID *psDroid)
{
	UDWORD				mask = 0;
	WEAPON_STATS		*psWStats;
	BODY_STATS			*psBStats;
	PROPULSION_STATS	*psPStats;

	// get the turret type
	switch (psDroid->droidType)
	{
	case DROID_PERSON:
	case DROID_CYBORG:
	case DROID_CYBORG_SUPER:
	case DROID_WEAPON:
		//if (psDroid->numWeaps > 0)
        if (psDroid->asWeaps[0].nStat > 0)
		{
			psWStats = asWeaponStats + psDroid->asWeaps[0].nStat;
			if (!proj_Direct(psWStats))
			{
				mask |= SCR_DT_WEAP_IDF;
			}
			else if (psWStats->surfaceToAir & SHOOT_IN_AIR)
			{
				mask |= SCR_DT_WEAP_AIR;
			}
			else
			{
				mask |= SCR_DT_WEAP_GROUND;
			}
		}
		else
		{
			mask |= SCR_DT_SENSOR;
		}
		break;
	case DROID_SENSOR:
	case DROID_ECM:
		mask |= SCR_DT_SENSOR;
		break;
	case DROID_CONSTRUCT:
    case DROID_CYBORG_CONSTRUCT:
		mask |= SCR_DT_CONSTRUCT;
		break;
	case DROID_COMMAND:
		mask |= SCR_DT_COMMAND;
		break;
	case DROID_REPAIR:
    case DROID_CYBORG_REPAIR:
		mask |= SCR_DT_REPAIR;
		break;
	case DROID_TRANSPORTER:
		break;
	case DROID_DEFAULT:
	case DROID_ANY:
	default:
		ASSERT( false,
			"scrUnitTargetMask: unknown or invalid target unit type" );
		break;
	}

	// get the body type
	psBStats = asBodyStats + psDroid->asBits[COMP_BODY].nStat;
	switch (psBStats->size)
	{
	case SIZE_LIGHT:
		mask |= SCR_DT_LIGHT;
		break;
	case SIZE_MEDIUM:
		mask |= SCR_DT_MEDIUM;
		break;
	case SIZE_HEAVY:
		mask |= SCR_DT_HEAVY;
		break;
	case SIZE_SUPER_HEAVY:
		mask |= SCR_DT_SUPER_HEAVY;
		break;
	default:
		ASSERT( false,
			"scrUnitTargetMask: unknown or invalid target body size" );
		break;
	}

	// get the propulsion type
	psPStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	switch(psPStats->propulsionType)
	{
	case PROPULSION_TYPE_WHEELED:
		mask |= SCR_DT_WHEEL;
		break;
	case PROPULSION_TYPE_TRACKED:
		mask |= SCR_DT_TRACK;
		break;
	case PROPULSION_TYPE_LEGGED:
	case PROPULSION_TYPE_JUMP:
		mask |= SCR_DT_LEGS;
		break;
	case PROPULSION_TYPE_HOVER:
		mask |= SCR_DT_HOVER;
		break;
	case PROPULSION_TYPE_LIFT:
		mask |= SCR_DT_VTOL;
		break;
	case PROPULSION_TYPE_HALF_TRACKED:
		mask |= SCR_DT_HTRACK;
		break;
	case PROPULSION_TYPE_PROPELLOR:
		mask |= SCR_DT_PROPELLOR;
		break;
	case PROPULSION_TYPE_SKI:
	default:
		ASSERT( false,
			"scrUnitTargetMask: unknown or invalid target unit propulsion type" );
		break;
	}


	return mask;
}

// prioritise droid targets
static void scrDroidTargetPriority(DROID **ppsTarget, DROID *psCurr)
{
	// priority to things with weapons
	if ( ((*ppsTarget) == NULL) ||
         ((*ppsTarget)->asWeaps[0].nStat == 0) )
	{
		*ppsTarget = psCurr;
	}
}


// what type of object for scrTargetInArea
enum
{
	SCR_TAR_STRUCT,
	SCR_TAR_DROID,
};

// get target mask function for scrTargetInArea
typedef UDWORD (*TARGET_MASK)(BASE_OBJECT *);

// see if one target is preferable to another
typedef void   (*TARGET_PREF)(BASE_OBJECT **, BASE_OBJECT *);

// generic find a target in an area given preference data
static BASE_OBJECT *scrTargetInArea(SDWORD tarPlayer, SDWORD visPlayer, SDWORD tarType, SDWORD cluster,
							 SDWORD x1, SDWORD y1, SDWORD x2, SDWORD y2)
{
	BASE_OBJECT		*psTarget, *psCurr;
	SDWORD			temp;
	BOOL			bVisCheck;
	UDWORD			tarMask;
	TARGET_MASK		getTargetMask;
	TARGET_PREF		targetPriority;
	UDWORD			prefer, ignore;

	if (tarPlayer < 0 || tarPlayer >= MAX_PLAYERS)
	{
		ASSERT( false,
			"scrTargetInArea: invalid target player number" );
		return NULL;
	}

	if (x1 > x2)
	{
		temp = x2;
		x2 = x1;
		x1 = temp;
	}
	if (y1 > y2)
	{
		temp = y2;
		y2 = y1;
		y1 = temp;
	}

	// see if a visibility check is required and for which player
	if (visPlayer < 0 || visPlayer >= MAX_PLAYERS)
	{
		bVisCheck = false;
	}
	else
	{
		bVisCheck = true;
	}
		bVisCheck = false;

	// see which target type to use
	switch (tarType)
	{
	case SCR_TAR_STRUCT:
		getTargetMask = (TARGET_MASK)scrStructTargetMask;
		targetPriority = (TARGET_PREF)scrStructTargetPriority;
		prefer = scrStructPref;
		ignore = scrStructIgnore;
		psCurr = (BASE_OBJECT *)apsStructLists[tarPlayer];
		break;
	case SCR_TAR_DROID:
		getTargetMask = (TARGET_MASK)scrDroidTargetMask;
		targetPriority = (TARGET_PREF)scrDroidTargetPriority;
		prefer = scrDroidPref;
		ignore = scrDroidIgnore;
		psCurr = (BASE_OBJECT *)apsDroidLists[tarPlayer];
		break;
	default:
		ASSERT( false, "scrTargetInArea: invalid target type" );
		return NULL;
	}

	psTarget = NULL;
	for(; psCurr; psCurr=psCurr->psNext)
	{
		if ((!bVisCheck || psCurr->visible[visPlayer]) &&
			(cluster == 0 || psCurr->cluster == cluster) &&
			((SDWORD)psCurr->pos.x >= x1) &&
			((SDWORD)psCurr->pos.x <= x2) &&
			((SDWORD)psCurr->pos.y >= y1) &&
			((SDWORD)psCurr->pos.y <= y2))
		{
			tarMask = getTargetMask(psCurr);
			if (tarMask & ignore)
			{
				// skip any that match ignore
				continue;
			}
			else if (tarMask & prefer)
			{
				// got a prefered target - go with that
				psTarget = psCurr;
				break;
			}
			else
			{
				// see if the current target has priority over the previous one
				targetPriority(&psTarget,psCurr);
			}
		}
	}


	return psTarget;
}

// get a structure target in an area using the preferences
WZ_DECL_UNUSED static BOOL scrStructTargetInArea(void)
{
	SDWORD		x1,y1,x2,y2;
	SDWORD		tarPlayer, visPlayer;

	if (!stackPopParams(6, VAL_INT, &tarPlayer, VAL_INT, &visPlayer,
						   VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	scrFunctionResult.v.oval = (STRUCTURE *)scrTargetInArea(tarPlayer, visPlayer, SCR_TAR_STRUCT, 0, x1,y1, x2,y2);

	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// get a structure target on the map using the preferences
WZ_DECL_UNUSED static BOOL scrStructTargetOnMap(void)
{
	SDWORD		tarPlayer, visPlayer;
	STRUCTURE	*psTarget;

	if (!stackPopParams(2, VAL_INT, &tarPlayer, VAL_INT, &visPlayer))
	{
		return false;
	}

	psTarget = (STRUCTURE *)scrTargetInArea(tarPlayer, visPlayer, SCR_TAR_STRUCT, 0,
									scrollMinX*TILE_UNITS,scrollMinY*TILE_UNITS,
									scrollMaxX*TILE_UNITS,scrollMaxY*TILE_UNITS);

	scrFunctionResult.v.oval = psTarget;
	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// get a droid target in an area using the preferences
WZ_DECL_UNUSED static BOOL scrDroidTargetInArea(void)
{
	SDWORD		x1,y1,x2,y2;
	SDWORD		tarPlayer, visPlayer;
	DROID		*psTarget;

	if (!stackPopParams(6, VAL_INT, &tarPlayer, VAL_INT, &visPlayer,
						   VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	psTarget = (DROID *)scrTargetInArea(tarPlayer, visPlayer, SCR_TAR_DROID, 0, x1,y1, x2,y2);

	scrFunctionResult.v.oval = psTarget;
	if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// get a droid target on the map using the preferences
WZ_DECL_UNUSED static BOOL scrDroidTargetOnMap(void)
{
	SDWORD		tarPlayer, visPlayer;
	DROID		*psTarget;

	if (!stackPopParams(2, VAL_INT, &tarPlayer, VAL_INT, &visPlayer))
	{
		return false;
	}

	psTarget = (DROID *)scrTargetInArea(tarPlayer, visPlayer, SCR_TAR_DROID, 0,
							scrollMinX*TILE_UNITS,scrollMinY*TILE_UNITS,
							scrollMaxX*TILE_UNITS,scrollMaxY*TILE_UNITS);

	scrFunctionResult.v.oval = psTarget;
	if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// get a target from a cluster using the preferences
WZ_DECL_UNUSED static BOOL scrTargetInCluster(void)
{
	SDWORD		tarPlayer, tarType, visPlayer, clusterID, cluster;
	BASE_OBJECT	*psTarget;

	if (!stackPopParams(2, VAL_INT, &clusterID, VAL_INT, &visPlayer))
	{
		return false;
	}

	if (clusterID < 0 || clusterID >= CLUSTER_MAX)
	{
		ASSERT( false, "scrTargetInCluster: invalid clusterID" );
		return false;
	}

	cluster = aClusterMap[clusterID];
	tarPlayer = aClusterInfo[cluster] & CLUSTER_PLAYER_MASK;
	tarType = (aClusterInfo[cluster] & CLUSTER_DROID) ? SCR_TAR_DROID : SCR_TAR_STRUCT;

	psTarget = scrTargetInArea(tarPlayer, visPlayer, tarType, cluster,
							scrollMinX*TILE_UNITS,scrollMinY*TILE_UNITS,
							scrollMaxX*TILE_UNITS,scrollMaxY*TILE_UNITS);

	scrFunctionResult.v.oval = psTarget;
	if (!stackPushResult((INTERP_TYPE)ST_BASEOBJECT, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// ********************************************************************************************
// ********************************************************************************************
// Additional Skirmish Code for superduper skirmish games may99
// ********************************************************************************************
// ********************************************************************************************

static int scrSkCanBuildTemplate(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	STRUCTURE *psStructure = (STRUCTURE*)luaWZObj_checkobject(L, 2, OBJ_STRUCTURE);
	DROID_TEMPLATE *psTempl = luaWZObj_checktemplate(L, 3);

	// is factory big enough?
	if(!validTemplateForFactory(psTempl, psStructure) )
	{
		goto failTempl;
	}

	if ((asBodyStats + psTempl->asParts[COMP_BODY])->size > ((FACTORY*)psStructure->pFunctionality)->capacity  )
	{
		goto failTempl;
	}

	// is every component from template available?
	// body available.
	if( apCompLists[player][COMP_BODY][psTempl->asParts[COMP_BODY]] != AVAILABLE )
	{
		goto failTempl;
	}

	// propulsion method available.
	if( apCompLists[player][COMP_PROPULSION][psTempl->asParts[COMP_PROPULSION]] != AVAILABLE )
	{
		goto failTempl;
	}

	// weapon/sensor

	switch (droidTemplateType(psTempl))
	{
	case DROID_CYBORG:		        // cyborg-type thang.. no need to check weapon.
	case DROID_CYBORG_SUPER:		// super cyborg-type thang
		break;
	case DROID_WEAPON:
		if( apCompLists[player][COMP_WEAPON][ psTempl->asWeaps[0] ] != AVAILABLE )
		{
			goto failTempl;
		}
		break;
	case DROID_SENSOR:
		if( apCompLists[player][COMP_SENSOR][psTempl->asParts[COMP_SENSOR]] != AVAILABLE )
		{
			goto failTempl;
		}
		break;
	case DROID_ECM:
		if( apCompLists[player][COMP_ECM][psTempl->asParts[COMP_ECM]] != AVAILABLE )
		{
			goto failTempl;
		}
		break;
	case DROID_REPAIR:
		if( apCompLists[player][COMP_REPAIRUNIT][psTempl->asParts[COMP_REPAIRUNIT]] != AVAILABLE )
		{
			goto failTempl;
		}
		break;
	case DROID_CYBORG_REPAIR:
		if( apCompLists[player][COMP_REPAIRUNIT][psTempl->asParts[COMP_REPAIRUNIT]] != AVAILABLE )
		{
			goto failTempl;
		}
		break;
	case DROID_COMMAND:
		if( apCompLists[player][COMP_BRAIN][psTempl->asParts[COMP_BRAIN]] != AVAILABLE )
		{
			goto failTempl;
		}
		break;
	case DROID_CONSTRUCT:
		if( apCompLists[player][COMP_CONSTRUCT][psTempl->asParts[COMP_CONSTRUCT]] != AVAILABLE )
		{
			goto failTempl;
		}
		break;
	case DROID_CYBORG_CONSTRUCT:
		if( apCompLists[player][COMP_CONSTRUCT][psTempl->asParts[COMP_CONSTRUCT]] != AVAILABLE )
		{
			goto failTempl;
		}
		break;

	case DROID_PERSON:		        // person
	case DROID_TRANSPORTER:	        // guess what this is!
	case DROID_DEFAULT:		        // Default droid
	case DROID_ANY:
	default:
		return luaL_error(L, "scrSkCanBuildTemplate: Unhandled template type" );
		abort();
		break;
	}

	lua_pushboolean(L, true);
	return 1;

failTempl:
	lua_pushboolean(L, false);
	return 1;
}

// ********************************************************************************************
/** locate the enemy
 * gives a target location given a player to attack.
 */
static int scrSkLocateEnemy(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	STRUCTURE	*psStruct;

	// find where the player has some structures..	// factories or hq.
	for(psStruct=apsStructLists[player];
		psStruct &&
			!( psStruct->pStructureType->type == REF_HQ
			|| psStruct->pStructureType->type == REF_FACTORY
			|| psStruct->pStructureType->type == REF_CYBORG_FACTORY
			|| psStruct->pStructureType->type == REF_VTOL_FACTORY
			);
		psStruct=psStruct->psNext);


	// set the x and y accordingly..
	if(psStruct)
	{
		luaWZObj_pushstructure(L, psStruct);
		return 1;
	}
	lua_pushnil(L);
	return 1;
}

// ********************************************************************************************

BOOL skTopicAvail(UWORD inc, UDWORD player)
{
	UDWORD				incPR, incS;
	PLAYER_RESEARCH		*pPlayerRes = asPlayerResList[player];
	BOOL				bPRFound, bStructFound;


	//if the topic is possible and has not already been researched - add to list
	if ((IsResearchPossible(&pPlayerRes[inc])))
	{
		if ((IsResearchCompleted(&pPlayerRes[inc])==false) && (IsResearchStarted(&pPlayerRes[inc])==false))
		{
		return true;
		}
	}

	// make sure that the research is not completed  or started by another researchfac
	if ((IsResearchCompleted(&pPlayerRes[inc])==false) && (IsResearchStarted(&pPlayerRes[inc])==false))
	{
		// Research is not completed  ... also  it has not been started by another researchfac

		//if there aren't any PR's - go to next topic
		if (!asResearch[inc].numPRRequired)
		{
			return false;
		}

		//check for pre-requisites
		bPRFound = true;
		for (incPR = 0; incPR < asResearch[inc].numPRRequired; incPR++)
		{
			if (IsResearchCompleted(&(pPlayerRes[asResearch[inc].pPRList[incPR]]))==0)
			{
				//if haven't pre-requisite - quit checking rest
				bPRFound = false;
				break;
			}
		}
		if (!bPRFound)
		{
			//if haven't pre-requisites, skip the rest of the checks
			return false;
		}

		//check for structure effects
		bStructFound = true;
		for (incS = 0; incS < asResearch[inc].numStructures; incS++)
		{
			//if (!checkStructureStatus(asStructureStats + asResearch[inc].
			//	pStructList[incS], playerID, SS_BUILT))
			if (!checkSpecificStructExists(asResearch[inc].pStructList[incS],
				player))
			{
				//if not built, quit checking
				bStructFound = false;
				break;
			}
		}
		if (!bStructFound)
		{
			//if haven't all structs built, skip to next topic
			return false;
		}

		return true;
	}
	return false;
}

// ********************************************************************************************
static int scrSkDoResearch(lua_State *L)
{
	UWORD				i;
	RESEARCH_FACILITY	*psResFacilty;
	PLAYER_RESEARCH		*pPlayerRes;
	RESEARCH			*pResearch;
	
	STRUCTURE *psBuilding = (STRUCTURE*)luaWZObj_checkobject(L, 1, OBJ_STRUCTURE);
	int player            = luaWZ_checkplayer(L, 2);
	// this parameter is passed but unused
	// int bias              = luaL_checkint(L, 3);

	psResFacilty =	(RESEARCH_FACILITY*)psBuilding->pFunctionality;

	if(psResFacilty->psSubject != NULL)
	{
		// not finshed yet..
		return 0;
	}

	// choose a topic to complete.
	for(i=0;i<numResearch;i++)
	{
		if (skTopicAvail(i, player) && (!bMultiPlayer || !beingResearchedByAlly(i, player)))
		{
			break;
		}
	}

	if(i != numResearch)
	{
		pResearch = asResearch + i;
		if (bMultiMessages)
		{
			sendResearchStatus(psBuilding, pResearch->ref - REF_RESEARCH_START, player, true);
		}
		else
		{
			pPlayerRes				= asPlayerResList[player]+ i;
			psResFacilty->psSubject = (BASE_STATS*)pResearch;		  //set the subject up

			if (IsResearchCancelled(pPlayerRes))
			{
				psResFacilty->powerAccrued = pResearch->researchPower;//set up as if all power available for cancelled topics
			}
			else
			{
				psResFacilty->powerAccrued = 0;
			}

			MakeResearchStarted(pPlayerRes);
			psResFacilty->timeStarted = ACTION_START_TIME;
			psResFacilty->timeStartHold = 0;
			psResFacilty->timeToResearch = pResearch->researchPoints / 	psResFacilty->researchPoints;
			if (psResFacilty->timeToResearch == 0)
			{
				psResFacilty->timeToResearch = 1;
			}
		}

#if defined (DEBUG)
		{
			char				sTemp[128];
			sprintf(sTemp,"player:%d starts topic: %s",player, asResearch[i].pName );
			NETlogEntry(sTemp, SYNC_FLAG, 0);
		}
#endif
	}

	return 0;
}

// ********************************************************************************************
static int scrSkVtolEnableCheck(lua_State *L)
{
	UDWORD i;

	int player = luaWZ_checkplayer(L, 1);

	// vtol factory
	for(i=0;(i<numStructureStats)&&(asStructureStats[i].type != REF_VTOL_FACTORY);i++);
	if( (i < numStructureStats) && (apStructTypeLists[player][i] == AVAILABLE) )
	{
		// vtol propulsion
		for(i=0;(i<numPropulsionStats);i++)
		{
			if((asPropulsionStats[i].propulsionType == PROPULSION_TYPE_LIFT)
			 && apCompLists[player][COMP_PROPULSION][i] == AVAILABLE)
			{
				lua_pushboolean(L, true);
				return 1;
			}
		}
	}

	lua_pushboolean(L, false);
	return 1;
}

// ********************************************************************************************
static int scrSkGetFactoryCapacity(lua_State *L)
{
	int count=0;
	STRUCTURE *psStructure = (STRUCTURE*)luaWZObj_checkobject(L, 1, OBJ_STRUCTURE);

	if(psStructure && StructIsFactory(psStructure))
	{
		count = ((FACTORY *)psStructure->pFunctionality)->capacity;
	}

	lua_pushinteger(L, count);
	return 1;
}
// ********************************************************************************************
static int scrSkDifficultyModifier(lua_State *L)
{
	RESEARCH_FACILITY	*psResFacility;
	STRUCTURE		*psStr;
	PLAYER_RESEARCH		*pPlayerRes;
	
	int player = luaL_checkint(L,1);

	/* Skip cheats if difficulty modifier slider is set to minimum.
	 * (0 - player disabled, 20 - max value, UBYTE_MAX - autogame)
	 */
	if (game.skDiff[player] <= 1 || game.skDiff[player] == UBYTE_MAX)
	{
		return 0;
	}

	// power modifier, range: 0-1000
	giftPower(ANYPLAYER, player, game.skDiff[player] * 50, true);

	//research modifier
	for (psStr=apsStructLists[player];psStr;psStr=psStr->psNext)
	{
		if (psStr->pStructureType->type == REF_RESEARCH)
		{
			psResFacility =	(RESEARCH_FACILITY*)psStr->pFunctionality;

			// subtract 0 - 80% off the time to research.
			if (psResFacility->psSubject)
			{
				RESEARCH	*psResearch = (RESEARCH *)psResFacility->psSubject;

				pPlayerRes = asPlayerResList[player] + (psResearch->ref - REF_RESEARCH_START);
				pPlayerRes->currentPoints += (psResearch->researchPoints * 4 * game.skDiff[player]) / 100;
			}
		}

	}

	return 0;
}

// ********************************************************************************************

// not a direct script function but a helper for scrSkDefenseLocation and scrSkDefenseLocationB
static int defenseLocation(lua_State *L, BOOL noNarrowGateways)
{
	UDWORD		x,y,gX,gY,dist,nearestSoFar,count;
	GATEWAY		*psGate,*psChosenGate;
	BASE_STATS	*psStats,*psWStats;
	UDWORD		x1,x2,x3,x4,y1,y2,y3,y4;
	BOOL		noWater;
	UDWORD      minCount;
	UDWORD      offset;

	int worldX = luaL_checkint(L, 1);
	int worldY = luaL_checkint(L, 2);
	int statIndex = luaL_checkint(L, 3);
	int statIndex2 = luaL_checkint(L, 4);
	DROID *psDroid = (DROID*)luaWZObj_checkobject(L, 5, OBJ_DROID);
	// and eventual last player argument is ignored

	ASSERT_OR_RETURN( false, statIndex < numStructureStats, "Invalid range referenced for numStructureStats, %d > %d", statIndex, numStructureStats);
	psStats = (BASE_STATS *)(asStructureStats + statIndex);

	ASSERT_OR_RETURN( false, statIndex2 < numStructureStats, "Invalid range referenced for numStructureStats, %d > %d", statIndex2, numStructureStats);
	psWStats = (BASE_STATS *)(asStructureStats + statIndex2);

    // check for wacky coords.
	if( worldX < 0 ||
	    worldX > world_coord(mapWidth) ||
	    worldY < 0 ||
	    worldY > world_coord(mapHeight) )
	{
		goto failed;
	}

	x = map_coord(worldX);					// change to tile coords.
	y = map_coord(worldY);

	// go down the gateways, find the nearest gateway with >1 empty tiles
	nearestSoFar = UDWORD_MAX;
	psChosenGate = NULL;
	for (psGate = gwGetGateways(); psGate; psGate = psGate->psNext)
	{
		count = 0;
		noWater = true;
		// does it have >1 tile unoccupied.
		if(psGate->x1 == psGate->x2)
		{// vert
			//skip gates that are too short
			if(noNarrowGateways && (psGate->y2 - psGate->y1) <= 2)
			{
				continue;
			}
			gX = psGate->x1;
			for(gY=psGate->y1;gY <= psGate->y2; gY++)
			{
				if(! TileIsOccupied(mapTile(gX,gY) ))
				{
					count++;
				}
				if (terrainType(mapTile(gX,gY)) == TER_WATER)
				{
					noWater = false;
				}
			}
		}
		else
		{// horiz
			//skip gates that are too short
			if(noNarrowGateways && (psGate->x2 - psGate->x1) <= 2)
			{
				continue;
			}
			gY = psGate->y1;
			for(gX=psGate->x1;gX <= psGate->x2; gX++)
			{
				if(! TileIsOccupied(mapTile(gX,gY) ))
				{
					count++;
				}
				if (terrainType(mapTile(gX,gY)) == TER_WATER)
				{
					noWater = false;
				}
			}
		}
		if(noNarrowGateways) {
		    minCount = 2;
		} else {
		    minCount = 1;
		}
		if(count > minCount && noWater)	//<NEW> min 2 tiles
		{
			// ok it's free. Is it the nearest one yet?
			/* Get gateway midpoint */
			gX = (psGate->x1 + psGate->x2)/2;
			gY = (psGate->y1 + psGate->y2)/2;
			/* Estimate the distance to it */
			dist = iHypot(x - gX, y - gY);
			/* Is it best we've found? */
			if(dist<nearestSoFar && dist<30)
			{
				/* Yes, then keep a record of it */
				nearestSoFar = dist;
				psChosenGate = psGate;
			}
		}
	}

	if(!psChosenGate)	// we have a gateway.
	{
		goto failed;
	}

	// find an unnocupied tile on that gateway.
	if(psChosenGate->x1 == psChosenGate->x2)
	{// vert
		gX = psChosenGate->x1;
		for(gY=psChosenGate->y1;gY <= psChosenGate->y2; gY++)
		{
			if(! TileIsOccupied(mapTile(gX,gY) ))
			{
				y = gY;
				x = gX;
				break;
			}
		}
	}
	else
	{// horiz
		gY = psChosenGate->y1;
		for(gX=psChosenGate->x1;gX <= psChosenGate->x2; gX++)
		{
			if(! TileIsOccupied(mapTile(gX,gY) ))
			{
				y = gY;
				x = gX;
				break;
			}
		}
	}

	// order the droid to build two walls, one either side of the gateway.
	// or one in the case of a 2 size gateway.

	//find center of the gateway
	x = (psChosenGate->x1 + psChosenGate->x2)/2;
	y = (psChosenGate->y1 + psChosenGate->y2)/2;

	//find start pos of the gateway
	x1 = world_coord(psChosenGate->x1) + (TILE_UNITS / 2);
	y1 = world_coord(psChosenGate->y1) + (TILE_UNITS / 2);

	if(noNarrowGateways) {
	    offset = 2;
	} else {
	    offset = 1;
	}
	if(psChosenGate->x1 == psChosenGate->x2)	//vert
	{
		x2 = x1;	//vert: end x pos of the first section = start x pos
		y2 = world_coord(y - 1) + TILE_UNITS / 2;	//start y loc of the first sec
		x3 = x1;
		y3 = world_coord(y + offset) + TILE_UNITS / 2;
	}
	else		//hor
	{
		x2 = world_coord(x - 1) + TILE_UNITS / 2;
		y2 = y1;
		x3 = world_coord(x+offset) + TILE_UNITS / 2;
		y3 = y1;

	}
	//end coords of the second section
	x4 = world_coord(psChosenGate->x2) + TILE_UNITS / 2;
	y4 = world_coord(psChosenGate->y2) + TILE_UNITS / 2;

	// first section.
	if(x1 == x2 && y1 == y2)	//first sec is 1 tile only: ((2 tile gate) or (3 tile gate and first sec))
	{
		orderDroidStatsLocDir(psDroid, DORDER_BUILD, psWStats, x1, y1, 0);
	}
	else
	{
		orderDroidStatsTwoLocDir(psDroid, DORDER_LINEBUILD, psWStats,  x1, y1, x2, y2, 0);
	}

	// second section
	if(x3 == x4 && y3 == y4)
	{
		orderDroidStatsLocDirAdd(psDroid, DORDER_BUILD, psWStats, x3, y3, 0);
	}
	else
	{
		orderDroidStatsTwoLocDirAdd(psDroid, DORDER_LINEBUILD, psWStats,  x3, y3, x4, y4, 0);
	}

	// back to world coords and return result.
	lua_pushinteger(L, world_coord(x) + (TILE_UNITS / 2));
	lua_pushinteger(L, world_coord(y) + (TILE_UNITS / 2));
	return 2;

failed:
	return 0;
}


// return a good place to build a defence, given a starting point
static int scrSkDefenseLocation(lua_State *L)
{
    return defenseLocation(L, false);
}

// return a good place to build a defence with a min number of clear tiles
static int scrSkDefenseLocationB(lua_State *L)
{
    return defenseLocation(L, true);
}

static int scrSkFireLassat(lua_State *L)
{
	int player = luaWZ_checkplayer(L, 1);
	BASE_OBJECT *psObj = luaWZObj_checkbaseobject(L, 2);

	if(psObj)
	{
		orderStructureObj(player, psObj);
	}

	return 0;
}

//-----------------------
// New functions
//-----------------------
WZ_DECL_UNUSED static BOOL scrActionDroidObj(void)
{
	DROID			*psDroid;
	DROID_ACTION		action;
	BASE_OBJECT		*psObj;

	if (!stackPopParams(3, ST_DROID, &psDroid, VAL_INT, &action, ST_BASEOBJECT, &psObj))
	{
		debug(LOG_ERROR, "scrActionDroidObj: failed to pop");
		return false;
	}

	ASSERT( psDroid != NULL,
		"scrOrderUnitObj: Invalid unit pointer" );
	ASSERT( psObj != NULL,
		"scrOrderUnitObj: Invalid object pointer" );

	if (psDroid == NULL || psObj == NULL)
	{
		return false;
	}

	if (action != DACTION_DROIDREPAIR)
	{
		debug(LOG_ERROR, "scrActionDroidObj: this action is not supported");
		return false;
	}

	actionDroidObj(psDroid, action, (BASE_OBJECT *)psObj);

	return true;
}

//<script function - improved version
// variables for the group iterator
DROID_GROUP		*psScrIterateGroupB[MAX_PLAYERS];
DROID			*psScrIterateGroupDroidB[MAX_PLAYERS];

/// initialise iterating a groups members
static int scrInitIterateGroupB(lua_State *L)
{
	DROID_GROUP *psGroup = luaWZObj_checkgroup(L, 1);
	int bucket = luaWZ_checkplayer(L, 2);

	psScrIterateGroupB[bucket] = psGroup;
	psScrIterateGroupDroidB[bucket] = psGroup->psList;

	return 0;
}

//script function - improved version
/// iterate through a groups members
static int scrIterateGroupB(lua_State *L)
{
	DROID *psDroid;
	
	DROID_GROUP *psGroup = luaWZObj_checkgroup(L, 1);
	int bucket = luaWZ_checkplayer(L, 2);

	if (psGroup != psScrIterateGroupB[bucket])
	{
		return luaL_error(L, "invalid group, InitGroupIterateB not called?" );
	}

	if (psScrIterateGroupDroidB[bucket] != NULL)
	{
		psDroid = psScrIterateGroupDroidB[bucket];
		psScrIterateGroupDroidB[bucket] = psScrIterateGroupDroidB[bucket]->psGrpNext;
	}
	else
	{
		lua_pushnil(L);
		return 1;
	}

	luaWZObj_pushdroid(L, psDroid);
	return 1;
}

static int scrDroidCanReach(lua_State *L)
{
	DROID *psDroid = (DROID*)luaWZObj_checkobject(L, 1, OBJ_DROID);
	int x = luaL_checkint(L, 2);
	int y = luaL_checkint(L, 3);

	const PROPULSION_STATS *psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	const Vector3i rPos = {x, y, 0};

	lua_pushboolean(L, fpathCheck(psDroid->pos, rPos, psPropStats->propulsionType));
	return 1;
}

// ********************************************************************************************
// ********************************************************************************************

static int scrGroupCountMembers(lua_State *L)
{
	DROID_GROUP *group = luaWZObj_checkgroup(L, 1);
	
	lua_pushinteger(L, grpNumMembers(group));
	return 1;
}

void registerScriptAIfuncs(lua_State *L)
{
	lua_register(L, "groupAddArea", scrGroupAddArea);
	lua_register(L, "Group", scrNewGroup);
	lua_register(L, "groupAddGroup", scrGroupAddGroup);
	lua_register(L, "initIterateGroup", scrInitIterateGroup);
	lua_register(L, "iterateGroup", scrIterateGroup);
	lua_register(L, "groupAddDroid", scrGroupAddDroid);
	lua_register(L, "orderGroupLoc", scrOrderGroupLoc);
	lua_register(L, "groupCountMembers", scrGroupCountMembers);
	lua_register(L, "idleGroup", scrIdleGroup);
	lua_register(L, "orderDroidStatsLoc", scrOrderDroidStatsLoc);
	lua_register(L, "skCanBuildTemplate", scrSkCanBuildTemplate);
	lua_register(L, "skLocateEnemy", scrSkLocateEnemy);
	lua_register(L, "orderDroid", scrOrderDroid);
	lua_register(L, "setDroidSecondary", scrSetDroidSecondary);
	lua_register(L, "skDoResearch", scrSkDoResearch);
	lua_register(L, "orderDroidObj", scrOrderDroidObj);
	lua_register(L, "skDifficultyModifier", scrSkDifficultyModifier);
	lua_register(L, "skVtolEnableCheck", scrSkVtolEnableCheck);
	lua_register(L, "droidLeaveGroup", scrDroidLeaveGroup);
	lua_register(L, "initIterateGroupB", scrInitIterateGroupB);
	lua_register(L, "iterateGroupB", scrIterateGroupB);
	lua_register(L, "skGetFactoryCapacity", scrSkGetFactoryCapacity);
	lua_register(L, "orderDroidLoc", scrOrderDroidLoc);
	lua_register(L, "skDefenseLocation", scrSkDefenseLocation);
	lua_register(L, "skDefenseLocationB", scrSkDefenseLocationB);
	lua_register(L, "droidCanReach", scrDroidCanReach);
	lua_register(L, "skFireLassat", scrSkFireLassat);
	lua_register(L, "orderGroupObj", scrOrderGroupObj);
	lua_register(L, "groupMember", scrGroupMember);
	lua_register(L, "setGroupSecondary", scrSetGroupSecondary);
	//lua_register(L, "", );
	//lua_register(L, "", );
	//lua_register(L, "", );
}
