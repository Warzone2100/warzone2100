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

#ifndef __INCLUDED_SRC_HUD_INTEL_INTELKIT_H__
#define __INCLUDED_SRC_HUD_INTEL_INTELKIT_H__

#include "intelmodel.h"

#include "../hudskin.h"

#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/button.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <nonstd/optional.hpp>

/// What the intelligence panel's views share: their text lines, dividers, item pictures, scrollbar and buttons
namespace intelkit
{

class IntelButton;

constexpr int BUTTON_HEIGHT = 26;

HudSkin &skin();
PIELIGHT alpha(PIELIGHT color, uint8_t a);
/// The baseline of a line of text whose top is at `top`
float baselineAt(WzText &text, int top);
/// One line, cut with an ellipsis to `maxWidth` (hudDrawTextLine). Returns the width it uses.
int drawLine(WzText &text, const std::string &string, iV_fonts font, int x, int top, int maxWidth, PIELIGHT color, bool alignRight = false);
std::string timeText(const HudIntelEntry &entry);
void drawUnreadDot(int cx, int cy);
/// A dark rounded well, for a picture
void drawWell(const WzRect &rect);
/// A research topic's model, still, fitted in `rect`
void drawResearchModel(const RESEARCH *research, const WzRect &rect);
/// An item's picture: its topic's model, or a play mark for a briefing
void drawThumb(const HudIntelEntry &entry, const WzRect &rect);
/// A divider inside the panel, muted
void drawDivider(int x0, int y0, int x1, int y1);
/// A thin track and handle, instead of the framed Classic scrollbar
void displayScrollbar(WIDGET *widget, UDWORD xOffset, UDWORD yOffset);
/// A position chip's texts: its number, and the digits whose height above one baseline centers every number alike
struct ChipText
{
	WzText number;
	WzText digits;
};
/// A player's position number on a chip of their color (as the connection panel's)
void drawPositionChip(ChipText &text, unsigned player, const WzRect &rect);
/// An all-capital headline ("TRANSPORT MISSION") in sentence case, others as written
std::string sentenceCase(const std::string &text);
/// An entry's kind ("Research · Cyborgs", "Briefing · Transport mission")
std::string entrySubtitle(const HudIntelEntry &entry);
/// A rounded label (a tag, a state): outlined, or filled. Returns its width.
int drawTag(WzText &text, const std::string &label, int x, int centerY, PIELIGHT color, bool filled, iV_fonts font = font_small);
/// A tag's width, from the text it's drawn with (as drawTag sets it)
int tagWidth(WzText &text, const std::string &label, iV_fonts font = font_small);
int tagWidth(const std::string &label, iV_fonts font = font_small);
/// A switch, on or off
void drawSwitch(const WzRect &rect, bool on);

/// A menu in the skin's popover look: an optional heading, items (an icon, a label, a detail on the right or a switch)
/// and separators. It opens under its anchor, over everything, and closes on a click outside it or Escape.
class IntelMenu : public WIDGET
{
public:
	struct Item
	{
		std::string icon;
		std::string label;
		std::string detail;
		bool enabled = true;
		bool danger = false;
		nonstd::optional<bool> toggle;        ///< a switch's state: the item stays open when clicked
		std::function<void ()> onChosen;
	};

	static std::shared_ptr<IntelMenu> make(int itemHeight, int minWidth);
	/// A heading, drawn by `draw` in its rectangle (the menu's width, `height`)
	void setHeading(int height, std::function<void (const WzRect &rect)> draw);
	void addItem(const Item &item);
	void addSeparator();
	bool empty() const { return rows.empty(); }
	/// Under `anchor`, its right edge on the anchor's (`alignRight`) or its left edge
	void open(const std::shared_ptr<WIDGET> &anchor, bool alignRight);
	void close();

	int32_t idealWidth() override;
	int32_t idealHeight() override;
	void geometryChanged() override;

protected:
	void display(int xOffset, int yOffset) override;

private:
	int itemHeight = 34;
	int minWidth = 0;
	int headingHeight = 0;
	std::function<void (const WzRect &rect)> drawHeading;
	std::vector<std::shared_ptr<WIDGET>> rows;
	std::weak_ptr<WIDGET> popover;
};

/// Whether an IntelMenu is open (it takes Escape)
bool menuIsOpen();
/// Closes the open IntelMenu (the panel is closing)
void closeMenu();

/// A question over a panel, which it covers (and takes the clicks of): a title, a line, and two buttons
class IntelConfirm : public WIDGET
{
public:
	static std::shared_ptr<IntelConfirm> make(const std::string &title, const std::string &message, const std::string &action, bool danger,
	                                          std::function<void ()> onConfirm);
	/// Closes it without the action (Escape, Cancel)
	void cancel();
	void confirm();
	void geometryChanged() override;

protected:
	void display(int xOffset, int yOffset) override;

private:
	std::string title;
	std::string message;
	std::function<void ()> onConfirm;
	std::shared_ptr<IntelButton> cancelButton;
	std::shared_ptr<IntelButton> actionButton;
	WzText titleText;
	WzText messageText;
};

/// A text button in the skin's card look, maybe with an icon (primary: the accent's)
class IntelButton : public W_BUTTON
{
public:
	static std::shared_ptr<IntelButton> make(const std::string &text, const std::string &icon, bool primary, int height, bool plain = false);

	void setLabel(const std::string &newLabel);
	int32_t idealWidth() override;
	int32_t idealHeight() override { return buttonHeight; }

	std::string label;
	std::string iconName;
	std::string trailingIcon;   ///< after the text (a menu's chevron)
	bool primary = false;
	bool plain = false;         ///< text only, no card (the order's menu)
	bool danger = false;        ///< the bad state's color (breaking an alliance)
	bool active = false;        ///< drawn selected (a menu's button while its menu is open, a toggle that's on)
	int buttonHeight = BUTTON_HEIGHT;

protected:
	int iconSize() const { return buttonHeight >= 30 ? 16 : 14; }
	void display(int xOffset, int yOffset) override;

private:
	WzText text;
};

}

#endif // __INCLUDED_SRC_HUD_INTEL_INTELKIT_H__
