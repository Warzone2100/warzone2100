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

// Mass corner head-on across Sk-Rush's north-western quarter: two full
// clusters of cyborgs swap starting positions. The south-eastern cluster
// leaves its corridor at the bend near 10,33 while the north-western one cuts
// the same corner from the other side, turning left across the exit. Both
// flows want the inside line, so a cutting crowd builds against the emerging
// column at the corner and the side corridor entrance at 7,25 backs up behind
// it. Routes hug the shortest line here because nothing prices the corner,
// which makes this the acceptance case for shaping routes around open bends,
// at a scale where the failure cannot hide.

const ME = 0;
const N_UNITS = 136;   // clone wars!!, per side

var a = [];
var b = [];

function eventStartLevel()
{
	hackNetOff();
	benchEnable(ME, "cyborg");
	a = benchSpawnClones(ME, "cyborg", 24, 37, N_UNITS);
	b = benchSpawnClones(ME, "cyborg", 6, 13, N_UNITS);
	hackNetOn();

	benchOrderAllTo(a, 6, 13);
	benchOrderAllTo(b, 24, 37);

	debug("movebench: rush corner, " + (a.length + b.length) + " units");
}
