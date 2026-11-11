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

// Opposing flows through Sk-Rush's centre with a mid-corridor departure: the
// cyborgs run the long diagonal corridor south-west and peel off south toward
// the centre partway down, the tanks run the full corridor the other way. The
// cyborgs' turn is the lane handedness acceptance case: their departure bend
// sits at an inner junction, often with its vertex corner-cut a tile inside
// the corridor being left, and the correct assignment puts them on the inside
// of their turn with the tanks outside, so the flows never cross at it. The
// cyborgs are re-ordered to the same destination mid-transit, regenerating
// paths from inside coordinated space.

const ME = 0;
const N_UNITS = 41;   // one clone wars worth per side, as manually tested
const REORDER_MS = 25000;

var cyborgs = [];
var reorders = 0;

const CYB_TARGET_X = 46, CYB_TARGET_Y = 48;

function rushReorder()
{
	// Twice only. Re-ordering forever would keep issuing degenerate short legs
	// after arrival and pollute the per-leg statistics.
	if (++reorders > 2)
	{
		removeTimer("rushReorder");
		return;
	}
	benchOrderAllTo(cyborgs, CYB_TARGET_X, CYB_TARGET_Y);
}

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "lightcannonht");
	benchEnable(ME, "cyborg");
	// The tanks pack the small raised area, ringed around mid-ridge so the
	// overflow crowds the rise instead of a block's far rows running down the
	// south slope below it.
	var tanks = benchSpawnCluster(ME, "lightcannonht", 35, 45, N_UNITS);
	cyborgs = benchSpawnBlock(ME, "cyborg", 57, 31, 4, +1, N_UNITS);
	hackNetOn();

	benchOrderAllTo(tanks, 57, 31);
	benchOrderAllTo(cyborgs, CYB_TARGET_X, CYB_TARGET_Y);
	setTimer("rushReorder", REORDER_MS);

	debug("movebench: rush turn, " + (tanks.length + cyborgs.length) + " units");
}
