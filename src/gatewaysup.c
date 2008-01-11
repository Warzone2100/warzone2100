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
 * Additional functions for the Gateway system.
 * Only needed for map preprocessing.
 *
 */

#include "lib/framework/frame.h"
#include "map.h"

#include <assert.h>
#include <string.h>
#include "gateway.h"

// Structures and defines for SeedFill().
typedef int Pixel;		/* 1-channel frame buffer assumed */

struct FRect {			/* window: a discrete 2-D rectangle */
    int x0, y0;			/* xmin and ymin */
    int x1, y1;			/* xmax and ymax (inclusive) */
};

struct Segment {
	int y;			//
	int xl;			// Filled horizontal segment of scanline y for xl<=x<=xr.
	int xr;			// Parent segment was on line y-dy.  dy=1 or -1
	int dy;			//
};

#define MAX_STACK 10000		/* max depth of stack */

#define PUSH(Y, XL, XR, DY)	/* push new segment on stack */ \
	debug( LOG_GATEWAY, "PUSH y %d x %d->%d dy %d\n", Y, XL, XR, DY); \
    if (sp<stack+MAX_STACK) \
    {sp->y = Y; sp->xl = XL; sp->xr = XR; sp->dy = DY; sp++;}

#define POP(Y, XL, XR, DY)	/* pop segment off stack */ \
    {sp--; Y = sp->y+(DY = sp->dy); XL = sp->xl; XR = sp->xr;} \
	debug( LOG_GATEWAY, "POP  y %d x %d->%d dy %d\n", Y, XL, XR, DY);


// whether the flood fill is running over water
static BOOL bGwWaterFlood = FALSE;;

// check for a blocking tile for the flood fill
static BOOL gwFloodBlock(SDWORD x, SDWORD y);

// generate the zone equivalence tables
static BOOL gwGenerateZoneEquiv(SDWORD numZones);


// Flood fill a map zone from a given point
// stopping at blocking tiles
/*
 * fill: set the pixel at (x,y) and all of its 4-connected neighbors
 * with the same pixel value to the new pixel value nv.
 * A 4-connected neighbor is a pixel above, below, left, or right of a pixel.
 */
static void gwSeedFill(SDWORD x, SDWORD y, SDWORD nv)
{
    int l, x1, x2, dy;
    Pixel ov;							/* old pixel value */

    struct Segment stack[MAX_STACK], *sp = stack;	/* stack of filled segments */

    ov = gwGetZone(x, y);		/* read pv at seed point */

    if (ov==nv) {
		return;
	}

    PUSH(y, x, x, 1);			/* needed in some cases */
    PUSH(y+1, x, x, -1);		/* seed segment (popped 1st) */

    while (sp>stack) {
		/* pop segment off stack and fill a neighboring scan line */
		POP(y, x1, x2, dy);
		/*
		 * segment of scan line y-dy for x1<=x<=x2 was previously filled,
		 * now explore adjacent pixels in scan line y
		 */
		debug( LOG_GATEWAY, "-ve (%d,%d)->", x1,y);
		for (x=x1; !gwFloodBlock(x,y) && (gwGetZone(x, y)==ov); x--) {
			gwSetZone(x, y, nv);
		}
		debug( LOG_GATEWAY, "(%d,%d) %s\n", x+1,y, x<x1?"OK":"");

		if (x>=x1) {
			goto skip;
		}

		l = x+1;
		if (l<x1) {
			PUSH(y, l, x1-1, -dy);		/* leak on left? */
		}
		x = x1+1;

		do {
			debug( LOG_GATEWAY, "+ve (%d,%d)->", x,y);
			for (;!gwFloodBlock(x,y) && (gwGetZone(x, y)==ov); x++) {
				gwSetZone(x, y, nv);
			}
			debug( LOG_GATEWAY, "(%d,%d) %s\n", x-1,y, (x>l)&&(x>x1+1)?"OK":"");

			PUSH(y, l, x-1, dy);

			if (x>x2+1) {
				PUSH(y, x2+1, x-1, -dy);	/* leak on right? */
			}

skip:
			for (x++; x<=x2 && (gwFloodBlock(x,y) || gwGetZone(x, y)!=ov); x++);
			l = x;
		} while (x<=x2);
    }
}


// set the tiles a gateway covers to a zone
static void gwSetGatewayZone(GATEWAY *psGate, SDWORD zone)
{
	SDWORD	pos;

	if (psGate->x1 == psGate->x2)
	{
		for(pos = psGate->y1; pos <= psGate->y2; pos++)
		{
			gwSetZone(psGate->x1, pos, zone);
		}
	}
	else
	{
		for(pos = psGate->x1; pos <= psGate->x2; pos++)
		{
			gwSetZone(pos, psGate->y1, zone);
		}
	}
}


// find the first zone on a line
static SDWORD gwFindFirstZone(SDWORD y)
{
	SDWORD	x, zone;

	zone = 0;
	while (zone == 0 && y < gwMapHeight())
	{
		for(x=0; x<gwMapWidth() && zone == 0; x+= 1)
		{
			zone = gwGetZone(x,y);
		}
		y++;
	}

	return zone;
}


// Process the map to create all the map zones
BOOL gwProcessMap(void)
{
	GATEWAY		*psCurr;
	SDWORD		currZone, zoneTest;
	SDWORD		floodX,floodY;
	SDWORD		gatewayZone;
	SDWORD		prevZone, nextZone, x,y;

	// create the blank zone map
	if (!gwCreateBlankZoneMap())
	{
		return FALSE;
	}

	// nothing more to do if there are no gateways
	if (psGateways == NULL)
	{
		return TRUE;
	}

	// reset the gateway zone entries
	for(psCurr = psGateways; psCurr; psCurr = psCurr->psNext)
	{
		psCurr->zone1 = 0;
		psCurr->zone2 = 0;
	}

	// flood fill from all the gateways
	psCurr = psGateways;
	currZone = 1;
	do
	{
		debug( LOG_GATEWAY, "Processing gateway (%d,%d)->(%d,%d)\n",
			psCurr->x1,psCurr->y1, psCurr->x2,psCurr->y2);

		// do a flood fill from the current gateway
		if (psCurr->zone1 == 0)
		{
			// first zone is left/above
			if (psCurr->x1 == psCurr->x2)
			{
				// vertical - go left
				floodX = psCurr->x1 - 1;
				floodY = (psCurr->y2 - psCurr->y1)/2 + psCurr->y1;
			}
			else
			{
				// horizontal - go above
				floodX = (psCurr->x2 - psCurr->x1)/2 + psCurr->x1;
				floodY = psCurr->y1 - 1;
			}
		}
		else // psCurr->zone2 == 0
		{
			// second zone is right/below
			if (psCurr->x1 == psCurr->x2)
			{
				// vertical - go right
				floodX = psCurr->x1 + 1;
				floodY = (psCurr->y2 - psCurr->y1)/2 + psCurr->y1;
			}
			else
			{
				// horizontal - go below
				floodX = (psCurr->x2 - psCurr->x1)/2 + psCurr->x1;
				floodY = psCurr->y1 + 1;
			}
		}

		// see if a previous flood fill reached this gateway
		zoneTest = gwGetZone(floodX,floodY);
		if (zoneTest != 0)
		{
			// simple case just have to link the gateway to the zone
			gatewayZone = zoneTest;
		}
		else
		{
			// check the zones havn't overflowed
			if (currZone > UBYTE_MAX)
			{
				debug( LOG_ERROR, "gwProcessMap: too many zones\n" );
				abort();
				return FALSE;
			}

			// floodFill
			if (gwTileIsWater(floodX,floodY))
			{
				bGwWaterFlood = TRUE;
			}
			gwSeedFill(floodX,floodY, currZone);
			bGwWaterFlood = FALSE;
			gatewayZone = currZone;

			currZone += 1;
		}

		// set the gateway zone
		if (psCurr->zone1 == 0)
		{
			psCurr->zone1 = (UBYTE)gatewayZone;
			// the gateway is always in it's own zone1
			gwSetGatewayZone(psCurr, gatewayZone);
		}
		else
		{
			psCurr->zone2 = (UBYTE)gatewayZone;
		}

		// see if there are any gateways left to process
		for(psCurr=psGateways; psCurr; psCurr = psCurr->psNext)
		{
			if ((psCurr->zone1 == 0) || (psCurr->zone2 == 0))
			{
				break;
			}
		}
	} while (psCurr != NULL);

	// fill in any areas that are left
	for(y=0; y<gwMapHeight(); y+= 1)
	{
		for(x=0; x<gwMapWidth(); x+= 1)
		{
			if (gwTileIsWater(x,y) &&
				gwGetZone(x,y) == 0)
			{
				// check the zones havn't overflowed
				if (currZone > UBYTE_MAX)
				{
					debug( LOG_ERROR, "gwProcessMap: too many zones\n" );
					abort();
					return FALSE;
				}

				bGwWaterFlood = TRUE;
				gwSeedFill(x,y, currZone);
				bGwWaterFlood = FALSE;
				currZone += 1;
			}
			else if (!gwFloodBlock(x,y) &&
					 gwGetZone(x,y) == 0)
			{
				// check the zones havn't overflowed
				if (currZone > UBYTE_MAX)
				{
					debug( LOG_ERROR, "gwProcessMap: too many zones\n" );
					abort();
					return FALSE;
				}

				gwSeedFill(x,y, currZone);
				currZone += 1;
			}
		}
	}

	// now average out the zones so that blocking tiles are in the same zone as their neighbour
	for(y=0; y<gwMapHeight(); y+= 1)
	{
		prevZone = gwFindFirstZone(y);
		for(x=0; x<gwMapWidth(); x+= 1)
		{
			nextZone = gwGetZone(x,y);
			if (gwFloodBlock(x,y) && nextZone == 0)
			{
				gwSetZone(x,y, prevZone);
			}
			else if (nextZone != 0)
			{
				prevZone = nextZone;
			}
		}
	}


#ifdef DEBUG_GROUP4
	{
		SDWORD	x,y, size, pos;
		UBYTE	*pCode;

		size = gwMapHeight() * sizeof(UBYTE *);
		for(y=0; y<gwMapHeight(); y+=1)
		{
			pCode = apRLEZones[y];
			pos = 0;
			x = 0;
			while (x < gwMapWidth())
			{
				x += pCode[pos];
				pos += 2;
				size += 2;
			}
		}

		debug( LOG_GATEWAY, "Gateway Zones : %d\n", currZone);
		debug( LOG_GATEWAY, "RLE Zone map size: %d\n", size);
	}
#endif

	if (!gwGenerateZoneEquiv(currZone))
	{
		return FALSE;
	}

	return TRUE;
}


// see if a zone is already in the equivalence array
static BOOL gwEquivZonePresent(UBYTE *aEquiv, SDWORD numEquiv, SDWORD zone)
{
	SDWORD	i;

	for(i=0; i<numEquiv; i++)
	{
		if (aEquiv[i] == zone)
		{
			return TRUE;
		}
	}

	return FALSE;
}


// check a neighbour tile for equivalence
static void gwCheckNeighbourEquiv(SDWORD zone, SDWORD x, SDWORD y)
{
	SDWORD	nZone;

	nZone = gwGetZone(x,y);
	if (nZone != zone &&
		!gwFloodBlock(x,y) &&
		!gwEquivZonePresent(apEquivZones[zone], aNumEquiv[zone], nZone))
	{
		debug( LOG_GATEWAY, "zone %d equivalent to zone %d\n", zone, nZone);
		apEquivZones[zone][aNumEquiv[zone]] = (UBYTE)nZone;
		aNumEquiv[zone] += 1;
	}
}


// generate the zone equivalence tables
static BOOL gwGenerateZoneEquiv(SDWORD numZones)
{
	SDWORD		x,y, i;
	UBYTE		aEquiv[UBYTE_MAX];
	SDWORD		numEquiv, localZone;

	if (apEquivZones == NULL)
	{
		if (!gwNewEquivTable(numZones))
		{
			return FALSE;
		}
	}

	// just allocate the maximum space when generating the table
	memset(aEquiv, 0, sizeof(aEquiv));
	for(i=0; i<gwNumZones; i+= 1)
	{
		if (!gwSetZoneEquiv(i, gwNumZones, aEquiv))
		{
			return FALSE;
		}
		aNumEquiv[i] = 0;
	}

	// go over the map - skipping edge tiles to avoid going over the
	// edge of the map
	numEquiv = 0;
	for(y=1; y<gwMapHeight()-1; y+= 1)
	{
		for(x=1; x<gwMapWidth()-1; x+= 1)
		{
			if (gwTileIsWater(x,y))
			{
				// found a water tile - see if it is next to a different zone
				localZone = gwGetZone(x,y);

				gwCheckNeighbourEquiv(localZone, x-1,y);
				gwCheckNeighbourEquiv(localZone, x,y-1);
				gwCheckNeighbourEquiv(localZone, x+1,y);
				gwCheckNeighbourEquiv(localZone, x,y+1);
			}
		}
	}

	return TRUE;
}


/******************************************************************************************************/
/*                            RLE Zone data access functions                                          */

// Create a new blank RLE Zone map suitable for creating zones in
BOOL gwCreateBlankZoneMap(void)
{
	SDWORD		i;

	if (apRLEZones != NULL)
	{
		gwFreeZoneMap();
	}

	apRLEZones = (UBYTE**)malloc(sizeof(UBYTE *) * gwMapHeight());
	if (apRLEZones == NULL)
	{
		debug( LOG_ERROR, "gwCreateBlankZoneMap: Out of memory" );
		abort();
		return FALSE;
	}
	for(i=0; i< gwMapHeight(); i++)
	{

		apRLEZones[i] = (UBYTE*)malloc(gwMapWidth() * 2);

		if (apRLEZones[i] == NULL)
		{
			debug( LOG_ERROR, "gwCreateBlankZoneMap: Out of memory" );
			abort();
			return FALSE;
		}
	}

	// set all the zones to zero
	for(i=0; i<gwMapHeight(); i++)
	{
		apRLEZones[i][0] = (UBYTE)gwMapWidth();
		apRLEZones[i][1] = 0;
	}

	return TRUE;
}


// Decompress a line of the zone map
// pBuffer should point to a buffer of gwMapWidth() bytes
static void gwDecompressLine(SDWORD line, UBYTE *pBuffer)
{
	SDWORD	rlePos, bufPos, count,zone, store;

	rlePos = 0;
	bufPos = 0;
	while (bufPos < gwMapWidth())
	{
		count = apRLEZones[line][rlePos];
		zone  = apRLEZones[line][rlePos + 1];
		rlePos += 2;

		for(store=0; store < count; store ++)
		{
			ASSERT( bufPos < gwMapWidth(),
				"gwDecompressLine: Invalid RLE code" );

			pBuffer[bufPos] = (UBYTE)zone;
			bufPos += 1;
		}
	}
}

// Compress a line of the zone map
// pBuffer should point to a buffer of gwMapWidth() bytes
static void gwCompressLine(SDWORD line, UBYTE *pBuffer)
{
	SDWORD rlePos, bufPos, count,zone;

	rlePos = 0;
	bufPos = 0;
	while (bufPos < gwMapWidth())
	{
		zone = pBuffer[bufPos];
		count = 0;
		while ((pBuffer[bufPos] == zone) && (bufPos < gwMapWidth()))
		{
			count += 1;
			bufPos += 1;
		}

		apRLEZones[line][rlePos] = (UBYTE)count;
		apRLEZones[line][rlePos + 1] = (UBYTE)zone;

		rlePos += 2;
	}
}


// Set the zone for a coordinate
void gwSetZone(SDWORD x, SDWORD y, SDWORD zone)
{
	UBYTE	aBuffer[GW_MAP_MAXWIDTH];

	ASSERT( (x >= 0) && (x < gwMapWidth()) && (y >= 0) && (y < gwMapHeight()),
		"gwSetZone: invalid coordinates" );

	gwDecompressLine(y, aBuffer);

	aBuffer[x] = (UBYTE)zone;

	gwCompressLine(y, aBuffer);
}


/******************************************************************************************************/
/*                   Gateway data access functions                                                    */

// check for a blocking tile for the flood fill
static BOOL gwFloodBlock(SDWORD x, SDWORD y)
{
	MAPTILE		*psTile;
	SDWORD		type;
	BOOL		gateway;

	if ((x < 0) || (x >= gwMapWidth()) ||
		(y < 0) || (y >= gwMapHeight()))
	{
		return TRUE;
	}

	psTile = mapTile(x,y);
	type = terrainType(psTile);
	gateway = (psTile->tileInfoBits & BITS_GATEWAY) != 0;

	return gateway ||
		   ( !bGwWaterFlood && ((type == TER_CLIFFFACE) || (type == TER_WATER))) ||
		   ( bGwWaterFlood && (type != TER_WATER) );
}
