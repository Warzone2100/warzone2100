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

// Conflicting diagonal flows whose paths cross inside the pass.
//
// Both blocks approach the gap from the west and leave to the east while
// swapping sides of the wall. The north block runs north-west to south-east,
// the south block runs south-west to north-east, so their shortest paths
// intersect inside the passage rather than merely meeting head-on.
//
// Lane formation alone cannot resolve this, because the optimal lines genuinely
// cross. Compare against the separating scenario, which is the same map and the
// same two blocks with the diagonals arranged so they do not conflict.

const ME = 0;
const N_PER_SIDE = 16;

const X_WEST = 22;       // approach side for both blocks
const X_EAST = 38;       // exit side for both blocks
const Y_NORTH = 24;
const Y_SOUTH = 42;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	var north = benchSpawnBlock(ME, "medium", X_WEST, Y_NORTH, 4, -1, N_PER_SIDE);
	var south = benchSpawnBlock(ME, "medium", X_WEST, Y_SOUTH, 4, +1, N_PER_SIDE);
	hackNetOn();

	benchOrderFanOut(north, X_EAST, Y_SOUTH, 8, +1);   // NW -> SE
	benchOrderFanOut(south, X_EAST, Y_NORTH, 8, -1);   // SW -> NE

	debug("movebench: crossing, " + (north.length + south.length) + " units");
}
