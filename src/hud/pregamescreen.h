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
 *  The screen before a networked game starts, in the Modern skin: one panel through its three stages (waiting for the
 *  players to load, team planning, the final countdown). Your team's plans in the team strategy view, with a teammate
 *  still loading in their row. The other teams as name pills (chips at the compact size). Without team planning,
 *  everyone in two columns. The chat bar under it. The Classic skin keeps src/screens/netpregamescreen.cpp's screen.
 */

#ifndef __INCLUDED_SRC_HUD_PREGAMESCREEN_H__
#define __INCLUDED_SRC_HUD_PREGAMESCREEN_H__

#include "lib/widget/widget.h"

#include <functional>
#include <memory>

/// Whether the skin draws the screen before a networked game starts
bool hudPregameScreenEnabled();
/// The screen, which calls `onClose` when the game starts (createGameStartScreen() registers it)
std::shared_ptr<W_SCREEN> hudMakePregameScreen(std::function<void ()> onClose);

#endif // __INCLUDED_SRC_HUD_PREGAMESCREEN_H__
