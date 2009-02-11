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

#ifndef __INCLUDED_SRC_ASTART_H__
#define __INCLUDED_SRC_ASTART_H__

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
	ASR_NEAREST,    ///< found a partial route to a nearby position
};

/** Use the A* algorithm to find a path
 *
 *  @ingroup pathfinding
 */
SDWORD fpathAStarRoute(MOVE_CONTROL *psMove, SDWORD sx, SDWORD sy, SDWORD fx, SDWORD fy, PROPULSION_TYPE propulsion);

/** Check LOS (Line Of Sight) between two tiles
 */
extern BOOL fpathTileLOS(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2);

/** Clean up the path finding node table.
 *
 *  @note Call this <em>only</em> on shutdown to prevent memory from leaking.
 *
 *  @ingroup pathfinding
 */
extern void fpathHardTableReset(void);

#endif // __INCLUDED_SRC_ASTART_H__
