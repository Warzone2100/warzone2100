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

#include "lib/ivis_common/ivisdef.h" //ivis matrix code
#include "lib/ivis_opengl/piematrix.h"

#include "objectdef.h"
#include "map.h"
#include "display3d.h"
#include "geometry.h"
#include "lib/gamelib/gtime.h"
#include "hci.h"
#include "display.h"

#define AMPLITUDE_HEIGHT        100
#define SIZE_SINE_TABLE         100
#define deg (M_PI / SIZE_SINE_TABLE)

/* The arc over which bullets fly */
static UBYTE sineHeightTable[SIZE_SINE_TABLE];

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

/* Angle returned is reflected in line x=0 */
SDWORD calcDirection(UDWORD x0, UDWORD y0, UDWORD x1, UDWORD y1)
{
	/* Watch out here - should really be y1-y0, but coordinate system is reversed in Y */
	SDWORD	xDif = (x1-x0), yDif = (y0-y1);
	double	angle = atan2(yDif, xDif) * 180.0 / M_PI;
	SDWORD	angleInt = (SDWORD) angle;

	angleInt+=90;
	if (angleInt<0)
		angleInt+=360;

	ASSERT( angleInt >= 0 && angleInt < 360,
		"calcDirection: droid direction out of range" );

	return(angleInt);
}


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
			    dist = MAX(xDif,yDif) + MIN(xDif,yDif)/2;	// approximates, but never more than 11% out...
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
		if ((((quad->coords[i].y<=pt->y) && (pt->y<quad->coords[j].y)) ||
             ((quad->coords[j].y<=pt->y) && (pt->y<quad->coords[i].y))) &&
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
	if(sum>=0 && sum<=360)
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
	for(i=0; i<MAX_PLAYERS && !psReturn; i++)
	{
		/* Got thru' all structures for this player - again drop out if match */
		for (psStructure = apsStructLists[i];
			psStructure && !psReturn; psStructure = psStructure->psNext)
		{
			/* Get structure coords */
			strX = psStructure->x;
			strY = psStructure->y;
			/* And extents */
			width = psStructure->pStructureType->baseWidth*TILE_UNITS;
			breadth = psStructure->pStructureType->baseBreadth*TILE_UNITS;
			/* Within x boundary? */


			if((centreX > (strX-(width/2))) && (centreX < (strX+(width/2))) )
			{
				if((centreY > (strY-(breadth/2))) && (centreY < (strY+(breadth/2))) )
				{
					psReturn = psStructure;
				}
			}

/*			if((centreX > (strX-width)) && (centreX < (strX+width)) )
			{
				if((centreY > (strY-breadth)) && (centreY < (strY+breadth)) )
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
		psFeature && !psReturn; psFeature = psFeature->psNext)
		{
			/* Get the features coords */
			strX = psFeature->x;
			strY = psFeature->y;
			/* And it's base dimensions */
			width = psFeature->psStats->baseWidth*TILE_UNITS;
			breadth = psFeature->psStats->baseBreadth*TILE_UNITS;
			/* Does tile centre lie within the area covered by base of feature? */
			/* First check for x */
			if((centreX > (strX-(width/2))) && (centreX < (strX+(width/2))) )
			{
				/* Got a match on the x - now try y */
				if((centreY > (strY-(breadth/2))) && (centreY < (strY+(breadth/2))) )
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

// Approximates a square root - never more than 11% out...
UDWORD	dirtySqrt( SDWORD x1, SDWORD y1, SDWORD x2, SDWORD y2)
{
UDWORD	xDif,yDif;
UDWORD	retVal;

	xDif = abs(x1-x2);
	yDif = abs(y1-y2);

	retVal = (MAX(xDif,yDif) + (MIN(xDif,yDif)/2));
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
