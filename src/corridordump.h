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
/** \file
 *  Headless corridor detector dump.
 *
 *  Loads a named map as a headless skirmish, runs the corridor detector on it
 *  once, writes the geometry as JSON and an ASCII map, and quits. Used to
 *  validate the detector on real maps before any coordination behaviour is
 *  built on top of it.
 */

#pragma once

#include <string>

/// Selects the map to dump and arms the run. The map name is a level name, ex.
/// "Sk-Rush". Returns false if the name is empty.
bool corridorDumpSelectMap(const char *mapName);

/// True while a corridor dump run is armed, so the skirmish launch loads the
/// requested map and the game loop dumps on the first update.
bool corridorDumpActive();

/// The level name to load for the dump.
const char *corridorDumpMapName();

/// Game-loop hook. On the first call after the map is loaded, builds the
/// corridor map, writes the dump, and quits.
void corridorDumpUpdate();
