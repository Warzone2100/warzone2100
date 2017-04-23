/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
 * @file raycast.cpp
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

static void initSteps(int32_t srcM, int32_t dstM, int32_t &tile, int32_t &step, int32_t &cur, int32_t &end)
{
	int increasing = srcM < dstM;
	step = -1 + 2 * increasing;
	tile = srcM - step;
	cur = srcM + increasing;
	end = dstM + increasing;
}

// Finds the next intersection of the line with a vertical grid line (or with a horizontal grid line, if called with x and y swapped).
static bool tryStep(int32_t &tile, int32_t step, int32_t &cur, int32_t end, int32_t &px, int32_t &py, int32_t sx, int32_t sy, int32_t dx, int32_t dy)
{
	tile += step;

	if (cur == end)
	{
		return false;  // No more vertical grid lines to cross before reaching the endpoint.
	}

	// Find the point on the line with the x coordinate world_coord(cur).
	px = world_coord(cur);
	py = sy + int64_t(px - sx) * (dy - sy) / (dx - sx);

	cur += step;
	return true;
}

void rayCast(Vector2i src, Vector2i dst, RAY_CALLBACK callback, void *data)
{
	if (!callback(src, 0, data) || src == dst)  // Start at src.
	{
		return;  // Callback gave up after the first point, or there are no other points.
	}

	Vector2i srcM = map_coord(src);
	Vector2i dstM = map_coord(dst);

	Vector2i step, tile, cur, end;
	initSteps(srcM.x, dstM.x, tile.x, step.x, cur.x, end.x);
	initSteps(srcM.y, dstM.y, tile.y, step.y, cur.y, end.y);

	Vector2i prev(0, 0);  // Dummy initialisation.
	bool first = true;
	Vector2i nextX(0, 0), nextY(0, 0);  // Dummy initialisations.
	bool canX = tryStep(tile.x, step.x, cur.x, end.x, nextX.x, nextX.y, src.x, src.y, dst.x, dst.y);
	bool canY = tryStep(tile.y, step.y, cur.y, end.y, nextY.y, nextY.x, src.y, src.x, dst.y, dst.x);
	while (canX || canY)
	{
		int32_t xDist = abs(nextX.x - src.x) + abs(nextX.y - src.y);
		int32_t yDist = abs(nextY.x - src.x) + abs(nextY.y - src.y);
		Vector2i sel;
		Vector2i selTile;
		if (canX && (!canY || xDist < yDist))  // The line crosses a vertical grid line next.
		{
			sel = nextX;
			selTile = tile;
			canX = tryStep(tile.x, step.x, cur.x, end.x, nextX.x, nextX.y, src.x, src.y, dst.x, dst.y);
		}
		else  // The line crosses a horizontal grid line next.
		{
			assert(canY);
			sel = nextY;
			selTile = tile;
			canY = tryStep(tile.y, step.y, cur.y, end.y, nextY.y, nextY.x, src.y, src.x, dst.y, dst.x);
		}
		if (!first)
		{
			// Find midpoint.
			Vector2i avg = (prev + sel) / 2;
			// But make sure it's on the right tile, since it could be off-by-one if the line passes exactly through a grid intersection.
			avg.x = std::min(std::max(avg.x, world_coord(selTile.x)), world_coord(selTile.x + 1) - 1);
			avg.y = std::min(std::max(avg.y, world_coord(selTile.y)), world_coord(selTile.y + 1) - 1);
			if (!worldOnMap(avg) || !callback(avg, iHypot(avg), data))
			{
				return;  // Callback doesn't want any more points, or we reached the edge of the map, so return.
			}
		}
		prev = sel;
		first = false;
	}

	// Include the endpoint.
	if (!worldOnMap(dst))
	{
		return;  // Stop, since reached the edge of the map.
	}
	callback(dst, iHypot(dst), data);
}

//-----------------------------------------------------------------------------------
/* Will return false when we've hit the edge of the grid */
static bool getTileHeightCallback(Vector2i pos, int32_t dist, void *data)
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
	HeightCallbackHelp_t help = {map_Height(x, y), 0};

	Vector3i src(x, y, 0);
	Vector3i delta(iSinCosR(direction, 5430), 0);
	rayCast(src.xy, (src + delta).xy, getTileHeightCallback, &help); // FIXME Magic value

	*pitch = help.pitch;
}
