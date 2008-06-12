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
/**
 * @file raycast.c
 *
 * Raycasting routine that gives intersection points with map tiles.
 *
 */

#include "lib/framework/frame.h"
#include "lib/framework/math-help.h"

#include "raycast.h"
#include "objects.h"
#include "map.h"
#include "display3d.h"
#include "effects.h"


// accuracy for the raycast lookup tables
#define RAY_ACC 12

#define RAY_ACCMUL (1<<RAY_ACC)


/* x and y increments for each ray angle */
static SDWORD	rayDX[NUM_RAYS], rayDY[NUM_RAYS];
static SDWORD	rayHDist[NUM_RAYS], rayVDist[NUM_RAYS];
static SDWORD	rayFPTan[NUM_RAYS], rayFPInvTan[NUM_RAYS];
static SDWORD	rayFPInvCos[NUM_RAYS], rayFPInvSin[NUM_RAYS];

typedef struct {
	const int height;
	float pitch;
} HeightCallbackHelp_t;

typedef struct {
	const int minDist, origHeight;
	int highestHeight;
	float pitch;
} HighestCallbackHelp_t;


/* Initialise the ray tables */
BOOL rayInitialise(void)
{
	SDWORD i;
	float angle = 0.0f;
	float val;

	for(i = 0; i < NUM_RAYS; i++)
	{
		// Set up the fixed offset tables for calculating the intersection points
		val = tanf(angle);

		rayDX[i] = (SDWORD)(TILE_UNITS * RAY_ACCMUL * val);

		if (i <= NUM_RAYS/4 ||
			(i >= 3*NUM_RAYS/4))
		{
			rayDX[i] = -rayDX[i];
		}

		if(val == 0) {
			val = (float)1;	// Horrible hack to avoid divide by zero.
		}

		rayDY[i] = (SDWORD)(TILE_UNITS * RAY_ACCMUL / val);
		if (i >= NUM_RAYS/2)
		{
			rayDY[i] = -rayDY[i];
		}

		// These are used to calculate the initial intersection
		rayFPTan[i] = val * (float)RAY_ACCMUL;
		rayFPInvTan[i] = (float)RAY_ACCMUL / val;

		// Set up the trig tables for calculating the offset distances
		val = sinf(angle);
		if(val == 0.0f) {
			val = 1.0f;
		}
		rayFPInvSin[i] = (float)RAY_ACCMUL / val;
		if (i >= NUM_RAYS/2)
		{
			rayVDist[i] = (float) - TILE_UNITS / val;
		}
		else
		{
			rayVDist[i] = (float)TILE_UNITS / val;
		}

		val = cosf(angle);
		if(val == 0.0f) {
			val = 1.0f;
		}
		rayFPInvCos[i] = (float)RAY_ACCMUL / val;
		if (i < NUM_RAYS/4 || i > 3*NUM_RAYS/4)
		{
			rayHDist[i] = (float)TILE_UNITS / val;
		}
		else
		{
			rayHDist[i] = (float)-TILE_UNITS / val;
		}

		angle += RAY_ANGLE;
	}

	return true;
}


/* cast a ray from x,y (world coords) at angle ray (0-360)
 * The ray angle starts at zero along the positive y axis and
 * increases towards -ve X.
 *
 * Sorry about the wacky angle set up but that was what I thought
 * warzone used, but turned out not to be after I wrote it.
 */
void rayCast(UDWORD x, UDWORD y, UDWORD ray, UDWORD length, RAY_CALLBACK callback, void * data)
{
	int hdInc = 0, vdInc = 0; // increases in x and y distance per intersection
	int hDist, vDist; // distance to current horizontal and vertical intersectionse
	Vector2i sVert = { 0, 0 }, sHoriz = { 0, 0 };
	int vdx = 0, hdy = 0; // vertical x increment, horiz y inc

	// Clipping is done with the position offset by TILE_UNITS/4 to account
	// for the rounding errors when the intersection length is calculated.
	// Bit of a hack but I'm pretty sure it doesn't let through anything
	// that should be clippped.

	// initialise the horizontal intersection calculations
	// and clip to the top and bottom of the map
	// (no horizontal intersection for a horizontal ray)
	if (ray != NUM_RAYS/4 && ray != 3*NUM_RAYS/4)
	{
		if (ray < NUM_RAYS/4 || ray > 3*NUM_RAYS/4)
		{
			// intersection
			sHoriz.y = (y & ~TILE_MASK) + TILE_UNITS;
			hdy = TILE_UNITS;
		}
		else
		{
			// intersection
			sHoriz.y = (y & ~TILE_MASK) - 1;
			hdy = -TILE_UNITS;
		}

		// Horizontal x is kept in fixed point form until passed to the callback
		// to avoid rounding errors
		// Horizontal y is in integer form all the time
		sHoriz.x = (x << RAY_ACC) + (((SDWORD)y-sHoriz.y) * rayFPTan[ray]);

		// Set up the distance calculations
		hDist = ((sHoriz.y - (SDWORD)y) * rayFPInvCos[ray]) >> RAY_ACC;
		hdInc = rayHDist[ray];
	}
	else
	{
		// ensure no horizontal intersections are calculated
		hDist = length;
	}

	// initialise the vertical intersection calculations
	// and clip to the left and right of the map
	// (no vertical intersection for a vertical ray)
	if (ray != 0 && ray != NUM_RAYS/2) // was: 0*NUM_RAYS/4 && 2*NUM_RAYS/2
	{
		if (ray >= NUM_RAYS/2)
		{
			// intersection
			sVert.x = (x & ~TILE_MASK) + TILE_UNITS;
			vdx = TILE_UNITS;
		}
		else
		{
			// intersection
			sVert.x = (x & ~TILE_MASK) - 1;
			vdx = -TILE_UNITS;
		}

		// Vertical y is kept in fixed point form until passed to the callback
		// to avoid rounding errors
		// Vertical x is in integer form all the time
		sVert.y = (y << RAY_ACC) + ((SDWORD)x-sVert.x) * rayFPInvTan[ray];

		// Set up the distance calculations
		vDist = (((SDWORD)x - sVert.x) * rayFPInvSin[ray]) >> RAY_ACC;
		vdInc = rayVDist[ray];
	}
	else
	{
		// ensure no vertical intersections are calculated
		vDist = length;
	}

	ASSERT( hDist != 0 && vDist != 0,
		"rayCast: zero distance" );
	ASSERT( (hDist == (SDWORD)length || hdInc > 0) &&
			(vDist == (SDWORD)length || vdInc > 0),
		"rayCast: negative (or 0) distance increment" );

	while(hDist < (SDWORD)length ||
		  vDist < (SDWORD)length)
	{
		// choose the next closest intersection
		if (hDist < vDist)
		{
			// clip to the edge of the map
			if (sHoriz.x < 0 || (sHoriz.x >> RAY_ACC) >= world_coord(mapWidth) ||
				sHoriz.y < 0 || sHoriz.y >= world_coord(mapHeight))
			{
				return;
			}

			// pass through the current intersection, converting x from fixed point
			if (!callback( sHoriz.x >> RAY_ACC,sHoriz.y, hDist, data))
			{
				// callback doesn't want any more points so return
				return;
			}

			// update for the next intersection
			sHoriz.x += rayDX[ray];
			sHoriz.y += hdy;
			hDist += hdInc;
		}
		else
		{
			// clip to the edge of the map
			if (sVert.x < 0 || sVert.x >= world_coord(mapWidth) ||
				sVert.y < 0 || (sVert.y >> RAY_ACC) >= world_coord(mapHeight))
			{
				return;
			}

			// pass through the current intersection, converting y from fixed point
			if (!callback( sVert.x,sVert.y >> RAY_ACC, vDist, data))
			{
				// callback doesn't want any more points so return
				return;
			}

			// update for the next intersection
			sVert.x += vdx;
			sVert.y += rayDY[ray];
			vDist += vdInc;
		}
		ASSERT( hDist != 0 && vDist != 0,
			"rayCast: zero distance" );
	}
}


// Calculate the angle to cast a ray between two points
unsigned int rayPointsToAngle(int x1, int y1, int x2, int y2)
{
	int xdiff = x2 - x1, ydiff = y1 - y2;
	int angle = (float)(NUM_RAYS / 2) * (1.0f + atan2f(xdiff, ydiff) / (float)M_PI);

	angle = angle % NUM_RAYS;

	ASSERT( angle >= 0 && angle < NUM_RAYS,
		"rayPointsToAngle: angle out of range" );

	return angle;
}


/* Distance of a point from a line.
 * NOTE: This is not 100% accurate - it approximates to get the square root
 *
 * This is based on Graphics Gems II setion 1.3
 */
SDWORD rayPointDist(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2,
					   SDWORD px,SDWORD py)
{
	SDWORD	a, lxd,lyd, dist;

	lxd = x2 - x1;
	lyd = y2 - y1;

	a = (py - y1)*lxd - (px - x1)*lyd;
	if (a < 0)
	{
		a = -a;
	}
	if (lxd < 0)
	{
		lxd = -lxd;
	}
	if (lyd < 0)
	{
		lyd = -lyd;
	}

	if (lxd < lyd)
	{
		dist = a / (lxd + lyd - lxd/2);
	}
	else
	{
		dist = a / (lxd + lyd - lyd/2);
	}

	return dist;
}


//-----------------------------------------------------------------------------------
/*	Gets the maximum terrain height along a certain direction to the edge of the grid
	from wherever you specify, as well as the distance away
*/
//-----------------------------------------------------------------------------------
static BOOL	getTileHighestCallback(SDWORD x, SDWORD y, SDWORD dist, void* data)
{
	HighestCallbackHelp_t * help = data;

	if (clipXY(x, y))
	{
		unsigned int height = map_Height(x,y);
		if (height > help->highestHeight && dist >= help->minDist)
		{
			int heightDif = height - help->origHeight;
			help->pitch = rad2degf(atan2f(heightDif, world_coord(6)));// (float)(dist - world_coord(3))));
			help->highestHeight = height;
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------------
/* Will return false when we've hit the edge of the grid */
static BOOL	getTileHeightCallback(SDWORD x, SDWORD y, SDWORD dist, void* data)
{
	HeightCallbackHelp_t * help = data;
#ifdef TEST_RAY
	Vector3i pos;
#endif

	/* Are we still on the grid? */
	if (clipXY(x, y))
	{
		BOOL HasTallStructure = TileHasTallStructure(mapTile(map_coord(x), map_coord(y)));

		if (dist > TILE_UNITS || HasTallStructure)
		{
		// Only do it the current tile is > TILE_UNITS away from the starting tile. Or..
		// there is a tall structure  on the current tile and the current tile is not the starting tile.
			/* Get height at this intersection point */
			int height = map_Height(x,y), heightDif;
			float newPitch;

			if (HasTallStructure)
			{
				height += TALLOBJECT_ADJUST;
			}

			if (height <= help->height)
			{
				heightDif = 0;
			}
			else
			{
				heightDif = height - help->height;
			}

			/* Work out the angle to this point from start point */
			newPitch = rad2degf(atan2f(heightDif, dist));

			/* Is this the steepest we've found? */
			if (newPitch > help->pitch)
			{
				/* Yes, then keep a record of it */
				help->pitch = newPitch;
			}
			//---

#ifdef TEST_RAY
			pos.x = x;
			pos.y = height;
			pos.z = y;
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,false,NULL,0);
#endif
		}

		/* Not at edge yet - so exit */
		return true;
	}

	/* We've hit edge of grid - so exit!! */
	return false;
}

void	getBestPitchToEdgeOfGrid(UDWORD x, UDWORD y, UDWORD direction, SDWORD *pitch)
{
	HeightCallbackHelp_t help = { map_Height(x,y), 0.0f };

	rayCast(x, y, direction % NUM_RAYS, 5430, getTileHeightCallback, NULL);

	*pitch = help.pitch;
}

void	getPitchToHighestPoint( UDWORD x, UDWORD y, UDWORD direction,
							   UDWORD thresholdDistance, SDWORD *pitch)
{
	HighestCallbackHelp_t help = { thresholdDistance, map_Height(x,y), map_Height(x,y), 0.0f };

	rayCast(x, y, direction % NUM_RAYS, 3000, getTileHighestCallback, NULL);

	*pitch = help.pitch;
}
