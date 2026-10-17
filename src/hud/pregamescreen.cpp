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

#include "pregamescreen.h"
#include "hudchatbar.h"
#include "intel/intelkit.h"
#include "intel/teamstrategyview.h"

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/netplay/netplay.h"
#include "lib/widget/button.h"
#include "lib/widget/form.h"
#include "lib/widget/widgint.h"

#include "../console.h"
#include "../hci/teamstrategy.h"
#include "../multiplay.h"
#include "../screens/netpregamescreen.h"

#include <algorithm>
#include <chrono>
#include <deque>

using namespace intelkit;

namespace
{

constexpr int COMPACT_BELOW_WIDTH = 960;
constexpr int COMPACT_BELOW_HEIGHT = 600;
constexpr int MARGIN = 12;
constexpr int COMPACT_MARGIN = 4;
constexpr int HEADER = 62;
constexpr int HEADER_COMPACT = 44;
constexpr int PROGRESS = 3;
constexpr int FULL_WIDTH = 980;
constexpr int ROSTER_WIDTH = 560;
constexpr int PAD = 16;
constexpr int PAD_COMPACT = 10;
constexpr int PILL = 26;
constexpr int CHIP = 18;
constexpr int CHIP_COMPACT = 20;
constexpr int CARD = 32;
constexpr int CARD_COMPACT = 26;
constexpr int CHAT_LINE = 20;
constexpr int CHAT_LINES = 5;
constexpr int CHAT_HEIGHT = 3 * CHAT_LINE + 14;
constexpr int CHAT_BUTTON_ROW = 36;
constexpr int LEGEND_COMPACT = 40;
constexpr uint32_t CHAT_SHOWN_MS = 12000;
constexpr uint32_t CHAT_FADE_MS = 1200;
constexpr uint32_t TOAST_SHOWN_MS = 6000;

enum class Stage
{
	Waiting,
	Planning,
	Starting
};

/// The stage the tags show (the ready marks only while waiting)
Stage shownStage = Stage::Waiting;

bool compactLayout()
{
	return screenWidth < COMPACT_BELOW_WIDTH || screenHeight < COMPACT_BELOW_HEIGHT;
}

bool localIsSpectator()
{
	return selectedPlayer >= MAX_PLAYERS || NetPlay.players[selectedPlayer].isSpectator;
}

/// As the Classic screen's: a human still joining, or ready. An AI is ready. Anyone else has left
HudPlanRowStatus loadingStatus(uint32_t player)
{
	if (player >= MAX_CONNECTED_PLAYERS)
	{
		return HudPlanRowStatus::Left;
	}
	if (isHumanPlayer(player))
	{
		if (ingame.PendingDisconnect[player])
		{
			return HudPlanRowStatus::Left;
		}
		return ingame.JoiningInProgress[player] ? HudPlanRowStatus::Loading : HudPlanRowStatus::Ready;
	}
	if (player < MAX_PLAYERS && NetPlay.players[player].ai >= 0)
	{
		return HudPlanRowStatus::Ready;
	}
	return HudPlanRowStatus::Left;
}

bool isAi(uint32_t player)
{
	return player < MAX_PLAYERS && !isHumanPlayer(player) && NetPlay.players[player].ai >= 0;
}

/// The players (connected humans, humans who left, AIs), by position
std::vector<uint32_t> gamePlayers()
{
	std::vector<uint32_t> result;
	for (uint32_t player = 0; player < std::min<uint32_t>(game.maxPlayers, MAX_PLAYERS); ++player)
	{
		if (isHumanPlayer(player) || NetPlay.players[player].difficulty == AIDifficulty::HUMAN || NetPlay.players[player].ai >= 0)
		{
			result.push_back(player);
		}
	}
	std::sort(result.begin(), result.end(), [](uint32_t a, uint32_t b) { return NetPlay.players[a].position < NetPlay.players[b].position; });
	return result;
}

std::vector<uint32_t> gameSpectators()
{
	std::vector<uint32_t> result;
	for (uint32_t player = MAX_PLAYERS; player < MAX_CONNECTED_PLAYERS; ++player)
	{
		if (isHumanPlayer(player) || NetPlay.players[player].difficulty == AIDifficulty::HUMAN)
		{
			result.push_back(player);
		}
	}
	return result;
}

/// A team's label, in capitals ("TEAM A")
std::string teamName(int32_t team)
{
	return astringf(_("TEAM %c"), static_cast<char>('A' + std::max(0, std::min(team, 25))));
}

/// Whether any team has two players (otherwise, free for all: no team names)
bool gameHasTeams()
{
	std::vector<int32_t> teams;
	for (uint32_t player : gamePlayers())
	{
		const int32_t team = checkedGetPlayerTeam(static_cast<int32_t>(player));
		if (std::find(teams.begin(), teams.end(), team) != teams.end())
		{
			return true;
		}
		teams.push_back(team);
	}
	return false;
}

std::string joinNames(const std::vector<std::string> &names)
{
	std::string result;
	for (size_t i = 0; i < names.size(); ++i)
	{
		if (i > 0)
		{
			result += (i + 1 == names.size()) ? _(" and ") : ", ";
		}
		result += names[i];
	}
	return result;
}

/// "Still loading: Wren, Harrier and 3 spectators"
std::string stillLoadingLine()
{
	std::vector<std::string> names;
	size_t players = 0;
	for (uint32_t player : gamePlayers())
	{
		if (loadingStatus(player) == HudPlanRowStatus::Loading)
		{
			names.push_back(getPlayerName(player));
			++players;
		}
	}
	if (players > 3)
	{
		names.clear();
		names.push_back(astringf(ngettext("%zu player", "%zu players", players), players));
	}
	size_t spectators = 0;
	for (uint32_t player : gameSpectators())
	{
		spectators += loadingStatus(player) == HudPlanRowStatus::Loading ? 1 : 0;
	}
	if (spectators > 0)
	{
		names.push_back(astringf(ngettext("%zu spectator", "%zu spectators", spectators), spectators));
	}
	if (names.empty())
	{
		return _("Everyone is ready");
	}
	return astringf(_("Still loading: %s"), joinNames(names).c_str());
}

const char *statusLabel(HudPlanRowStatus status)
{
	switch (status)
	{
	case HudPlanRowStatus::None: return "";
	case HudPlanRowStatus::Loading: return _("Loading…");
	case HudPlanRowStatus::Ready: return _("Ready");
	case HudPlanRowStatus::Left: return _("Left");
	}
	return "";
}

/// The top of a line whose capitals are centered on `cy`, half a pixel higher
int capsCenteredTop(WzText &caps, iV_fonts font, int cy)
{
	caps.setText(WzString::fromUtf8("H"), font);
	return static_cast<int>(std::floor(static_cast<float>(cy) - caps.inkTop() / 2.f)) + caps.aboveBase();
}

/// A player's state: a spinner while loading, a check when ready (only while waiting), or "left"
void drawMark(HudPlanRowStatus status, int cx, int cy, int size)
{
	const HudColors &colors = skin().colors();
	switch (status)
	{
	case HudPlanRowStatus::None:
		break;
	case HudPlanRowStatus::Loading:
		drawSpinner(cx, cy, size / 2, colors.accent);
		break;
	case HudPlanRowStatus::Ready:
		if (shownStage == Stage::Waiting)
		{
			skin().drawNamedIcon("objective-done", WzRect(cx - size / 2, cy - size / 2, size, size), colors.stateOk);
		}
		break;
	case HudPlanRowStatus::Left:
		skin().drawNamedIcon("connection-left", WzRect(cx - size / 2, cy - size / 2, size, size), colors.stateBad);
		break;
	}
}

std::string loadingTime(uint32_t since)
{
	const uint32_t seconds = (realTime - since) / 1000;
	return astringf("%u:%02u", seconds / 60, seconds % 60);
}

/// The host's menu for a player still loading: how long, and kicking them (or a vote to)
void openKickMenu(uint32_t player, const std::shared_ptr<WIDGET> &anchor, uint32_t openedAt)
{
	if (!pregameCanKickLoadingPlayer(player))
	{
		return;
	}
	auto menu = IntelMenu::make(32, 210);
	auto heading = std::make_shared<WzText>();
	const std::string name = getPlayerName(player);
	menu->setHeading(30, [heading, name, openedAt](const WzRect &rect) {
		drawLine(*heading, astringf(_("%s · loading %s"), name.c_str(), loadingTime(openedAt).c_str()), font_small, rect.x() + 12,
		         rect.y() + 9, rect.width() - 24, skin().colors().textSecondary);
	});
	IntelMenu::Item kick;
	kick.icon = "kick";
	kick.label = pregameKickStartsVote(player) ? astringf(_("Vote to kick %s"), name.c_str()) : astringf(_("Kick %s"), name.c_str());
	kick.danger = true;
	kick.onChosen = [player]() {
		pregameKickLoadingPlayer(player);
	};
	menu->addItem(kick);
	menu->open(anchor, false);
}

enum class TagLook
{
	Pill,    ///< another team's player (full size): chip, name, AI, state
	Chip,    ///< another team's player (compact): the chip, with its state on its corner
	Card,    ///< everyone, without team planning: chip, name, AI, team, state
	Name     ///< a spectator, without team planning: state and name
};

/// A player as the screen shows them: the host can open the menu of one still loading
class PlayerTag : public W_BUTTON
{
public:
	PlayerTag(uint32_t player, TagLook look, uint32_t openedAt): player(player), look(look), openedAt(openedAt) {}

	uint32_t playerIndex() const { return player; }

	int32_t idealWidth() override
	{
		const std::string playerName = getPlayerName(player);
		switch (look)
		{
		case TagLook::Pill:
			return 4 + CHIP + 7 + iV_GetTextWidth(playerName.c_str(), font_regular) + (isAi(player) ? 6 + tagWidth("AI") : 0) + 22;
		case TagLook::Chip:
			return CHIP_COMPACT + 6;
		case TagLook::Card:
			return 220;
		case TagLook::Name:
			return 12 + 5 + iV_GetTextWidth(playerName.c_str(), font_small) + 4;
		}
		return 0;
	}

	int32_t idealHeight() override
	{
		switch (look)
		{
		case TagLook::Pill: return PILL;
		case TagLook::Chip: return CHIP_COMPACT + 6;
		case TagLook::Card: return compactLayout() ? CARD_COMPACT : CARD;
		case TagLook::Name: return 18;
		}
		return 0;
	}

	std::string getTip() override
	{
		const HudPlanRowStatus status = loadingStatus(player);
		std::string tip = astringf("%u · %s", NetPlay.players[player].position, getPlayerName(player));
		if (status == HudPlanRowStatus::Loading || status == HudPlanRowStatus::Left)
		{
			tip += std::string(" · ") + statusLabel(status);
		}
		return look == TagLook::Chip || canOpen() ? tip : std::string();
	}

	void released(W_CONTEXT *context, WIDGET_KEY key) override
	{
		const bool clicked = (state & WBUT_DOWN) != 0;
		W_BUTTON::released(context, key);
		if (clicked && canOpen())
		{
			openKickMenu(player, shared_from_this(), openedAt);
		}
	}

	void display(int xOffset, int yOffset) override
	{
		const HudColors &colors = skin().colors();
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		const HudPlanRowStatus status = loadingStatus(player);
		const bool away = status == HudPlanRowStatus::Loading || status == HudPlanRowStatus::Left;
		const bool hot = canOpen() && isHighlighted();
		const int cy = rect.y() + rect.height() / 2;
		switch (look)
		{
		case TagLook::Pill:
		{
			const float r = rect.height() / 2.f;
			const PIELIGHT border = hot ? alpha(colors.accent, 200) : (away ? alpha(colors.accent, 70) : pal_RGBA(120, 150, 190, 46));
			pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
			                   away ? pal_RGBA(0, 0, 0, 0) : pal_RGBA(255, 255, 255, 9), r, border, 1.f);
			const WzRect chip(rect.x() + 4, cy - CHIP / 2, CHIP, CHIP);
			drawPositionChip(chipText, player, chip);
			if (away)
			{
				dimRect(chip, 140);
			}
			int x0 = chip.right() + 7;
			const int markWidth = away ? 18 : 0;
			const int aiWidth = isAi(player) ? 6 + tagWidth(aiText, "AI") : 0;
			drawLine(name, getPlayerName(player), font_regular, x0, capsCenteredTop(caps, font_regular, cy),
			         rect.right() - 9 - markWidth - aiWidth - x0, away ? colors.textSecondary : colors.textPrimary);
			x0 += std::min(name.width(), rect.right() - 9 - markWidth - aiWidth - x0);
			if (isAi(player))
			{
				drawTag(aiText, "AI", x0 + 6, cy, colors.textSecondary, false);
			}
			if (away)
			{
				drawMark(status, rect.right() - 9 - 6, cy, 12);
			}
			break;
		}
		case TagLook::Chip:
		{
			const WzRect chip(rect.x() + 1, rect.y() + 1, CHIP_COMPACT, CHIP_COMPACT);
			drawPositionChip(chipText, player, chip);
			if (hot)
			{
				pie_DrawRoundedBox(static_cast<float>(chip.x() - 2), static_cast<float>(chip.y() - 2), static_cast<float>(chip.right() + 2),
				                   static_cast<float>(chip.bottom() + 2), pal_RGBA(0, 0, 0, 0), 4.f, colors.accent, 1.f);
			}
			if (away)
			{
				dimRect(chip, 140);
			}
			if (status != HudPlanRowStatus::None && (away || shownStage == Stage::Waiting))
			{
				const int mark = 11;
				const int mx = chip.right();
				const int my = chip.bottom();
				pie_DrawRoundedBox(static_cast<float>(mx - mark / 2 - 2), static_cast<float>(my - mark / 2 - 2), static_cast<float>(mx + mark / 2 + 2),
				                   static_cast<float>(my + mark / 2 + 2), colors.panelBackground, mark / 2.f + 2.f, pal_RGBA(0, 0, 0, 0), 0.f);
				drawMark(status, mx, my, mark);
			}
			break;
		}
		case TagLook::Card:
		{
			const bool you = player == selectedPlayer;
			pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
			                   you ? alpha(colors.accent, 24) : pal_RGBA(255, 255, 255, 9), 4.f, hot ? alpha(colors.accent, 220) : pal_RGBA(0, 0, 0, 0), 1.f);
			if (you)
			{
				pie_BoxFill(rect.x(), rect.y() + 2, rect.x() + 3, rect.bottom() - 2, colors.accent);
			}
			const int chipSize = rect.height() >= CARD ? 20 : 18;
			const WzRect chip(rect.x() + 10, cy - chipSize / 2, chipSize, chipSize);
			drawPositionChip(chipText, player, chip);
			if (away)
			{
				dimRect(chip, 140);
			}
			int right = rect.right() - 10;
			drawMark(status, right - 7, cy, 14);
			right -= 14 + 10;
			if (gameHasTeams())
			{
				const std::string team = teamName(checkedGetPlayerTeam(static_cast<int32_t>(player)));
				drawLine(teamText, team, font_small, right, capsCenteredTop(caps, font_small, cy), 70, colors.textSecondary, true);
				right -= std::min(teamText.width(), 70) + 10;
			}
			const int x0 = chip.right() + 9;
			const int aiWidth = isAi(player) ? 6 + tagWidth(aiText, "AI") : 0;
			const iV_fonts font = you ? font_regular_bold : font_regular;
			drawLine(name, getPlayerName(player), font, x0, capsCenteredTop(caps, font, cy), right - aiWidth - x0,
			         away ? colors.textSecondary : colors.textPrimary);
			if (isAi(player))
			{
				drawTag(aiText, "AI", x0 + std::min(name.width(), right - aiWidth - x0) + 6, cy, colors.textSecondary, false);
			}
			break;
		}
		case TagLook::Name:
		{
			drawMark(status == HudPlanRowStatus::Ready && shownStage != Stage::Waiting ? HudPlanRowStatus::None : status, rect.x() + 6, cy, 11);
			drawLine(name, getPlayerName(player), font_small, rect.x() + 17, capsCenteredTop(caps, font_small, cy), rect.width() - 17,
			         away ? colors.textSecondary : colors.textPrimary);
			if (hot)
			{
				pie_BoxFill(rect.x() + 17, rect.bottom() - 1, rect.x() + 17 + std::min(name.width(), rect.width() - 17), rect.bottom(), alpha(colors.accent, 200));
			}
			break;
		}
		}
	}

private:
	bool canOpen() const { return pregameCanKickLoadingPlayer(player); }

	uint32_t player;
	TagLook look;
	uint32_t openedAt;
	ChipText chipText;
	WzText name;
	WzText teamText;
	WzText aiText;
	WzText caps;
};

/// The other teams above the view: a team a cell (two to a row when there are several), its pills wrapping in it,
/// or (compact) one line of chips, after your team's AIs
class OthersArea : public WIDGET
{
public:
	struct Group
	{
		int32_t team = 0;
		std::vector<std::shared_ptr<PlayerTag>> tags;
		int labelX = 0;
		int labelY = 0;
	};

	/// `yourAis` (compact only): your team's AIs, which aren't in the view
	void initialize(const std::vector<std::pair<int32_t, std::vector<uint32_t>>> &teams, const std::vector<uint32_t> &yourAis, bool compactLayout,
	                uint32_t openedAt)
	{
		compact = compactLayout;
		for (uint32_t player : yourAis)
		{
			auto tag = std::make_shared<PlayerTag>(player, TagLook::Chip, openedAt);
			attach(tag);
			yours.push_back(tag);
		}
		for (const auto &team : teams)
		{
			Group group;
			group.team = team.first;
			for (uint32_t player : team.second)
			{
				auto tag = std::make_shared<PlayerTag>(player, compact ? TagLook::Chip : TagLook::Pill, openedAt);
				attach(tag);
				group.tags.push_back(tag);
			}
			groups.push_back(std::move(group));
		}
	}

	bool empty() const { return groups.empty() && yours.empty(); }

	/// Lays the tags out at `w`, and returns the height
	int layoutAt(int w)
	{
		if (compact)
		{
			const int labelWidth = iV_GetTextWidth(_("OTHERS"), font_small) + 14;
			int x0 = PAD_COMPACT;
			int y0 = 6;
			if (!yours.empty())
			{
				x0 += iV_GetTextWidth(_("YOUR TEAM"), font_small) + 10;
				for (auto &tag : yours)
				{
					tag->setGeometry(x0, y0, tag->idealWidth(), tag->idealHeight());
					x0 += CHIP_COMPACT + 10;
				}
				x0 += 14;
			}
			othersX = x0;
			x0 += labelWidth;
			for (auto &group : groups)
			{
				const int letter = 12;
				const int needed = letter + static_cast<int>(group.tags.size()) * (CHIP_COMPACT + 10);
				if (x0 + needed > w - PAD_COMPACT && x0 > PAD_COMPACT + labelWidth)
				{
					x0 = PAD_COMPACT + labelWidth;
					y0 += CHIP_COMPACT + 12;
				}
				group.labelX = x0;
				group.labelY = y0 + 3;
				x0 += letter + 6;
				for (auto &tag : group.tags)
				{
					tag->setGeometry(x0, y0, tag->idealWidth(), tag->idealHeight());
					x0 += CHIP_COMPACT + 10;
				}
				x0 += 8;
			}
			return y0 + CHIP_COMPACT + 6 + 8;
		}
		const int columns = groups.size() > 1 ? 2 : 1;
		const int gap = 24;
		const int cellWidth = (w - 2 * PAD - (columns - 1) * gap) / columns;
		const int labelWidth = 62;
		int y0 = 10;
		for (size_t i = 0; i < groups.size(); i += columns)
		{
			int rowHeight = 0;
			for (int c = 0; c < columns && i + c < groups.size(); ++c)
			{
				Group &group = groups[i + c];
				const int cellX = PAD + c * (cellWidth + gap);
				group.labelX = cellX;
				group.labelY = y0 + (PILL - static_cast<int>(iV_GetTextLineSize(font_small))) / 2;
				int x0 = cellX + labelWidth;
				int yy = y0;
				for (auto &tag : group.tags)
				{
					const int tw = std::min(tag->idealWidth(), cellWidth - labelWidth);
					if (x0 + tw > cellX + cellWidth && x0 > cellX + labelWidth)
					{
						x0 = cellX + labelWidth;
						yy += PILL + 6;
					}
					tag->setGeometry(x0, yy, tw, PILL);
					x0 += tw + 8;
				}
				rowHeight = std::max(rowHeight, yy + PILL - y0);
			}
			y0 += rowHeight + 8;
		}
		return y0 + 2;
	}

	void display(int xOffset, int yOffset) override
	{
		const HudColors &colors = skin().colors();
		if (compact)
		{
			if (!yours.empty())
			{
				drawLine(yoursText, _("YOUR TEAM"), font_small, xOffset + x() + PAD_COMPACT, yOffset + y() + 9, 90, colors.textSecondary);
			}
			if (!groups.empty())
			{
				drawLine(othersText, _("OTHERS"), font_small, xOffset + x() + othersX, yOffset + y() + 9, 80, colors.textSecondary);
			}
		}
		labels.resize(groups.size());
		for (size_t i = 0; i < groups.size(); ++i)
		{
			const std::string label = compact ? std::string(1, static_cast<char>('A' + std::max(0, std::min(groups[i].team, 25)))) : teamName(groups[i].team);
			drawLine(labels[i], label, font_small, xOffset + x() + groups[i].labelX, yOffset + y() + groups[i].labelY, compact ? 14 : 58, colors.textSecondary);
		}
	}

private:
	bool compact = false;
	int othersX = 0;
	std::vector<std::shared_ptr<PlayerTag>> yours;
	std::vector<Group> groups;
	std::vector<WzText> labels;
	WzText othersText;
	WzText yoursText;
};

/// Without team planning (or for a spectator, or a player without teammates): everyone in two columns, and the
/// spectators by name under them
class RosterArea : public WIDGET
{
public:
	void initialize(bool compactLayout, uint32_t openedAt)
	{
		compact = compactLayout;
		for (uint32_t player : gamePlayers())
		{
			auto tag = std::make_shared<PlayerTag>(player, TagLook::Card, openedAt);
			attach(tag);
			cards.push_back(tag);
		}
		for (uint32_t player : gameSpectators())
		{
			auto tag = std::make_shared<PlayerTag>(player, TagLook::Name, openedAt);
			attach(tag);
			spectators.push_back(tag);
		}
	}

	int layoutAt(int w)
	{
		const int pad = compact ? PAD_COMPACT : PAD;
		const int columns = cards.size() > 4 ? 2 : 1;
		const int gap = 8;
		const int cardWidth = (w - 2 * pad - (columns - 1) * gap) / columns;
		const int cardHeight = compact ? CARD_COMPACT : CARD;
		const int rows = static_cast<int>((cards.size() + columns - 1) / columns);
		for (size_t i = 0; i < cards.size(); ++i)
		{
			const int column = static_cast<int>(i) / std::max(1, rows);
			const int row = static_cast<int>(i) % std::max(1, rows);
			cards[i]->setGeometry(pad + column * (cardWidth + gap), (compact ? 8 : 12) + row * (cardHeight + 6), cardWidth, cardHeight);
		}
		int y0 = (compact ? 8 : 12) + rows * (cardHeight + 6) + 4;
		if (!spectators.empty())
		{
			headingY = y0 + 4;
			y0 += 4 + static_cast<int>(iV_GetTextLineSize(font_small)) + 6;
			int x0 = pad;
			for (auto &tag : spectators)
			{
				const int tw = std::min(tag->idealWidth(), w - 2 * pad);
				if (x0 + tw > w - pad && x0 > pad)
				{
					x0 = pad;
					y0 += 20;
				}
				tag->setGeometry(x0, y0, tw, 18);
				x0 += tw + 14;
			}
			y0 += 20;
		}
		return y0 + (compact ? 6 : 10);
	}

	void display(int xOffset, int yOffset) override
	{
		if (spectators.empty())
		{
			return;
		}
		size_t ready = 0;
		for (auto &tag : spectators)
		{
			ready += loadingStatus(tag->playerIndex()) == HudPlanRowStatus::Ready ? 1 : 0;
		}
		const int pad = compact ? PAD_COMPACT : PAD;
		const int x0 = xOffset + x() + pad;
		const int y0 = yOffset + y() + headingY;
		const int s = 13;
		skin().drawNamedIcon("visibility", WzRect(x0, y0 + (static_cast<int>(iV_GetTextLineSize(font_small)) - s) / 2, s, s), skin().colors().textSecondary);
		const std::string line = shownStage == Stage::Waiting ? astringf(_("Spectators · %zu of %zu ready"), ready, spectators.size())
		                                                      : astringf(_("Spectators · %zu"), spectators.size());
		drawLine(heading, line, font_small, x0 + s + 6, y0, width() - 2 * pad - s - 6, skin().colors().textSecondary);
	}

private:
	bool compact = false;
	int headingY = 0;
	std::vector<std::shared_ptr<PlayerTag>> cards;
	std::vector<std::shared_ptr<PlayerTag>> spectators;
	WzText heading;
};

/// The header's spectators count, which lists them (and, for the host, opens the menu of one still loading)
class SpectatorsButton : public W_BUTTON
{
public:
	explicit SpectatorsButton(uint32_t openedAt): openedAt(openedAt)
	{
		setTip(_("Spectators"));
	}

	std::string label() const
	{
		const std::vector<uint32_t> list = gameSpectators();
		size_t ready = 0;
		for (uint32_t player : list)
		{
			ready += loadingStatus(player) == HudPlanRowStatus::Ready ? 1 : 0;
		}
		return shownStage == Stage::Waiting ? astringf("%zu/%zu", ready, list.size()) : astringf("%zu", list.size());
	}

	int32_t idealWidth() override
	{
		return 8 + 15 + 6 + iV_GetTextWidth(label().c_str(), font_small) + 8;
	}

	void clicked(W_CONTEXT *context, WIDGET_KEY key) override
	{
		W_BUTTON::clicked(context, key);
		openList();
	}

	void display(int xOffset, int yOffset) override
	{
		const HudColors &colors = skin().colors();
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		if (isHighlighted())
		{
			pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
			                   pal_RGBA(255, 255, 255, 12), skin().metrics().cardRadius, alpha(colors.panelBorder, 255), 1.f);
		}
		const int s = 15;
		const int cy = rect.y() + rect.height() / 2;
		skin().drawNamedIcon("visibility", WzRect(rect.x() + 8, cy - s / 2, s, s), colors.textSecondary);
		text.setText(WzString::fromUtf8(label()), font_small);
		text.render(static_cast<float>(rect.x() + 8 + s + 6), std::round(cy - text.lineSize() / 2.f - text.aboveBase()), colors.textSecondary);
	}

private:
	void openList()
	{
		const std::vector<uint32_t> list = gameSpectators();
		if (list.empty())
		{
			return;
		}
		auto menu = IntelMenu::make(28, 200);
		size_t ready = 0;
		for (uint32_t player : list)
		{
			ready += loadingStatus(player) == HudPlanRowStatus::Ready ? 1 : 0;
		}
		auto heading = std::make_shared<WzText>();
		const std::string title = astringf(_("Spectators · %zu of %zu ready"), ready, list.size());
		menu->setHeading(28, [heading, title](const WzRect &rect) {
			drawLine(*heading, title, font_small, rect.x() + 12, rect.y() + 8, rect.width() - 24, skin().colors().textSecondary);
		});
		std::weak_ptr<WIDGET> weakSelf = shared_from_this();
		const uint32_t since = openedAt;
		for (uint32_t player : list)
		{
			const HudPlanRowStatus status = loadingStatus(player);
			IntelMenu::Item item;
			item.icon = status == HudPlanRowStatus::Ready ? "objective-done" : (status == HudPlanRowStatus::Loading ? "connection-waiting" : "connection-left");
			item.label = getPlayerName(player);
			item.detail = statusLabel(status);
			if (pregameCanKickLoadingPlayer(player))
			{
				item.onChosen = [player, weakSelf, since]() {
					widgScheduleTask([player, weakSelf, since]() {
						if (auto self = weakSelf.lock())
						{
							openKickMenu(player, self, since);
						}
					});
				};
			}
			menu->addItem(item);
		}
		menu->open(shared_from_this(), true);
	}

	uint32_t openedAt;
	WzText text;
};

/// A console line, as a chat line: who said it, to which channel (as the console writes it, ex. "Allies"), and what
struct ChatLine
{
	std::string name;
	std::string channel;
	std::string text;
	int32_t sender = SYSTEM_MESSAGE;
	bool team = false;
	uint32_t time = 0;
	WzText nameText;
	WzText tagText;
	WzText rendered;
};

/// Splits a console line ("[01:18] Kestrel (Allies): gl hf", the time when the console's timestamps are on) into its
/// parts (a line it can't split stays whole)
void parseChatLine(ChatLine &line, const std::string &raw)
{
	std::string text = raw;
	if (text.size() > 2 && text[0] == '[')
	{
		const size_t close = text.find("] ");
		if (close != std::string::npos && close < 12)
		{
			text = text.substr(close + 2);
		}
	}
	if (line.sender >= 0 && line.sender < MAX_CONNECTED_PLAYERS)
	{
		const std::string name = getPlayerName(line.sender);
		if (!name.empty() && text.compare(0, name.size(), name) == 0)
		{
			const std::string rest = text.substr(name.size());
			if (rest.size() > 2 && rest[0] == ' ' && rest[1] == '(')
			{
				const size_t close = rest.find("): ");
				if (close != std::string::npos)
				{
					line.name = name;
					line.channel = rest.substr(2, close - 2);
					line.text = rest.substr(close + 3);
					return;
				}
			}
			if (rest.compare(0, 2, ": ") == 0)
			{
				line.name = name;
				line.text = rest.substr(2);
				return;
			}
		}
	}
	line.text = text;
}

class PregamePanel;
class PregameRoot;

/// The chat lines, over the panel (compact) or under it: drawn by the screen, above its other widgets
class ChatLinesLayer : public WIDGET
{
public:
	explicit ChatLinesLayer(PregameRoot *root): root(root)
	{
		setTransparentToMouse(true);
	}
	void display(int xOffset, int yOffset) override;

private:
	PregameRoot *root;
};

/// The whole screen: the backdrop, the panel, the chat lines and bar, the stages and the keys
class PregameRoot : public W_FORM
{
public:
	~PregameRoot() override
	{
		consoleRemoveMessageListener(listener);
	}

	void initialize(std::function<void ()> close);

	void geometryChanged() override { layoutChildren(); }
	void run(W_CONTEXT *context) override;
	void display(int xOffset, int yOffset) override;

	Stage stage = Stage::Waiting;
	std::chrono::steady_clock::time_point stageStart;
	float stageFraction() const;
	int stageSecondsLeft() const;

	void openChat();
	void displayChatLines(int xOffset, int yOffset);

private:
	void layoutChildren();
	void enterStage(Stage next);
	void showCompactBar(bool shown);

	bool compact = false;
	bool closing = false;
	bool chatFinished = false;
	std::function<void ()> onClose;
	std::shared_ptr<PregamePanel> panel;
	std::shared_ptr<HudChatBar> chatBar;
	std::shared_ptr<ChatLinesLayer> chatLines;
	std::shared_ptr<CONSOLE_MESSAGE_LISTENER> listener;
	std::deque<ChatLine> lines;
	ChipText lineChip;
};

class PregamePanel : public WIDGET
{
public:
	void initialize(PregameRoot *owner, bool compactLayout, uint32_t openedAt)
	{
		root = owner;
		compact = compactLayout;
		planning = gameHasTeamStrategyView();
		const bool spectator = localIsSpectator();
		std::vector<uint32_t> teammates = spectator ? std::vector<uint32_t>() : teamStrategyTeammates(false);
		withView = planning && !spectator && !teammates.empty();
		localTeam = spectator ? -1 : checkedGetPlayerTeam(static_cast<int32_t>(selectedPlayer));

		if (withView)
		{
			// The other teams' players, and your team's AIs (which aren't in the view) on its line,
			// or first on the compact line
			std::vector<std::pair<int32_t, std::vector<uint32_t>>> teams;
			std::vector<uint32_t> yourAis;
			for (uint32_t player : gamePlayers())
			{
				const int32_t team = checkedGetPlayerTeam(static_cast<int32_t>(player));
				if (player == selectedPlayer || std::find(teammates.begin(), teammates.end(), player) != teammates.end())
				{
					continue;
				}
				if (team == localTeam)
				{
					yourAis.push_back(player);
					continue;
				}
				auto it = std::find_if(teams.begin(), teams.end(), [team](const std::pair<int32_t, std::vector<uint32_t>> &t) { return t.first == team; });
				if (it == teams.end())
				{
					teams.push_back({team, {player}});
				}
				else
				{
					it->second.push_back(player);
				}
			}
			others = std::make_shared<OthersArea>();
			others->initialize(teams, compact ? yourAis : std::vector<uint32_t>(), compact, openedAt);
			attach(others);

			HudTeamStrategyEmbedding embed;
			embed.sending = false;
			embed.status = [this](uint32_t player) -> HudPlanRowStatus {
				return (root && root->stage == Stage::Waiting) ? loadingStatus(player) : (loadingStatus(player) == HudPlanRowStatus::Left ? HudPlanRowStatus::Left : HudPlanRowStatus::None);
			};
			embed.playerHasMenu = [](uint32_t player) { return pregameCanKickLoadingPlayer(player); };
			embed.openPlayerMenu = [openedAt](uint32_t player, const std::shared_ptr<WIDGET> &anchor) {
				openKickMenu(player, anchor, openedAt);
			};
			embed.legendRightGutter = compact ? 80 : 0;
			embed.factsLine = factsLine(Stage::Waiting);
			embed.plain = true;
			view = HudTeamStrategyView::make(compact, embed);
			attach(view);
			if (!compact)
			{
				for (uint32_t player : yourAis)
				{
					auto tag = std::make_shared<PlayerTag>(player, TagLook::Pill, openedAt);
					attach(tag);
					teamAis.push_back(tag);
				}
			}
		}
		else
		{
			roster = std::make_shared<RosterArea>();
			roster->initialize(compact, openedAt);
			attach(roster);
		}

		if (!gameSpectators().empty() && withView)
		{
			spectators = std::make_shared<SpectatorsButton>(openedAt);
			attach(spectators);
		}
		if (compact)
		{
			chatButton = IntelButton::make(_("Chat"), "chat", false, 26);
			chatButton->setTip(_("Chat (Enter)"));
			chatButton->addOnClickHandler([this](W_BUTTON &) {
				widgScheduleTask([this]() {
					if (root)
					{
						root->openChat();
					}
				});
			});
			attach(chatButton);
		}
	}

	bool hasView() const { return withView; }

	/// The view's line: your team, and while waiting, when the plans go out
	std::string factsLine(Stage stage) const
	{
		return astringf(_("%s · YOUR TEAM"), teamName(localTeam).c_str());
	}
	const std::shared_ptr<HudTeamStrategyView> &strategyView() const { return view; }

	int headerHeight() const { return compact ? HEADER_COMPACT : HEADER; }

	/// Its height at `w`, with at most `maxHeight` (the view scrolls)
	int heightFor(int w, int maxHeight)
	{
		int h = headerHeight() + PROGRESS;
		if (others && !others->empty())
		{
			h += others->layoutAt(w);
		}
		if (view)
		{
			h += std::min(view->heightFor(w), std::max(120, maxHeight - h));
		}
		if (roster)
		{
			h += roster->layoutAt(w);
			if (compact)
			{
				h += CHAT_BUTTON_ROW;
			}
		}
		return std::min(h, maxHeight);
	}

	void geometryChanged() override
	{
		const int w = width();
		int y0 = headerHeight() + PROGRESS;
		if (spectators)
		{
			const int sw = spectators->idealWidth();
			const int right = compact ? 92 : 168;
			spectators->setGeometry(w - right - sw - 8, (headerHeight() - 26) / 2, sw, 26);
		}
		if (others && !others->empty())
		{
			const int oh = others->layoutAt(w);
			others->setGeometry(0, y0, w, oh);
			y0 += oh;
			othersBottom = y0;
		}
		if (view)
		{
			view->setGeometry(0, y0, w, std::max(0, height() - y0));
			// Your team's AIs: after the view's line ("TEAM A · YOUR TEAM"), as long as there's room
			int x0 = PAD + static_cast<int>(iV_GetTextWidth(factsLine(Stage::Waiting).c_str(), font_small)) + 14;
			const int ty = y0 + 9 + static_cast<int>(iV_GetTextLineSize(font_small)) / 2 - PILL / 2;
			for (auto &tag : teamAis)
			{
				const int tw = std::min(tag->idealWidth(), w - PAD - x0);
				tag->show(tw >= 60);
				tag->setGeometry(x0, ty, std::max(0, tw), PILL);
				x0 += tw + 8;
			}
			if (chatButton)
			{
				const int bw = chatButton->idealWidth();
				chatButton->setGeometry(w - PAD_COMPACT - bw, height() - LEGEND_COMPACT / 2 - 2 - 13, bw, 26);
			}
		}
		if (roster)
		{
			const int footer = compact ? CHAT_BUTTON_ROW : 0;
			roster->setGeometry(0, y0, w, std::max(0, height() - y0 - footer));
			roster->layoutAt(w);
			if (chatButton)
			{
				const int bw = chatButton->idealWidth();
				chatButton->setGeometry(w - PAD_COMPACT - bw, height() - footer + (footer - 26) / 2, bw, 26);
			}
		}
	}

	void display(int xOffset, int yOffset) override
	{
		HudSkin &hudSkin = currentHudSkin();
		const HudColors &colors = hudSkin.colors();
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		hudSkin.drawPanelFrame(rect, rect);
		const int header = headerHeight();
		const int pad = compact ? PAD_COMPACT : PAD;
		const Stage stage = root ? root->stage : Stage::Waiting;

		// The stage: its icon, title and line
		const int iconSize = compact ? 18 : 20;
		const int iy = rect.y() + header / 2;
		if (stage == Stage::Waiting)
		{
			drawSpinner(rect.x() + pad + iconSize / 2, iy, iconSize / 2, colors.accent);
		}
		else
		{
			hudSkin.drawNamedIcon(planning ? "team-strategy" : "players", WzRect(rect.x() + pad, iy - iconSize / 2, iconSize, iconSize), colors.accent);
		}
		std::string titleLine;
		std::string subLine;
		switch (stage)
		{
		case Stage::Waiting:
			titleLine = _("Waiting for players");
			subLine = stillLoadingLine();
			break;
		case Stage::Planning:
			titleLine = _("Team planning");
			subLine = withView ? _("Agree on what each of you builds") : _("The teams are agreeing on their plans");
			break;
		case Stage::Starting:
			titleLine = _("Game starting");
			subLine = withView ? (compact ? _("Your team's plans are set") : _("Your team's plans are set · change them in the game: Intelligence, Team strategy"))
			                   : _("Get ready");
			break;
		}
		const int tx = rect.x() + pad + iconSize + (compact ? 10 : 12);
		const int rightGutter = (compact ? 92 : 168) + (spectators ? spectators->width() + 12 : 0);
		const iV_fonts titleFont = compact ? font_regular_bold : font_medium_bold;
		const int titleSize = static_cast<int>(iV_GetTextLineSize(titleFont));
		const iV_fonts subFont = compact ? font_small : font_regular;
		const int subSize = static_cast<int>(iV_GetTextLineSize(subFont));
		const int top = rect.y() + (header - titleSize - subSize - 2) / 2;
		drawLine(title, titleLine, titleFont, tx, top, rect.right() - rightGutter - tx, colors.textPrimary);
		drawLine(sub, subLine, subFont, tx, top + titleSize + 2, rect.right() - rightGutter - tx, colors.textSecondary);

		// The right: the players ready, or the time to the start
		std::string big;
		std::string small;
		if (stage == Stage::Waiting)
		{
			const std::vector<uint32_t> players = gamePlayers();
			const size_t ready = static_cast<size_t>(std::count_if(players.begin(), players.end(), [](uint32_t p) { return loadingStatus(p) == HudPlanRowStatus::Ready; }));
			big = std::to_string(ready);
			small = compact ? astringf("/%zu", players.size()) : astringf(_("of %zu ready"), players.size());
		}
		else
		{
			const int seconds = root ? root->stageSecondsLeft() : 0;
			const int shownSeconds = std::max(1, seconds);
			big = stage == Stage::Planning ? astringf("%d:%02d", shownSeconds / 60, shownSeconds % 60) : std::to_string(shownSeconds);
			small = compact ? std::string() : std::string(_("starts in"));
		}
		const iV_fonts bigFont = compact ? font_medium_bold : font_large;
		bigText.setText(WzString::fromUtf8(big), bigFont);
		smallText.setText(WzString::fromUtf8(small), font_small);
		const float baseline = std::round(static_cast<float>(iy) + bigText.lineSize() / 2.f - bigText.lineSize() - bigText.aboveBase());
		if (stage == Stage::Waiting)
		{
			const int sx = rect.right() - pad - smallText.width();
			smallText.render(static_cast<float>(sx), baseline + bigText.aboveBase() - smallText.aboveBase() + (bigText.lineSize() - smallText.lineSize()) * 0.75f,
			                 colors.textSecondary);
			bigText.render(static_cast<float>(sx - 6 - bigText.width()), baseline, colors.textPrimary);
		}
		else
		{
			const int bx = rect.right() - pad - bigText.width();
			bigText.render(static_cast<float>(bx), baseline, colors.textPrimary);
			smallText.render(static_cast<float>(bx - 8 - smallText.width()),
			                 baseline + bigText.aboveBase() - smallText.aboveBase() + (bigText.lineSize() - smallText.lineSize()) * 0.75f, colors.textSecondary);
		}

		// The header's divider, or the progress (draining to the start), and the other dividers
		const int py = rect.y() + header;
		if (stage == Stage::Waiting || !root)
		{
			drawDivider(rect.x() + 1, py, rect.right() - 1, py + 1);
		}
		else
		{
			pie_UniTransBoxFill(static_cast<float>(rect.x() + 1), static_cast<float>(py), static_cast<float>(rect.right() - 1), static_cast<float>(py + PROGRESS),
			                    pal_RGBA(120, 150, 190, 30));
			const float fraction = root->stageFraction();
			pie_UniTransBoxFill(static_cast<float>(rect.x() + 1), static_cast<float>(py), static_cast<float>(rect.x() + 1) + (rect.width() - 2) * fraction,
			                    static_cast<float>(py + PROGRESS), colors.accent);
		}
		if (others && !others->empty())
		{
			drawDivider(rect.x() + 1, rect.y() + othersBottom, rect.right() - 1, rect.y() + othersBottom + 1);
		}
		if (roster && compact)
		{
			const int fy = rect.bottom() - CHAT_BUTTON_ROW;
			drawDivider(rect.x() + 1, fy, rect.right() - 1, fy + 1);
		}
	}

private:
	PregameRoot *root = nullptr;
	bool compact = false;
	bool planning = false;
	bool withView = false;
	int32_t localTeam = -1;
	int othersBottom = 0;
	std::shared_ptr<OthersArea> others;
	std::vector<std::shared_ptr<PlayerTag>> teamAis;
	std::shared_ptr<HudTeamStrategyView> view;
	std::shared_ptr<RosterArea> roster;
	std::shared_ptr<SpectatorsButton> spectators;
	std::shared_ptr<IntelButton> chatButton;
	WzText title;
	WzText sub;
	WzText bigText;
	WzText smallText;
};

void PregameRoot::initialize(std::function<void ()> close)
{
	onClose = std::move(close);
	compact = compactLayout();
	shownStage = Stage::Waiting;
	stageStart = std::chrono::steady_clock::now();
	const uint32_t openedAt = realTime;

	panel = std::make_shared<PregamePanel>();
	attach(panel);
	panel->initialize(this, compact, openedAt);

	chatLines = std::make_shared<ChatLinesLayer>(this);
	attach(chatLines);

	const bool teamChat = gameHasTeamStrategyView() && panel->hasView();
	chatBar = HudChatBar::make(compact, teamChat ? WzChatMode::Team : WzChatMode::Glob);
	attach(chatBar);
	chatBar->onFinished = [this]() {
		chatFinished = true;
		if (compact)
		{
			showCompactBar(false);
		}
	};
	if (compact)
	{
		chatBar->hide();
	}

	listener = std::make_shared<CONSOLE_MESSAGE_LISTENER>([this](ConsoleMessage const &message) {
		ChatLine line;
		line.sender = message.sender;
		line.team = message.team;
		parseChatLine(line, message.text);
		line.time = realTime;
		lines.push_back(std::move(line));
		while (lines.size() > 8)
		{
			lines.pop_front();
		}
	});
	consoleAddMessageListener(listener);

	setCalcLayout([](WIDGET *widget) {
		widget->setGeometry(0, 0, static_cast<int>(screenWidth), static_cast<int>(screenHeight));
	});
}

void PregameRoot::layoutChildren()
{
	if (width() == 0 || height() == 0)
	{
		return;
	}
	chatLines->setGeometry(0, 0, width(), height());
	if (compact)
	{
		panel->setGeometry(COMPACT_MARGIN, COMPACT_MARGIN, width() - 2 * COMPACT_MARGIN, height() - 2 * COMPACT_MARGIN);
		const int bh = chatBar->idealHeight();
		chatBar->setGeometry(COMPACT_MARGIN, height() - COMPACT_MARGIN - bh, width() - 2 * COMPACT_MARGIN, bh);
		return;
	}
	const int w = std::min(width() - 2 * MARGIN, panel->hasView() ? FULL_WIDTH : ROSTER_WIDTH);
	const int bh = chatBar->idealHeight();
	const int barY = height() - MARGIN - bh;
	const int above = barY - MARGIN - CHAT_HEIGHT;
	const int h = panel->heightFor(w, above);
	const int x0 = (width() - w) / 2;
	panel->setGeometry(x0, MARGIN + std::max(0, (above - h) / 2), w, h);
	const int barWidth = std::max(w, std::min(width() - 2 * MARGIN, 640));
	chatBar->setGeometry((width() - barWidth) / 2, barY, barWidth, bh);
}

void PregameRoot::showCompactBar(bool shown)
{
	if (shown)
	{
		chatBar->show();
	}
	else
	{
		chatBar->closeQuickChat();
		chatBar->hide();
	}
}

void PregameRoot::openChat()
{
	if (compact)
	{
		showCompactBar(true);
	}
	if (!chatFreeChatEnabled() && chatQuickChatAvailable())
	{
		if (!chatBar->quickChatOpen())
		{
			chatBar->toggleQuickChat();
		}
		return;
	}
	chatBar->startTyping();
}

float PregameRoot::stageFraction() const
{
	const int total = stage == Stage::Planning ? WZ_TEAMPLANNING_COUNTDOWN_MS : WZ_FINAL_COUNTDOWN_MS;
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - stageStart).count();
	return std::max(0.f, std::min(1.f, 1.f - static_cast<float>(elapsed) / static_cast<float>(total)));
}

int PregameRoot::stageSecondsLeft() const
{
	const int total = stage == Stage::Planning ? WZ_TEAMPLANNING_COUNTDOWN_MS : WZ_FINAL_COUNTDOWN_MS;
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - stageStart).count();
	return static_cast<int>((std::max<long long>(0, total - elapsed) + 999) / 1000);
}

void PregameRoot::enterStage(Stage next)
{
	stage = next;
	shownStage = next;
	stageStart = std::chrono::steady_clock::now();
	if (auto view = panel->strategyView())
	{
		// (As with the Classic screen, the plans go to the team when the planning starts, and are fixed as the game starts)
		view->setSending(true);
		view->setReadOnly(next == Stage::Starting);
		view->setFactsLine(panel->factsLine(next));
	}
}

void PregameRoot::run(W_CONTEXT *)
{
	// The stages, as the Classic screen's
	if (!closing)
	{
		const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - stageStart).count();
		switch (stage)
		{
		case Stage::Waiting:
		{
			bool waiting = false;
			for (uint32_t player = 0; player < MAX_CONNECTED_PLAYERS; ++player)
			{
				waiting = waiting || (isHumanPlayer(player) && ingame.JoiningInProgress[player]);
			}
			if (!waiting)
			{
				enterStage(gameHasTeamStrategyView() ? Stage::Planning : Stage::Starting);
			}
			break;
		}
		case Stage::Planning:
			if (elapsed >= WZ_TEAMPLANNING_COUNTDOWN_MS)
			{
				enterStage(Stage::Starting);
			}
			break;
		case Stage::Starting:
			if (elapsed >= WZ_FINAL_COUNTDOWN_MS)
			{
				closing = true;
				auto close = onClose;
				widgScheduleTask([close]() {
					shutdownGameStartScreen();
					if (close)
					{
						close();
					}
				});
			}
			break;
		}
	}

	// The keys: Enter chats, the view's arrows, Space and Backspace, and Escape closes quick chat
	// (the field has them while typing)
	const bool typing = chatBar->isTyping();
	if (!typing && !menuIsOpen() && !chatFinished)
	{
		const bool shift = keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT);
		auto view = panel->strategyView();
		if (keyPressed(KEY_RETURN) || keyPressed(KEY_KPENTER))
		{
			inputClearBuffer();
			openChat();
		}
		else if (keyPressed(KEY_ESC))
		{
			if (compact)
			{
				showCompactBar(false);
			}
			else
			{
				chatBar->closeQuickChat();
			}
		}
		else if (view && stage != Stage::Starting)
		{
			const KEY_CODE back = compact ? KEY_UPARROW : KEY_LEFTARROW;
			const KEY_CODE forward = compact ? KEY_DOWNARROW : KEY_RIGHTARROW;
			if (keyPressed(back))
			{
				view->moveFocus(-1);
			}
			else if (keyPressed(forward))
			{
				view->moveFocus(1);
			}
			else if (keyPressed(KEY_SPACE))
			{
				view->stepFocus(!shift);
			}
			else if (keyPressed(KEY_BACKSPACE))
			{
				view->stepFocus(false);
			}
		}
	}
	chatFinished = false;
	// (As with the Classic screen, the game's keys wait until it starts)
	inputLoseFocus();
}

void PregameRoot::display(int xOffset, int yOffset)
{
	const float x0 = static_cast<float>(xOffset + x());
	const float y0 = static_cast<float>(yOffset + y());
	pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), pal_RGBA(2, 4, 6, 238));
}

void ChatLinesLayer::display(int xOffset, int yOffset)
{
	root->displayChatLines(xOffset + x(), yOffset + y());
}

void PregameRoot::displayChatLines(int xOffset, int yOffset)
{
	// The chat lines: above the bar (fading after a while, all while typing), or compact,
	// the latest over the panel's bottom
	const HudColors &colors = skin().colors();
	const bool all = chatBar->isTyping() || chatBar->quickChatOpen();
	std::vector<ChatLine *> shown;
	for (auto it = lines.rbegin(); it != lines.rend() && shown.size() < static_cast<size_t>(compact ? 2 : CHAT_LINES); ++it)
	{
		const uint32_t age = realTime - it->time;
		if (all || age < (compact ? TOAST_SHOWN_MS : CHAT_SHOWN_MS) + CHAT_FADE_MS)
		{
			shown.push_back(&*it);
		}
	}
	if (shown.empty())
	{
		return;
	}
	int bottom = 0;
	int left = 0;
	int availableWidth = 0;
	if (compact)
	{
		bottom = chatBar->visible() ? chatBar->y() - 6 : height() - COMPACT_MARGIN - LEGEND_COMPACT - 4;
		left = COMPACT_MARGIN + 8;
		availableWidth = width() - 2 * COMPACT_MARGIN - 120;
	}
	else
	{
		bottom = chatBar->y() - 6;
		left = chatBar->x() + 4;
		availableWidth = chatBar->width() - 8;
		const int fit = (bottom - (panel->y() + panel->height() + 6)) / CHAT_LINE;
		if (fit < static_cast<int>(shown.size()))
		{
			shown.resize(static_cast<size_t>(std::max(0, fit)));
		}
	}
	int y = bottom;
	for (ChatLine *line : shown)
	{
		const uint32_t age = realTime - line->time;
		const uint32_t shownFor = compact ? TOAST_SHOWN_MS : CHAT_SHOWN_MS;
		float fade = 1.f;
		if (!all && age > shownFor)
		{
			fade = std::max(0.f, 1.f - static_cast<float>(age - shownFor) / CHAT_FADE_MS);
		}
		const auto faded = [fade](PIELIGHT color) {
			color.byte.a = static_cast<uint8_t>(color.byte.a * fade);
			return color;
		};
		y -= CHAT_LINE;
		const int x0 = left + static_cast<int>(xOffset);
		const int ty = y + static_cast<int>(yOffset);
		const int cy = ty + CHAT_LINE / 2;
		const bool fromPlayer = line->sender >= 0 && line->sender < MAX_PLAYERS;

		// Its parts' widths: the chip, the channel's tag (Team, none for everyone), the name, the text
		std::string tag;
		if (!line->name.empty() && !line->channel.empty() && line->channel != _("Global"))
		{
			tag = line->team ? std::string(_("Team")) : line->channel;
		}
		int partsWidth = (fromPlayer ? 22 : 0) + (tag.empty() ? 0 : tagWidth(line->tagText, tag) + 6);
		if (!line->name.empty())
		{
			line->nameText.setText(WzString::fromUtf8(line->name + ":"), font_regular_bold);
			partsWidth += line->nameText.width() + 5;
		}
		line->rendered.setText(WzString::fromUtf8(line->text), font_regular, std::max(40, availableWidth - partsWidth));
		if (compact)
		{
			pie_DrawRoundedBox(static_cast<float>(x0 - 6), static_cast<float>(ty), static_cast<float>(x0 + partsWidth + line->rendered.width() + 6),
			                   static_cast<float>(ty + CHAT_LINE - 2), faded(pal_RGBA(8, 11, 16, 225)), 4.f, faded(pal_RGBA(120, 150, 190, 60)), 1.f);
		}
		int tx = x0;
		if (fromPlayer)
		{
			const WzRect chip(tx, cy - 8 - 1, 16, 16);
			drawPositionChip(lineChip, static_cast<unsigned>(line->sender), chip);
			if (fade < 1.f)
			{
				dimRect(chip, static_cast<uint8_t>(255 * (1.f - fade)));
			}
			tx += 22;
		}
		if (!tag.empty())
		{
			tx += drawTag(line->tagText, tag, tx, cy - 1, faded(line->team ? colors.accent : colors.textSecondary), false) + 6;
		}
		const float baseline = std::round(cy - line->rendered.lineSize() / 2.f - line->rendered.aboveBase()) - 1.f;
		if (!line->name.empty())
		{
			skin().drawOverlayText(line->nameText, static_cast<float>(tx), baseline, faded(colors.textPrimary));
			tx += line->nameText.width() + 5;
		}
		skin().drawOverlayText(line->rendered, static_cast<float>(tx), baseline, faded(line->sender < 0 ? colors.textSecondary : colors.textPrimary));
	}
}

}

bool hudPregameScreenEnabled()
{
	return bMultiPlayer && currentHudSkin().drawsPregameScreen();
}

std::shared_ptr<W_SCREEN> hudMakePregameScreen(std::function<void ()> onClose)
{
	auto screen = W_SCREEN::make();
	auto root = std::make_shared<PregameRoot>();
	screen->psForm->attach(root);
	root->initialize(std::move(onClose));
	root->callCalcLayout();
	return screen;
}
