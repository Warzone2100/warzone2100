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

// Many units ordered to a single destination, on the open ground north of the
// wall.
//
// Every unit is ordered to the same tile, which makes the destination a point
// attractor. Each unit runs its own path to the identical tile and follows the
// same centreline, so they converge single-file instead of spreading.
//
// unitsArrived is inherently low here and is not a failure. A hundred units
// cannot fit within arrival tolerance of one tile, so read stuckRemainingTiles
// for how tightly the blob packed in, and bumps and repaths for thrash.

const ME = 0;
const N_UNITS = 100;

const Y_START = 6;
const Y_GOAL = 28;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	var blob = benchSpawnBlock(ME, "medium", X_GAP - 5, Y_START, 10, +1, N_UNITS);
	hackNetOn();

	benchOrderAllTo(blob, X_GAP, Y_GOAL);

	debug("movebench: blob, " + blob.length + " units to one tile");
}
