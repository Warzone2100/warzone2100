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

// Spawns the units the pathfinding benchmark pushes through the worker pool.
//
// The direct measurement builds its requests by hand and needs nothing here.
// These exist only for the queued measurement, which has to go through
// fpathDroidRoute and therefore needs real droids to route.
//
// They are placed above the first band of the serpentine and never ordered
// anywhere by this script, so whatever they do is the benchmark's doing. Wheeled
// to match the propulsion the direct requests use, so both measurements search
// the same blocking map.

const N_UNITS = 32;
const SPAWN_X = 8;       // clear of the west gap, so the block is not sitting in it
const SPAWN_Y = 4;
const COLS = 8;

const UNIT = { name: "PathBench Unit", body: "Body5REC", prop: "wheeled01", weap: "MG1Mk1" };

function eventStartLevel()
{
	setPower(1000000, me);
	makeComponentAvailable(UNIT.body, me);
	makeComponentAvailable(UNIT.prop, me);
	makeComponentAvailable(UNIT.weap, me);

	for (var i = 0; i < N_UNITS; i++)
	{
		var x = SPAWN_X + (i % COLS);
		var y = SPAWN_Y + Math.floor(i / COLS);
		addDroid(me, x, y, UNIT.name, UNIT.body, UNIT.prop, "", "", UNIT.weap);
	}
	debug("pathbench: spawned " + N_UNITS + " units");
}
