/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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

static INTERP_VAL	scrFunctionResult;	//function return value to be pushed to stack

// Add a droid to a group
BOOL scrGroupAddDroid(void)
{
	DROID_GROUP		*psGroup;
	DROID			*psDroid;

	if (!stackPopParams(2, ST_GROUP, &psGroup, ST_DROID, &psDroid))
	{
		return FALSE;
	}

	ASSERT( psGroup != NULL,
		"scrGroupAdd: Invalid group pointer" );
	ASSERT( psDroid != NULL,
		"scrGroupAdd: Invalid droid pointer" );
	if (psDroid == NULL)
	{
		return FALSE;
	}
	if (psDroid->droidType == DROID_COMMAND)
	{
		debug( LOG_ERROR,
			"scrGroupAdd: cannot add a command droid to a group" );
		return FALSE;
	}
	if (psDroid->droidType == DROID_TRANSPORTER)
	{
		debug( LOG_ERROR,
			"scrGroupAdd: cannot add a transporter to a group" );
		return FALSE;
	}

	grpJoin(psGroup, psDroid);

	return TRUE;
}


// Add droids in an area to a group
BOOL scrGroupAddArea(void)
{
	DROID_GROUP		*psGroup;
	DROID			*psDroid;
	SDWORD			x1,y1,x2,y2, player;

	if (!stackPopParams(6, ST_GROUP, &psGroup, VAL_INT, &player,
							VAL_INT,&x1,VAL_INT,&y1, VAL_INT,&x2,VAL_INT,&y2))
	{
		return FALSE;
	}

	ASSERT( psGroup != NULL,
		"scrGroupAdd: Invalid group pointer" );

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( FALSE, "scrGroupAddArea: invalid player" );
		return FALSE;
	}

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

	return TRUE;
}


// Add groupless droids in an area to a group
BOOL scrGroupAddAreaNoGroup(void)
{
	DROID_GROUP		*psGroup;
	DROID			*psDroid;
	SDWORD			x1,y1,x2,y2, player;

	if (!stackPopParams(6, ST_GROUP, &psGroup, VAL_INT, &player,
							VAL_INT,&x1,VAL_INT,&y1, VAL_INT,&x2,VAL_INT,&y2))
	{
		return FALSE;
	}

	ASSERT( psGroup != NULL,
		"scrGroupAddNoGroup: Invalid group pointer" );

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( FALSE, "scrGroupAddAreaNoGroup: invalid player" );
		return FALSE;
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

	return TRUE;
}


// Move the droids from one group to another
BOOL scrGroupAddGroup(void)
{
	DROID_GROUP		*psTo, *psFrom;
	DROID			*psDroid, *psNext;

	if (!stackPopParams(2, ST_GROUP, &psTo, ST_GROUP, &psFrom))
	{
		return FALSE;
	}

	ASSERT( psTo != NULL,
		"scrGroupAddGroup: Invalid group pointer" );
	ASSERT( psFrom != NULL,
		"scrGroupAddGroup: Invalid group pointer" );

	for(psDroid=psFrom->psList; psDroid; psDroid=psNext)
	{
		psNext = psDroid->psGrpNext;
		grpJoin(psTo, psDroid);
	}

	return TRUE;
}


// check if a droid is a member of a group
BOOL scrGroupMember(void)
{
	DROID_GROUP		*psGroup;
	DROID			*psDroid;
	BOOL			retval;

	if (!stackPopParams(2, ST_GROUP, &psGroup, ST_DROID, &psDroid))
	{
		return FALSE;
	}

	ASSERT( psGroup != NULL,
		"scrGroupMember: Invalid group pointer" );
	ASSERT( psDroid != NULL,
		"scrGroupMember: Invalid droid pointer" );
	if (psDroid == NULL)
	{
		return FALSE;
	}

	if (psDroid->psGroup == psGroup)
	{
		retval = TRUE;
	}
	else
	{
		retval = FALSE;
	}

	scrFunctionResult.v.bval=retval;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}


// returns number of idle droids in a group.
BOOL scrIdleGroup(void)
{
	DROID_GROUP *psGroup;
	DROID		*psDroid;
	UDWORD		count=0;

	if (!stackPopParams(1, ST_GROUP, &psGroup))
	{
		return FALSE;
	}

	ASSERT( psGroup != NULL,
		"scrIdleGroup: invalid group pointer" );

	for(psDroid = psGroup->psList;psDroid; psDroid = psDroid->psGrpNext)
	{
		if(  psDroid->order == DORDER_NONE
		  || (psDroid->order == DORDER_GUARD && psDroid->psTarget == NULL))
		{
			count++;
		}
	}

	scrFunctionResult.v.ival = (SDWORD)count;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return FALSE;
	}
	return TRUE;
}

// variables for the group iterator
static DROID_GROUP		*psScrIterateGroup;
static DROID			*psScrIterateGroupDroid;

// initialise iterating a groups members
BOOL scrInitIterateGroup(void)
{
	DROID_GROUP	*psGroup;

	if (!stackPopParams(1, ST_GROUP, &psGroup))
	{
		return FALSE;
	}

	ASSERT( psGroup != NULL,
		"scrInitGroupIterate: invalid group pointer" );

	psScrIterateGroup = psGroup;
	psScrIterateGroupDroid = psGroup->psList;

	return TRUE;
}


// iterate through a groups members
BOOL scrIterateGroup(void)
{
	DROID_GROUP	*psGroup;
	DROID		*psDroid;

	if (!stackPopParams(1, ST_GROUP, &psGroup))
	{
		return FALSE;
	}

	if (psGroup != psScrIterateGroup)
	{
		debug(LOG_ERROR, "scrIterateGroup: invalid group, InitGroupIterate not called?" );
		return FALSE;
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
		return FALSE;
	}

	return TRUE;
}


// initialise iterating a cluster
BOOL scrInitIterateCluster(void)
{
	SDWORD	clusterID;

	if (!stackPopParams(1, VAL_INT, &clusterID))
	{
		return FALSE;
	}

	clustInitIterate(clusterID);

	return TRUE;
}


// iterate a cluster
BOOL scrIterateCluster(void)
{
	BASE_OBJECT		*psObj;

	psObj = clustIterate();

	scrFunctionResult.v.oval = psObj;
	if (!stackPushResult((INTERP_TYPE)ST_BASEOBJECT, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}


// remove a droid from a group
BOOL scrDroidLeaveGroup(void)
{
	DROID			*psDroid;

	if (!stackPopParams(1, ST_DROID, &psDroid))
	{
		return FALSE;
	}

	if (psDroid->psGroup != NULL)
	{
		grpLeave(psDroid->psGroup, psDroid);
	}

	return TRUE;
}


// Give a group an order
BOOL scrOrderGroup(void)
{
	DROID_GROUP		*psGroup;
	DROID_ORDER		order;

	if (!stackPopParams(2, ST_GROUP, &psGroup, VAL_INT, &order))
	{
		return FALSE;
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
		ASSERT( FALSE,
			"scrOrderGroup: Invalid order" );
		return FALSE;
	}

	debug( LOG_NEVER, "scrOrderGroup: group %p (%d) order %d\n", psGroup, grpNumMembers(psGroup), order);
	orderGroup(psGroup, order);

	return TRUE;
}


// Give a group an order to a location
BOOL scrOrderGroupLoc(void)
{
	DROID_GROUP		*psGroup;
	DROID_ORDER		order;
	SDWORD			x,y;

	if (!stackPopParams(4, ST_GROUP, &psGroup, VAL_INT, &order, VAL_INT, &x, VAL_INT, &y))
	{
		return FALSE;
	}

	ASSERT( psGroup != NULL,
		"scrOrderGroupLoc: Invalid group pointer" );

	if (order != DORDER_MOVE &&
		order != DORDER_SCOUT)
	{
		ASSERT( FALSE,
			"scrOrderGroupLoc: Invalid order" );
		return FALSE;
	}
	if (x < 0
	 || x > world_coord(mapWidth)
	 || y < 0
	 || y > world_coord(mapHeight))
	{
		ASSERT( FALSE,
			"scrOrderGroupLoc: Invalid location" );
		return FALSE;
	}

	debug( LOG_NEVER, "scrOrderGroupLoc: group %p (%d) order %d (%d,%d)\n",
		psGroup, grpNumMembers(psGroup), order, x,y);
	orderGroupLoc(psGroup, order, (UDWORD)x,(UDWORD)y);

	return TRUE;
}


// Give a group an order to an object
BOOL scrOrderGroupObj(void)
{
	DROID_GROUP		*psGroup;
	DROID_ORDER		order;
	BASE_OBJECT		*psObj;

	if (!stackPopParams(3, ST_GROUP, &psGroup, VAL_INT, &order, ST_BASEOBJECT, &psObj))
	{
		return FALSE;
	}

	ASSERT( psGroup != NULL,
		"scrOrderGroupObj: Invalid group pointer" );
	ASSERT( psObj != NULL,
		"scrOrderGroupObj: Invalid object pointer" );

	if (order != DORDER_ATTACK &&
		order != DORDER_HELPBUILD &&
		order != DORDER_DEMOLISH &&
		order != DORDER_REPAIR &&
		order != DORDER_OBSERVE &&
		order != DORDER_EMBARK &&
		order != DORDER_FIRESUPPORT &&
		order != DORDER_DROIDREPAIR)
	{
		ASSERT( FALSE,
			"scrOrderGroupObj: Invalid order" );
		return FALSE;
	}

	debug( LOG_NEVER, "scrOrderGroupObj: group %p (%d) order %d,  obj type %d player %d id %d\n",
		psGroup, grpNumMembers(psGroup), order, psObj->type, psObj->player, psObj->id);

	orderGroupObj(psGroup, order, psObj);

	return TRUE;
}

// Give a droid an order
BOOL scrOrderDroid(void)
{
	DROID			*psDroid;
	DROID_ORDER		order;

	if (!stackPopParams(2, ST_DROID, &psDroid, VAL_INT, &order))
	{
		return FALSE;
	}

	ASSERT( psDroid != NULL,
		"scrOrderUnit: Invalid unit pointer" );
	if (psDroid == NULL)
	{
		return FALSE;
	}

	if (order != DORDER_STOP &&
		order != DORDER_RETREAT &&
		order != DORDER_DESTRUCT &&
		order != DORDER_RTR &&
		order != DORDER_RTB &&
		order != DORDER_RUN)
	{
		ASSERT( FALSE,
			"scrOrderUnit: Invalid order" );
		return FALSE;
	}

	orderDroid(psDroid, order);

	return TRUE;
}


// Give a Droid an order to a location
BOOL scrOrderDroidLoc(void)
{
	DROID			*psDroid;
	DROID_ORDER		order;
	SDWORD			x,y;

	if (!stackPopParams(4, ST_DROID, &psDroid, VAL_INT, &order, VAL_INT, &x, VAL_INT, &y))
	{
		return FALSE;
	}

	ASSERT( psDroid != NULL,
		"scrOrderUnitLoc: Invalid unit pointer" );
	if (psDroid == NULL)
	{
		return FALSE;
	}

	if (order != DORDER_MOVE &&
		order != DORDER_SCOUT)
	{
		ASSERT( FALSE,
			"scrOrderUnitLoc: Invalid order" );
		return FALSE;
	}
	if (x < 0
	 || x > world_coord(mapWidth)
	 || y < 0
	 || y > world_coord(mapHeight))
	{
		ASSERT( FALSE,
			"scrOrderUnitLoc: Invalid location" );
		return FALSE;
	}

	orderDroidLoc(psDroid, order, (UDWORD)x,(UDWORD)y);

	return TRUE;
}


// Give a Droid an order to an object
BOOL scrOrderDroidObj(void)
{
	DROID			*psDroid;
	DROID_ORDER		order;
	BASE_OBJECT		*psObj;

	if (!stackPopParams(3, ST_DROID, &psDroid, VAL_INT, &order, ST_BASEOBJECT, &psObj))
	{
		return FALSE;
	}

	ASSERT( psDroid != NULL,
		"scrOrderUnitObj: Invalid unit pointer" );
	ASSERT( psObj != NULL,
		"scrOrderUnitObj: Invalid object pointer" );
	if (psDroid == NULL || psObj == NULL)
	{
		return FALSE;
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
		ASSERT( FALSE,
			"scrOrderUnitObj: Invalid order" );
		return FALSE;
	}

	orderDroidObj(psDroid, order, psObj);

	return TRUE;
}

// Give a Droid an order with a stat
BOOL scrOrderDroidStatsLoc(void)
{
	DROID			*psDroid;
	DROID_ORDER		order;
	SDWORD			x,y, statIndex;
	BASE_STATS		*psStats;

	if (!stackPopParams(5, ST_DROID, &psDroid, VAL_INT, &order, ST_STRUCTURESTAT, &statIndex,
						   VAL_INT, &x, VAL_INT, &y))
	{
		return FALSE;
	}

	if (statIndex < 0 || statIndex >= (SDWORD)numStructureStats)
	{
		ASSERT( FALSE,
			"scrOrderUnitStatsLoc: invalid structure stat" );
		return FALSE;
	}
	psStats = (BASE_STATS *)(asStructureStats + statIndex);

	ASSERT( psDroid != NULL,
		"scrOrderUnitStatsLoc: Invalid Unit pointer" );
	ASSERT( psStats != NULL,
		"scrOrderUnitStatsLoc: Invalid object pointer" );
	if (psDroid == NULL)
	{
		return FALSE;
	}

	if ((x < 0) || (x > (SDWORD)mapWidth*TILE_UNITS) ||
		(y < 0) || (y > (SDWORD)mapHeight*TILE_UNITS))
	{
		ASSERT( FALSE,
			"scrOrderUnitStatsLoc: Invalid location" );
		return FALSE;
	}

	if (order != DORDER_BUILD)
	{
		ASSERT( FALSE,
			"scrOrderUnitStatsLoc: Invalid order" );
		return FALSE;
	}

	// Don't allow scripts to order structure builds if players structure
	// limit has been reached.
	if(IsPlayerStructureLimitReached(psDroid->player) == FALSE) {
		orderDroidStatsLoc(psDroid, order, psStats, (UDWORD)x,(UDWORD)y);
	}

	return TRUE;
}


// set the secondary state for a droid
BOOL scrSetDroidSecondary(void)
{
	DROID		*psDroid;
	SECONDARY_ORDER	sec;
	SECONDARY_STATE	state;

	if (!stackPopParams(3, ST_DROID, &psDroid, VAL_INT, &sec, VAL_INT, &state))
	{
		return FALSE;
	}

	ASSERT( psDroid != NULL,
		"scrSetUnitSecondary: invalid unit pointer" );
	if (psDroid == NULL)
	{
		return FALSE;
	}

	secondarySetState(psDroid, sec, state);

	return TRUE;
}

// set the secondary state for a droid
BOOL scrSetGroupSecondary(void)
{
	DROID_GROUP		*psGroup;
	SECONDARY_ORDER		sec;
	SECONDARY_STATE		state;

	if (!stackPopParams(3, ST_GROUP, &psGroup, VAL_INT, &sec, VAL_INT, &state))
	{
		return FALSE;
	}

	ASSERT( psGroup != NULL,
		"scrSetGroupSecondary: invalid group pointer" );

	grpSetSecondary(psGroup, sec, state);

	return TRUE;
}


// add a droid to a commander
BOOL scrCmdDroidAddDroid(void)
{
	DROID		*psDroid, *psCommander;

	if (!stackPopParams(2, ST_DROID, &psCommander, ST_DROID, &psDroid))
	{
		return FALSE;
	}

	cmdDroidAddDroid(psCommander, psDroid);

	return TRUE;
}

// returns max number of droids in a commander group
BOOL scrCmdDroidMaxGroup(void)
{
	DROID		*psCommander;

	if (!stackPopParams(1, ST_DROID, &psCommander))
	{
		return FALSE;
	}

	ASSERT(psCommander != NULL,
		"scrCmdDroidMaxGroup: NULL pointer passed");

	scrFunctionResult.v.ival = cmdDroidMaxGroup(psCommander);
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

// store the prefered and ignore targets for the target functions
UDWORD	scrStructPref, scrStructIgnore;
UDWORD	scrDroidPref, scrDroidIgnore;


// reset the structure preferences
BOOL scrResetStructTargets(void)
{
	scrStructPref = 0;
	scrStructIgnore = 0;

	return TRUE;
}


// reset the droid preferences
BOOL scrResetDroidTargets(void)
{
	scrDroidPref = 0;
	scrDroidIgnore = 0;

	return TRUE;
}


// set prefered structure target types
BOOL scrSetStructTarPref(void)
{
	UDWORD	pref;

	if (!stackPopParams(1, VAL_INT, &pref))
	{
		return FALSE;
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

	return TRUE;
}


// set structure target ignore types
BOOL scrSetStructTarIgnore(void)
{
	UDWORD	pref;

	if (!stackPopParams(1, VAL_INT, &pref))
	{
		return FALSE;
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

	return TRUE;
}


// set prefered droid target types
BOOL scrSetDroidTarPref(void)
{
	UDWORD	pref;

	if (!stackPopParams(1, VAL_INT, &pref))
	{
		return FALSE;
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

	return TRUE;
}

// set droid target ignore types
BOOL scrSetDroidTarIgnore(void)
{
	UDWORD	pref;

	if (!stackPopParams(1, VAL_INT, &pref))
	{
		return FALSE;
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

	return TRUE;
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
	default:
		ASSERT( FALSE,
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
		ASSERT( FALSE,
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
		ASSERT( FALSE,
			"scrUnitTargetMask: unknown or invalid target body size" );
		break;
	}

	// get the propulsion type
	psPStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	switch(psPStats->propulsionType)
	{
	case WHEELED:
		mask |= SCR_DT_WHEEL;
		break;
	case TRACKED:
		mask |= SCR_DT_TRACK;
		break;
	case LEGGED:
	case JUMP:
		mask |= SCR_DT_LEGS;
		break;
	case HOVER:
		mask |= SCR_DT_HOVER;
		break;
	case LIFT:
		mask |= SCR_DT_VTOL;
		break;
	case HALF_TRACKED:
		mask |= SCR_DT_HTRACK;
		break;
	case PROPELLOR:
	case SKI:
	default:
		ASSERT( FALSE,
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
		ASSERT( FALSE,
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
		bVisCheck = FALSE;
	}
	else
	{
		bVisCheck = TRUE;
	}
		bVisCheck = FALSE;

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
		ASSERT( FALSE, "scrTargetInArea: invalid target type" );
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
BOOL scrStructTargetInArea(void)
{
	SDWORD		x1,y1,x2,y2;
	SDWORD		tarPlayer, visPlayer;

	if (!stackPopParams(6, VAL_INT, &tarPlayer, VAL_INT, &visPlayer,
						   VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return FALSE;
	}

	scrFunctionResult.v.oval = (STRUCTURE *)scrTargetInArea(tarPlayer, visPlayer, SCR_TAR_STRUCT, 0, x1,y1, x2,y2);

	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

// get a structure target on the map using the preferences
BOOL scrStructTargetOnMap(void)
{
	SDWORD		tarPlayer, visPlayer;
	STRUCTURE	*psTarget;

	if (!stackPopParams(2, VAL_INT, &tarPlayer, VAL_INT, &visPlayer))
	{
		return FALSE;
	}

	psTarget = (STRUCTURE *)scrTargetInArea(tarPlayer, visPlayer, SCR_TAR_STRUCT, 0,
									scrollMinX*TILE_UNITS,scrollMinY*TILE_UNITS,
									scrollMaxX*TILE_UNITS,scrollMaxY*TILE_UNITS);

	scrFunctionResult.v.oval = psTarget;
	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

// get a droid target in an area using the preferences
BOOL scrDroidTargetInArea(void)
{
	SDWORD		x1,y1,x2,y2;
	SDWORD		tarPlayer, visPlayer;
	DROID		*psTarget;

	if (!stackPopParams(6, VAL_INT, &tarPlayer, VAL_INT, &visPlayer,
						   VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return FALSE;
	}

	psTarget = (DROID *)scrTargetInArea(tarPlayer, visPlayer, SCR_TAR_DROID, 0, x1,y1, x2,y2);

	scrFunctionResult.v.oval = psTarget;
	if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

// get a droid target on the map using the preferences
BOOL scrDroidTargetOnMap(void)
{
	SDWORD		tarPlayer, visPlayer;
	DROID		*psTarget;

	if (!stackPopParams(2, VAL_INT, &tarPlayer, VAL_INT, &visPlayer))
	{
		return FALSE;
	}

	psTarget = (DROID *)scrTargetInArea(tarPlayer, visPlayer, SCR_TAR_DROID, 0,
							scrollMinX*TILE_UNITS,scrollMinY*TILE_UNITS,
							scrollMaxX*TILE_UNITS,scrollMaxY*TILE_UNITS);

	scrFunctionResult.v.oval = psTarget;
	if (!stackPushResult((INTERP_TYPE)ST_DROID, &scrFunctionResult))
	{
		return FALSE;
	}

	return TRUE;
}

// get a target from a cluster using the preferences
BOOL scrTargetInCluster(void)
{
	SDWORD		tarPlayer, tarType, visPlayer, clusterID, cluster;
	BASE_OBJECT	*psTarget;

	if (!stackPopParams(2, VAL_INT, &clusterID, VAL_INT, &visPlayer))
	{
		return FALSE;
	}

	if (clusterID < 0 || clusterID >= CLUSTER_MAX)
	{
		ASSERT( FALSE, "scrTargetInCluster: invalid clusterID" );
		return FALSE;
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
		return FALSE;
	}

	return TRUE;
}

// ********************************************************************************************
// ********************************************************************************************
// Additional Skirmish Code for superduper skirmish games may99
// ********************************************************************************************
// ********************************************************************************************

BOOL scrSkCanBuildTemplate(void)
{
	STRUCTURE *psStructure;
	DROID_TEMPLATE *psTempl;

	SDWORD player;

	if (!stackPopParams(3,VAL_INT, &player,ST_STRUCTURE, &psStructure, ST_TEMPLATE, &psTempl))
	{
		return FALSE;
	}

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
		debug( LOG_ERROR, "scrSkCanBuildTemplate: Unhandled template type" );
		abort();
		break;
	}

	scrFunctionResult.v.bval = TRUE;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))		// yes
	{
		return FALSE;
	}
	return TRUE;

failTempl:
	scrFunctionResult.v.bval = FALSE;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))		// no
	{
		return FALSE;
	}
	return TRUE;
}

// ********************************************************************************************
// locate the enemy
// gives a target location given a player to attack.
BOOL scrSkLocateEnemy(void)
{
	SDWORD		player;//,*x,*y;
	STRUCTURE	*psStruct;

	if (!stackPopParams(1,VAL_INT, &player))
	{
		return FALSE;
	}

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
		scrFunctionResult.v.oval = psStruct;
		if (!stackPushResult((INTERP_TYPE)ST_BASEOBJECT, &scrFunctionResult))		// success!
		{
			return FALSE;
		}
	}
	else
	{
		scrFunctionResult.v.oval = NULL;
		if (!stackPushResult((INTERP_TYPE)ST_BASEOBJECT, &scrFunctionResult))		// part success
		{
			return FALSE;
		}
	}
	return TRUE;
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
		if ((IsResearchCompleted(&pPlayerRes[inc])==FALSE) && (IsResearchStarted(&pPlayerRes[inc])==FALSE))
		{
		return TRUE;
		}
	}

	// make sure that the research is not completed  or started by another researchfac
	if ((IsResearchCompleted(&pPlayerRes[inc])==FALSE) && (IsResearchStarted(&pPlayerRes[inc])==FALSE))
	{
		// Research is not completed  ... also  it has not been started by another researchfac

		//if there aren't any PR's - go to next topic
		if (!asResearch[inc].numPRRequired)
		{
			return FALSE;
		}

		//check for pre-requisites
		bPRFound = TRUE;
		for (incPR = 0; incPR < asResearch[inc].numPRRequired; incPR++)
		{
			if (IsResearchCompleted(&(pPlayerRes[asResearch[inc].pPRList[incPR]]))==0)
			{
				//if haven't pre-requisite - quit checking rest
				bPRFound = FALSE;
				break;
			}
		}
		if (!bPRFound)
		{
			//if haven't pre-requisites, skip the rest of the checks
			return FALSE;
		}

		//check for structure effects
		bStructFound = TRUE;
		for (incS = 0; incS < asResearch[inc].numStructures; incS++)
		{
			//if (!checkStructureStatus(asStructureStats + asResearch[inc].
			//	pStructList[incS], playerID, SS_BUILT))
			if (!checkSpecificStructExists(asResearch[inc].pStructList[incS],
				player))
			{
				//if not built, quit checking
				bStructFound = FALSE;
				break;
			}
		}
		if (!bStructFound)
		{
			//if haven't all structs built, skip to next topic
			return FALSE;
		}

		return TRUE;
	}
	return FALSE;
}
// ********************************************************************************************
BOOL scrSkDoResearch(void)
{
	SDWORD				player, bias;//,timeToResearch;//,*x,*y;
	UWORD				i;

	char				sTemp[128];
	STRUCTURE			*psBuilding;
	RESEARCH_FACILITY	*psResFacilty;
	PLAYER_RESEARCH		*pPlayerRes;
	RESEARCH			*pResearch;

	if (!stackPopParams(3,ST_STRUCTURE, &psBuilding, VAL_INT, &player, VAL_INT,&bias ))
	{
		return FALSE;
	}

	psResFacilty =	(RESEARCH_FACILITY*)psBuilding->pFunctionality;

	if(psResFacilty->psSubject != NULL)
	{
		// not finshed yet..
		return TRUE;
	}

	// choose a topic to complete.
	for(i=0;i<numResearch;i++)
	{
		if( skTopicAvail(i,player) )
		{
			break;
		}
	}

	if(i != numResearch)
	{
		pResearch = (asResearch+i);
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

		sprintf(sTemp,"player:%d starts topic: %s",player, asResearch[i].pName );
		NETlogEntry(sTemp,0,0);


	}

	return TRUE;
}

// ********************************************************************************************
BOOL scrSkVtolEnableCheck(void)
{
	SDWORD player;
	UDWORD i;

	if (!stackPopParams(1,VAL_INT, &player ))
	{
		return FALSE;
	}


	// vtol factory
	for(i=0;(i<numStructureStats)&&(asStructureStats[i].type != REF_VTOL_FACTORY);i++);
	if( (i < numStructureStats) && (apStructTypeLists[player][i] == AVAILABLE) )
	{
		// vtol propulsion
		for(i=0;(i<numPropulsionStats);i++)
		{
			if((asPropulsionStats[i].propulsionType == LIFT)
			 && apCompLists[player][COMP_PROPULSION][i] == AVAILABLE)
			{
				scrFunctionResult.v.bval = TRUE;
				if (!stackPushResult(VAL_BOOL, &scrFunctionResult))		// success!
				{
					return FALSE;
				}
				return TRUE;
			}
		}
	}

	scrFunctionResult.v.bval = FALSE;
	if (!stackPushResult(VAL_BOOL, &scrFunctionResult))		// success!
	{
		return FALSE;
	}
	return TRUE;
}

// ********************************************************************************************
BOOL scrSkGetFactoryCapacity(void)
{
	SDWORD count=0;
	STRUCTURE *psStructure;

	if (!stackPopParams(1,ST_STRUCTURE, &psStructure))
	{
		return FALSE;
	}

	if(psStructure && StructIsFactory(psStructure))
	{
		count = ((FACTORY *)psStructure->pFunctionality)->capacity;
	}

	scrFunctionResult.v.ival = count;
	if (!stackPushResult(VAL_INT, &scrFunctionResult))
	{
		return FALSE;
	}
	return TRUE;
}
// ********************************************************************************************
BOOL scrSkDifficultyModifier(void)
{
	SDWORD              amount,player;
	RESEARCH_FACILITY	*psResFacility;
	STRUCTURE           *psStr;
	PLAYER_RESEARCH		*pPlayerRes;

	if (!stackPopParams(1,VAL_INT, &player ))
	{
		return FALSE;
	}

	/* Skip cheats if difficulty modifier slider is set to minimum.
	 * (0 - player disabled, 20 - max value)
	 */
	if(game.skDiff[player] <= 1)
	{
		return TRUE;
	}

	// power modifier
	amount = game.skDiff[player]*40;		//(0-DIFF_SLIDER_STOPS)*25

	if(amount > 0)
	{
		addPower(player,amount);
	}
	else
	{
		usePower(player,(0-amount));
	}

	//research modifier.??
	for(psStr=apsStructLists[player];psStr;psStr=psStr->psNext)
	{
		// subtract 0 - 60% off the time to research.
		if(psStr->pStructureType->type == REF_RESEARCH)
		{
			psResFacility =	(RESEARCH_FACILITY*)psStr->pFunctionality;

			/*if(psResFacility->timeToResearch )
			{
				amount = psResFacility->timeToResearch;
				psResFacility->timeToResearch  = amount - ( (amount*3*game.skDiff[player])/100);
			}*/
            //this is not appropriate now the timeToResearch is not used - 10/06/99 so...
            //add 0-60% to the amount required to research
            if (psResFacility->psSubject)
            {
                pPlayerRes = asPlayerResList[player] +
                    (((RESEARCH *)psResFacility->psSubject)->ref - REF_RESEARCH_START);
                pPlayerRes->currentPoints += ((((RESEARCH *)psResFacility->psSubject)->
                    researchPoints * 3 * game.skDiff[player])/100);
            }
		}

	}

	//free stuff??


	return TRUE;
}

// ********************************************************************************************

// not a direct script function but a helper for scrSkDefenseLocation and scrSkDefenseLocationB
static BOOL defenseLocation(BOOL variantB)
{
	SDWORD		*pX,*pY,statIndex,statIndex2;
	UDWORD		x,y,gX,gY,dist,player,nearestSoFar,count;
	GATEWAY		*psGate,*psChosenGate;
	DROID		*psDroid;
	BASE_STATS	*psStats,*psWStats;
	UDWORD		x1,x2,x3,x4,y1,y2,y3,y4;
	BOOL		noWater;
	UDWORD      minCount;
	UDWORD      offset;

	if (!stackPopParams(6,
						VAL_REF|VAL_INT, &pX,
						VAL_REF|VAL_INT, &pY,
						ST_STRUCTURESTAT, &statIndex,
						ST_STRUCTURESTAT, &statIndex2,
						ST_DROID, &psDroid,
						VAL_INT, &player) )
	{
		debug(LOG_ERROR,"defenseLocation: failed to pop");
		return FALSE;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( FALSE, "defenseLocation:player number is too high" );
		return FALSE;
	}

	psStats = (BASE_STATS *)(asStructureStats + statIndex);
	psWStats = (BASE_STATS *)(asStructureStats + statIndex2);

    // check for wacky coords.
	if(		*pX < 0
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
	for(psGate= psGateways; psGate; psGate= psGate->psNext)
	{
		count = 0;
		noWater = TRUE;
		// does it have >1 tile unoccupied.
		if(psGate->x1 == psGate->x2)
		{// vert
			//skip gates that are too short
			if(variantB && (psGate->y2 - psGate->y1) <= 2)
			{
				continue;
			}
			gX = psGate->x1;
			for(gY=psGate->y1;gY <= psGate->y2; gY++)
			{
				if(! TILE_OCCUPIED(mapTile(gX,gY) ))
				{
					count++;
				}
				if (terrainType(mapTile(gX,gY)) == TER_WATER)
				{
					noWater = FALSE;
				}
			}
		}
		else
		{// horiz
			//skip gates that are too short
			if(variantB && (psGate->x2 - psGate->x1) <= 2)
			{
				continue;
			}
			gY = psGate->y1;
			for(gX=psGate->x1;gX <= psGate->x2; gX++)
			{
				if(! TILE_OCCUPIED(mapTile(gX,gY) ))
				{
					count++;
				}
				if (terrainType(mapTile(gX,gY)) == TER_WATER)
				{
					noWater = FALSE;
				}
			}
		}
		if(variantB) {
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
			dist = dirtySqrt(x,y,gX,gY);
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
			if(! TILE_OCCUPIED(mapTile(gX,gY) ))
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
			if(! TILE_OCCUPIED(mapTile(gX,gY) ))
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

	scrFunctionResult.v.bval = TRUE;
	if (!stackPushResult(VAL_BOOL,&scrFunctionResult))		// success
	{
		return FALSE;
	}


	// order the droid to build two walls, one either side of the gateway.
	// or one in the case of a 2 size gateway.

	//find center of the gateway
	x = (psChosenGate->x1 + psChosenGate->x2)/2;
	y = (psChosenGate->y1 + psChosenGate->y2)/2;

	//find start pos of the gateway
	x1 = world_coord(psChosenGate->x1) + (TILE_UNITS / 2);
	y1 = world_coord(psChosenGate->y1) + (TILE_UNITS / 2);

	if(variantB) {
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
		orderDroidStatsLoc(psDroid, DORDER_BUILD, psWStats, x1, y1);
	}
	else
	{
		orderDroidStatsTwoLoc(psDroid, DORDER_LINEBUILD, psWStats,  x1, y1,x2,y2);
	}

	// second section
	if(x3 == x4 && y3 == y4)
	{
		orderDroidStatsLocAdd(psDroid, DORDER_BUILD, psWStats, x3, y3);
	}
	else
	{
		orderDroidStatsTwoLocAdd(psDroid, DORDER_LINEBUILD, psWStats,  x3, y3,x4,y4);
	}

	return TRUE;

failed:
	scrFunctionResult.v.bval = FALSE;
	if (!stackPushResult(VAL_BOOL,&scrFunctionResult))		// failed!
	{
		return FALSE;
	}
	return TRUE;
}


// return a good place to build a defence, given a starting point
BOOL scrSkDefenseLocation(void)
{
    return defenseLocation(FALSE);
}

// return a good place to build a defence with a min number of clear tiles
BOOL scrSkDefenseLocationB(void)
{
    return defenseLocation(TRUE);
}


BOOL scrSkFireLassat(void)
{
	SDWORD	player;
	BASE_OBJECT *psObj;

	if (!stackPopParams(2,  VAL_INT, &player, ST_BASEOBJECT, &psObj))
	{
		return FALSE;
	}

	if(psObj)
	{
		orderStructureObj(player, psObj);
	}

	return TRUE;
}

//-----------------------
// New functions
//-----------------------
BOOL scrActionDroidObj(void)
{
	DROID			*psDroid;
	DROID_ACTION		action;
	BASE_OBJECT		*psObj;

	if (!stackPopParams(3, ST_DROID, &psDroid, VAL_INT, &action, ST_BASEOBJECT, &psObj))
	{
		debug(LOG_ERROR, "scrActionDroidObj: failed to pop");
		return FALSE;
	}

	ASSERT( psDroid != NULL,
		"scrOrderUnitObj: Invalid unit pointer" );
	ASSERT( psObj != NULL,
		"scrOrderUnitObj: Invalid object pointer" );

	if (psDroid == NULL || psObj == NULL)
	{
		return FALSE;
	}

	if (action != DACTION_DROIDREPAIR)
	{
		debug(LOG_ERROR, "scrActionDroidObj: this action is not supported");
		return FALSE;
	}

	actionDroidObj(psDroid, action, (BASE_OBJECT *)psObj);

	return TRUE;
}

//<script function - improved version
// variables for the group iterator
DROID_GROUP		*psScrIterateGroupB[MAX_PLAYERS];
DROID			*psScrIterateGroupDroidB[MAX_PLAYERS];

// initialise iterating a groups members
BOOL scrInitIterateGroupB(void)
{
	DROID_GROUP	*psGroup;
	SDWORD		bucket;

	if (!stackPopParams(2, ST_GROUP, &psGroup, VAL_INT, &bucket))
	{
		debug(LOG_ERROR, "scrInitIterateGroupB: stackPopParams failed");
		return FALSE;
	}

	ASSERT( psGroup != NULL,
		"scrInitIterateGroupB: invalid group pointer" );

	ASSERT( bucket < MAX_PLAYERS,
		"scrInitIterateGroupB: invalid bucket" );

	psScrIterateGroupB[bucket] = psGroup;
	psScrIterateGroupDroidB[bucket] = psGroup->psList;

	return TRUE;
}

//script function - improved version
// iterate through a groups members
BOOL scrIterateGroupB(void)
{
	DROID_GROUP	*psGroup;
	DROID		*psDroid;
	SDWORD		bucket;

	if (!stackPopParams(2, ST_GROUP, &psGroup, VAL_INT, &bucket))
	{
		debug(LOG_ERROR, "scrIterateGroupB: stackPopParams failed");
		return FALSE;
	}

	ASSERT( bucket < MAX_PLAYERS,
		"scrIterateGroupB: invalid bucket" );

	if (psGroup != psScrIterateGroupB[bucket])
	{
		ASSERT( FALSE, "scrIterateGroupB: invalid group, InitGroupIterateB not called?" );
		return FALSE;
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
		return FALSE;
	}

	return TRUE;
}

// ********************************************************************************************
// ********************************************************************************************
