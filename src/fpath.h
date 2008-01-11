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
 * Fpath.h
 *
 * Interface to the routing functions
 *
 */
#ifndef _fpath_h
#define _fpath_h

#define FPATH_LOOP_LIMIT	600

// return values for routing
typedef enum _fpath_retval
{
	FPR_OK,				// found a route
	FPR_FAILED,			// failed to find a route
	FPR_WAIT,			// route was too long to calculate this frame
						// routing will continue on succeeding frames
	FPR_RESCHEDULE,		// didn't try to route because too much time has been
						// spent routing this frame
} FPATH_RETVAL;

// initialise the findpath module
extern BOOL fpathInitialise(void);

// update the findpath system each frame
extern void fpathUpdate(void);

// access functions for the loop limit
extern void fpathSetMaxRoute(SDWORD max);

// Find a route for an object to a location
extern FPATH_RETVAL fpathRoute(BASE_OBJECT *psObj, MOVE_CONTROL *psMoveCntl,
							   SDWORD targetX, SDWORD targetY);

extern BOOL (*fpathBlockingTile)(SDWORD x, SDWORD y);

// Check if the map tile at a location blocks a droid
extern BOOL fpathGroundBlockingTile(SDWORD x, SDWORD y);
extern BOOL fpathHoverBlockingTile(SDWORD x, SDWORD y);
extern BOOL fpathLiftBlockingTile(SDWORD x, SDWORD y);
extern BOOL fpathLiftSlideBlockingTile(SDWORD x, SDWORD y);

/* set the correct blocking tile function */
extern void fpathSetBlockingTile( UBYTE ubPropulsionType );

/* set pointer for current fpath object - GJ hack */
extern void fpathSetCurrentObject( BASE_OBJECT *psDroid );

/* set direct path to position */
extern void fpathSetDirectRoute( BASE_OBJECT *psObj,
							SDWORD targetX, SDWORD targetY );

#endif
