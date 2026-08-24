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
 *  Corridor coordination, the layer between the route planner and local
 *  steering. A droid moving through a detected corridor is steered to keep to
 *  its own side, so opposing flows split into two lanes rather than colliding
 *  as a blob. A droid waiting to enter a corridor whose flow runs the other way
 *  is held outside until its turn, so the two directions alternate instead of
 *  stuffing head-on. The droid's route is never changed, only the point its
 *  steering aims at and whether it advances this tick.
 */

#pragma once

#include "lib/framework/vector.h"

#include <cstdint>
#include <optional>
#include <vector>

struct DROID;

/// What one tick of the gate hands to the next.
/// Everything else the gate holds is rebuilt from synced droid state before anything reads it, so this is all a save has to carry.
/// The checksum records which corridor map it was measured against, since the flow is indexed by corridor id.
struct CorridorGateCarry
{
	uint32_t mapChecksum = 0;
	std::vector<uint8_t> flow;   ///< per corridor, bit 0 flow toward the far mouth, bit 1 toward the near one
};

/// The carry a save should record.
std::optional<CorridorGateCarry> corridorGateCarry();

/// Restores a carry after a load.
/// Applied on the next update, and only if it was recorded against the corridor map that is now loaded.
void corridorGateSetCarry(std::optional<CorridorGateCarry> carry);

/// Recomputes the per-corridor flow state for this tick. Call once before any
/// droid moves, so every droid decides against the same snapshot.
void corridorGateUpdate();

/// If the droid is inside a detected corridor, fills laneTarget with a steering
/// point that keeps it on its side and returns true. Returns false when the
/// droid is not inside a corridor, so the caller steers to its own waypoint.
/// Deterministic and integer, reads only synced state.
bool corridorLaneTarget(const DROID *psDroid, Vector2i &laneTarget);

/// The speed a droid queued at a corridor may move at this tick, ramping down
/// as it nears its queue slot and zero once there, so the file rolls smoothly.
/// Returns moveSpeed unchanged for a droid that is not queued.
int corridorQueueSpeed(const DROID *psDroid, int moveSpeed);

/// Keeps collision slides from shoving a droid across the centerline into the
/// opposing lane. Steering keeps a droid on its side, but contact resolution
/// knows nothing of lanes, and under crowding it is what actually moves droids.
/// Cuts only the crossing component of the given velocity, in place.
void corridorClampSlide(const DROID *psDroid, int32_t *pdx, int32_t *pdy);

/// Whether this corridor carries opposing flows this tick. The gate updates
/// before the overlay build and before any droid moves, so every reader sees
/// the tick's own state, derived from synced state alone. False for
/// out-of-range ids.
bool corridorContested(int corridorId);

/// How the corridor layer wants the blocked watchdog to treat this droid.
enum CorridorHold
{
	CORRIDOR_HOLD_NONE,     ///< not held by the layer, normal watch
	CORRIDOR_HOLD_QUEUED,   ///< queued at an outer mouth, the speed hold stops it, reset the watch
	CORRIDOR_HOLD_JUNCTION, ///< waiting into an inner junction, pause the watch but keep it running
};
CorridorHold corridorHold(const DROID *psDroid);
