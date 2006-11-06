/*
 * Group.c
 *
 * Link droids together into a group for AI etc.
 *
 */
#include <string.h>

#include "lib/framework/frame.h"
#include "objects.h"
#include "group.h"
#include "orderdef.h"

#include "multiplay.h"

// sizes for the group heap
#define GRP_HEAP_INIT	45
#define GRP_HEAP_EXT	15

// heap for the droid group structure
OBJ_HEAP	*psGrpHeap;


// initialise the group system
BOOL grpInitialise(void)
{
	if (!HEAP_CREATE(&psGrpHeap, sizeof(DROID_GROUP), GRP_HEAP_INIT, GRP_HEAP_EXT))
	{
		return FALSE;
	}

	return TRUE;
}


// shutdown the group system
void grpShutDown(void)
{
	HEAP_DESTROY(psGrpHeap);
}


// create a new group
BOOL grpCreate(DROID_GROUP	**ppsGroup)
{
	if (!HEAP_ALLOC(psGrpHeap, (void**) ppsGroup))
	{
		return FALSE;
	}

	(*ppsGroup)->type = GT_NORMAL;
	(*ppsGroup)->refCount = 0;
	(*ppsGroup)->psList = NULL;
	(*ppsGroup)->psCommander = NULL;
	memset(&(*ppsGroup)->sRunData, 0, sizeof(RUN_DATA));

	return TRUE;
}


// add a droid to a group
void grpJoin(DROID_GROUP *psGroup, DROID *psDroid)
{
	psGroup->refCount += 1;

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
		"grpJoin: invalid group pointer" );

	// if psDroid == NULL just increase the refcount don't add anything to the list
	if (psDroid != NULL)
	{
		if (psGroup->psList && psDroid->player != psGroup->psList->player)
		{
			ASSERT( FALSE,"grpJoin: Cannot have more than one players droids in a group" );
			return;
		}

		if (psDroid->psGroup != NULL)
		{
			grpLeave(psDroid->psGroup, psDroid);
		}

		psDroid->psGroup = psGroup;

		if (psDroid->droidType == DROID_TRANSPORTER)
		{
			ASSERT( (psGroup->type == GT_NORMAL),
				"grpJoin: Cannot have two transporters in a group" );
			psGroup->type = GT_TRANSPORTER;
			psDroid->psGrpNext = psGroup->psList;
			psGroup->psList = psDroid;
		}
		else if ((psDroid->droidType == DROID_COMMAND) &&
				 (psGroup->type != GT_TRANSPORTER))
		{
			ASSERT( (psGroup->type == GT_NORMAL) && (psGroup->psCommander == NULL),
				"grpJoin: Cannot have two command droids in a group" );
			psGroup->type = GT_COMMAND;
			psGroup->psCommander = psDroid;
		}
		else
		{
			psDroid->psGrpNext = psGroup->psList;
			psGroup->psList = psDroid;
		}
	}
}


// add a droid to a group at the end of the list
void grpJoinEnd(DROID_GROUP *psGroup, DROID *psDroid)
{
	DROID		*psPrev, *psCurr;

	psGroup->refCount += 1;

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
		"grpJoin: invalid group pointer" );

	// if psDroid == NULL just increase the refcount don't add anything to the list
	if (psDroid != NULL)
	{
		if (psGroup->psList && psDroid->player != psGroup->psList->player)
		{
			ASSERT( FALSE,"grpJoin: Cannot have more than one players droids in a group" );
			return;
		}

		if (psDroid->psGroup != NULL)
		{
			grpLeave(psDroid->psGroup, psDroid);
		}

		psDroid->psGroup = psGroup;

		if (psDroid->droidType == DROID_COMMAND)
		{
			ASSERT( (psGroup->type == GT_NORMAL) && (psGroup->psCommander == NULL),
				"grpJoin: Cannot have two command droids in a group" );
			psGroup->type = GT_COMMAND;
			psGroup->psCommander = psDroid;
		}
		else
		{
			// add the droid to the end of the list
			psPrev = NULL;
			psDroid->psGrpNext = NULL;
			for(psCurr = psGroup->psList; psCurr; psCurr=psCurr->psGrpNext)
			{
				psPrev = psCurr;
			}
			if (psPrev != NULL)
			{
				psPrev->psGrpNext = psDroid;
			}
			else
			{
				psGroup->psList = psDroid;
			}
		}
	}
}


// remove a droid from a group
void grpLeave(DROID_GROUP *psGroup, DROID *psDroid)
{
	DROID	*psPrev, *psCurr;

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
		"grpLeave: invalid group pointer" );

	if (   (psDroid != NULL )
		&& (psDroid->psGroup != psGroup) )
	{
		ASSERT( FALSE, "grpLeave: droid group does not match" );
		return;
	}


	psGroup->refCount -= 1;

	// if psDroid == NULL just decrease the refcount don't remove anything from the list
	if (psDroid != NULL &&
		(psDroid->droidType != DROID_COMMAND ||
		 psGroup->type != GT_COMMAND))
	{
		psPrev = NULL;
		for(psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			if (psCurr == psDroid)
			{
				break;
			}
			psPrev = psCurr;
		}
		ASSERT( psCurr != NULL, "grpLeave: droid not found" );
		if (psCurr != NULL)
		{
			if (psPrev)
			{
				psPrev->psGrpNext = psCurr->psGrpNext;
			}
			else
			{
				psGroup->psList = psGroup->psList->psGrpNext;
			}
		}
	}

	if (psDroid)
	{
		psDroid->psGroup = NULL;
		psDroid->psGrpNext = NULL;

		if ((psDroid->droidType == DROID_COMMAND) &&
			(psGroup->type == GT_COMMAND))
		{
			psGroup->type = GT_NORMAL;
			psGroup->psCommander = NULL;
		}
		else if ((psDroid->droidType == DROID_TRANSPORTER) &&
				 (psGroup->type == GT_TRANSPORTER))
		{
			psGroup->type = GT_NORMAL;
		}
	}

	// free the group structure if necessary
	if (psGroup->refCount <= 0)
	{
		HEAP_FREE(psGrpHeap, psGroup);
	}
}

// count the members of a group
SDWORD grpNumMembers(DROID_GROUP *psGroup)
{
	DROID	*psCurr;
	SDWORD	num;

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
		"grpNumMembers: invalid droid group" );

	num = 0;
	for(psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
	{
		num += 1;
	}

	return num;
}


// remove all droids from a group
void grpReset(DROID_GROUP *psGroup)
{
	DROID	*psCurr, *psNext;

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
		"grpReset: invalid droid group" );

	for(psCurr = psGroup->psList; psCurr; psCurr = psNext)
	{
		psNext = psCurr->psGrpNext;
		grpLeave(psGroup, psCurr);
	}
}

/* Give a group an order */
//void orderGroupBase(DROID_GROUP *psGroup, DROID_ORDER_DATA *psData)
//{
//	DROID *psCurr;
//	BOOL usedgrouporder=FALSE;

//	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
//		"orderGroupBase: invalid droid group" );
//
//	if (bMultiPlayer && SendGroupOrder(	psGroup, psData->x,	psData->y,	psData->psObj) )
//	{	// turn off multiplay messages,since we've send a group one instead.
//		bMultiPlayer =FALSE;
//		usedgrouporder = TRUE;
//	}
//
//	for (psCurr = psGroup->psList; psCurr; psCurr=psCurr->psGrpNext)
//	{
//		orderDroidBase(psCurr, psData);
//	}

//	if( usedgrouporder)
//	{
//		bMultiPlayer = TRUE;
//	}
//}

/* Give a group an order */
void orderGroup(DROID_GROUP *psGroup, DROID_ORDER order)
{
	DROID *psCurr;

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
		"orderGroup: invalid droid group" );

	for (psCurr = psGroup->psList; psCurr; psCurr=psCurr->psGrpNext)
	{
		orderDroid(psCurr, order);
	}
}

/* Give a group of droids an order */
void orderGroupLoc(DROID_GROUP *psGroup, DROID_ORDER order, UDWORD x, UDWORD y)
{
	DROID	*psCurr;

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
		"orderGroupLoc: invalid droid group" );

	if(bMultiPlayer)
	{
		SendGroupOrderGroup(psGroup,order,x,y,NULL);
		bMultiPlayer = FALSE;

		for(psCurr=psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			orderDroidLoc(psCurr, order, x,y);
		}

		bMultiPlayer = TRUE;
	}
	else
	{
		for(psCurr=psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			orderDroidLoc(psCurr, order, x,y);
		}
	}
}


/* Give a group of droids an order */
void orderGroupObj(DROID_GROUP *psGroup, DROID_ORDER order, BASE_OBJECT *psObj)
{
	DROID	*psCurr;

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
		"orderGroupObj: invalid droid group" );

	if(bMultiPlayer)
	{
		SendGroupOrderGroup(psGroup,order,0,0,psObj);
		bMultiPlayer = FALSE;

		for(psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			orderDroidObj(psCurr, order, psObj);
		}

		bMultiPlayer = TRUE;
	}
	else
	{
		for(psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			orderDroidObj(psCurr, order, psObj);
		}
	}
}

/* set the secondary state for a group of droids */
void grpSetSecondary(DROID_GROUP *psGroup, SECONDARY_ORDER sec, SECONDARY_STATE state)
{
	DROID	*psCurr;

	ASSERT( PTRVALID(psGroup, sizeof(DROID_GROUP)),
		"grpSetSecondary: invalid droid group" );

	for(psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
	{
		secondarySetState(psCurr, sec, state);
	}
}
