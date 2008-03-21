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

#define FPATH_LOOP_LIMIT	600

/** Return values for routing */
typedef enum _fpath_retval
{
	FPR_OK,				// found a route
	FPR_FAILED,			// failed to find a route
	FPR_WAIT,			// route was too long to calculate this frame
						// routing will continue on succeeding frames
	FPR_RESCHEDULE,		// didn't try to route because too much time has been
						// spent routing this frame
} FPATH_RETVAL;

/** Initialise the path-finding module. */
extern BOOL fpathInitialise(void);

/**
 *	Update the findpath system each frame. It checks whether a game object has a
 *	path-finding job that was not finished in the previous frame, and if this
 *	game object is dead, remove it from the job queue.
 */
extern void fpathUpdate(void);

/** Find a route for an object to a location. */
extern FPATH_RETVAL fpathRoute(BASE_OBJECT *psObj, MOVE_CONTROL *psMoveCntl, SDWORD targetX, SDWORD targetY);

extern BOOL (*fpathBlockingTile)(SDWORD x, SDWORD y);

/** Check if the map tile at a location blocks a droid. */
extern BOOL fpathGroundBlockingTile(SDWORD x, SDWORD y);
extern BOOL fpathHoverBlockingTile(SDWORD x, SDWORD y);
extern BOOL fpathLiftSlideBlockingTile(SDWORD x, SDWORD y);

/** Set the correct blocking tile function. */
extern void fpathSetBlockingTile( UBYTE ubPropulsionType );

/** Set pointer for current fpath object - hack. */
extern void fpathSetCurrentObject( BASE_OBJECT *psDroid );

/**
 *	Set direct path to position. Plan a path from psObj's current position to given position 
 *	without taking obstructions into consideration. Used for instance by VTOLs.
 */
extern void fpathSetDirectRoute(BASE_OBJECT *psObj, SDWORD targetX, SDWORD targetY);

#endif // __INCLUDED_SRC_FPATH_H__
