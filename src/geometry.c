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
/* Geometry.c - holds trig/vector deliverance specific stuff for 3D */
/* Alex McLean, Pumpkin Studios, EIDOS Interactive */

#include "lib/framework/frame.h"
#include <stdio.h>
#include <stdlib.h>


#include <math.h>


#include "lib/ivis_common/ivisdef.h" //ivis matrix code
#include "lib/ivis_common/geo.h" //ivis matrix code

#include "objectdef.h"
#include "map.h"
#include "display3d.h"
#include "geometry.h"
#include "lib/gamelib/gtime.h"
#include "hci.h"
#include "display.h"




void testAngles(void);
void	processImpact(UDWORD worldX, UDWORD worldY, UBYTE severity,UDWORD tilesAcross);
//UDWORD	getTileOwner(UDWORD	x, UDWORD y);
//BASE_OBJECT	*getTileOccupier(UDWORD x, UDWORD y);
//STRUCTURE	*getTileStructure(UDWORD x, UDWORD y);
//FEATURE		*getTileFeature(UDWORD x, UDWORD y);
void	baseObjScreenCoords	( BASE_OBJECT *baseObj, iPoint *pt				);
SDWORD	calcDirection		( UDWORD x0, UDWORD y0, UDWORD x1, UDWORD y1	);
UDWORD	adjustDirection		( SDWORD present, SDWORD difference				);

void initBulletTable( void );
int inQuad(POINT *pt, QUAD *quad);

/* The arc over which bullets fly */
UBYTE	sineHeightTable[SIZE_SINE_TABLE];

//BOOL	bScreenShakeActive = FALSE;
//UDWORD	screenShakeStarted = 0;
//UDWORD	screenShakeLength = 0;




void initBulletTable( void )
{
UDWORD	 i;
UBYTE	height;
	for (i=0; i<SIZE_SINE_TABLE; i++)
	{
	height = (UBYTE) (AMPLITUDE_HEIGHT*sin(i*deg));
	sineHeightTable[i] = height;
	}
}

//void	attemptScreenShake(void)
//{
//	if(!bScreenShakeActive)
//	{
//		bScreenShakeActive = TRUE;
//		screenShakeStarted = gameTime;
//		screenShakeLength = 1500;
//	}
//}

/* Angle returned is reflected in line x=0 */
SDWORD	calcDirection(UDWORD x0, UDWORD y0, UDWORD x1, UDWORD y1)
{
SDWORD	xDif,yDif;
SDWORD	angleInt;
double	angle;

	angleInt = 0;
	xDif = (x1-x0);

	/* Watch out here - should really be y1-y0, but coordinate system is reversed in Y */
	yDif = (y0-y1);
	angle = atan2(yDif,xDif);
	angle = (double) (180*(angle/pi));
	angleInt = (SDWORD) angle;

	angleInt+=90;
	if (angleInt<0)
		angleInt+=360;

	ASSERT( angleInt >= 0 && angleInt < 360,
		"calcDirection: droid direction out of range" );

	return(angleInt);
}




#ifndef WIN32
#define max(a,b) (((a)>(b))?(a):(b))
#define min(a,b) (((a)<(b))?(a):(b))
#endif
// -------------------------------------------------------------------------------------------
/*	A useful function and one that should have been written long ago, assuming of course
	that is hasn't been!!!! Alex M, 24th Sept, 1998. Returns the nearest unit
	to a given world coordinate - we can choose whether we require that the unit be
	selected or not... Makes sending the most logical unit to do something very easy.

  NB*****THIS WON'T PICK A VTOL DROID*****
*/

DROID	*getNearestDroid(UDWORD x, UDWORD y, BOOL bSelected)
{
DROID	*psDroid,*psBestUnit;
UDWORD	xDif,yDif,dist;
UDWORD	bestSoFar;

	/* Go thru' all the droids  - how often have we seen this - a MACRO maybe? */
	for(psDroid = apsDroidLists[selectedPlayer],psBestUnit = NULL, bestSoFar = UDWORD_MAX;
		psDroid; psDroid = psDroid->psNext)
	{
        if (!vtolDroid(psDroid))
        {
		    /* Clever (?) bit that reads whether we're interested in droids being selected or not */
		    if( (bSelected ? psDroid->selected : TRUE ) )
		    {
			    /* Get the differences */
			    xDif = abs(psDroid->x - x);
			    yDif = abs(psDroid->y - y);
			    /* Approximates the distance away - using a sqrt approximation */
			    dist = max(xDif,yDif) + (min(xDif,yDif))/2;	// approximates, but never more than 11% out...
			    /* Is this the nearest one we got so far? */
			    if(dist<bestSoFar)
			    {
				    /* Yes, then keep a record of the distance for comparison... */
				    bestSoFar = dist;
				    /* ..and store away the droid responsible */
				    psBestUnit = psDroid;
			    }
            }
		}
	}
	return(psBestUnit);
}
// -------------------------------------------------------------------------------------------

/* Returns non-zero if a point is in a 4 sided polygon */
/* See header file for definition of QUAD */
int inQuad(POINT *pt, QUAD *quad)
{
int i, j, c = 0;

	for (i = 0, j = 3; i < 4; j = i++)
	{
		if ((((quad->coords[i].y<=pt->y) AND (pt->y<quad->coords[j].y)) OR
             ((quad->coords[j].y<=pt->y) AND (pt->y<quad->coords[i].y))) AND
			(pt->x < (quad->coords[j].x - quad->coords[i].x) *
			(pt->y - quad->coords[i].y) / (quad->coords[j].y -
			quad->coords[i].y) + quad->coords[i].x))

          c = !c;
    }

    return c;
}

UDWORD	adjustDirection(SDWORD present, SDWORD difference)
{
SDWORD	sum;

	sum = present+difference;
	if(sum>=0 AND sum<=360)
	{
		return(UDWORD)(sum);
	}

	if (sum<0)
	{
		return(UDWORD)(360+sum);
	}

	if (sum>360)
	{
		return(UDWORD)(sum-360);
	}
	return 0;
}

/* Return a signed difference in direction : a - b
 * result is 180 .. -180
 */
SDWORD directionDiff(SDWORD a, SDWORD b)
{
	SDWORD	diff = a - b;

	if (diff > 180)
	{
		return diff - 360;
	}
	else if (diff < -180)
	{
		return 360 + diff;
	}

	return diff;
}



void WorldPointToScreen( iPoint *worldPt, iPoint *screenPt )
{
iVector vec,null;
//MAPTILE	*psTile;
UDWORD	worldX,worldY;
SDWORD	xShift,zShift;
int32	rx,rz;
	/* Get into game context */
	/* Get the x,z translation components */
	rx = player.p.x & (TILE_UNITS-1);
 	rz = player.p.z & (TILE_UNITS-1);


	/* Push identity matrix onto stack */
	iV_MatrixBegin();

	/* Set the camera position */
	pie_MATTRANS(camera.p.x,camera.p.y,camera.p.z);

	/* Rotate for the player */
	iV_MatrixRotateZ(player.r.z);
	iV_MatrixRotateX(player.r.x);
	iV_MatrixRotateY(player.r.y);

	/* Translate */
	iV_TRANSLATE(-rx,-player.p.y,rz);




	/* No rotation is necessary*/
	null.x = 0; null.y = 0; null.z = 0;

	/* Pull out coords now, because we use them twice */
	worldX = worldPt->x;
	worldY = worldPt->y;

	/* Get the coordinates of the object into the grid */
	vec.x = ( worldX - player.p.x) - terrainMidX*TILE_UNITS;
	vec.z = terrainMidY*TILE_UNITS - (worldY - player.p.z);

	/* Which tile is it on? - In order to establish height (y coordinate in 3 space) */
//	psTile = mapTile(worldX/TILE_UNITS,worldY/TILE_UNITS);
//	vec.y = psTile->height;
	vec.y = map_Height(worldX/TILE_UNITS,worldY/TILE_UNITS);

	/* Set matrix context to local - get an identity matrix */
	iV_MatrixBegin();

	/* Translate */
	iV_TRANSLATE(vec.x,vec.y,vec.z);
 	xShift = player.p.x & (TILE_UNITS-1);
	zShift = player.p.z & (TILE_UNITS-1);

	/* Translate */
	iV_TRANSLATE(xShift,0,-zShift);

	/* Project - no rotation being done. So effectively mapping from 3 space to 2 space */
	pie_RotProj(&null,screenPt);

	/* Pop remaining matrices */
	pie_MatEnd();
	pie_MatEnd();
}



/*	Calculates the RELATIVE screen coords of a game object from its BASE_OBJECT pointer */
/*	Alex - Saturday 5th July, 1997  */
/*	Returns result in POINT pt. They're relative in the sense that even if you pass
	a pointer to an object that isn't on screen, it'll still return a result - just that
	the coords may be negative or larger than screen dimensions in either (or both) axis (axes).
	Remember also, that the Y coordinate axis is reversed for our display in that increasing Y
	implies a movement DOWN the screen, and NOT up. */

void
baseObjScreenCoords(BASE_OBJECT *baseObj, iPoint *pt)
{
	iPoint	worldPt;

	worldPt.x = baseObj->x;
	worldPt.y = baseObj->y;

	WorldPointToScreen( &worldPt, pt );
}

/* Get the structure pointer for a specified tile coord. NULL if no structure */
STRUCTURE	*getTileStructure(UDWORD x, UDWORD y)
{
STRUCTURE	*psStructure;
STRUCTURE	*psReturn;
UDWORD		centreX, centreY;
UDWORD		strX,strY;
UDWORD		width,breadth;
UDWORD		i;

	/* No point in checking if there's no structure here! */
	if(!TILE_HAS_STRUCTURE(mapTile(x,y)))
	{
		return(NULL);
	}

	/* Otherwise - see which one it is! */
	psReturn = NULL;
	/* Get the world coords for the tile centre */
	centreX = (x<<TILE_SHIFT)+(TILE_UNITS/2);
	centreY = (y<<TILE_SHIFT)+(TILE_UNITS/2);

	/* Go thru' all players - drop out if match though */
	for(i=0; i<MAX_PLAYERS AND !psReturn; i++)
	{
		/* Got thru' all structures for this player - again drop out if match */
		for (psStructure = apsStructLists[i];
			psStructure AND !psReturn; psStructure = psStructure->psNext)
		{
			/* Get structure coords */
			strX = psStructure->x;
			strY = psStructure->y;
			/* And extents */
			width = psStructure->pStructureType->baseWidth*TILE_UNITS;
			breadth = psStructure->pStructureType->baseBreadth*TILE_UNITS;
			/* Within x boundary? */


			if((centreX > (strX-(width/2))) AND (centreX < (strX+(width/2))) )
			{
				if((centreY > (strY-(breadth/2))) AND (centreY < (strY+(breadth/2))) )
				{
					psReturn = psStructure;
				}
			}

/*			if((centreX > (strX-width)) AND (centreX < (strX+width)) )
			{
				if((centreY > (strY-breadth)) AND (centreY < (strY+breadth)) )
				{
					psReturn = psStructure;
				}
			}
*/
		}
	}
	/* Send back either NULL or structure */
	return(psReturn);
}

/* Sends back the feature on the specified tile - NULL if no feature */
FEATURE	*getTileFeature(UDWORD x, UDWORD y)
{
FEATURE		*psFeature;
FEATURE		*psReturn;
UDWORD		centreX, centreY;
UDWORD		strX,strY;
UDWORD		width,breadth;
//UDWORD		i;

	/* No point in checking if there's no feature here! */
	if(!TILE_HAS_FEATURE(mapTile(x,y)))
	{
		return(NULL);
	}

	/* Otherwise - see which one it is! */
	psReturn = NULL;
	/* Get the world coords for the tile centre */
	centreX = (x<<TILE_SHIFT)+(TILE_UNITS/2);
	centreY = (y<<TILE_SHIFT)+(TILE_UNITS/2);

	/* Go through all features for this player - again drop out if we get one */
	for (psFeature = apsFeatureLists[0];
		psFeature AND !psReturn; psFeature = psFeature->psNext)
		{
			/* Get the features coords */
			strX = psFeature->x;
			strY = psFeature->y;
			/* And it's base dimensions */
			width = psFeature->psStats->baseWidth*TILE_UNITS;
			breadth = psFeature->psStats->baseBreadth*TILE_UNITS;
			/* Does tile centre lie within the area covered by base of feature? */
			/* First check for x */
			if((centreX > (strX-(width/2))) AND (centreX < (strX+(width/2))) )
			{
				/* Got a match on the x - now try y */
				if((centreY > (strY-(breadth/2))) AND (centreY < (strY+(breadth/2))) )
				{
					/* Got it! */
					psReturn = psFeature;
				}
			}
		}

	/* Send back either NULL or feature pointer */
	return(psReturn);
}

/*	Will return a base_object pointer to either a structure or feature - depending
	what's on tile. Returns NULL if nothing */
BASE_OBJECT	*getTileOccupier(UDWORD x, UDWORD y)
{

//DBPRINTF(("gto x=%d y=%d (%d,%d)\n",x,y,x*TILE_UNITS,y*TILE_UNITS);
	/* Firsty - check there is something on it?! */
	if(!TILE_OCCUPIED(mapTile(x,y)))
	{
//DBPRINTF(("gto nothing\n");
		/* Nothing here at all! */
		return(NULL);
	}

	/* Now check we can see it... */
	if(TEST_TILE_VISIBLE(selectedPlayer,mapTile(x,y)) == FALSE)
	{
		return(NULL);
	}

	/* Has it got a fetaure? */
	if(TILE_HAS_FEATURE(mapTile(x,y)))
	{
//DBPRINTF(("gto feature\n");
		/* Return the feature */
		return( (BASE_OBJECT *) getTileFeature(x,y) );
	}
	/*	Otherwise check for a structure - we can do else here since a tile cannot
		have both a feature and structure simultaneously */
	else if (TILE_HAS_STRUCTURE(mapTile(x,y)))
	{
//DBPRINTF(("gto structure\n");
		/* Send back structure pointer */
		return( (BASE_OBJECT *) getTileStructure(x,y) );
	}
	return NULL;
}

/* Will return the player who presently has a structure on the specified tile */
UDWORD	getTileOwner(UDWORD	x, UDWORD y)
{
STRUCTURE	*psStruct;
UDWORD		retVal;

	/* Arbitrary error code - player 8 (non existent) owns tile from invalid request */
	retVal = MAX_PLAYERS;

	/* Check it has a structure - cannot have owner otherwise */
	if(!TILE_HAS_STRUCTURE(mapTile(x,y)))
	{
		debug( LOG_ERROR, "Asking for the owner of a tile with no structure on it!!!" );
		abort();
	}
	else
	{
		/* Get a pointer to the structure */
		psStruct = getTileStructure(x,y);

		/* Did we get one - failsafe really as TILE_HAS_STRUCTURE should get it */
		if(psStruct!=NULL)
		{
			/* Pull out the player number */
			retVal = psStruct->player;
		}
	}
	/* returns eith the player number or MAX_PLAYERS to signify error */
	return(retVal);
}

static void getObjectsOnTile(MAPTILE *psTile)
{
/*UDWORD	i;
FEATURE	*psFeature;
DROID	*psDroid;
STRUCTURE	*psStructure;*/

	(void)psTile;

}

// Approximates a square root - never more than 11% out...
UDWORD	dirtySqrt( SDWORD x1, SDWORD y1, SDWORD x2, SDWORD y2)
{
UDWORD	xDif,yDif;
UDWORD	retVal;

	xDif = abs(x1-x2);
	yDif = abs(y1-y2);

	retVal = (max(xDif,yDif) + (min(xDif,yDif)/2));
	return(retVal);
}
//-----------------------------------------------------------------------------------
BOOL	droidOnScreen( DROID *psDroid, SDWORD tolerance )
{
SDWORD	dX,dY;

	if (DrawnInLastFrame(psDroid->sDisplay.frameNumber)==TRUE)
		{
			dX = psDroid->sDisplay.screenX;
			dY = psDroid->sDisplay.screenY;
			/* Is it on screen */
			if(dX > (0-tolerance) && dY > (0-tolerance)
				&& dX < (SDWORD)(pie_GetVideoBufferWidth()+tolerance)
				&& dY < (SDWORD)(pie_GetVideoBufferHeight()+tolerance))
			{
				return(TRUE);
			}
		}
	return(FALSE);
}


void	processImpact(UDWORD worldX, UDWORD worldY, UBYTE severity, UDWORD tilesAcross)
{
//MAPTILE	*psTile;
UDWORD	height;
SDWORD	newHeight;
UDWORD	distance;
float	multiplier;
UDWORD	damage;
UDWORD	i,j;
UDWORD	xDif,yDif;
SDWORD	tileX,tileY;
UDWORD	maxDisplacement;
UDWORD	maxDistance;

	ASSERT( severity<MAX_TILE_DAMAGE,"Damage is too severe" );
	/* Make sure it's odd */
	if( !(tilesAcross & 0x01))
	{
		tilesAcross-=1;
	}
	tileX = ((worldX>>TILE_SHIFT)-(tilesAcross/2-1));
	tileY = ((worldY>>TILE_SHIFT)-(tilesAcross/2-1));
	maxDisplacement = ((tilesAcross/2+1) * TILE_UNITS);
	maxDisplacement = (UDWORD)((float)maxDisplacement * (float)1.42);
	maxDistance = (UDWORD)sqrt(((float)maxDisplacement * (float)maxDisplacement));

	if(tileX < 0) tileX = 0;
	if(tileY < 0) tileY = 0;

	for(i=tileX; i<tileX+tilesAcross-1; i++)
	{
		for(j=tileY; j<tileY+tilesAcross-1; j++)
		{
			/* Only process tiles that are on the map */
			if(tileX < (SDWORD)mapWidth AND tileY<(SDWORD)mapHeight)
			{
				xDif = abs(worldX - (i<<TILE_SHIFT));
				yDif = abs(worldY - (j<<TILE_SHIFT));
				distance = (UDWORD)sqrt(( (float)(xDif*xDif) + (float)(yDif*yDif) ));
				multiplier = (1-((float) ( (float)distance / (float) maxDistance)));
				multiplier = (float) (1.0 - ((float)distance/(float)maxDistance));
				/* Are we talking less than 15% damage? i.e - at the edge of carater? */
				if(multiplier<0.15)
				{
					/* In which case make the crater edge have jagged edges */
					multiplier+= (float)((float)(20-rand()%40) * 0.01);
				}

				height = mapTile(i,j)->height;
				damage = (UDWORD) ((float)severity*multiplier);
				newHeight = height-damage;
				if(newHeight < 0)
				{
					newHeight = 0;
				}
				setTileHeight(i,j,newHeight);
			}
		}
	}
}



