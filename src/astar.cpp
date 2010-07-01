/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone Resurrection Project

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
 *  See http://en.wikipedia.org/wiki/A*_search_algorithm for more information.
 */

#ifndef WZ_TESTING
#include "lib/framework/frame.h"

#include "astar.h"
#include "map.h"
#endif

#include <vector>
#include <algorithm>

/** Counter to implement lazy deletion from nodeArray.
 *
 *  @see fpathTableReset
 */
static uint16_t resetIterationCount = 0;

/// A coordinate.
struct PathCoord
{
	PathCoord() {}
	PathCoord(int16_t x_, int16_t y_) : x(x_), y(y_) {}
	bool operator ==(PathCoord const &z) const { return x == z.x && y == z.y; }
	bool operator !=(PathCoord const &z) const { return !(*this == z); }

	int16_t x, y;
};

/** The structure to store a node of the route in node table
 *
 *  @ingroup pathfinding
 */
struct PathNode
{
	bool operator <(PathNode const &z) const
	{
		// Sort decending est, fallback to ascending dist, fallback to sorting by position.
		return est  > z.est ||
		      (est == z.est &&
		       dist  < z.dist ||
		      (dist == z.dist &&
		       p.x  < z.p.x ||
		      (p.x == z.p.x &&
		       p.y  < z.p.y)));
	}

	PathCoord p;                    // Map coords.
	unsigned  dist, est;            // Distance so far and estimate to end.
};
struct PathExploredNode
{
	uint16_t iteration;
	int8_t   dx, dy;                // Offset from previous point in the route.
	unsigned dist;                  // Shortest known distance to tile.
	bool     visited;
};

/** A map's maximum width & height
 */
#define MAX_MAP_SIZE (UINT8_MAX + 1)

/** Table of closed nodes
 */
static PathExploredNode nodeArray[MAX_MAP_SIZE][MAX_MAP_SIZE];

// Convert a direction into an offset
// dir 0 => x = 0, y = -1
static const Vector2i aDirOffset[] =
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
	if (resetIterationCount < UINT16_MAX)
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
			nodeArray[x][y].iteration = UINT16_MAX;
		}
	}

	resetIterationCount = 0;
}

/** Get the nearest entry in the open list
 */
/// Takes the current best node, and removes from the node tree.
static inline PathNode fpathTakeNode(std::vector<PathNode> &nodes)
{
	// find the node with the lowest distance
	// if equal totals, give preference to node closer to target
	PathNode ret = nodes.front();

	// remove the node from the list
	std::pop_heap(nodes.begin(), nodes.end());  // Move the best node from the front of nodes to the back of nodes, preserving the tree properties, setting the front to the next best node.
	nodes.pop_back();                           // Pop the best node (which we will be returning).

	return ret;
}

/** Estimate the distance to the target point
 */
static inline unsigned WZ_DECL_CONST fpathEstimate(PathCoord s, PathCoord f)
{
	// Cost of moving horizontal/vertical = 70, cost of moving diagonal = 99, 99/70 = 1.41428571... ≈ √2 = 1.41421356...
	unsigned xDelta = abs(s.x - f.x), yDelta = abs(s.y - f.y);
	return std::min(xDelta, yDelta)*(99 - 70) + std::max(xDelta, yDelta)*70;
}

/** Generate a new node
 */
static inline void fpathNewNode(std::vector<PathNode> &nodes, PathCoord dest, PathCoord pos, unsigned prevDist, PathCoord prevPos)
{
	ASSERT(pos.x < ARRAY_SIZE(nodeArray) && pos.y < ARRAY_SIZE(nodeArray[pos.x]), "X (%d) or Y (%d) coordinate for path finding node is out of range!", pos.x, pos.y);

	// Create the node.
	PathNode node;
	node.p = pos;
	node.dist = prevDist + fpathEstimate(prevPos, pos);
	node.est = node.dist + fpathEstimate(pos, dest);

	PathExploredNode &expl = nodeArray[pos.x][pos.y];
	if (expl.iteration == resetIterationCount && (expl.visited || expl.dist <= node.dist))
	{
		return;  // Already visited this tile. Do nothing.
	}

	// Remember where we have been, and remember the way back.
	expl.iteration = resetIterationCount;
	expl.dx = pos.x - prevPos.x;
	expl.dy = pos.y - prevPos.y;
	expl.dist = node.dist;
	expl.visited = false;

	// Add the node to the node tree.
	nodes.push_back(node);                       // Add the new node to nodes.
	std::push_heap(nodes.begin(), nodes.end());  // Move the new node to the right place in the tree.
}

static std::vector<PathNode> nodes;
static std::vector<Vector2i> path;

SDWORD fpathAStarRoute(MOVE_CONTROL *psMove, PATHJOB *psJob)
{
	PathCoord       nearestCoord(0, 0);
	unsigned        nearestDist = 0xFFFFFFFF;

	int             retval = ASR_OK;

	const PathCoord tileS(map_coord(psJob->origX), map_coord(psJob->origY));
	const PathCoord tileF(map_coord(psJob->destX), map_coord(psJob->destY));

	fpathTableReset();
	nodes.clear();  // Could declare nodes here, but making global to save allocations.

	// Add the start point to the open list
	fpathNewNode(nodes, tileF, tileS, 0, tileS);
	ASSERT(!nodes.empty(), "fpathNewNode failed to add node.");

	// search for a route
	while (!nodes.empty())
	{
		PathNode node = fpathTakeNode(nodes);
		if (nodeArray[node.p.x][node.p.y].visited)
		{
			continue;  // Already been here.
		}
		nodeArray[node.p.x][node.p.y].visited = true;

		// note the nearest node to the target so far
		if (node.est - node.dist < nearestDist)
		{
			nearestCoord = node.p;
			nearestDist = node.est - node.dist;
		}

		if (node.p == tileF)
		{
			// reached the target
			nearestCoord = node.p;
			break;
		}

		// loop through possible moves in 8 directions to find a valid move
		for (int dir = 0; dir < ARRAY_SIZE(aDirOffset); ++dir)
		{
			/*
			   5  6  7
			     \|/
			   4 -I- 0
			     /|\
			   3  2  1
			   odd:orthogonal-adjacent tiles even:non-orthogonal-adjacent tiles
			*/
			if (dir % 2 != 0)
			{
				int x, y;

				// We cannot cut corners
				x = node.p.x + aDirOffset[(dir + 1) % 8].x;
				y = node.p.y + aDirOffset[(dir + 1) % 8].y;
				if (fpathBaseBlockingTile(x, y, psJob->propulsion, psJob->owner, psJob->moveType))
				{
					continue;
				}
				x = node.p.x + aDirOffset[(dir + 7) % 8].x;
				y = node.p.y + aDirOffset[(dir + 7) % 8].y;
				if (fpathBaseBlockingTile(x, y, psJob->propulsion, psJob->owner, psJob->moveType))
				{
					continue;
				}
			}

			// Try a new location
			int x = node.p.x + aDirOffset[dir].x;
			int y = node.p.y + aDirOffset[dir].y;

			// See if the node is a blocking tile
			if (fpathBaseBlockingTile(x, y, psJob->propulsion, psJob->owner, psJob->moveType))
			{
				// tile is blocked, skip it
				continue;
			}

			// Now insert the point into the appropriate list, if not already visited.
			fpathNewNode(nodes, tileF, PathCoord(x, y), node.dist, node.p);
		}
	}
	ASSERT(nearestDist != 0xFFFFFFFF, "Failed to find a point closer to the destination than infinity.");

	// return the nearest route if no actual route was found
	if (nearestCoord != tileF)
	{
		retval = ASR_NEAREST;
	}

	// Get route, in reverse order.
	path.clear();  // Could declare path here, but making global to save allocations.
	for (PathCoord p = nearestCoord; p != tileS; p = PathCoord(p.x - nodeArray[p.x][p.y].dx, p.y - nodeArray[p.x][p.y].dy))
	{
		ASSERT(tileOnMap(p.x, p.y), "Assigned XY coordinates (%d, %d) not on map!", (int)p.x, (int)p.y);

		Vector2i v = {world_coord(p.x) + TILE_UNITS / 2, world_coord(p.y) + TILE_UNITS / 2};
		path.push_back(v);
	}
	if (path.empty())
	{
		// We are probably already in the destination tile. Go to the exact coordinates.
		Vector2i v = {psJob->destX, psJob->destY};
		path.push_back(v);
	}
	else if (retval == ASR_OK)
	{
		// Found exact path, so use exact coordinates for last point, no reason to lose precision
		Vector2i v = {psJob->destX, psJob->destY};
		path.front() = v;
	}

	// TODO FIXME once we can change numPoints to something larger than uint16_t
	psMove->numPoints = std::min<int>(UINT16_MAX, path.size());

	// Allocate memory
	psMove->asPath = static_cast<Vector2i *>(malloc(sizeof(*psMove->asPath) * path.size()));
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
	//
	// If many droids are heading towards the same destination, then destination
	// to source would be faster if reusing the information in nodeArray. --Cyp
	std::copy(path.rbegin(), path.rend(), psMove->asPath);

	psMove->DestinationX = path.back().x;
	psMove->DestinationY = path.back().y;

	return retval;
}
