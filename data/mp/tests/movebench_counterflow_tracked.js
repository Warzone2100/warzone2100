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

// Two blocks of medium tanks belonging to the same player swap sides through a
// 2-wide gap, so they meet head-on. Nothing here is hostile. The units simply
// cannot resolve each other.
//
// Tracked units are used because only ground-model units take pitch from slope.
// Persons and cyborgs skip updateDroidOrientation, so they would not see the
// sloped corridor walls at all.

const ME = 0;
const N_PER_SIDE = 16;

const Y_NORTH = 24;      // north block's row nearest the wall
const Y_SOUTH = 42;      // south block's row nearest the wall

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	var north = benchSpawnBlock(ME, "medium", X_GAP - 2, Y_NORTH, 4, -1, N_PER_SIDE);
	var south = benchSpawnBlock(ME, "medium", X_GAP - 2, Y_SOUTH, 4, +1, N_PER_SIDE);
	hackNetOn();

	benchOrderFanOut(north, X_GAP - 4, Y_SOUTH, 8, +1);   // north block, swapping south
	benchOrderFanOut(south, X_GAP - 4, Y_NORTH, 8, -1);   // south block, swapping north

	debug("movebench: counterflow, " + (north.length + south.length) + " units");
}
