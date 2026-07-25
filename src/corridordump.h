// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
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
