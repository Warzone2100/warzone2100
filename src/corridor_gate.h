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

#include "lib/framework/frame.h"
#include "lib/framework/vector.h"

#include <array>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

struct CorridorMap;
struct DROID;
struct GameWorld;

/// What one tick of the gate hands to the next.
/// Everything else the gate holds is rebuilt from synced droid state before anything reads it, so this is all a save has to carry.
/// The checksum records which corridor map it was measured against, since the flow is indexed by corridor id.
struct CorridorGateCarry
{
	uint32_t mapChecksum = 0;
	std::vector<uint8_t> flow;   ///< per corridor, bit 0 flow toward the far mouth, bit 1 toward the near one
};

/// One cohort's corridor flow state, derived each update from the droids of one player and its
/// allies against the world's corridor map. Coordination never reaches across enemy lines: an
/// enemy stream is not a flow to form lanes with or to wait for, it is an obstacle, exactly as
/// legacy movement treats it everywhere. Everything here except prevFlow is rebuilt by
/// corridorGateUpdate before anything reads it.
struct CorridorFlowState
{
	/// Per-corridor flow direction for this tick: +1, -1, or 0 for open. Rebuilt each
	/// tick from synced droid state, so it is never saved.
	std::vector<int8_t> activeDir;

	/// Per-corridor, whether both directions have droids at it this tick. When only
	/// one direction is present it fills the whole passage, when both it splits.
	std::vector<uint8_t> contested;

	/// Per-droid queue slot for this tick: the position along the approach line,
	/// negative behind the mouth, that the droid should advance to and wait at.
	/// The approaching droids of each direction are ranked front to back, so they
	/// file in one behind another instead of arriving abreast at a mouth that fits
	/// one or two. Rebuilt each tick from synced droid state, so it is never saved.
	std::unordered_map<uint32_t, int32_t> slotAlong;

	/// Per corridor and direction, the span of centerline indices the direction's
	/// routes actually cover this tick, read as far as the route scan window sees.
	/// Gates the mouth crossing zones: an entry zone the opposing flow's coverage
	/// stays clear of is free to pre-sort across, ex. merging through the unused
	/// half of a junction neck, instead of honouring a reservation nothing will
	/// claim there. Rebuilt each tick, never saved.
	std::vector<int> useLoFwd, useHiFwd, useLoBwd, useHiBwd;

	/// Per corridor and direction, how many droids are inside it this tick.
	/// The pre-sort only engages against a stream physically in the passage, so it
	/// sorts a merger against real oncoming traffic and never reshapes a column
	/// entering ahead of traffic that is still far away. Rebuilt each tick.
	std::vector<int> cInsideFwd, cInsideBwd;
	/// Per corridor and direction, droids inside or approaching it. Counting only
	/// insiders would make a joint read one-way exactly when the crossing is
	/// busiest, since a droid crossing between two passages is inside neither.
	std::vector<int> cFlowFwd, cFlowBwd;

	/// Per-corridor lane handedness for this tick: +1 keeps each direction to its
	/// right, -1 to its left. Keeping right is an arbitrary convention, and the
	/// right convention is dictated by where the traffic exits: a flow turning off
	/// to one side wants the lane on the inside of its turn, or it must cross the
	/// opposing lane exactly at the turn. Rebuilt each tick, never saved.
	std::vector<int8_t> handed;

	/// Flow at each corridor as of the last update, bit 0 toward the far mouth and bit 1 toward
	/// the near one, the same shape the save carry records. Read one update deep: whether a joint
	/// carries both directions is judged against this settled picture, not against counts still
	/// being taken. prevFlowChecksum records the corridor map it was measured against, since the
	/// flow is indexed by corridor id: flow from another map is discarded rather than applied, and
	/// a restored carry that matches the loaded map survives the first update's reset.
	std::vector<uint8_t> prevFlow;
	uint32_t prevFlowChecksum = 0;
};

/// One world's corridor flow, one state per player. Owned by the GameWorld so it travels through
/// world swaps with the droids and map it describes, and a save records any world's carries from
/// the world itself. Each player's state is measured over its own and allied droids, the same
/// cohort the congestion overlay stamps, and a droid is always judged against its owner's state.
/// Mutually allied players hold identical state, computed once and shared.
struct CorridorFlowWorld
{
	std::array<CorridorFlowState, MAX_PLAYERS> perPlayer;

	/// Which corridors instance the last update ran against. The first update against a new one
	/// announces the map into the sync stream and clears the joint flow the previous map's ticks
	/// left behind.
	const CorridorMap *seenMap = nullptr;
	uint32_t seenChecksum = 0;
};

/// The carry a save should record for this world and player, or nullopt while no update or
/// restored carry has measured that player's flow against the world's current corridor map.
std::optional<CorridorGateCarry> corridorGateCarry(const GameWorld &world, unsigned player);

/// Restores a saved carry into this world's flow for one player.
/// Applied immediately, and only if it was recorded against the corridor map the world now holds.
void corridorGateRestoreCarry(GameWorld &world, unsigned player, const CorridorGateCarry &carry);

/// Recomputes the world's per-corridor flow state for this tick. Call once
/// before any droid moves, so every droid decides against the same snapshot.
void corridorGateUpdate(GameWorld &world);

/// If the droid is inside a detected corridor, fills laneTarget with a steering
/// point that keeps it on its side and returns true. Returns false when the
/// droid is not inside a corridor, so the caller steers to its own waypoint.
/// Deterministic and integer, reads only synced state.
bool corridorLaneTarget(const GameWorld &world, const DROID *psDroid, Vector2i &laneTarget);

/// The speed a droid queued at a corridor may move at this tick, ramping down
/// as it nears its queue slot and zero once there, so the file rolls smoothly.
/// Returns moveSpeed unchanged for a droid that is not queued.
int corridorQueueSpeed(const GameWorld &world, const DROID *psDroid, int moveSpeed);

/// Keeps collision slides from shoving a droid across the centerline into the
/// opposing lane. Steering keeps a droid on its side, but contact resolution
/// knows nothing of lanes, and under crowding it is what actually moves droids.
/// Cuts only the crossing component of the given velocity, in place.
void corridorClampSlide(const GameWorld &world, const DROID *psDroid, int32_t *pdx, int32_t *pdy);

/// Whether this corridor carries opposing flows of this player's cohort this
/// tick. The gate updates before the overlay build and before any droid moves,
/// so every reader sees the tick's own state, derived from synced state alone.
/// False for out-of-range ids or players.
bool corridorContested(const GameWorld &world, unsigned player, int corridorId);

/// How the corridor layer wants the blocked watchdog to treat this droid.
enum CorridorHold
{
	CORRIDOR_HOLD_NONE,     ///< not held by the layer, normal watch
	CORRIDOR_HOLD_QUEUED,   ///< queued at an outer mouth, the speed hold stops it, reset the watch
	CORRIDOR_HOLD_JUNCTION, ///< waiting into an inner junction, pause the watch but keep it running
};
CorridorHold corridorHold(const GameWorld &world, const DROID *psDroid);
