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
	additional terms, or (at your option) any later version, as published
	by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
	the top-level directory of the source distribution. This notice must
	be kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef __INCLUDED_SRC_HUD_HUDGEOMETRYDUMP_H__
#define __INCLUDED_SRC_HUD_HUDGEOMETRYDUMP_H__

#include <string>

/// --hud-geometry-dump=<file>[,<game time ms>[,<state>...]] (test only, tests/hud/README.md): at the first capture
/// point at or after the game time (default 3000), game time stops and the HUD is put in each state in turn (by
/// default, the panel states). For each, every visible widget's screen rectangle, z-order and name, and which widget a
/// click at each point of the screen reaches, are written to <file> as JSON, and a screenshot is saved to
/// hudtest/<state>.png in the write directory. Then the game quits.
/// Returns false on a bad value.
bool hudSetGeometryDump(const std::string &spec);
bool hudGeometryDumpActive();

/// Hook called by gameLoop() at the capture point (see gamestate_capture.h)
void hudGeometryDumpAfterMessages();
/// Hook called by renderLoop() each frame, after the interface's refresh
void hudGeometryDumpFrame();

#endif // __INCLUDED_SRC_HUD_HUDGEOMETRYDUMP_H__
