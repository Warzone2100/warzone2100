/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/piepalette.h"

#include "bridge.h"
#include "display3d.h"
#include "effects.h"
#include "map.h"

/**
 * @file bridge.c
 * Bridges - currently unused.
 *
 * Handles rendering and placement of bridging sections for
 * traversing water and ravines?! My guess is this won't make it into
 * the final game, but we'll see...
 * Alex McLean, Pumpkin Studios EIDOS Interactive, 1998.
 *
 * He was right. It did not make the final game. The code below
 * is unused. Can it be reused? - Per, 2007
 */

#define MINIMUM_BRIDGE_SPAN	2
#define MAXIMUM_BRIDGE_SPAN	12

/*
Returns true or false as to whether a bridge is valid.
For it to be true - all intervening points must be lower than the start
and end points. We can also check other stuff here like what it's going
over. Also, it has to be between a minimum and maximum length and
one of the axes must share the same values.
*/
bool bridgeValid(int startX, int startY, int endX, int endY)
{
	bool	xBridge, yBridge;
	int	bridgeLength, i, minX, minY, maxX, maxY;

	/* Establish axes allignment */
	xBridge = startX == endX;
	yBridge = startY == endY;

	/* At least one axis must be constant */
	if (!xBridge && !yBridge)
	{
		return false;
	}

	minX = MIN(startX, endX);
	maxX = MAX(startX, endX);
	minY = MIN(startY, endY);
	maxY = MAX(startY, endY);

	/* Get the bridge length */
	bridgeLength = (xBridge ? abs(startY - endY) : abs(startX - endX));

	/* check it's not too long or short */
	if (bridgeLength < MINIMUM_BRIDGE_SPAN || bridgeLength > MAXIMUM_BRIDGE_SPAN)
	{
		return false;
	}

	if (xBridge)
	{
		for (i = minY + 1; i < maxY - 1; i++)
		{
			if (terrainType(mapTile(startX, i)) != TER_WATER)
			{
				debug(LOG_ERROR, "Bridge cannot cross !water - X");
				return false;
			}
		}
	}
	else
	{
		for (i = minX + 1; i < maxX - 1; i++)
		{
			if (terrainType(mapTile(i, startY)) != TER_WATER)
			{
				debug(LOG_ERROR, "Bridge cannot cross !water - Y");
				return false;
			}
		}
	}

	return true;
}

/*
	This function will actually draw a wall section
	Slightly different from yer basic structure draw in that
	it's not alligned to the terrain as bridge sections sit
	at a height stored in their structure - as they're above the ground
	and wouldn't be much use if they weren't, bridge wise.
*/
bool	renderBridgeSection(STRUCTURE *psStructure)
{
	Vector3i dv;

	/* Bomb out if it's not visible */
	if (!psStructure->visible[selectedPlayer])
	{
		return false;
	}

	/* Establish where it is in the world */
	dv.x = psStructure->pos.x - player.p.x;
	dv.z = -(psStructure->pos.y - player.p.z);
	dv.y = psStructure->pos.z;

	/* Push the indentity matrix */
	pie_MatBegin();

	/* Translate */
	pie_TRANSLATE(dv.x, dv.y, dv.z);

	pie_Draw3DShape(psStructure->sDisplay.imd, 0, 0, WZCOL_WHITE, 0, 0);

	pie_MatEnd();
	return(true);
}

/*
	This will work out all the info you need about the bridge including
	length - height to set sections at in order to allign to terrain and
	what you need to alter start and end terrain heights by to establish to
	connection.
*/
void getBridgeInfo(int startX, int startY, int endX, int endY, BRIDGE_INFO *info)
{
	int	startHeight, endHeight;

	/* Copy over the location coordinates, and make sure they go lower to higher */
	info->startX = MIN(startX, endX);
	info->startY = MIN(startY, endY);
	info->endX = MAX(startX, endX);
	info->endY = MAX(startY, endY);

	/* Get the heights of the start and end positions */
	startHeight = map_TileHeight(startX, startY);
	endHeight = map_TileHeight(endX, endY);

	/* Find out which is higher */
	info->startHighest = (startHeight >= endHeight);

	if (info->startHighest)
	{
		info->heightChange = startHeight - endHeight;
		info->bridgeHeight = startHeight;
	}
	else
	{
		info->heightChange = endHeight - startHeight;
		info->bridgeHeight = endHeight;
	}

	info->bConstantX = (startX == endX);

	/* We've got a bridge of constant X */
	if (info->bConstantX)
	{
		info->bridgeLength = abs(startY - endY);
	}
	else
	{
		info->bridgeLength = abs(startX - endX);
	}
}

void testBuildBridge(UDWORD startX, UDWORD startY, UDWORD endX, UDWORD endY)
{
	BRIDGE_INFO	bridge;
	UDWORD	i;
	Vector3i dv;

	if (bridgeValid(startX, startY, endX, endY))
	{
		getBridgeInfo(startX, startY, endX, endY, &bridge);
		dv.y = bridge.bridgeHeight;
		dv.x = bridge.startX * TILE_UNITS + TILE_UNITS / 2;
		dv.z = bridge.startY * TILE_UNITS + TILE_UNITS / 2;
		addEffect(&dv, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL, false, NULL, 0);
		if (bridge.bConstantX)
		{
			for (i = bridge.startY + 1; i < bridge.endY - 1; i++)
			{
				dv.x = ((bridge.startX * TILE_UNITS) + (TILE_UNITS / 2));
				dv.z = ((i * TILE_UNITS) + (TILE_UNITS / 2));
				addEffect(&dv, EFFECT_SMOKE, SMOKE_TYPE_DRIFTING, false, NULL, 0);
			}
		}
		else
		{
			for (i = startX + 1; i < bridge.endX - 1; i++)
			{
				dv.x = ((i * TILE_UNITS) + (TILE_UNITS / 2));
				dv.z = ((bridge.startY * TILE_UNITS) + (TILE_UNITS / 2));
				addEffect(&dv, EFFECT_SMOKE, SMOKE_TYPE_DRIFTING, false, NULL, 0);
			}
		}
		dv.x = bridge.endX * TILE_UNITS + TILE_UNITS / 2;
		dv.z = bridge.endY * TILE_UNITS + TILE_UNITS / 2;
		addEffect(&dv, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL, false, NULL, 0);
	}
	else
	{
		dv.y = map_TileHeight(startX, startY);
		dv.x = startX * TILE_UNITS + TILE_UNITS / 2;
		dv.z = startY * TILE_UNITS + TILE_UNITS / 2;
		addEffect(&dv, EFFECT_EXPLOSION, EXPLOSION_TYPE_LASER, false, NULL, 0);
	}
}
