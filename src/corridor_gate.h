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

struct DROID;

/// Recomputes the per-corridor flow state for this tick. Call once before any
/// droid moves, so every droid decides against the same snapshot.
void corridorGateUpdate();

/// If the droid is inside a detected corridor, fills laneTarget with a steering
/// point that keeps it on its side and returns true. Returns false when the
/// droid is not inside a corridor, so the caller steers to its own waypoint.
/// Deterministic and integer, reads only synced state.
bool corridorLaneTarget(const DROID *psDroid, Vector2i &laneTarget);

/// True if the droid is waiting to enter a corridor whose current flow runs the
/// other way, so the movement layer should hold it in place until its turn.
bool corridorShouldHold(const DROID *psDroid);
