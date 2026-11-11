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

// Movement benchmark map. An L-shaped passage forming a right-angle corridor.
//
// A cluster stages in the north, runs south down the vertical leg, rounds the
// corner, and exits east.
//
// The map is cliff everywhere except the staging area, the two corridor legs
// and the goal area, so the corridor is the only route. Bounding the corridor
// with walls alone would leave units free to path around it.
//
// As on the corridor map, cliff tiles are raised so the passable tiles beside
// them inherit a slope.

const mapWidth = 64;
const mapHeight = 64;

const TEX_GROUND = 0;    // ttypes.ttp: passable
const TEX_CLIFF = 18;    // ttypes.ttp: TER_CLIFFFACE, blocking
const CLIFF_HEIGHT = 128;

const PASS = 2;          // corridor width, in tiles

// Staging area (north), where the cluster starts.
const STAGE_X0 = 22, STAGE_X1 = 42;
const STAGE_Y0 = 4,  STAGE_Y1 = 20;

// Vertical leg, running south out of the staging area into the corner.
const VLEG_X0 = 30, VLEG_X1 = VLEG_X0 + PASS;
const VLEG_Y0 = 20, VLEG_Y1 = 32;

// Horizontal leg, running east out of the corner to the goal area.
const HLEG_Y0 = 30, HLEG_Y1 = HLEG_Y0 + PASS;
const HLEG_X0 = 30, HLEG_X1 = 52;

// Goal area (east).
const GOAL_X0 = 50, GOAL_X1 = 62;
const GOAL_Y0 = 24, GOAL_Y1 = 40;

function inBox(x, y, x0, x1, y0, y1)
{
	return x >= x0 && x < x1 && y >= y0 && y < y1;
}

function isOpen(x, y)
{
	return inBox(x, y, STAGE_X0, STAGE_X1, STAGE_Y0, STAGE_Y1)
	    || inBox(x, y, VLEG_X0, VLEG_X1, VLEG_Y0, VLEG_Y1)
	    || inBox(x, y, HLEG_X0, HLEG_X1, HLEG_Y0, HLEG_Y1)
	    || inBox(x, y, GOAL_X0, GOAL_X1, GOAL_Y0, GOAL_Y1);
}

let texture = new Array(mapWidth * mapHeight);
let height = new Array(mapWidth * mapHeight);

for (let y = 0; y < mapHeight; y++)
{
	for (let x = 0; x < mapWidth; x++)
	{
		const i = mapWidth * y + x;
		if (isOpen(x, y))
		{
			texture[i] = TEX_GROUND;
			height[i] = 0;
		}
		else
		{
			texture[i] = TEX_CLIFF;
			height[i] = CLIFF_HEIGHT;
		}
	}
}

setMapData(mapWidth, mapHeight, texture, height, [], [], []);
