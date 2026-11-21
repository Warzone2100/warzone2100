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
 *  The design screen (src/design.cpp) in the Modern skin: the same widgets in the same places, drawn as the skin's
 *  fields, cards and wells, with vector icons for the parts and the stats, and flat bars. Each function falls back to
 *  Classic's drawing (or returns false for design.cpp to draw it) when the skin doesn't draw the screen.
 */

#ifndef __INCLUDED_SRC_HUD_HUDDESIGN_H__
#define __INCLUDED_SRC_HUD_HUDDESIGN_H__

#include "lib/framework/types.h"

#include <string>

class WIDGET;
class WzText;

/// Whether the skin draws the design screen
bool hudDrawsDesignScreen();

/// Display functions: the part buttons, Store, Delete, and the parts list's Weapons and Systems buttons
void hudDesignDisplayButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
/// A part button whose part is still missing (Classic's flashing)
void hudDesignDisplayButtonFlash(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
/// A stat's icon (the labels before the bars)
void hudDesignDisplayStatIcon(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);
/// The design's name
void hudDesignDisplayEditBox(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset);

/// The part buttons' column (nothing) and the totals' form (a field). False: draw Classic's
bool hudDesignDrawForm(const WIDGET &form, int x, int y);
/// The 3D view's background: a well with a soft light under the droid
bool hudDesignDrawView(int x, int y, int width, int height);
/// Behind a part's stats: a well for its picture (brighter while pointed at)
bool hudDesignDrawStatForm(int x, int y, int width, int height, bool highlighted);

struct HudDesignBarMetrics
{
	int leftGutter = 0;
	int rightGutter = 0;
};
HudDesignBarMetrics hudDesignBarMetrics();

/// A stat's bar: the value before it, then the bar. While a part is pointed at (compared >= 0), what it changes in
/// the good or bad tone, and the difference after the bar
struct HudDesignBarLook
{
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	HudDesignBarMetrics metrics;   ///< (measured once, when the bar is made)
	float fraction = 0.f;   ///< (0-1)
	float compared = -1.f;  ///< (0-1, or < 0: nothing pointed at)
	bool better = false;
	std::string value;
	std::string delta;      ///< (empty: no difference)
};
bool hudDesignDrawStatBar(const HudDesignBarLook &look, WzText &valueText, WzText &deltaText);

#endif // __INCLUDED_SRC_HUD_HUDDESIGN_H__
