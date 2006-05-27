/* Atmos.c - Handles atmospherics such as snow and rain */
/* Alex McLean, Pumpkin Studios, EIDOS Interactive */
/*
	At present, the water effects are part of the atmos
	system and aren't properly implemented in the software mode
*/
#include "lib/framework/frame.h"
#include "lib/ivis_common/piedef.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_common/piestate.h"
#include "display3d.h"
#include "display3ddef.h"
#include "lib/gamelib/gtime.h"
#include "miscimd.h"
#include "map.h"
#include "atmos.h"
#include "loop.h"
#include "lib/ivis_common/geo.h"
#include "effects.h"
#include "lighting.h"
#include "bucket3d.h"
#include "hci.h"

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// Shift all this gubbins into a .h file if it makes it into game
// -----------------------------------------------------------------------------
/* Roughly one per tile */
#define	MAX_ATMOS_PARTICLES		((VISIBLE_XTILES * VISIBLE_YTILES))
#define	SNOW_SPEED_DRIFT		(40 - rand()%80)
#define SNOW_SPEED_FALL			(0-(rand()%40 + 80))
#define	RAIN_SPEED_DRIFT		(rand()%50)
#define	RAIN_SPEED_FALL			(0-((rand()%300) + 700))

#define	TYPE_WATER				75
#define TYPE_LAND				76

typedef enum
{
AP_RAIN,
AP_SNOW
} AP_TYPE;

typedef enum
{
APS_ACTIVE,
APS_INACTIVE,
} AP_STATUS;

ATPART	asAtmosParts[MAX_ATMOS_PARTICLES];
static	FRACT	fraction;
static	UDWORD	freeParticle;
static	UDWORD	weather;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void	atmosInitSystem			( void );
void	atmosUpdateSystem		( void );
void	atmosDrawParticles		( void );
void	processParticle			( ATPART *psPart );
void	atmosAddParticle		( iVector *pos, AP_TYPE type );
void	renderParticle			( ATPART *psPart );
void	testParticleWrap		( ATPART *psPart );
void	atmosSetWeatherType		( WT_CLASS type );
WT_CLASS	atmosGetWeatherType	( void );
// -----------------------------------------------------------------------------
/* Setup all the particles */
void	atmosInitSystem( void )
{
UDWORD	i;

	for(i=0; i<MAX_ATMOS_PARTICLES; i++)
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
/* Move the particles */
void	atmosUpdateSystem( void )
{
UDWORD	i;
UDWORD	numberToAdd;
iVector	pos;

	/* Establish how long the last game frame took */
	fraction = MAKEFRACT(frameTime)/GAME_TICKS_PER_SEC;

 //	if(weather==WT_NONE)
 //	{
 //		return;
 //	}

	for(i=0; i<MAX_ATMOS_PARTICLES; i++)
	{
		/* See if it's active */
		if(asAtmosParts[i].status == APS_ACTIVE)
		{
			processParticle(&asAtmosParts[i]);
		}
	}

	/* This bit below needs to go into a "precipitation function" */
	if(!gamePaused() AND weather!=WT_NONE)
	{
		numberToAdd = ((weather==WT_SNOWING) ? 2 : 4);
		/* Temporary stuff - just adds a few particles! */
		for(i=0; i<numberToAdd; i++)
		{

			pos.x = player.p.x + ((visibleXTiles/2)*TILE_UNITS);
			pos.z = player.p.z + ((visibleYTiles/2)*TILE_UNITS);
			pos.x += (((visibleXTiles/2) - rand()%visibleXTiles) * TILE_UNITS);
			pos.z += (((visibleXTiles/2) - rand()%visibleXTiles) * TILE_UNITS);
			pos.y = 1000;

			/* If we've got one on the grid */
			if(pos.x>0 AND pos.z>0 AND
			   pos.x<(SDWORD)((mapWidth-1)*TILE_UNITS) AND
			   pos.z<(SDWORD)((mapHeight-1)*TILE_UNITS) )
			{
			   	/* On grid, so which particle shall we add? */
				switch(weather)
				{
				case WT_SNOWING:
					atmosAddParticle(&pos,AP_SNOW);
					break;
				case WT_RAINING:
					atmosAddParticle(&pos,AP_RAIN);
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
/* Moves one of the particles */
void	processParticle( ATPART *psPart )
{
SDWORD	groundHeight;
iVector	pos;
UDWORD	x,y;
MAPTILE	*psTile;


	/* Only move if the game isn't paused */
	if(!gamePaused())
	{
		/* Move the particle - frame rate controlled */
 		psPart->position.x += (psPart->velocity.x * fraction);
		psPart->position.y += (psPart->velocity.y * fraction);
		psPart->position.z += (psPart->velocity.z * fraction);

		/* Wrap it around if it's gone off grid... */
	   	testParticleWrap(psPart);

		/* If it's gone off the WORLD... */
		if(psPart->position.x<0 OR psPart->position.z<0 OR
		   psPart->position.x>((mapWidth-1)*TILE_UNITS) OR
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
			groundHeight = map_Height((UDWORD)MAKEINT(psPart->position.x),(UDWORD)MAKEINT(psPart->position.z));

			/* Are we below ground? */
			if( (MAKEINT(psPart->position.y) < groundHeight) OR (psPart->position.y<0.0f) )
			{
				/* Kill it and return */
				psPart->status = APS_INACTIVE;
				if(psPart->type == AP_RAIN)
				{
					x = (MAKEINT(psPart->position.x))>>TILE_SHIFT;
					y = (MAKEINT(psPart->position.z))>>TILE_SHIFT;
					psTile = mapTile(x,y);
					if(TERRAIN_TYPE(psTile) == TER_WATER AND TEST_TILE_VISIBLE(selectedPlayer,psTile))
					{
						pos.x = MAKEINT(psPart->position.x);
						pos.z = MAKEINT(psPart->position.z);
						pos.y = groundHeight;
						effectSetSize(60);
						addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SPECIFIED,TRUE,getImdFromIndex(MI_SPLASH),0);
					}
				}
				return;
			}
		}
		if(psPart->type == AP_SNOW)
		{
			if(rand()%30 == 1)
			{
				psPart->velocity.z = MAKEFRACT(SNOW_SPEED_DRIFT);
			}
			if(rand()%30 == 1)
			{
				psPart->velocity.x = MAKEFRACT(SNOW_SPEED_DRIFT);
			}
		}
	}
}

// -----------------------------------------------------------------------------
/* Adds a particle to the system if it can */
void	atmosAddParticle( iVector *pos, AP_TYPE type )
{
UDWORD	activeCount;
UDWORD	i;

	for(i=freeParticle,activeCount=0; (asAtmosParts[i].status==APS_ACTIVE)
		AND activeCount<MAX_ATMOS_PARTICLES; i++)
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
	asAtmosParts[freeParticle].position.x = MAKEFRACT(pos->x);
	asAtmosParts[freeParticle].position.y = MAKEFRACT(pos->y);
	asAtmosParts[freeParticle].position.z = MAKEFRACT(pos->z);

	/* Setup its velocity */
	if(type == AP_RAIN)
	{
		asAtmosParts[freeParticle].velocity.x = MAKEFRACT(RAIN_SPEED_DRIFT);
		asAtmosParts[freeParticle].velocity.y = MAKEFRACT(RAIN_SPEED_FALL);
		asAtmosParts[freeParticle].velocity.z = MAKEFRACT(RAIN_SPEED_DRIFT);
	}
	else
	{
		asAtmosParts[freeParticle].velocity.x = MAKEFRACT(SNOW_SPEED_DRIFT);
		asAtmosParts[freeParticle].velocity.y = MAKEFRACT(SNOW_SPEED_FALL);
		asAtmosParts[freeParticle].velocity.z = MAKEFRACT(SNOW_SPEED_DRIFT);
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
			if(clipXY((UDWORD)MAKEINT(asAtmosParts[i].position.x),(UDWORD)MAKEINT(asAtmosParts[i].position.z)))
			{
#ifndef BUCKET
				/* Draw it right now */
				renderParticle(&asAtmosParts[i]);
#else
				/* Add it to the bucket */
				bucketAddTypeToList(RENDER_PARTICLE,&asAtmosParts[i]);
#endif
			}
		}
	}
}

// -----------------------------------------------------------------------------
void	renderParticle( ATPART *psPart )
{
	iVector	dv;
	UDWORD brightness, specular;
	SDWORD centreX, centreZ;
	SDWORD x, y, z;

	x = MAKEINT(psPart->position.x);
	y = MAKEINT(psPart->position.y);
	z = MAKEINT(psPart->position.z);
	/* Transform it */
	dv.x = ((UDWORD)x - player.p.x) - terrainMidX * TILE_UNITS;
	dv.y = (UDWORD)y;
	dv.z = terrainMidY * TILE_UNITS - ((UDWORD)z - player.p.z);
	iV_MatrixBegin();							/* Push the indentity matrix */
	iV_TRANSLATE(dv.x,dv.y,dv.z);
	rx = player.p.x & (TILE_UNITS-1);			/* Get the x,z translation components */
	rz = player.p.z & (TILE_UNITS-1);
	iV_TRANSLATE(rx,0,-rz);						/* Translate */
	/* Make it face camera */
	iV_MatrixRotateY(-player.r.y);
	iV_MatrixRotateX(-player.r.x);
	/* Scale it... */
	pie_MatScale(psPart->size);
	/* Draw it... */
	centreX = ( player.p.x + ((visibleXTiles/2)<<TILE_SHIFT) );
	centreZ = ( player.p.z + ((visibleYTiles/2)<<TILE_SHIFT) );
	brightness = lightDoFogAndIllumination(pie_MAX_BRIGHT_LEVEL,centreX - x,centreZ - z, &specular);
   	pie_Draw3DShape(psPart->imd, 0, 0, brightness, 0, pie_NO_BILINEAR, 0);
	iV_MatrixEnd();
}
// -----------------------------------------------------------------------------
/*	Makes a particle wrap around - if it goes off the grid, then it returns
	on the other side - provided it's still on world... Which it should be */
void	testParticleWrap( ATPART *psPart )
{
	/* Gone off left side */
	if(psPart->position.x < player.p.x)
	{
		psPart->position.x += (visibleXTiles*TILE_UNITS);
	}

	/* Gone off right side */
	else if(psPart->position.x > (player.p.x + (visibleXTiles*TILE_UNITS)))
	{
		psPart->position.x -= (visibleXTiles*TILE_UNITS);
	}

	/* Gone off top */
	if(psPart->position.z < player.p.z)
	{
		psPart->position.z += (visibleYTiles*TILE_UNITS);
	}

	/* Gone off bottom */
	else if(psPart->position.z > (player.p.z + (visibleYTiles*TILE_UNITS)))
	{
		psPart->position.z -= (visibleYTiles*TILE_UNITS);
	}
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
