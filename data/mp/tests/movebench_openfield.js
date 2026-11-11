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

// A wide block translating laterally across open ground.
//
// No choke, no opposing flow, no parked obstacles. Units keep station and only
// make glancing contact, mostly as they converge on their goals.
//
// This is a guard rather than a target. Any non-trivial yielding here is a
// fault, and bumps and arrival times must not regress against the baseline.

const ME = 0;
const N_UNITS = 40;

const X_START = 8;
const X_GOAL = 46;
const Y_ROW = 10;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	var block = benchSpawnBlock(ME, "medium", X_START, Y_ROW, 10, +1, N_UNITS);
	hackNetOn();

	// Straight lateral translation: same heading for everyone, so nothing here
	// resembles a head-on stall.
	benchOrderFanOut(block, X_GOAL, Y_ROW, 10, +1);

	debug("movebench: openfield, " + block.length + " units translating");
}
