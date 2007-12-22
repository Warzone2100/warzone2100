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
 * RayCast.c
 *
 * raycasting routine that gives intersection points with map tiles
 *
 */

#include <stdio.h>

#include "lib/framework/frame.h"
#include "lib/framework/trig.h"

#include "objects.h"
#include "map.h"

#include "raycast.h"
#include "display3d.h"
//#ifdef ALEXM
#include "effects.h"
//#endif


// accuracy for the raycast lookup tables
#define RAY_ACC		12

#define RAY_ACCMUL	(1<<RAY_ACC)

// Get the tile from tile coords
#define RAY_TILE(x,y) mapTile((x),(y))

// control the type of clip method
// 0 - clip on ray length (faster but it doesn't always work :-)
// 1 - clip on coordinates (accurate but possibly a bit slower)
#define RAY_CLIP	1

// ray point
typedef struct _ray_point
{
	SDWORD	x,y;
} RAY_POINT;

/* x and y increments for each ray angle */
static SDWORD	rayDX[NUM_RAYS], rayDY[NUM_RAYS];
static SDWORD	rayHDist[NUM_RAYS], rayVDist[NUM_RAYS];
//static float	rayTan[NUM_RAYS], rayCos[NUM_RAYS], raySin[NUM_RAYS];
static SDWORD	rayFPTan[NUM_RAYS], rayFPInvTan[NUM_RAYS];
static SDWORD	rayFPInvCos[NUM_RAYS], rayFPInvSin[NUM_RAYS];

/* Initialise the ray tables */

BOOL rayInitialise(void)
{
	SDWORD	i;
	float	angle = 0.f;
	float	val;

	for(i=0; i<NUM_RAYS; i++)
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
		rayFPTan[i] = MAKEINT(FRACTmul(val, (float)RAY_ACCMUL));
		rayFPInvTan[i] = MAKEINT(FRACTdiv((float)RAY_ACCMUL, val));

		// Set up the trig tables for calculating the offset distances
		val = (float)sin(angle);
		if(val == 0) {
			val = (float)1;
		}
		rayFPInvSin[i] = MAKEINT(FRACTdiv((float)RAY_ACCMUL, val));
		if (i >= NUM_RAYS/2)
		{
			rayVDist[i] = MAKEINT(FRACTdiv((float)-TILE_UNITS, val));
		}
		else
		{
			rayVDist[i] = MAKEINT(FRACTdiv((float)TILE_UNITS, val));
		}

		val = (float)cos(angle);
		if(val == 0) {
			val = (float)1;
		}
		rayFPInvCos[i] = MAKEINT(FRACTdiv((float)RAY_ACCMUL, val));
		if (i < NUM_RAYS/4 || i > 3*NUM_RAYS/4)
		{
			rayHDist[i] = MAKEINT(FRACTdiv((float)TILE_UNITS, val));
		}
		else
		{
			rayHDist[i] = MAKEINT(FRACTdiv((float)-TILE_UNITS, val));
		}

		angle += RAY_ANGLE;
	}

	return TRUE;
}


// playstation
//void rayCast(UDWORD x, UDWORD y, UDWORD ray, UDWORD length, RAY_CALLBACK callback)
//{
//	static UDWORD Tx;
//	static UDWORD Ty;
//	static UDWORD Tray;
//	static UDWORD Tlength;
//	static RAY_CALLBACK Tcallback;
//
//	Tx = x;
//	Ty = y;
//	Tray = ray;
//	Tlength = length;
//	Tcallback = callback;
//	// Stack in the DCache.
//	SetSpDCache();
//	rayC(Tx, Ty, Tray, Tlength, Tcallback);
//	SetSpNormal();
//}

/* cast a ray from x,y (world coords) at angle ray (0-360)
 * The ray angle starts at zero along the positive y axis and
 * increases towards -ve X.
 *
 * Sorry about the wacky angle set up but that was what I thought
 * warzone used, but turned out not to be after I wrote it.
 */
void rayCast(UDWORD x, UDWORD y, UDWORD ray, UDWORD length, RAY_CALLBACK callback)
{
	SDWORD		hdInc=0, vdInc=0;		// increases in x and y distance per intersection
	SDWORD		hDist, vDist;		// distance to current horizontal and vertical intersectionse
	RAY_POINT	sVert = { 0, 0 }, sHoriz = { 0, 0 };
	SDWORD		vdx=0, hdy=0;			// vertical x increment, horiz y inc
#if RAY_CLIP == 0
	SDWORD		newLen, clipLen;	// ray length after clipping
#endif

	// Clipping is done with the position offset by TILE_UNITS/4 to account
	// for the rounding errors when the intersection length is calculated.
	// Bit of a hack but I'm pretty sure it doesn't let through anything
	// that should be clippped.

#if RAY_CLIP == 0
	// Initial clip length is just the length of the ray
	clipLen = (SDWORD)length;
#endif

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

#if RAY_CLIP == 0
			// clipping
			newLen = ((world_coord(mapHeight) - ((SDWORD)y + TILE_UNITS / 4))
						* rayFPInvCos[ray])	>> RAY_ACC;
			if (newLen < clipLen)
			{
				clipLen = newLen;
			}
#endif
		}
		else
		{
			// intersection
			sHoriz.y = (y & ~TILE_MASK) - 1;
			hdy = -TILE_UNITS;

#if RAY_CLIP == 0
			// clipping
			newLen = ((TILE_UNITS/4 - (SDWORD)y) * rayFPInvCos[ray]) >> RAY_ACC;
			if (newLen < clipLen)
			{
				clipLen = newLen;
			}
#endif
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
	if (ray != 0 && ray != NUM_RAYS/2)
	{
		if (ray >= NUM_RAYS/2)
		{
			// intersection
			sVert.x = (x & ~TILE_MASK) + TILE_UNITS;
			vdx = TILE_UNITS;

#if RAY_CLIP == 0
			// clipping
			newLen = ((((SDWORD)x + TILE_UNITS / 4) - world_coord(mapWidth))
						* rayFPInvSin[ray])	>> RAY_ACC;
			if (newLen < clipLen)
			{
				clipLen = newLen;
			}
#endif
		}
		else
		{
			// intersection
			sVert.x = (x & ~TILE_MASK) - 1;
			vdx = -TILE_UNITS;

#if RAY_CLIP == 0
			// clipping
			newLen = (((SDWORD)x - TILE_UNITS/4) * rayFPInvSin[ray]) >> RAY_ACC;
			if (newLen < clipLen)
			{
				clipLen = newLen;
			}
#endif
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

#if RAY_CLIP == 0
	while(hDist < clipLen ||
		  vDist < clipLen)
	{
		// choose the next closest intersection
		if (hDist < vDist)
		{
			// pass through the current intersection, converting x from fixed point
			if (!callback( sHoriz.x >> RAY_ACC,sHoriz.y, hDist))
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
			// pass through the current intersection, converting y from fixed point
			if (!callback( sVert.x,sVert.y >> RAY_ACC, vDist))
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
#elif RAY_CLIP == 1
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
			if (!callback( sHoriz.x >> RAY_ACC,sHoriz.y, hDist))
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
			if (!callback( sVert.x,sVert.y >> RAY_ACC, vDist))
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
#endif
}


// Calculate the angle to cast a ray between two points
UDWORD rayPointsToAngle(SDWORD x1,SDWORD y1, SDWORD x2,SDWORD y2)
{
	SDWORD		xdiff, ydiff;
	SDWORD		angle;

	xdiff = x2 - x1;
	ydiff = y1 - y2;


	angle = (SDWORD)((NUM_RAYS / 2) * atan2(xdiff, ydiff) / M_PI);


	angle += NUM_RAYS/2;
	angle = angle % NUM_RAYS;

	ASSERT( angle >= 0 && angle < NUM_RAYS,
		"rayPointsToAngle: angle out of range" );

	return (UDWORD)angle;
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
// typedef BOOL (*RAY_CALLBACK)(SDWORD x, SDWORD y, SDWORD dist);
//void rayCast(UDWORD x, UDWORD y, UDWORD ray, UDWORD length, RAY_CALLBACK callback)

//#define TEST_RAY

/* Nasty global vars - put into a structure? */
//-----------------------------------------------------------------------------------
SDWORD	gHeight;
float	gPitch;
UDWORD	gStartTileX;
UDWORD	gStartTileY;

SDWORD	gHighestHeight,gHOrigHeight;
SDWORD	gHMinDist;
float	gHPitch;


//-----------------------------------------------------------------------------------
static BOOL	getTileHighestCallback(SDWORD x, SDWORD y, SDWORD dist)
{
	SDWORD	heightDif;
	UDWORD	height;
	//Vector3i	pos;

	if(clipXY(x,y))
	{
		height = map_Height(x,y);
		if( (height > gHighestHeight) && (dist >= gHMinDist) )
		{
			heightDif = height - gHOrigHeight;
			gHPitch = RAD_TO_DEG(atan2((float)heightDif,
			                           (float)world_coord(6)));// (float)(dist - world_coord(3))));
			gHighestHeight = height;
  		}
//		pos.x = x;
//		pos.y = height;
//		pos.z = y;
//		addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,FALSE,NULL,0);
	}
	else
	{
		return(FALSE);
	}

	return(TRUE);

}
//-----------------------------------------------------------------------------------
/* Will return false when we've hit the edge of the grid */
static BOOL	getTileHeightCallback(SDWORD x, SDWORD y, SDWORD dist)
{
	SDWORD	height,heightDif;
	float	newPitch;
	BOOL HasTallStructure = FALSE;
#ifdef TEST_RAY
	Vector3i pos;
#endif

	/* Are we still on the grid? */
	if(clipXY(x,y))
	{
		HasTallStructure = TILE_HAS_TALLSTRUCTURE(mapTile(map_coord(x), map_coord(y)));

		if( (dist>TILE_UNITS) || HasTallStructure)
		{
		// Only do it the current tile is > TILE_UNITS away from the starting tile. Or..
		// there is a tall structure  on the current tile and the current tile is not the starting tile.
//		if( (dist>TILE_UNITS) ||
//			( (HasTallStructure = TILE_HAS_TALLSTRUCTURE(mapTile(map_coord(x), map_coord(y)))) &&
//			((map_coord(x) != gStartTileX) || (map_coord(y) != gStartTileY)) ) ) {
			/* Get height at this intersection point */
			height = map_Height(x,y);

			if(HasTallStructure) {
				height += 300;	//TALLOBJECT_ADJUST;
			}

			if(height<=gHeight)
			{
				heightDif = 0;
			}
			else
			{
				heightDif = height-gHeight;
			}

			/* Work out the angle to this point from start point */

			newPitch = RAD_TO_DEG(atan2((float)heightDif, (float)dist));


			/* Is this the steepest we've found? */
			if(newPitch>gPitch)
			{
				/* Yes, then keep a record of it */
				gPitch = newPitch;
			}
			//---

#ifdef TEST_RAY
			pos.x = x;
			pos.y = height;
			pos.z = y;
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,FALSE,NULL,0);
#endif
	//		if(height > gMaxRayHeight)
	//		{
	//			gMaxRayHeight = height;
	//			gRayDist = dist;
	//			return(TRUE);
	//		}
		}
	}
	else
	{
		/* We've hit edge of grid - so exit!! */
		return(FALSE);
	}

	/* Not at edge yet - so exit */
	return(TRUE);
}

void	getBestPitchToEdgeOfGrid(UDWORD x, UDWORD y, UDWORD direction, SDWORD *pitch)
{
	/* Set global var to clear */
	gPitch = 0.f;
	gHeight = map_Height(x,y);
	gStartTileX = map_coord(x);
	gStartTileY = map_coord(y);
//#ifdef TEST_RAY
//DBPRINTF(("%d\n",direction);
//#endif
	rayCast(x,y, direction%360,5430,getTileHeightCallback);
	*pitch = MAKEINT(gPitch);
}

void	getPitchToHighestPoint( UDWORD x, UDWORD y, UDWORD direction,
							   UDWORD thresholdDistance, SDWORD *pitch)
{
	gHPitch = 0.f;
	gHOrigHeight = map_Height(x,y);
	gHighestHeight = map_Height(x,y);
	gHMinDist = thresholdDistance;
	rayCast(x,y,direction%360,3000,getTileHighestCallback);
	*pitch = MAKEINT(gHPitch);
}
