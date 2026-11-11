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

// Mixed-speed cluster rounding the same corner.
//
// Interleaving medium and heavy tanks adds the catch-up pile-up. Faster units
// reach the corner attractor and stack behind slower ones, and a heavy tank's
// slower pivot at the bend stalls everything behind it.
//
// The uniform corner scenario is the control that says any difference came from
// speed heterogeneity rather than from the geometry.

const ME = 0;
const N_UNITS = 24;

const X_STAGE = 27;
const Y_STAGE = 10;
const X_GOAL = 53;
const Y_GOAL = 28;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	benchEnable(ME, "heavy");
	var cluster = benchSpawnMixedBlock(ME, ["medium", "heavy"], X_STAGE, Y_STAGE, 8, +1, N_UNITS);
	hackNetOn();

	benchOrderFanOut(cluster, X_GOAL, Y_GOAL, 8, +1);

	debug("movebench: corner_mixed, " + cluster.length + " units round the bend");
}
