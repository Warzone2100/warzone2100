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
/** \file
	Where the in-game console goes, as pure functions of the screen size (no game dependencies).
*/

#ifndef __INCLUDED_SRC_HCI_INGAMECONSOLE_LAYOUT_H__
#define __INCLUDED_SRC_HCI_INGAMECONSOLE_LAYOUT_H__

#include <algorithm>

/// The widest the console's text gets (in logical pixels)
constexpr int CONSOLE_MAX_WIDTH = 900;
/// The space kept between the console and the screen's left and right edges
constexpr int CONSOLE_SIDE_MARGIN = 16;

/// The room kept below the expanded console for the chat box and quick chat
constexpr int CONSOLE_CHAT_RESERVE = 372;
/// The fewest lines the expanded console shows, however small the screen
constexpr int CONSOLE_MIN_LINES = 4;
/// The expanded console's padding, and its tabs above the lines
constexpr int CONSOLE_PANEL_PADDING = 4;
constexpr int CONSOLE_TAB_HEIGHT = 20;
constexpr int CONSOLE_TAB_GAP = 4;
constexpr int CONSOLE_PANEL_CHROME_HEIGHT = 2 * CONSOLE_PANEL_PADDING + CONSOLE_TAB_HEIGHT + CONSOLE_TAB_GAP;

struct ConsoleRect
{
	int x;
	int y;
	int width;
	int height;

	int bottom() const
	{
		return y + height;
	}
};

/// The width the console's lines wrap to
inline int consoleColumnWidth(int viewWidth)
{
	return std::max(0, std::min(CONSOLE_MAX_WIDTH, viewWidth - 2 * CONSOLE_SIDE_MARGIN));
}

/// Where the expanded console goes on a screen of this size, whether or not it's expanded now.
/// It starts at the top, a third of the screen tall at most, and leaves CONSOLE_CHAT_RESERVE below it,
/// so it shrinks to CONSOLE_MIN_LINES lines on the smallest screens.
inline ConsoleRect consoleExpandedRect(int viewWidth, int viewHeight, int linePitch)
{
	const int width = consoleColumnWidth(viewWidth);
	const int available = std::min(viewHeight / 3, viewHeight - CONSOLE_CHAT_RESERVE);
	const int lines = std::max(CONSOLE_MIN_LINES, (available - CONSOLE_PANEL_CHROME_HEIGHT) / std::max(linePitch, 1));
	return {(viewWidth - width) / 2, 0, width, CONSOLE_PANEL_CHROME_HEIGHT + lines * linePitch};
}

#endif // __INCLUDED_SRC_HCI_INGAMECONSOLE_LAYOUT_H__
