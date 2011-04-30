/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
 * @file raycast.c
 *
 * Raycasting routine that gives intersection points with map tiles.
 *
 */

#include "lib/framework/frame.h"

#include "raycast.h"
#include "map.h" // TILE_UNITS
#include "display3d.h" // clipXY()


struct HeightCallbackHelp_t
{
	const int height;
	uint16_t pitch;
};


void rayCast(Vector3i src, uint16_t direction, uint32_t length, RAY_CALLBACK callback, void *data)
{
	uint32_t currLen;
	for (currLen = 0; currLen < length; currLen += TILE_UNITS)
	{
		Vector3i curr = src + Vector3i(iSinCosR(direction, currLen), 0);
		// stop at the edge of the map
		if (curr.x < 0 || curr.x >= world_coord(mapWidth) ||
			curr.y < 0 || curr.y >= world_coord(mapHeight))
		{
			return;
		}

		if (!callback(curr, currLen, data))
		{
			// callback doesn't want any more points so return
			return;
		}
	}
}

//-----------------------------------------------------------------------------------
/* Will return false when we've hit the edge of the grid */
static bool getTileHeightCallback(Vector3i pos, int32_t dist, void *data)
{
	HeightCallbackHelp_t *help = (HeightCallbackHelp_t *)data;
#ifdef TEST_RAY
	Vector3i effect;
#endif

	/* Are we still on the grid? */
	if (clipXY(pos.x, pos.y))
	{
		bool HasTallStructure = blockTile(map_coord(pos.x), map_coord(pos.y), AUX_MAP) & AIR_BLOCKED;

		if (dist > TILE_UNITS || HasTallStructure)
		{
			// Only do it the current tile is > TILE_UNITS away from the starting tile. Or..
			// there is a tall structure  on the current tile and the current tile is not the starting tile.
			/* Get height at this intersection point */
			int height = map_Height(pos.x, pos.y), heightDiff;
			uint16_t newPitch;

			if (HasTallStructure)
			{
				height += TALLOBJECT_ADJUST;
			}

			if (height <= help->height)
			{
				heightDiff = 0;
			}
			else
			{
				heightDiff = height - help->height;
			}

			/* Work out the angle to this point from start point */
			newPitch = iAtan2(heightDiff, dist);

			/* Is this the steepest we've found? */
			if (angleDelta(newPitch - help->pitch) > 0)
			{
				/* Yes, then keep a record of it */
				help->pitch = newPitch;
			}
			//---

#ifdef TEST_RAY
			effect.x = pos.x;
			effect.y = height;
			effect.z = pos.y;
			addEffect(&effect, EFFECT_EXPLOSION, EXPLOSION_TYPE_SMALL, false, NULL, 0);
#endif
		}

		/* Not at edge yet - so exit */
		return true;
	}

	/* We've hit edge of grid - so exit!! */
	return false;
}

void getBestPitchToEdgeOfGrid(UDWORD x, UDWORD y, uint16_t direction, uint16_t *pitch)
{
	HeightCallbackHelp_t help = {map_Height(x,y), 0};

	rayCast(Vector3i(x, y, 0), direction, 5430, getTileHeightCallback, &help); // FIXME Magic value

	*pitch = help.pitch;
}
