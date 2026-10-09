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

#include "hudwidgets.h"

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/piepalette.h"

#include "../component.h"

#include <algorithm>
#include <cmath>

constexpr int CHIP_PADDING = 6;
constexpr int CHIP_ICON_INSET = 3;

static bool shiftDown()
{
	return keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT);
}

std::shared_ptr<HudChip> HudChip::make(const WzString &text, const std::string &tip)
{
	auto chip = std::make_shared<HudChip>();
	chip->setText(text);
	if (!tip.empty())
	{
		chip->setTip(tip);
	}
	chip->style |= WBUT_SECONDARY;
	return chip;
}

void HudChip::setText(const WzString &text)
{
	if (text == label)
	{
		return;
	}
	label = text;
	cachedText.setText(label, font_small);
	pieces.clear();

	const std::string utf8 = label.toUtf8();
	auto isSymbol = [](const std::string &word) {
		return !word.empty() && static_cast<unsigned char>(word[0]) >= 0x80 && WzString::fromUtf8(word).length() == 1;
	};
	const size_t firstSpace = utf8.find(' ');
	const size_t lastSpace = utf8.rfind(' ');
	size_t bodyStart = 0;
	size_t bodyEnd = utf8.size();
	if (isSymbol(utf8.substr(0, firstSpace)))
	{
		bodyStart = (firstSpace == std::string::npos) ? utf8.size() : firstSpace + 1;
	}
	if (lastSpace != std::string::npos && lastSpace + 1 > bodyStart && isSymbol(utf8.substr(lastSpace + 1)))
	{
		bodyEnd = lastSpace;
	}
	if (bodyStart == 0 && bodyEnd == utf8.size())
	{
		return;
	}
	auto add = [this, &utf8](size_t start, size_t end, bool symbol) {
		if (start >= end)
		{
			return;
		}
		Piece piece;
		piece.text.setText(WzString::fromUtf8(utf8.substr(start, end - start)), font_small);
		piece.x = static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(utf8.substr(0, start)), font_small));
		piece.symbol = symbol;
		pieces.push_back(std::move(piece));
	};
	add(0, bodyStart == 0 ? 0 : std::min(firstSpace, utf8.size()), true);
	add(bodyStart, bodyEnd, false);
	add(bodyEnd == utf8.size() ? utf8.size() : lastSpace + 1, utf8.size(), true);
}

void HudChip::setIcon(const std::string &iconName)
{
	icon = iconName;
}

bool HudChip::showsIcon()
{
	return !icon.empty() && currentHudSkin().hasNamedIcon(icon);
}

void HudChip::setActive(bool newActive)
{
	active = newActive;
}

void HudChip::setTone(HudTone newTone)
{
	tone = newTone;
}

void HudChip::setCardShape(bool newCardShape)
{
	cardShape = newCardShape;
}

int32_t HudChip::idealWidth()
{
	if (showsIcon())
	{
		return HUD_CHIP_HEIGHT + CHIP_PADDING;
	}
	return std::max(HUD_MIN_HIT_SIZE, cachedText.width() + CHIP_PADDING * 2);
}

int32_t HudChip::idealHeight()
{
	return HUD_CHIP_HEIGHT;
}

void HudChip::display(int xOffset, int yOffset)
{
	const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
	const bool hovered = (getState() & WBUT_HIGHLIGHT) != 0;
	auto &skin = currentHudSkin();
	if (cardShape)
	{
		skin.drawCard(rect, active ? HudCardState::Selected : (hovered ? HudCardState::Highlighted : HudCardState::Normal));
	}
	else
	{
		skin.drawChip(rect, active ? HudTone::Accent : tone, active || hovered);
	}
	const PIELIGHT color = active ? skin.colors().textPrimary : (tone == HudTone::Neutral ? skin.colors().textSecondary : skin.toneColor(tone));
	const PIELIGHT ink = hovered && !active ? skin.colors().textPrimary : color;
	if (showsIcon())
	{
		const int size = rect.height() - CHIP_ICON_INSET * 2;
		if (skin.drawNamedIcon(icon, WzRect(rect.x() + (rect.width() - size) / 2, rect.y() + CHIP_ICON_INSET, size, size), ink))
		{
			return;
		}
	}
	const float textX = cachedText.inkCenteredX(rect);
	const int textY = rect.y() + (rect.height() - cachedText.lineSize()) / 2 - cachedText.aboveBase();
	if (pieces.empty())
	{
		cachedText.render(textX, static_cast<float>(textY), ink);
		return;
	}
	for (auto &piece : pieces)
	{
		const float y = piece.symbol ? std::round(static_cast<float>(rect.y()) + (static_cast<float>(rect.height()) - piece.text.inkHeight()) / 2.f - piece.text.inkTop())
			: static_cast<float>(textY);
		piece.text.render(textX + static_cast<float>(piece.x), y, ink);
	}
}

void HudChip::released(W_CONTEXT *psContext, WIDGET_KEY key)
{
	const bool wasDown = (state & WBUT_DOWN) != 0;
	W_BUTTON::released(psContext, key);
	if (wasDown && onPressed)
	{
		onPressed(*this, shiftDown() || key == WKEY_SECONDARY);
	}
}

HudZoneForm::HudZoneForm()
{
	style |= WFORM_INVISIBLE;
	setTransparentToMouse(true);
}

void HudOverlayText::setText(const WzString &text, PIELIGHT color)
{
	textColor = color;
	if (text == string)
	{
		return;
	}
	string = text;
	cachedText.setText(string, font);
}

int32_t HudOverlayText::idealWidth()
{
	return cachedText.width();
}

int32_t HudOverlayText::idealHeight()
{
	return iV_GetTextLineSize(font);
}

void HudOverlayText::display(int xOffset, int yOffset)
{
	if (string.isEmpty())
	{
		return;
	}
	currentHudSkin().drawOverlayText(cachedText, static_cast<float>(xOffset + x()), static_cast<float>(yOffset + y() - cachedText.aboveBase()), textColor);
}

int hudDrawTextLine(WzText &text, float x, float baseline, int maxWidth, PIELIGHT color, bool alignRight)
{
	text.setText(text.getText(), text.getFontID(), std::max(maxWidth, 1));
	text.render(alignRight ? x - static_cast<float>(text.width()) : x, baseline, color);
	return text.width();
}

int hudFlowLayout(const std::vector<std::shared_ptr<WIDGET>> &widgets, int x, int y, int width, int gap, int lineHeight)
{
	int cursorX = 0;
	int line = 0;
	bool any = false;
	for (const auto &widget : widgets)
	{
		if (widget == nullptr || !widget->visible())
		{
			continue;
		}
		const int w = std::min(width, widget->idealWidth());
		if (cursorX > 0 && cursorX + w > width)
		{
			cursorX = 0;
			++line;
		}
		widget->setGeometry(x + cursorX, y + line * (lineHeight + gap), w, lineHeight);
		cursorX += w + gap;
		any = true;
	}
	return any ? (line + 1) * lineHeight + line * gap : 0;
}

void hudDrawPlayerMark(int x, int y, int size, unsigned player)
{
	pie_UniTransBoxFill(static_cast<float>(x - 1), static_cast<float>(y - 1), static_cast<float>(x + size + 1), static_cast<float>(y + size + 1), pal_RGBA(200, 210, 225, 150));
	pie_BoxFill(x, y, x + size, y + size, pal_GetTeamColour(getPlayerColour(player)));
}

void hudOpenMenu(const std::shared_ptr<PopoverMenuWidget> &menu, const std::shared_ptr<WIDGET> &anchor)
{
	const int32_t height = std::min<int32_t>(menu->idealHeight(), static_cast<int32_t>(pie_GetVideoBufferHeight()));
	menu->setGeometry(menu->x(), menu->y(), menu->idealWidth(), height);
	menu->openMenu(anchor, PopoverWidget::Alignment::LeftOfParent, Vector2i(0, 1));
}
