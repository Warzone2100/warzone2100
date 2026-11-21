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

#include "hudconnection.h"
#include "hudids.h"
#include "hudrightcolumn.h"
#include "hudsettings.h"
#include "hudskin.h"
#include "hudwidgets.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/netplay/netplay.h"
#include "lib/widget/form.h"
#include "lib/widget/widget.h"

#include "../component.h"
#include "../hci.h"
#include "../multiplay.h"
#include "../vectorimageloader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <set>
#include <vector>

namespace
{

constexpr int ROW_HEIGHT = 20;
constexpr int ROW_GAP = 2;
constexpr int PAD = 3;
constexpr int EDGE = 2;           ///< the row's colored edge, on its left
constexpr int ICON_COLUMN = 30;   ///< (the dropped plug is wider than square)
constexpr int ICON_HEIGHT = 14;
constexpr int CHIP = 13;
constexpr int CHIP_GAP = 2;
constexpr int LINE_HEIGHT = 16;   ///< open: a line per player
constexpr int NAME_WIDTH = 84;
constexpr size_t PLAYERS_SHOWN = 3;
constexpr uint32_t ANNOUNCE_MS = 4000;
constexpr uint32_t WAITING_HOLD_MS = 1000;
constexpr uint32_t WAITING_MUTED_MS = 3000;
constexpr uint32_t WAITING_AMBER_MS = 10000;
constexpr uint32_t PULSE_MS = 1000;

/// The problems, in the statuses' order (as in the Classic column): Waiting last, so it can come and go without moving
/// the other rows
constexpr std::array<CONNECTION_STATUS, 4> PROBLEMS = {CONNECTIONSTATUS_PLAYER_DROPPED, CONNECTIONSTATUS_PLAYER_LEAVING,
                                                       CONNECTIONSTATUS_DESYNC, CONNECTIONSTATUS_WAITING_FOR_PLAYER};

/// The players' numbers: their positions, as the Classic warnings and the lobby show them
const std::string POSITION_NAMES = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

bool testShown = false;
bool testOpen = false;
bool panelExists = false;

const char *iconPath(CONNECTION_STATUS status)
{
	switch (status)
	{
	case CONNECTIONSTATUS_PLAYER_DROPPED: return "images/hud/icons/connection-dropped.svg";
	case CONNECTIONSTATUS_PLAYER_LEAVING: return "images/hud/icons/connection-left.svg";
	case CONNECTIONSTATUS_DESYNC: return "images/hud/icons/connection-desync.svg";
	default: return "images/hud/icons/connection-waiting.svg";
	}
}

const char *label(CONNECTION_STATUS status)
{
	switch (status)
	{
	case CONNECTIONSTATUS_PLAYER_DROPPED: return _("Dropped");
	case CONNECTIONSTATUS_PLAYER_LEAVING: return _("Player left");
	case CONNECTIONSTATUS_DESYNC: return _("Desync");
	default: return _("Waiting");
	}
}

PIELIGHT mix(PIELIGHT from, PIELIGHT to, float amount)
{
	auto channel = [amount](uint8_t a, uint8_t b) {
		return static_cast<uint8_t>(std::lround(a + (b - a) * amount));
	};
	return pal_RGBA(channel(from.byte.r, to.byte.r), channel(from.byte.g, to.byte.g), channel(from.byte.b, to.byte.b),
	                channel(from.byte.a, to.byte.a));
}

PIELIGHT withAlpha(PIELIGHT color, uint8_t alpha)
{
	color.byte.a = alpha;
	return color;
}

PIELIGHT opaque(PIELIGHT color)
{
	return withAlpha(color, 255);
}

/// The players with a status, in their positions' order (the numbers they are known by)
std::vector<unsigned> playersWith(CONNECTION_STATUS status)
{
	std::vector<unsigned> players;
	for (unsigned player = 0; player < MAX_PLAYERS; ++player)
	{
		if (NETcheckPlayerConnectionStatus(status, player))
		{
			players.push_back(player);
		}
	}
	std::sort(players.begin(), players.end(), [](unsigned a, unsigned b) {
		return NetPlay.players[a].position < NetPlay.players[b].position;
	});
	return players;
}

class ConnectionPanel : public WIDGET
{
public:
	/// Polls the statuses, and sizes and places the panel (false if there is nothing to show)
	bool refresh()
	{
		pollRows();
		if (rows.empty())
		{
			return false;
		}
		layout();
		return true;
	}

	void clicked(W_CONTEXT *, WIDGET_KEY) override
	{
		pinnedOpen = !pinnedOpen;
	}
	void highlight(W_CONTEXT *) override
	{
		hovered = true;
	}
	void highlightLost() override
	{
		hovered = false;
	}
	void setHelp(nonstd::optional<WidgetHelp> newHelp) override { help = newHelp; }
	WidgetHelp const *getHelp() const override { return help.has_value() ? &help.value() : nullptr; }

protected:
	void display(int xOffset, int yOffset) override;

private:
	nonstd::optional<WidgetHelp> help;

	struct Row
	{
		CONNECTION_STATUS status;
		std::vector<unsigned> players;
		int height = ROW_HEIGHT;
	};

	bool open() const
	{
		return hovered || pinnedOpen || (testShown && testOpen) || realTime < announceUntil;
	}

	/// 0 while a wait is short, to 1 as it goes on
	float waitingSeverity() const
	{
		const uint32_t waited = testShown ? 12000 : waitedMs;
		const float span = static_cast<float>(WAITING_AMBER_MS - WAITING_MUTED_MS);
		return std::min(std::max(static_cast<float>(waited) - static_cast<float>(WAITING_MUTED_MS), 0.f), span) / span;
	}

	PIELIGHT tone(CONNECTION_STATUS status) const
	{
		const HudColors &colors = currentHudSkin().colors();
		switch (status)
		{
		case CONNECTIONSTATUS_PLAYER_DROPPED:
		case CONNECTIONSTATUS_DESYNC:
			return colors.stateBad;
		case CONNECTIONSTATUS_PLAYER_LEAVING:
			return colors.stateWarn;
		default:
			return mix(colors.textSecondary, colors.stateWarn, waitingSeverity());
		}
	}

	void pollRows()
	{
		rows.clear();
		std::set<std::pair<int, unsigned>> present;
		for (CONNECTION_STATUS status : PROBLEMS)
		{
			std::vector<unsigned> players = playersWith(status);
			if (status == CONNECTIONSTATUS_WAITING_FOR_PLAYER)
			{
				// The status lapses a moment after each stalled frame, so a flaky connection would make the row flicker
				if (!players.empty())
				{
					waitingPlayers = players;
					waitingSeenAt = realTime;
					if (const uint32_t waited = gameTimeWaitingForPlayersDuration())
					{
						waitedMs = waited;
					}
				}
				else if (!waitingPlayers.empty() && realTime - waitingSeenAt < WAITING_HOLD_MS)
				{
					players = waitingPlayers;
				}
				else
				{
					waitingPlayers.clear();
					waitedMs = 0;
				}
			}
			else
			{
				for (unsigned player : players)
				{
					present.emplace(status, player);
					if (!testShown && announced.count({status, player}) == 0)
					{
						announceUntil = realTime + ANNOUNCE_MS;
					}
				}
			}
			if (!players.empty())
			{
				rows.push_back({status, std::move(players)});
			}
		}
		announced = std::move(present);
	}

	int textWidth(WzText &text, const std::string &string, iV_fonts font)
	{
		text.setText(WzString::fromUtf8(string), font);
		return text.width();
	}

	void layout()
	{
		const bool named = open();
		int width = 0;
		int labelWidth = 0;
		if (named)
		{
			for (size_t i = 0; i < rows.size(); ++i)
			{
				labelWidth = std::max(labelWidth, textWidth(labels[i], label(rows[i].status), font_regular));
			}
			labelWidth += 8;
		}
		for (size_t r = 0; r < rows.size(); ++r)
		{
			Row &row = rows[r];
			const size_t shown = std::min(row.players.size(), PLAYERS_SHOWN);
			const size_t more = row.players.size() - shown;
			int rowWidth = EDGE + ICON_COLUMN + 2;
			if (named)
			{
				int names = 0;
				for (size_t i = 0; i < shown; ++i)
				{
					names = std::max(names, std::min(NAME_WIDTH, textWidth(nameTexts[row.players[i]], getPlayerName(row.players[i]), font_small)));
				}
				rowWidth += labelWidth + CHIP + 3 + names + 6;
				if (row.status == CONNECTIONSTATUS_WAITING_FOR_PLAYER && waitedSeconds() > 0)
				{
					rowWidth += textWidth(seconds, secondsText(), font_regular) + 8;
				}
				row.height = std::max(ROW_HEIGHT, static_cast<int>(shown + (more ? 1 : 0)) * LINE_HEIGHT + 4);
			}
			else
			{
				rowWidth += static_cast<int>(shown) * (CHIP + CHIP_GAP) - CHIP_GAP + (more ? CHIP_GAP + moreChipWidth(moreCounts[r], more) : 0) + 4;
				row.height = ROW_HEIGHT;
			}
			width = std::max(width, rowWidth);
		}
		width += 2 * PAD;
		int height = 2 * PAD - ROW_GAP;
		for (const Row &row : rows)
		{
			height += row.height + ROW_GAP;
		}
		// (Placed by the right column)
		setGeometry(x(), y(), width, height);
	}

	unsigned waitedSeconds() const
	{
		const uint32_t waited = testShown ? 12000 : waitedMs;
		return waited >= WAITING_MUTED_MS ? waited / 1000 : 0;
	}

	std::string secondsText() const
	{
		return astringf(_("%u s"), waitedSeconds());
	}

	int moreChipWidth(WzText &text, size_t more)
	{
		return textWidth(text, astringf("+%u", static_cast<unsigned>(more)), font_bar) + 6;
	}

	void drawChip(unsigned player, int x, int y)
	{
		const PIELIGHT color = pal_GetTeamColour(getPlayerColour(player));
		const auto lighter = [](uint8_t c) { return static_cast<uint8_t>(c + (255 - c) / 5); };
		pie_DrawRoundedBox(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x + CHIP), static_cast<float>(y + CHIP), color, 3.f,
		                   pal_RGBA(lighter(color.byte.r), lighter(color.byte.g), lighter(color.byte.b), 255), 1.f);
		const unsigned position = std::min<unsigned>(NetPlay.players[player].position, POSITION_NAMES.size() - 1);
		WzText &number = numbers[position];
		number.setText(WzString::fromUtf8(std::string(1, POSITION_NAMES[position])), font_bar);
		const int luminance = (color.byte.r * 299 + color.byte.g * 587 + color.byte.b * 114) / 1000;
		number.renderCenteredOnInk(WzRect(x, y, CHIP, CHIP - 1), luminance > 128 ? pal_RGBA(10, 10, 10, 255) : pal_RGBA(250, 250, 250, 255), &digitsText());
	}

	void drawMoreChip(WzText &text, size_t more, int x, int y)
	{
		const int w = moreChipWidth(text, more);
		pie_DrawRoundedBox(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x + w), static_cast<float>(y + CHIP),
		                   pal_RGBA(46, 54, 68, 255), 3.f, pal_RGBA(87, 94, 105, 255), 1.f);
		text.renderCenteredOnInk(WzRect(x, y, w, CHIP - 1), currentHudSkin().colors().textPrimary, &digitsText());
	}

	/// Numbers share the digits' baseline (WzText::renderCenteredOnInk)
	WzText &digitsText()
	{
		digits.setText(WzString::fromUtf8("0123456789"), font_bar);
		return digits;
	}

	/// The baseline that centers a line of text with the chips on it: by the font's line metrics, as the HUD's other text
	float baselineFor(iV_fonts font, int centerY)
	{
		WzText &line = font == font_small ? smallLine : regularLine;
		line.setText(WzString::fromUtf8("H"), font);
		const float top = static_cast<float>(centerY - CHIP / 2);
		return std::round(top + (CHIP - static_cast<float>(line.lineSize())) / 2.f - static_cast<float>(line.aboveBase()));
	}

	void drawName(unsigned player, int x, int centerY)
	{
		WzText &text = nameTexts[player];
		text.setText(WzString::fromUtf8(getPlayerName(player)), font_small, NAME_WIDTH);
		hudDrawTextLine(text, static_cast<float>(x), baselineFor(font_small, centerY), NAME_WIDTH, currentHudSkin().colors().textSecondary);
	}

	void drawText(WzText &text, const std::string &string, iV_fonts font, float x, int centerY, PIELIGHT color, bool alignRight = false)
	{
		text.setText(WzString::fromUtf8(string), font);
		text.render(alignRight ? x - static_cast<float>(text.width()) : x, baselineFor(font, centerY), color);
	}

	std::vector<Row> rows;
	std::set<std::pair<int, unsigned>> announced;
	uint32_t announceUntil = 0;
	std::vector<unsigned> waitingPlayers;
	uint32_t waitingSeenAt = 0;
	uint32_t waitedMs = 0;
	bool hovered = false;
	bool pinnedOpen = false;
	std::array<WzText, 32> numbers;
	std::array<WzText, PROBLEMS.size()> moreCounts;
	WzText digits;
	WzText regularLine;
	WzText smallLine;
	std::array<WzText, PROBLEMS.size()> labels;
	WzText seconds;
	WzText moreText;
	std::array<WzText, MAX_PLAYERS> nameTexts;
};

void ConnectionPanel::display(int xOffset, int yOffset)
{
	const HudColors &colors = currentHudSkin().colors();
	const bool named = open();
	const int x0 = xOffset + x();
	const int y0 = yOffset + y();
	const int x1 = x0 + width();
	const int y1 = y0 + height();

	PIELIGHT worst = tone(CONNECTIONSTATUS_WAITING_FOR_PLAYER);
	for (const Row &row : rows)
	{
		if (row.status != CONNECTIONSTATUS_WAITING_FOR_PLAYER)
		{
			worst = tone(row.status);
			if (row.status != CONNECTIONSTATUS_PLAYER_LEAVING)
			{
				break;
			}
		}
	}
	const PIELIGHT panel = opaque(colors.panelBackground);
	pie_DrawRoundedBox(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(x1), static_cast<float>(y1), withAlpha(panel, 230), 5.f,
	                   mix(opaque(colors.panelBorder), worst, 0.35f), 1.f);

	// The pulse: calm (once a second), and none with Reduce Motion
	const float pulse = hudReduceMotion() ? 1.f : 0.7f + 0.3f * std::cos(static_cast<float>(realTime % PULSE_MS) / PULSE_MS * 6.2831853f);
	int labelWidth = 0;
	if (named)
	{
		for (size_t i = 0; i < rows.size(); ++i)
		{
			labelWidth = std::max(labelWidth, textWidth(labels[i], label(rows[i].status), font_regular));
		}
		labelWidth += 8;
	}

	int y = y0 + PAD;
	for (size_t i = 0; i < rows.size(); ++i)
	{
		const Row &row = rows[i];
		const PIELIGHT rowTone = tone(row.status);
		const bool waiting = row.status == CONNECTIONSTATUS_WAITING_FOR_PLAYER;
		const float severity = waiting ? waitingSeverity() : 1.f;
		const float strength = severity >= 1.f ? pulse : 0.2f * severity;
		const int rx0 = x0 + PAD;
		const int rx1 = x1 - PAD;
		pie_DrawRoundedBox(static_cast<float>(rx0), static_cast<float>(y), static_cast<float>(rx1), static_cast<float>(y + row.height),
		                   mix(panel, rowTone, 0.05f + 0.13f * strength), 3.f);
		pie_BoxFill(rx0, y + 2, rx0 + EDGE, y + row.height - 2, rowTone);

		const int centerY = y + ROW_HEIGHT / 2;
		const std::string path = iconPath(row.status);
		const float iconWidth = std::round(ICON_HEIGHT * gameVectorImageAspect(path, true));
		drawGameVectorImage(path, std::round(rx0 + EDGE + (ICON_COLUMN - iconWidth) / 2.f), static_cast<float>(centerY - ICON_HEIGHT / 2),
		                    iconWidth, static_cast<float>(ICON_HEIGHT), rowTone, 0.f, true);

		int x = rx0 + EDGE + ICON_COLUMN + 2;
		const size_t shown = std::min(row.players.size(), PLAYERS_SHOWN);
		const size_t more = row.players.size() - shown;
		if (named)
		{
			drawText(labels[i], label(row.status), font_regular, static_cast<float>(x), centerY, colors.textPrimary);
			x += labelWidth;
			int lineY = centerY;
			for (size_t p = 0; p < shown; ++p)
			{
				drawChip(row.players[p], x, lineY - CHIP / 2);
				drawName(row.players[p], x + CHIP + 3, lineY);
				lineY += LINE_HEIGHT;
			}
			if (more)
			{
				drawText(moreText, astringf(_("+%u more"), static_cast<unsigned>(more)), font_small, static_cast<float>(x + 1), lineY, colors.textSecondary);
			}
			if (waiting && waitedSeconds() > 0)
			{
				drawText(seconds, secondsText(), font_regular, static_cast<float>(rx1 - 5), centerY, rowTone, true);
			}
		}
		else
		{
			for (size_t p = 0; p < shown; ++p)
			{
				drawChip(row.players[p], x, centerY - CHIP / 2);
				x += CHIP + CHIP_GAP;
			}
			if (more)
			{
				drawMoreChip(moreCounts[i], more, x, centerY - CHIP / 2);
			}
		}
		y += row.height + ROW_GAP;
	}
}

ConnectionPanel *findPanel()
{
	return psWScreen ? dynamic_cast<ConnectionPanel *>(widgGetFromID(psWScreen, IDHUD_CONNECTION_PANEL)) : nullptr;
}

}

void hudUpdateConnectionPanel()
{
	if (psWScreen == nullptr)
	{
		return;
	}
	ConnectionPanel *panel = findPanel();
	if (panel == nullptr)
	{
		auto created = std::make_shared<ConnectionPanel>();
		created->id = IDHUD_CONNECTION_PANEL;
		created->setHelp(WidgetHelp()
			.setTitle(_("Connection Problems"))
			.setDescription(_("Players who dropped, left or are out of sync, and the player the game is waiting for. "
			                  "Hover over or tap it for their names.")));
		psWScreen->psForm->attach(created);
		hudRightColumnSet(HudRightColumnSlot::Connection, created);
		panel = created.get();
		panelExists = true;
	}
	panel->show(panel->refresh());
	hudLayoutRightColumn();
}

void hudRemoveConnectionPanel()
{
	if (panelExists && psWScreen != nullptr)
	{
		widgDelete(psWScreen, IDHUD_CONNECTION_PANEL);
	}
	panelExists = false;
}

void hudShowConnectionWarningsForTests(bool show, bool open)
{
	testShown = show;
	testOpen = open;
}

void hudApplyConnectionWarningsForTests()
{
	if (!testShown)
	{
		return;
	}
	std::vector<unsigned> others;
	for (unsigned player = 0; player < std::min<unsigned>(game.maxPlayers, MAX_PLAYERS); ++player)
	{
		if (player != selectedPlayer)
		{
			others.push_back(player);
		}
	}
	if (others.empty())
	{
		return;
	}
	NETsetPlayerConnectionStatus(CONNECTIONSTATUS_PLAYER_DROPPED, others[0]);
	NETsetPlayerConnectionStatus(CONNECTIONSTATUS_PLAYER_LEAVING, others[others.size() > 1 ? 1 : 0]);
	for (unsigned player : others)
	{
		NETsetPlayerConnectionStatus(CONNECTIONSTATUS_DESYNC, player);
	}
	NETsetPlayerConnectionStatus(CONNECTIONSTATUS_WAITING_FOR_PLAYER, others.back());
}
