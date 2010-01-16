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
/*
 * mapgrid.cpp
 *
 * Functions for storing objects in a quad-tree like object over the map.
 * The objects are stored in the quad-tree.
 *
 */
#include "lib/framework/frame.h"
#include "objects.h"
#include "map.h"

#include "mapgrid.h"
#include "pointtree.h"

// the current state of the iterator
void **gridIterator;

PointTree *gridPointTree = NULL;  // A quad-tree-like object.

// initialise the grid system
BOOL gridInitialise(void)
{
	ASSERT(gridPointTree == NULL, "gridInitialise already called, without calling gridShutDown.");
	gridPointTree = new PointTree;

	return true;  // Yay, nothing failed!
}

// reset the grid system
void gridReset(void)
{
	gridPointTree->clear();

	// Put all existing objects into the point tree.
	for (unsigned player = 0; player < MAX_PLAYERS; player++)
	{
		BASE_OBJECT *start[3] = {(BASE_OBJECT *)apsDroidLists[player], (BASE_OBJECT *)apsStructLists[player], (BASE_OBJECT *)apsFeatureLists[player]};
		for (unsigned type = 0; type != sizeof(start)/sizeof(*start); ++type)
		{
			for (BASE_OBJECT *psObj = start[type]; psObj != NULL; psObj = psObj->psNext)
			{
				if (!psObj->died)
				{
					gridPointTree->insert(psObj, psObj->pos.x, psObj->pos.y);
				}
			}
		}
	}

	gridPointTree->sort();
}

// shutdown the grid system
void gridShutDown(void)
{
	delete gridPointTree;
	gridPointTree = NULL;
}

static bool isInRadius(int32_t x, int32_t y, uint32_t radius)
{
	return x*x + y*y <= radius*radius;
}

// initialise the grid system to start iterating through units that
// could affect a location (x,y in world coords)
void gridStartIterate(int32_t x, int32_t y, uint32_t radius)
{
	gridPointTree->query(x, y, radius);
	PointTree::ResultVector::iterator w = gridPointTree->lastQueryResults.begin(), i;
	for (i = gridPointTree->lastQueryResults.begin(); i != gridPointTree->lastQueryResults.end(); ++i)
	{
		BASE_OBJECT *obj = static_cast<BASE_OBJECT *>(*i);
		if (isInRadius(obj->pos.x - x, obj->pos.y - y, radius))  // Check that search result is less than radius (since they can be up to a factor of sqrt(2) more).
		{
			*w = *i;
			++w;
		}
	}
	gridPointTree->lastQueryResults.erase(w, i);  // Erase all points that were a bit too far.
	gridPointTree->lastQueryResults.push_back(NULL);  // NULL-terminate the result.
	gridIterator = &gridPointTree->lastQueryResults[0];
	/*
	// In case you are curious.
	debug(LOG_WARNING, "gridStartIterate(%d, %d, %u) found %u objects", x, y, radius, (unsigned)gridPointTree->lastQueryResults.size() - 1);
	*/
}

// compact some of the grid arrays
void gridGarbageCollect(void)
{
	gridReset();
}

// Display all the grid's an object is a member of
void gridDisplayCoverage(BASE_OBJECT *psObj)
{
}
