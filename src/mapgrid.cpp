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
/*
 * mapgrid.cpp
 *
 * Functions for storing objects in a quad-tree like object over the map.
 * The objects are stored in the quad-tree.
 *
 */
#include "lib/framework/types.h"
#include "objects.h"
#include "map.h"

#include "mapgrid.h"
#include "pointtree.h"
#include "game_world.h"
#include "perfcounters.h"

#include <memory>


static PointTree *gridPointTree = nullptr;  // A quad-tree-like object.
static unsigned gridPersonCount = 0;
static PointTree::Filter *gridFiltersUnseen;
static PointTree::Filter *gridFiltersDroidsByPlayer;
static PointTree::Filter *gridFiltersDroidsRepairCandidates;

/// One result buffer per live GridQuery, indexed by nesting depth.
/// The pool grows on-demand. Each buffer is its own allocation, so growing the pool never moves a buffer that a live GridQuery points at.
static std::vector<std::unique_ptr<GridList>> gridQueryBuffers;
static unsigned gridQueryDepth = 0;

/// Buffers allocated at init by gridInitialise(). Normal play nests only a few deep, so the pool is not expected to grow past this.
static const unsigned GRID_QUERY_BUFFERS_PREALLOCATED = 8;

/// Nesting past the preallocated buffers is logged, and nesting to this depth ASSERTs (to help track down perf or logic issues - normal depth should be substantially less).
static const unsigned GRID_QUERY_DEPTH_SUSPICIOUS = 16;

// initialise the grid system
bool gridInitialise()
{
	ASSERT(gridPointTree == nullptr, "gridInitialise already called, without calling gridShutDown.");
	gridPointTree = new PointTree;
	gridFiltersUnseen = new PointTree::Filter[MAX_PLAYERS];
	gridFiltersDroidsByPlayer = new PointTree::Filter[MAX_PLAYERS];
	gridFiltersDroidsRepairCandidates = new PointTree::Filter[MAX_PLAYERS];

	ASSERT(gridQueryDepth == 0, "gridInitialise called with %u grid queries still live", gridQueryDepth);
	gridQueryBuffers.clear();
	gridQueryBuffers.reserve(GRID_QUERY_BUFFERS_PREALLOCATED);
	for (unsigned i = 0; i < GRID_QUERY_BUFFERS_PREALLOCATED; ++i)
	{
		gridQueryBuffers.push_back(std::make_unique<GridList>());
	}

	return true;  // Yay, nothing failed!
}

// reset the grid system
void gridReset(GameWorld& world)
{
	gridPointTree->clear();
	gridPersonCount = 0;

	// Put all existing objects into the point tree.
	for (unsigned player = 0; player < MAX_PLAYERS; player++)
	{
		for (DROID* psObj : world.objects.droids[player])
		{
			if (!psObj->died)
			{
				gridPointTree->insert(psObj, psObj->pos.x, psObj->pos.y);
				gridPersonCount += psObj->droidType == DROID_PERSON;
				for (unsigned char& viewer : psObj->seenThisTick)
				{
					viewer = 0;
				}
			}
		}
		for (BASE_OBJECT* psObj : world.objects.structures[player])
		{
			if (!psObj->died)
			{
				gridPointTree->insert(psObj, psObj->pos.x, psObj->pos.y);
				for (unsigned char& viewer : psObj->seenThisTick)
				{
					viewer = 0;
				}
			}
		}
	}
	for (BASE_OBJECT* psObj : world.objects.features[0])
	{
		if (!psObj->died)
		{
			gridPointTree->insert(psObj, psObj->pos.x, psObj->pos.y);
			for (unsigned char& viewer : psObj->seenThisTick)
			{
				viewer = 0;
			}
		}
	}

	gridPointTree->sort();

	for (unsigned player = 0; player < MAX_PLAYERS; ++player)
	{
		gridFiltersUnseen[player].reset(*gridPointTree);
		gridFiltersDroidsByPlayer[player].reset(*gridPointTree);
		gridFiltersDroidsRepairCandidates[player].reset(*gridPointTree);
	}
}

unsigned gridLivePersonCount()
{
	return gridPersonCount;
}

// shutdown the grid system
void gridShutDown()
{
	delete gridPointTree;
	gridPointTree = nullptr;
	delete[] gridFiltersUnseen;
	gridFiltersUnseen = nullptr;
	delete[] gridFiltersDroidsByPlayer;
	gridFiltersDroidsByPlayer = nullptr;
	delete[] gridFiltersDroidsRepairCandidates;
	gridFiltersDroidsRepairCandidates = nullptr;

	ASSERT(gridQueryDepth == 0, "gridShutDown called with %u grid queries still live", gridQueryDepth);
	gridQueryBuffers.clear();
}

static bool isInRadius(int32_t x, int32_t y, uint32_t radius)
{
	// cast to int64 to avoid integer overflow
	return ((int64_t)x * (int64_t)x + (int64_t)y * (int64_t)y) <= ((int64_t)radius * (int64_t)radius);
}

/// Claims the next result buffer, adding one to the pool if every existing buffer is in use.
/// It stays claimed until the GridQuery built from the returned slot is destroyed.
/// The buffer keeps the capacity its last use grew it to.
static GridList &gridClaimBuffer(unsigned &slot)
{
	slot = gridQueryDepth;
	if (slot == gridQueryBuffers.size())
	{
		if (slot < GRID_QUERY_DEPTH_SUSPICIOUS)
		{
			debug(LOG_INFO, "Grid queries nested %u deep, growing the buffer pool beyond its preallocated %u", slot + 1, GRID_QUERY_BUFFERS_PREALLOCATED);
		}
		ASSERT(slot != GRID_QUERY_DEPTH_SUSPICIOUS, "Grid queries nested %u deep - check the code!", slot + 1);
		gridQueryBuffers.push_back(std::make_unique<GridList>());
	}
	return *gridQueryBuffers[slot];
}

GridQuery::~GridQuery()
{
	ASSERT(gridQueryDepth == slot + 1, "GridQuery released out of order, expected slot %u but this is %u", gridQueryDepth - 1, slot);
	gridQueryDepth = slot;
}

// initialise the grid system to start iterating through units that
// could affect a location (x,y in world coords)
template<class Condition>
static const GridList *gridStartIterateFiltered(unsigned &slot, int32_t x, int32_t y, uint32_t radius, PointTree::Filter *filter, Condition const &condition)
{
	WZ_PERF_SCOPE(T_gridQuery);
	WZ_PERF_COUNT(C_gridQueries, 1);
	PointTree::ResultVector &found = (filter == nullptr)
	                                 ? gridPointTree->query(x, y, radius)
	                                 : gridPointTree->query(*filter, x, y, radius);

	// The results are compacted in place first and copied out second - tested faster than filtering
	// straight into the result buffer, due to compiler handling of possible aliasing.
	PointTree::ResultVector::iterator w = found.begin(), i;
	for (i = w; i != found.end(); ++i)
	{
		BASE_OBJECT *obj = static_cast<BASE_OBJECT *>(*i);
		if (!condition.test(obj))  // Check if we should skip this object.
		{
			filter->erase(gridPointTree->lastFilteredQueryIndices[i - found.begin()]);  // Stop the object from appearing in future searches.
		}
		else if (isInRadius(obj->pos.x - x, obj->pos.y - y, radius))  // Check that search result is less than radius (since they can be up to a factor of sqrt(2) more).
		{
			*w = *i;
			++w;
		}
	}
	found.erase(w, i);  // Erase all points that were a bit too far.

	GridList &gridList = gridClaimBuffer(slot);
	gridList.resize(found.size());
	for (unsigned n = 0; n < gridList.size(); ++n)
	{
		gridList[n] = (BASE_OBJECT *)found[n];
	}
	WZ_PERF_COUNT(C_gridResultsReturned, gridList.size());
	++gridQueryDepth;
	return &gridList;
}

template<class Condition>
static const GridList *gridStartIterateFilteredArea(unsigned &slot, int32_t x, int32_t y, int32_t x2, int32_t y2, Condition const &condition)
{
	WZ_PERF_SCOPE(T_gridQuery);
	WZ_PERF_COUNT(C_gridQueries, 1);
	PointTree::ResultVector &found = gridPointTree->query(x, y, x2, y2);

	GridList &gridList = gridClaimBuffer(slot);
	gridList.resize(found.size());
	for (unsigned n = 0; n < gridList.size(); ++n)
	{
		gridList[n] = (BASE_OBJECT *)found[n];
	}
	WZ_PERF_COUNT(C_gridResultsReturned, gridList.size());
	++gridQueryDepth;
	return &gridList;
}

struct ConditionTrue
{
	bool test(BASE_OBJECT *) const
	{
		return true;
	}
};

GridQuery gridStartIterate(int32_t x, int32_t y, uint32_t radius)
{
	unsigned slot;
	const GridList *results = gridStartIterateFiltered(slot, x, y, radius, nullptr, ConditionTrue());
	return GridQuery(slot, results);
}

GridQuery gridStartIterateArea(int32_t x, int32_t y, uint32_t x2, uint32_t y2)
{
	unsigned slot;
	const GridList *results = gridStartIterateFilteredArea(slot, x, y, x2, y2, ConditionTrue());
	return GridQuery(slot, results);
}

struct ConditionDroidsByPlayer
{
	ConditionDroidsByPlayer(int32_t player_) : player(player_) {}
	bool test(BASE_OBJECT *obj) const
	{
		return obj->type == OBJ_DROID && obj->player == player;
	}
	int player;
};

GridQuery gridStartIterateDroidsByPlayer(int32_t x, int32_t y, uint32_t radius, int player)
{
	unsigned slot;
	const GridList *results = gridStartIterateFiltered(slot, x, y, radius, &gridFiltersDroidsByPlayer[player], ConditionDroidsByPlayer(player));
	return GridQuery(slot, results);
}

struct ConditionDroidCandidateForRepair
{
	ConditionDroidCandidateForRepair(int32_t player_) : player(player_) {}
	bool test(BASE_OBJECT *obj) const
	{
		if (obj->type != OBJ_DROID) return false;
		const DROID *psDroid = (const DROID*) obj;
		const bool isOwnOrAlly = psDroid->player == player || aiCheckAlliances(psDroid->player, player);
		const bool isVTOL = psDroid->getPropulsionStats()->propulsionType == PROPULSION_TYPE_LIFT;
		// either it's a ground unit, or it's a VTOL on ground
		const bool isOnGround = (!isVTOL) || (isVTOL && (psDroid->sMove.Status == MOVEINACTIVE && psDroid->sMove.iVertSpeed == 0));
		// Note: no check for droidIsDamaged(psDroid) this is intentional
		return !psDroid->died && isOwnOrAlly && isOnGround;
	}
	int player;
};

GridQuery gridStartIterateRepairCandidates(int32_t x, int32_t y, uint32_t radius, int player)
{
	unsigned slot;
	const GridList *results = gridStartIterateFiltered(slot, x, y, radius, &gridFiltersDroidsRepairCandidates[player], ConditionDroidCandidateForRepair(player));
	return GridQuery(slot, results);
}

struct ConditionUnseen
{
	ConditionUnseen(int32_t player_) : player(player_) {}
	bool test(BASE_OBJECT *obj) const
	{
		return obj->seenThisTick[player] < UINT8_MAX;
	}
	int player;
};

GridQuery gridStartIterateUnseen(int32_t x, int32_t y, uint32_t radius, int player)
{
	unsigned slot;
	const GridList *results = gridStartIterateFiltered(slot, x, y, radius, &gridFiltersUnseen[player], ConditionUnseen(player));
	return GridQuery(slot, results);
}
