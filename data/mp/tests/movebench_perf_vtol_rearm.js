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

// VTOLs cycling between a defended target and their rearming pads.
//
// The pads' idle update walks every droid of the owner and then every ally to
// find one waiting to rearm, and each candidate walks the list again to see
// whether a pad is occupied. AA fire keeps the VTOLs damaged so they keep
// coming back, and the AA sites are the only homing-direct projectiles in the
// suite that are aimed upward.

const FLIER = 0;
const DEFENDER = 1;

const N_VTOL = 20;
const VTOL_COLS = 10;
const Y_VTOL = 6;

const N_PADS = 4;
const Y_PADS = 4;

const N_DEFEND = 40;
const DEFEND_COLS = 10;
const Y_DEFEND = 45;

const N_AA = 4;
const Y_AA = 42;

function eventStartLevel()
{
	hackNetOff();
	benchEnable(FLIER, "vtol");
	benchEnable(DEFENDER, "medium");

	var pads = 0;
	for (var i = 0; i < N_PADS; i++)
	{
		if (addStructure("A0VtolPad", FLIER, (X_GAP - 4 + i * 2) * 128, Y_PADS * 128) !== null)
		{
			pads++;
		}
	}
	for (var j = 0; j < N_AA; j++)
	{
		addStructure("AASite-QuadMg1", DEFENDER, (X_GAP - 4 + j * 3) * 128, Y_AA * 128);
	}

	var vtols = benchSpawnFixed(FLIER, "vtol", X_GAP - 5, Y_VTOL, VTOL_COLS, +1, N_VTOL);
	var defenders = benchSpawnBlock(DEFENDER, "medium", X_GAP - 5, Y_DEFEND, DEFEND_COLS, +1, N_DEFEND);
	hackNetOn();

	for (var k = 0; k < vtols.length; k++)
	{
		orderDroidLoc(vtols[k], DORDER_SCOUT, X_GAP - 5 + (k % DEFEND_COLS), Y_DEFEND);
	}

	debug("movebench: perf_vtol_rearm, " + vtols.length + " VTOLs, " + pads + " pads, "
	      + defenders.length + " defenders");
}
