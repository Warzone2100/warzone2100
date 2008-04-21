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

/** The maximum amount of iterations we want to spend in the inner loop of the
 *  A* algorithm.
 *
 *  If this number is succeeded, we will return ASR_PARTIAL and continue the
 *  path finding during the next next frame. Function fpathRoute() will return
 *  FPR_WAIT if it was already working on the currently requested droid.
 *  FPR_RESCHEDULE indicates that fpathRoute() didn't even get started on the
 *  current droid and it'll require rescheduling in the next frame.
 *
 *  @ingroup pathfinding
 */
#define FPATH_LOOP_LIMIT	600

/** The maximum length of an A* computed route, expressed in nodes (aka tiles)
 *
 *  @ingroup pathfinding
 */
#define ASTAR_MAXROUTE  50

/** The buffer to store a route in
 *
 *  @ingroup pathfinding
 */
typedef struct _astar_route
{
	Vector2i asPos[ASTAR_MAXROUTE];
	SDWORD finalX, finalY, numPoints;
} ASTAR_ROUTE;

// counters for a-star
extern int astarInner;

/** Reset the A* counters
 *
 *  This function resets astarInner among others.
 *
 *  @ingroup pathfinding
 */
extern void astarResetCounters(void);

/** return codes for astar
 *
 *  @ingroup pathfinding
 */
enum
{
	ASR_OK,         ///< found a route
	ASR_FAILED,     ///< no route could be found
	ASR_PARTIAL,    ///< routing cannot be finished this frame, and should be continued the next frame
	ASR_NEAREST,    ///< found a partial route to a nearby position
};

/** route modes for astar
 *
 *  @ingroup pathfinding
 */
enum
{
	ASR_NEWROUTE,   ///< start a new route
	ASR_CONTINUE,   ///< continue a route that was partially completed the last frame
};

/** Use the A* algorithm to find a path
 *
 *  @ingroup pathfinding
 */
extern SDWORD fpathAStarRoute(SDWORD routeMode, ASTAR_ROUTE *psRoutePoints,
							SDWORD sx, SDWORD sy, SDWORD fx, SDWORD fy);

// Check los between two tiles
extern BOOL fpathTileLOS(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2);

#endif // __INCLUDED_SRC_ASTART_H__
