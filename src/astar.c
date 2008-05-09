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

/** Keeps track of the amount of iterations done in the inner loop of our A*
 *  implementation.
 *
 *  @see FPATH_LOOP_LIMIT
 *
 *  @ingroup pathfinding
 */
int astarInner = 0;

/** Counter to implement lazy deletion from nodeArray.
 *
 *  @see fpathTableReset
 */
static int resetIterationCount = 0;

/** The structure to store a node of the route in node table
 *
 *  @ingroup pathfinding
 */
typedef struct _fp_node
{
	int     x, y;           // map coords
	SWORD	dist, est;	// distance so far and estimate to end
	SWORD	type;		// open or closed node
	struct _fp_node *psOpen;
	struct _fp_node	*psRoute;	// Previous point in the route
	struct _fp_node *psNext;
	int iteration;
} FP_NODE;

// types of node
#define NT_OPEN		1
#define NT_CLOSED	2

/** List of open nodes
 */
static FP_NODE* psOpen;

/** A map's maximum width & height
 */
#define MAX_MAP_SIZE (UINT8_MAX + 1)

/** Table of closed nodes
 */
static FP_NODE* nodeArray[MAX_MAP_SIZE][MAX_MAP_SIZE] = { { NULL }, { NULL } };

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

/** Add a node to the node table
 *
 *  @param psNode to add to the table
 */
static void fpathAddNode(FP_NODE* psNode)
{
	const int x = psNode->x;
	const int y = psNode->y;

	ASSERT(x < ARRAY_SIZE(nodeArray) && y < ARRAY_SIZE(nodeArray[x]), "X (%d) or Y %d) coordinate for path finding node is out of range!", x, y);

	// Lets not leak memory
	if (nodeArray[x][y])
	{
		free(nodeArray[x][y]);
	}

	nodeArray[x][y] = psNode;

	// Assign this node to the current iteration (this node will only remain
	// valid for as long as it's `iteration` member has the same value as
	// resetIterationCount.
	psNode->iteration = resetIterationCount;
}

/** See if a node is in the table
 *  Check whether there is a node for the given coordinates in the table
 *
 *  @param x,y the coordinates to check for
 *  @return a pointer to the node if one could be found, or NULL otherwise.
 */
static FP_NODE* fpathGetNode(int x, int y)
{
	FP_NODE * psFound;

	ASSERT(x < ARRAY_SIZE(nodeArray) && y < ARRAY_SIZE(nodeArray[x]), "X (%d) or Y %d) coordinate for path finding node is out of range!", x, y);

	psFound = nodeArray[x][y];
	if (psFound
	 && psFound->iteration == resetIterationCount)
	{
		return psFound;
	}

	return NULL;
}

/** Reset the node table
 *  
 *  @NOTE The actual implementation does a lazy reset, because resetting the
 *        entire node table is expensive.
 */
static void fpathTableReset(void)
{
	// Reset node table, simulate this by incrementing the iteration
	// counter, which will invalidate all nodes currently in the table. See
	// the implementation of fpathGetNode().
	++resetIterationCount;

	// Check to prevent overflows of resetIterationCount
	if (resetIterationCount < INT_MAX - 1)
	{
		ASSERT(resetIterationCount > 0, "Integer overflow occurred!");

		return;
	}

	// If we're about to overflow resetIterationCount, reset the entire
	// table for real (not lazy for once in a while) and start counting
	// at zero (0) again.
	fpathHardTableReset();
}

void fpathHardTableReset()
{
	int x, y;

	for (x = 0; x < ARRAY_SIZE(nodeArray); ++x)
	{
		for (y = 0; y < ARRAY_SIZE(nodeArray[x]); ++y)
		{
			if (nodeArray[x][y])
			{
				free(nodeArray[x][y]);
				nodeArray[x][y] = NULL;
			}
		}
	}

	resetIterationCount = 0;
}

/** Compare two nodes
 */
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

/** make a 50/50 random choice
 */
static BOOL fpathRandChoice(void)
{
	return ONEINTWO;
}

/** Add a node to the open list
 */
static void fpathOpenAdd(FP_NODE *psNode)
{
	psNode->psOpen = psOpen;
	psOpen = psNode;
}

/** Get the nearest entry in the open list
 */
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

/** Estimate the distance to the target point
 */
static SDWORD fpathEstimate(SDWORD x, SDWORD y, SDWORD fx, SDWORD fy)
{
	SDWORD xdiff, ydiff;

	xdiff = x > fx ? x - fx : fx - x;
	ydiff = y > fy ? y - fy : fy - y;

	xdiff = xdiff * 10;
	ydiff = ydiff * 10;

	return xdiff > ydiff ? xdiff + ydiff/2 : xdiff/2 + ydiff;
}

/** Generate a new node
 */
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

/** The visibility ray callback
 */
static BOOL fpathVisCallback(SDWORD x, SDWORD y, SDWORD dist, PROPULSION_TYPE propulsion)
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

	if (fpathBlockingTile(map_coord(x), map_coord(y), propulsion))
	{
		// found an obstruction
		obstruction = true;
		return false;
	}

	return true;
}

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

	rayCast(x1, y1, rayPointsToAngle(x1, y1, x2, y2), RAY_MAXLEN, INVALID_PROP_TYPE, fpathVisCallback);

	return !obstruction;
}

SDWORD fpathAStarRoute(SDWORD routeMode, MOVE_CONTROL *psMove, SDWORD sx, SDWORD sy, SDWORD fx, SDWORD fy, PROPULSION_TYPE propulsion)
{
 	FP_NODE		*psFound, *psCurr, *psNew, *psParent, *psNext;
static 	FP_NODE		*psNearest, *psRoute;
	SDWORD		dir, x,y, currDist;
	SDWORD		retval;
	const int       tileSX = map_coord(sx);
	const int       tileSY = map_coord(sy);
	const int       tileFX = map_coord(fx);
	const int       tileFY = map_coord(fy);

	if (routeMode == ASR_NEWROUTE)
	{
		fpathTableReset();

		// Add the start point to the open list
		psCurr = fpathNewNode(tileSX,tileSY, 0, NULL);
		if (!psCurr)
		{
			fpathTableReset();
			return ASR_FAILED;
		}
		// estimate the estimated distance/moves
		psCurr->est = (SWORD)fpathEstimate(psCurr->x, psCurr->y, tileFX, tileFY);
		psOpen = NULL;
		fpathOpenAdd(psCurr);
		fpathAddNode(psCurr);
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
			psFound = fpathGetNode(x, y);
			if (psFound && psFound->dist <= currDist)
			{
				// already visited node by a shorter route
// 				debug( LOG_NEVER, "blocked (%d)     : %3d, %3d dist %d\n", psFound->type, x, y, currDist );
				continue;
			}

			// If the tile hasn't been visited see if it is a blocking tile
			if (!psFound && fpathBlockingTile(x, y, propulsion))
			{
				// tile is blocked, skip it
// 				debug( LOG_NEVER, "blocked          : %3d, %3d\n", x, y );
				continue;
			}

			astarInner += 1;
			ASSERT(astarInner >= 0, "astarInner overflowed!");

			// Now insert the point into the appropriate list
			if (!psFound)
			{
				// Not in open or closed lists - add to the open list
				psNew = fpathNewNode(x,y, currDist, psCurr);
				if (psNew)
				{
					psNew->est = (SWORD)fpathEstimate(x,y, tileFX, tileFY);
					fpathOpenAdd(psNew);
					fpathAddNode(psNew);
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
//		fpathAddNode(psCurr);
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
		int index, count = psMove->numPoints;

		// get the route in the correct order
		// If as I suspect this is to reverse the list, then it's my suspicion that
		// we could route from destination to source as opposed to source to
		// destination. We could then save the reversal. to risky to try now...Alex M
		//
		// The idea is impractical, because you can't guarentee that the target is
		// reachable. As I see it, this is the reason why psNearest got introduced.
		// -- Dennis L.
		psParent = NULL;
		for(psCurr=psRoute; psCurr; psCurr=psNext)
		{
			psNext = psCurr->psRoute;
			psCurr->psRoute = psParent;
			psParent = psCurr;
			count++;
		}
		psRoute = psParent;

		psCurr = psRoute;
		psMove->asPath = realloc(psMove->asPath, sizeof(*psMove->asPath) * count);
		index = psMove->numPoints;
		while (psCurr && index < count)
		{
			psMove->asPath[index].x = psCurr->x;
			psMove->asPath[index].y = psCurr->y;
			index++;
			psCurr = psCurr->psRoute;
		}
		psMove->numPoints = index;
		psMove->DestinationX = psMove->asPath[index - 1].x;
		psMove->DestinationY = psMove->asPath[index - 1].y;
	}
	else
	{
		retval = ASR_FAILED;
	}

	fpathTableReset();
	return retval;
}
