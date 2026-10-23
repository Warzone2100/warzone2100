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

#include "hudconsole.h"
#include "hudchatbar.h"
#include "hudskin.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"
#include "lib/widget/widget.h"

#include "../hci.h"
#include "../multiplay.h"

#include <algorithm>
#include <cmath>

constexpr int CHIP_GAP = 6;
constexpr int TAG_GAP = 6;
constexpr int NAME_GAP = 5;
constexpr int TIME_GAP = 6;
/// Between the console and the options button, when it would reach it
constexpr int OPTIONS_GAP = 8;

static PIELIGHT faded(PIELIGHT color, float opacity)
{
	color.byte.a = static_cast<uint8_t>(color.byte.a * std::max(0.f, std::min(1.f, opacity)));
	return color;
}

static int chipSize(int pitch)
{
	return std::max(10, pitch - 3);
}

bool hudConsoleSkinned()
{
	return currentHudSkin().drawsConsole();
}

int hudConsoleChatRowHeight()
{
	return HudChatBar::barHeight(screenWidth < HUD_CHAT_BAR_COMPACT_WIDTH);
}

int hudConsoleRight(int x, int y, int right)
{
	if (WIDGET *options = psWScreen ? widgGetFromID(psWScreen, IDRET_OPTIONS) : nullptr)
	{
		const int optionsX = options->screenPosX();
		if (options->visible() && right > optionsX - OPTIONS_GAP && y < options->screenPosY() + options->height())
		{
			return std::max(x, optionsX - OPTIONS_GAP);
		}
	}
	return right;
}

ConsoleRect hudConsoleExpandedRect(const ConsoleRect &classic)
{
	ConsoleRect rect = classic;
	rect.height = classic.height - CONSOLE_PANEL_CHROME_HEIGHT + 2 * HUD_CONSOLE_PADDING + hudConsoleChatRowHeight();
	rect.width = hudConsoleRight(rect.x, rect.y, rect.x + rect.width) - rect.x;
	return rect;
}

void hudParseChatLine(const std::string &raw, int sender, std::string &name, std::string &channel, std::string &text)
{
	name.clear();
	channel.clear();
	text = raw;
	if (text.size() > 2 && text[0] == '[')
	{
		const size_t close = text.find("] ");
		if (close != std::string::npos && close < 12)
		{
			text = text.substr(close + 2);
		}
	}
	if (sender < 0 || sender >= MAX_CONNECTED_PLAYERS)
	{
		return;
	}
	const std::string playerName = getPlayerName(sender);
	if (playerName.empty() || text.compare(0, playerName.size(), playerName) != 0)
	{
		return;
	}
	const std::string rest = text.substr(playerName.size());
	if (rest.size() > 2 && rest[0] == ' ' && rest[1] == '(')
	{
		const size_t close = rest.find("): ");
		if (close != std::string::npos)
		{
			name = playerName;
			channel = rest.substr(2, close - 2);
			text = rest.substr(close + 3);
			return;
		}
	}
	if (rest.compare(0, 2, ": ") == 0)
	{
		name = playerName;
		text = rest.substr(2);
	}
}

HudConsolePrefix hudConsolePrefix(const WzString &message, int player, bool team, int pitch, WzString &text)
{
	HudConsolePrefix prefix;
	const std::string raw = message.toUtf8();
	std::string name;
	std::string channel;
	std::string body;
	hudParseChatLine(raw, player, name, channel, body);
	if (raw.size() > 2 && raw[0] == '[')
	{
		const size_t close = raw.find("] ");
		if (close != std::string::npos && close < 12)
		{
			prefix.time = raw.substr(0, close + 1);
		}
	}
	text = WzString::fromUtf8(body);
	if (name.empty())
	{
		// The console's own message, or one it couldn't split: as written, after the time
		prefix.width = prefix.time.empty() ? 0 : iV_GetTextWidth(prefix.time.c_str(), font_regular) + TIME_GAP;
		return prefix;
	}
	prefix.name = name;
	prefix.team = team;
	if (player >= 0 && player < MAX_PLAYERS)
	{
		prefix.sender = player;
	}
	if (team)
	{
		prefix.tag = _("Team");
	}
	else if (!channel.empty() && channel != _("Global"))
	{
		prefix.tag = channel;
	}
	int width = prefix.time.empty() ? 0 : iV_GetTextWidth(prefix.time.c_str(), font_regular) + TIME_GAP;
	if (prefix.sender >= 0)
	{
		width += chipSize(pitch) + CHIP_GAP;
	}
	if (!prefix.tag.empty())
	{
		width += intelkit::tagWidth(prefix.tag) + TAG_GAP;
	}
	width += iV_GetTextWidth((prefix.name + ":").c_str(), font_regular_bold) + NAME_GAP;
	prefix.width = width;
	return prefix;
}

void hudConsoleDrawPrefix(HudConsolePrefixTexts &texts, const HudConsolePrefix &prefix, int x, int y, int pitch, float opacity, bool overlay)
{
	HudSkin &skin = currentHudSkin();
	const HudColors &colors = skin.colors();
	const int cy = y + pitch / 2;
	const int baseline = y + (pitch - iV_GetTextLineSize(font_regular)) / 2 - iV_GetTextAboveBase(font_regular);
	auto drawText = [&](WzText &text, int textX, PIELIGHT color) {
		if (overlay)
		{
			skin.drawOverlayText(text, static_cast<float>(textX), static_cast<float>(baseline), color);
		}
		else
		{
			text.render(textX, baseline, color);
		}
	};
	if (!prefix.time.empty())
	{
		texts.time.setText(WzString::fromUtf8(prefix.time), font_regular);
		drawText(texts.time, x, faded(colors.textSecondary, opacity));
		x += texts.time.width() + TIME_GAP;
	}
	if (prefix.sender >= 0)
	{
		const int size = chipSize(pitch);
		intelkit::drawPositionChip(texts.chip, static_cast<unsigned>(prefix.sender), WzRect(x, cy - size / 2, size, size), opacity);
		x += size + CHIP_GAP;
	}
	if (!prefix.tag.empty())
	{
		x += intelkit::drawTag(texts.tag, prefix.tag, x, cy, faded(prefix.team ? colors.accent : colors.textSecondary, opacity), false) + TAG_GAP;
	}
	if (!prefix.name.empty())
	{
		texts.name.setText(WzString::fromUtf8(prefix.name + ":"), font_regular_bold);
		drawText(texts.name, x, faded(colors.textPrimary, opacity));
	}
}

PIELIGHT hudConsoleTextColor(int player, float opacity)
{
	const HudColors &colors = currentHudSkin().colors();
	return faded(player >= 0 ? colors.textPrimary : colors.textSecondary, opacity);
}
