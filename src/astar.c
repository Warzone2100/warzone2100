/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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

#ifndef WZ_TESTING
#include "lib/framework/frame.h"

#include "astar.h"
#include "map.h"
#endif

/** Counter to implement lazy deletion from nodeArray.
 *
 *  @see fpathTableReset
 */
static uint16_t resetIterationCount = 0;

/** The structure to store a node of the route in node table
 *
 *  @ingroup pathfinding
 */
typedef struct _fp_node
{
	struct _fp_node *psOpen;
	struct _fp_node	*psRoute;	// Previous point in the route
	uint16_t	iteration;
	short		x, y;           // map coords
	short		dist, est;	// distance so far and estimate to end
	short		type;		// open or closed node
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

	if (x < 0 || y < 0 || x >= ARRAY_SIZE(nodeArray) || y >= ARRAY_SIZE(nodeArray[x]))
	{
		return NULL;
	}

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
	if (resetIterationCount < UINT16_MAX - 1)
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

/** Add a node to the open list
 */
static inline void fpathOpenAdd(FP_NODE *psNode)
{
	psNode->psOpen = psOpen;
	psOpen = psNode;
}

/** Get the nearest entry in the open list
 */
static FP_NODE *fpathOpenGet(void)
{
	FP_NODE	*psNode, *psCurr, *psPrev, *psParent = NULL;

	if (psOpen == NULL)
	{
		return NULL;
	}

	// find the node with the lowest distance
	psPrev = NULL;
	psNode = psOpen;
	for(psCurr = psOpen; psCurr; psCurr = psCurr->psOpen)
	{
		const int first = psCurr->dist + psCurr->est;
		const int second = psNode->dist + psNode->est;

		// if equal totals, give preference to node closer to target
		if (first < second || (first == second && psCurr->est < psNode->est))
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
static SDWORD WZ_DECL_CONST fpathEstimate(SDWORD x, SDWORD y, SDWORD fx, SDWORD fy)
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

SDWORD fpathAStarRoute(MOVE_CONTROL *psMove, PATHJOB *psJob)
{
	FP_NODE		*psFound, *psCurr, *psNew;
	FP_NODE		*psNearest, *psRoute;
	SDWORD		dir, x, y, currDist;
	SDWORD		retval = ASR_OK;
	const int       tileSX = map_coord(psJob->origX);
	const int       tileSY = map_coord(psJob->origY);
	const int       tileFX = map_coord(psJob->destX);
	const int       tileFY = map_coord(psJob->destY);

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

	// search for a route
	while (psOpen != NULL)
	{
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

		// loop through possible moves in 8 directions to find a valid move
		for (dir = 0; dir < NUM_DIR; dir += 1)
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
				// We cannot cut corners
				x = psCurr->x + aDirOffset[(dir + 1) % 8].x;
				y = psCurr->y + aDirOffset[(dir + 1) % 8].y;
				if (fpathBaseBlockingTile(x, y, psJob->propulsion, psJob->owner, psJob->moveType))
				{
					continue;
				}
				x = psCurr->x + aDirOffset[(dir - 1) % 8].x;
				y = psCurr->y + aDirOffset[(dir - 1) % 8].y;
				if (fpathBaseBlockingTile(x, y, psJob->propulsion, psJob->owner, psJob->moveType))
				{
					continue;
				}

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
				continue;
			}

			// If the tile hasn't been visited see if it is a blocking tile
			if (!psFound && fpathBaseBlockingTile(x, y, psJob->propulsion, psJob->owner, psJob->moveType))
			{
				// tile is blocked, skip it
				continue;
			}

			// Now insert the point into the appropriate list
			if (!psFound)
			{
				// Not in open or closed lists - add to the open list
				psNew = fpathNewNode(x,y, currDist, psCurr);
				if (psNew)
				{
					psNew->est = fpathEstimate(x, y, tileFX, tileFY);
					fpathOpenAdd(psNew);
					fpathAddNode(psNew);
				}
			}
			else if (psFound->type == NT_OPEN)
			{
				// already in the open list but this is shorter
				psFound->dist = (SWORD)currDist;
				psFound->psRoute = psCurr;
			}
			else if (psFound->type == NT_CLOSED)
			{
				// already in the closed list but this is shorter
				psFound->type = NT_OPEN;
				psFound->dist = (SWORD)currDist;
				psFound->psRoute = psCurr;
				fpathOpenAdd(psFound);
			}
		}

		// add the current point to the closed nodes
		psCurr->type = NT_CLOSED;
	}

	// return the nearest route if no actual route was found
	if (!psRoute && psNearest)
	{
		psRoute = psNearest;
		retval = ASR_NEAREST;
	}

	if (psRoute)
	{
		int	count = 0;	// calculated length of route

		// Count length of route
		psCurr = psRoute;
		while (psCurr)
		{
			ASSERT(tileOnMap(psCurr->x, psCurr->y), "Assigned XY coordinates (%d, %d) not on map!", (int)psCurr->x, (int)psCurr->y);
			psCurr = psCurr->psRoute;
			count++;
		}

		// TODO FIXME once we can change numPoints to something larger than uchar
		psMove->numPoints = MIN(255, count);

		// Allocate memory
		psMove->asPath = calloc(sizeof(*psMove->asPath), count);
		ASSERT(psMove->asPath, "Out of memory");
		if (!psMove->asPath)
		{
			fpathHardTableReset();
			return ASR_FAILED;
		}

		// get the route in the correct order
		// If as I suspect this is to reverse the list, then it's my suspicion that
		// we could route from destination to source as opposed to source to
		// destination. We could then save the reversal. to risky to try now...Alex M
		//
		// The idea is impractical, because you can't guarentee that the target is
		// reachable. As I see it, this is the reason why psNearest got introduced.
		// -- Dennis L.
		psCurr = psRoute;
		if (psCurr && retval == ASR_OK)
		{
			// Found exact path, so use exact coordinates for last point, no reason to lose precision
			count--;
			psMove->asPath[count].x = psJob->destX;
			psMove->asPath[count].y = psJob->destY;
			psMove->DestinationX = psJob->destX;
			psMove->DestinationY = psJob->destY;
			psCurr = psCurr->psRoute;
		}
		else
		{
			psMove->DestinationX = world_coord(psCurr->x) + TILE_UNITS / 2;
			psMove->DestinationY = world_coord(psCurr->y) + TILE_UNITS / 2;
		}
		while (psCurr)
		{
			count--;
			psMove->asPath[count].x = world_coord(psCurr->x) + TILE_UNITS / 2;
			psMove->asPath[count].y = world_coord(psCurr->y) + TILE_UNITS / 2;
			psCurr = psCurr->psRoute;
		}
	}
	else
	{
		retval = ASR_FAILED;
	}

	fpathTableReset();
	return retval;
}
