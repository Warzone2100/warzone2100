/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_ASTART_H__
#define __INCLUDED_SRC_ASTART_H__

#include <vector>
#include <list>
#include <memory>

#include "baseobject.h"

/** return codes for astar
 *
 *  @ingroup pathfinding
 */
enum ASR_RETVAL
{
	ASR_OK,         ///< found a route
	ASR_FAILED,     ///< no route could be found
	ASR_NEAREST,    ///< found a partial route to a nearby position
};

/// A coordinate.
struct PathCoord
{
	PathCoord(): x(0), y(0) {}
	PathCoord(int16_t x_, int16_t y_) : x(x_), y(y_) {}
	bool operator ==(PathCoord const &z) const
	{
		return x == z.x && y == z.y;
	}
	bool operator !=(PathCoord const &z) const
	{
		return !(*this == z);
	}

	int16_t x, y;
};

/// Common type for all pathfinding costs.
using cost_t = unsigned;

/** The structure to store a node of the route in node table
 *
 *  @ingroup pathfinding
 */
struct PathNode
{
	bool operator <(PathNode const &z) const
	{
		// Sort descending est, fallback to ascending dist, fallback to sorting by position.
		if (est  != z.est)
		{
			return est  > z.est;
		}
		if (dist != z.dist)
		{
			return dist < z.dist;
		}
		if (p.x  != z.p.x)
		{
			return p.x  < z.p.x;
		}
		return p.y  < z.p.y;
	}

	PathCoord p;                    // Map coords.
	cost_t  dist, est;            // Distance so far and estimate to end.
};

struct PathExploredTile
{
	PathExploredTile() : iteration(0xFFFF), dx(0), dy(0), dist(0), visited(false) {}

	uint16_t iteration;
	int8_t   dx, dy;                // Offset from previous point in the route.
	cost_t   dist;                  // Shortest known distance to tile.
	bool     visited;
};

struct PathBlockingType
{
	uint32_t gameTime;

	PROPULSION_TYPE propulsion;
	int owner;
	/// It is the same as FPATH_MOVETYPE, but can have a wider meaning.
	int moveType;
};

/// Blocking map for PF wave exploration.
/// This map is extracted from game map and used in PF query.
struct PathBlockingMap
{
	PathBlockingType type;
	std::vector<bool> map;
	std::vector<bool> dangerMap;
	int width = 0;
	int height = 0;
	int tileShift = 0;

	PathCoord worldToMap(int x, int y) const;
	bool operator ==(PathBlockingType const &z) const;
	bool isBlocked(int x, int y) const {
		if (x < 0 || y < 0 || x >= width || y >= width)
			return true;
		return map[x + y*width];
	}
};

struct PathNonblockingArea
{
	PathNonblockingArea() {}
	PathNonblockingArea(StructureBounds const &st) : x1(st.map.x), x2(st.map.x + st.size.x), y1(st.map.y), y2(st.map.y + st.size.y) {}

	bool operator ==(PathNonblockingArea const &z) const
	{
		return x1 == z.x1 && x2 == z.x2 && y1 == z.y1 && y2 == z.y2;
	}

	bool operator !=(PathNonblockingArea const &z) const
	{
		return !(*this == z);
	}

	bool isNonblocking(int x, int y) const
	{
		return x >= x1 && x < x2 && y >= y1 && y < y2;
	}

	int16_t x1 = 0;
	int16_t x2 = 0;
	int16_t y1 = 0;
	int16_t y2 = 0;
};

struct PATHJOB;
/**
 * A cache of blocking maps for pathfinding system.
 */
class PathMapCache {
public:
	void clear();

	/// Assigns blocking map for PF job.
	/// It is expected to be called from the main thread.
	void assignBlockingMap(PATHJOB& psJob);

protected:
	/// Lists of blocking maps from current tick.
	std::vector<std::shared_ptr<PathBlockingMap>> fpathBlockingMaps;
	/// Game time for all blocking maps in fpathBlockingMaps.
	uint32_t fpathCurrentGameTime;
};

/// Data structures used for pathfinding, can contain cached results.
/// Separate context can
struct PathfindContext
{
	/// Test if this context can be reused for new search.
	bool matches(std::shared_ptr<PathBlockingMap> &blockingMap_, PathCoord tileS_, PathNonblockingArea dstIgnore_, bool reverse) const;

	/// Assign new search configuration to a context.
	void assign(std::shared_ptr<PathBlockingMap> &blockingMap_, PathCoord tileS_, PathNonblockingArea dstIgnore_, bool reverse);

	const PathExploredTile& tile(const PathCoord& pc) const {
		return map[pc.x + width * pc.y];
	}

	PathExploredTile& tile(const PathCoord& pc) {
		return map[pc.x + width * pc.y];
	}

	bool isTileExplored(const PathExploredTile& tile) const {
		return tile.iteration == iteration;
	}

	bool isTileVisited(const PathExploredTile& tile) const {
		return tile.iteration == iteration && tile.visited;
	}
	/// Start tile for pathfinding.
	/// It is used for context caching and reusing existing contexts for other units.
	/// (May be either source or target tile.)
	PathCoord       tileS;
	uint32_t        myGameTime = 0;

	/** Counter to implement lazy deletion from map.
	 *
	 *  @see fpathTableReset
	 */
	uint16_t        iteration = 0;

	/// Local geometry of a map.
	/// It can differ from dimensions of global map.
	int width = 0, height = 0;

	/// This is the context for reverse search.
	bool reverse = true;
	std::vector<PathNode> nodes;        ///< Edge of explored region of the map.
	std::vector<PathExploredTile> map;  ///< Map, with paths leading back to tileS.
	std::shared_ptr<PathBlockingMap> blockingMap; ///< Map of blocking tiles for the type of object which needs a path.
	PathNonblockingArea dstIgnore;      ///< Area of structure at destination which should be considered nonblocking.
};

/** Use the A* algorithm to find a path
 *
 *  @ingroup pathfinding
 */
ASR_RETVAL fpathAStarRoute(std::list<PathfindContext>& fpathContexts, MOVE_CONTROL *psMove, PATHJOB *psJob);

#endif // __INCLUDED_SRC_ASTART_H__
