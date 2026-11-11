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

// Many indirect projectiles in flight at once.
//
// A bank of mortar emplacements shells a parked block well inside their range
// and outside the block's own. Nothing much moves, so the tick is dominated by
// the in-flight collision loop, periodical damage and the structures' target
// selection. This is also the reference scenario for the render-side timers -
// run it with --movementbenchwatch=perf_artillery.

const TARGET = 0;
const GUNNER = 1;

const N_GUNS = 40;
const GUN_COLS = 20;
const Y_GUNS = 10;

const N_TARGETS = 100;
const TARGET_COLS = 20;
const Y_TARGETS = 24;

// Indirect-fire structures shoot only at what a sensor designates for them, and
// a mortar pit's own sensor does not reach this far.
const N_SENSORS = 2;
const Y_SENSORS = 14;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(TARGET, "medium");
	benchEnable(GUNNER, "medium");
	var built = 0;
	for (var i = 0; i < N_GUNS; i++)
	{
		var x = X_GAP - 10 + (i % GUN_COLS);
		var y = Y_GUNS + Math.floor(i / GUN_COLS) * 2;
		if (addStructure("Emplacement-MortarPit01", GUNNER, x * 128, y * 128) !== null)
		{
			built++;
		}
	}
	for (var j = 0; j < N_SENSORS; j++)
	{
		addStructure("Sys-SensoTower02", GUNNER, (X_GAP - 4 + j * 8) * 128, Y_SENSORS * 128);
	}
	var targets = benchSpawnBlock(TARGET, "medium", X_GAP - 10, Y_TARGETS, TARGET_COLS, +1, N_TARGETS);
	hackNetOn();

	// No orders: the block sits where it is and is shelled. Its machineguns
	// cannot reach the emplacements, so the exchange stays one-sided.
	debug("movebench: perf_artillery, " + built + " mortar pits shelling " + targets.length + " tanks");
}
