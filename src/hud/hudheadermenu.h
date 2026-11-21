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
 *  The panel header's menu button: in the Compact layout with Larger Touch Targets, one button in place of a panel's
 *  small header buttons. Its menu lists them as large items, each drawn with the button it stands for and labeled with
 *  its tooltip. A tap on one does what a click on the button does.
 */

#ifndef __INCLUDED_SRC_HUD_HUDHEADERMENU_H__
#define __INCLUDED_SRC_HUD_HUDHEADERMENU_H__

#include "lib/widget/button.h"

#include <functional>
#include <memory>
#include <vector>

class PopoverWidget;

struct HudHeaderMenuEntry
{
	std::shared_ptr<W_BUTTON> button;
	/// (else the button's tooltip)
	WzString label;
	/// The button draws its own label (ex. View Tree): the item shows the label alone
	bool noIcon = false;
	/// What it does takes the player elsewhere (ex. placing the delivery point): the menu closes
	bool closesMenu = false;
	/// A count the button steps (ex. the factory's loops): − and + on the item, a right-click and a click on the button
	std::function<WzString ()> stepperValue;
	std::string stepperLessTip;
	std::string stepperMoreTip;
};

class HudHeaderMenuButton : public W_BUTTON
{
public:
	static std::shared_ptr<HudHeaderMenuButton> make(std::function<std::vector<HudHeaderMenuEntry> ()> entries);
	~HudHeaderMenuButton() override;

	void display(int xOffset, int yOffset) override;
	void closeMenu();

protected:
	HudHeaderMenuButton() = default;

private:
	void openMenu();

	std::function<std::vector<HudHeaderMenuEntry> ()> entries;
	std::shared_ptr<PopoverWidget> menu;
};

/// The header controls' height with Larger Touch Targets (the menu button, the page control, the close button)
constexpr int HUD_TOUCH_HEADER_CONTROL_HEIGHT = 18;
/// The menu button's width
constexpr int HUD_HEADER_MENU_BUTTON_WIDTH = 28;

#endif // __INCLUDED_SRC_HUD_HUDHEADERMENU_H__
