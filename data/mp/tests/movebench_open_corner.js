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

// The rush_corner head-on with the corridors stripped away. Same corner, same
// clusters, same relative spots: the mass's south-west corner matches
// Sk-Rush's, the clusters spawn at the identical coordinates and swap
// positions, but the flanking terrain is gone so nothing here detects as a
// corridor. Whatever corner scrum forms is the route layer's own doing, with
// no lane machinery involved on any backend.

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

	debug("movebench: open corner, " + (a.length + b.length) + " units");
}
