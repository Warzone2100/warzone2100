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

// The mountain_chain scenario with the cyborg destination moved to the near
// side of the tank intake, so the arriving cyborgs must cross the tanks'
// approach right at the chain exit - the exit-stream against intake-line
// tension, which the base scenario's far-side destination does not exercise.
//
// Opposing tank and cyborg flows through Sk-Mountain's chained passes, the
// winding corridor system between the western and north-eastern entrances.
// The chain holds every hazard the synthetic maps cannot: adjoining member
// mouths across small pockets, a one-tile pinch that a cyborg and a tank can
// thread together but two tanks cannot, S-bends against cliff corners, and a
// junction pocket mid-chain. Cyborgs run west to north-east, tanks the
// reverse, and the cyborgs are re-ordered to the same destination twice while
// mid-transit, which regenerates their paths from inside coordinated space -
// historically the sharpest stress on route-grounded classification.

const ME = 0;
const N_UNITS = 41;   // one clone wars worth per side, as manually tested
const REORDER_MS = 25000;

var cyborgs = [];
var reorders = 0;

const CYB_TARGET_X = 76, CYB_TARGET_Y = 70;

function mountainCrossReorder()
{
	// Twice only. Re-ordering forever would keep issuing degenerate short legs
	// after arrival and pollute the per-leg statistics.
	if (++reorders > 2)
	{
		removeTimer("mountainCrossReorder");
		return;
	}
	benchOrderAllTo(cyborgs, CYB_TARGET_X, CYB_TARGET_Y);
}

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "lightcannonht");
	benchEnable(ME, "cyborg");
	var tanks = benchSpawnBlock(ME, "lightcannonht", 78, 70, 6, +1, N_UNITS);
	cyborgs = benchSpawnBlock(ME, "cyborg", 58, 82, 5, +1, N_UNITS);
	hackNetOn();

	benchOrderAllTo(tanks, 60, 83);
	benchOrderAllTo(cyborgs, CYB_TARGET_X, CYB_TARGET_Y);
	setTimer("mountainCrossReorder", REORDER_MS);

	debug("movebench: mountain chain cross, " + (tanks.length + cyborgs.length) + " units");
}
