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

// A repair facility and a screen of repair droids working on a damaged column.
//
// findSomeoneToRepair() runs from the repair facility's update and from an idle repair droid's, and
// nothing else in the suite reaches either: no scenario builds a repair facility, and no roster fields
// a DROID_REPAIR. So the repair-candidate query, its priority queue and the RTR clearing path all go
// unexercised without this.

const ME = 0;

const N_REPAIR = 8;
const N_WOUNDED = 30;
const COLS = 6;

// A repair facility searches only TILE_UNITS * 5 / 2 around itself and a repair droid REPAIR_MAXDIST,
// so the wounded sit in the droids' reach and on the edge of the facilities'. Packing them onto the
// facilities instead pins every one in Repairing for the whole run and the search stops being called.
const X0 = X_GAP - 8;
const Y_FAC = 8;
const Y_REPAIR = 11;
const Y_WOUNDED = 14;

// Light damage on a period longer than the repair it causes, so the wounded cycle between needing a
// repairer and releasing one. Damage that outruns the repair pins every repairer on one target.
const DAMAGED_PERCENT = 60;
const REDAMAGE_MS = 4000;

function woundEveryone()
{
	var droids = enumDroid(ME);
	for (var i = 0; i < droids.length; i++)
	{
		if (droids[i].droidType !== DROID_REPAIR)
		{
			setHealth(droids[i], DAMAGED_PERCENT);
		}
	}
}

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "medium");
	benchEnable(ME, "repair");
	enableStructure("A0RepairCentre3", ME);

	var placed = 0;
	for (var i = 0; i < 3; i++)
	{
		if (addStructure("A0RepairCentre3", ME, (X0 + i * 5) * 128, Y_FAC * 128) !== null)
		{
			placed++;
		}
	}

	var repairers = benchSpawnFixed(ME, "repair", X0, Y_REPAIR, COLS, +1, N_REPAIR, 2);
	var wounded = benchSpawnFixed(ME, "medium", X0, Y_WOUNDED, COLS, +1, N_WOUNDED, 2);
	hackNetOn();

	woundEveryone();
	setTimer("woundEveryone", REDAMAGE_MS);

	debug("movebench: perf_repair_facility, " + placed + " facilities, " + repairers.length
	      + " repairers, " + wounded.length + " wounded");
}
