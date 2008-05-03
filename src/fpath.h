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
/** @file
 *  Interface to the routing functions
 */

#ifndef __INCLUDED_SRC_FPATH_H__
#define __INCLUDED_SRC_FPATH_H__

#include "droiddef.h"

/** Return values for routing
 *
 *  @ingroup pathfinding
 */
typedef enum _fpath_retval
{
	FPR_OK,         ///< found a route
	FPR_FAILED,     ///< failed to find a route
	FPR_WAIT,       ///< route was too long to calculate this frame, routing will continue on succeeding frames
	FPR_RESCHEDULE, ///< didn't try to route because too much time has been spent on routing this frame
} FPATH_RETVAL;

/** Initialise the path-finding module.
 *
 *  @ingroup pathfinding
 */
extern BOOL fpathInitialise(void);

extern void fpathUpdate(void);

/** Find a route for a droid to a location.
 *  
 *  @ingroup pathfinding
 */
extern FPATH_RETVAL fpathRoute(DROID* psDroid, SDWORD targetX, SDWORD targetY);

/** Function pointer to the currently in-use blocking tile check function.
 *  
 *  This function will check if the map tile at the given location blocks droids
 *  with the currently selected propulsion type.
 *
 *  @param x,y the parameters of the map tile to check
 *  @return true if the given tile is blocking for this droid
 *
 *  @ingroup pathfinding
 */
extern BOOL (*fpathBlockingTile)(SDWORD x, SDWORD y);

/** Check if the map tile at the given location blocks a droid with "normal"
 *  propulsion.
 *
 *  @return true if the given tile is blocking for a "normal" droid
 *
 *  @ingroup pathfinding
 */
extern BOOL fpathGroundBlockingTile(SDWORD x, SDWORD y);

/** Check if the map tile at the given location blocks a droid with hovercraft
 *  propulsion.
 *
 *  @return true if the given tile is blocking for a hovercraft droid
 *
 *  @ingroup pathfinding
 */
extern BOOL fpathHoverBlockingTile(SDWORD x, SDWORD y);

/** Check if the map tile at the given location blocks a droid with VTOL
 *  propulsion.
 *
 *  @return true if the given tile is blocking for a VTOL droid
 *
 *  @ingroup pathfinding
 */
extern BOOL fpathLiftSlideBlockingTile(SDWORD x, SDWORD y);

/** Set the correct blocking tile function.
 *  @ingroup pathfinding
 */
extern void fpathSetBlockingTile( UBYTE ubPropulsionType );

/** Set pointer for current pathfinding droid
 *
 *  HACK!
 *
 *  @ingroup pathfinding
 */
extern void fpathSetCurrentDroid(DROID* psDroid);

/** Set a direct path to position.
 *
 *  Plan a path from @c psDroid's current position to given position without
 *  taking obstructions into consideration.
 *
 *  Used for instance by VTOLs.
 *
 *  @ingroup pathfinding
 */
extern void fpathSetDirectRoute(DROID* psDroid, SDWORD targetX, SDWORD targetY);

#endif // __INCLUDED_SRC_FPATH_H__
