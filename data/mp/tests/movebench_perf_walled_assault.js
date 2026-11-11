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

// A large force ordered to attack a target sealed behind an unbroken wall.
//
// The goal is unreachable, so every attacker re-paths against a fully blocked
// destination every time its route fails, which is the pathfinding storm case.
// The wall ring also puts a structure on every line of fire into the base, so
// the defenders' target selection traces a blocked line for each candidate.

const ATTACKER = 0;
const DEFENDER = 1;

const N_ATTACK = 300;
const ATTACK_COLS = 30;
const Y_ATTACK = 54;

// The base occupies the ring x0..x1 by y0..y1 in the open ground south of the
// wall. Towers sit on every other interior tile, so the ring encloses 30.
const BASE_X0 = 25;
const BASE_X1 = 39;
const BASE_Y0 = 36;
const BASE_Y1 = 50;

function buildWallRing()
{
	for (var x = BASE_X0; x <= BASE_X1; x++)
	{
		addStructure("A0HardcreteMk1Wall", DEFENDER, x * 128, BASE_Y0 * 128);
		addStructure("A0HardcreteMk1Wall", DEFENDER, x * 128, BASE_Y1 * 128);
	}
	for (var y = BASE_Y0 + 1; y < BASE_Y1; y++)
	{
		addStructure("A0HardcreteMk1Wall", DEFENDER, BASE_X0 * 128, y * 128);
		addStructure("A0HardcreteMk1Wall", DEFENDER, BASE_X1 * 128, y * 128);
	}
}

function buildTowers()
{
	var towers = [];
	for (var y = BASE_Y0 + 3; y < BASE_Y1 - 2; y += 2)
	{
		for (var x = BASE_X0 + 2; x < BASE_X1 - 1; x += 2)
		{
			var s = addStructure("WallTower01", DEFENDER, x * 128, y * 128);
			if (s !== null)
			{
				towers.push(s);
			}
		}
	}
	return towers;
}

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ATTACKER, "medium");
	benchEnable(DEFENDER, "medium");
	buildWallRing();
	var towers = buildTowers();
	var attackers = benchSpawnBlock(ATTACKER, "medium", X_GAP - 15, Y_ATTACK, ATTACK_COLS, +1, N_ATTACK);
	hackNetOn();

	// One target for everyone, in the middle of the sealed base.
	var target = towers[Math.floor(towers.length / 2)];
	for (var i = 0; i < attackers.length; i++)
	{
		orderDroidObj(attackers[i], DORDER_ATTACK, target);
	}

	debug("movebench: perf_walled_assault, " + attackers.length + " attackers vs "
	      + towers.length + " walled towers");
}
