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
#include <memory>
#include <iterator>
#include <cstddef>

#include "lib/netplay/sync_debug.h"
#include "game_world.h"
#include "congestion_overlay.h"
#include "pathfinding_backend.h"
#include "corridor_map.h"

#if WZ_PATHFINDING_INSTRUMENTATION
uint64_t *g_pathNodesExpanded = nullptr;
#endif

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
	unsigned  dist, est;            // Distance so far and estimate to end.
};
struct PathExploredTile
{
	PathExploredTile() : iteration(0xFFFF), dx(0), dy(0), dist(0), visited(false) {}

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

// Data structures used for pathfinding, can contain cached results.
struct PathfindContext
{
	PathfindContext() : myGameTime(0), iteration(0), blockingMap(nullptr) {}
	bool isBlocked(int x, int y) const
	{
		if (dstIgnore.isNonblocking(x, y))
		{
			return false;  // The path is actually blocked here by a structure, but ignore it since it's where we want to go (or where we came from).
		}
		// Not sure whether the out-of-bounds check is needed, can only happen if pathfinding is started on a blocking tile (or off the map).
		return x < 0 || y < 0 || x >= gameWorld.map.width || y >= gameWorld.map.height || blockingMap->map[x + y * gameWorld.map.width];
	}
	bool isDangerous(int x, int y) const
	{
		return !blockingMap->dangerMap.empty() && blockingMap->dangerMap[x + y * gameWorld.map.width];
	}
	bool matches(const std::shared_ptr<const PathBlockingMap> &blockingMap_, const std::shared_ptr<const DynamicCostOverlay> &overlay_, PathCoord tileS_, PathNonblockingArea dstIgnore_) const
	{
		// Must check myGameTime == blockingMap_->type.gameTime, otherwise blockingMap could be a deleted pointer which coincidentally compares equal to the valid pointer blockingMap_.
		// The overlay pointer is part of the key too, so a context built for one cohort's traffic is never reused for another's.
		return myGameTime == blockingMap_->type.gameTime && blockingMap == blockingMap_ && overlay == overlay_ && tileS == tileS_ && dstIgnore == dstIgnore_;
	}
	void assign(const std::shared_ptr<const PathBlockingMap> &blockingMap_, const std::shared_ptr<const DynamicCostOverlay> &overlay_, PathCoord tileS_, PathNonblockingArea dstIgnore_)
	{
		blockingMap = blockingMap_;
		overlay = overlay_;
		tileS = tileS_;
		dstIgnore = dstIgnore_;
		myGameTime = blockingMap->type.gameTime;
		nodes.clear();

		// Make the iteration not match any value of iteration in map.
		if (++iteration == 0xFFFF)
		{
			map.clear();  // There are no values of iteration guaranteed not to exist in map, so clear the map.
			iteration = 0;
		}
		map.resize(static_cast<size_t>(gameWorld.map.width) * static_cast<size_t>(gameWorld.map.height));  // Allocate space for map, if needed.
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
	std::shared_ptr<const PathBlockingMap> blockingMap; ///< Map of blocking tiles for the type of object which needs a path.
	std::shared_ptr<const DynamicCostOverlay> overlay;  ///< Flow soft cost consumed by fpathNewNode, null for the legacy backend.
	PathNonblockingArea dstIgnore;      ///< Area of structure at destination which should be considered nonblocking.
};

/// Lists of blocking maps from current tick.
static std::vector<std::shared_ptr<PathBlockingMap>> fpathBlockingMaps;
/// Game time for all blocking maps in fpathBlockingMaps.
static uint32_t fpathCurrentGameTime;

// Convert a direction into an offset
// dir 0 => x = 0, y = -1
static const Vector2i aDirOffset[] =
{
	Vector2i(0, 1),
	Vector2i(-1, 1),
	Vector2i(-1, 0),
	Vector2i(-1, -1),
	Vector2i(0, -1),
	Vector2i(1, -1),
	Vector2i(1, 0),
	Vector2i(1, 1),
};

void fpathHardTableReset()
{
	fpathBlockingMaps.clear();
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
static inline unsigned WZ_DECL_PURE fpathEstimate(PathCoord s, PathCoord f)
{
	// Cost of moving horizontal/vertical = 70*2, cost of moving diagonal = 99*2, 99/70 = 1.41428571... ≈ √2 = 1.41421356...
	unsigned xDelta = abs(s.x - f.x), yDelta = abs(s.y - f.y);
	return std::min(xDelta, yDelta) * (198 - 140) + std::max(xDelta, yDelta) * 140;
}
static inline unsigned WZ_DECL_PURE fpathGoodEstimate(PathCoord s, PathCoord f)
{
	// Cost of moving horizontal/vertical = 70*2, cost of moving diagonal = 99*2, 99/70 = 1.41428571... ≈ √2 = 1.41421356...
	return iHypot((s.x - f.x) * 140, (s.y - f.y) * 140);
}

/** Generate a new node
 */
// Flow-cost scale: with facing vectors summed at 64 per body and edge deltas
// at 64 per axis, one opposing body met head-on prices about one tile step.
constexpr int32_t FLOW_COST_SHIFT = 5;
constexpr int32_t FLOW_COST_CAP = 1120;   // at most eight steps of cost from one tile's flow
constexpr int32_t FLOW_DEST_EXEMPT = 840; // no flow pricing within ~6 tiles of the search's own goal
constexpr int32_t MASS_COST_CAP = 1120;   // parity with the flow cap, at most eight steps from one tile's crowd
constexpr int32_t MASS_START_EXEMPT = 420; // no mass pricing within ~3 tiles of the search's start

static inline void fpathNewNode(PathfindContext &context, PathCoord dest, PathCoord pos, unsigned prevDist, PathCoord prevPos)
{
	ASSERT_OR_RETURN(, (unsigned)pos.x < (unsigned)gameWorld.map.width && (unsigned)pos.y < (unsigned)gameWorld.map.height, "X (%d) or Y (%d) coordinate for path finding node is out of range!", pos.x, pos.y);

	// Create the node.
	PathNode node;
	unsigned costFactor = context.isDangerous(pos.x, pos.y) ? 5 : 1;
	Vector2i delta = Vector2i(pos.x - prevPos.x, pos.y - prevPos.y) * 64;
	bool isDiagonal = delta.x && delta.y;
	// Soft cost for arriving at this tile, added on top of the step. Flow
	// prices arriving AGAINST the facing of the units holding the tile, so it
	// depends on the arrival direction, and any tile carrying flow must also
	// skip the interpolation below, whose arithmetic assumes
	// direction-independent step costs.
	unsigned overlayCost = 0;
	bool tileHasFlow = false;
	if (context.overlay)
	{
		const size_t tileIdx = static_cast<size_t>(pos.x) + static_cast<size_t>(pos.y) * gameWorld.map.width;
		if (!context.overlay->flowX.empty())
		{
			const int32_t fx = context.overlay->flowX[tileIdx];
			const int32_t fy = context.overlay->flowY[tileIdx];
			if (context.overlay->consumeFlow)
			{
				tileHasFlow = fx != 0 || fy != 0;
				const int32_t dot = delta.x * fx + delta.y * fy;
				// No flow pricing close to this search's own destination: a goal
				// crowd faces outward, and charging arrivals for their own crowd
				// sends them circling it instead of parking. Note the cached
				// context can run the search reversed, in which case this exempts
				// the origin side instead - measured better than exempting both.
				if (dot < 0 && fpathGoodEstimate(pos, dest) > FLOW_DEST_EXEMPT)
				{
					overlayCost += std::min<int32_t>(FLOW_COST_CAP, -dot >> FLOW_COST_SHIFT);
				}
			}
			if (context.overlay->consumeMass && !context.overlay->mass.empty())
			{
				// Mass prices only tiles whose occupants are not moving
				// coherently. An aligned column's summed facing keeps its L1
				// length near or above the tile's mass, a parked or canceled
				// crowd's falls far below, so single bodies and columns stay
				// free and only real crowds pay. The cost is the same from
				// every arrival direction, so context sharing stays exact.
				// Not destination-exempted: symmetric cost spreads approaches
				// around a goal crowd, the circling risk is specific to
				// directional cost.
				// A droid standing inside a parked crowd must plan out through
				// its neighbors: with those tiles priced, every escape route
				// detours around bodies it could shove past, the first
				// waypoints never clear, and the blocked watchdog cycles it
				// against its own crowd for the rest of the run. The radius
				// keys on the context's start tile, so shared contexts stay
				// valid, and a reversed context exempts the goal side instead,
				// which measured harmless: the crowd tiles beyond the radius
				// still price every approach.
				const int32_t m = context.overlay->mass[tileIdx];
				if (m > 0 && (abs(fx) + abs(fy)) * 2 < m
				    && fpathGoodEstimate(pos, context.tileS) > MASS_START_EXEMPT)
				{
					overlayCost += std::min<int32_t>(MASS_COST_CAP, m);
				}
			}
		}
	}
	node.p = pos;
	node.dist = prevDist + fpathEstimate(prevPos, pos) * costFactor + overlayCost;
	node.est = node.dist + fpathGoodEstimate(pos, dest);

	PathExploredTile &expl = context.map[pos.x + pos.y * gameWorld.map.width];
	if (expl.iteration == context.iteration)
	{
		if (expl.visited)
		{
			return;  // Already visited this tile. Do nothing.
		}
		Vector2i deltaA = delta;
		Vector2i deltaB = Vector2i(expl.dx, expl.dy);
		Vector2i deltaDelta = deltaA - deltaB;  // Vector pointing from current considered source tile leading to pos, to the previously considered source tile leading to pos.
		// Skip the interpolation when this tile carries overlay cost. It smooths a
		// path by backing the step cost out of the distance, which the extra
		// overlay term would throw off. The overlay cost is the same from either
		// source tile, so both visits skip together and the search stays exact.
		if (overlayCost == 0 && !tileHasFlow && abs(deltaDelta.x) + abs(deltaDelta.y) == 64)
		{
			// prevPos is tile A or B, and pos is tile P. We were previously called with prevPos being tile B or A, and pos tile P.
			// We want to find the distance to tile P, taking into account that the actual shortest path involves coming from somewhere between tile A and tile B.
			// +---+---+
			// |   | P |
			// +---+---+
			// | A | B |
			// +---+---+
			unsigned distA = node.dist - (isDiagonal ? 198 : 140) * costFactor; // If isDiagonal, node is A and expl is B.
			unsigned distB = expl.dist - (isDiagonal ? 140 : 198) * costFactor;
			if (!isDiagonal)
			{
				std::swap(distA, distB);
				std::swap(deltaA, deltaB);
			}
			int gradientX = int(distB - distA) / costFactor;
			if (gradientX > 0 && gradientX <= 98)  // 98 = floor(140/√2), so gradientX <= 98 is needed so that gradientX < gradientY.
			{
				// The distance gradient is now known to be somewhere between the direction from A to P and the direction from B to P.
				static const uint8_t gradYLookup[99] = {140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 140, 139, 139, 139, 139, 139, 139, 139, 139, 139, 138, 138, 138, 138, 138, 138, 137, 137, 137, 137, 137, 136, 136, 136, 136, 135, 135, 135, 134, 134, 134, 134, 133, 133, 133, 132, 132, 132, 131, 131, 130, 130, 130, 129, 129, 128, 128, 127, 127, 126, 126, 126, 125, 125, 124, 123, 123, 122, 122, 121, 121, 120, 119, 119, 118, 118, 117, 116, 116, 115, 114, 113, 113, 112, 111, 110, 110, 109, 108, 107, 106, 106, 105, 104, 103, 102, 101, 100};
				int gradientY = gradYLookup[gradientX];  // = sqrt(140² -  gradientX²), rounded to nearest integer
				unsigned distP = gradientY * costFactor + distB;
				node.est -= node.dist - distP;
				node.dist = distP;
				delta = (deltaA * gradientX + deltaB * (gradientY - gradientX)) / gradientY;
			}
		}
		if (expl.dist <= node.dist)
		{
			return;  // A different path to this tile is shorter.
		}
	}

	// Remember where we have been, and remember the way back.
	expl.iteration = context.iteration;
	expl.dx = delta.x;
	expl.dy = delta.y;
	expl.dist = node.dist;
	expl.visited = false;

	// Add the node to the node heap.
	context.nodes.push_back(node);                               // Add the new node to nodes.
	std::push_heap(context.nodes.begin(), context.nodes.end());  // Move the new node to the right place in the heap.
}

/// Recalculates estimates to new tileF tile.
static void fpathAStarReestimate(PathfindContext &context, PathCoord tileF)
{
	for (auto &node : context.nodes)
	{
		node.est = node.dist + fpathGoodEstimate(node.p, tileF);
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
	bool foundIt = false;
	while (!context.nodes.empty() && !foundIt)
	{
		PathNode node = fpathTakeNode(context.nodes);
		if (context.map[node.p.x + node.p.y * gameWorld.map.width].visited)
		{
			continue;  // Already been here.
		}
		context.map[node.p.x + node.p.y * gameWorld.map.width].visited = true;

#if WZ_PATHFINDING_INSTRUMENTATION
		if (g_pathNodesExpanded)
		{
			++*g_pathNodesExpanded;
		}
#endif

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
			foundIt = true;  // Break out of loop, but not before inserting neighbour nodes, since the neighbours may be important if the context gets reused.
		}

		// loop through possible moves in 8 directions to find a valid move
		for (unsigned dir = 0; dir < ARRAY_SIZE(aDirOffset); ++dir)
		{
			// Try a new location
			int x = node.p.x + aDirOffset[dir].x;
			int y = node.p.y + aDirOffset[dir].y;

			/*
			   5  6  7
			     \|/
			   4 -I- 0
			     /|\
			   3  2  1
			   odd:orthogonal-adjacent tiles even:non-orthogonal-adjacent tiles
			*/
			if (dir % 2 != 0 && !context.dstIgnore.isNonblocking(node.p.x, node.p.y) && !context.dstIgnore.isNonblocking(x, y))
			{
				int x2, y2;

				// We cannot cut corners
				x2 = node.p.x + aDirOffset[(dir + 1) % 8].x;
				y2 = node.p.y + aDirOffset[(dir + 1) % 8].y;
				if (context.isBlocked(x2, y2))
				{
					continue;
				}
				x2 = node.p.x + aDirOffset[(dir + 7) % 8].x;
				y2 = node.p.y + aDirOffset[(dir + 7) % 8].y;
				if (context.isBlocked(x2, y2))
				{
					continue;
				}
			}

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

static void fpathInitContext(PathfindContext &context, const std::shared_ptr<const PathBlockingMap> &blockingMap, const std::shared_ptr<const DynamicCostOverlay> &overlay, PathCoord tileS, PathCoord tileRealS, PathCoord tileF, PathNonblockingArea dstIgnore)
{
	context.assign(blockingMap, overlay, tileS, dstIgnore);

	// Add the start point to the open list
	fpathNewNode(context, tileF, tileRealS, 0, tileRealS);
	ASSERT(!context.nodes.empty(), "fpathNewNode failed to add node.");
}

class PathfindContextList
{
public:
	struct Iterator
	{
		using iterator_category = std::random_access_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = PathfindContext;
		using pointer = value_type*;
		using reference = value_type&;

		Iterator(PathfindContextList& list, size_t idx) : m_list(list), m_idx(idx)
		{}

		Iterator(const Iterator& other)
		: m_list(other.m_list)
		, m_idx(other.m_idx)
		{}

		Iterator& operator=(const Iterator& other)
		{
			m_list = other.m_list;
			m_idx = other.m_idx;
			return *this;
		}

		reference operator*() const { return m_list.contexts[m_list.orderedIndexes[m_idx]]; }
		pointer operator->() const { return &m_list.contexts[m_list.orderedIndexes[m_idx]]; }

		Iterator& operator++() { ++m_idx; return *this; }
		Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

		Iterator& operator--() { --m_idx; return *this; }
		Iterator operator--(int) { Iterator tmp = *this; --(*this); return tmp; }

		Iterator operator+(difference_type n) const { return Iterator(m_list, m_idx + n); }
		Iterator operator-(difference_type n) const { return Iterator(m_list, m_idx - n); }
		difference_type operator-(const Iterator& other) const { return m_idx - other.m_idx; }
		Iterator& operator+=(difference_type n) { m_idx += n; return *this; }
		Iterator& operator-=(difference_type n) { m_idx -= n; return *this; }

		reference operator[](difference_type n) const { return m_list.contexts[m_list.orderedIndexes[n]]; }

		bool operator== (const Iterator& other) const { return m_idx == other.m_idx; }
		bool operator!= (const Iterator& other) const { return m_idx != other.m_idx; }
		bool operator< (const Iterator& other) const { return m_idx < other.m_idx; }
		bool operator> (const Iterator& other) const { return m_idx > other.m_idx; }
		bool operator<= (const Iterator& other) const { return m_idx <= other.m_idx; }
		bool operator>= (const Iterator& other) const { return m_idx >= other.m_idx; }

	private:
		friend class PathfindContextList;

		PathfindContextList& m_list;
		size_t m_idx;
	};

public:

	Iterator push_back(PathfindContext& ctx);
	Iterator push_back(PathfindContext&& ctx);

	void moveToFront(Iterator it); // invalidates any iterators

	Iterator begin() { return Iterator(*this, 0); }
	Iterator end() { return Iterator(*this, orderedIndexes.size()); }

	void clear();

	bool empty() const { return contexts.empty(); }
	PathfindContext& front() { return contexts[orderedIndexes.front()]; }

	size_t size() const { return contexts.size(); }

private:
	std::vector<PathfindContext> contexts;
	std::vector<size_t> orderedIndexes;
};

PathfindContextList::Iterator PathfindContextList::push_back(PathfindContext& ctx)
{
	contexts.push_back(ctx);
	orderedIndexes.push_back(contexts.size() - 1);
	return Iterator(*this, orderedIndexes.size() - 1);
}

PathfindContextList::Iterator PathfindContextList::push_back(PathfindContext&& ctx)
{
	contexts.push_back(std::move(ctx));
	orderedIndexes.push_back(contexts.size() - 1);
	return Iterator(*this, orderedIndexes.size() - 1);
}

void PathfindContextList::moveToFront(Iterator it)
{
	auto oIt = orderedIndexes.begin() + it.m_idx;
	std::rotate(orderedIndexes.begin(), oIt, oIt + 1);
}

void PathfindContextList::clear()
{
	contexts.clear();
	orderedIndexes.clear();
}

class FPathExecuteContextImpl : public FPathExecuteContext
{
public:
	virtual ~FPathExecuteContextImpl();

	void resetForNewGameTimeIfNeeded(const PATHJOB& job);
public:
	/// Last recently used list of contexts.
	PathfindContextList fpathContexts;
	/// Used to avoid extra allocations in fpathAStarRoute
	std::vector<Vector2i> pathBuffer;
};

FPathExecuteContext::~FPathExecuteContext()
{ }

FPathExecuteContextImpl::~FPathExecuteContextImpl()
{ }

void FPathExecuteContextImpl::resetForNewGameTimeIfNeeded(const PATHJOB& job)
{
	if (!fpathContexts.empty() && job.blockingMap->type.gameTime != fpathContexts.front().myGameTime)
	{
		fpathContexts.clear();
	}
}

std::shared_ptr<FPathExecuteContext> makeFPathExecuteContext()
{
	return std::make_shared<FPathExecuteContextImpl>();
}

// ---- route shaping ----
// Accepted routes are shaped on the path worker threads after the search: the
// spine stands off walls, then each direction offsets to its own right so
// opposing flows sharing a line ride parallel lanes with no crossing points.
// A pure function of the path, the job's blocking snapshot and the load-time
// corridor map, so it is deterministic, identical on every client and stable
// under replanning.

constexpr int32_t WALL_CLEAR = 448;                 // target clearance between the spine and blocking ground
constexpr int32_t KEEP_RIGHT_HALF = 192;            // lateral shift to each route's right, half the opposing separation
constexpr int32_t SUBLANE_STEP = 64;                // id-hashed sub-lane spread around the lane line, so a
                                                    // direction's column marches several files wide, not one
constexpr int32_t KEEP_RIGHT_RAMP = 384;            // the shift ramps in over this much arc from the route start
constexpr int32_t KEEP_RIGHT_DEST_RAMP = 1024;      // and back out over this much before the destination, converging
                                                    // approaches rotate their offsets so crowds must gather unshifted

static bool bendBlockedTile(const PATHJOB *psJob, int tx, int ty)
{
	if (tx < 0 || ty < 0 || tx >= gameWorld.map.width || ty >= gameWorld.map.height)
	{
		return true;
	}
	return psJob->blockingMap->map[tx + ty * gameWorld.map.width];
}

/// The largest outward offset up to want, in 32wu steps, whose landing tile
/// and the tile half a step beyond are both clear, so a shaped point never
/// lands against a wall face. Zero when even the smallest step is blocked.
static int32_t bendAllowedOffset(const PATHJOB *psJob, Vector2i p, Vector2i outwardStep, int32_t want)
{
	for (int32_t o = want; o > 0; o -= 32)
	{
		Vector2i q(p.x + outwardStep.x * o / TILE_UNITS, p.y + outwardStep.y * o / TILE_UNITS);
		Vector2i qm = map_coord(q);
		Vector2i qb = map_coord(Vector2i(q.x + outwardStep.x / 2, q.y + outwardStep.y / 2));
		if (!bendBlockedTile(psJob, qm.x, qm.y) && !bendBlockedTile(psJob, qb.x, qb.y))
		{
			return o;
		}
	}
	return 0;
}

constexpr int32_t WALL_PROBE_STEP = 64;
constexpr int32_t WALL_KEEP = 320;                  // lane offsets stop this far from a wall face

/// Distance from a point to the first blocking tile along a one-tile
/// perpendicular step, probed coarsely, capped just past WALL_CLEAR.
static int32_t wallDistance(const PATHJOB *psJob, Vector2i p, Vector2i stepT)
{
	for (int32_t d = WALL_PROBE_STEP; d <= WALL_CLEAR; d += WALL_PROBE_STEP)
	{
		Vector2i t = map_coord(Vector2i(p.x + stepT.x * d / TILE_UNITS, p.y + stepT.y * d / TILE_UNITS));
		if (bendBlockedTile(psJob, t.x, t.y))
		{
			return d;
		}
	}
	return WALL_CLEAR + WALL_PROBE_STEP;
}

/// Stands the whole route off walls: wherever the path runs beside blocking
/// ground with open ground on the other side, the spine moves away from the
/// wall toward a target clearance, and where walls flank both sides it edges
/// toward the middle. String-pulled routes hug walls along every leg of a
/// corner, not only at the bend, so a bend-scoped standoff leaves the legs
/// scraping - this pass replaces it. It keeps off claimed corridor ground,
/// ramps at the endpoints, probes clearance and limits slope like the lane
/// pass that runs after it, so both directions offset from one
/// wall-respecting spine.
static void fpathWallClearance(const PATHJOB *psJob, MOVE_CONTROL *psMove)
{
	std::vector<Vector2i> &path = psMove->asPath;
	const size_t n = path.size();
	if (n < 4)
	{
		return;
	}
	const CorridorMap *cmap = gameWorld.map.corridors.get();

	static thread_local std::vector<int32_t> arcFwd;
	arcFwd.assign(n, 0);
	for (size_t k = 1; k < n; ++k)
	{
		arcFwd[k] = arcFwd[k - 1] + iHypot(path[k] - path[k - 1]);
	}
	const int32_t total = arcFwd[n - 1];
	if (total < KEEP_RIGHT_RAMP + KEEP_RIGHT_DEST_RAMP)
	{
		return;
	}

	static thread_local std::vector<int32_t> want;
	static thread_local std::vector<Vector2i> awayStep;
	want.assign(n, 0);
	awayStep.assign(n, Vector2i(0, 0));

	for (size_t k = 1; k + 1 < n; ++k)
	{
		Vector2i d = path[k + 1] - path[k - 1];
		int32_t len = iHypot(d);
		if (len == 0)
		{
			continue;
		}
		Vector2i t = map_coord(path[k]);
		if (cmap != nullptr && cmap->claimed(t.x, t.y))
		{
			continue;
		}
		Vector2i right(-d.y * TILE_UNITS / len, d.x * TILE_UNITS / len);
		if (right.x == 0 && right.y == 0)
		{
			continue;
		}
		Vector2i left(-right.x, -right.y);
		const int32_t dR = wallDistance(psJob, path[k], right);
		const int32_t dL = wallDistance(psJob, path[k], left);
		if (dR > WALL_CLEAR && dL > WALL_CLEAR)
		{
			continue;   // open ground both sides
		}
		int32_t desired;
		Vector2i away;
		if (dR <= WALL_CLEAR && dL <= WALL_CLEAR)
		{
			// walls both sides: edge toward the middle
			desired = (std::max(dR, dL) - std::min(dR, dL)) / 2;
			away = dR < dL ? left : right;
		}
		else
		{
			const int32_t nearDist = std::min(dR, dL);
			desired = WALL_CLEAR - nearDist;
			away = dR < dL ? left : right;
		}
		if (desired <= 0)
		{
			continue;
		}
		desired = std::min(desired, desired * arcFwd[k] / KEEP_RIGHT_RAMP);
		desired = std::min(desired, desired * (total - arcFwd[k]) / KEEP_RIGHT_DEST_RAMP);
		if (desired <= 0)
		{
			continue;
		}
		awayStep[k] = away;
		want[k] = bendAllowedOffset(psJob, path[k], away, desired);
	}

	// Slope limit at 1/2 against zero boundaries, steeper than the lane pass
	// so the clearance can develop over the few tiles a leg gives it.
	for (size_t k = 1; k + 1 < n; ++k)
	{
		int32_t seg = arcFwd[k] - arcFwd[k - 1];
		want[k] = std::min(want[k], want[k - 1] + seg / 2);
	}
	for (size_t k = n - 1; k-- > 1; )
	{
		int32_t seg = arcFwd[k + 1] - arcFwd[k];
		want[k] = std::min(want[k], want[k + 1] + seg / 2);
	}

	for (size_t k = 1; k + 1 < n; ++k)
	{
		if (want[k] > 0)
		{
			path[k].x += awayStep[k].x * want[k] / TILE_UNITS;
			path[k].y += awayStep[k].y * want[k] / TILE_UNITS;
		}
	}

}


/// Shifts the whole route to its own right on unclaimed ground, so two
/// opposing flows sharing a line ride parallel offset lines instead of
/// meeting head on, along legs and corners alike, with no crossing points
/// anywhere. Runs after the corner standoff, so both directions offset from
/// one shared spine. The shift ramps in from zero at both route ends, drops
/// to zero on ground the corridor layer claims, is clearance probed per
/// point and slope limited so a clamp never leaves a lateral step.
static void fpathKeepRightOffset(const PATHJOB *psJob, MOVE_CONTROL *psMove)
{
	std::vector<Vector2i> &path = psMove->asPath;
	const size_t n = path.size();
	if (n < 4)
	{
		return;
	}
	// Three sub-lanes per direction, chosen by droid id, so the offset is
	// stable across this droid's reroutes but the column spreads.
	// Mixed before the modulus: synchronised ids are generated with a stride,
	// and a raw modulus collapses every droid into one sub-lane whenever the
	// stride shares a factor with the lane count.
	const int32_t laneHalf = KEEP_RIGHT_HALF + (static_cast<int32_t>(((psJob->droidID * 2654435761u) >> 16) % 3u) - 1) * SUBLANE_STEP;
	const CorridorMap *cmap = gameWorld.map.corridors.get();

	static thread_local std::vector<int32_t> arcFwd;
	arcFwd.assign(n, 0);
	for (size_t k = 1; k < n; ++k)
	{
		arcFwd[k] = arcFwd[k - 1] + iHypot(path[k] - path[k - 1]);
	}
	const int32_t total = arcFwd[n - 1];
	if (total < KEEP_RIGHT_RAMP + KEEP_RIGHT_DEST_RAMP)
	{
		return;  // too short to ramp in and back out
	}

	static thread_local std::vector<int32_t> want;
	static thread_local std::vector<Vector2i> rightStep;
	want.assign(n, 0);
	rightStep.assign(n, Vector2i(0, 0));

	for (size_t k = 1; k + 1 < n; ++k)
	{
		Vector2i d = path[k + 1] - path[k - 1];
		int32_t len = iHypot(d);
		if (len == 0)
		{
			continue;
		}
		Vector2i t = map_coord(path[k]);
		if (cmap != nullptr && cmap->claimed(t.x, t.y))
		{
			continue;
		}
		Vector2i right(-d.y * TILE_UNITS / len, d.x * TILE_UNITS / len);
		if (right.x == 0 && right.y == 0)
		{
			continue;
		}
		int32_t w = laneHalf;
		w = std::min(w, laneHalf * arcFwd[k] / KEEP_RIGHT_RAMP);
		w = std::min(w, laneHalf * (total - arcFwd[k]) / KEEP_RIGHT_DEST_RAMP);
		// The lane offset never presses a route within the keep floor of a
		// wall. The spine's clearance, the lane shift and the follower's
		// chord cutting all spend from one budget, and without a floor the
		// wall-side lane spends it to zero and drives the face.
		const int32_t room = wallDistance(psJob, path[k], right);
		if (room <= WALL_CLEAR)
		{
			w = std::min(w, room - WALL_KEEP);
		}
		if (w <= 0)
		{
			continue;
		}
		rightStep[k] = right;
		want[k] = bendAllowedOffset(psJob, path[k], right, w);
	}

	// A gentler slope than the bend limiter: the offset builds over ~4 tiles,
	// so routes drift into their lane and drift back out approaching claimed
	// ground or a clearance pocket, instead of stepping at the boundary.
	for (size_t k = 1; k + 1 < n; ++k)
	{
		int32_t seg = arcFwd[k] - arcFwd[k - 1];
		want[k] = std::min(want[k], want[k - 1] + seg / 4);
	}
	for (size_t k = n - 1; k-- > 1; )
	{
		int32_t seg = arcFwd[k + 1] - arcFwd[k];
		want[k] = std::min(want[k], want[k + 1] + seg / 4);
	}

	for (size_t k = 1; k + 1 < n; ++k)
	{
		if (want[k] > 0)
		{
			path[k].x += rightStep[k].x * want[k] / TILE_UNITS;
			path[k].y += rightStep[k].y * want[k] / TILE_UNITS;
		}
	}
}
// ---- end route bend shaping ----


ASR_RETVAL fpathAStarRoute(const std::shared_ptr<FPathExecuteContext>& ctx, MOVE_CONTROL *psMove, PATHJOB *psJob)
{
	ASR_RETVAL      retval = ASR_OK;

	bool            mustReverse = true;

	auto ctxImpl = std::static_pointer_cast<FPathExecuteContextImpl>(ctx);
	ctxImpl->resetForNewGameTimeIfNeeded(*psJob);

	auto& fpathContexts = ctxImpl->fpathContexts;
	const PathCoord tileOrig(map_coord(psJob->origX), map_coord(psJob->origY));
	const PathCoord tileDest(map_coord(psJob->destX), map_coord(psJob->destY));
	const PathNonblockingArea dstIgnore(psJob->dstStructure);

	PathCoord endCoord;  // Either nearest coord (mustReverse = true) or orig (mustReverse = false).

	auto contextIterator = fpathContexts.begin();
	for (; contextIterator != fpathContexts.end(); ++contextIterator)
	{
		if (!contextIterator->matches(psJob->blockingMap, psJob->overlay, tileDest, dstIgnore))
		{
			// This context is not for the same droid type and same destination.
			continue;
		}

		// We have tried going to tileDest before.

		if (contextIterator->map[tileOrig.x + tileOrig.y * gameWorld.map.width].iteration == contextIterator->iteration
		    && contextIterator->map[tileOrig.x + tileOrig.y * gameWorld.map.width].visited)
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
		contextIterator = fpathContexts.push_back(PathfindContext());

		// Init a new context, overwriting the oldest one if we are caching too many.
		// We will be searching from orig to dest, since we don't know where the nearest reachable tile to dest is.
		fpathInitContext(*contextIterator, psJob->blockingMap, psJob->overlay, tileOrig, tileOrig, tileDest, dstIgnore);
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
	std::vector<Vector2i>& path = ctxImpl->pathBuffer;
	path.clear();

	Vector2i newP(0, 0);
	for (Vector2i p(world_coord(endCoord.x) + TILE_UNITS / 2, world_coord(endCoord.y) + TILE_UNITS / 2); true; p = newP)
	{
		ASSERT_OR_RETURN(ASR_FAILED, worldOnMap(gameWorld.map, p.x, p.y), "Assigned XY coordinates (%d, %d) not on map!", (int)p.x, (int)p.y);
		ASSERT_OR_RETURN(ASR_FAILED, path.size() < (static_cast<size_t>(gameWorld.map.width) * static_cast<size_t>(gameWorld.map.height)), "Pathfinding got in a loop.");

		path.push_back(p);

		PathExploredTile &tile = context.map[map_coord(p.x) + map_coord(p.y) * gameWorld.map.width];
		newP = p - Vector2i(tile.dx, tile.dy) * (TILE_UNITS / 64);
		Vector2i mapP = map_coord(newP);
		int xSide = newP.x - world_coord(mapP.x) > TILE_UNITS / 2 ? 1 : -1; // 1 if newP is on right-hand side of the tile, or -1 if newP is on the left-hand side of the tile.
		int ySide = newP.y - world_coord(mapP.y) > TILE_UNITS / 2 ? 1 : -1; // 1 if newP is on bottom side of the tile, or -1 if newP is on the top side of the tile.
		if (context.isBlocked(mapP.x + xSide, mapP.y))
		{
			newP.x = world_coord(mapP.x) + TILE_UNITS / 2; // Point too close to a blocking tile on left or right side, so move the point to the middle.
		}
		if (context.isBlocked(mapP.x, mapP.y + ySide))
		{
			newP.y = world_coord(mapP.y) + TILE_UNITS / 2; // Point too close to a blocking tile on rop or bottom side, so move the point to the middle.
		}
		if (map_coord(p) == Vector2i(context.tileS.x, context.tileS.y) || p == newP)
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

	// Allocate memory
	psMove->asPath.resize(path.size());

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
		std::copy(path.rbegin(), path.rend(), psMove->asPath.data());

		if (!context.isBlocked(tileOrig.x, tileOrig.y))  // If blocked, searching from tileDest to tileOrig wouldn't find the tileOrig tile.
		{
			// Next time, search starting from nearest reachable tile to the destination.
			fpathInitContext(context, psJob->blockingMap, psJob->overlay, tileDest, context.nearestCoord, tileOrig, dstIgnore);
		}
	}
	else
	{
		// Copy the list.
		std::copy(path.begin(), path.end(), psMove->asPath.data());
	}

	// Move context to beginning of last recently used list.
	if (contextIterator != fpathContexts.begin())  // Not sure whether or not the splice is a safe noop, if equal.
	{
		fpathContexts.moveToFront(contextIterator);
	}

	psMove->destination = psMove->asPath[path.size() - 1];

	if (retval == ASR_OK || (retval == ASR_NEAREST && psJob->acceptNearest))
	{
		if (pathfindingDirectionalBiasEnabled() && psJob->propulsion != PROPULSION_TYPE_LIFT)
		{
			fpathWallClearance(psJob, psMove);
			fpathKeepRightOffset(psJob, psMove);
		}
	}

	return retval;
}

void fpathSetBlockingMap(PATHJOB *psJob)
{
	if (fpathCurrentGameTime != gameTime)
	{
		// New tick, remove maps which are no longer needed.
		fpathCurrentGameTime = gameTime;
		fpathBlockingMaps.clear();
	}

	// Figure out which map we are looking for.
	PathBlockingType type;
	type.gameTime = gameTime;
	type.propulsion = psJob->propulsion;
	type.owner = psJob->owner;
	type.moveType = psJob->moveType;

	// Find the map.
	auto i = std::find_if(fpathBlockingMaps.begin(), fpathBlockingMaps.end(), [&](std::shared_ptr<PathBlockingMap> const &ptr) {
		return *ptr == type;
	});
	if (i == fpathBlockingMaps.end())
	{
		// Didn't find the map, so i does not point to a map.
		auto blockMap = std::make_shared<PathBlockingMap>();
		fpathBlockingMaps.push_back(blockMap);

		// blockMap now points to an empty map with no data. Fill the map.
		blockMap->type = type;
		std::vector<bool> &map = blockMap->map;
		map.resize(static_cast<size_t>(gameWorld.map.width) * static_cast<size_t>(gameWorld.map.height));
		uint32_t checksumMap = 0, checksumDangerMap = 0, factor = 0;
		for (int y = 0; y < gameWorld.map.height; ++y)
			for (int x = 0; x < gameWorld.map.width; ++x)
			{
				map[x + y * gameWorld.map.width] = fpathBaseBlockingTile(gameWorld.map, x, y, type.propulsion, type.owner, type.moveType);
				checksumMap ^= map[x + y * gameWorld.map.width] * (factor = 3 * factor + 1);
			}
		if (!isHumanPlayer(type.owner) && type.moveType == FMT_MOVE)
		{
			std::vector<bool> &dangerMap = blockMap->dangerMap;
			dangerMap.resize(static_cast<size_t>(gameWorld.map.width) * static_cast<size_t>(gameWorld.map.height));
			for (int y = 0; y < gameWorld.map.height; ++y)
				for (int x = 0; x < gameWorld.map.width; ++x)
				{
					dangerMap[x + y * gameWorld.map.width] = auxTile(gameWorld.map, x, y, type.owner) & AUXBITS_THREAT;
					checksumDangerMap ^= dangerMap[x + y * gameWorld.map.width] * (factor = 3 * factor + 1);
				}
		}
		syncDebug("blockingMap(%d,%d,%d,%d) = %08X %08X", gameTime, psJob->propulsion, psJob->owner, psJob->moveType, checksumMap, checksumDangerMap);

		psJob->blockingMap = blockMap;
	}
	else
	{
		syncDebug("blockingMap(%d,%d,%d,%d) = cached", gameTime, psJob->propulsion, psJob->owner, psJob->moveType);

		psJob->blockingMap = *i;
	}
}
