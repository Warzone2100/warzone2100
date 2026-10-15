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
 *  Per-tick flow field for the congestion-aware planner.
 *
 *  Once per tick the planner rasterizes friendly droid facings into a summed
 *  flow vector per tile. A search prices arriving against that flow as extra
 *  step cost, so it routes around an oncoming column when there is a cheap way
 *  around and through it when there is not. The cost is never a hard block, so
 *  it only unclogs traffic and never changes which tiles are passable.
 *
 *  The field is relationship-specific. A searching player sees its own and
 *  allied units' flow and enemy units as nothing, so parking units to deny a
 *  chokepoint to an enemy still works exactly as before.
 *
 *  Built on the main thread and then read-only, keyed by gameTime, so a worker
 *  thread that reads it during a search never sees it change.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

/// What one of the overlay owner's droids added to the flow field, and the goal it was heading for.
struct OwnFlowStamp
{
	uint32_t droidId;
	int16_t  cx, cy;   ///< the tile the stamp is centered on
	int16_t  fx, fy;   ///< the facing added to every tile of the stamp
	uint32_t goal;     ///< the droid's destination tile index + 1, or 0 if it had none
};

/// One cohort's flow field for one tick. A cohort is a player together with
/// everyone allied to it, keyed by that player.
struct DynamicCostOverlay
{
	uint32_t gameTime = 0;
	int      cohortPlayer = -1;   ///< the player whose own and allied traffic this holds
	int      width = 0;
	int      height = 0;
	std::vector<int16_t> flowX;    ///< summed facing vectors of occupants, per tile, row-major
	std::vector<int16_t> flowY;
	std::vector<uint16_t> mass;    ///< summed occupant mass, MASS_UNIT per stamped body, per tile
	std::vector<OwnFlowStamp> ownStamps;   ///< the owner's own flow stamps, only when flow is consumed
	bool consumeFlow = false;      ///< the search prices opposing flow from this overlay
	bool consumeMass = false;      ///< the search prices incoherent crowd mass from this overlay
	uint32_t checksum = 0;         ///< fold of the flow field, emitted to syncDebug
};

/// Builds one overlay per player that has droids. Each holds that player's
/// own and allied flow. The returned vector is indexed by player, null
/// where a player has no droids. Iterates in a fixed order so the result
/// is deterministic. The consume flags snapshot the synced setting at build
/// time, so a worker reading the overlay never reads settings.
std::vector<std::shared_ptr<const DynamicCostOverlay>> buildCongestionOverlays(uint32_t buildTime, bool consumeFlow, bool consumeMass);

/// The summed flow stamps of one group, the overlay owner's droids that were heading for the same goal tile
/// when the overlay was built, over the group's bounding box. Immutable once built.
struct GroupFlowMap
{
	uint32_t key = 0;   ///< the group's goal, as OwnFlowStamp::goal
	int x0 = 0, y0 = 0, w = 0, h = 0;   ///< the bounding box, in tiles
	std::vector<int32_t> fx, fy;        ///< summed facings per tile of the box, row-major

	/// The group's flow on a tile, or false (and nothing written) outside its bounding box.
	bool lookup(int x, int y, int32_t &gx, int32_t &gy) const
	{
		const int lx = x - x0;
		const int ly = y - y0;
		if (lx < 0 || ly < 0 || lx >= w || ly >= h)
		{
			return false;
		}
		gx = fx[lx + ly * w];
		gy = fy[lx + ly * w];
		return true;
	}
};

/// Sums the stamps of the overlay owner's droids whose goal is `key` (non-zero).
std::shared_ptr<const GroupFlowMap> buildGroupFlowMap(const DynamicCostOverlay &overlay, uint32_t key);
