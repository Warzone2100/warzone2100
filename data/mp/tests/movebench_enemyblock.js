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

// An enemy-held choke must stay solid.
//
// Player 0 is ordered through the pass. Player 1, non-allied, parks unarmed
// units in it and is given no orders. Walling a choke with units is a
// legitimate tactic, so this asserts three things:
//
//   - attackers must not get through. Spawn placement varies per arrangement
//     and a stray attacker occasionally slips the plug, so the criterion is a
//     median of 2 or fewer arrivals across arrangements rather than a hard zero
//   - defenders must not be shuffled aside, and hold position for the whole run
//   - yieldsIssued must be 0, since right-of-way is negotiated only among a
//     player's own or allied units and never across enemy lines
//
// Both sides are unarmed on purpose. Two armed hostile blocks would shoot each
// other and clear the choke, measuring combat rather than blocking.
//
// The attacking force is deliberately small. Blocking is not absolute here.
// Crowd pressure squeezes units sideways into blocked terrain, and the leak
// grows with the size of the pressing force. At this size the invariant holds
// cleanly. The enemyblock_press scenario covers the leak itself.

const ATTACKER = 0;
const DEFENDER = 1;

const N_ATTACK = 4;
const N_DEFEND = 6;

const Y_NORTH = 24;
const Y_SOUTH = 42;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ATTACKER, "truck");
	benchEnable(DEFENDER, "heavytruck");

	// Defenders plug the 2-wide gap across all three wall rows. They must be
	// heavy-bodied to actually seal it - see the roster note on clearance.
	benchSpawnFixed(DEFENDER, "heavytruck", X_GAP - 1, Y_WALL, 2, +1, N_DEFEND);

	var attackers = benchSpawnBlock(ATTACKER, "truck", X_GAP - 2, Y_NORTH, 4, -1, N_ATTACK);
	hackNetOn();

	// Ordered straight through the held pass. They should not get there.
	benchOrderFanOut(attackers, X_GAP - 4, Y_SOUTH, 8, +1);

	debug("movebench: enemyblock, " + attackers.length + " attackers vs " + N_DEFEND + " defenders");
}
