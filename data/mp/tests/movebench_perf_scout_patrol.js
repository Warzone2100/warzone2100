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

// Armed units on patrol past a hostile parked block.
//
// A patrolling droid runs the nearest-target search on every tick it has no
// action, unthrottled, which is the one path in orderUpdateDroid that does so.
// The parked block gives it something to find without resolving the patrol.

const SCOUT = 0;
const PARKED = 1;

const N_SCOUT = 30;
const SCOUT_COLS = 10;

const N_PARKED = 20;
const PARKED_COLS = 10;

const X_START = X_GAP - 10;
const Y_START = 8;
const X_END = X_GAP + 10;
const Y_END = 26;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(SCOUT, "medium");
	benchEnable(PARKED, "truck");
	var scouts = benchSpawnBlock(SCOUT, "medium", X_START, Y_START, SCOUT_COLS, +1, N_SCOUT);
	benchSpawnFixed(PARKED, "truck", X_GAP - 5, 16, PARKED_COLS, +1, N_PARKED);
	hackNetOn();

	for (var i = 0; i < scouts.length; i++)
	{
		orderDroidLoc(scouts[i], DORDER_PATROL, X_END, Y_END);
	}

	debug("movebench: perf_scout_patrol, " + scouts.length + " scouts past " + N_PARKED + " parked");
}
