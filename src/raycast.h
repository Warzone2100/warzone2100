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
 *  Raycaster functions
 */

#ifndef __INCLUDED_SRC_RAYCAST_H__
#define __INCLUDED_SRC_RAYCAST_H__

#define NUM_RAYS		360

#define RAY_ANGLE		((float)(2 * M_PI / NUM_RAYS))

#define RAY_LENGTH		(TILE_UNITS * 5)

// maximum length for a visiblity ray
#define RAY_MAXLEN	0x7ffff

/* Initialise the visibility rays */
extern BOOL rayInitialise(void);

/* The raycast intersection callback.
 * Return FALSE if no more points are required, TRUE otherwise
 */
typedef BOOL (*RAY_CALLBACK)(SDWORD x, SDWORD y, SDWORD dist);

/* cast a ray from x,y (world coords) at angle ray (0-NUM_RAYS) */
extern void rayCast(UDWORD x, UDWORD y, UDWORD ray, UDWORD length,
					RAY_CALLBACK callback);

// Calculate the angle to cast a ray between two points
extern UDWORD rayPointsToAngle(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2);

/* Distance of a point from a line.
 * NOTE: This is not 100% accurate - it approximates to get the square root
 *
 * This is based on Graphics Gems II setion 1.3
 */
extern SDWORD rayPointDist(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2,
						   SDWORD px,SDWORD py);

// Calculates the maximum height and distance found along a line from any
// point to the edge of the grid
extern void	getBestPitchToEdgeOfGrid(UDWORD x, UDWORD y, UDWORD direction, SDWORD *pitch);

extern void	getPitchToHighestPoint( UDWORD x, UDWORD y, UDWORD direction, 
								   UDWORD thresholdDistance, SDWORD *pitch );

#endif // __INCLUDED_SRC_RAYCAST_H__
