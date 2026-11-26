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

/** @file
 *  The status line (with the power line) and the selection readout, in the free zone Z1 where the level name and the
 *  game clock are drawn in Classic.
 */

#ifndef __INCLUDED_SRC_HUD_HUDSTATUSLINE_H__
#define __INCLUDED_SRC_HUD_HUDSTATUSLINE_H__

#include "hud.h"

#include <memory>

std::unique_ptr<HudModule> hudMakeStatusLine();
std::unique_ptr<HudModule> hudMakeSelectionReadout();

/// Where the status line goes, whether it is on or not (nothing when the zone is too narrow)
WzRect hudStatusLineRect();

/// Whether the status line shows the clock now, so the Classic level name and clock are not drawn (src/display3d.cpp)
bool hudStatusLineShown();

#endif // __INCLUDED_SRC_HUD_HUDSTATUSLINE_H__
