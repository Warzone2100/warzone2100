/*
 * ScriptAI.c
 *
 * Script functions to support the AI system
 *
 */

// various script tracing printf's
//#define DEBUG_GROUP0
// script order printf's
//#define DEBUG_GROUP1
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

// Add a droid to a group
BOOL scrGroupAddDroid(void)
{
	DROID_GROUP		*psGroup;
	DROID			*psDroid;

	if (!stackPopParams(2, ST_GROUP, &psGroup, ST_DROID, &psDroid))
	{
		return FALSE;
	}

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
		"scrGroupAdd: Invalid group pointer" );
	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"scrGroupAdd: Invalid droid pointer" );
	if (psDroid == NULL)
	{
		return FALSE;
	}
	if (psDroid->droidType == DROID_COMMAND)
	{
		ASSERT( FALSE,
			"scrGroupAdd: cannot add a command droid to a group" );
		return FALSE;
	}
	if (psDroid->droidType == DROID_TRANSPORTER)
	{
		ASSERT( FALSE,
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

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
		"scrGroupAdd: Invalid group pointer" );

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( FALSE, "scrGroupAddArea: invalid player" );
		return FALSE;
	}

// 	debug( LOG_SCRIPT, "groupAddArea: player %d (%d,%d) -> (%d,%d)\n", player, x1, y1, x2, y2 );

	for(psDroid=apsDroidLists[player]; psDroid; psDroid=psDroid->psNext)
	{
		if (((SDWORD)psDroid->x >= x1) && ((SDWORD)psDroid->x <= x2) &&
			((SDWORD)psDroid->y >= y1) && ((SDWORD)psDroid->y <= y2) &&
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

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
		"scrGroupAddNoGroup: Invalid group pointer" );

	if (player < 0 || player >= MAX_PLAYERS)
	{
		ASSERT( FALSE, "scrGroupAddAreaNoGroup: invalid player" );
		return FALSE;
	}

	for(psDroid=apsDroidLists[player]; psDroid; psDroid=psDroid->psNext)
	{
		if (((SDWORD)psDroid->x >= x1) && ((SDWORD)psDroid->x <= x2) &&
			((SDWORD)psDroid->y >= y1) && ((SDWORD)psDroid->y <= y2) &&
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

	ASSERT( PTRVALID(psTo, sizeof(DROID_GROUP)),
		"scrGroupAddGroup: Invalid group pointer" );
	ASSERT( PTRVALID(psFrom, sizeof(DROID_GROUP)),
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

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
		"scrGroupMember: Invalid group pointer" );
	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
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

	if (!stackPushResult(VAL_BOOL, retval))
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

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
		"scrIdleGroup: invalid group pointer" );

	for(psDroid = psGroup->psList;psDroid; psDroid = psDroid->psGrpNext)
	{
		if(  psDroid->order == DORDER_NONE
		  || (psDroid->order == DORDER_GUARD && psDroid->psTarget == NULL))
		{
			count++;
		}
	}

	if (!stackPushResult(VAL_INT, count))
	{
		return FALSE;
	}
	return TRUE;
}

// variables for the group iterator
DROID_GROUP		*psScrIterateGroup;
DROID			*psScrIterateGroupDroid;

// initialise iterating a groups members
BOOL scrInitIterateGroup(void)
{
	DROID_GROUP	*psGroup;

	if (!stackPopParams(1, ST_GROUP, &psGroup))
	{
		return FALSE;
	}

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
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
		ASSERT( FALSE, "scrIterateGroup: invalid group, InitGroupIterate not called?" );
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

	if (!stackPushResult((INTERP_TYPE)ST_DROID, (SDWORD)psDroid))
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

	if (!stackPushResult((INTERP_TYPE)ST_BASEOBJECT, (SDWORD)psObj))
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
	SDWORD			order;

	if (!stackPopParams(2, ST_GROUP, &psGroup, VAL_INT, &order))
	{
		return FALSE;
	}

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
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

	DBP1(("scrOrderGroup: group %p (%d) order %d\n", psGroup, grpNumMembers(psGroup), order));
	orderGroup(psGroup, order);

	return TRUE;
}


// Give a group an order to a location
BOOL scrOrderGroupLoc(void)
{
	DROID_GROUP		*psGroup;
	SDWORD			order, x,y;

	if (!stackPopParams(4, ST_GROUP, &psGroup, VAL_INT, &order, VAL_INT, &x, VAL_INT, &y))
	{
		return FALSE;
	}

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
		"scrOrderGroupLoc: Invalid group pointer" );

	if (order != DORDER_MOVE &&
		order != DORDER_SCOUT)
	{
		ASSERT( FALSE,
			"scrOrderGroupLoc: Invalid order" );
		return FALSE;
	}
	if (x < 0 || x > (SDWORD)(mapWidth << TILE_SHIFT) ||
		y < 0 || y > (SDWORD)(mapHeight << TILE_SHIFT))
	{
		ASSERT( FALSE,
			"scrOrderGroupLoc: Invalid location" );
		return FALSE;
	}

	DBP1(("scrOrderGroupLoc: group %p (%d) order %d (%d,%d)\n",
		psGroup, grpNumMembers(psGroup), order, x,y));
	orderGroupLoc(psGroup, order, (UDWORD)x,(UDWORD)y);

	return TRUE;
}


// Give a group an order to an object
BOOL scrOrderGroupObj(void)
{
	DROID_GROUP		*psGroup;
	SDWORD			order;
	BASE_OBJECT		*psObj;

	if (!stackPopParams(3, ST_GROUP, &psGroup, VAL_INT, &order, ST_BASEOBJECT, &psObj))
	{
		return FALSE;
	}

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
		"scrOrderGroupObj: Invalid group pointer" );
	ASSERT( PTRVALID(psObj, sizeof(BASE_OBJECT)),
		"scrOrderGroupObj: Invalid object pointer" );

	if (order != DORDER_ATTACK &&
		order != DORDER_HELPBUILD &&
		order != DORDER_DEMOLISH &&
		order != DORDER_REPAIR &&
		order != DORDER_OBSERVE &&
		order != DORDER_EMBARK &&
		order != DORDER_FIRESUPPORT)
	{
		ASSERT( FALSE,
			"scrOrderGroupObj: Invalid order" );
		return FALSE;
	}

	DBP1(("scrOrderGroupObj: group %p (%d) order %d,  obj type %d player %d id %d\n",
		psGroup, grpNumMembers(psGroup), order, psObj->type, psObj->player, psObj->id));
	orderGroupObj(psGroup, order, psObj);

	return TRUE;
}

// Give a droid an order
BOOL scrOrderDroid(void)
{
	DROID			*psDroid;
	SDWORD			order;

	if (!stackPopParams(2, ST_DROID, &psDroid, VAL_INT, &order))
	{
		return FALSE;
	}

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
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
	SDWORD			order, x,y;

	if (!stackPopParams(4, ST_DROID, &psDroid, VAL_INT, &order, VAL_INT, &x, VAL_INT, &y))
	{
		return FALSE;
	}

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
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
	if (x < 0 || x > (SDWORD)(mapWidth << TILE_SHIFT) ||
		y < 0 || y > (SDWORD)(mapHeight << TILE_SHIFT))
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
	SDWORD			order;
	BASE_OBJECT		*psObj;

	if (!stackPopParams(3, ST_DROID, &psDroid, VAL_INT, &order, ST_BASEOBJECT, &psObj))
	{
		return FALSE;
	}

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"scrOrderUnitObj: Invalid unit pointer" );
	ASSERT( PTRVALID(psObj, sizeof(BASE_OBJECT)),
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
		order != DORDER_FIRESUPPORT)
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
	SDWORD			order, x,y, statIndex;
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

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"scrOrderUnitStatsLoc: Invalid Unit pointer" );
	ASSERT( PTRVALID(psStats, sizeof(BASE_STATS)),
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
//#ifdef PSX
//	else {
//		BeepMessage(STR_GAM_MAXSTRUCTSREACHED);
//#endif
//	}

	return TRUE;
}


// set the secondary state for a droid
BOOL scrSetDroidSecondary(void)
{
	DROID		*psDroid;
	SDWORD		sec, state;

	if (!stackPopParams(3, ST_DROID, &psDroid, VAL_INT, &sec, VAL_INT, &state))
	{
		return FALSE;
	}

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
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
	SDWORD			sec, state;

	if (!stackPopParams(3, ST_GROUP, &psGroup, VAL_INT, &sec, VAL_INT, &state))
	{
		return FALSE;
	}

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
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
UDWORD scrStructTargetMask(STRUCTURE *psStruct)
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
        if (psStats->psWeapStat == NULL && psStats->pSensor != NULL)
		{
			mask = SCR_ST_SENSOR;
		}
		//else if (psStats->numWeaps > 0)
        else if (psStats->psWeapStat != NULL)
		{
			//psWStats = psStats->asWeapList[0];
            psWStats = psStats->psWeapStat;
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
void scrStructTargetPriority(STRUCTURE **ppsTarget, STRUCTURE *psCurr)
{
	// skip walls unless they are explicitly asked for
	if ((scrStructPref & SCR_ST_WALL) ||
		((psCurr->pStructureType->type != REF_WALL) &&
		 (psCurr->pStructureType->type != REF_WALLCORNER)) )
	{
		*ppsTarget = psCurr;
	}
}


UDWORD scrDroidTargetMask(DROID *psDroid)
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
void scrDroidTargetPriority(DROID **ppsTarget, DROID *psCurr)
{
	// priority to things with weapons
	if ( ((*ppsTarget) == NULL) ||
		 //((*ppsTarget)->numWeaps == 0) )
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
BASE_OBJECT *scrTargetInArea(SDWORD tarPlayer, SDWORD visPlayer, SDWORD tarType, SDWORD cluster,
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
			((SDWORD)psCurr->x >= x1) &&
			((SDWORD)psCurr->x <= x2) &&
			((SDWORD)psCurr->y >= y1) &&
			((SDWORD)psCurr->y <= y2))
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
	STRUCTURE	*psTarget;

	if (!stackPopParams(6, VAL_INT, &tarPlayer, VAL_INT, &visPlayer,
						   VAL_INT, &x1, VAL_INT, &y1, VAL_INT, &x2, VAL_INT, &y2))
	{
		return FALSE;
	}

	psTarget = (STRUCTURE *)scrTargetInArea(tarPlayer, visPlayer, SCR_TAR_STRUCT, 0, x1,y1, x2,y2);

	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, (UDWORD)psTarget))
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

	if (!stackPushResult((INTERP_TYPE)ST_STRUCTURE, (UDWORD)psTarget))
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

	if (!stackPushResult((INTERP_TYPE)ST_DROID, (UDWORD)psTarget))
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

	if (!stackPushResult((INTERP_TYPE)ST_DROID, (UDWORD)psTarget))
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

	if (!stackPushResult((INTERP_TYPE)ST_BASEOBJECT, (UDWORD)psTarget))
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

	SDWORD player, structure, templ;

	if (!stackPopParams(3,VAL_INT, &player,ST_STRUCTURE, &structure, ST_TEMPLATE, &templ))
	{
		return FALSE;
	}

	psTempl =  (DROID_TEMPLATE*) templ;
	psStructure = (STRUCTURE *) structure;

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

	case DROID_COMMAND:
	case DROID_CONSTRUCT:	        // Constructor droid
	case DROID_PERSON:		        // person
    case DROID_CYBORG_CONSTRUCT:	// cyborg-construct thang
    case DROID_CYBORG_REPAIR:		// cyborg-repair thang
	case DROID_TRANSPORTER:	        // guess what this is!
	case DROID_DEFAULT:		        // Default droid
	case DROID_ANY:
	default:
		debug( LOG_ERROR, "scrSkCanBuildTemplate: Unhandled template type" );
		abort();
		break;
	}

	if (!stackPushResult(VAL_BOOL, TRUE))		// yes
	{
		return FALSE;
	}
	return TRUE;

failTempl:
	if (!stackPushResult(VAL_BOOL, FALSE))		// no
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
		if (!stackPushResult((INTERP_TYPE)ST_BASEOBJECT, (SDWORD)psStruct ))		// success!
		{
			return FALSE;
		}
	}
	else
	{
		if (!stackPushResult((INTERP_TYPE)ST_BASEOBJECT, 0))		// part success
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
	SDWORD				structure, player, bias;//,timeToResearch;//,*x,*y;
	UWORD				i;

	STRING				sTemp[128];
	STRUCTURE			*psBuilding;
	RESEARCH_FACILITY	*psResFacilty;
	PLAYER_RESEARCH		*pPlayerRes;
	RESEARCH			*pResearch;

	if (!stackPopParams(3,ST_STRUCTURE, &structure, VAL_INT, &player, VAL_INT,&bias ))
	{
		return FALSE;
	}

	psBuilding	=	(STRUCTURE *) structure;
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
/*
	// do it.
	if(i != numResearch)
	{
		researchResult(i,(UBYTE)player,FALSE);

		sprintf(sTemp,"player:%d did topic: %s",player, asResearch[i].pName );
		NETlogEntry(sTemp,0,0);

		SendResearch((UBYTE)player,i );
	}

	// set delay for next topic.


	timeToResearch = (asResearch+i)->researchPoints / ((RESEARCH_FACILITY*)psResearch->pFunctionality)->researchPoints;;

	if (!stackPushResult(VAL_INT, timeToResearch))		// return time to do it..
	{
		return FALSE;
	}
*/

//	UDWORD				count;
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
				if (!stackPushResult(VAL_BOOL, TRUE))		// success!
				{
					return FALSE;
				}
				return TRUE;
			}
		}
	}

	if (!stackPushResult(VAL_BOOL, FALSE))		// success!
	{
		return FALSE;
	}
	return TRUE;
}

// ********************************************************************************************
BOOL scrSkGetFactoryCapacity(void)
{
	SDWORD count=0,structure;
	STRUCTURE *psStructure;

	if (!stackPopParams(1,ST_STRUCTURE, &structure))
	{
		return FALSE;
	}
	psStructure = (STRUCTURE *) structure;

	if(psStructure && StructIsFactory(psStructure))
	{
		count = ((FACTORY *)psStructure->pFunctionality)->capacity;
	}

	if (!stackPushResult(VAL_INT, count))
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

	// power modifier
	amount = game.skDiff[player]*40;		//(0-20)*25
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

//		subtract 0 - 60% off the time to research.
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
// return a good place to build a defence, given a starting point

BOOL scrSkDefenseLocation(void)
{
	SDWORD		*pX,*pY,statIndex,statIndex2;
	UDWORD		x,y,gX,gY,dist,player,nearestSoFar,count;
	GATEWAY		*psGate,*psChosenGate;
	DROID		*psDroid;
	BASE_STATS	*psStats,*psWStats;
	UDWORD		x1,x2,x3,x4,y1,y2,y3,y4;
	BOOL		noWater;

	if (!stackPopParams(6,
						VAL_REF|VAL_INT, &pX,
						VAL_REF|VAL_INT, &pY,
						ST_STRUCTURESTAT, &statIndex,
						ST_STRUCTURESTAT, &statIndex2,
						ST_DROID, &psDroid,
						VAL_INT, &player) )
	{
		return FALSE;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( FALSE, "scrSkDefenseLocation:player number is too high" );
		return FALSE;
	}

	psStats = (BASE_STATS *)(asStructureStats + statIndex);
	psWStats = (BASE_STATS *)(asStructureStats + statIndex2);

    // check for wacky coords.
	if(		*pX < 0
		||	*pX > (SDWORD)(mapWidth<<TILE_SHIFT)
		||	*pY < 0
		||	*pY > (SDWORD)(mapHeight<<TILE_SHIFT)
	  )
	{
		goto failed;
	}

	x = *pX >> TILE_SHIFT;					// change to tile coords.
	y = *pY >> TILE_SHIFT;

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
			gX = psGate->x1;
			for(gY=psGate->y1;gY <= psGate->y2; gY++)
			{
				if(! TILE_OCCUPIED(mapTile(gX,gY) ))
				{
					count++;
				}
				if(TERRAIN_TYPE(mapTile(gX,gY)) == TER_WATER)
				{
					noWater = FALSE;
				}
			}
		}
		else
		{// horiz
			gY = psGate->y1;
			for(gX=psGate->x1;gX <= psGate->x2; gX++)
			{
				if(! TILE_OCCUPIED(mapTile(gX,gY) ))
				{
					count++;
				}
				if(TERRAIN_TYPE(mapTile(gX,gY)) == TER_WATER)
				{
					noWater = FALSE;
				}
			}
		}
		if(count > 1 && noWater)
		{	// ok it's free. Is it the nearest one yet?
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
	*pX = (x << TILE_SHIFT) + (TILE_UNITS/2);		// return centre of tile.
	*pY = (y << TILE_SHIFT) + (TILE_UNITS/2);

	if (!stackPushResult(VAL_BOOL,TRUE))		// success
	{
		return FALSE;
	}


	// order the droid to build two walls, one either side of the gateway.
	// or one in the case of a 2 size gateway.

	x = (psChosenGate->x1 + psChosenGate->x2)/2;
	y = (psChosenGate->y1 + psChosenGate->y2)/2;

	x1 = (psChosenGate->x1 << TILE_SHIFT) + (TILE_UNITS/2);
	y1 = (psChosenGate->y1 << TILE_SHIFT) + (TILE_UNITS/2);

	if(psChosenGate->x1 == psChosenGate->x2)
	{
		x2 = x1;
		y2 = ((y-1) << TILE_SHIFT) + (TILE_UNITS/2);
		x3 = x1;
		y3 = ((y+1) << TILE_SHIFT) + (TILE_UNITS/2);
	}
	else
	{
		x2 = ((x-1) << TILE_SHIFT) + (TILE_UNITS/2);
		y2 = y1;
		x3 = ((x+1) << TILE_SHIFT) + (TILE_UNITS/2);
		y3 = y1;

	}
	x4 = (psChosenGate->x2 << TILE_SHIFT) + (TILE_UNITS/2);
	y4 = (psChosenGate->y2 << TILE_SHIFT) + (TILE_UNITS/2);


	// first section.
	if(x1 == x2 && y1 == y2)
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
	if (!stackPushResult(VAL_BOOL,FALSE))		// failed!
	{
		return FALSE;
	}
	return TRUE;
}

// return a good place to build a defence with a min number of clear tiles
BOOL scrSkDefenseLocationB(void)
{
	SDWORD		*pX,*pY,statIndex,statIndex2;
	UDWORD		x,y,gX,gY,dist,player,nearestSoFar,count;
	GATEWAY		*psGate,*psChosenGate;
	DROID		*psDroid;
	BASE_STATS	*psStats,*psWStats;
	UDWORD		x1,x2,x3,x4,y1,y2,y3,y4;
	BOOL		noWater;

	if (!stackPopParams(6,
						VAL_REF|VAL_INT, &pX,
						VAL_REF|VAL_INT, &pY,
						ST_STRUCTURESTAT, &statIndex,
						ST_STRUCTURESTAT, &statIndex2,
						ST_DROID, &psDroid,
						VAL_INT, &player) )
	{
		debug(LOG_ERROR,"scrSkDefenseLocationB: failed to pop");
		return FALSE;
	}

	if (player >= MAX_PLAYERS)
	{
		ASSERT( FALSE, "scrSkDefenseLocationB:player number is too high" );
		return FALSE;
	}

	psStats = (BASE_STATS *)(asStructureStats + statIndex);
	psWStats = (BASE_STATS *)(asStructureStats + statIndex2);

    // check for wacky coords.
	if(		*pX < 0
		||	*pX > (SDWORD)(mapWidth<<TILE_SHIFT)
		||	*pY < 0
		||	*pY > (SDWORD)(mapHeight<<TILE_SHIFT)
	  )
	{
		goto failed;
	}

	x = *pX >> TILE_SHIFT;					// change to tile coords.
	y = *pY >> TILE_SHIFT;

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
			if((psGate->y2 - psGate->y1) <= 2)
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
				if(TERRAIN_TYPE(mapTile(gX,gY)) == TER_WATER)
				{
					noWater = FALSE;
				}
			}
		}
		else
		{// horiz

			//skip gates that are too short
			if((psGate->x2 - psGate->x1) <= 2)
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
				if(TERRAIN_TYPE(mapTile(gX,gY)) == TER_WATER)
				{
					noWater = FALSE;
				}
			}
		}
		if(count > 2 && noWater)	//<NEW> min 2 tiles
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
	*pX = (x << TILE_SHIFT) + (TILE_UNITS/2);		// return centre of tile.
	*pY = (y << TILE_SHIFT) + (TILE_UNITS/2);

	if (!stackPushResult(VAL_BOOL,TRUE))		// success
	{
		return FALSE;
	}


	// order the droid to build two walls, one either side of the gateway.
	// or one in the case of a 2 size gateway.

	//find center of the gateway
	x = (psChosenGate->x1 + psChosenGate->x2)/2;
	y = (psChosenGate->y1 + psChosenGate->y2)/2;

	//find start pos of the gateway
	x1 = (psChosenGate->x1 << TILE_SHIFT) + (TILE_UNITS/2);
	y1 = (psChosenGate->y1 << TILE_SHIFT) + (TILE_UNITS/2);

	if(psChosenGate->x1 == psChosenGate->x2)	//vert
	{
		x2 = x1;	//vert: end x pos of the first section = start x pos
		y2 = ((y-1) << TILE_SHIFT) + (TILE_UNITS/2);	//start y loc of the first sec
		x3 = x1;
		y3 = ((y+2) << TILE_SHIFT) + (TILE_UNITS/2);	//<NEW>	//start y loc of the second sec
	}
	else		//hor
	{
		x2 = ((x-1) << TILE_SHIFT) + (TILE_UNITS/2);
		y2 = y1;
		x3 = ((x+2) << TILE_SHIFT) + (TILE_UNITS/2);	//<NEW>
		y3 = y1;

	}
	//end coords of the second section
	x4 = (psChosenGate->x2 << TILE_SHIFT) + (TILE_UNITS/2);
	y4 = (psChosenGate->y2 << TILE_SHIFT) + (TILE_UNITS/2);

	//some temp checks
	if(x2 < x1)
	{
		debug(LOG_ERROR,"scrSkDefenseLocationB: x2 < x1");
		return FALSE;
	}
	if(x3 > x4)
	{
		debug(LOG_ERROR,"scrSkDefenseLocationB: x2 < x1");
		return FALSE;
	}

	if(y2 < y1)
	{
		debug(LOG_ERROR,"scrSkDefenseLocationB: y2 < y1");
		return FALSE;
	}
	if(y3 > y4)
	{
		debug(LOG_ERROR,"scrSkDefenseLocationB: y3 > y4");
		return FALSE;
	}


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
	if (!stackPushResult(VAL_BOOL,FALSE))		// failed!
	{
		return FALSE;
	}
	return TRUE;
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
	SDWORD			action;
	BASE_OBJECT		*psObj;

	if (!stackPopParams(3, ST_DROID, &psDroid, VAL_INT, &action, ST_BASEOBJECT, &psObj))
	{
		debug(LOG_ERROR, "scrActionDroidObj: failed to pop");
		return FALSE;
	}

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"scrOrderUnitObj: Invalid unit pointer" );
	ASSERT( PTRVALID(psObj, sizeof(BASE_OBJECT)),
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

	actionDroidObj(psDroid, action, psObj);

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

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
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

	if (!stackPushResult((INTERP_TYPE)ST_DROID, (SDWORD)psDroid))
	{
		debug(LOG_ERROR, "scrIterateGroupB: stackPushResult failed");
		return FALSE;
	}

	return TRUE;
}

// ********************************************************************************************
// Give a Droid a build order
/*BOOL scrSkOrderDroidLineBuild(void)
{
	DROID			*psDroid;
	SDWORD			x1,y1,x2,y2, statIndex;
	BASE_STATS		*psStats;

	if (!stackPopParams(6, ST_DROID, &psDroid, ST_STRUCTURESTAT, &statIndex,
						   VAL_INT, &x1, VAL_INT, &y1,VAL_INT, &x2, VAL_INT, &y2))
	{
		return FALSE;
	}

	psStats = (BASE_STATS *)(asStructureStats + statIndex);

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"scrOrderDroidLineBuild: Invalid Unit pointer" );
	ASSERT( PTRVALID(psStats, sizeof(BASE_STATS)),
		"scrOrderDroidLineBuild: Invalid object pointer" );
	if (psDroid == NULL)
	{
		return FALSE;
	}

	if ((x1 < 0) || (x1 > (SDWORD)mapWidth*TILE_UNITS) ||
		(y1 < 0) || (y1 > (SDWORD)mapHeight*TILE_UNITS)||
		(x2 < 0) || (x2 > (SDWORD)mapWidth*TILE_UNITS) ||
		(y2 < 0) || (y2 > (SDWORD)mapHeight*TILE_UNITS) )
	{
		ASSERT( FALSE,
			"scrOrderDroidLineBuild: Invalid location" );
		return FALSE;
	}


	if(IsPlayerStructureLimitReached(psDroid->player) == FALSE)
	{
		orderDroidStatsTwoLoc(psDroid, DORDER_LINEBUILD, psStats, (UDWORD)x1,(UDWORD)y1,(UDWORD)x2,(UDWORD)y2);
	}

	return TRUE;
}
*/
// ********************************************************************************************

// skirmish template storage facility

// storage definition
/*
typedef struct _skirmishstore {
	SDWORD					index;
	DROID_TEMPLATE			*psTempl;
	struct _skirmishstore	*psNext;
}SKIRMISHSTORE;

#define numSkPiles	8
SKIRMISHSTORE	*skirmishStore[numSkPiles];

// init templates
BOOL skInitSkirmishTemplates(void)
{
	UBYTE i;
	for(i=0;i<numSkPiles;i++)
	{
		skirmishStore[i] = NULL;
	}
	if (!stackPushResult(VAL_BOOL, TRUE))		// success!
	{
		return FALSE;
	}
	return TRUE;
}

// add template
BOOL skAddTemplate(void)
{
	SKIRMISHSTORE *psT;

	SDWORD			stempl,index,pile;
	DROID_TEMPLATE	*psTempl;

	if (!stackPopParams(3,VAL_INT, &pile,VAL_INT, &index, ST_TEMPLATE, &stempl))
	{
		return FALSE;
	}
	psTempl =(DROID_TEMPLATE *)stempl;
//	psT = MALLOC(sizeof(SKIRMISHSTORE));
	HEAP_ALLOC(psTemplateHeap,&psT);
	if ( !psT)
	{
		goto fail;
	}
	psT->index			= index;
	psT->psTempl		= psTempl;
	psT->psNext			= skirmishStore[pile];
	skirmishStore[pile] = psT;

	if (!stackPushResult(VAL_BOOL, TRUE))		// yes
	{
		return FALSE;
	}
	return TRUE;
fail:
	if (!stackPushResult(VAL_BOOL, FALSE))		// no
	{
		return FALSE;
	}
	return TRUE;
}


// get template
BOOL skGetTemplate(void)
{
	SDWORD	index,pile;
	SKIRMISHSTORE *psT;

	if (!stackPopParams(2,VAL_INT, &pile,VAL_INT, &index))
	{
		return FALSE;
	}

	for( psT = skirmishStore[pile]; psT && (psT->index != index); psT=psT->psNext);
	if(!psT)
	{
		DBERROR(("failed to find required skTemplate"));
		return FALSE;
	}

	if (!stackPushResult(ST_TEMPLATE, (UDWORD)(psT->psTempl) ))
	{
		return FALSE;
	}
	return TRUE;
}

// clear templates
BOOL skClearSkirmishTemplates(void)
{
	UBYTE i;
	for(i=0;i<numSkPiles;i++)
	{
		while(skirmishStore[i])
		{
			HEAP_FREE(psTemplateHeap,skirmishStore[i]);
		}
	}

	return TRUE;
}
*/
// ********************************************************************************************
// ********************************************************************************************
