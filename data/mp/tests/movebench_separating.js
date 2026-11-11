// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

include("tests/movebench_common.js");

// Opposite diagonals that separate without coordination.
//
// The north block stays east, the south block stays west. Path cost is genuine
// shortest path, so each block hugs the side of the pass it was already biased
// toward and the two streams self-separate into distinct lanes.
//
// That corner-hugging is doing real work here, which is why this cell is much
// easier than crossing despite the same map, unit count and head-on setup. It
// guards against a counter-flow change destroying the free separation.

const ME = 0;
const N_PER_SIDE = 16;

const X_WEST = 22;
const X_EAST = 38;
const Y_NORTH = 24;
const Y_SOUTH = 42;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	var north = benchSpawnBlock(ME, "medium", X_EAST, Y_NORTH, 4, -1, N_PER_SIDE);
	var south = benchSpawnBlock(ME, "medium", X_WEST, Y_SOUTH, 4, +1, N_PER_SIDE);
	hackNetOn();

	benchOrderFanOut(north, X_EAST, Y_SOUTH, 8, +1);   // NE -> SE, stays east
	benchOrderFanOut(south, X_WEST, Y_NORTH, 8, -1);   // SW -> NW, stays west

	debug("movebench: separating, " + (north.length + south.length) + " units");
}
