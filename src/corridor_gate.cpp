// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** \file
 *  Corridor coordination. See corridor_gate.h.
 */

#include "corridor_gate.h"

#include "lib/framework/frame.h"
#include "lib/framework/fixedpoint.h"
#include "lib/framework/trig.h"

#include "corridor_map.h"
#include "droid.h"
#include "game_world.h"
#include "map.h"

#include <algorithm>
#include <cstdint>

namespace
{

// How far off a droid's tile to look for a corridor centerline, in tiles. Half
// the widest corridor, since a droid can sit that far to the side of the line.
const int SEARCH_RADIUS = 3;

// How many centerline points ahead of the droid to aim at. Steering toward a
// point a few tiles down its lane keeps the path smooth rather than snapping to
// the nearest centerline point.
const int LOOKAHEAD = 3;

} // anonymous namespace

bool corridorLaneTarget(const DROID *psDroid, Vector2i &laneTarget)
{
	const CorridorMap *cmap = gameWorld.map.corridors.get();
	if (cmap == nullptr || cmap->corridors.empty())
	{
		return false;
	}
	const Vector2i pos = psDroid->pos.xy();
	const int tx = map_coord(pos.x);
	const int ty = map_coord(pos.y);

	// Nearest corridor centerline tile to the droid.
	int bestId = -1;
	int bestTileDistSq = SEARCH_RADIUS * SEARCH_RADIUS + 1;
	for (int dy = -SEARCH_RADIUS; dy <= SEARCH_RADIUS; ++dy)
	{
		for (int dx = -SEARCH_RADIUS; dx <= SEARCH_RADIUS; ++dx)
		{
			const int16_t id = cmap->at(tx + dx, ty + dy);
			if (id < 0)
			{
				continue;
			}
			const int distSq = dx * dx + dy * dy;
			if (distSq < bestTileDistSq)
			{
				bestTileDistSq = distSq;
				bestId = id;
			}
		}
	}
	if (bestId < 0)
	{
		return false;
	}

	const Corridor &c = cmap->corridors[static_cast<size_t>(bestId)];
	if (c.centerline.size() < 3)
	{
		return false;
	}

	// Nearest centerline point, so the local direction and lane offset are taken
	// where the droid actually is.
	size_t nearest = 0;
	int64_t nearestDistSq = INT64_MAX;
	for (size_t i = 0; i < c.centerline.size(); ++i)
	{
		const Vector2i d = c.centerline[i] - pos;
		const int64_t distSq = static_cast<int64_t>(d.x) * d.x + static_cast<int64_t>(d.y) * d.y;
		if (distSq < nearestDistSq)
		{
			nearestDistSq = distSq;
			nearest = i;
		}
	}

	const size_t lo = nearest > 0 ? nearest - 1 : nearest;
	const size_t hi = nearest + 1 < c.centerline.size() ? nearest + 1 : nearest;
	const Vector2i axis = c.centerline[hi] - c.centerline[lo];
	if (axis.x == 0 && axis.y == 0)
	{
		return false;
	}

	// Which way along the corridor the droid is trying to go, so opposing flows
	// take opposite lanes.
	const Vector2i toTarget = psDroid->sMove.target - pos;
	const int forward = (toTarget.x * axis.x + toTarget.y * axis.y) >= 0 ? 1 : -1;
	const Vector2i travel = axis * forward;

	// A point a few tiles ahead down the corridor, offset to the travelling
	// droid's right so the two directions separate into two lanes.
	const int aheadIdx = std::clamp(static_cast<int>(nearest) + forward * LOOKAHEAD,
	                                0, static_cast<int>(c.centerline.size()) - 1);
	const Vector2i ahead = c.centerline[aheadIdx];
	const int32_t laneOffset = c.widthProfile[nearest] / 4;
	const uint16_t travelDir = iAtan2(travel);
	// Angles run clockwise from a downward y axis, so a right turn is minus a
	// quarter, which puts each direction on its own right and the two on opposite
	// sides of the corridor.
	const Vector2i toRight = iSinCosR(static_cast<uint16_t>(travelDir - DEG(90)), laneOffset);

	laneTarget = ahead + toRight;
	return true;
}
