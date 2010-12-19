/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
 * @file advvis.c
 * 
 * Makes smooth transitions for terrain visibility.
 */

#include "lib/framework/frame.h"

#include "advvis.h"
#include "map.h"

// ------------------------------------------------------------------------------------
#define FADE_IN_TIME	(GAME_TICKS_PER_SEC/10)
#define	START_DIVIDE	(8)

static BOOL bRevealActive = false;


// ------------------------------------------------------------------------------------
void	avUpdateTiles( void )
{
	const int len = mapHeight * mapWidth;
	const int playermask = 1 << selectedPlayer;
	UDWORD i = 0;
	float maxLevel, increment = graphicsTimeAdjustedIncrement(FADE_IN_TIME);	// call once per frame
	MAPTILE *psTile;

	/* Go through the tiles */
	for (psTile = psMapTiles; i < len; i++)
	{
		maxLevel = psTile->illumination;

		if (psTile->level > 0 || psTile->tileExploredBits & playermask)	// seen
		{
			// If we are not omniscient, and we are not seeing the tile, and none of our allies see the tile...
			if (!godMode && !(alliancebits[selectedPlayer] & (satuplinkbits | psTile->sensorBits)))
			{
				maxLevel /= 2;
			}
			if (psTile->level > maxLevel)
			{
				psTile->level = MAX(psTile->level - increment, 0);
			}
			else if (psTile->level < maxLevel)
			{
				psTile->level = MIN(psTile->level + increment, maxLevel);
			}
		}
		psTile++;
	}
}


// ------------------------------------------------------------------------------------
UDWORD	avGetObjLightLevel(BASE_OBJECT *psObj,UDWORD origLevel)
{
	float div = (float)psObj->visible[selectedPlayer] / 255.f;
	unsigned int lowest = origLevel / START_DIVIDE;
	unsigned int newLevel = div * origLevel;

	if(newLevel < lowest)
	{
		newLevel = lowest;
	}

	return newLevel;
}

// ------------------------------------------------------------------------------------
BOOL	getRevealStatus( void )
{
	return(bRevealActive);
}

// ------------------------------------------------------------------------------------
void	setRevealStatus( BOOL val )
{
	debug(LOG_FOG, "avSetRevealStatus: Setting reveal to %s", val ? "ON" : "OFF");
	bRevealActive = val;
}

// ------------------------------------------------------------------------------------
void	preProcessVisibility( void )
{
UDWORD		i,j;
MAPTILE		*psTile;

	for(i=0; i<mapWidth;i++)
	{
		for(j=0; j<mapHeight; j++)
		{
			psTile = mapTile(i,j);
			psTile->level = 0;

			if (!bRevealActive || TEST_TILE_VISIBLE(selectedPlayer, psTile))
			{
				psTile->level = psTile->illumination;
			}
		}
	}
}
// ------------------------------------------------------------------------------------
