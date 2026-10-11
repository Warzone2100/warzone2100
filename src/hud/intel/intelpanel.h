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

#ifndef __INCLUDED_SRC_HUD_INTEL_INTELPANEL_H__
#define __INCLUDED_SRC_HUD_INTEL_INTELPANEL_H__

#include <string>

struct MESSAGE;

/// The HUD's intelligence screen: a panel over the paused, dimmed world, in place of the Classic screen
/// (src/intelmap.cpp) when the skin draws it (HudSkin::drawsIntelligenceScreen), in single player.
/// Its layout is compact (the panel filling the screen) when the full panel doesn't fit.

/// Whether the intelligence button opens this screen
bool hudIntelScreenEnabled();
/// Opens it on `message` (when given), playing its videos at once when `play`, and pauses the game (single player)
void hudIntelScreenOpen(MESSAGE *message, bool play);
/// Closes it and lets the game go on
void hudIntelScreenClose();
bool hudIntelScreenIsOpen();

/// For the HUD geometry dump: "select=<row>", "pinned=1", "filter=<all|briefings|research|unread>",
/// "order=<newest|kind>", "search=<text>", "tree=1" (the selected report in the research tree)
bool hudIntelScreenApplyForTests(const std::string &setting);

#endif // __INCLUDED_SRC_HUD_INTEL_INTELPANEL_H__
