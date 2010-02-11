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
 *  Interface for the unit movement system
 */

#ifndef __INCLUDED_SRC_MOVE_H__
#define __INCLUDED_SRC_MOVE_H__

#include "objectdef.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

/* The base movement speed */
extern float	baseSpeed;

// The next object that should get the router when a lot of units are
// in a MOVEROUTE state
extern DROID	*psNextRouteDroid;

/* Initialise the movement system */
extern BOOL moveInitialise(void);

/* Update the base speed for all movement */
extern void moveUpdateBaseSpeed(void);

/* Set a target location for a droid to move to  - returns a BOOL based on if there is a path to the destination (true if there is a path)*/
extern BOOL moveDroidTo(DROID *psDroid, UDWORD x, UDWORD y);

/* Set a target location for a droid to move to  - returns a BOOL based on if there is a path to the destination (true if there is a path)*/
// the droid will not join a formation when it gets to the location
extern BOOL moveDroidToNoFormation(DROID *psDroid, UDWORD x, UDWORD y);

// move a droid directly to a location (used by vtols only)
extern void moveDroidToDirect(DROID *psDroid, UDWORD x, UDWORD y);

// Get a droid to turn towards a locaton
extern void moveTurnDroid(DROID *psDroid, UDWORD x, UDWORD y);

/* Stop a droid */
extern void moveStopDroid(DROID *psDroid);

/*Stops a droid dead in its tracks - doesn't allow for any little skidding bits*/
extern void moveReallyStopDroid(DROID *psDroid);

/* Get a droid to do a frame's worth of moving */
extern void moveUpdateDroid(DROID *psDroid);

SDWORD moveCalcDroidSpeed(DROID *psDroid);

/* Frame update for the movement of a tracked droid */
extern void moveUpdateTracked(DROID *psDroid);

/* update body and turret to local slope */
extern void updateDroidOrientation(DROID *psDroid);

/* audio callback used to kill movement sounds */
extern BOOL moveCheckDroidMovingAndVisible( void *psObj );

// set a vtol to be hovering in the air
void moveMakeVtolHover( DROID *psDroid );

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_SRC_MOVE_H__
