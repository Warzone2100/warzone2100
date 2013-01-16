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
	/*
	Edit3D.c - to ultimately contain the map editing functions -
	they are presently scattered in various files .
	Alex McLean, Pumpkin Studios, EIDOS Interactive, 1997
	*/

#include "lib/framework/frame.h"
#include "map.h"
#include "edit3d.h"
#include "display3d.h"
#include "objects.h"
#include "display.h"
#include "hci.h"

/*
Definition of a tile to highlight - presently more than is required
but means that we can highlight any individual tile in future. An
x coordinate that is greater than mapWidth implies that the highlight
is invalid (not currently being used)
*/

UDWORD	buildState = BUILD3D_NONE;
BUILDDETAILS	sBuildDetails;
HIGHLIGHT		buildSite;
int brushSize = 1;
bool quickQueueMode = false;

// Initialisation function for statis & globals in this module.
//
void Edit3DInitVars(void)
{
	buildState = BUILD3D_NONE;
	brushSize = 1;
}

/* Raises a tile by a #defined height */
void raiseTile(int tile3dX, int tile3dY)
{
	int i, j;

	if (tile3dX < 0 || tile3dX > mapWidth - 1 || tile3dY < 0 || tile3dY > mapHeight - 1)
	{
		return;
	}
	for (i = tile3dX; i <= MIN(mapWidth - 1, tile3dX + brushSize); i++)
	{
		for (j = tile3dY; j <= MIN(mapHeight - 1, tile3dY + brushSize); j++)
		{
			adjustTileHeight(mapTile(i, j), TILE_RAISE);
			markTileDirty(i,j);
		}
	}
}

/* Lowers a tile by a #defined height */
void lowerTile(int tile3dX, int tile3dY)
{
	int i, j;

	if (tile3dX < 0 || tile3dX > mapWidth - 1 || tile3dY < 0 || tile3dY > mapHeight - 1)
	{
		return;
	}
	for (i = tile3dX; i <= MIN(mapWidth - 1, tile3dX + brushSize); i++)
	{
		for (j = tile3dY; j <= MIN(mapHeight - 1, tile3dY + brushSize); j++)
		{
			adjustTileHeight(mapTile(i, j), TILE_LOWER);
			markTileDirty(i,j);
		}
	}
}

/* Ensures any adjustment to tile elevation is within allowed ranges */
void	adjustTileHeight(MAPTILE *psTile, SDWORD adjust)
{
	int32_t newHeight = psTile->height + adjust*ELEVATION_SCALE;

	if (newHeight >= MIN_TILE_HEIGHT*ELEVATION_SCALE && newHeight <= MAX_TILE_HEIGHT*ELEVATION_SCALE)
	{
		psTile->height = newHeight;
	}
}

bool	inHighlight(UDWORD realX, UDWORD realY)
{
	bool	retVal = false;

	if (realX>=buildSite.xTL && realX<=buildSite.xBR)
	{
		if (realY>=buildSite.yTL && realY<=buildSite.yBR)
		{
			retVal = true;
		}
	}

	return(retVal);
}

void init3DBuilding(BASE_STATS *psStats,BUILDCALLBACK CallBack,void *UserData)
{
	ASSERT_OR_RETURN(,psStats, "Bad parameter");

	buildState = BUILD3D_POS;

	sBuildDetails.CallBack = CallBack;
	sBuildDetails.UserData = UserData;
	sBuildDetails.x = mouseTileX;
	sBuildDetails.y = mouseTileY;

	if (psStats->ref >= REF_STRUCTURE_START &&
		psStats->ref < (REF_STRUCTURE_START + REF_RANGE))
	{
		sBuildDetails.width = ((STRUCTURE_STATS *)psStats)->baseWidth;
		sBuildDetails.height = ((STRUCTURE_STATS *)psStats)->baseBreadth;
		sBuildDetails.psStats = psStats;
	}
	else if (psStats->ref >= REF_FEATURE_START &&
			 psStats->ref < (REF_FEATURE_START + REF_RANGE))
	{
		sBuildDetails.width = ((FEATURE_STATS *)psStats)->baseWidth;
		sBuildDetails.height = ((FEATURE_STATS *)psStats)->baseBreadth;
		sBuildDetails.psStats = psStats;
	}
	else /*if (psStats->ref >= REF_TEMPLATE_START &&
			 psStats->ref < (REF_TEMPLATE_START + REF_RANGE))*/
	{
		sBuildDetails.width = 1;
		sBuildDetails.height = 1;
		sBuildDetails.psStats = psStats;
	}
}

void	kill3DBuilding		( void )
{
    //cancel the drag boxes
    dragBox3D.status = DRAG_INACTIVE;
    wallDrag.status = DRAG_INACTIVE;
	buildState = BUILD3D_NONE;
}


// Call once per frame to handle structure positioning and callbacks.
//
bool process3DBuilding(void)
{
	//if not trying to build ignore
	if (buildState == BUILD3D_NONE)
  	{
		if (quickQueueMode && !ctrlShiftDown())
		{
			quickQueueMode = false;
			intDemolishCancel();
		}
		return true;
	}

	/* Need to update the building locations if we're building */
	int bX = clip(mouseTileX, 2, mapWidth - 3);
	int bY = clip(mouseTileY, 2, mapHeight - 3);

	if (buildState != BUILD3D_FINISHED)
	{
		Vector2i size = getStatsSize(sBuildDetails.psStats, player.r.y);
		Vector2i offset = size * (TILE_UNITS / 2);  // This presumably gets added to the chosen coordinates, somewhere, based on looking at what pickStructLocation does. No idea where it gets added, though.

		if (validLocation(sBuildDetails.psStats, world_coord(Vector2i(bX, bY)) + offset, player.r.y, selectedPlayer, true))
		{
			buildState = BUILD3D_VALID;
		}
		else
		{
			buildState = BUILD3D_POS;
		}
	}

	sBuildDetails.x = buildSite.xTL = bX;
	sBuildDetails.y = buildSite.yTL = bY;
	if (((player.r.y + 0x2000) & 0x4000) == 0)
	{
		buildSite.xBR = buildSite.xTL+sBuildDetails.width-1;
		buildSite.yBR = buildSite.yTL+sBuildDetails.height-1;
	}
	else
	{
		// Rotated 90°, swap width and height
		buildSite.xBR = buildSite.xTL+sBuildDetails.height-1;
		buildSite.yBR = buildSite.yTL+sBuildDetails.width-1;
	}

	if( (buildState == BUILD3D_FINISHED) && (sBuildDetails.CallBack != NULL) )
	{
		sBuildDetails.CallBack(sBuildDetails.x,sBuildDetails.y,sBuildDetails.UserData);
		buildState = BUILD3D_NONE;
		return true;
	}
	if (quickQueueMode && !ctrlShiftDown())
	{
		buildState = BUILD3D_NONE;
		quickQueueMode = false;
	}

	return false;
}


/* See if a structure location has been found */
bool found3DBuilding(UDWORD *x, UDWORD *y)
{
	if (buildState != BUILD3D_FINISHED || x == NULL || y == NULL)
	{
		return false;
	}

	*x = sBuildDetails.x;
	*y = sBuildDetails.y;

	if (ctrlShiftDown())
	{
		quickQueueMode = true;
		init3DBuilding(sBuildDetails.psStats, sBuildDetails.CallBack, sBuildDetails.UserData);
	}
	else
	{
		buildState = BUILD3D_NONE;
	}

	return true;
}

/* See if a second position for a build has been found */
bool found3DBuildLocTwo(UDWORD *px1, UDWORD *py1, UDWORD *px2, UDWORD *py2)
{
	if ( (((STRUCTURE_STATS *)sBuildDetails.psStats)->type != REF_WALL &&
		  ((STRUCTURE_STATS *)sBuildDetails.psStats)->type != REF_GATE &&
		  ((STRUCTURE_STATS *)sBuildDetails.psStats)->type != REF_DEFENSE &&
		  ((STRUCTURE_STATS *)sBuildDetails.psStats)->type != REF_REARM_PAD) ||
		wallDrag.status != DRAG_RELEASED)
	{
		return false;
	}

    //whilst we're still looking for a valid location - return false
    if (buildState == BUILD3D_POS)
    {
        return false;
    }

	wallDrag.status = DRAG_INACTIVE;
	*px1 = wallDrag.x1;
	*py1 = wallDrag.y1;
	*px2 = wallDrag.x2;
	*py2 = wallDrag.y2;

	if (ctrlShiftDown())
	{
		quickQueueMode = true;
		init3DBuilding(sBuildDetails.psStats, sBuildDetails.CallBack, sBuildDetails.UserData);
	}

	return true;
}

/*returns true if the build state is not equal to BUILD3D_NONE*/
bool tryingToGetLocation(void)
{
    if (buildState == BUILD3D_NONE)
    {
        return false;
    }
    else
    {
        return true;
    }
}
