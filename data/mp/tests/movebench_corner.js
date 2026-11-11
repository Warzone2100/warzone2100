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

// A single cluster rounding a corner into a narrow passage, on the L-shaped map.
//
// One cluster, one direction, no opposing flow, so this is congestion and
// serialisation rather than a head-on conflict.
//
// Corners bunch for a specific reason. moveBestTarget picks its aim point by
// ray-casting line of sight, and a corner wall blocks sight of any waypoint
// past the bend, so every unit's look-ahead stops at the same inside-corner
// tile until it physically rounds it. That tile becomes a point attractor for
// the whole cluster, on top of them all sharing one racing line.
//
// A 2-wide pinch admits only so many units per second, so the goal is an
// orderly, prompt queue rather than the absence of a queue. Watch bumps and
// hardStops rather than raw arrival time.

const ME = 0;
const N_UNITS = 24;

const X_STAGE = 27;      // 8-wide block straddling the corridor mouth at x=30,31
const Y_STAGE = 10;
const X_GOAL = 53;       // inside the goal area east of the corner
const Y_GOAL = 28;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	var cluster = benchSpawnBlock(ME, "medium", X_STAGE, Y_STAGE, 8, +1, N_UNITS);
	hackNetOn();

	benchOrderFanOut(cluster, X_GOAL, Y_GOAL, 8, +1);

	debug("movebench: corner, " + cluster.length + " units round the bend");
}
