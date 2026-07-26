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
 *  as a blob. The droid's route is not changed, only the point its steering
 *  aims at while inside the corridor.
 */

#pragma once

#include "lib/framework/vector.h"

struct DROID;

/// If the droid is inside a detected corridor, fills laneTarget with a steering
/// point that keeps it on its side of the corridor and returns true. Returns
/// false when the droid is not in a corridor, so the caller steers to its own
/// waypoint as usual. Deterministic and integer, reads only synced state.
bool corridorLaneTarget(const DROID *psDroid, Vector2i &laneTarget);
