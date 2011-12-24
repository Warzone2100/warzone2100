/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
 *  How this works:
 *  * First time (in a given tick)  that some droid  wants to pathfind  to a particular
 *    destination,  the A*  algorithm from source to  destination is used.  The desired
 *    destination,  and the nearest  reachable point  to the  destination is saved in a
 *    Context.
 *  * Second time (in a given tick)  that some droid wants to  pathfind to a particular
 *    destination,  the appropriate  Context is found,  and the A* algorithm is used to
 *    find a path from the nearest reachable point to the destination  (which was saved
 *    earlier), to the source.
 *  * Subsequent times  (in a given tick) that some droid wants to pathfind to a parti-
 *    cular destination,  the path is looked up in appropriate Context.  If the path is
 *    not already known,  the A* weights are adjusted, and the previous A*  pathfinding
 *    is continued until the new source is reached.  If the new source is  not reached,
 *    the droid is  on a  different island than the previous droid,  and pathfinding is
 *    restarted from the first step.
 *  Up to 30 pathfinding maps from A* are cached, in a LRU list. The PathNode heap con-
 *  tains the  priority-heap-sorted  nodes which are to be explored.  The path back  is
 *  stored in the PathExploredTile 2D array of tiles.
 */

#ifndef WZ_TESTING
#include "lib/framework/frame.h"

#include "astar.h"
#include "map.h"
#endif

#include <list>
#include <vector>
#include <algorithm>

#include "lib/framework/crc.h"
#include "lib/netplay/netplay.h"

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
		if (est  != z.est)  return est  > z.est;
		if (dist != z.dist) return dist < z.dist;
		if (p.x  != z.p.x)  return p.x  < z.p.x;
		                    return p.y  < z.p.y;
	}

	PathCoord p;                    // Map coords.
	unsigned  dist, est;            // Distance so far and estimate to end.
};
struct PathExploredTile
{
	PathExploredTile() : iteration(0xFFFF) {}

	uint16_t iteration;
	int8_t   dx, dy;                // Offset from previous point in the route.
	unsigned dist;                  // Shortest known distance to tile.
	bool     visited;
};

struct PathBlockingType
{
	uint32_t gameTime;

	PROPULSION_TYPE propulsion;
	int owner;
	FPATH_MOVETYPE moveType;
};
/// Pathfinding blocking map
struct PathBlockingMap
{
	bool operator ==(PathBlockingType const &z) const
	{
		return type.gameTime == z.gameTime &&
		       fpathIsEquivalentBlocking(type.propulsion, type.owner, type.moveType,
		                                    z.propulsion,    z.owner,    z.moveType);
	}

	PathBlockingType type;
	std::vector<bool> map;
	std::vector<bool> dangerMap;	// using threatBits
};

struct PathNonblockingArea
{
	PathNonblockingArea() {}
	PathNonblockingArea(StructureBounds const &st) : x1(st.map.x), x2(st.map.x + st.size.x), y1(st.map.y), y2(st.map.y + st.size.y) {}
	bool operator ==(PathNonblockingArea const &z) const { return x1 == z.x1 && x2 == z.x2 && y1 == z.y1 && y2 == z.y2; }
	bool operator !=(PathNonblockingArea const &z) const { return !(*this == z); }
	bool isNonblocking(int x, int y) const
	{
		return x >= x1 && x < x2 && y >= y1 && y < y2;
	}

	int16_t x1, x2, y1, y2;
};

// Data structures used for pathfinding, can contain cached results.
struct PathfindContext
{
	PathfindContext() : myGameTime(0), iteration(0), blockingMap(NULL) {}
	bool isBlocked(int x, int y) const
	{
		if (srcIgnore.isNonblocking(x, y) || dstIgnore.isNonblocking(x, y))
		{
			return false;  // The path is actually blocked here by a structure, but ignore it since it's where we want to go (or where we came from).
		}
		// Not sure whether the out-of-bounds check is needed, can only happen if pathfinding is started on a blocking tile (or off the map).
		return x < 0 || y < 0 || x >= mapWidth || y >= mapHeight || blockingMap->map[x + y*mapWidth];
	}
	bool isDangerous(int x, int y) const
	{
		return !blockingMap->dangerMap.empty() && blockingMap->dangerMap[x + y*mapWidth];
	}
	bool matches(PathBlockingMap const *blockingMap_, PathCoord tileS_, PathNonblockingArea srcIgnore_, PathNonblockingArea dstIgnore_) const
	{
		// Must check myGameTime == blockingMap_->type.gameTime, otherwise blockingMap could be a deleted pointer which coincidentally compares equal to the valid pointer blockingMap_.
		return myGameTime == blockingMap_->type.gameTime && blockingMap == blockingMap_ && tileS == tileS_ && srcIgnore == srcIgnore_ && dstIgnore == dstIgnore_;
	}
	void assign(PathBlockingMap const *blockingMap_, PathCoord tileS_, PathNonblockingArea srcIgnore_, PathNonblockingArea dstIgnore_)
	{
		blockingMap = blockingMap_;
		tileS = tileS_;
		srcIgnore = srcIgnore_;
		dstIgnore = dstIgnore_;
		myGameTime = blockingMap->type.gameTime;
		nodes.clear();

		// Make the iteration not match any value of iteration in map.
		if (++iteration == 0xFFFF)
		{
			map.clear();  // There are no values of iteration guaranteed not to exist in map, so clear the map.
			iteration = 0;
		}
		map.resize(mapWidth * mapHeight);  // Allocate space for map, if needed.
	}

	PathCoord       tileS;                // Start tile for pathfinding. (May be either source or target tile.)
	uint32_t        myGameTime;

	PathCoord       nearestCoord;         // Nearest reachable tile to destination.

	/** Counter to implement lazy deletion from map.
	 *
	 *  @see fpathTableReset
	 */
	uint16_t        iteration;

	std::vector<PathNode> nodes;        ///< Edge of explored region of the map.
	std::vector<PathExploredTile> map;  ///< Map, with paths leading back to tileS.
	PathBlockingMap const *blockingMap; ///< Map of blocking tiles for the type of object which needs a path.
	PathNonblockingArea srcIgnore;      ///< Area of structure at source which should be considered nonblocking.
	PathNonblockingArea dstIgnore;      ///< Area of structure at destination which should be considered nonblocking.
};

/// Last recently used list of contexts.
static std::list<PathfindContext> fpathContexts;

/// Lists of blocking maps from current tick.
static std::list<PathBlockingMap> fpathBlockingMaps;
/// Lists of blocking maps from previous tick, will be cleared next tick (since it will be no longer needed after that).
static std::list<PathBlockingMap> fpathPrevBlockingMaps;
/// Game time for all blocking maps in fpathBlockingMaps.
static uint32_t fpathCurrentGameTime;

// Convert a direction into an offset
// dir 0 => x = 0, y = -1
static const Vector2i aDirOffset[] =
{
	Vector2i( 0, 1),
	Vector2i(-1, 1),
	Vector2i(-1, 0),
	Vector2i(-1,-1),
	Vector2i( 0,-1),
	Vector2i( 1,-1),
	Vector2i( 1, 0),
	Vector2i( 1, 1),
};

void fpathHardTableReset()
{
	fpathContexts.clear();
	fpathBlockingMaps.clear();
	fpathPrevBlockingMaps.clear();
}

/** Get the nearest entry in the open list
 */
/// Takes the current best node, and removes from the node heap.
static inline PathNode fpathTakeNode(std::vector<PathNode> &nodes)
{
	// find the node with the lowest distance
	// if equal totals, give preference to node closer to target
	PathNode ret = nodes.front();

	// remove the node from the list
	std::pop_heap(nodes.begin(), nodes.end());  // Move the best node from the front of nodes to the back of nodes, preserving the heap properties, setting the front to the next best node.
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
static inline void fpathNewNode(PathfindContext &context, PathCoord dest, PathCoord pos, unsigned prevDist, PathCoord prevPos)
{
	ASSERT_OR_RETURN(, (unsigned)pos.x < (unsigned)mapWidth && (unsigned)pos.y < (unsigned)mapHeight, "X (%d) or Y (%d) coordinate for path finding node is out of range!", pos.x, pos.y);

	// Create the node.
	PathNode node;
	unsigned costFactor = context.isDangerous(pos.x, pos.y) ? 5 : 1;
	node.p = pos;
	node.dist = prevDist + fpathEstimate(prevPos, pos)*costFactor;
	node.est = node.dist + fpathEstimate(pos, dest);

	PathExploredTile &expl = context.map[pos.x + pos.y*mapWidth];
	if (expl.iteration == context.iteration && (expl.visited || expl.dist <= node.dist))
	{
		return;  // Already visited this tile. Do nothing.
	}

	// Remember where we have been, and remember the way back.
	expl.iteration = context.iteration;
	expl.dx = pos.x - prevPos.x;
	expl.dy = pos.y - prevPos.y;
	expl.dist = node.dist;
	expl.visited = false;

	// Add the node to the node heap.
	context.nodes.push_back(node);                               // Add the new node to nodes.
	std::push_heap(context.nodes.begin(), context.nodes.end());  // Move the new node to the right place in the heap.
}

/// Recalculates estimates to new tileF tile.
static void fpathAStarReestimate(PathfindContext &context, PathCoord tileF)
{
	for (std::vector<PathNode>::iterator node = context.nodes.begin(); node != context.nodes.end(); ++node)
	{
		node->est = node->dist + fpathEstimate(node->p, tileF);
	}

	// Changing the estimates breaks the heap ordering. Fix the heap ordering.
	std::make_heap(context.nodes.begin(), context.nodes.end());
}

/// Returns nearest explored tile to tileF.
static PathCoord fpathAStarExplore(PathfindContext &context, PathCoord tileF)
{
	PathCoord       nearestCoord(0, 0);
	unsigned        nearestDist = 0xFFFFFFFF;

	// search for a route
	while (!context.nodes.empty())
	{
		PathNode node = fpathTakeNode(context.nodes);
		if (context.map[node.p.x + node.p.y*mapWidth].visited)
		{
			continue;  // Already been here.
		}
		context.map[node.p.x + node.p.y*mapWidth].visited = true;

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
		for (unsigned dir = 0; dir < ARRAY_SIZE(aDirOffset); ++dir)
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
				if (context.isBlocked(x, y))
				{
					continue;
				}
				x = node.p.x + aDirOffset[(dir + 7) % 8].x;
				y = node.p.y + aDirOffset[(dir + 7) % 8].y;
				if (context.isBlocked(x, y))
				{
					continue;
				}
			}

			// Try a new location
			int x = node.p.x + aDirOffset[dir].x;
			int y = node.p.y + aDirOffset[dir].y;

			// See if the node is a blocking tile
			if (context.isBlocked(x, y))
			{
				// tile is blocked, skip it
				continue;
			}

			// Now insert the point into the appropriate list, if not already visited.
			fpathNewNode(context, tileF, PathCoord(x, y), node.dist, node.p);
		}
	}

	return nearestCoord;
}

static void fpathInitContext(PathfindContext &context, PathBlockingMap const *blockingMap, PathCoord tileS, PathCoord tileRealS, PathCoord tileF, PathNonblockingArea srcIgnore, PathNonblockingArea dstIgnore)
{
	context.assign(blockingMap, tileS, srcIgnore, dstIgnore);

	// Add the start point to the open list
	fpathNewNode(context, tileF, tileRealS, 0, tileRealS);
	ASSERT(!context.nodes.empty(), "fpathNewNode failed to add node.");
}

ASR_RETVAL fpathAStarRoute(MOVE_CONTROL *psMove, PATHJOB *psJob)
{
	ASR_RETVAL      retval = ASR_OK;

	bool            mustReverse = true;

	const PathCoord tileOrig(map_coord(psJob->origX), map_coord(psJob->origY));
	const PathCoord tileDest(map_coord(psJob->destX), map_coord(psJob->destY));
	const PathNonblockingArea srcIgnore(psJob->srcStructure);
	const PathNonblockingArea dstIgnore(psJob->dstStructure);

	PathCoord endCoord;  // Either nearest coord (mustReverse = true) or orig (mustReverse = false).

	std::list<PathfindContext>::iterator contextIterator = fpathContexts.begin();
	for (contextIterator = fpathContexts.begin(); contextIterator != fpathContexts.end(); ++contextIterator)
	{
		if (!contextIterator->matches(psJob->blockingMap, tileDest, srcIgnore, dstIgnore))
		{
			// This context is not for the same droid type and same destination.
			continue;
		}

		// We have tried going to tileDest before.

		if (contextIterator->map[tileOrig.x + tileOrig.y*mapWidth].iteration == contextIterator->iteration
		 && contextIterator->map[tileOrig.x + tileOrig.y*mapWidth].visited)
		{
			// Already know the path from orig to dest.
			endCoord = tileOrig;
		}
		else
		{
			// Need to find the path from orig to dest, continue previous exploration.
			fpathAStarReestimate(*contextIterator, tileOrig);
			endCoord = fpathAStarExplore(*contextIterator, tileOrig);
		}

		if (endCoord != tileOrig)
		{
			// orig turned out to be on a different island than what this context was used for, so can't use this context data after all.
			continue;
		}

		mustReverse = false;  // We have the path from the nearest reachable tile to dest, to orig.
		break;  // Found the path! Don't search more contexts.
	}

	if (contextIterator == fpathContexts.end())
	{
		// We did not find an appropriate context. Make one.

		if (fpathContexts.size() < 30)
		{
			fpathContexts.push_back(PathfindContext());
		}
		--contextIterator;

		// Init a new context, overwriting the oldest one if we are caching too many.
		// We will be searching from orig to dest, since we don't know where the nearest reachable tile to dest is.
		fpathInitContext(*contextIterator, psJob->blockingMap, tileOrig, tileOrig, tileDest, srcIgnore, dstIgnore);
		endCoord = fpathAStarExplore(*contextIterator, tileDest);
		contextIterator->nearestCoord = endCoord;
	}

	PathfindContext &context = *contextIterator;

	// return the nearest route if no actual route was found
	if (context.nearestCoord != tileDest)
	{
		retval = ASR_NEAREST;
	}

	// Get route, in reverse order.
	static std::vector<Vector2i> path;  // Declared static to save allocations.
	path.clear();

	PathCoord newP;
	for (PathCoord p = endCoord; true; p = newP)
	{
		ASSERT_OR_RETURN(ASR_FAILED, tileOnMap(p.x, p.y), "Assigned XY coordinates (%d, %d) not on map!", (int)p.x, (int)p.y);
		ASSERT_OR_RETURN(ASR_FAILED, path.size() < (unsigned)mapWidth*mapHeight, "Pathfinding got in a loop.");

		path.push_back(Vector2i(world_coord(p.x) + TILE_UNITS / 2, world_coord(p.y) + TILE_UNITS / 2));

		PathExploredTile &tile = context.map[p.x + p.y*mapWidth];
		newP = PathCoord(p.x - tile.dx, p.y - tile.dy);
		if (p == context.tileS || p == newP)
		{
			break;  // We stopped moving, because we reached the destination or the closest reachable tile to context.tileS. Give up now.
		}
	}
	if (retval == ASR_OK)
	{
		// Found exact path, so use exact coordinates for last point, no reason to lose precision
		Vector2i v(psJob->destX, psJob->destY);
		if (mustReverse)
		{
			path.front() = v;
		}
		else
		{
			path.back() = v;
		}
	}

	// TODO FIXME once we can change numPoints to something larger than int
	psMove->numPoints = std::min<size_t>(INT32_MAX - 1, path.size());

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
	if (mustReverse)
	{
		// Copy the list, in reverse.
		std::copy(path.rbegin(), path.rend(), psMove->asPath);

		if (!context.isBlocked(tileOrig.x, tileOrig.y))  // If blocked, searching from tileDest to tileOrig wouldn't find the tileOrig tile.
		{
			// Next time, search starting from nearest reachable tile to the destination.
			fpathInitContext(context, psJob->blockingMap, tileDest, context.nearestCoord, tileOrig, srcIgnore, dstIgnore);
		}
	}
	else
	{
		// Copy the list.
		std::copy(path.begin(), path.end(), psMove->asPath);
	}

	// Move context to beginning of last recently used list.
	if (contextIterator != fpathContexts.begin())  // Not sure whether or not the splice is a safe noop, if equal.
	{
		fpathContexts.splice(fpathContexts.begin(), fpathContexts, contextIterator);
	}

	psMove->destination = psMove->asPath[path.size() - 1];

	return retval;
}

void fpathSetBlockingMap(PATHJOB *psJob)
{
	if (fpathCurrentGameTime != gameTime)
	{
		// New tick, remove maps which are no longer needed.
		fpathCurrentGameTime = gameTime;
		fpathPrevBlockingMaps.swap(fpathBlockingMaps);
		fpathBlockingMaps.clear();
	}

	// Figure out which map we are looking for.
	PathBlockingType type;
	type.gameTime = gameTime;
	type.propulsion = psJob->propulsion;
	type.owner = psJob->owner;
	type.moveType = psJob->moveType;

	// Find the map.
	std::list<PathBlockingMap>::iterator i = std::find(fpathBlockingMaps.begin(), fpathBlockingMaps.end(), type);
	if (i == fpathBlockingMaps.end())
	{
		// Didn't find the map, so i does not point to a map.
		fpathBlockingMaps.push_back(PathBlockingMap());
		--i;

		// i now points to an empty map with no data. Fill the map.
		i->type = type;
		std::vector<bool> &map = i->map;
		map.resize(mapWidth*mapHeight);
		uint32_t checksumMap = 0, checksumDangerMap = 0, factor = 0;
		for (int y = 0; y < mapHeight; ++y)
			for (int x = 0; x < mapWidth; ++x)
		{
			map[x + y*mapWidth] = fpathBaseBlockingTile(x, y, type.propulsion, type.owner, type.moveType);
			checksumMap ^= map[x + y*mapWidth]*(factor = 3*factor + 1);
		}
		if (!isHumanPlayer(type.owner) && type.moveType == FMT_MOVE)
		{
			std::vector<bool> &dangerMap = i->dangerMap;
			dangerMap.resize(mapWidth*mapHeight);
			for (int y = 0; y < mapHeight; ++y)
				for (int x = 0; x < mapWidth; ++x)
			{
				dangerMap[x + y*mapWidth] = auxTile(x, y, type.owner) & AUXBITS_THREAT;
				checksumDangerMap ^= map[x + y*mapWidth]*(factor = 3*factor + 1);
			}
		}
		syncDebug("blockingMap(%d,%d,%d,%d) = %08X %08X", gameTime, psJob->propulsion, psJob->owner, psJob->moveType, checksumMap, checksumDangerMap);
	}
	else
	{
		syncDebug("blockingMap(%d,%d,%d,%d) = cached", gameTime, psJob->propulsion, psJob->owner, psJob->moveType);
	}

	// i now points to the correct map. Make psJob->blockingMap point to it.
	psJob->blockingMap = &*i;
}
