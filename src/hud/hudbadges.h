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
 *  Object-row badges: the units of the current design still to make and the loop on factory jobs
 *  (in place of the Classic production run number), and on hold or waiting for power, inside the existing job buttons.
 */

#ifndef __INCLUDED_SRC_HUD_HUDBADGES_H__
#define __INCLUDED_SRC_HUD_HUDBADGES_H__

#include "lib/ivis_opengl/textdraw.h"

#include <string>

struct STRUCTURE;

/// A small pill with a count (ex. "×3") and, while looping, the loop icon (a loop sign where the skin draws no icons)
class HudCountBadge
{
public:
	void set(const std::string &text, bool loop);
	bool empty() const { return text.empty() && !loop; }
	int width();
	void draw(int x, int y);

private:
	void update();
	bool hasRenderedText() const { return !text.empty() || (loop && !renderedWithIcon); }

	std::string text;
	bool loop = false;
	WzText rendered;
	bool renderedWithIcon = false;
	bool dirty = true;
};

/// Over the job button of a factory or research facility (screen rectangle), when the module is on
void hudDrawJobBadges(STRUCTURE *psStruct, int x, int y, int height);
void hudReleaseJobBadges();

#endif // __INCLUDED_SRC_HUD_HUDBADGES_H__
