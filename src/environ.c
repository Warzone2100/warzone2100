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
Environ.c - handles the enviroment stuff that's stored in tables
used for the mist and water effects. These are preprocessed.
*/


// -------------------------------------------------------------------------------
#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "display3d.h"
#include "environ.h"
#include "map.h"

// -------------------------------------------------------------------------------
#define RANDOMLY_ONE_OR_MINUS_ONE	(rand()%2 ? -1 : 1)
#define ENVIRON_WATER_INIT_VALUE	(10 + (rand()%10))
#define ENVIRON_LAND_INIT_VALUE		(32 + (rand()%32))
//#define ENVIRON_WATER_DATA_VALUE	(70 + (rand()%30))
#define ENVIRON_WATER_DATA_VALUE	(155 + (100-rand()%200))
#define ENVIRON_LAND_DATA_VALUE		(0)
#define ENVIRON_WATER_LOWEST		(0.0f)
#define ENVIRON_WATER_HIGHEST		(20.0f)
#define ENVIRON_LAND_LOWEST			(0.0f)
#define ENVIRON_LAND_HIGHEST		(64.0f)
#define ENVIRON_WATER_SPEED			(8.0f)
#define ENVIRON_LAND_SPEED			(24.0f)

// -------------------------------------------------------------------------------
#define	BYTE_TRUE  1
#define BYTE_FALSE 0

// -------------------------------------------------------------------------------
typedef enum
{
ET_WATER,
ET_LAND
} ET_TYPE;

// -------------------------------------------------------------------------------
typedef struct environ_data
{
UBYTE	bProcess;
UBYTE	type;
float	val;
UBYTE	data;
float	vec;
}ENVIRON_DATA;

// -------------------------------------------------------------------------------
ENVIRON_DATA	*pEnvironData = NULL;
static	BOOL	bWaterOnMap = FALSE;

// -------------------------------------------------------------------------------
BOOL	waterOnMap(void)
{
	return(bWaterOnMap);
}
// -------------------------------------------------------------------------------
//this function just allocates the memory now for MaxMapWidth, MaxMapHeight
BOOL    environInit( void )
{
	pEnvironData = (ENVIRON_DATA*)malloc(sizeof(struct environ_data) * MAP_MAXWIDTH * MAP_MAXHEIGHT);
	if(!pEnvironData)
	{
		debug( LOG_ERROR, "Can't get memory for the environment data" );
		abort();
		return FALSE;
	}
    return TRUE;
}

//this function is called whenever the map changes - load new level or return from an offWorld map
void environReset(void)
{
UDWORD	i,j,index;
MAPTILE	*psTile;

	if(pEnvironData == NULL ) // loading map preview..
	{
		return;
	}

	bWaterOnMap = FALSE;
	for(i=0; i<mapHeight; i++)
	{
		for(j=0; j<mapWidth; j++)
		{
			index = (i*mapWidth) + j;
			psTile = mapTile(j,i);
			if(TERRAIN_TYPE(psTile) == TER_WATER)
			{
				bWaterOnMap = TRUE;
				pEnvironData[index].type = ET_WATER;
				pEnvironData[index].val = MAKEFRACT(ENVIRON_WATER_INIT_VALUE);
				pEnvironData[index].data = ENVIRON_WATER_DATA_VALUE;
				pEnvironData[index].bProcess = BYTE_TRUE;
			}
			else
			{
				pEnvironData[index].type = ET_LAND;
				pEnvironData[index].val = MAKEFRACT(0);//ENVIRON_LAND_INIT_VALUE;
				pEnvironData[index].data = ENVIRON_LAND_DATA_VALUE;
				pEnvironData[index].bProcess = BYTE_FALSE;
			}

			pEnvironData[index].vec = MAKEFRACT(RANDOMLY_ONE_OR_MINUS_ONE);
		}
	}
}

// -------------------------------------------------------------------------------
void	environUpdate( void )
{
	UDWORD	i,j;
	UDWORD	index;
	float	value,newValue;
	float	increment = 0;
	float	lowest = 0;
	float	highest = 0;
	UDWORD	startX,startY,endX,endY;
	float	fraction;

	//at the moment this function is getting called between levels and so crashes - quick check here for now
	if (pEnvironData == NULL)
	{
		return;
	}

	/* Only process the ones on the grid. Find top left */
	if(player.p.x>=0)
	{
		startX = player.p.x/TILE_UNITS;
	}
	else
	{
		startX = 0;
	}
	if(player.p.z >= 0)
	{
		startY = player.p.z/TILE_UNITS;
	}
	else
	{
		startY = 0;
	}

	/* Find bottom right */
	endX = startX + visibleXTiles;
	endY = startY + visibleYTiles;

	/* Clip, as we may be off map */
	if(endX>mapWidth-1) endX = mapWidth-1;
	if(endY>mapHeight-1) endY = mapHeight-1;

	/* Find frame interval */
	fraction = MAKEFRACT(frameTime)/GAME_TICKS_PER_SEC;

	/* Go through the grid */
	for(i=startY; i<endY; i++)
	{
		for(j=startX; j<endX; j++)
		{
			/* Get our index */
			index = (i*mapWidth) + j;

			/* Is it being updated? */
			if(pEnvironData[index].bProcess)
			{
				/* Get old value */
				value = pEnvironData[index].val;

				/* Establish extents for movement */
				switch(pEnvironData[index].type)
				{
				case ET_WATER:
					lowest = ENVIRON_WATER_LOWEST;
					highest = ENVIRON_WATER_HIGHEST;
					increment = ((MAKEFRACT(ENVIRON_WATER_SPEED)*pEnvironData[index].vec) * fraction);
					break;
				case ET_LAND:
					lowest = ENVIRON_LAND_LOWEST;
					highest = ENVIRON_LAND_HIGHEST;
					increment = ((MAKEFRACT(ENVIRON_LAND_SPEED)*pEnvironData[index].vec) * fraction);
					break;
				default:
					debug( LOG_ERROR, "Weird environment type found" );
					abort();
					break;
				}

				/* Check bounds */
				if(value + increment <= lowest )
				{
					newValue = lowest;
					/* Flip sign */
		  			pEnvironData[index].vec *= -1;
				}
				else if(value + increment > highest)
				{
					newValue = highest;
					/* Flip sign */
		  			pEnvironData[index].vec *= -1;
				}
				else
				{
				   	/* Bounds are fine */
					newValue = value + increment;
				}

				/* Store away new value */
				pEnvironData[index].val = newValue;
			}
		}
	}
}

// -------------------------------------------------------------------------------
UDWORD	environGetValue( UDWORD x, UDWORD y )
{
SDWORD	retVal;

	retVal = MAKEINT(pEnvironData[(y*mapWidth) + x].val);
	if(retVal<0) retVal = 0;
	return(retVal);

}
// -------------------------------------------------------------------------------
UDWORD	environGetData( UDWORD x, UDWORD y )
{
SDWORD	retVal;
	retVal = (pEnvironData[(y*mapWidth) + x].data);
	if(retVal<0) retVal = 0;
	return(retVal);
}

// -------------------------------------------------------------------------------
/* Return linear interpolated mist value of x,y */
extern UDWORD map_MistValue(UDWORD x, UDWORD y)
{
	UDWORD	retVal;
	UDWORD tileX, tileY, tileYOffset;
	SDWORD h0, hx, hy, hxy;
	SDWORD dx, dy, ox, oy;
/*	ASSERT( x < (mapWidth << TILE_SHIFT),
		"mapHeight: x coordinate bigger than map width" );
	ASSERT( y < (mapHeight<< TILE_SHIFT),
		"mapHeight: y coordinate bigger than map height" );
*/
    x = x > SDWORD_MAX ? 0 : x;//negative SDWORD passed as UDWORD
    x = x >= (mapWidth << TILE_SHIFT) ? ((mapWidth-1) << TILE_SHIFT) : x;
    y = y > SDWORD_MAX ? 0 : y;//negative SDWORD passed as UDWORD
	y = y >= (mapHeight << TILE_SHIFT) ? ((mapHeight-1) << TILE_SHIFT) : y;

	/* Tile comp */
	tileX = x >> TILE_SHIFT;
	tileY = y >> TILE_SHIFT;

	/* Inter tile comp */
	ox = map_round(x);
	oy = map_round(y);

	/* If this happens, then get quick height */
	if(!x && !y)
	{
		return(map_TileMistValue(tileX,tileY));
	}

	tileYOffset = (tileY * mapWidth);

//	ox = (SDWORD)x - (SDWORD)(tileX << TILE_SHIFT);
//	oy = (SDWORD)y - (SDWORD)(tileY << TILE_SHIFT);

	ASSERT( ox < TILE_UNITS, "mapHeight: x offset too big" );
	ASSERT( oy < TILE_UNITS, "mapHeight: y offset too big" );
	ASSERT( ox >= 0, "mapHeight: x offset too small" );
	ASSERT( oy >= 0, "mapHeight: y offset too small" );

	//different code for 4 different triangle cases
	if (psMapTiles[tileX + tileYOffset].texture & TILE_TRIFLIP)
	{
		if ((ox + oy) > TILE_UNITS)//tile split top right to bottom left object if in bottom right half
		{
			ox = TILE_UNITS - ox;
			oy = TILE_UNITS - oy;
			hy = MAKEINT(pEnvironData[tileX + tileYOffset + mapWidth].val);
			hx = MAKEINT(pEnvironData[tileX + 1 + tileYOffset].val);
			hxy= MAKEINT(pEnvironData[tileX + 1 + tileYOffset + mapWidth].val);

			dx = ((hy - hxy) * ox )/ TILE_UNITS;
			dy = ((hx - hxy) * oy )/ TILE_UNITS;

			retVal = (UDWORD)(hxy + dx + dy);
			return(retVal*4);
		}
		else //tile split top right to bottom left object if in top left half
		{
			h0 = MAKEINT(pEnvironData[tileX + tileYOffset].val);
			hy = MAKEINT(pEnvironData[tileX + tileYOffset + mapWidth].val);
			hx = MAKEINT(pEnvironData[tileX + 1 + tileYOffset].val);

			dx = ((hx - h0) * ox )/ TILE_UNITS;
			dy = ((hy - h0) * oy )/ TILE_UNITS;

			retVal = (UDWORD)(h0 + dx + dy);
			return (retVal*4);
		}
	}
	else
	{
		if (ox > oy) //tile split topleft to bottom right object if in top right half
		{
			h0 = MAKEINT(pEnvironData[tileX + tileYOffset].val);
			hx = MAKEINT(pEnvironData[tileX + 1 + tileYOffset].val);
			hxy= MAKEINT(pEnvironData[tileX + 1 + tileYOffset + mapWidth].val);

			dx = ((hx - h0) * ox )/ TILE_UNITS;
			dy = ((hxy - hx) * oy )/ TILE_UNITS;
			retVal = (UDWORD)(h0 + dx + dy);
			return (retVal*4);
		}
		else //tile split topleft to bottom right object if in bottom left half
		{
			h0 = MAKEINT(pEnvironData[tileX + tileYOffset].val);
			hy = MAKEINT(pEnvironData[tileX + tileYOffset + mapWidth].val);
			hxy = MAKEINT(pEnvironData[tileX + 1 + tileYOffset + mapWidth].val);

			dx = ((hxy - hy) * ox )/ TILE_UNITS;
			dy = ((hy - h0) * oy )/ TILE_UNITS;

			retVal = (UDWORD)(h0 + dx + dy);
			return (retVal*4);
		}
	}
	return 0;
}

// -------------------------------------------------------------------------------
/* Return height of tile at x,y */
UDWORD map_TileMistValue(UDWORD x, UDWORD y)
{
    x = x >= (mapWidth) ? (mapWidth-1) : x;
	y = y >= (mapHeight) ? (mapHeight-1) : y;

	ASSERT( x < mapWidth,
		"mapTile: x coordinate bigger than map width" );
	ASSERT( y < mapHeight,
		"mapTile: y coordinate bigger than map height" );

	return (MAKEINT(pEnvironData[x + (y * mapWidth)].val)*4);
}
// -------------------------------------------------------------------------------
void	environShutDown( void )
{
	if(pEnvironData)
	{
		free(pEnvironData);
	}
}
