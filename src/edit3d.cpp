/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
#include "lib/framework/math_ext.h"
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

BuildState buildState = BUILD3D_NONE;
HIGHLIGHT buildSite;
BUILDDETAILS sBuildDetails;
int brushSize = 1;
bool quickQueueMode = false;

// Initialisation function for statis & globals in this module.
//
void Edit3DInitVars()
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
			markTileDirty(i, j);
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
			markTileDirty(i, j);
		}
	}
}

/* Ensures any adjustment to tile elevation is within allowed ranges */
void	adjustTileHeight(MAPTILE *psTile, SDWORD adjust)
{
	int32_t newHeight = psTile->height + adjust;

	if (newHeight >= TILE_MIN_HEIGHT && newHeight <= TILE_MAX_HEIGHT)
	{
		psTile->height = newHeight;
	}
}

void init3DBuilding(BASE_STATS *psStats, BUILDCALLBACK CallBack, void *UserData)
{
	ASSERT_OR_RETURN(, psStats, "Bad parameter");

	buildState = BUILD3D_POS;

	sBuildDetails.CallBack = CallBack;
	sBuildDetails.UserData = UserData;

	if (psStats->hasType(STAT_STRUCTURE))
	{
		sBuildDetails.width = ((STRUCTURE_STATS *)psStats)->baseWidth;
		sBuildDetails.height = ((STRUCTURE_STATS *)psStats)->baseBreadth;
		sBuildDetails.psStats = psStats;
	}
	else if (psStats->hasType(STAT_FEATURE))
	{
		sBuildDetails.width = ((FEATURE_STATS *)psStats)->baseWidth;
		sBuildDetails.height = ((FEATURE_STATS *)psStats)->baseBreadth;
		sBuildDetails.psStats = psStats;
	}
	else /*if (psStats->hasType(STAT_TEMPLATE))*/
	{
		sBuildDetails.width = 1;
		sBuildDetails.height = 1;
		sBuildDetails.psStats = psStats;
	}

	sBuildDetails.x = map_coord(mousePos.x - world_coord(sBuildDetails.width)/2);
	sBuildDetails.y = map_coord(mousePos.y - world_coord(sBuildDetails.height)/2);
}

void	kill3DBuilding()
{
	//cancel the drag boxes
	dragBox3D.status = DRAG_INACTIVE;
	wallDrag.status = DRAG_INACTIVE;
	buildState = BUILD3D_NONE;
}


// Call once per frame to handle structure positioning and callbacks.
//
bool process3DBuilding()
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
	int border = 5*TILE_UNITS/2;
	Vector2i bv = {clip(mousePos.x, border, mapWidth*TILE_UNITS - border), clip(mousePos.y, border, mapHeight*TILE_UNITS - border)};
	Vector2i size = getStatsSize(sBuildDetails.psStats, getBuildingDirection());
	Vector2i worldSize = world_coord(size);
	bv = round_to_nearest_tile(bv - worldSize/2) + worldSize/2;

	if (buildState != BUILD3D_FINISHED)
	{
		bool isValid = true;
		if (wallDrag.status != DRAG_INACTIVE && sBuildDetails.psStats->hasType(STAT_STRUCTURE) && canLineBuild())
		{
			wallDrag.pos2 = mousePos;  // Why must this be done here? If not doing it here, dragging works almost normally, except it suddenly stops working if the drag becomes invalid.

			auto lb = calcLineBuild(static_cast<STRUCTURE_STATS *>(sBuildDetails.psStats), getBuildingDirection(), wallDrag.pos, wallDrag.pos2);
			for (int i = 0; i < lb.count && isValid; ++i)
			{
				isValid &= validLocation(sBuildDetails.psStats, lb[i], getBuildingDirection(), selectedPlayer, true);
			}
		}
		else
		{
			isValid = validLocation(sBuildDetails.psStats, bv, getBuildingDirection(), selectedPlayer, true);
		}

		buildState = isValid? BUILD3D_VALID : BUILD3D_POS;
	}

	sBuildDetails.x = buildSite.xTL = map_coord(bv.x - worldSize.x/2);
	sBuildDetails.y = buildSite.yTL = map_coord(bv.y - worldSize.y/2);
	if ((getBuildingDirection() & 0x4000) == 0)
	{
		buildSite.xBR = buildSite.xTL + sBuildDetails.width - 1;
		buildSite.yBR = buildSite.yTL + sBuildDetails.height - 1;
	}
	else
	{
		// Rotated 90°, swap width and height
		buildSite.xBR = buildSite.xTL + sBuildDetails.height - 1;
		buildSite.yBR = buildSite.yTL + sBuildDetails.width - 1;
	}

	if ((buildState == BUILD3D_FINISHED) && (sBuildDetails.CallBack != nullptr))
	{
		sBuildDetails.CallBack(sBuildDetails.x, sBuildDetails.y, sBuildDetails.UserData);
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

void incrementBuildingDirection(uint16_t amount)
{
	sBuildDetails.directionShift += amount;
}

uint16_t getBuildingDirection()
{
	return snapDirection(playerPos.r.y + sBuildDetails.directionShift);
}

/* See if a structure location has been found */
bool found3DBuilding(Vector2i &pos)
{
	if (buildState != BUILD3D_FINISHED)
	{
		return false;
	}

	pos = world_coord({sBuildDetails.x, sBuildDetails.y}) + world_coord(
		(getBuildingDirection() & 0x4000) == 0?
			Vector2i{sBuildDetails.width, sBuildDetails.height} : Vector2i{sBuildDetails.height, sBuildDetails.width}
	)/2;

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
bool found3DBuildLocTwo(Vector2i &pos, Vector2i &pos2)
{
	if (wallDrag.status != DRAG_RELEASED || !canLineBuild())
	{
		return false;
	}

	//whilst we're still looking for a valid location - return false
	if (buildState == BUILD3D_POS)
	{
		return false;
	}

	wallDrag.status = DRAG_INACTIVE;
	STRUCTURE_STATS *stats = (STRUCTURE_STATS *)sBuildDetails.psStats;
	LineBuild lb = calcLineBuild(stats, getBuildingDirection(), wallDrag.pos, wallDrag.pos2);
	pos = lb.begin;
	pos2 = lb.back();

	if (ctrlShiftDown())
	{
		quickQueueMode = true;
		init3DBuilding(sBuildDetails.psStats, sBuildDetails.CallBack, sBuildDetails.UserData);
	}

	return true;
}

/*returns true if the build state is not equal to BUILD3D_NONE*/
bool tryingToGetLocation()
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
