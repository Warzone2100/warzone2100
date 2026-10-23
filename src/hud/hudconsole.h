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
 *  The in-game console (src/hci/ingameconsole.cpp) in the Modern skin: its messages as chat lines (the sender's
 *  position chip, the Team tag, the name in bold, the text wrapped after them), the feed outlined over the 3D view,
 *  and the history as a card whose bottom row is the chat bar.
 */

#ifndef __INCLUDED_SRC_HUD_HUDCONSOLE_H__
#define __INCLUDED_SRC_HUD_HUDCONSOLE_H__

#include "lib/framework/wzstring.h"
#include "lib/ivis_opengl/pietypes.h"
#include "lib/ivis_opengl/textdraw.h"

#include "../hci/ingameconsole_layout.h"
#include "intel/intelkit.h"

#include <string>

/// Whether the skin draws the console
bool hudConsoleSkinned();

/// The expanded console in the skin: the history's lines over the chat bar's row, without Classic's tabs
/// (on a narrow screen it stops short of the options button)
ConsoleRect hudConsoleExpandedRect(const ConsoleRect &classic);
/// Where the console's lines from `x` at `y` (on the screen) end, short of the options button when they would reach it
int hudConsoleRight(int x, int y, int right);
/// The padding around the history's lines
constexpr int HUD_CONSOLE_PADDING = 8;
/// The chat bar's row, at the bottom of the expanded console
int hudConsoleChatRowHeight();

/// Splits a console line ("[01:18] Kestrel (Allies): gl hf", the time when the console's timestamps are on) into the
/// sender's name, the channel as the console writes it, and the text (a line it can't split stays whole as the text)
void hudParseChatLine(const std::string &raw, int sender, std::string &name, std::string &channel, std::string &text);

/// What a message's first line starts with, as a chat line: the sender's chip, the Team tag, the name in bold
struct HudConsolePrefix
{
	std::string time;   ///< (the console's timestamp, when they're on)
	int sender = -1;    ///< (< 0: the console's own message, without a chip)
	std::string tag;    ///< (Team, or a channel the console names, empty for everyone)
	bool team = false;
	std::string name;   ///< (empty: no name)
	int width = 0;      ///< Where the text starts
};
/// Splits a message, as the console wrote it, into its prefix and its text (`pitch`: the line's height)
HudConsolePrefix hudConsolePrefix(const WzString &message, int player, bool team, int pitch, WzString &text);

/// The texts a prefix is drawn with
struct HudConsolePrefixTexts
{
	WzText time;
	intelkit::ChipText chip;
	WzText tag;
	WzText name;
};
/// Draws a prefix on a line whose top is `y`. `overlay`: over the 3D view (the feed), outlined as the skin's overlay text
void hudConsoleDrawPrefix(HudConsolePrefixTexts &texts, const HudConsolePrefix &prefix, int x, int y, int pitch, float opacity, bool overlay);
/// A message's text color: a player's, or the console's own (muted)
PIELIGHT hudConsoleTextColor(int player, float opacity);

#endif // __INCLUDED_SRC_HUD_HUDCONSOLE_H__
