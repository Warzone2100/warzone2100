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

// A cyborg crowd crossing through itself, on the open ground north of the wall.
//
// The movement neighbour queries scale with how many objects sit inside a
// droid's collision radius, so this is the case that makes moveCalcDroidSlide
// and moveCheckSquished dominate a tick. Half the block is sent to the far
// side and half back to the start row, so the two flows interpenetrate at the
// densest point rather than merely converging.
//
// unitsArrived is meaningless here - every unit in a half shares one goal tile.
// Read the perf counters, not the movement metrics.

const ME = 0;
const N_UNITS = 400;
const COLS = 20;

const Y_START = 6;
const Y_GOAL = 28;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "cyborg");
	var blob = benchSpawnBlock(ME, "cyborg", X_GAP - 10, Y_START, COLS, +1, N_UNITS);
	hackNetOn();

	var half = Math.floor(blob.length / 2);
	for (var i = 0; i < blob.length; i++)
	{
		orderDroidLoc(blob[i], DORDER_MOVE, X_GAP, i < half ? Y_GOAL : Y_START);
	}

	debug("movebench: perf_cyborg_blob, " + blob.length + " cyborgs crossing");
}
