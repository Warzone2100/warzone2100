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
 *  Small controls the HUD modules share, drawn by the skin.
 */

#ifndef __INCLUDED_SRC_HUD_HUDWIDGETS_H__
#define __INCLUDED_SRC_HUD_HUDWIDGETS_H__

#include "hudskin.h"

#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/button.h"
#include "lib/widget/form.h"
#include "lib/widget/popovermenu.h"

#include <functional>
#include <memory>

/// The smallest a clickable HUD addition may be, in logical pixels
constexpr int HUD_MIN_HIT_SIZE = 16;
constexpr int HUD_CHIP_HEIGHT = 16;

/// A toggle or action: a filter chip, a type chip, an idle counter
class HudChip : public W_BUTTON
{
public:
	static std::shared_ptr<HudChip> make(const WzString &text, const std::string &tip = std::string());

	void setText(const WzString &text);
	const WzString &text() const { return label; }
	/// Shown instead of the text by a skin that draws icons (HudSkin::hasNamedIcon)
	void setIcon(const std::string &iconName);
	void setActive(bool active);
	bool isActive() const { return active; }
	void setTone(HudTone tone);
	/// Drawn as a card (like the skin's other buttons) instead of a chip
	void setCardShape(bool cardShape);
	int32_t idealWidth() override;
	int32_t idealHeight() override;

	/// Shift-clicks (and right-clicks) as well as clicks
	std::function<void (HudChip &chip, bool shift)> onPressed;

protected:
	void display(int xOffset, int yOffset) override;
	void released(W_CONTEXT *psContext, WIDGET_KEY key) override;

private:
	/// A label's leading or trailing symbol is centered on its own drawn pixels: symbol fonts sit lower than the text's
	struct Piece
	{
		WzText text;
		int x = 0;
		bool symbol = false;
	};

	bool showsIcon();

	WzString label;
	std::string icon;
	WzText cachedText;
	std::vector<Piece> pieces;
	bool active = false;
	bool cardShape = false;
	HudTone tone = HudTone::Neutral;
};

/// An invisible form that holds a module's widgets in its free zone, with help for the help overlay
class HudZoneForm : public W_FORM
{
public:
	HudZoneForm();
	void setHelp(optional<WidgetHelp> newHelp) override { help = newHelp; }
	WidgetHelp const *getHelp() const override { return help.has_value() ? &help.value() : nullptr; }

private:
	optional<WidgetHelp> help;
};

/// One line of text over the 3D view, drawn by the skin (with the text outline option)
class HudOverlayText : public WIDGET
{
public:
	void setText(const WzString &text, PIELIGHT color);
	int32_t idealWidth() override;
	int32_t idealHeight() override;
	iV_fonts font = font_small;

protected:
	void display(int xOffset, int yOffset) override;

private:
	WzString string;
	PIELIGHT textColor = WZCOL_TEXT_BRIGHT;
	WzText cachedText;
};

/// Draws a line of text at `baseline`, cut to `maxWidth` with an ellipsis when it doesn't fit (WzText::setText()).
/// Returns the width it uses.
int hudDrawTextLine(WzText &text, float x, float baseline, int maxWidth, PIELIGHT color, bool alignRight = false);

/// Sizes a popover menu to its items (at most the screen's height) and opens it under `anchor`, left-aligned with it
void hudOpenMenu(const std::shared_ptr<PopoverMenuWidget> &menu, const std::shared_ptr<WIDGET> &anchor);

/// Lays widgets out left to right at their ideal widths, starting a new line when one would not fit.
/// Returns the height used.
int hudFlowLayout(const std::vector<std::shared_ptr<WIDGET>> &widgets, int x, int y, int width, int gap, int lineHeight);

/// A player's team color, as a square with a light edge
void hudDrawPlayerMark(int x, int y, int size, unsigned player);

#endif // __INCLUDED_SRC_HUD_HUDWIDGETS_H__
