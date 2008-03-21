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

#ifndef __INCLUDED_SRC_ASTART_H__
#define __INCLUDED_SRC_ASTART_H__

// the buffer to store a route in
#define ASTAR_MAXROUTE		50
typedef struct _astar_route
{
	Vector2i asPos[ASTAR_MAXROUTE];
	SDWORD finalX, finalY, numPoints;
} ASTAR_ROUTE;

// counters for a-star
extern SDWORD astarInner;

// reset the astar counters
extern void astarResetCounters(void);

// Initialise the findpath routine
extern BOOL astarInitialise(void);

// Shutdown the findpath routine
extern void fpathShutDown(void);

// return codes for astar
enum
{
	ASR_OK,			// found a route
	ASR_FAILED,		// no route
	ASR_PARTIAL,	// routing cannot be finished this frame
					// and should be continued the next frame
	ASR_NEAREST,	// found a partial route to a nearby position
};

// route modes for astar
enum
{
	ASR_NEWROUTE,	// start a new route
	ASR_CONTINUE,	// continue a route that was partially completed the last frame
};

// A* findpath
extern SDWORD fpathAStarRoute(SDWORD routeMode, ASTAR_ROUTE *psRoutePoints,
							SDWORD sx, SDWORD sy, SDWORD fx, SDWORD fy);

// Check los between two tiles
extern BOOL fpathTileLOS(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2);

#endif // __INCLUDED_SRC_ASTART_H__
