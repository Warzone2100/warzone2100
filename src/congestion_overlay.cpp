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
 *  Flow field build. See congestion_overlay.h.
 */

#include "congestion_overlay.h"

#include "lib/framework/frame.h"

#include "objects.h"
#include "ai.h"          // aiCheckAlliances, alliances
#include "map.h"
#include "game_world.h"
#include "droid.h"

#include "lib/framework/trig.h"

#include <algorithm>
#include <cstdio>

namespace
{

// Tiles of halo stamped around each droid, so paths keep a little clear of
// friendly traffic rather than grazing it.
const int FOOTPRINT_RADIUS = 1;

// One droid's contribution to the flow field: its body-facing unit vector at
// this length, summed per tile. A stopped droid still contributes, it faces
// where it is pushing, which is what an opposing search needs to see.
const int32_t FLOW_UNIT = 64;

// One droid's contribution to the mass channel, per stamped tile. Equal to
// FLOW_UNIT so a tile's mass and the L1 length of its summed flow compare
// directly: aligned occupants keep the flow near or above the mass, idle or
// canceled occupants leave it far below.
const int32_t MASS_UNIT = 64;

uint32_t foldChecksum(const DynamicCostOverlay &overlay)
{
	uint32_t checksum = 0, factor = 0;
	for (size_t i = 0; i < overlay.flowX.size(); ++i)
	{
		checksum ^= static_cast<uint16_t>(overlay.flowX[i]) * (factor = 3 * factor + 1);
		checksum ^= static_cast<uint16_t>(overlay.flowY[i]) * (factor = 3 * factor + 1);
	}
	for (size_t i = 0; i < overlay.mass.size(); ++i)
	{
		checksum ^= overlay.mass[i] * (factor = 3 * factor + 1);
	}
	return checksum;
}

} // anonymous namespace

std::vector<std::shared_ptr<const DynamicCostOverlay>> buildCongestionOverlays(uint32_t buildTime)
{
	const int width = gameWorld.map.width;
	const int height = gameWorld.map.height;
	const size_t cells = static_cast<size_t>(width) * static_cast<size_t>(height);

	// One writable overlay per player that has droids. Players ascending, so the
	// build order is fixed.
	std::vector<std::shared_ptr<DynamicCostOverlay>> building(MAX_PLAYERS);
	for (int player = 0; player < MAX_PLAYERS; ++player)
	{
		if (gameWorld.objects.droids[player].empty())
		{
			continue;
		}
		auto overlay = std::make_shared<DynamicCostOverlay>();
		overlay->gameTime = buildTime;
		overlay->cohortPlayer = player;
		overlay->width = width;
		overlay->height = height;
		overlay->flowX.assign(cells, 0);
		overlay->flowY.assign(cells, 0);
		overlay->mass.assign(cells, 0);
		building[player] = std::move(overlay);
	}

	// Stamp every droid into the overlays that treat it as own or allied. Owners
	// ascending, then each list in its stored order, so the sum is deterministic.
	for (int owner = 0; owner < MAX_PLAYERS; ++owner)
	{
		for (const DROID *psDroid : gameWorld.objects.droids[owner])
		{
			const int cx = map_coord(psDroid->pos.x);
			const int cy = map_coord(psDroid->pos.y);
			const bool flowDroid = !psDroid->isVtol();
			const int32_t fx = flowDroid ? iSinR(psDroid->rot.direction, FLOW_UNIT) : 0;
			const int32_t fy = flowDroid ? iCosR(psDroid->rot.direction, FLOW_UNIT) : 0;

			for (int player = 0; player < MAX_PLAYERS; ++player)
			{
				const auto &overlay = building[player];
				if (!overlay)
				{
					continue;
				}
				if (player != owner && !aiCheckAlliances(player, owner))
				{
					continue;   // enemy units contribute nothing, preserving the block
				}
				for (int dy = -FOOTPRINT_RADIUS; dy <= FOOTPRINT_RADIUS; ++dy)
				{
					const int y = cy + dy;
					if (y < 0 || y >= height)
					{
						continue;
					}
					for (int dx = -FOOTPRINT_RADIUS; dx <= FOOTPRINT_RADIUS; ++dx)
					{
						const int x = cx + dx;
						if (x < 0 || x >= width)
						{
							continue;
						}
						const size_t idx = static_cast<size_t>(x) + static_cast<size_t>(y) * static_cast<size_t>(width);
						if (flowDroid)
						{
							overlay->flowX[idx] = static_cast<int16_t>(std::clamp<int32_t>(overlay->flowX[idx] + fx, INT16_MIN, INT16_MAX));
							overlay->flowY[idx] = static_cast<int16_t>(std::clamp<int32_t>(overlay->flowY[idx] + fy, INT16_MIN, INT16_MAX));
							overlay->mass[idx] = static_cast<uint16_t>(std::min<int32_t>(overlay->mass[idx] + MASS_UNIT, UINT16_MAX));
						}
					}
				}
			}
		}
	}


	std::vector<std::shared_ptr<const DynamicCostOverlay>> result(MAX_PLAYERS);
	for (int player = 0; player < MAX_PLAYERS; ++player)
	{
		if (building[player])
		{
			building[player]->checksum = foldChecksum(*building[player]);
			result[player] = building[player];
		}
	}
	return result;
}
