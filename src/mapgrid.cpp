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
				gridPointTree->insert(psObj, psObj->pos.x, psObj->pos.y);
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

// add an object to the grid system
void gridAddObject(BASE_OBJECT *psObj)
{
	ASSERT_OR_RETURN(, psObj != NULL, "Attempted to add a NULL pointer");
	ASSERT_OR_RETURN(, !isDead(psObj), "Attempted to add dead object %s(%d) to the map grid!", objInfo(psObj), (int)psObj->id);
	//gridCalcCoverage(psObj, (SDWORD)psObj->pos.x, (SDWORD)psObj->pos.y, GRID_ADDOBJECT);
}

// move an object within the grid
// oldX,oldY are the old position of the object in world coords
void gridMoveDroid(DROID* psDroid, SDWORD oldX, SDWORD oldY)
{
	if (map_coord(psDroid->pos.x) == map_coord(oldX)
	 && map_coord(psDroid->pos.y) == map_coord(oldY))
	{
		// havn't changed the tile the object is on, don't bother updating
		return;
	}

	//gridCalcCoverage((BASE_OBJECT*)psDroid, oldX,oldY, GRID_REMOVEOBJECT);
	//gridCalcCoverage((BASE_OBJECT*)psDroid, psDroid->pos.x, psDroid->pos.y, GRID_ADDOBJECT);
}

// remove an object from the grid system
void gridRemoveObject(BASE_OBJECT *psObj)
{
	//gridCalcCoverage(psObj, (SDWORD)psObj->pos.x, (SDWORD)psObj->pos.y, GRID_REMOVEOBJECT);
}

// initialise the grid system to start iterating through units that
// could affect a location (x,y in world coords)
void gridStartIterate(SDWORD x, SDWORD y/*, uint32_t radius*/)
{
	uint32_t radius = 20*TILE_UNITS;

	gridIterator = pointTreeQuery(gridPointTree, x, y, radius);  // Use the C interface, for NULL termination.
	/*
	// In case you are curious.
	int len = 0;
	for(void **x = gridIterator; *x != NULL; ++x)
		++len;
	debug(LOG_WARNING, "gridStartIterate found %d objects", len);
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
