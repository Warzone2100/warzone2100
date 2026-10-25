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
 *  A list's pages as numbered segments that share the room they're given, in place of its page tabs (the stats panels,
 *  with panel filters). With touch or a gamepad, a tap opens a picker of large page buttons instead.
 */

#ifndef __INCLUDED_SRC_HUD_HUDPAGECONTROL_H__
#define __INCLUDED_SRC_HUD_HUDPAGECONTROL_H__

#include "lib/widget/widget.h"

#include <functional>
#include <memory>
#include <vector>

class ListTabWidget;
class PopoverWidget;

class HudPageControl : public WIDGET
{
public:
	static std::shared_ptr<HudPageControl> make(const std::shared_ptr<ListTabWidget> &list);

	size_t pages() const;
	/// The number of pages changed
	std::function<void (HudPageControl &)> onPagesChanged;

	/// The segments are as tall as the control (for touch), and drawn this tall in its middle (ex. as tall as the buttons
	/// beside it)
	void setVisualHeight(int height);
	int visualHeight() const { return drawnHeight; }
	/// One segment, the current page of how many ("2 / 3"), that opens the picker (Larger Touch Targets, in the Compact
	/// layout's header)
	void setSingleSegment(bool single);

protected:
	void geometryChanged() override;
	void display(int xOffset, int yOffset) override;

protected:
	HudPageControl() = default;

public:
	~HudPageControl() override;

private:
	class Segment;

	void layoutSegments();
	void pressed(const Segment &segment);
	void openPicker();

	std::weak_ptr<ListTabWidget> list;
	std::vector<std::shared_ptr<Segment>> segments;
	int frameWidth = 0;
	int drawnHeight = 13;
	bool singleSegment = false;
	std::shared_ptr<PopoverWidget> picker;
};

/// Clicks count as taps (the geometry dump's touch state)
void hudSetTouchInputForTests(bool on);
/// The last pointer input was a touch (or the touch state of the geometry dump)
bool hudPointerIsTouch();

#endif // __INCLUDED_SRC_HUD_HUDPAGECONTROL_H__
