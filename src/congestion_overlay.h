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
	uint32_t checksum = 0;         ///< fold of the flow field, emitted to syncDebug
};

/// Builds one overlay per player that has droids. Each holds that player's
/// own and allied flow. The returned vector is indexed by player, null
/// where a player has no droids. Iterates in a fixed order so the result
/// is deterministic.
std::vector<std::shared_ptr<const DynamicCostOverlay>> buildCongestionOverlays(uint32_t buildTime);
