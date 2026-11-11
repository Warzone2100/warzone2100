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

// A large same-speed mass strafing back and forth across open ground.
//
// Selecting a mass of units and walking them repeatedly across open terrain is
// core play, and it is the behavior most exposed to collateral damage from
// changes aimed at chokepoints. This is a guard rather than a target. Nothing
// here should improve, and nothing should regress.
//
// The reversal is the risk moment. When the mass turns around, trailing units
// become leading ones and briefly face the rest of their own formation, which
// looks like opposing headings plus contact. A single translation never
// produces that, so the reorders matter more than the distance covered.
//
// What to watch: yieldsIssued should stay at zero, and bumps, giveUps and
// formationSpreadTiles must not rise against the baseline. The runner is told
// not to stop on completion, since the mass arrives at the end of every leg.

const ME = 0;
const N_UNITS = 60;

const X_WEST = 8;
const X_EAST = 44;
const Y_ROW = 6;
const COLS = 10;

const LEG_MS = 15000;    // reversal interval, in game time

var headingEast = true;

function strafeReverse()
{
	headingEast = !headingEast;
	benchOrderFanOut(enumDroid(ME), headingEast ? X_EAST : X_WEST, Y_ROW, COLS, +1);
}

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	var mass = benchSpawnBlock(ME, "medium", X_WEST, Y_ROW, COLS, +1, N_UNITS);
	hackNetOn();

	benchOrderFanOut(mass, X_EAST, Y_ROW, COLS, +1);
	setTimer("strafeReverse", LEG_MS);

	debug("movebench: strafe, " + mass.length + " units");
}
