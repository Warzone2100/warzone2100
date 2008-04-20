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
/** @file
 *  A* based path finding
 */

#include <assert.h>

#include "lib/framework/frame.h"

#include "objects.h"
#include "map.h"
#include "raycast.h"
#include "fpath.h"

#include "astar.h"

static SDWORD	astarOuter, astarRemove;
SDWORD	astarInner;

// The structure to store a node of the route
typedef struct _fp_node
{
	SWORD	x,y;		// map coords
	SWORD	dist, est;	// distance so far and estimate to end
	SWORD	type;		// open or closed node
	struct _fp_node *psOpen;
	struct _fp_node	*psRoute;	// Previous point in the route
	struct _fp_node *psNext;
} FP_NODE;

// types of node
#define NT_OPEN		1
#define NT_CLOSED	2

// List of open nodes
FP_NODE		*psOpen;

// Size of closed hash table
#define FPATH_TABLESIZE		4091

// Hash table for closed nodes
FP_NODE*        apsNodes[FPATH_TABLESIZE] = { NULL };

#define NUM_DIR		8

// Convert a direction into an offset
// dir 0 => x = 0, y = -1
static const Vector2i aDirOffset[NUM_DIR] =
{
	{ 0, 1},
	{-1, 1},
	{-1, 0},
	{-1,-1},
	{ 0,-1},
	{ 1,-1},
	{ 1, 0},
	{ 1, 1},
};

// reset the astar counters
void astarResetCounters(void)
{
	astarInner = 0;
	astarOuter = 0;
	astarRemove = 0;
}

/* next four used in HashPJW */
#define	BITS_IN_int32_t (sizeof(int32_t) * CHAR_BIT)
#define	THREE_QUARTERS  ((BITS_IN_int32_t * 3) / 4)
#define	ONE_EIGHTH      (BITS_IN_int32_t / 8)
#define	HIGH_BITS       (~((int32_t)(~0) >> ONE_EIGHTH))

/** Hash string
 *  Adaptation of Peter Weinberger's (PJW) generic hashing algorithm listed in
 *  Binstock+Rex, "Practical Algorithms" p 69.
 *
 *  Hacked to use coordinates instead of a string
 */
static SDWORD fpathHashFunc(int32_t x, int32_t y)
{
	char    buf[sizeof(x) + sizeof(y)];
	char   *c;
	int32_t hashValue = 0;

	memcpy(&buf[0],         &x, sizeof(x));
	memcpy(&buf[sizeof(x)], &y, sizeof(y));

	for (c = &buf[0]; c != &buf[ARRAY_SIZE(buf)]; ++c)
	{
		int32_t highBits;

		if (*c == 0)
			continue;

		hashValue = (hashValue << ONE_EIGHTH) + *c;
		highBits = hashValue & HIGH_BITS;

		if (highBits != 0)
		{
			hashValue = (hashValue ^ (highBits >> THREE_QUARTERS)) & ~HIGH_BITS;
		}
	}

	hashValue %= ARRAY_SIZE(apsNodes);

	return hashValue;
}

// Add a node to the hash table
static void fpathHashAdd(FP_NODE *apsTable[], FP_NODE *psNode)
{
	SDWORD	index;

	index = fpathHashFunc(psNode->x, psNode->y);

	psNode->psNext = apsTable[index];
	apsTable[index] = psNode;
}

// See if a node is in the hash table
static FP_NODE *fpathHashPresent(FP_NODE *apsTable[], SDWORD x, SDWORD y)
{
	SDWORD		index;
	FP_NODE		*psFound;

	index = fpathHashFunc(x,y);
	psFound = apsTable[index];
	while (psFound && !(psFound->x == x && psFound->y == y))
	{
		psFound = psFound->psNext;
	}

	return psFound;
}

// Reset the hash tables
static void fpathHashReset(void)
{
	int i;

	for(i = 0; i< ARRAY_SIZE(apsNodes); ++i)
	{
		while (apsNodes[i])
		{
			FP_NODE* toFree = apsNodes[i];
			apsNodes[i] = apsNodes[i]->psNext;

			free(toFree);
		}
	}
}

// Compare two nodes
static inline SDWORD fpathCompare(FP_NODE *psFirst, FP_NODE *psSecond)
{
	SDWORD	first,second;

	first = psFirst->dist + psFirst->est;
	second = psSecond->dist + psSecond->est;
	if (first < second)
	{
		return -1;
	}
	else if (first > second)
	{
		return 1;
	}

	// equal totals, give preference to node closer to target
	if (psFirst->est < psSecond->est)
	{
		return -1;
	}
	else if (psFirst->est > psSecond->est)
	{
		return 1;
	}

	// exactly equal
	return 0;
}

// make a 50/50 random choice
static BOOL fpathRandChoice(void)
{
	return ONEINTWO;
}

// Add a node to the open list
static void fpathOpenAdd(FP_NODE *psNode)
{
	psNode->psOpen = psOpen;
	psOpen = psNode;
}

// Get the nearest entry in the open list
static FP_NODE *fpathOpenGet(void)
{
	FP_NODE	*psNode, *psCurr, *psPrev, *psParent = NULL;
	SDWORD	comp;

	if (psOpen == NULL)
	{
		return NULL;
	}

	// find the node with the lowest distance
	psPrev = NULL;
	psNode = psOpen;
	for(psCurr = psOpen; psCurr; psCurr = psCurr->psOpen)
	{
		comp = fpathCompare(psCurr, psNode);
		if (comp < 0 || (comp == 0 && fpathRandChoice()))
		{
			psParent = psPrev;
			psNode = psCurr;
		}
		psPrev = psCurr;
	}

	// remove the node from the list
	if (psNode == psOpen)
	{
		// node is at head of list
		psOpen = psOpen->psOpen;
	}
	else
	{
		psParent->psOpen = psNode->psOpen;
	}

	return psNode;
}

// estimate the distance to the target point
static SDWORD fpathEstimate(SDWORD x, SDWORD y, SDWORD fx, SDWORD fy)
{
	SDWORD xdiff, ydiff;

	xdiff = x > fx ? x - fx : fx - x;
	ydiff = y > fy ? y - fy : fy - y;

	xdiff = xdiff * 10;
	ydiff = ydiff * 10;

	return xdiff > ydiff ? xdiff + ydiff/2 : xdiff/2 + ydiff;
}

// Generate a new node
static FP_NODE *fpathNewNode(SDWORD x, SDWORD y, SDWORD dist, FP_NODE *psRoute)
{
	FP_NODE	*psNode = malloc(sizeof(FP_NODE));

	if (psNode == NULL)
	{
		debug(LOG_ERROR, "fpathNewNode: Out of memory");
		return NULL;
	}

	psNode->x = (SWORD)x;
	psNode->y = (SWORD)y;
	psNode->dist = (SWORD)dist;
	psNode->psRoute = psRoute;
	psNode->type = NT_OPEN;

	return psNode;
}

// Variables for the callback
static SDWORD	finalX,finalY, vectorX,vectorY;
static BOOL		obstruction;

// The visibility ray callback
static BOOL fpathVisCallback(SDWORD x, SDWORD y, SDWORD dist)
{
	SDWORD	vx,vy;

	dist = dist;

	// See if this point is past the final point (dot product)
	vx = x - finalX;
	vy = y - finalY;
	if (vx*vectorX + vy*vectorY <=0)
	{
		return false;
	}

	if (fpathBlockingTile(map_coord(x), map_coord(y)))
	{
		// found an obstruction
		obstruction = true;
		return false;
	}

	return true;
}

// Check los between two tiles
BOOL fpathTileLOS(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2)
{
	// convert to world coords
	x1 = world_coord(x1) + TILE_UNITS / 2;
	y1 = world_coord(y1) + TILE_UNITS / 2;
	x2 = world_coord(x2) + TILE_UNITS / 2;
	y2 = world_coord(y2) + TILE_UNITS / 2;

	// Initialise the callback variables
	finalX = x2;
	finalY = y2;
	vectorX = x1 - x2;
	vectorY = y1 - y2;
	obstruction = false;

	rayCast(x1,y1, rayPointsToAngle(x1,y1,x2,y2),
			RAY_MAXLEN, fpathVisCallback);

	return !obstruction;
}

// A* findpath
SDWORD fpathAStarRoute(SDWORD routeMode, ASTAR_ROUTE *psRoutePoints,
					 SDWORD sx, SDWORD sy, SDWORD fx, SDWORD fy)
{
 	FP_NODE		*psFound, *psCurr, *psNew, *psParent, *psNext;
static 	FP_NODE		*psNearest, *psRoute;
	SDWORD		tileSX,tileSY,tileFX,tileFY;
	SDWORD		dir, x,y, currDist;
	SDWORD		index;
	SDWORD		retval;

	tileSX = map_coord(sx);
	tileSY = map_coord(sy);

	tileFX = map_coord(fx);
	tileFY = map_coord(fy);

	if (routeMode == ASR_NEWROUTE)
	{
		fpathHashReset();

		// Add the start point to the open list
		psCurr = fpathNewNode(tileSX,tileSY, 0, NULL);
		if (!psCurr)
		{
			fpathHashReset();
			return ASR_FAILED;
		}
		// estimate the estimated distance/moves
		psCurr->est = (SWORD)fpathEstimate(psCurr->x, psCurr->y, tileFX, tileFY);
		psOpen = NULL;
		fpathOpenAdd(psCurr);
		fpathHashAdd(apsNodes, psCurr);
		psRoute = NULL;
		psNearest = NULL;
	}

	// search for a route
	while (psOpen != NULL)
	{
		if (astarInner > FPATH_LOOP_LIMIT)
		{
			return ASR_PARTIAL;
		}

		psCurr = fpathOpenGet();

		if (psCurr->x == tileFX && psCurr->y == tileFY)
		{
			// reached the target
			psRoute = psCurr;
			break;
		}

		// note the nearest node to the target so far
		if (psNearest == NULL || psCurr->est < psNearest->est)
		{
			psNearest = psCurr;
		}

		astarOuter += 1;

		// loop through possible moves in 8 directions to find a valid move
		for(dir=0; dir<NUM_DIR; dir+=1)
		{
			/* make non-orthogonal-adjacent moves' dist a bit longer/cost a bit more
			   5  6  7
			     \|/
			   4 -I- 0
			     /|\
			   3  2  1
			   odd:orthogonal-adjacent tiles even:non-orthogonal-adjacent tiles
			   odd ones get extra 4 units(1.414 times) of distance/costs
			*/
			if (dir % 2 == 0)
			{
				currDist = psCurr->dist + 10;
			}
			else
			{
				currDist = psCurr->dist + 14;
			}

			// Try a new location
			x = psCurr->x + aDirOffset[dir].x;
			y = psCurr->y + aDirOffset[dir].y;


			// See if the node has already been visited
			psFound = fpathHashPresent(apsNodes, x,y);
			if (psFound && psFound->dist <= currDist)
			{
				// already visited node by a shorter route
// 				debug( LOG_NEVER, "blocked (%d)     : %3d, %3d dist %d\n", psFound->type, x, y, currDist );
				continue;
			}

			// If the tile hasn't been visited see if it is a blocking tile
			if (!psFound && fpathBlockingTile(x,y))
			{
				// tile is blocked, skip it
// 				debug( LOG_NEVER, "blocked          : %3d, %3d\n", x, y );
				continue;
			}

			astarInner += 1;

			// Now insert the point into the appropriate list
			if (!psFound)
			{
				// Not in open or closed lists - add to the open list
				psNew = fpathNewNode(x,y, currDist, psCurr);
				if (psNew)
				{
					psNew->est = (SWORD)fpathEstimate(x,y, tileFX, tileFY);
					fpathOpenAdd(psNew);
					fpathHashAdd(apsNodes, psNew);
// 					debug( LOG_NEVER, "new              : %3d, %3d (%d,%d) = %d\n", x, y, currDist, psNew->est, currDist + psNew->est );
				}
			}
			else if (psFound->type == NT_OPEN)
			{
				astarRemove += 1;

				// already in the open list but this is shorter
				psFound->dist = (SWORD)currDist;
				psFound->psRoute = psCurr;
// 				debug( LOG_NEVER, "replace open     : %3d, %3d dist %d\n", x, y, currDist, psFound->est, currDist + psFound->est );
			}
			else if (psFound->type == NT_CLOSED)
			{
				// already in the closed list but this is shorter
				psFound->type = NT_OPEN;
				psFound->dist = (SWORD)currDist;
				psFound->psRoute = psCurr;
				fpathOpenAdd(psFound);
// 				debug( LOG_NEVER, "replace closed   : %3d, %3d dist %d\n", x, y, currDist, psFound->est, currDist + psFound->est );
			}
			else
			{
				ASSERT(!"the open and closed lists are fried/wrong", "fpathAStarRoute: the open and closed lists are f***ed");
			}
		}

		// add the current point to the closed nodes
//		fpathHashAdd(apsClosed, psCurr);
		psCurr->type = NT_CLOSED;
// 		debug( LOG_NEVER, "HashAdd - closed : %3d,%3d (%d,%d) = %d\n", psCurr->pos.x, psCurr->pos.y, psCurr->dist, psCurr->est, psCurr->dist+psCurr->est );
	}

	retval = ASR_OK;

	// return the nearest route if no actual route was found
	if (!psRoute && psNearest)
//		!((psNearest->pos.x == tileSX) && (psNearest->pos.y == tileSY)))
	{
		psRoute = psNearest;
		fx = world_coord(psNearest->x) + TILE_UNITS / 2;
		fy = world_coord(psNearest->y) + TILE_UNITS / 2;
		retval = ASR_NEAREST;
	}

	if (psRoute)
	{
		// get the route in the correct order
		//	If as I suspect this is to reverse the list, then it's my suspicion that
		//	we could route from destination to source as opposed to source to
		//	destination. We could then save the reversal. to risky to try now...Alex M
		psParent = NULL;
		for(psCurr=psRoute; psCurr; psCurr=psNext)
		{
			psNext = psCurr->psRoute;
			psCurr->psRoute = psParent;
			psParent = psCurr;
		}
		psRoute = psParent;

		psCurr = psRoute;
		index = psRoutePoints->numPoints;
		while (psCurr && (index < ASTAR_MAXROUTE))
		{
			psRoutePoints->asPos[index].x =	psCurr->x;
			psRoutePoints->asPos[index].y =	psCurr->y;
			index += 1;
			psCurr = psCurr->psRoute;
		}
		psRoutePoints->numPoints = index;
		psRoutePoints->finalX = psRoutePoints->asPos[index-1].x;
		psRoutePoints->finalY = psRoutePoints->asPos[index-1].y;
	}
	else
	{
		retval = ASR_FAILED;
	}

	fpathHashReset();
	return retval;
}
