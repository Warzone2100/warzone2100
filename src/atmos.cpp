/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
 * @file atmos.c
 *
 * Handles atmospherics such as snow and rain.
*/

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/piematrix.h"

#include "atmos.h"
#include "display3d.h"
#include "effects.h"
#include "hci.h"
#include "loop.h"
#include "map.h"
#include "miscimd.h"

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// Shift all this gubbins into a .h file if it makes it into game
// -----------------------------------------------------------------------------
/* Roughly one per tile */
#define	MAX_ATMOS_PARTICLES		(MAP_MAXWIDTH * MAP_MAXHEIGHT)
#define	SNOW_SPEED_DRIFT		(40 - rand()%80)
#define SNOW_SPEED_FALL			(0-(rand()%40 + 80))
#define	RAIN_SPEED_DRIFT		(rand()%50)
#define	RAIN_SPEED_FALL			(0-((rand()%300) + 700))

enum AP_TYPE
{
AP_RAIN,
AP_SNOW
};

enum AP_STATUS
{
APS_ACTIVE,
APS_INACTIVE,
};

static ATPART	asAtmosParts[MAX_ATMOS_PARTICLES];
static	UDWORD	freeParticle;
static	UDWORD	weather;

/* Setup all the particles */
void	atmosInitSystem()
{
	for (int i = 0; i < MAX_ATMOS_PARTICLES; i++)
	{
		/* None are being used initially */
		asAtmosParts[i].status = APS_INACTIVE;
	}
	/* Start at the beginning */
	freeParticle = 0;

	/* No weather to start with */
	weather	= WT_NONE;
}

// -----------------------------------------------------------------------------
/*	Makes a particle wrap around - if it goes off the grid, then it returns
	on the other side - provided it's still on world... Which it should be */
static void testParticleWrap(ATPART *psPart)
{
	/* Gone off left side */
	if(psPart->position.x < player.p.x-world_coord(visibleTiles.x)/2)
	{
		psPart->position.x += world_coord(visibleTiles.x);
	}

	/* Gone off right side */
	else if(psPart->position.x > (player.p.x + world_coord(visibleTiles.x)/2))
	{
		psPart->position.x -= world_coord(visibleTiles.x);
	}

	/* Gone off top */
	if(psPart->position.z < player.p.z - world_coord(visibleTiles.y)/2)
	{
		psPart->position.z += world_coord(visibleTiles.y);
	}

	/* Gone off bottom */
	else if(psPart->position.z > (player.p.z + world_coord(visibleTiles.y)/2))
	{
		psPart->position.z -= world_coord(visibleTiles.y);
	}
}

// -----------------------------------------------------------------------------
/* Moves one of the particles */
static void processParticle(ATPART *psPart)
{
	SDWORD	groundHeight;
	Vector3i pos;
	UDWORD	x,y;
	MAPTILE	*psTile;

	/* Only move if the game isn't paused */
	if(!gamePaused())
	{
		/* Move the particle - frame rate controlled */
 		psPart->position.x += graphicsTimeAdjustedIncrement(psPart->velocity.x);
		psPart->position.y += graphicsTimeAdjustedIncrement(psPart->velocity.y);
		psPart->position.z += graphicsTimeAdjustedIncrement(psPart->velocity.z);

		/* Wrap it around if it's gone off grid... */
	   	testParticleWrap(psPart);

		/* If it's gone off the WORLD... */
		if(psPart->position.x<0 || psPart->position.z<0 ||
		   psPart->position.x>((mapWidth-1)*TILE_UNITS) ||
		   psPart->position.z>((mapHeight-1)*TILE_UNITS) )
		{
			/* The kill it */
			psPart->status = APS_INACTIVE;
			return;
		}

		/* What height is the ground under it? Only do if low enough...*/
		if(psPart->position.y < 255*ELEVATION_SCALE)
		{
			/* Get ground height */
			groundHeight = map_Height(psPart->position.x, psPart->position.z);

			/* Are we below ground? */
			if ((int)psPart->position.y < groundHeight
			 || psPart->position.y < 0.f)
			{
				/* Kill it and return */
				psPart->status = APS_INACTIVE;
				if(psPart->type == AP_RAIN)
				{
					x = map_coord(psPart->position.x);
					y = map_coord(psPart->position.z);
					psTile = mapTile(x,y);
					if (terrainType(psTile) == TER_WATER && TEST_TILE_VISIBLE(selectedPlayer,psTile))
					{
						pos.x = psPart->position.x;
						pos.z = psPart->position.z;
						pos.y = groundHeight;
						effectSetSize(60);
						addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SPECIFIED,true,getImdFromIndex(MI_SPLASH),0);
					}
				}
				return;
			}
		}
		if(psPart->type == AP_SNOW)
		{
			if(rand()%30 == 1)
			{
				psPart->velocity.z = (float)SNOW_SPEED_DRIFT;
			}
			if(rand()%30 == 1)
			{
				psPart->velocity.x = (float)SNOW_SPEED_DRIFT;
			}
		}
	}
}

// -----------------------------------------------------------------------------
/* Adds a particle to the system if it can */
static void atmosAddParticle(const Vector3f &pos, AP_TYPE type)
{
	UDWORD	activeCount;
	UDWORD	i;

	for (i = freeParticle, activeCount = 0; asAtmosParts[i].status == APS_ACTIVE && activeCount < MAX_ATMOS_PARTICLES; i++)
	{
		activeCount++;
		/* Check for wrap around */
		if(i>= (MAX_ATMOS_PARTICLES-1))
		{
			/* Go back to the first one */
			i = 0;
		}
	}

	/* Check the list isn't just full of essential effects */
	if(activeCount>=MAX_ATMOS_PARTICLES-1)
	{
		/* All of the particles active!?!? */
		return;
	}
	else
	{
		freeParticle = i;
	}

	/* Record it's type */
	asAtmosParts[freeParticle].type = (UBYTE)type;

	/* Make it active */
	asAtmosParts[freeParticle].status = APS_ACTIVE;

	/* Setup the imd */
	switch(type)
	{
		case AP_SNOW:
			asAtmosParts[freeParticle].imd = getImdFromIndex(MI_SNOW);
			asAtmosParts[freeParticle].size = 80;
			break;
		case AP_RAIN:
			asAtmosParts[freeParticle].imd = getImdFromIndex(MI_RAIN);
			asAtmosParts[freeParticle].size = 50;
			break;
		default:
			break;
	}

	/* Setup position */
	asAtmosParts[freeParticle].position = pos;

	/* Setup its velocity */
	if(type == AP_RAIN)
	{
		asAtmosParts[freeParticle].velocity = Vector3f(RAIN_SPEED_DRIFT, RAIN_SPEED_FALL, RAIN_SPEED_DRIFT);
	}
	else
	{
		asAtmosParts[freeParticle].velocity = Vector3f(SNOW_SPEED_DRIFT, SNOW_SPEED_FALL, SNOW_SPEED_DRIFT);
	}
}

// -----------------------------------------------------------------------------
/* Move the particles */
void atmosUpdateSystem()
{
	UDWORD	i;
	UDWORD	numberToAdd;
	Vector3f pos;

	// we don't want to do any of this while paused.
	if(!gamePaused() && weather!=WT_NONE)
	{
		for(i = 0; i < MAX_ATMOS_PARTICLES; i++)
		{
			/* See if it's active */
			if(asAtmosParts[i].status == APS_ACTIVE)
			{
				processParticle(&asAtmosParts[i]);
			}
		}

		/* This bit below needs to go into a "precipitation function" */
		numberToAdd = ((weather==WT_SNOWING) ? 2 : 4);

		/* Temporary stuff - just adds a few particles! */
		for(i=0; i<numberToAdd; i++)
		{
			pos.x = player.p.x;
			pos.z = player.p.z;
			pos.x += world_coord(rand()%visibleTiles.x-visibleTiles.x/2);
			pos.z += world_coord(rand()%visibleTiles.x-visibleTiles.y/2);
			pos.y = 1000;

			/* If we've got one on the grid */
			if(pos.x>0 && pos.z>0 &&
			   pos.x<(SDWORD)world_coord(mapWidth-1) &&
			   pos.z<(SDWORD)world_coord(mapHeight-1) )
			{
			   	/* On grid, so which particle shall we add? */
				switch(weather)
				{
				case WT_SNOWING:
					atmosAddParticle(pos, AP_SNOW);
					break;
				case WT_RAINING:
					atmosAddParticle(pos, AP_RAIN);
					break;
				case WT_NONE:
					break;
				default:
					break;
				}
			}
		}
	}
}

// -----------------------------------------------------------------------------
void	atmosDrawParticles( void )
{
UDWORD	i;

	if(weather==WT_NONE)
	{
		return;
	}

	/* Traverse the list */
	for(i=0; i<MAX_ATMOS_PARTICLES; i++)
	{
		/* Don't bother unless it's active */
		if(asAtmosParts[i].status == APS_ACTIVE)
		{
			/* Is it on the grid */
			if (clipXY(asAtmosParts[i].position.x, asAtmosParts[i].position.z))
			{
				renderParticle(&asAtmosParts[i]);
			}
		}
	}
}

// -----------------------------------------------------------------------------
void	renderParticle( ATPART *psPart )
{
	Vector3i dv;

	/* Transform it */
	dv.x = psPart->position.x - player.p.x;
	dv.y = psPart->position.y;
	dv.z = -(psPart->position.z - player.p.z);
	pie_MatBegin();					/* Push the current matrix */
	pie_TRANSLATE(dv.x,dv.y,dv.z);
	/* Make it face camera */
	pie_MatRotY(-player.r.y);
	pie_MatRotY(-player.r.x);
	/* Scale it... */
	pie_MatScale(psPart->size / 100.f);
	/* Draw it... */
	pie_Draw3DShape(psPart->imd, 0, 0, WZCOL_WHITE, 0, 0);
	pie_MatEnd();
}

// -----------------------------------------------------------------------------
void	atmosSetWeatherType(WT_CLASS type)
{
	if(type == WT_NONE)
	{
		atmosInitSystem();
	}
	else
	{
		weather = type;
	}
}

// -----------------------------------------------------------------------------
WT_CLASS	atmosGetWeatherType( void )
{
	return (WT_CLASS)weather;
}
