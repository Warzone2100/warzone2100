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
used for the water effects. These are preprocessed.
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
#define ENVIRON_WATER_DATA_VALUE	(rand()%50)
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

	for(i=0; i<mapHeight; i++)
	{
		for(j=0; j<mapWidth; j++)
		{
			index = (i*mapWidth) + j;
			psTile = mapTile(j,i);
			if(terrainType(psTile) == TER_WATER)
			{
				pEnvironData[index].type = ET_WATER;
				pEnvironData[index].val = (float)ENVIRON_WATER_INIT_VALUE;
				pEnvironData[index].data = ENVIRON_WATER_DATA_VALUE;
				pEnvironData[index].bProcess = BYTE_TRUE;
			}
			else
			{
				pEnvironData[index].type = ET_LAND;
				pEnvironData[index].val = 0.f; //ENVIRON_LAND_INIT_VALUE;
				pEnvironData[index].data = ENVIRON_LAND_DATA_VALUE;
				pEnvironData[index].bProcess = BYTE_FALSE;
			}

			pEnvironData[index].vec = (float)RANDOMLY_ONE_OR_MINUS_ONE;
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
void	environShutDown( void )
{
	if(pEnvironData)
	{
		free(pEnvironData);
		pEnvironData = NULL;
	}
}
