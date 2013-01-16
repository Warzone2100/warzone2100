/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

static INTERP_VAL	scrFunctionResult;	//function return value to be pushed to stack

// Add a droid to a group
bool scrGroupAddDroid(void)
{
	DROID_GROUP		*psGroup;
	DROID			*psDroid;

	if (!stackPopParams(2, ST_GROUP, &psGroup, ST_DROID, &psDroid))
	{
		return false;
	}

	ASSERT(psGroup != NULL,
	       "scrGroupAdd: Invalid group pointer");
	ASSERT(psDroid != NULL,
	       "scrGroupAdd: Invalid droid pointer");
	if (psDroid == NULL)
	{
		return false;
	}
	if (psDroid->droidType == DROID_COMMAND)
	{
		debug(LOG_ERROR,
		      "scrGroupAdd: cannot add a command droid to a group");
		return false;
	}
	if (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)
	{
		debug(LOG_ERROR,
		      "scrGroupAdd: cannot add a transporter to a group");
		return false;
	}

	psGroup->add(psDroid);

	return true;
}


// Add droids in an area to a group
bool scrGroupAddArea(void)
{
	DROID_GROUP		*psGroup;
	DROID			*psDroid;
	SDWORD			x1, y1, x2, y2, player;

	if (!stackPopParams(6, ST_GROUP, &psGroup, VAL_INT, &player,
	        VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	ASSERT(psGroup != NULL,
	       "scrGroupAdd: Invalid group pointer");

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT(false, "scrGroupAddArea: invalid player");
		return false;
	}

	for (psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if (((SDWORD)psDroid->pos.x >= x1) && ((SDWORD)psDroid->pos.x <= x2) &&
		    ((SDWORD)psDroid->pos.y >= y1) && ((SDWORD)psDroid->pos.y <= y2) &&
		    psDroid->droidType != DROID_COMMAND &&
		    (psDroid->droidType != DROID_TRANSPORTER && psDroid->droidType != DROID_SUPERTRANSPORTER))

		{
			psGroup->add(psDroid);
		}
	}

	return true;
}


// Add groupless droids in an area to a group
bool scrGroupAddAreaNoGroup(void)
{
	DROID_GROUP		*psGroup;
	DROID			*psDroid;
	SDWORD			x1, y1, x2, y2, player;

	if (!stackPopParams(6, ST_GROUP, &psGroup, VAL_INT, &player,
	        VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	ASSERT(psGroup != NULL,
	       "scrGroupAddNoGroup: Invalid group pointer");

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT(false, "scrGroupAddAreaNoGroup: invalid player");
		return false;
	}

	for (psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if (((SDWORD)psDroid->pos.x >= x1) && ((SDWORD)psDroid->pos.x <= x2) &&
		    ((SDWORD)psDroid->pos.y >= y1) && ((SDWORD)psDroid->pos.y <= y2) &&
		    psDroid->droidType != DROID_COMMAND &&
		    (psDroid->droidType != DROID_TRANSPORTER && psDroid->droidType != DROID_SUPERTRANSPORTER) &&
		    psDroid->psGroup   == NULL)
		{
			psGroup->add(psDroid);
		}
	}

	return true;
}


// Move the droids from one group to another
bool scrGroupAddGroup(void)
{
	DROID_GROUP		*psTo, *psFrom;
	DROID			*psDroid, *psNext;

	if (!stackPopParams(2, ST_GROUP, &psTo, ST_GROUP, &psFrom))
	{
		return false;
	}

	ASSERT(psTo != NULL,
	       "scrGroupAddGroup: Invalid group pointer");
	ASSERT(psFrom != NULL,
	       "scrGroupAddGroup: Invalid group pointer");

	for (psDroid = psFrom->psList; psDroid; psDroid = psNext)
	{
		psNext = psDroid->psGrpNext;
		psTo->add(psDroid);
	}

	return true;
}


// check if a droid is a member of a group
bool scrGroupMember(void)
{
	DROID_GROUP		*psGroup;
	DROID			*psDroid;
	bool			retval;

	if (!stackPopParams(2, ST_GROUP, &psGroup, ST_DROID, &psDroid))
	{
		return false;
	}

	ASSERT(psGroup != NULL,
	       "scrGroupMember: Invalid group pointer");
	ASSERT(psDroid != NULL,
	       "scrGroupMember: Invalid droid pointer");
	if (psDroid == NULL)
	{
		return false;
	}

	if (psDroid->psGroup == psGroup)
	{
		retval = true;
	}
	else
	{
		retval = false;
	}

	scrFunctionResult.v.bval = retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// returns number of idle droids in a group.
bool scrIdleGroup(void)
{
	DROID_GROUP *psGroup;
	DROID		*psDroid;
	UDWORD		count = 0;

	if (!stackPopParams(1, ST_GROUP, &psGroup))
	{
		return false;
	}
	ASSERT_OR_RETURN(false, psGroup != NULL, "Invalid group pointer");

	for (psDroid = psGroup->psList; psDroid; psDroid = psDroid->psGrpNext)
	{
		if (psDroid->order.type == DORDER_NONE || (psDroid->order.type == DORDER_GUARD && psDroid->order.psObj == NULL))
		{
			count++;
		}
	}

	scrFunctionResult.v.ival = (SDWORD)count;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}
	return true;
}

// variables for the group iterator
static DROID_GROUP		*psScrIterateGroup;
static DROID			*psScrIterateGroupDroid;

// initialise iterating a groups members
bool scrInitIterateGroup(void)
{
	DROID_GROUP	*psGroup;

	if (!stackPopParams(1, ST_GROUP, &psGroup))
	{
		return false;
	}

	ASSERT(psGroup != NULL,
	       "scrInitGroupIterate: invalid group pointer");

	psScrIterateGroup = psGroup;
	psScrIterateGroupDroid = psGroup->psList;

	return true;
}


// iterate through a groups members
bool scrIterateGroup(void)
{
	DROID_GROUP	*psGroup;
	DROID		*psDroid;

	if (!stackPopParams(1, ST_GROUP, &psGroup))
	{
		return false;
	}

	if (psGroup != psScrIterateGroup)
	{
		debug(LOG_ERROR, "scrIterateGroup: invalid group, InitGroupIterate not called?");
		return false;
	}

	if (psScrIterateGroupDroid != NULL)
	{
		psDroid = psScrIterateGroupDroid;
		psScrIterateGroupDroid = psScrIterateGroupDroid->psGrpNext;
	}
	else
	{
		psDroid = NULL;
	}

	scrFunctionResult.v.oval = psDroid;
	if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
	{
		return false;
	}

	return true;
}


// initialise iterating a cluster
bool scrInitIterateCluster(void)
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
bool scrIterateCluster(void)
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


// remove a droid from a group
bool scrDroidLeaveGroup(void)
{
	DROID			*psDroid;

	if (!stackPopParams(1, ST_DROID, &psDroid))
	{
		return false;
	}

	if (psDroid->psGroup != NULL)
	{
		psDroid->psGroup->remove(psDroid);
	}

	return true;
}


// Give a group an order
bool scrOrderGroup(void)
{
	DROID_GROUP		*psGroup;
	DROID_ORDER		order;

	if (!stackPopParams(2, ST_GROUP, &psGroup, VAL_INT, &order))
	{
		return false;
	}

	ASSERT(psGroup != NULL,
	       "scrOrderGroup: Invalid group pointer");

	if (order != DORDER_STOP &&
	    order != DORDER_RETREAT &&
	    order != DORDER_DESTRUCT &&
	    order != DORDER_RTR &&
	    order != DORDER_RTB &&
	    order != DORDER_RUN)
	{
		ASSERT(false,
		       "scrOrderGroup: Invalid order");
		return false;
	}

	debug(LOG_NEVER, "group %p (%u) order %d", psGroup, psGroup->getNumMembers(), order);
	psGroup->orderGroup(order);

	return true;
}


// Give a group an order to a location
bool scrOrderGroupLoc(void)
{
	DROID_GROUP		*psGroup;
	DROID_ORDER		order;
	SDWORD			x, y;

	if (!stackPopParams(4, ST_GROUP, &psGroup, VAL_INT, &order, VAL_INT, &x, VAL_INT, &y))
	{
		return false;
	}

	ASSERT(psGroup != NULL,
	       "scrOrderGroupLoc: Invalid group pointer");

	if (order != DORDER_MOVE &&
	    order != DORDER_SCOUT)
	{
		ASSERT(false,
		       "scrOrderGroupLoc: Invalid order");
		return false;
	}
	if (x < 0
	    || x > world_coord(mapWidth)
	    || y < 0
	    || y > world_coord(mapHeight))
	{
		ASSERT(false, "Invalid map location (%d, %d), max is (%d, %d)", x, y, world_coord(mapWidth), world_coord(mapHeight));
		return false;
	}

	debug(LOG_NEVER, "group %p (%u) order %d (%d,%d)",
	      psGroup, psGroup->getNumMembers(), order, x, y);
	psGroup->orderGroup(order, (UDWORD)x, (UDWORD)y);

	return true;
}


// Give a group an order to an object
bool scrOrderGroupObj(void)
{
	DROID_GROUP		*psGroup;
	DROID_ORDER		order;
	BASE_OBJECT		*psObj;

	if (!stackPopParams(3, ST_GROUP, &psGroup, VAL_INT, &order, ST_BASEOBJECT, &psObj))
	{
		return false;
	}

	ASSERT(psGroup != NULL,
	       "scrOrderGroupObj: Invalid group pointer");
	ASSERT(psObj != NULL,
	       "scrOrderGroupObj: Invalid object pointer");

	if (order != DORDER_ATTACK &&
	    order != DORDER_HELPBUILD &&
	    order != DORDER_DEMOLISH &&
	    order != DORDER_REPAIR &&
	    order != DORDER_OBSERVE &&
	    order != DORDER_EMBARK &&
	    order != DORDER_FIRESUPPORT &&
	    order != DORDER_DROIDREPAIR)
	{
		ASSERT(false,
		       "scrOrderGroupObj: Invalid order");
		return false;
	}

	debug(LOG_NEVER, "group %p (%u) order %d,  obj type %d player %d id %d",
	      psGroup, psGroup->getNumMembers(), order, psObj->type, psObj->player, psObj->id);

	psGroup->orderGroup(order, psObj);

	return true;
}

// Give a droid an order
bool scrOrderDroid(void)
{
	DROID			*psDroid;
	DROID_ORDER		order;

	if (!stackPopParams(2, ST_DROID, &psDroid, VAL_INT, &order))
	{
		return false;
	}

	ASSERT(psDroid != NULL,
	       "scrOrderUnit: Invalid unit pointer");
	if (psDroid == NULL)
	{
		return false;
	}

	if (order != DORDER_STOP &&
	    order != DORDER_RETREAT &&
	    order != DORDER_DESTRUCT &&
	    order != DORDER_RTR &&
	    order != DORDER_RTB &&
	    order != DORDER_RUN &&
	    order != DORDER_NONE)
	{
		ASSERT(false,
		       "scrOrderUnit: Invalid order %d", order);
		return false;
	}

	orderDroid(psDroid, order, ModeQueue);

	return true;
}


// Give a Droid an order to a location
bool scrOrderDroidLoc(void)
{
	DROID			*psDroid;
	DROID_ORDER		order;
	SDWORD			x, y;

	if (!stackPopParams(4, ST_DROID, &psDroid, VAL_INT, &order, VAL_INT, &x, VAL_INT, &y))
	{
		return false;
	}

	ASSERT(psDroid != NULL,
	       "scrOrderUnitLoc: Invalid unit pointer");
	if (psDroid == NULL)
	{
		return false;
	}

	if (order != DORDER_MOVE &&
	    order != DORDER_SCOUT)
	{
		ASSERT(false,
		       "scrOrderUnitLoc: Invalid order");
		return false;
	}
	if (x < 0
	    || x > world_coord(mapWidth)
	    || y < 0
	    || y > world_coord(mapHeight))
	{
		ASSERT(false,
		       "scrOrderUnitLoc: Invalid location");
		return false;
	}

	orderDroidLoc(psDroid, order, x, y, ModeQueue);

	return true;
}


// Give a Droid an order to an object
bool scrOrderDroidObj(void)
{
	DROID			*psDroid;
	DROID_ORDER		order;
	BASE_OBJECT		*psObj;

	if (!stackPopParams(3, ST_DROID, &psDroid, VAL_INT, &order, ST_BASEOBJECT, &psObj))
	{
		return false;
	}

	ASSERT(psDroid != NULL,
	       "scrOrderUnitObj: Invalid unit pointer");
	ASSERT(psObj != NULL,
	       "scrOrderUnitObj: Invalid object pointer");
	if (psDroid == NULL || psObj == NULL)
	{
		return false;
	}

	if (order != DORDER_ATTACK &&
	    order != DORDER_HELPBUILD &&
	    order != DORDER_DEMOLISH &&
	    order != DORDER_REPAIR &&
	    order != DORDER_OBSERVE &&
	    order != DORDER_EMBARK &&
	    order != DORDER_FIRESUPPORT &&
	    order != DORDER_DROIDREPAIR)
	{
		ASSERT(false,
		       "scrOrderUnitObj: Invalid order");
		return false;
	}

	orderDroidObj(psDroid, order, psObj, ModeQueue);

	return true;
}

// Give a Droid an order with a stat
bool scrOrderDroidStatsLoc(void)
{
	DROID			*psDroid;
	DROID_ORDER		order;
	SDWORD			x, y, statIndex;

	if (!stackPopParams(5, ST_DROID, &psDroid, VAL_INT, &order, ST_STRUCTURESTAT, &statIndex,
	        VAL_INT, &x, VAL_INT, &y))
	{
		return false;
	}

	if (statIndex < 0 || statIndex >= (SDWORD)numStructureStats)
	{
		ASSERT(false, "Invalid structure stat");
		return false;
	}

	ASSERT_OR_RETURN(false, statIndex < numStructureStats, "Invalid range referenced for numStructureStats, %d > %d", statIndex, numStructureStats);
	STRUCTURE_STATS *psStats = asStructureStats + statIndex;

	ASSERT_OR_RETURN(false, psDroid != NULL, "Invalid Unit pointer");
	ASSERT_OR_RETURN(false, psStats != NULL, "Invalid object pointer");
	if (psDroid == NULL)
	{
		return false;
	}

	if ((x < 0) || (x > (SDWORD)mapWidth * TILE_UNITS) ||
	    (y < 0) || (y > (SDWORD)mapHeight * TILE_UNITS))
	{
		ASSERT(false, "Invalid location");
		return false;
	}

	if (order != DORDER_BUILD)
	{
		ASSERT(false, "Invalid order");
		return false;
	}
	// HACK: FIXME: Looks like a script error in the player*.slo files
	// buildOnExactLocation() which references previously destroyed buildings from
	// _stat = rebuildStructStat[_count]  causes this.
	if (strcmp(psStats->pName, "A0ADemolishStructure") == 0)
	{
		// I don't feel like spamming a ASSERT here, we *know* it is a issue.
		return true;
	}

	orderDroidStatsLocDir(psDroid, order, psStats, x, y, 0, ModeQueue);

	return true;
}


// set the secondary state for a droid
bool scrSetDroidSecondary(void)
{
	DROID		*psDroid;
	SECONDARY_ORDER	sec;
	SECONDARY_STATE	state;

	if (!stackPopParams(3, ST_DROID, &psDroid, VAL_INT, &sec, VAL_INT, &state))
	{
		return false;
	}

	ASSERT(psDroid != NULL,
	       "scrSetUnitSecondary: invalid unit pointer");
	if (psDroid == NULL)
	{
		return false;
	}

	secondarySetState(psDroid, sec, state);

	return true;
}

// set the secondary state for a droid
bool scrSetGroupSecondary(void)
{
	DROID_GROUP		*psGroup;
	SECONDARY_ORDER		sec;
	SECONDARY_STATE		state;

	if (!stackPopParams(3, ST_GROUP, &psGroup, VAL_INT, &sec, VAL_INT, &state))
	{
		return false;
	}

	ASSERT(psGroup != NULL,
	       "scrSetGroupSecondary: invalid group pointer");

	psGroup->setSecondary(sec, state);

	return true;
}


// add a droid to a commander
bool scrCmdDroidAddDroid(void)
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
bool scrCmdDroidMaxGroup(void)
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
bool scrResetStructTargets(void)
{
	scrStructPref = 0;
	scrStructIgnore = 0;

	return true;
}


// reset the droid preferences
bool scrResetDroidTargets(void)
{
	scrDroidPref = 0;
	scrDroidIgnore = 0;

	return true;
}


// set prefered structure target types
bool scrSetStructTarPref(void)
{
	UDWORD	pref;

	if (!stackPopParams(1, VAL_INT, &pref))
	{
		return false;
	}

	ASSERT((SCR_ST_HQ					== pref) ||
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
	       "scrSetStructTarPref: unknown target preference");

	scrStructPref |= pref;

	return true;
}


// set structure target ignore types
bool scrSetStructTarIgnore(void)
{
	UDWORD	pref;

	if (!stackPopParams(1, VAL_INT, &pref))
	{
		return false;
	}

	ASSERT((SCR_ST_HQ					== pref) ||
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
	       "scrSetStructTarIgnore: unknown ignore target");

	scrStructIgnore |= pref;

	return true;
}


// set prefered droid target types
bool scrSetDroidTarPref(void)
{
	UDWORD	pref;

	if (!stackPopParams(1, VAL_INT, &pref))
	{
		return false;
	}

	ASSERT((SCR_DT_COMMAND		== pref) ||
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
	       "scrSetUnitTarPref: unknown target preference");


	scrDroidPref |= pref;

	return true;
}

// set droid target ignore types
bool scrSetDroidTarIgnore(void)
{
	UDWORD	pref;

	if (!stackPopParams(1, VAL_INT, &pref))
	{
		return false;
	}

	ASSERT((SCR_DT_COMMAND		== pref) ||
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
	       "scrSetUnitTarIgnore: unknown ignore target");

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
		ASSERT(false,
		       "scrStructTargetMask: unknown or invalid target structure type");
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
	     (psCurr->pStructureType->type != REF_WALLCORNER)))
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
	case DROID_SUPERTRANSPORTER:
		break;
	case DROID_DEFAULT:
	case DROID_ANY:
	default:
		ASSERT(false,
		       "scrUnitTargetMask: unknown or invalid target unit type");
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
		ASSERT(false,
		       "scrUnitTargetMask: unknown or invalid target body size");
		break;
	}

	// get the propulsion type
	psPStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	switch (psPStats->propulsionType)
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
		ASSERT(false,
		       "scrUnitTargetMask: unknown or invalid target unit propulsion type");
		break;
	}


	return mask;
}

// prioritise droid targets
static void scrDroidTargetPriority(DROID **ppsTarget, DROID *psCurr)
{
	// priority to things with weapons
	if (((*ppsTarget) == NULL) ||
	    ((*ppsTarget)->asWeaps[0].nStat == 0))
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
typedef UDWORD(*TARGET_MASK)(BASE_OBJECT *);

// see if one target is preferable to another
typedef void (*TARGET_PREF)(BASE_OBJECT **, BASE_OBJECT *);

// generic find a target in an area given preference data
static BASE_OBJECT *scrTargetInArea(SDWORD tarPlayer, SDWORD visPlayer, SDWORD tarType, SDWORD cluster,
        SDWORD x1, SDWORD y1, SDWORD x2, SDWORD y2)
{
	BASE_OBJECT		*psTarget, *psCurr;
	SDWORD			temp;
	UDWORD			tarMask;
	TARGET_MASK		getTargetMask;
	TARGET_PREF		targetPriority;
	UDWORD			prefer, ignore;

	if (tarPlayer < 0 || tarPlayer >= MAX_PLAYERS)
	{
		ASSERT(false,
		       "scrTargetInArea: invalid target player number");
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

	// see which target type to use
	switch (tarType)
	{
	case SCR_TAR_STRUCT:
		getTargetMask = (TARGET_MASK)scrStructTargetMask;
		targetPriority = (TARGET_PREF)scrStructTargetPriority;
		prefer = scrStructPref;
		ignore = scrStructIgnore;
		psCurr = apsStructLists[tarPlayer];
		break;
	case SCR_TAR_DROID:
		getTargetMask = (TARGET_MASK)scrDroidTargetMask;
		targetPriority = (TARGET_PREF)scrDroidTargetPriority;
		prefer = scrDroidPref;
		ignore = scrDroidIgnore;
		psCurr = apsDroidLists[tarPlayer];
		break;
	default:
		ASSERT(false, "scrTargetInArea: invalid target type");
		return NULL;
	}

	psTarget = NULL;
	for (; psCurr; psCurr = psCurr->psNext)
	{
		if ((cluster == 0 || psCurr->cluster == cluster) &&
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
				targetPriority(&psTarget, psCurr);
			}
		}
	}


	return psTarget;
}

// get a structure target in an area using the preferences
bool scrStructTargetInArea(void)
{
	SDWORD		x1, y1, x2, y2;
	SDWORD		tarPlayer, visPlayer;

	if (!stackPopParams(6, VAL_INT, &tarPlayer, VAL_INT, &visPlayer,
	        VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	scrFunctionResult.v.oval = (STRUCTURE *)scrTargetInArea(tarPlayer, visPlayer, SCR_TAR_STRUCT, 0, x1, y1, x2, y2);

	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// get a structure target on the map using the preferences
bool scrStructTargetOnMap(void)
{
	SDWORD		tarPlayer, visPlayer;
	STRUCTURE	*psTarget;

	if (!stackPopParams(2, VAL_INT, &tarPlayer, VAL_INT, &visPlayer))
	{
		return false;
	}

	psTarget = (STRUCTURE *)scrTargetInArea(tarPlayer, visPlayer, SCR_TAR_STRUCT, 0,
	        scrollMinX * TILE_UNITS, scrollMinY * TILE_UNITS,
	        scrollMaxX * TILE_UNITS, scrollMaxY * TILE_UNITS);

	scrFunctionResult.v.oval = psTarget;
	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// get a droid target in an area using the preferences
bool scrDroidTargetInArea(void)
{
	SDWORD		x1, y1, x2, y2;
	SDWORD		tarPlayer, visPlayer;
	DROID		*psTarget;

	if (!stackPopParams(6, VAL_INT, &tarPlayer, VAL_INT, &visPlayer,
	        VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return false;
	}

	psTarget = (DROID *)scrTargetInArea(tarPlayer, visPlayer, SCR_TAR_DROID, 0, x1, y1, x2, y2);

	scrFunctionResult.v.oval = psTarget;
	if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// get a droid target on the map using the preferences
bool scrDroidTargetOnMap(void)
{
	SDWORD		tarPlayer, visPlayer;
	DROID		*psTarget;

	if (!stackPopParams(2, VAL_INT, &tarPlayer, VAL_INT, &visPlayer))
	{
		return false;
	}

	psTarget = (DROID *)scrTargetInArea(tarPlayer, visPlayer, SCR_TAR_DROID, 0,
	        scrollMinX * TILE_UNITS, scrollMinY * TILE_UNITS,
	        scrollMaxX * TILE_UNITS, scrollMaxY * TILE_UNITS);

	scrFunctionResult.v.oval = psTarget;
	if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
	{
		return false;
	}

	return true;
}

// get a target from a cluster using the preferences
bool scrTargetInCluster(void)
{
	SDWORD		tarPlayer, tarType, visPlayer, clusterID, cluster;
	BASE_OBJECT	*psTarget;

	if (!stackPopParams(2, VAL_INT, &clusterID, VAL_INT, &visPlayer))
	{
		return false;
	}

	if (clusterID < 0 || clusterID >= CLUSTER_MAX)
	{
		ASSERT(false, "scrTargetInCluster: invalid clusterID");
		return false;
	}

	cluster = aClusterMap[clusterID];
	tarPlayer = aClusterInfo[cluster] & CLUSTER_PLAYER_MASK;
	tarType = (aClusterInfo[cluster] & CLUSTER_DROID) ? SCR_TAR_DROID : SCR_TAR_STRUCT;

	psTarget = scrTargetInArea(tarPlayer, visPlayer, tarType, cluster,
	        scrollMinX * TILE_UNITS, scrollMinY * TILE_UNITS,
	        scrollMaxX * TILE_UNITS, scrollMaxY * TILE_UNITS);

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

bool scrSkCanBuildTemplate(void)
{
	STRUCTURE *psStructure;
	DROID_TEMPLATE *psTempl;

	SDWORD player;

	if (!stackPopParams(3, VAL_INT, &player, ST_STRUCTURE, &psStructure, ST_TEMPLATE, &psTempl))
	{
		return false;
	}

	// is factory big enough?
	if (!validTemplateForFactory(psTempl, psStructure, false))
	{
		goto failTempl;
	}

	if ((asBodyStats + psTempl->asParts[COMP_BODY])->size > ((FACTORY *)psStructure->pFunctionality)->capacity)
	{
		goto failTempl;
	}

	// is every component from template available?
	// body available.
	if (apCompLists[player][COMP_BODY][psTempl->asParts[COMP_BODY]] != AVAILABLE)
	{
		goto failTempl;
	}

	// propulsion method available.
	if (apCompLists[player][COMP_PROPULSION][psTempl->asParts[COMP_PROPULSION]] != AVAILABLE)
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
		if (apCompLists[player][COMP_WEAPON][ psTempl->asWeaps[0] ] != AVAILABLE)
		{
			goto failTempl;
		}
		break;
	case DROID_SENSOR:
		if (apCompLists[player][COMP_SENSOR][psTempl->asParts[COMP_SENSOR]] != AVAILABLE)
		{
			goto failTempl;
		}
		break;
	case DROID_ECM:
		if (apCompLists[player][COMP_ECM][psTempl->asParts[COMP_ECM]] != AVAILABLE)
		{
			goto failTempl;
		}
		break;
	case DROID_REPAIR:
		if (apCompLists[player][COMP_REPAIRUNIT][psTempl->asParts[COMP_REPAIRUNIT]] != AVAILABLE)
		{
			goto failTempl;
		}
		break;
	case DROID_CYBORG_REPAIR:
		if (apCompLists[player][COMP_REPAIRUNIT][psTempl->asParts[COMP_REPAIRUNIT]] != AVAILABLE)
		{
			goto failTempl;
		}
		break;
	case DROID_COMMAND:
		if (apCompLists[player][COMP_BRAIN][psTempl->asParts[COMP_BRAIN]] != AVAILABLE)
		{
			goto failTempl;
		}
		break;
	case DROID_CONSTRUCT:
		if (apCompLists[player][COMP_CONSTRUCT][psTempl->asParts[COMP_CONSTRUCT]] != AVAILABLE)
		{
			goto failTempl;
		}
		break;
	case DROID_CYBORG_CONSTRUCT:
		if (apCompLists[player][COMP_CONSTRUCT][psTempl->asParts[COMP_CONSTRUCT]] != AVAILABLE)
		{
			goto failTempl;
		}
		break;

	case DROID_PERSON:		        // person
	case DROID_TRANSPORTER:	        // guess what this is!
	case DROID_SUPERTRANSPORTER:
	case DROID_DEFAULT:		        // Default droid
	case DROID_ANY:
	default:
		debug(LOG_FATAL, "scrSkCanBuildTemplate: Unhandled template type");
		abort();
		break;
	}

	scrFunctionResult.v.bval = true;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))		// yes
	{
		return false;
	}
	return true;

failTempl:
	scrFunctionResult.v.bval = false;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))		// no
	{
		return false;
	}
	return true;
}

// ********************************************************************************************
// locate the enemy
// gives a target location given a player to attack.
bool scrSkLocateEnemy(void)
{
	SDWORD		player;//,*x,*y;
	STRUCTURE	*psStruct;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	// find where the player has some structures..	// factories or hq.
	for (psStruct = apsStructLists[player];
	     psStruct &&
	     !(psStruct->pStructureType->type == REF_HQ
	       || psStruct->pStructureType->type == REF_FACTORY
	       || psStruct->pStructureType->type == REF_CYBORG_FACTORY
	       || psStruct->pStructureType->type == REF_VTOL_FACTORY
	      );
	     psStruct = psStruct->psNext) {}

	// set the x and y accordingly..
	if (psStruct)
	{
		scrFunctionResult.v.oval = psStruct;
		if (!stackPushResult((INTERP_TYPE)ST_BASEOBJECT, &scrFunctionResult))		// success!
		{
			return false;
		}
	}
	else
	{
		scrFunctionResult.v.oval = NULL;
		if (!stackPushResult((INTERP_TYPE)ST_BASEOBJECT, &scrFunctionResult))		// part success
		{
			return false;
		}
	}
	return true;
}

// ********************************************************************************************

bool skTopicAvail(UWORD inc, UDWORD player)
{
	UDWORD				incPR, incS;
	bool				bPRFound, bStructFound;


	//if the topic is possible and has not already been researched - add to list
	if ((IsResearchPossible(&asPlayerResList[player][inc])))
	{
		if (!IsResearchCompleted(&asPlayerResList[player][inc])
		    && !IsResearchStartedPending(&asPlayerResList[player][inc]))
		{
			return true;
		}
	}

	// make sure that the research is not completed  or started by another researchfac
	if (!IsResearchCompleted(&asPlayerResList[player][inc])
	    && !IsResearchStartedPending(&asPlayerResList[player][inc]))
	{
		// Research is not completed  ... also  it has not been started by another researchfac

		//if there aren't any PR's - go to next topic
		if (asResearch[inc].pPRList.empty())
		{
			return false;
		}

		//check for pre-requisites
		bPRFound = true;
		for (incPR = 0; incPR < asResearch[inc].pPRList.size(); incPR++)
		{
			if (IsResearchCompleted(&asPlayerResList[player][asResearch[inc].pPRList[incPR]]) == 0)
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
		for (incS = 0; incS < asResearch[inc].pStructList.size(); incS++)
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
bool scrSkDoResearch(void)
{
	SDWORD				player, bias;
	UWORD				i;
	STRUCTURE			*psBuilding;
	RESEARCH_FACILITY	*psResFacilty;

	if (!stackPopParams(3, ST_STRUCTURE, &psBuilding, VAL_INT, &player, VAL_INT, &bias))
	{
		return false;
	}

	psResFacilty =	(RESEARCH_FACILITY *)psBuilding->pFunctionality;

	if (psResFacilty->psSubject != NULL)
	{
		// not finshed yet..
		return true;
	}

	// choose a topic to complete.
	for (i = 0; i < asResearch.size(); i++)
	{
		if (skTopicAvail(i, player) && (!bMultiPlayer || !beingResearchedByAlly(i, player)))
		{
			break;
		}
	}

	if (i != asResearch.size())
	{
		sendResearchStatus(psBuilding, i, player, true);			// inform others, I'm researching this.
#if defined (DEBUG)
		{
			char	sTemp[128];
			sprintf(sTemp, "[debug]player:%d starts topic: %s", player, asResearch[i].pName);
			NETlogEntry(sTemp, SYNC_FLAG, 0);
		}
#endif
	}

	return true;
}

// ********************************************************************************************
bool scrSkVtolEnableCheck(void)
{
	SDWORD player;
	UDWORD i;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}


	// vtol factory
	for (i = 0; (i < numStructureStats) && (asStructureStats[i].type != REF_VTOL_FACTORY); i++) {}
	if ((i < numStructureStats) && (apStructTypeLists[player][i] == AVAILABLE))
	{
		// vtol propulsion
		for (i = 0; (i < numPropulsionStats); i++)
		{
			if ((asPropulsionStats[i].propulsionType == PROPULSION_TYPE_LIFT)
			    && apCompLists[player][COMP_PROPULSION][i] == AVAILABLE)
			{
				scrFunctionResult.v.bval = true;
				if (!stackPushResult(VAL_BOOL, &scrFunctionResult))		// success!
				{
					return false;
				}
				return true;
			}
		}
	}

	scrFunctionResult.v.bval = false;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))		// success!
	{
		return false;
	}
	return true;
}

// ********************************************************************************************
bool scrSkGetFactoryCapacity(void)
{
	SDWORD count = 0;
	STRUCTURE *psStructure;

	if (!stackPopParams(1, ST_STRUCTURE, &psStructure))
	{
		return false;
	}

	if (psStructure && StructIsFactory(psStructure))
	{
		count = ((FACTORY *)psStructure->pFunctionality)->capacity;
	}

	scrFunctionResult.v.ival = count;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return false;
	}
	return true;
}
// ********************************************************************************************
bool scrSkDifficultyModifier(void)
{
	int 			player;
	RESEARCH_FACILITY	*psResFacility;
	STRUCTURE		*psStr;
	PLAYER_RESEARCH		*pPlayerRes;

	if (!stackPopParams(1, VAL_INT, &player))
	{
		return false;
	}

	/* Skip cheats if difficulty modifier slider is set to minimum or this is a true multiplayer game.
	 * (0 - player disabled, 20 - max value, UBYTE_MAX - autogame)
	 */
	if (game.skDiff[player] <= 1 || game.skDiff[player] == UBYTE_MAX || NetPlay.bComms)
	{
		return true;
	}

	// power modifier, range: 0-1000
	giftPower(ANYPLAYER, player, game.skDiff[player] * 50, true);

	//research modifier
	for (psStr = apsStructLists[player]; psStr; psStr = psStr->psNext)
	{
		if (psStr->pStructureType->type == REF_RESEARCH)
		{
			psResFacility =	(RESEARCH_FACILITY *)psStr->pFunctionality;

			// subtract 0 - 80% off the time to research.
			if (psResFacility->psSubject)
			{
				RESEARCH	*psResearch = (RESEARCH *)psResFacility->psSubject;

				pPlayerRes = &asPlayerResList[player][psResearch->ref - REF_RESEARCH_START];
				pPlayerRes->currentPoints += (psResearch->researchPoints * 4 * game.skDiff[player]) / 100;
				syncDebug("AI %d is cheating with research time.", player);
			}
		}

	}

	return true;
}

// ********************************************************************************************

// not a direct script function but a helper for scrSkDefenseLocation and scrSkDefenseLocationB
static bool defenseLocation(bool variantB)
{
	SDWORD		*pX, *pY, statIndex, statIndex2;
	UDWORD		x, y, gX, gY, dist, player, nearestSoFar, count;
	GATEWAY		*psGate, *psChosenGate;
	DROID		*psDroid;
	UDWORD		x1, x2, x3, x4, y1, y2, y3, y4;
	bool		noWater;
	UDWORD      minCount;
	UDWORD      offset;

	if (!stackPopParams(6,
	        VAL_REF | VAL_INT, &pX,
	        VAL_REF | VAL_INT, &pY,
	        ST_STRUCTURESTAT, &statIndex,
	        ST_STRUCTURESTAT, &statIndex2,
	        ST_DROID, &psDroid,
	        VAL_INT, &player))
	{
		debug(LOG_ERROR, "defenseLocation: failed to pop");
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT(false, "defenseLocation:player number is too high");
		return false;
	}

	ASSERT_OR_RETURN(false, statIndex < numStructureStats, "Invalid range referenced for numStructureStats, %d > %d", statIndex, numStructureStats);

	ASSERT_OR_RETURN(false, statIndex2 < numStructureStats, "Invalid range referenced for numStructureStats, %d > %d", statIndex2, numStructureStats);
	STRUCTURE_STATS *psWStats = (asStructureStats + statIndex2);

	// check for wacky coords.
	if (*pX < 0
	    ||	*pX > world_coord(mapWidth)
	    ||	*pY < 0
	    ||	*pY > world_coord(mapHeight)
	   )
	{
		goto failed;
	}

	x = map_coord(*pX);					// change to tile coords.
	y = map_coord(*pY);

	// go down the gateways, find the nearest gateway with >1 empty tiles
	nearestSoFar = UDWORD_MAX;
	psChosenGate = NULL;
	for (psGate = gwGetGateways(); psGate; psGate = psGate->psNext)
	{
		if (auxTile(psGate->x1, psGate->y1, player) & AUXBITS_THREAT)
		{
			continue;	// enemy can shoot there, not safe to build
		}
		count = 0;
		noWater = true;
		// does it have >1 tile unoccupied.
		if (psGate->x1 == psGate->x2)
		{
			// vert
			//skip gates that are too short
			if (variantB && (psGate->y2 - psGate->y1) <= 2)
			{
				continue;
			}
			gX = psGate->x1;
			for (gY = psGate->y1; gY <= psGate->y2; gY++)
			{
				if (! TileIsOccupied(mapTile(gX, gY)))
				{
					count++;
				}
				if (terrainType(mapTile(gX, gY)) == TER_WATER)
				{
					noWater = false;
				}
			}
		}
		else
		{
			// horiz
			//skip gates that are too short
			if (variantB && (psGate->x2 - psGate->x1) <= 2)
			{
				continue;
			}
			gY = psGate->y1;
			for (gX = psGate->x1; gX <= psGate->x2; gX++)
			{
				if (! TileIsOccupied(mapTile(gX, gY)))
				{
					count++;
				}
				if (terrainType(mapTile(gX, gY)) == TER_WATER)
				{
					noWater = false;
				}
			}
		}
		if (variantB)
		{
			minCount = 2;
		}
		else
		{
			minCount = 1;
		}
		if (count > minCount && noWater)	//<NEW> min 2 tiles
		{
			// ok it's free. Is it the nearest one yet?
			/* Get gateway midpoint */
			gX = (psGate->x1 + psGate->x2) / 2;
			gY = (psGate->y1 + psGate->y2) / 2;
			/* Estimate the distance to it */
			dist = iHypot(x - gX, y - gY);
			/* Is it best we've found? */
			if (dist < nearestSoFar && dist < 30)
			{
				/* Yes, then keep a record of it */
				nearestSoFar = dist;
				psChosenGate = psGate;
			}
		}
	}

	if (!psChosenGate)	// we have a gateway.
	{
		goto failed;
	}

	// find an unnocupied tile on that gateway.
	if (psChosenGate->x1 == psChosenGate->x2)
	{
		// vert
		gX = psChosenGate->x1;
		for (gY = psChosenGate->y1; gY <= psChosenGate->y2; gY++)
		{
			if (! TileIsOccupied(mapTile(gX, gY)))
			{
				y = gY;
				x = gX;
				break;
			}
		}
	}
	else
	{
		// horiz
		gY = psChosenGate->y1;
		for (gX = psChosenGate->x1; gX <= psChosenGate->x2; gX++)
		{
			if (! TileIsOccupied(mapTile(gX, gY)))
			{
				y = gY;
				x = gX;
				break;
			}
		}
	}

	// back to world coords and store result.
	*pX = world_coord(x) + (TILE_UNITS / 2);		// return centre of tile.
	*pY = world_coord(y) + (TILE_UNITS / 2);

	scrFunctionResult.v.bval = true;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))		// success
	{
		return false;
	}


	// order the droid to build two walls, one either side of the gateway.
	// or one in the case of a 2 size gateway.

	//find center of the gateway
	x = (psChosenGate->x1 + psChosenGate->x2) / 2;
	y = (psChosenGate->y1 + psChosenGate->y2) / 2;

	//find start pos of the gateway
	x1 = world_coord(psChosenGate->x1) + (TILE_UNITS / 2);
	y1 = world_coord(psChosenGate->y1) + (TILE_UNITS / 2);

	if (variantB)
	{
		offset = 2;
	}
	else
	{
		offset = 1;
	}
	if (psChosenGate->x1 == psChosenGate->x2)	//vert
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
		x3 = world_coord(x + offset) + TILE_UNITS / 2;
		y3 = y1;

	}
	//end coords of the second section
	x4 = world_coord(psChosenGate->x2) + TILE_UNITS / 2;
	y4 = world_coord(psChosenGate->y2) + TILE_UNITS / 2;

	// first section.
	if (x1 == x2 && y1 == y2)	//first sec is 1 tile only: ((2 tile gate) or (3 tile gate and first sec))
	{
		orderDroidStatsLocDir(psDroid, DORDER_BUILD, psWStats, x1, y1, 0, ModeQueue);
	}
	else
	{
		orderDroidStatsTwoLocDir(psDroid, DORDER_LINEBUILD, psWStats,  x1, y1, x2, y2, 0, ModeQueue);
	}

	// second section
	if (x3 == x4 && y3 == y4)
	{
		orderDroidStatsLocDirAdd(psDroid, DORDER_BUILD, psWStats, x3, y3, 0);
	}
	else
	{
		orderDroidStatsTwoLocDirAdd(psDroid, DORDER_LINEBUILD, psWStats,  x3, y3, x4, y4, 0);
	}

	return true;

failed:
	scrFunctionResult.v.bval = false;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))		// failed!
	{
		return false;
	}
	return true;
}


// return a good place to build a defence, given a starting point
bool scrSkDefenseLocation(void)
{
	return defenseLocation(false);
}

// return a good place to build a defence with a min number of clear tiles
bool scrSkDefenseLocationB(void)
{
	return defenseLocation(true);
}


bool scrSkFireLassat(void)
{
	SDWORD	player;
	BASE_OBJECT *psObj;

	if (!stackPopParams(2,  VAL_INT, &player, ST_BASEOBJECT, &psObj))
	{
		return false;
	}

	if (psObj)
	{
		orderStructureObj(player, psObj);
	}

	return true;
}

//-----------------------
// New functions
//-----------------------
bool scrActionDroidObj(void)
{
	DROID			*psDroid;
	DROID_ACTION		action;
	BASE_OBJECT		*psObj;

	if (!stackPopParams(3, ST_DROID, &psDroid, VAL_INT, &action, ST_BASEOBJECT, &psObj))
	{
		debug(LOG_ERROR, "scrActionDroidObj: failed to pop");
		return false;
	}

	ASSERT(psDroid != NULL,
	       "scrOrderUnitObj: Invalid unit pointer");
	ASSERT(psObj != NULL,
	       "scrOrderUnitObj: Invalid object pointer");

	if (psDroid == NULL || psObj == NULL)
	{
		return false;
	}

	if (action != DACTION_DROIDREPAIR)
	{
		debug(LOG_ERROR, "scrActionDroidObj: this action is not supported");
		return false;
	}

	syncDebug("TODO: Synchronise this!");
	actionDroid(psDroid, action, psObj);

	return true;
}

//<script function - improved version
// variables for the group iterator
static DROID_GROUP *psScrIterateGroupB[MAX_PLAYERS];
static DROID *psScrIterateGroupDroidB[MAX_PLAYERS];

// initialise iterating a groups members
bool scrInitIterateGroupB(void)
{
	DROID_GROUP	*psGroup;
	SDWORD		bucket;

	if (!stackPopParams(2, ST_GROUP, &psGroup, VAL_INT, &bucket))
	{
		debug(LOG_ERROR, "scrInitIterateGroupB: stackPopParams failed");
		return false;
	}

	ASSERT(psGroup != NULL,
	       "scrInitIterateGroupB: invalid group pointer");

	ASSERT(bucket < MAX_PLAYERS,
	       "scrInitIterateGroupB: invalid bucket");

	psScrIterateGroupB[bucket] = psGroup;
	psScrIterateGroupDroidB[bucket] = psGroup->psList;

	return true;
}

//script function - improved version
// iterate through a groups members
bool scrIterateGroupB(void)
{
	DROID_GROUP	*psGroup;
	DROID		*psDroid;
	SDWORD		bucket;

	if (!stackPopParams(2, ST_GROUP, &psGroup, VAL_INT, &bucket))
	{
		debug(LOG_ERROR, "scrIterateGroupB: stackPopParams failed");
		return false;
	}

	ASSERT(bucket < MAX_PLAYERS,
	       "scrIterateGroupB: invalid bucket");

	if (psGroup != psScrIterateGroupB[bucket])
	{
		ASSERT(false, "scrIterateGroupB: invalid group, InitGroupIterateB not called?");
		return false;
	}

	if (psScrIterateGroupDroidB[bucket] != NULL)
	{
		psDroid = psScrIterateGroupDroidB[bucket];
		psScrIterateGroupDroidB[bucket] = psScrIterateGroupDroidB[bucket]->psGrpNext;
	}
	else
	{
		psDroid = NULL;
	}

	scrFunctionResult.v.oval = psDroid;
	if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
	{
		debug(LOG_ERROR, "scrIterateGroupB: stackPushResult failed");
		return false;
	}

	return true;
}

bool scrDroidCanReach(void)
{
	DROID			*psDroid;
	int			x, y;

	if (!stackPopParams(3, ST_DROID, &psDroid, VAL_INT, &x, VAL_INT, &y))
	{
		debug(LOG_ERROR, "Failed to pop parameters");
		return false;
	}
	if (psDroid)
	{
		const PROPULSION_STATS *psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
		const Vector3i rPos(x, y, 0);

		scrFunctionResult.v.bval = fpathCheck(psDroid->pos, rPos, psPropStats->propulsionType);
		if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
		{
			debug(LOG_ERROR, "stackPushResult failed");
			return false;
		}
		return true;
	}
	return false;
}

// ********************************************************************************************
// ********************************************************************************************
