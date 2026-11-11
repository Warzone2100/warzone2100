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

// The counterflow conflict across enemy lines:
// two blocks of unarmed trucks, one per hostile player, swap sides through the 2-wide gap.
//
// Coordination is negotiated only among a player's own or allied units, so neither side may
// lane up with, queue behind or wait for the other - the encounter resolves by contact, just
// like against any other enemy obstacle.
//
// Trucks rather than tanks for the same reason as enemyblock: armed hostiles would shoot the
// conflict away and we want to measure movement (not combat).

const NORTH = 0;
const SOUTH = 1;
const N_PER_SIDE = 16;

const Y_NORTH = 24;      // north block's row nearest the wall
const Y_SOUTH = 42;      // south block's row nearest the wall

function eventStartLevel()
{
	hackNetOff();
	benchEnable(NORTH, "truck");
	benchEnable(SOUTH, "truck");
	var north = benchSpawnBlock(NORTH, "truck", X_GAP - 2, Y_NORTH, 4, -1, N_PER_SIDE);
	var south = benchSpawnBlock(SOUTH, "truck", X_GAP - 2, Y_SOUTH, 4, +1, N_PER_SIDE);
	hackNetOn();

	benchOrderFanOut(north, X_GAP - 4, Y_SOUTH, 8, +1);   // north block, swapping south
	benchOrderFanOut(south, X_GAP - 4, Y_NORTH, 8, -1);   // south block, swapping north

	debug("movebench: counterflow_hostile, " + (north.length + south.length) + " units");
}
