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

#include "intelkit.h"

#include "lib/framework/frame.h"
#include "lib/framework/math_ext.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/netplay/netplay.h"
#include "lib/widget/popover.h"
#include "lib/widget/slider.h"

#include "../hudsettings.h"
#include "../hudtext.h"
#include "../hudwidgets.h"

#include "../../component.h"
#include "../../mapdisplay.h"
#include "../../multiplay.h"
#include "../../research.h"
#include "../../researchtree/researchtracks.h"

#include <algorithm>
#include <cctype>

namespace intelkit
{

HudSkin &skin()
{
	return currentHudSkin();
}

PIELIGHT alpha(PIELIGHT color, uint8_t a)
{
	color.byte.a = a;
	return color;
}

float baselineAt(WzText &text, int top)
{
	return static_cast<float>(top - text.aboveBase());
}

int drawLine(WzText &text, const std::string &string, iV_fonts font, int x, int top, int maxWidth, PIELIGHT color, bool alignRight)
{
	text.setText(WzString::fromUtf8(string), font, std::max(maxWidth, 1));
	return hudDrawTextLine(text, static_cast<float>(x), baselineAt(text, top), maxWidth, color, alignRight);
}

std::string timeText(const HudIntelEntry &entry)
{
	return entry.time.has_value() ? hudFormatDuration(*entry.time / GAME_TICKS_PER_SEC) : std::string();
}

void drawUnreadDot(int cx, int cy)
{
	const PIELIGHT accent = skin().colors().accent;
	pie_DrawRoundedBox(static_cast<float>(cx - 3), static_cast<float>(cy - 3), static_cast<float>(cx + 3), static_cast<float>(cy + 3), accent, 3.f);
}

void drawWell(const WzRect &rect)
{
	pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
	                   pal_RGBA(6, 9, 14, 230), 4.f, alpha(skin().colors().panelBorder, 150), 1.f);
}

void drawResearchModel(const RESEARCH *research, const WzRect &rect)
{
	if (research == nullptr)
	{
		return;
	}
	gfx_api::context::get().set_depth_range(0.f, 0.01f);
	renderResearchFittedToBuffer(const_cast<RESEARCH *>(research), static_cast<UDWORD>(rect.x() + rect.width() / 2),
	                             static_cast<UDWORD>(rect.y() + rect.height() / 2), static_cast<UDWORD>(std::min(rect.width(), rect.height())), false);
	pie_BeginInterface();
}

void drawThumb(const HudIntelEntry &entry, const WzRect &rect)
{
	drawWell(rect);
	if (entry.kind == HudIntelKind::Research)
	{
		drawResearchModel(entry.research, rect);
		return;
	}
	const int s = std::min(16, rect.height() - 8);
	skin().drawNamedIcon("replay-play", WzRect(rect.x() + (rect.width() - s) / 2, rect.y() + (rect.height() - s) / 2, s, s), skin().colors().accent);
}

void drawDivider(int x0, int y0, int x1, int y1)
{
	pie_UniTransBoxFill(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(x1), static_cast<float>(y1), pal_RGBA(120, 150, 190, 48));
}

void displayScrollbar(WIDGET *widget, UDWORD xOffset, UDWORD yOffset)
{
	auto slider = static_cast<W_SLIDER *>(widget);
	const int x0 = static_cast<int>(xOffset) + slider->x() + slider->width() - 4;
	const int y0 = static_cast<int>(yOffset) + slider->y();
	const int h = slider->height();
	pie_DrawRoundedBox(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(x0 + 3), static_cast<float>(y0 + h), pal_RGBA(255, 255, 255, 16), 1.5f);
	const int handle = std::max<int>(slider->barSize, 16);
	const int top = slider->numStops > 0 ? static_cast<int>(static_cast<int64_t>(h - handle) * slider->pos / slider->numStops) : 0;
	PIELIGHT color = skin().colors().textSecondary;
	color.byte.a = slider->isHighlighted() ? 200 : 130;
	pie_DrawRoundedBox(static_cast<float>(x0), static_cast<float>(y0 + top), static_cast<float>(x0 + 3), static_cast<float>(y0 + top + handle), color, 1.5f);
}

void drawPositionChip(ChipText &text, unsigned player, const WzRect &rect, float opacity)
{
	static const std::string POSITION_NAMES = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
	PIELIGHT color = pal_GetTeamColour(getPlayerColour(player));
	const auto lighter = [](uint8_t c) { return static_cast<uint8_t>(c + (255 - c) / 5); };
	const uint8_t alpha = static_cast<uint8_t>(255 * std::max(0.f, std::min(1.f, opacity)));
	color.byte.a = alpha;
	pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()), color, 3.f,
	                   pal_RGBA(lighter(color.byte.r), lighter(color.byte.g), lighter(color.byte.b), alpha), 1.f);
	const unsigned position = std::min<unsigned>(NetPlay.players[player].position, static_cast<unsigned>(POSITION_NAMES.size() - 1));
	const iV_fonts font = rect.height() >= 22 ? font_regular_bold : font_bar;
	text.number.setText(WzString::fromUtf8(std::string(1, POSITION_NAMES[position])), font);
	// (Numbers share the digits' baseline)
	text.digits.setText(WzString::fromUtf8("0123456789"), font);
	const int luminance = (color.byte.r * 299 + color.byte.g * 587 + color.byte.b * 114) / 1000;
	// (Half a pixel higher: centered numbers look low)
	text.number.renderCenteredOnInk(WzRect(rect.x(), rect.y(), rect.width(), rect.height() - 1), luminance > 128 ? pal_RGBA(10, 10, 10, alpha) : pal_RGBA(250, 250, 250, alpha), &text.digits);
}

int keyCapWidth(WzText &text, const std::string &key)
{
	text.setText(WzString::fromUtf8(key), font_small);
	return std::max(KEY_CAP_HEIGHT, text.width() + 8);
}

int drawKeyCap(WzText &text, const std::string &key, int x, int cy, KeyCapLook look)
{
	const HudColors &colors = skin().colors();
	const int w = keyCapWidth(text, key);
	const WzRect rect(x, cy - KEY_CAP_HEIGHT / 2, w, KEY_CAP_HEIGHT);
	PIELIGHT fill = pal_RGBA(0, 0, 0, 0);
	PIELIGHT border = alpha(colors.textSecondary, 150);
	PIELIGHT ink = colors.textPrimary;
	switch (look)
	{
	case KeyCapLook::Normal:
		break;
	case KeyCapLook::Lit:
		fill = alpha(colors.accent, 56);
		border = alpha(colors.accent, 210);
		break;
	case KeyCapLook::Dim:
		border = alpha(colors.textSecondary, 60);
		ink = alpha(colors.textSecondary, 120);
		break;
	}
	pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()), fill, 3.f,
	                   border, 1.f);
	// (Half a pixel higher: centered numbers look low)
	text.renderCenteredOnInk(WzRect(rect.x(), rect.y(), rect.width(), rect.height() - 1), ink);
	return w;
}

void drawSpinner(int cx, int cy, int radius, PIELIGHT color)
{
	constexpr int DOTS = 8;
	const float dot = std::max(2.f, radius / 2.5f);
	const int lead = hudReduceMotion() ? 0 : static_cast<int>((realTime / 110) % DOTS);
	for (int i = 0; i < DOTS; ++i)
	{
		const float angle = static_cast<float>(i) * 2.f * static_cast<float>(M_PI) / DOTS;
		const float x = cx + std::sin(angle) * (radius - dot / 2.f);
		const float y = cy - std::cos(angle) * (radius - dot / 2.f);
		const int age = (lead - i + DOTS) % DOTS;
		PIELIGHT c = color;
		c.byte.a = static_cast<uint8_t>(color.byte.a * (1.f - age * 0.11f));
		pie_DrawRoundedBox(x - dot / 2.f, y - dot / 2.f, x + dot / 2.f, y + dot / 2.f, c, dot / 2.f, pal_RGBA(0, 0, 0, 0), 0.f);
	}
}

void dimRect(const WzRect &rect, uint8_t amount)
{
	pie_UniTransBoxFill(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
	                    alpha(skin().colors().panelBackground, amount));
}

std::string sentenceCase(const std::string &text)
{
	// Only all-capital ASCII headlines ("TRANSPORT MISSION"): others are left as written
	const bool shouting = std::none_of(text.begin(), text.end(), [](unsigned char c) { return std::islower(c) || c >= 0x80; });
	if (!shouting || text.empty())
	{
		return text;
	}
	std::string result = text;
	std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
	return result;
}

std::string entrySubtitle(const HudIntelEntry &item)
{
	if (item.kind == HudIntelKind::Research)
	{
		// The progression it's in, or its kind of technology, as the detail panel's badge
		std::string group;
		if (item.research != nullptr)
		{
			group = item.research->category.isEmpty() ? researchTechCategoryName(*item.research).toUtf8() : item.research->category.toUtf8();
		}
		return group.empty() ? std::string(_("Research")) : astringf(_("Research · %s"), group.c_str());
	}
	const std::string kind = item.kind == HudIntelKind::Update ? _("Update") : _("Briefing");
	return item.headlineKind.empty() ? kind : kind + " · " + sentenceCase(item.headlineKind);
}

int tagWidth(WzText &text, const std::string &label, iV_fonts font)
{
	text.setText(WzString::fromUtf8(label), font);
	return text.width() + 12;
}

int tagWidth(const std::string &label, iV_fonts font)
{
	return static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(label), font)) + 12;
}

int drawTag(WzText &text, const std::string &label, int x, int centerY, PIELIGHT color, bool filled, iV_fonts font)
{
	const int w = tagWidth(text, label, font);
	const int h = static_cast<int>(text.lineSize()) + 4;
	const float top = static_cast<float>(centerY - h / 2);
	pie_DrawRoundedBox(static_cast<float>(x), top, static_cast<float>(x + w), top + static_cast<float>(h), alpha(color, filled ? 70 : 24), 3.f, alpha(color, 200), 1.f);
	text.render(static_cast<float>(x + 6), std::round(top + 2.f - text.aboveBase()), filled ? skin().colors().textPrimary : color);
	return w;
}

void drawSwitch(const WzRect &rect, bool on)
{
	const HudColors &colors = skin().colors();
	const float r = static_cast<float>(rect.height()) / 2.f;
	pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
	                   on ? alpha(colors.accent, 200) : pal_RGBA(10, 14, 20, 255), r, on ? colors.accent : alpha(colors.textSecondary, 150), 1.f);
	const float knob = static_cast<float>(rect.height() - 6);
	const float kx = on ? static_cast<float>(rect.right() - 3) - knob : static_cast<float>(rect.x() + 3);
	const float ky = static_cast<float>(rect.y() + 3);
	pie_DrawRoundedBox(kx, ky, kx + knob, ky + knob, on ? colors.textPrimary : colors.textSecondary, knob / 2.f);
}

namespace
{

bool menuOpen = false;
std::weak_ptr<WIDGET> openMenu;

/// A row of a menu: an item, or a separator
class IntelMenuRow : public W_BUTTON
{
public:
	IntelMenu::Item item;
	bool separator = false;
	std::weak_ptr<IntelMenu> menu;

	IntelMenuRow()
	{
		addOnClickHandler([](W_BUTTON &button) {
			auto &row = static_cast<IntelMenuRow &>(button);
			if (row.separator || !row.item.enabled)
			{
				return;
			}
			auto onChosen = row.item.onChosen;
			if (row.item.toggle.has_value())
			{
				row.item.toggle = !*row.item.toggle;
			}
			else if (auto strongMenu = row.menu.lock())
			{
				strongMenu->close();
			}
			if (onChosen)
			{
				widgScheduleTask(onChosen);
			}
		});
	}

	int32_t idealWidth() override
	{
		if (separator)
		{
			return 0;
		}
		const int labelWidth = static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(item.label), font_regular));
		const int detailWidth = item.toggle.has_value() ? 44 : (item.detail.empty() ? 0 : static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(item.detail), font_small)) + 24);
		return 12 + 18 + 12 + labelWidth + detailWidth + 14;
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		const HudColors &colors = skin().colors();
		if (separator)
		{
			drawDivider(rect.x() + 8, rect.y() + rect.height() / 2, rect.right() - 8, rect.y() + rect.height() / 2 + 1);
			return;
		}
		if (item.enabled && isHighlighted())
		{
			skin().drawCard(WzRect(rect.x() + 4, rect.y() + 1, rect.width() - 8, rect.height() - 2), HudCardState::Highlighted);
		}
		const PIELIGHT color = !item.enabled ? colors.stateDisabled : (item.danger ? colors.stateBad : colors.textPrimary);
		const int iconSize = 16;
		if (item.player.has_value())
		{
			drawPositionChip(chip, *item.player, WzRect(rect.x() + 12, rect.y() + (rect.height() - iconSize) / 2, iconSize, iconSize));
		}
		else if (!item.icon.empty())
		{
			skin().drawNamedIcon(item.icon, WzRect(rect.x() + 12, rect.y() + (rect.height() - iconSize) / 2, iconSize, iconSize),
			                     !item.enabled ? colors.stateDisabled : (item.danger ? colors.stateBad : colors.accent));
		}
		const int textX = rect.x() + 12 + iconSize + 12;
		int rightGutter = 12;
		if (item.toggle.has_value())
		{
			const WzRect toggleRect(rect.right() - 12 - 32, rect.y() + (rect.height() - 18) / 2, 32, 18);
			drawSwitch(toggleRect, *item.toggle);
			rightGutter = 12 + 32 + 12;
		}
		else if (!item.detail.empty())
		{
			detail.setText(WzString::fromUtf8(item.detail), font_small);
			const int detailWidth = detail.width();
			drawLine(detail, item.detail, font_small, rect.right() - 12, rect.y() + (rect.height() - static_cast<int>(iV_GetTextLineSize(font_small))) / 2,
			         detailWidth + 2, colors.textSecondary, true);
			rightGutter = 12 + detailWidth + 16;
		}
		drawLine(label, item.label, font_regular, textX, rect.y() + (rect.height() - static_cast<int>(iV_GetTextLineSize(font_regular))) / 2,
		         rect.right() - rightGutter - textX, color);
	}

private:
	WzText label;
	WzText detail;
	ChipText chip;
};

}

std::shared_ptr<IntelMenu> IntelMenu::make(int itemHeight, int minWidth)
{
	auto menu = std::make_shared<IntelMenu>();
	menu->itemHeight = itemHeight;
	menu->minWidth = minWidth;
	return menu;
}

void IntelMenu::setHeading(int height, std::function<void (const WzRect &rect)> draw)
{
	headingHeight = height;
	drawHeading = std::move(draw);
}

void IntelMenu::addItem(const Item &item)
{
	auto row = std::make_shared<IntelMenuRow>();
	row->item = item;
	row->menu = std::static_pointer_cast<IntelMenu>(shared_from_this());
	if (!item.enabled)
	{
		row->setState(WBUT_DISABLE);
	}
	rows.push_back(row);
	attach(row);
}

void IntelMenu::addSeparator()
{
	auto row = std::make_shared<IntelMenuRow>();
	row->separator = true;
	row->setTransparentToMouse(true);
	rows.push_back(row);
	attach(row);
}

int32_t IntelMenu::idealWidth()
{
	int32_t width = minWidth;
	for (auto &row : rows)
	{
		width = std::max(width, row->idealWidth());
	}
	return width;
}

int32_t IntelMenu::idealHeight()
{
	int32_t height = 6 + headingHeight;
	for (auto &row : rows)
	{
		height += static_cast<IntelMenuRow &>(*row).separator ? 9 : itemHeight;
	}
	return height + 6;
}

void IntelMenu::geometryChanged()
{
	int y0 = 6 + headingHeight;
	for (auto &row : rows)
	{
		const int h = static_cast<IntelMenuRow &>(*row).separator ? 9 : itemHeight;
		row->setGeometry(0, y0, width(), h);
		y0 += h;
	}
}

void IntelMenu::display(int xOffset, int yOffset)
{
	const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
	skin().drawPopoverFrame(rect);
	if (drawHeading && headingHeight > 0)
	{
		const WzRect heading(rect.x(), rect.y() + 6, rect.width(), headingHeight);
		drawHeading(heading);
		drawDivider(rect.x() + 1, heading.bottom() - 1, rect.right() - 1, heading.bottom());
	}
}

void IntelMenu::open(const std::shared_ptr<WIDGET> &anchor, bool alignRight)
{
	setGeometry(0, 0, idealWidth(), idealHeight());
	auto self = std::static_pointer_cast<IntelMenu>(shared_from_this());
	menuOpen = true;
	auto opened = PopoverWidget::makePopover(anchor, self, PopoverWidget::Style::Interactive,
	                                         alignRight ? PopoverWidget::Alignment::RightOfParent : PopoverWidget::Alignment::LeftOfParent, Vector2i(0, 2),
	                                         []() { menuOpen = false; });
	popover = opened;
	openMenu = self;
}

void IntelMenu::close()
{
	if (auto strongPopover = std::static_pointer_cast<PopoverWidget>(popover.lock()))
	{
		strongPopover->close();
	}
}

bool menuIsOpen()
{
	return menuOpen;
}

void closeMenu()
{
	if (auto menu = std::static_pointer_cast<IntelMenu>(openMenu.lock()))
	{
		menu->close();
	}
	openMenu.reset();
}

std::shared_ptr<IntelConfirm> IntelConfirm::make(const std::string &title, const std::string &message, const std::string &action, bool danger,
                                                 std::function<void ()> onConfirm)
{
	auto confirm = std::make_shared<IntelConfirm>();
	confirm->title = title;
	confirm->message = message;
	confirm->onConfirm = std::move(onConfirm);
	std::weak_ptr<IntelConfirm> weakConfirm = confirm;
	confirm->cancelButton = IntelButton::make(_("Cancel"), "", false, 30);
	confirm->cancelButton->addOnClickHandler([weakConfirm](W_BUTTON &) {
		if (auto strong = weakConfirm.lock())
		{
			strong->cancel();
		}
	});
	confirm->actionButton = IntelButton::make(action, danger ? "alliance-broken" : "", !danger, 30);
	confirm->actionButton->danger = danger;
	confirm->actionButton->addOnClickHandler([weakConfirm](W_BUTTON &) {
		if (auto strong = weakConfirm.lock())
		{
			strong->confirm();
		}
	});
	confirm->attach(confirm->cancelButton);
	confirm->attach(confirm->actionButton);
	return confirm;
}

constexpr int CONFIRM_WIDTH = 380;
constexpr int CONFIRM_HEIGHT = 140;

static WzRect confirmCard(const WzRect &area)
{
	const int w = std::min(CONFIRM_WIDTH, area.width() - 16);
	return WzRect(area.x() + (area.width() - w) / 2, area.y() + (area.height() - CONFIRM_HEIGHT) / 2, w, CONFIRM_HEIGHT);
}

void IntelConfirm::geometryChanged()
{
	const WzRect card = confirmCard(WzRect(0, 0, width(), height()));
	const int y0 = card.bottom() - 16 - 30;
	actionButton->setGeometry(card.right() - 16 - actionButton->idealWidth(), y0, actionButton->idealWidth(), 30);
	cancelButton->setGeometry(actionButton->x() - 8 - cancelButton->idealWidth(), y0, cancelButton->idealWidth(), 30);
}

void IntelConfirm::display(int xOffset, int yOffset)
{
	const WzRect area(xOffset + x(), yOffset + y(), width(), height());
	pie_UniTransBoxFill(static_cast<float>(area.x()), static_cast<float>(area.y()), static_cast<float>(area.right()), static_cast<float>(area.bottom()),
	                    pal_RGBA(0, 0, 0, 120));
	const WzRect card = confirmCard(area);
	skin().drawPopoverFrame(card);
	const HudColors &colors = skin().colors();
	titleText.setText(WzString::fromUtf8(title), font_regular_bold);
	titleText.render(static_cast<float>(card.x() + 16), baselineAt(titleText, card.y() + 16), colors.textPrimary);
	drawLine(messageText, message, font_small, card.x() + 16, card.y() + 46, card.width() - 32, colors.textSecondary);
}

void IntelConfirm::cancel()
{
	auto self = shared_from_this();
	widgScheduleTask([self]() {
		if (auto psParent = self->parent())
		{
			psParent->detach(self);
		}
	});
}

void IntelConfirm::confirm()
{
	auto action = onConfirm;
	cancel();
	if (action)
	{
		widgScheduleTask(action);
	}
}

std::shared_ptr<IntelButton> IntelButton::make(const std::string &text, const std::string &icon, bool primary, int height, bool plain)
{
	auto button = std::make_shared<IntelButton>();
	button->label = text;
	button->iconName = icon;
	button->primary = primary;
	button->plain = plain;
	button->buttonHeight = height;
	button->setGeometry(0, 0, button->idealWidth(), height);
	return button;
}

void IntelButton::setLabel(const std::string &newLabel)
{
	label = newLabel;
	setGeometry(x(), y(), idealWidth(), height());
}

int32_t IntelButton::idealWidth()
{
	const int iconWidth = iconName.empty() ? 0 : iconSize() + (label.empty() ? 0 : 6);
	text.setText(WzString::fromUtf8(label), font_small);
	const int textWidth = label.empty() ? 0 : text.width();
	const int trailingIconWidth = trailingIcon.empty() ? 0 : iconSize() + 4;
	return label.empty() ? buttonHeight : textWidth + iconWidth + trailingIconWidth + (plain ? 4 : 20);
}

void IntelButton::display(int xOffset, int yOffset)
{
	const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
	const bool disabled = (getState() & WBUT_DISABLE) != 0;
	const HudColors &colors = skin().colors();
	if (plain)
	{
	}
	else if (primary && !disabled)
	{
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
		                   alpha(colors.accent, isHighlighted() ? 110 : 70), skin().metrics().cardRadius, colors.accent, 1.f);
	}
	else
	{
		skin().drawCard(rect, disabled ? HudCardState::Disabled : (active ? HudCardState::Selected : (isHighlighted() ? HudCardState::Highlighted : HudCardState::Normal)));
	}
	PIELIGHT color = disabled ? alpha(colors.stateDisabled, 110) : ((plain && !isHighlighted()) ? colors.textSecondary : colors.textPrimary);
	if (danger && !disabled)
	{
		color = colors.stateBad;
	}
	int x0 = rect.x() + (label.empty() ? (rect.width() - iconSize()) / 2 : (plain ? 2 : 10));
	if (!iconName.empty())
	{
		const PIELIGHT iconColor = disabled ? alpha(colors.stateDisabled, 110) : (danger ? colors.stateBad : ((primary || active) ? colors.textPrimary : colors.textSecondary));
		skin().drawNamedIcon(iconName, WzRect(x0, rect.y() + (rect.height() - iconSize()) / 2, iconSize(), iconSize()), iconColor);
		x0 += iconSize() + 6;
	}
	if (!label.empty())
	{
		text.setText(WzString::fromUtf8(label), font_small);
		const float base = static_cast<float>(rect.y()) + (static_cast<float>(rect.height()) - text.lineSize()) / 2.f - text.aboveBase();
		text.render(static_cast<float>(x0), std::round(base), color);
		x0 += text.width() + 4;
	}
	if (!trailingIcon.empty())
	{
		skin().drawNamedIcon(trailingIcon, WzRect(x0, rect.y() + (rect.height() - iconSize()) / 2, iconSize(), iconSize()), plain ? color : colors.textSecondary);
	}
}

}
