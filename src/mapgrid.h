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
 *  Allows querying which objects are within a given radius of a given location.
 */

#ifndef __INCLUDED_SRC_MAPGRID_H__
#define __INCLUDED_SRC_MAPGRID_H__

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

extern void **gridIterator;  ///< The iterator.


// initialise the grid system
extern BOOL gridInitialise(void);

// shutdown the grid system
extern void gridShutDown(void);

// reset the grid system
extern void gridReset(void);

// TODO THIS FUNCTION DOES NOT DO ANYTHING ANYMORE. Delete this function
extern void gridAddObject(BASE_OBJECT *psObj);

// TODO THIS FUNCTION DOES NOT DO ANYTHING ANYMORE. Delete this function
extern void gridMoveDroid(DROID* psDroid, SDWORD oldX, SDWORD oldY);

// TODO THIS FUNCTION DOES NOT DO ANYTHING ANYMORE. Delete this function
extern void gridRemoveObject(BASE_OBJECT *psObj);

// Calls gridReset().
// TODO Remove this function, call gridReset() directly.
extern void gridGarbageCollect(void);

// Display all the grid's an object is a member of
// TODO This function doesn't do anything, should probably be deleted.
extern void gridDisplayCoverage(BASE_OBJECT *psObj);

#define PREVIOUS_DEFAULT_GRID_SEARCH_RADIUS (20*TILE_UNITS)
/// Find all objects within radius. Call gridIterate() to get the search results.
extern void gridStartIterate(int32_t x, int32_t y, uint32_t radius);

/// Get the next search result from gridStartIterate, or NULL if finished.
static inline BASE_OBJECT *gridIterate(void)
{
	return (BASE_OBJECT *)*gridIterator++;
}

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_SRC_MAPGRID_H__
