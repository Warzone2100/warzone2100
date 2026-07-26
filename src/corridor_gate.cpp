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
#include "pathfinding_backend.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace
{

// How far off a droid's tile to look for a corridor centerline, in tiles. Half
// the widest corridor, since a droid can sit that far to the side of the line.
const int SEARCH_RADIUS = 3;

// How many centerline points ahead to aim the steering, so a droid follows its
// lane smoothly rather than snapping to the nearest point.
const int LOOKAHEAD = 3;

// A droid whose nearest centerline point is within this of a mouth counts as
// still at the entrance rather than through it.
const int MOUTH_MARGIN = 2;

// Which way a droid relates to a corridor this tick.
enum Relation
{
	NOT_IN_CORRIDOR,
	INSIDE,       ///< between the mouths, within the width, following a lane
	APPROACHING,  ///< outside a mouth and heading in, subject to the flow gate
};

struct Query
{
	Relation relation = NOT_IN_CORRIDOR;
	int corridorId = -1;
	size_t nearest = 0;         ///< nearest centerline point index
	Vector2i axis = Vector2i(0, 0); ///< local centerline direction, index-increasing
	int dir = 0;                ///< +1 travelling toward the far mouth, -1 toward the near one
};

// Per-corridor flow direction for this tick: +1, -1, or 0 for open. Rebuilt each
// tick from synced droid state, so it is never saved.
std::vector<int8_t> g_activeDir;

Query queryCorridor(const Vector2i &pos, const Vector2i &target)
{
	Query q;
	const CorridorMap *cmap = gameWorld.map.corridors.get();
	if (cmap == nullptr || cmap->corridors.empty())
	{
		return q;
	}
	const int tx = map_coord(pos.x);
	const int ty = map_coord(pos.y);

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
		return q;
	}

	const Corridor &c = cmap->corridors[static_cast<size_t>(bestId)];
	if (c.centerline.size() < 3)
	{
		return q;
	}
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
		return q;
	}

	q.corridorId = bestId;
	q.nearest = nearest;
	q.axis = axis;
	const Vector2i toTarget = target - pos;
	q.dir = (static_cast<int64_t>(toTarget.x) * axis.x + static_cast<int64_t>(toTarget.y) * axis.y) >= 0 ? 1 : -1;

	const size_t last = c.centerline.size() - 1;
	const bool endpoint = (nearest == 0 || nearest == last);
	const Vector2i fromLine = pos - c.centerline[nearest];
	int64_t lateral = static_cast<int64_t>(fromLine.x) * axis.y - static_cast<int64_t>(fromLine.y) * axis.x;
	if (lateral < 0)
	{
		lateral = -lateral;
	}
	const bool withinWidth = lateral <= static_cast<int64_t>(c.widthProfile[nearest] / 2) * iHypot(axis);

	if (!endpoint && withinWidth)
	{
		q.relation = INSIDE;
	}
	else if ((nearest <= static_cast<size_t>(MOUTH_MARGIN) && q.dir > 0)
	         || (nearest + static_cast<size_t>(MOUTH_MARGIN) >= last && q.dir < 0))
	{
		q.relation = APPROACHING;   // at a mouth, heading into the corridor
	}
	return q;
}

Query queryDroid(const DROID *psDroid)
{
	return queryCorridor(psDroid->pos.xy(), psDroid->sMove.target);
}

} // anonymous namespace

void corridorGateUpdate()
{
	const CorridorMap *cmap = gameWorld.map.corridors.get();
	if (!pathfindingCorridorLanesEnabled() || cmap == nullptr)
	{
		g_activeDir.clear();
		return;
	}
	const size_t n = cmap->corridors.size();
	std::vector<int> insideFwd(n, 0);
	std::vector<int> insideBwd(n, 0);
	std::vector<uint32_t> lowestId(n, UINT32_MAX);
	std::vector<int8_t> lowestIdDir(n, 0);

	for (unsigned player = 0; player < MAX_PLAYERS; ++player)
	{
		for (const DROID *psDroid : gameWorld.objects.droids[player])
		{
			const Query q = queryDroid(psDroid);
			if (q.relation == NOT_IN_CORRIDOR)
			{
				continue;
			}
			const size_t c = static_cast<size_t>(q.corridorId);
			if (q.relation == INSIDE)
			{
				(q.dir > 0 ? insideFwd[c] : insideBwd[c]) += 1;
			}
			if (psDroid->id < lowestId[c])
			{
				lowestId[c] = psDroid->id;
				lowestIdDir[c] = static_cast<int8_t>(q.dir);
			}
		}
	}

	g_activeDir.assign(n, 0);
	for (size_t c = 0; c < n; ++c)
	{
		if (insideFwd[c] > insideBwd[c])
		{
			g_activeDir[c] = 1;
		}
		else if (insideBwd[c] > insideFwd[c])
		{
			g_activeDir[c] = -1;
		}
		else
		{
			// No majority either way, so the lowest-id droid at this corridor,
			// which every client picks the same, sets the direction that flows.
			g_activeDir[c] = lowestIdDir[c];
		}
	}
}

bool corridorLaneTarget(const DROID *psDroid, Vector2i &laneTarget)
{
	const Query q = queryDroid(psDroid);
	if (q.relation != INSIDE)
	{
		return false;
	}
	const Corridor &c = gameWorld.map.corridors->corridors[static_cast<size_t>(q.corridorId)];
	const Vector2i travel = q.axis * q.dir;

	const int aheadIdx = std::clamp(static_cast<int>(q.nearest) + q.dir * LOOKAHEAD,
	                                0, static_cast<int>(c.centerline.size()) - 1);
	const Vector2i ahead = c.centerline[aheadIdx];
	const int32_t laneOffset = c.widthProfile[q.nearest] / 4;
	const uint16_t travelDir = iAtan2(travel);
	// Angles run clockwise from a downward y axis, so a right turn is minus a
	// quarter, which puts each direction on its own right and the two on opposite
	// sides of the corridor.
	const Vector2i toRight = iSinCosR(static_cast<uint16_t>(travelDir - DEG(90)), laneOffset);

	laneTarget = ahead + toRight;
	return true;
}

// Only a corridor too narrow for a lane each way needs turn-taking. A wider one
// runs both directions at once in lanes, so metering it would just stall a
// direction and block the shared mouth.
const int32_t METER_MAX_WIDTH = 2 * TILE_UNITS;

// Held droids wait at least this far back from the mouth, so the mouth stays
// clear for the flow coming out of it. Holding right at the mouth would block
// the opposing exit and deadlock the corridor.
const int32_t HOLD_STANDOFF = 2 * TILE_UNITS;

bool corridorShouldHold(const DROID *psDroid)
{
	const Query q = queryDroid(psDroid);
	if (q.relation != APPROACHING || q.corridorId < 0)
	{
		return false;
	}
	const size_t c = static_cast<size_t>(q.corridorId);
	if (c >= g_activeDir.size() || g_activeDir[c] == 0 || q.dir == g_activeDir[c])
	{
		return false;   // corridor open, or this droid's direction has it
	}
	const Corridor &cor = gameWorld.map.corridors->corridors[c];
	if (cor.minWidth >= METER_MAX_WIDTH)
	{
		return false;   // wide enough for both directions at once
	}
	// Only hold droids still back from the mouth. One already at the apron enters
	// rather than stopping on the exit, and the rest queue behind it.
	const Vector2i mouth = q.dir > 0 ? cor.centerline.front() : cor.centerline.back();
	const Vector2i d = psDroid->pos.xy() - mouth;
	const int64_t distSq = static_cast<int64_t>(d.x) * d.x + static_cast<int64_t>(d.y) * d.y;
	return distSq >= static_cast<int64_t>(HOLD_STANDOFF) * HOLD_STANDOFF;
}
