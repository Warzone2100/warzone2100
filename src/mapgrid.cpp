/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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


static PointTree *gridPointTree = nullptr;  // A quad-tree-like object.
static PointTree::Filter *gridFiltersUnseen;
static PointTree::Filter *gridFiltersDroidsByPlayer;

// initialise the grid system
bool gridInitialise()
{
	ASSERT(gridPointTree == nullptr, "gridInitialise already called, without calling gridShutDown.");
	gridPointTree = new PointTree;
	gridFiltersUnseen = new PointTree::Filter[MAX_PLAYERS];
	gridFiltersDroidsByPlayer = new PointTree::Filter[MAX_PLAYERS];

	return true;  // Yay, nothing failed!
}

// reset the grid system
void gridReset()
{
	gridPointTree->clear();

	// Put all existing objects into the point tree.
	for (unsigned player = 0; player < MAX_PLAYERS; player++)
	{
		BASE_OBJECT *start[3] = {(BASE_OBJECT *)apsDroidLists[player], (BASE_OBJECT *)apsStructLists[player], (BASE_OBJECT *)apsFeatureLists[player]};
		for (unsigned type = 0; type != sizeof(start) / sizeof(*start); ++type)
		{
			for (BASE_OBJECT *psObj = start[type]; psObj != nullptr; psObj = psObj->psNext)
			{
				if (!psObj->died)
				{
					gridPointTree->insert(psObj, psObj->pos.x, psObj->pos.y);
					for (unsigned char &viewer : psObj->seenThisTick)
					{
						viewer = 0;
					}
				}
			}
		}
	}

	gridPointTree->sort();

	for (unsigned player = 0; player < MAX_PLAYERS; ++player)
	{
		gridFiltersUnseen[player].reset(*gridPointTree);
		gridFiltersDroidsByPlayer[player].reset(*gridPointTree);
	}
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
}

static bool isInRadius(int32_t x, int32_t y, uint32_t radius)
{
	return (uint32_t)(x * x + y * y) <= radius * radius;
}

// initialise the grid system to start iterating through units that
// could affect a location (x,y in world coords)
template<class Condition>
static GridList const &gridStartIterateFiltered(int32_t x, int32_t y, uint32_t radius, PointTree::Filter *filter, Condition const &condition)
{
	if (filter == nullptr)
	{
		gridPointTree->query(x, y, radius);
	}
	else
	{
		gridPointTree->query(*filter, x, y, radius);
	}
	PointTree::ResultVector::iterator w = gridPointTree->lastQueryResults.begin(), i;
	for (i = w; i != gridPointTree->lastQueryResults.end(); ++i)
	{
		BASE_OBJECT *obj = static_cast<BASE_OBJECT *>(*i);
		if (!condition.test(obj))  // Check if we should skip this object.
		{
			filter->erase(gridPointTree->lastFilteredQueryIndices[i - gridPointTree->lastQueryResults.begin()]);  // Stop the object from appearing in future searches.
		}
		else if (isInRadius(obj->pos.x - x, obj->pos.y - y, radius))  // Check that search result is less than radius (since they can be up to a factor of sqrt(2) more).
		{
			*w = *i;
			++w;
		}
	}
	gridPointTree->lastQueryResults.erase(w, i);  // Erase all points that were a bit too far.
	/*
	// In case you are curious.
	debug(LOG_WARNING, "gridStartIterateFiltered(%d, %d, %u) found %u objects", x, y, radius, (unsigned)gridPointTree->lastQueryResults.size());
	*/
	static GridList gridList;
	gridList.resize(gridPointTree->lastQueryResults.size());
	for (unsigned n = 0; n < gridList.size(); ++n)
	{
		gridList[n] = (BASE_OBJECT *)gridPointTree->lastQueryResults[n];
	}
	return gridList;
}

template<class Condition>
static GridList const &gridStartIterateFilteredArea(int32_t x, int32_t y, int32_t x2, int32_t y2, Condition const &condition)
{
	gridPointTree->query(x, y, x2, y2);

	static GridList gridList;
	gridList.resize(gridPointTree->lastQueryResults.size());
	for (unsigned n = 0; n < gridList.size(); ++n)
	{
		gridList[n] = (BASE_OBJECT *)gridPointTree->lastQueryResults[n];
	}
	return gridList;
}

struct ConditionTrue
{
	bool test(BASE_OBJECT *) const
	{
		return true;
	}
};

GridList const &gridStartIterate(int32_t x, int32_t y, uint32_t radius)
{
	return gridStartIterateFiltered(x, y, radius, nullptr, ConditionTrue());
}

GridList const &gridStartIterateArea(int32_t x, int32_t y, uint32_t x2, uint32_t y2)
{
	return gridStartIterateFilteredArea(x, y, x2, y2, ConditionTrue());
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

GridList const &gridStartIterateDroidsByPlayer(int32_t x, int32_t y, uint32_t radius, int player)
{
	return gridStartIterateFiltered(x, y, radius, &gridFiltersDroidsByPlayer[player], ConditionDroidsByPlayer(player));
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

GridList const &gridStartIterateUnseen(int32_t x, int32_t y, uint32_t radius, int player)
{
	return gridStartIterateFiltered(x, y, radius, &gridFiltersUnseen[player], ConditionUnseen(player));
}

BASE_OBJECT **gridIterateDup()
{
	size_t bytes = gridPointTree->lastQueryResults.size() * sizeof(void *);
	BASE_OBJECT **ret = (BASE_OBJECT **)malloc(bytes);
	memcpy(ret, &gridPointTree->lastQueryResults[0], bytes);
	return ret;
}
