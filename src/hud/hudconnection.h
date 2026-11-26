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

#ifndef __INCLUDED_SRC_HUD_HUDCONNECTION_H__
#define __INCLUDED_SRC_HUD_HUDCONNECTION_H__

/// The multiplayer connection warnings as a panel in the right column (hudrightcolumn.h), for a skin that draws them so
/// (HudSkin::drawsConnectionPanel): a row per problem (a player dropped or left, out of sync, the game waiting for a
/// player) with its icon and the players' chips. It names them while hovered, after a tap, and for a few seconds when
/// a player drops, leaves or desyncs. Waiting is muted while the wait is short, and turns amber as it goes on.
///
/// Each frame: shows, updates or hides the panel
void hudUpdateConnectionPanel();
/// Removes the panel (the warnings are drawn the Classic way)
void hudRemoveConnectionPanel();

/// For the HUD geometry dump: an example of each problem, set every frame (open: the panel named), or none
void hudShowConnectionWarningsForTests(bool show, bool open);
/// Sets the example's statuses while it is shown (each frame, before the warnings are drawn)
void hudApplyConnectionWarningsForTests();

#endif // __INCLUDED_SRC_HUD_HUDCONNECTION_H__
