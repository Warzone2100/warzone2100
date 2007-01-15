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
/* AdvVis.c Alex McLean */
/* Experiment - possibly only for the faster configurations */
/* Makes smooth transitions for terrain visibility */

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"

#include "advvis.h"
#include "display3d.h"
#include "hci.h"
#include "map.h"

/* This uses oodles of memory and so can only be done on the PC */

// ------------------------------------------------------------------------------------
#define FADE_IN_TIME	(GAME_TICKS_PER_SEC/10)
#define	START_DIVIDE	(8)

static UDWORD avConsidered;
static UDWORD avCalculated;
static UDWORD avIgnored;

static BOOL bRevealActive = FALSE;


// ------------------------------------------------------------------------------------
void	avSetStatus(BOOL var)
{
	bRevealActive = var;
}

void	avInformOfChange(SDWORD x, SDWORD y)
{
MAPTILE	*psTile;
SDWORD	lowerX,upperX,lowerY,upperY;

	psTile= mapTile(x,y);
	if(psTile->level == UBYTE_MAX)
	{
		lowerX = player.p.x/TILE_UNITS;
		upperX = lowerX + visibleXTiles;
		lowerY = player.p.z/TILE_UNITS;
		upperY = lowerY + visibleYTiles;
		if(lowerX<0) lowerX = 0;
		if(lowerY<0) lowerY = 0;
		if(x>lowerX AND x<upperX AND y>lowerY AND y<upperY)
		{
			/* tile is on grid - so initiate fade up */
			psTile->level = 0;
		}
		else
		{
			/* tile is off the gird, so force to maximum and finish */
			psTile->level = psTile->illumination;
			psTile->bMaxed = TRUE;
		}
	}
	else
	{
		/* Already know about this one - so exit */
		return;
	}
}


// ------------------------------------------------------------------------------------
static void processAVTile(UDWORD x, UDWORD y)
{
	FRACT time;
	MAPTILE *psTile;
	UDWORD newLevel;

	psTile = mapTile(x, y);
	if (psTile->level == UBYTE_MAX OR psTile->bMaxed)
	{
		return;
	}

	time = (MAKEFRACT(frameTime) / GAME_TICKS_PER_SEC);
	newLevel = MAKEINT(psTile->level + (time * FADE_IN_TIME));
	if (newLevel >= psTile->illumination)
	{
		psTile->level = psTile->illumination;
		psTile->bMaxed = TRUE;
	}
	else
	{
		psTile->level = (UBYTE)newLevel;
	}
}


// ------------------------------------------------------------------------------------
void	avUpdateTiles( void )
{
SDWORD	startX,startY,endX,endY;
UDWORD	i,j;

	/* Clear stats */
	avConsidered = 0;
	avCalculated = 0;
	avIgnored = 0;

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
	if(startX<0) startX = 0;
	if(startY<0) startY = 0;
	if(endX>(SDWORD)(mapWidth-1))  endX = (SDWORD)(mapWidth-1);
	if(endY>(SDWORD)(mapHeight-1)) endY =(SDWORD)(mapHeight-1);

	/* Go through the grid */
	for(i=startY; i<(UDWORD)endY; i++)
	{
		for(j=startX; j<(UDWORD)endX; j++)
		{
			processAVTile(j,i);
		}
	}

	avConsidered = (visibleXTiles * visibleYTiles);
}


// ------------------------------------------------------------------------------------
UDWORD	avGetObjLightLevel(BASE_OBJECT *psObj,UDWORD origLevel)
{
FRACT	div;
UDWORD	lowest,newLevel;


	div = MAKEFRACT(psObj->visible[selectedPlayer])/255;
	lowest = origLevel/START_DIVIDE;
	newLevel = (UDWORD)(div*origLevel);
	if(newLevel<lowest)
	{
		newLevel = lowest;
	}
	return(newLevel);
}

// ------------------------------------------------------------------------------------
void	avGetStats(UDWORD *considered, UDWORD *ignored, UDWORD *calculated)
{
	*considered = avConsidered;
	*ignored	= avIgnored;
	*calculated = avCalculated;
}

// ------------------------------------------------------------------------------------
BOOL	getRevealStatus( void )
{
	return(bRevealActive);
}

// ------------------------------------------------------------------------------------
void	setRevealStatus( BOOL val )
{
	bRevealActive = val;
}
// ------------------------------------------------------------------------------------
void	preProcessVisibility( void )
{
UDWORD		i,j;
MAPTILE		*psTile;
//STRUCTURE	*psStruct;
//FEATURE		*psFeature;

	for(i=0; i<mapWidth;i++)
	{
		for(j=0; j<mapHeight; j++)
		{
			psTile = mapTile(i,j);
		   	if(TEST_TILE_VISIBLE(selectedPlayer,psTile))
		  	{
				psTile->bMaxed = TRUE;
				psTile->level = psTile->illumination;
                //can't have this cos when load up a save game where a structure has been built by the
                //enemy in an area that has been seen before it flags the structure as visible!
				/*if(TILE_HAS_STRUCTURE(psTile))
				{
				 	psStruct = getTileStructure(i,j);
                    if (psStruct)
                    {
					    psStruct->visible[selectedPlayer] = UBYTE_MAX;
                    }
                    else
                    {
                        ASSERT( FALSE, "preProcessVisibility: should be a structure at %d, %d", i, j );
                    }
				}*/
				/*
				if(TILE_HAS_FEATURE(psTile))
				{
					psFeature = getTileFeature(i,j);
					psFeature->visible[selectedPlayer] = UBYTE_MAX;
				}
				*/
		  	}
			else
			{
			 	psTile->level = UBYTE_MAX;
				psTile->bMaxed = FALSE;
			}
		}
	}


}
// ------------------------------------------------------------------------------------
