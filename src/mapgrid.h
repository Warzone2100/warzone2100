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
/*
 * MapGrid.h
 *
 * Definitions for storing objects in a grid over the map.
 * The objects are stored in every grid over which they might
 * have some influence.
 *
 */
#ifndef _mapgrid_h
#define _mapgrid_h

/** Number of objects in each chunk of the grid array. */
#define MAX_GRID_ARRAY_CHUNK 32

// Objects are stored in an extensible array for each grid
typedef struct _grid_array
{
	BASE_OBJECT *apsObjects[MAX_GRID_ARRAY_CHUNK];

	struct _grid_array *psNext;
} GRID_ARRAY;


/** The number of tiles per grid. */
#define GRID_SIZE	8

#define GRID_MAXAREA (MAP_MAXAREA/(GRID_SIZE*GRID_SIZE))

// initialise the grid system
extern BOOL gridInitialise(void);

// shutdown the grid system
extern void gridShutDown(void);

//clear the grid of everything on it
extern void gridClear(void);

// reset the grid system
extern void gridReset(void);

// add an object to the grid system
extern void gridAddObject(BASE_OBJECT *psObj);

// move an object within the grid
// oldX,oldY are the old position of the object in world coords
extern void gridMoveObject(BASE_OBJECT *psObj, SDWORD oldX, SDWORD oldY);

// remove an object from the grid system
extern void gridRemoveObject(BASE_OBJECT *psObj);

// compact some of the grid arrays
extern void gridGarbageCollect(void);

// Display all the grid's an object is a member of
extern void gridDisplayCoverage(BASE_OBJECT *psObj);

// initialise the grid system to start iterating through units that
// could affect a location (x,y in world coords)
extern void gridStartIterate(SDWORD x, SDWORD y);

// get the next object that could affect a location,
// should only be called after gridStartIterate
extern BASE_OBJECT *gridIterate(void);

#endif
