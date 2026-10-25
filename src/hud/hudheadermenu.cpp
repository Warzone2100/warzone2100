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

#include "hudheadermenu.h"
#include "hudskin.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/popover.h"

#include <algorithm>

namespace
{

constexpr int ITEM_HEIGHT = 36;
constexpr int ITEM_WIDTH = 252;
constexpr int ITEM_PAD = 8;
constexpr int ICON_COLUMN_WIDTH = 30;
constexpr int MENU_PADDING = 4;
constexpr int MENU_GAP = 2;
constexpr int STEP_WIDTH = 28;
constexpr int STEP_HEIGHT = 26;
constexpr int STEP_VALUE_WIDTH = 32;

/// As a click on the button itself
void clickButton(W_BUTTON &button, WIDGET_KEY key)
{
	W_CONTEXT context = W_CONTEXT::ZeroContext();
	button.clicked(&context, key);
	button.released(&context, key);
}

/// The stepper's − or +
class StepButton : public W_BUTTON
{
public:
	explicit StepButton(bool plus_) : plus(plus_) {}

	void display(int xOffset, int yOffset) override
	{
		HudSkin &skin = currentHudSkin();
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		skin.drawChip(rect, HudTone::Neutral, (getState() & (WBUT_DOWN | WBUT_HIGHLIGHT)) != 0);
		const PIELIGHT ink = skin.colors().textPrimary;
		const int cx = rect.x() + rect.width() / 2;
		const int cy = rect.y() + rect.height() / 2;
		pie_BoxFill(cx - 5, cy - 1, cx + 5, cy + 1, ink);
		if (plus)
		{
			pie_BoxFill(cx - 1, cy - 5, cx + 1, cy + 5, ink);
		}
	}

private:
	bool plus;
};

class MenuFrame : public WIDGET
{
protected:
	void display(int xOffset, int yOffset) override
	{
		currentHudSkin().drawPopoverFrame(WzRect(xOffset + x(), yOffset + y(), width(), height()));
	}
};

/// One of the header's buttons, as a large item: the button drawn at its own size on the left, its label after it
class MenuItem : public W_BUTTON
{
public:
	explicit MenuItem(const HudHeaderMenuEntry &entry_) : entry(entry_) {}

	void addStepper()
	{
		if (!entry.stepperValue || entry.button == nullptr)
		{
			return;
		}
		std::weak_ptr<W_BUTTON> weakButton = entry.button;
		for (bool plus : {false, true})
		{
			auto step = std::make_shared<StepButton>(plus);
			step->setTip(plus ? entry.stepperMoreTip : entry.stepperLessTip);
			step->addOnClickHandler([weakButton, plus](W_BUTTON &) {
				if (auto button = weakButton.lock())
				{
					clickButton(*button, plus ? WKEY_PRIMARY : WKEY_SECONDARY);
				}
			});
			attach(step);
			(plus ? plusButton : minusButton) = step;
		}
	}

	void geometryChanged() override
	{
		if (minusButton && plusButton)
		{
			const int y = (height() - STEP_HEIGHT) / 2;
			plusButton->setGeometry(width() - ITEM_PAD - STEP_WIDTH, y, STEP_WIDTH, STEP_HEIGHT);
			minusButton->setGeometry(plusButton->x() - STEP_VALUE_WIDTH - STEP_WIDTH, y, STEP_WIDTH, STEP_HEIGHT);
		}
	}

	void display(int xOffset, int yOffset) override
	{
		auto button = entry.button;
		HudSkin &skin = currentHudSkin();
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		const bool active = (getState() & (WBUT_DOWN | WBUT_HIGHLIGHT)) != 0;
		if (active)
		{
			skin.drawCard(rect, HudCardState::Highlighted);
		}
		int textX = rect.x() + ITEM_PAD;
		if (button != nullptr && !entry.noIcon)
		{
			// (its own look in each skin, and its state: on or off, its count)
			const int iconX = rect.x() + ITEM_PAD + (ICON_COLUMN_WIDTH - button->width()) / 2;
			const int iconY = rect.y() + (rect.height() - button->height()) / 2;
			button->display(iconX - button->x(), iconY - button->y());
			textX += ICON_COLUMN_WIDTH + ITEM_PAD;
		}
		std::string tip = button != nullptr ? button->getTip() : std::string();
		// (its first line: the others are about the mouse, ex. "Right-click to ...")
		tip = tip.substr(0, tip.find('\n'));
		const WzString label = !entry.label.isEmpty() ? entry.label : WzString::fromUtf8(tip);
		const int textRight = minusButton ? rect.x() + minusButton->x() - ITEM_PAD : rect.right() - ITEM_PAD;
		text.setText(label, font_regular, std::max(10, textRight - textX));
		const int baseline = rect.y() + (rect.height() - text.lineSize()) / 2 - text.aboveBase();
		text.render(static_cast<float>(textX), static_cast<float>(baseline), skin.colors().textPrimary);
		if (minusButton && plusButton)
		{
			value.setText(entry.stepperValue(), font_regular_bold);
			const int valueX = rect.x() + minusButton->x() + minusButton->width() + (STEP_VALUE_WIDTH - value.width()) / 2;
			value.render(static_cast<float>(valueX), static_cast<float>(baseline), skin.colors().textPrimary);
		}
	}

	HudHeaderMenuEntry entry;

private:
	WzText text;
	WzText value;
	std::shared_ptr<StepButton> minusButton;
	std::shared_ptr<StepButton> plusButton;
};

}

std::shared_ptr<HudHeaderMenuButton> HudHeaderMenuButton::make(std::function<std::vector<HudHeaderMenuEntry> ()> entries)
{
	class make_shared_enabler : public HudHeaderMenuButton {};
	auto button = std::make_shared<make_shared_enabler>();
	button->entries = std::move(entries);
	button->setTip(_("More"));
	std::weak_ptr<HudHeaderMenuButton> weakButton = button;
	button->addOnClickHandler([weakButton](W_BUTTON &) {
		if (auto self = weakButton.lock())
		{
			if (self->menu != nullptr)
			{
				self->closeMenu();
			}
			else
			{
				self->openMenu();
			}
		}
	});
	return button;
}

HudHeaderMenuButton::~HudHeaderMenuButton()
{
	closeMenu();
}

void HudHeaderMenuButton::display(int xOffset, int yOffset)
{
	HudSkin &skin = currentHudSkin();
	const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
	const bool active = menu != nullptr || (getState() & (WBUT_DOWN | WBUT_HIGHLIGHT)) != 0;
	skin.drawChip(rect, HudTone::Neutral, active);
	const int icon = std::min(rect.width(), rect.height()) - 4;
	if (!skin.drawNamedIcon("more", WzRect(rect.x() + (rect.width() - icon) / 2, rect.y() + (rect.height() - icon) / 2, icon, icon), skin.colors().textPrimary))
	{
		// (three dots, where the skin has no icons)
		const int dotY = rect.y() + rect.height() / 2 - 1;
		for (int i = -1; i <= 1; ++i)
		{
			const int dotX = rect.x() + rect.width() / 2 + i * 5 - 1;
			pie_BoxFill(dotX, dotY, dotX + 2, dotY + 2, skin.colors().textPrimary);
		}
	}
}

void HudHeaderMenuButton::openMenu()
{
	const std::vector<HudHeaderMenuEntry> list = entries ? entries() : std::vector<HudHeaderMenuEntry>();
	if (list.empty())
	{
		return;
	}
	auto frame = std::make_shared<MenuFrame>();
	const int count = static_cast<int>(list.size());
	frame->setGeometry(0, 0, ITEM_WIDTH + MENU_PADDING * 2, MENU_PADDING * 2 + count * ITEM_HEIGHT + (count - 1) * MENU_GAP);
	std::weak_ptr<HudHeaderMenuButton> weakSelf = std::static_pointer_cast<HudHeaderMenuButton>(shared_from_this());
	for (int i = 0; i < count; ++i)
	{
		auto item = std::make_shared<MenuItem>(list[i]);
		frame->attach(item);
		item->addStepper();
		item->setGeometry(MENU_PADDING, MENU_PADDING + i * (ITEM_HEIGHT + MENU_GAP), ITEM_WIDTH, ITEM_HEIGHT);
		item->addOnClickHandler([weakSelf](W_BUTTON &clicked) {
			const HudHeaderMenuEntry entry = static_cast<MenuItem &>(clicked).entry;
			auto self = weakSelf.lock();
			if (entry.closesMenu && self != nullptr)
			{
				self->closeMenu();
			}
			if (entry.button != nullptr)
			{
				clickButton(*entry.button, WKEY_PRIMARY);
			}
		});
	}
	menu = PopoverWidget::makePopover(shared_from_this(), frame, PopoverWidget::Style::Interactive, PopoverWidget::Alignment::LeftOfParent,
	                                  Vector2i(0, 2), [weakSelf]() {
		if (auto self = weakSelf.lock())
		{
			self->menu.reset();
		}
	});
}

void HudHeaderMenuButton::closeMenu()
{
	if (menu != nullptr)
	{
		auto closing = menu;
		menu.reset();
		closing->close();
	}
}
