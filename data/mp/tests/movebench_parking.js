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

// A moving column through a field of parked units.
//
// Exercises moveShuffleDroid, which fires only against an ally already sitting
// at MOVEINACTIVE. The parked units are given no orders at all, so the runner
// never tracks them and unitsOrdered counts only the column.

const ME = 0;
const N_COLUMN = 8;
const N_PARKED = 24;
const PARK_SPACING = 2;   // loose field, not a solid wall of hulls

const Y_COLUMN = 8;      // column starts north of the parked field
const Y_PARKED = 18;     // parked field, 4 rows deep
const Y_GOAL = 28;       // goal is south of the parked field, so it must pass through

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	// Parked first, so the column has something to negotiate from tick 0. The
	// field is deliberately loose: packed shoulder to shoulder it is not
	// negotiable at all, which measures jamming rather than shuffle behavior.
	benchSpawnBlock(ME, "medium", X_GAP - 11, Y_PARKED, 12, +1, N_PARKED, PARK_SPACING);
	var column = benchSpawnBlock(ME, "medium", X_GAP - 2, Y_COLUMN, 4, -1, N_COLUMN);
	hackNetOn();

	benchOrderFanOut(column, X_GAP - 4, Y_GOAL, 8, +1);

	debug("movebench: parking, " + column.length + " moving past " + N_PARKED + " idle");
}
