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

#include "playersview.h"
#include "intelkit.h"

#include "../hudtext.h"

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/netplay/netplay.h"
#include "lib/widget/scrollablelist.h"

#include "../../ai.h"
#include "../../game_world.h"
#include "../../levels.h"
#include "../../droid.h"
#include "../../multigifts.h"
#include "../../multimenu.h"
#include "../../multiplay.h"
#include "../../multivote.h"
#include "../../objmem.h"
#include "../../power.h"

#include <algorithm>

using namespace intelkit;

namespace
{

constexpr int FACTS_HEIGHT = 34;
constexpr int HEADER_HEIGHT = 28;
constexpr int ROW_HEIGHT = 36;
constexpr int ROW_HEIGHT_COMPACT = 44;
constexpr int RAIL_WIDTH = 248;
constexpr int RAIL_MIN_PANEL_WIDTH = 1060;
constexpr int RAIL_ROW = 42;
constexpr int RAIL_ROWS = 6;
constexpr int ACTION = 26;
constexpr int ACTION_COMPACT = 32;
constexpr int ACTION_GAP = 4;
constexpr int PAD = 16;
constexpr int PAD_COMPACT = 8;
constexpr int SCROLLBAR = 9;
constexpr int CHIP = 20;
constexpr int CHIP_COMPACT = 24;
constexpr uint32_t LATEST_REFRESH_MS = 1000;

std::string allianceModeText()
{
	switch (game.alliance)
	{
	case NO_ALLIANCES: return _("no alliances");
	case ALLIANCES: return _("alliances allowed");
	case ALLIANCES_TEAMS: return _("locked teams");
	case ALLIANCES_UNSHARED: return _("locked teams, no shared research");
	}
	return std::string();
}

std::string lastColumnTitle()
{
	switch (multiMenuLastColumn())
	{
	case MultiMenuLastColumn::Power: return _("Power");
	case MultiMenuLastColumn::Ping: return _("Ping");
	case MultiMenuLastColumn::Structures: return _("Structures");
	}
	return std::string();
}

unsigned selectedDroidCount()
{
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return 0;
	}
	unsigned count = 0;
	for (const DROID *droid : gameWorld.objects.droids[selectedPlayer])
	{
		count += droid->selected ? 1 : 0;
	}
	return count;
}

bool isAI(uint32_t player)
{
	return player < MAX_PLAYERS && !isHumanPlayer(player) && NetPlay.players[player].difficulty != AIDifficulty::HUMAN && NetPlay.players[player].ai >= 0;
}

uint8_t allianceState(uint32_t player)
{
	return (selectedPlayer < MAX_PLAYERS && player < MAX_PLAYERS) ? alliances[selectedPlayer][player] : ALLIANCE_BROKEN;
}

/// The actions' slots, right to left: "…", mute, channel, and Give when the game allows giving
int actionSlots(bool compact)
{
	return compact ? 1 : (alliancesCanGiveAnything(game.alliance) ? 4 : 3);
}

/// Whether a row's "…" menu has anything in it (compact: the gifts, the channel and mute too)
bool moreMenuHasItems(uint32_t player, bool compact)
{
	if (compact && (multiMenuHasAllianceControls(player) || multiMenuHasChannel(player) || multiMenuHasMute(player)))
	{
		return true;
	}
	const uint8_t allianceStatus = allianceState(player);
	return (multiMenuCanChangeAlliance(player) && (allianceStatus == ALLIANCE_REQUESTED || allianceStatus == ALLIANCE_FORMED))
		|| (multiMenuHasChannel(player) && multiMenuCanStartKickVote(player));
}

/// One player's row: the chip, name and tags, the alliance, the numbers, and the actions
class PlayerRow : public W_BUTTON
{
public:
	PlayerRow(const std::shared_ptr<HudPlayersView> &view, uint32_t player): view(view), player(player) {}

	void initialize()
	{
		auto strongView = view.lock();
		const bool compact = strongView->isCompact();
		const int action = compact ? ACTION_COMPACT : ACTION;
		std::weak_ptr<HudPlayersView> weakView = view;
		const uint32_t who = player;

		allianceButton = IntelButton::make(_("Request"), "", false, compact ? 30 : 24);
		allianceButton->addOnClickHandler([who](W_BUTTON &) {
			widgScheduleTask([who]() {
				const uint8_t allianceStatus = allianceState(who);
				if ((allianceStatus == ALLIANCE_BROKEN || allianceStatus == ALLIANCE_INVITATION) && multiMenuCanChangeAlliance(who))
				{
					multiMenuToggleAlliance(who);
				}
			});
		});
		attach(allianceButton);

		if (!compact)
		{
			give = IntelButton::make("", "gift", false, action);
			give->setTip(_("Give"));
			give->addOnClickHandler([weakView, who](W_BUTTON &button) {
				if (auto strong = weakView.lock())
				{
					strong->openGiveMenu(who, button.shared_from_this());
				}
			});
			attach(give);
			channel = IntelButton::make("", "channel", false, action);
			channel->addOnClickHandler([who](W_BUTTON &) {
				openchannels[who] = !openchannels[who];
			});
			attach(channel);
			mute = IntelButton::make("", "volume", false, action);
			mute->addOnClickHandler([who](W_BUTTON &) {
				setPlayerMuted(who, !ingame.muteChat[who]);
			});
			attach(mute);
		}
		more = IntelButton::make("", "more", false, action);
		more->setTip(_("More"));
		more->addOnClickHandler([weakView, who](W_BUTTON &button) {
			if (auto strong = weakView.lock())
			{
				strong->openMoreMenu(who, button.shared_from_this());
			}
		});
		attach(more);
		update();
	}

	uint32_t playerIndex() const { return player; }

	void update()
	{
		const uint8_t allianceStatus = allianceState(player);
		const bool canChange = multiMenuCanChangeAlliance(player);
		const bool request = canChange && allianceStatus == ALLIANCE_BROKEN;
		const bool accept = canChange && allianceStatus == ALLIANCE_INVITATION;
		if (request || accept)
		{
			allianceButton->show();
			allianceButton->primary = accept;
			allianceButton->iconName = accept ? "alliance" : "";
			allianceButton->setLabel(accept ? _("Accept alliance") : _("Request"));
		}
		else
		{
			allianceButton->hide();
		}
		if (give)
		{
			showIf(give, multiMenuHasAllianceControls(player));
			if (multiMenuShowsGifts(player))
			{
				give->setState(0);
			}
			else
			{
				give->setState(WBUT_DISABLE);
			}
		}
		if (channel)
		{
			showIf(channel, multiMenuHasChannel(player));
			const bool open = openchannels[player];
			channel->iconName = open ? "channel" : "channel-off";
			channel->setTip(open ? _("Chat channel: open") : _("Chat channel: closed"));
		}
		if (mute)
		{
			showIf(mute, multiMenuHasMute(player));
			const bool muted = player < MAX_CONNECTED_PLAYERS && ingame.muteChat[player];
			mute->iconName = muted ? "volume-off" : "volume";
			mute->setTip(muted ? _("Chat muted") : _("Mute chat"));
		}
		showIf(more, player != selectedPlayer);
		if (moreMenuHasItems(player, view.lock() && view.lock()->isCompact()))
		{
			more->setState(0);
		}
		else
		{
			more->setState(WBUT_DISABLE);
		}
	}

	void geometryChanged() override
	{
		auto strongView = view.lock();
		if (!strongView || !allianceButton)
		{
			return;
		}
		const HudPlayersView::Columns &c = strongView->columns();
		const bool compact = strongView->isCompact();
		const int action = compact ? ACTION_COMPACT : ACTION;
		const int buttonHeight = allianceButton->height();
		allianceButton->setGeometry(c.alliance, (height() - buttonHeight) / 2, std::min(allianceButton->idealWidth(), c.allianceWidth), buttonHeight);
		int x0 = c.actions + actionSlots(compact) * (action + ACTION_GAP) - ACTION_GAP;
		for (auto &button : {more, mute, channel, give})
		{
			if (!button)
			{
				continue;
			}
			x0 -= action;
			button->setGeometry(x0, (height() - action) / 2, action, action);
			x0 -= ACTION_GAP;
		}
	}

	void run(W_CONTEXT *) override
	{
		update();
	}

	void display(int xOffset, int yOffset) override
	{
		auto strongView = view.lock();
		if (!strongView)
		{
			return;
		}
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		const HudColors &colors = skin().colors();
		const bool compact = strongView->isCompact();
		const bool you = player == selectedPlayer;
		const HudPlayersView::Columns &c = strongView->columns();
		if (you)
		{
			pie_UniTransBoxFill(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()), alpha(colors.accent, 22));
			pie_BoxFill(rect.x(), rect.y() + 1, rect.x() + 3, rect.bottom() - 1, colors.accent);
		}
		else if (strongView->isFocused(player) || isHighlighted())
		{
			skin().drawCard(rect, HudCardState::Highlighted);
		}
		else if ((index % 2) == 1)
		{
			pie_UniTransBoxFill(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()), pal_RGBA(255, 255, 255, 7));
		}
		const bool dead = multiMenuPlayerIsDead(player) && !NetPlay.players[player].isSpectator;
		const PIELIGHT textColor = dead ? colors.stateDisabled : colors.textPrimary;
		const int chip = compact ? CHIP_COMPACT : CHIP;
		const int centerY = rect.y() + rect.height() / 2;
		drawPositionChip(chipText, player, WzRect(rect.x() + c.name, centerY - chip / 2, chip, chip));
		int x0 = rect.x() + c.name + chip + 10;
		const iV_fonts nameFont = you ? font_regular_bold : font_regular;
		const int lineTop = centerY - static_cast<int>(iV_GetTextLineSize(nameFont)) / 2;
		std::string tag;
		PIELIGHT tagColor = colors.textSecondary;
		if (you)
		{
			tag = _("You");
			tagColor = colors.accent;
		}
		else if (player < MAX_CONNECTED_PLAYERS && NetPlay.players[player].isSpectator)
		{
			tag = _("Spectator");
		}
		else if (isAI(player))
		{
			tag = _("AI");
		}
		const int tagColumnWidth = tag.empty() ? 0 : tagWidth(tagText, tag) + 8;
		const int maxNameWidth = std::max(20, rect.x() + (c.allianceWidth > 0 ? c.alliance : c.scoreRight - 70) - 12 - x0 - tagColumnWidth);
		const std::string name = getPlayerName(player);
		x0 += drawLine(nameText, name, nameFont, x0, lineTop, maxNameWidth, textColor) + 8;
		if (!tag.empty())
		{
			drawTag(tagText, tag, x0, centerY, tagColor, false);
		}

		// The alliance, in words, where it isn't a button
		if (c.allianceWidth > 0 && !allianceButton->visible())
		{
			const uint8_t allianceStatus = allianceState(player);
			if (!you && multiMenuHasAllianceControls(player))
			{
				if (allianceStatus == ALLIANCE_FORMED)
				{
					const std::string allied = _("Allied");
					const int alliedWidth = drawTag(allianceText, allied, rect.x() + c.alliance, centerY, colors.stateOk, false, compact ? font_regular : font_small);
					if (!compact)
					{
						skin().drawNamedIcon("alliance", WzRect(rect.x() + c.alliance + alliedWidth + 8, centerY - 8, 16, 16), colors.stateOk);
					}
				}
				else if (allianceStatus == ALLIANCE_REQUESTED)
				{
					drawTag(allianceText, _("Request sent"), rect.x() + c.alliance, centerY, colors.textSecondary, false, compact ? font_regular : font_small);
				}
			}
		}

		if (player >= MAX_PLAYERS)
		{
			return;
		}
		const iV_fonts numberFont = compact ? font_regular : font_regular;
		const int numberTop = centerY - static_cast<int>(iV_GetTextLineSize(numberFont)) / 2;
		drawLine(score, multiMenuScoreText(player), numberFont, rect.x() + c.scoreRight, numberTop, 90, textColor, true);
		if (c.killsRight > 0)
		{
			drawLine(kills, multiMenuKillsText(player), numberFont, rect.x() + c.killsRight, numberTop, 60, textColor, true);
		}
		drawLine(units, multiMenuUnitsText(player), numberFont, rect.x() + c.unitsRight, numberTop, 60, textColor, true);
		std::string last = multiMenuLastColumnText(player);
		PIELIGHT lastColor = textColor;
		if (!last.empty() && multiMenuLastColumn() == MultiMenuLastColumn::Ping)
		{
			const int ping = ingame.PingTimes[player];
			lastColor = ping >= PING_LIMIT || ping >= 500 ? colors.stateBad : (ping >= 200 ? colors.stateWarn : colors.stateOk);
			if (ping < PING_LIMIT)
			{
				last = astringf(_("%s ms"), last.c_str());
			}
		}
		drawLine(lastText, last, numberFont, rect.x() + c.last, numberTop, std::max(10, c.actions - c.last - 8), lastColor);
	}

	size_t index = 0;

private:
	static void showIf(const std::shared_ptr<IntelButton> &button, bool shown)
	{
		if (shown)
		{
			button->show();
		}
		else
		{
			button->hide();
		}
	}

	std::weak_ptr<HudPlayersView> view;
	uint32_t player;
	std::shared_ptr<IntelButton> allianceButton;
	std::shared_ptr<IntelButton> give;
	std::shared_ptr<IntelButton> channel;
	std::shared_ptr<IntelButton> mute;
	std::shared_ptr<IntelButton> more;
	ChipText chipText;
	WzText nameText;
	WzText tagText;
	WzText allianceText;
	WzText score;
	WzText kills;
	WzText units;
	WzText lastText;
};

/// A row of the latest intelligence: the item's picture, title, kind and time, and the unread mark
class RailRow : public W_BUTTON
{
public:
	RailRow(const std::shared_ptr<HudPlayersView> &view, const std::vector<HudIntelEntry> *latest, size_t index): view(view), latest(latest), index(index)
	{
		addOnClickHandler([](W_BUTTON &button) {
			auto &row = static_cast<RailRow &>(button);
			auto strongView = row.view.lock();
			if (strongView && row.index < row.latest->size() && strongView->onOpenIntelligence)
			{
				const HudIntelEntry &entry = (*row.latest)[row.index];
				auto open = strongView->onOpenIntelligence;
				const HudIntelKind kind = entry.kind;
				const std::string key = entry.key;
				widgScheduleTask([open, kind, key]() { open(kind, key); });
			}
		});
	}

	void display(int xOffset, int yOffset) override
	{
		if (index >= latest->size())
		{
			return;
		}
		const HudIntelEntry &entry = (*latest)[index];
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		if (isHighlighted())
		{
			skin().drawCard(rect, HudCardState::Highlighted);
		}
		drawThumb(entry, WzRect(rect.x() + 4, rect.y() + 4, 36, rect.height() - 8));
		const HudColors &colors = skin().colors();
		const int textX = rect.x() + 50;
		const int textWidth = rect.right() - 16 - textX;
		drawLine(title, entry.title, entry.unread ? font_regular_bold : font_regular, textX, rect.y() + 4, textWidth, colors.textPrimary);
		std::string sub = entrySubtitle(entry);
		const std::string time = timeText(entry);
		if (!time.empty())
		{
			sub += " · " + time;
		}
		drawLine(subtitle, sub, font_small, textX, rect.y() + 23, rect.right() - 6 - textX, colors.textSecondary);
		if (entry.unread)
		{
			drawUnreadDot(rect.right() - 7, rect.y() + 12);
		}
	}

private:
	std::weak_ptr<HudPlayersView> view;
	const std::vector<HudIntelEntry> *latest;
	size_t index;
	WzText title;
	WzText subtitle;
};

}

std::shared_ptr<HudPlayersView> HudPlayersView::make(bool compact, bool withLatestIntelligence)
{
	auto view = std::make_shared<HudPlayersView>();
	view->initialize(compact, withLatestIntelligence);
	return view;
}

void HudPlayersView::initialize(bool compactLayout, bool withLatestIntelligence)
{
	compact = compactLayout;
	withRail = withLatestIntelligence && !compact;
	list = ScrollableListWidget::make();
	list->setScrollbarWidth(SCROLLBAR);
	list->setScrollbarDisplay(displayScrollbar);
	list->setExpandWhenScrollbarInvisible(false);
	attach(list);
	if (withRail)
	{
		latest = hudIntelEntries();
		auto self = std::static_pointer_cast<HudPlayersView>(shared_from_this());
		for (size_t i = 0; i < RAIL_ROWS; ++i)
		{
			auto row = std::make_shared<RailRow>(self, &latest, i);
			railRows.push_back(row);
			attach(row);
		}
		openIntelligence = IntelButton::make(_("Open intelligence"), "intelligence", false, 28);
		std::weak_ptr<HudPlayersView> weakSelf = self;
		openIntelligence->addOnClickHandler([weakSelf](W_BUTTON &) {
			auto strong = weakSelf.lock();
			if (strong && strong->onOpenIntelligence)
			{
				auto open = strong->onOpenIntelligence;
				widgScheduleTask([open]() { open(HudIntelKind::Research, std::string()); });
			}
		});
		attach(openIntelligence);
	}
	rebuildRows();
}

void HudPlayersView::rebuildRows()
{
	players.clear();
	for (uint32_t player = 0; player < MAX_CONNECTED_PLAYERS; ++player)
	{
		if (multiMenuListsPlayer(player))
		{
			players.push_back(player);
		}
	}
	// (As the menu: in the order of their positions)
	std::stable_sort(players.begin(), players.end(), [](uint32_t a, uint32_t b) {
		return NetPlay.players[a].position < NetPlay.players[b].position;
	});
	list->clear();
	rows.clear();
	auto self = std::static_pointer_cast<HudPlayersView>(shared_from_this());
	const int rowHeight = compact ? ROW_HEIGHT_COMPACT : ROW_HEIGHT;
	for (size_t i = 0; i < players.size(); ++i)
	{
		auto row = std::make_shared<PlayerRow>(self, players[i]);
		row->index = i;
		row->initialize();
		row->setGeometry(0, 0, std::max(1, list->width() - SCROLLBAR), rowHeight);
		std::weak_ptr<HudPlayersView> weakSelf = self;
		const size_t index = i;
		row->addOnClickHandler([weakSelf, index](W_BUTTON &) {
			if (auto strong = weakSelf.lock())
			{
				strong->focus = index;
			}
		});
		rows.push_back(row);
		list->addItem(row);
	}
	if (focus.has_value() && *focus >= rows.size())
	{
		focus.reset();
	}
	layoutChildren();
}

int HudPlayersView::heightFor(int width) const
{
	const int rowHeight = compact ? ROW_HEIGHT_COMPACT : ROW_HEIGHT;
	const int count = static_cast<int>(std::max<size_t>(players.size(), 1));
	if (compact)
	{
		return HEADER_HEIGHT + count * rowHeight + PAD_COMPACT;
	}
	int height = FACTS_HEIGHT + HEADER_HEIGHT + count * rowHeight + PAD;
	if (withRail && width >= RAIL_MIN_PANEL_WIDTH)
	{
		height = std::max(height, 12 + 24 + RAIL_ROWS * RAIL_ROW + 12 + 28 + PAD);
	}
	return height;
}

WzRect HudPlayersView::tableRect() const
{
	const int pad = compact ? PAD_COMPACT : PAD;
	const bool rail = withRail && width() >= RAIL_MIN_PANEL_WIDTH;
	const int right = rail ? width() - RAIL_WIDTH : width() - pad;
	const int top = compact ? 4 : FACTS_HEIGHT;
	return WzRect(pad, top, std::max(0, right - pad - (rail ? 10 : 0)), std::max(0, height() - top - (compact ? PAD_COMPACT : PAD)));
}

WzRect HudPlayersView::railRect() const
{
	return WzRect(width() - RAIL_WIDTH, 0, RAIL_WIDTH, height());
}

void HudPlayersView::layoutChildren()
{
	const WzRect table = tableRect();
	const int rowWidth = std::max(0, table.width() - SCROLLBAR);
	// The columns, from the right: the actions, the last column, Units, Kills, Score, then the alliance
	const int action = compact ? ACTION_COMPACT : ACTION;
	Columns c;
	c.name = compact ? 8 : 12;
	c.actions = rowWidth - (compact ? 6 : 10) - actionSlots(compact) * (action + ACTION_GAP) + ACTION_GAP;
	if (compact)
	{
		c.last = c.actions - 12 - 64;
		c.unitsRight = c.last - 16;
		c.scoreRight = c.unitsRight - 60;
		c.killsRight = 0;
		c.allianceWidth = game.alliance == NO_ALLIANCES ? 0 : 150;
		c.alliance = c.scoreRight - 64 - c.allianceWidth;
	}
	else
	{
		c.last = c.actions - 16 - 84;
		c.unitsRight = c.last - 24;
		c.killsRight = c.unitsRight - 64;
		c.scoreRight = c.killsRight - 64;
		c.allianceWidth = game.alliance == NO_ALLIANCES ? 0 : 170;
		c.alliance = c.scoreRight - 84 - c.allianceWidth;
	}
	cols = c;
	list->setGeometry(table.x(), table.y() + HEADER_HEIGHT, table.width(), table.height() - HEADER_HEIGHT);
	const int rowHeight = compact ? ROW_HEIGHT_COMPACT : ROW_HEIGHT;
	for (auto &row : rows)
	{
		row->setGeometry(row->x(), row->y(), rowWidth, rowHeight);
	}
	if (withRail)
	{
		const bool rail = width() >= RAIL_MIN_PANEL_WIDTH;
		const WzRect r = railRect();
		int y0 = 12 + 24;
		for (auto &row : railRows)
		{
			row->setGeometry(r.x() + 12, y0, r.width() - 24, RAIL_ROW);
			y0 += RAIL_ROW;
			if (rail)
			{
				row->show();
			}
			else
			{
				row->hide();
			}
		}
		openIntelligence->setGeometry(r.x() + 12, height() - PAD - 28, openIntelligence->idealWidth(), 28);
		if (rail)
		{
			openIntelligence->show();
		}
		else
		{
			openIntelligence->hide();
		}
	}
	if (confirm)
	{
		confirm->setGeometry(0, 0, width(), height());
	}
}

void HudPlayersView::geometryChanged()
{
	layoutChildren();
}

void HudPlayersView::run(W_CONTEXT *)
{
	// Players come and go (ex. a human leaves a game)
	size_t listed = 0;
	bool changed = false;
	for (uint32_t player = 0; player < MAX_CONNECTED_PLAYERS; ++player)
	{
		if (multiMenuListsPlayer(player))
		{
			changed = changed || listed >= players.size() || std::find(players.begin(), players.end(), player) == players.end();
			++listed;
		}
	}
	if (changed || listed != players.size())
	{
		widgScheduleTask([weakSelf = std::weak_ptr<HudPlayersView>(std::static_pointer_cast<HudPlayersView>(shared_from_this()))]() {
			if (auto strong = weakSelf.lock())
			{
				strong->rebuildRows();
			}
		});
	}
	if (withRail && realTime - latestRefreshedAt >= LATEST_REFRESH_MS)
	{
		latest = hudIntelEntries();
		latestRefreshedAt = realTime;
	}
}

void HudPlayersView::display(int xOffset, int yOffset)
{
	const HudColors &colors = skin().colors();
	const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
	const WzRect table = tableRect();
	const int tx = rect.x() + table.x();
	const int ty = rect.y() + table.y();
	if (!compact)
	{
		size_t count = 0;
		for (uint32_t player : players)
		{
			count += (player < MAX_PLAYERS && !NetPlay.players[player].isSpectator) ? 1 : 0;
		}
		const std::string facts = astringf(_("%zu players · %s · %s"), count, allianceModeText().c_str(), hudFormatDuration(gameTime / GAME_TICKS_PER_SEC).c_str());
		drawLine(factsText, facts, font_small, tx, rect.y() + 10, table.width(), colors.textSecondary);
	}
	const int headTop = ty + (HEADER_HEIGHT - static_cast<int>(iV_GetTextLineSize(font_small))) / 2;
	const PIELIGHT headColor = colors.textSecondary;
	drawLine(heads[0], _("Player"), font_small, tx + cols.name, headTop, 120, headColor);
	if (cols.allianceWidth > 0)
	{
		drawLine(heads[1], _("Alliance"), font_small, tx + cols.alliance, headTop, cols.allianceWidth, headColor);
	}
	drawLine(heads[2], _("Score"), font_small, tx + cols.scoreRight, headTop, 80, headColor, true);
	if (cols.killsRight > 0)
	{
		drawLine(heads[3], _("Kills"), font_small, tx + cols.killsRight, headTop, 60, headColor, true);
	}
	drawLine(heads[4], _("Units"), font_small, tx + cols.unitsRight, headTop, 60, headColor, true);
	drawLine(heads[5], lastColumnTitle(), font_small, tx + cols.last, headTop, std::max(10, cols.actions - cols.last - 4), headColor);
	drawDivider(tx, ty + HEADER_HEIGHT - 1, tx + table.width(), ty + HEADER_HEIGHT);

	if (withRail && width() >= RAIL_MIN_PANEL_WIDTH)
	{
		const WzRect r = railRect();
		drawDivider(rect.x() + r.x(), rect.y(), rect.x() + r.x() + 1, rect.bottom());
		drawLine(railTitle, _("LATEST INTELLIGENCE"), font_small, rect.x() + r.x() + 16, rect.y() + 14, r.width() - 32, headColor);
		if (latest.empty())
		{
			drawLine(noneText, _("No intelligence yet"), font_small, rect.x() + r.x() + 16, rect.y() + 44, r.width() - 32, headColor);
		}
	}
}

bool HudPlayersView::isFocused(uint32_t player) const
{
	return focus.has_value() && *focus < players.size() && players[*focus] == player;
}

void HudPlayersView::moveFocus(int delta)
{
	if (players.empty())
	{
		return;
	}
	const int last = static_cast<int>(players.size()) - 1;
	const int current = focus.has_value() ? static_cast<int>(*focus) : (delta > 0 ? -1 : last + 1);
	focus = static_cast<size_t>(std::max(0, std::min(last, current + delta)));
	list->scrollEnsureItemVisible(*focus);
}

void HudPlayersView::actOnFocus()
{
	if (!focus.has_value() || *focus >= rows.size() || players[*focus] == selectedPlayer || !moreMenuHasItems(players[*focus], compact))
	{
		return;
	}
	openMoreMenu(players[*focus], rows[*focus]);
}

bool HudPlayersView::backOut()
{
	if (confirm)
	{
		confirm->cancel();
		confirm.reset();
		return true;
	}
	return false;
}

void HudPlayersView::openGiveMenu(uint32_t player, const std::shared_ptr<WIDGET> &anchor)
{
	if (!multiMenuShowsGifts(player))
	{
		return;
	}
	auto menu = IntelMenu::make(compact ? 40 : 34, 300);
	const uint8_t to = static_cast<uint8_t>(player);
	IntelMenu::Item item;
	item.icon = "power";
	item.label = _("Give power");
	item.detail = astringf(_("a third: %d"), static_cast<int>(getPower(selectedPlayer) / 3));
	item.onChosen = [to]() { sendGift(POWER_GIFT, to); };
	menu->addItem(item);
	const unsigned selected = selectedDroidCount();
	item = IntelMenu::Item();
	item.icon = "units";
	item.label = _("Hand over the selected units");
	item.detail = astringf(ngettext("%u selected", "%u selected", selected), selected);
	item.enabled = selected > 0;
	item.onChosen = [to]() { sendGift(DROID_GIFT, to); };
	menu->addItem(item);
	if (multiMenuCanGiveResearchAndRadar())
	{
		item = IntelMenu::Item();
		item.icon = "visibility";
		item.label = _("Give a visibility report");
		item.onChosen = [to]() { sendGift(RADAR_GIFT, to); };
		menu->addItem(item);
		item = IntelMenu::Item();
		item.icon = "technology";
		item.label = _("Leak technology documents");
		item.onChosen = [to]() { sendGift(RESEARCH_GIFT, to); };
		menu->addItem(item);
	}
	menu->open(anchor, true);
}

void HudPlayersView::openMoreMenu(uint32_t player, const std::shared_ptr<WIDGET> &anchor)
{
	auto menu = IntelMenu::make(compact ? 40 : 34, compact ? 320 : 260);
	std::weak_ptr<HudPlayersView> weakSelf = std::static_pointer_cast<HudPlayersView>(shared_from_this());
	const uint8_t allianceStatus = allianceState(player);
	const bool chipLarge = compact;
	auto chipText = std::make_shared<ChipText>();
	auto name = std::make_shared<WzText>();
	auto tag = std::make_shared<WzText>();
	menu->setHeading(compact ? 44 : 38, [player, chipLarge, chipText, name, tag](const WzRect &rect) {
		const int chip = chipLarge ? CHIP_COMPACT : CHIP;
		const int cy = rect.y() + rect.height() / 2;
		drawPositionChip(*chipText, player, WzRect(rect.x() + 12, cy - chip / 2, chip, chip));
		const int x0 = rect.x() + 12 + chip + 10;
		const std::string playerName = getPlayerName(player);
		const iV_fonts font = font_regular_bold;
		const int w = drawLine(*name, playerName, font, x0, cy - static_cast<int>(iV_GetTextLineSize(font)) / 2, rect.width() - (x0 - rect.x()) - 90, skin().colors().textPrimary);
		if (allianceState(player) == ALLIANCE_FORMED && multiMenuHasAllianceControls(player))
		{
			drawTag(*tag, _("Allied"), x0 + w + 10, cy, skin().colors().stateOk, false);
		}
	});
	IntelMenu::Item item;
	if (compact)
	{
		if (multiMenuHasAllianceControls(player))
		{
			const bool ally = multiMenuShowsGifts(player);
			const uint8_t to = static_cast<uint8_t>(player);
			item.icon = "power";
			item.label = _("Give power");
			item.detail = astringf(_("a third: %d"), static_cast<int>(getPower(selectedPlayer) / 3));
			item.enabled = ally;
			item.onChosen = [to]() { sendGift(POWER_GIFT, to); };
			menu->addItem(item);
			const unsigned selected = selectedDroidCount();
			item = IntelMenu::Item();
			item.icon = "units";
			item.label = _("Hand over the selected units");
			item.detail = astringf(ngettext("%u selected", "%u selected", selected), selected);
			item.enabled = ally && selected > 0;
			item.onChosen = [to]() { sendGift(DROID_GIFT, to); };
			menu->addItem(item);
			if (multiMenuCanGiveResearchAndRadar())
			{
				item = IntelMenu::Item();
				item.icon = "visibility";
				item.label = _("Give a visibility report");
				item.enabled = ally;
				item.onChosen = [to]() { sendGift(RADAR_GIFT, to); };
				menu->addItem(item);
				item = IntelMenu::Item();
				item.icon = "technology";
				item.label = _("Leak technology documents");
				item.enabled = ally;
				item.onChosen = [to]() { sendGift(RESEARCH_GIFT, to); };
				menu->addItem(item);
			}
			menu->addSeparator();
		}
		if (multiMenuHasChannel(player))
		{
			item = IntelMenu::Item();
			item.icon = "channel";
			item.label = _("Chat channel");
			item.toggle = openchannels[player];
			item.onChosen = [player]() { openchannels[player] = !openchannels[player]; };
			menu->addItem(item);
		}
		if (multiMenuHasMute(player))
		{
			item = IntelMenu::Item();
			item.icon = "volume";
			item.label = _("Mute chat");
			item.toggle = ingame.muteChat[player];
			item.onChosen = [player]() { setPlayerMuted(player, !ingame.muteChat[player]); };
			menu->addItem(item);
		}
	}
	const bool canChange = multiMenuCanChangeAlliance(player);
	std::vector<IntelMenu::Item> last;
	if (canChange && allianceStatus == ALLIANCE_REQUESTED)
	{
		item = IntelMenu::Item();
		item.icon = "alliance-broken";
		item.label = _("Cancel the alliance request");
		item.onChosen = [player]() {
			if (allianceState(player) == ALLIANCE_REQUESTED && multiMenuCanChangeAlliance(player))
			{
				multiMenuToggleAlliance(player);
			}
		};
		last.push_back(item);
	}
	if (canChange && allianceStatus == ALLIANCE_FORMED)
	{
		item = IntelMenu::Item();
		item.icon = "alliance-broken";
		item.label = _("Break the alliance…");
		item.danger = true;
		item.onChosen = [weakSelf, player]() {
			if (auto strong = weakSelf.lock())
			{
				strong->askToBreakAlliance(player);
			}
		};
		last.push_back(item);
	}
	if (multiMenuHasChannel(player) && multiMenuCanStartKickVote(player))
	{
		item = IntelMenu::Item();
		item.icon = "kick";
		item.label = _("Start a kick vote");
		item.onChosen = [player]() { startKickVote(player); };
		last.push_back(item);
	}
	if (!last.empty() && !menu->empty())
	{
		menu->addSeparator();
	}
	for (const auto &lastItem : last)
	{
		menu->addItem(lastItem);
	}
	if (menu->empty())
	{
		return;
	}
	menu->open(anchor, true);
}

void HudPlayersView::askToBreakAlliance(uint32_t player)
{
	if (allianceState(player) != ALLIANCE_FORMED || !multiMenuCanChangeAlliance(player))
	{
		return;
	}
	if (confirm)
	{
		confirm->cancel();
	}
	std::weak_ptr<HudPlayersView> weakSelf = std::static_pointer_cast<HudPlayersView>(shared_from_this());
	confirm = IntelConfirm::make(astringf(_("Break the alliance with %s?"), getPlayerName(player)), _("You will no longer be allies."), _("Break the alliance"),
	                             true, [player, weakSelf]() {
		if (allianceState(player) == ALLIANCE_FORMED && multiMenuCanChangeAlliance(player))
		{
			multiMenuToggleAlliance(player);
		}
		if (auto strong = weakSelf.lock())
		{
			strong->confirm.reset();
		}
	});
	attach(confirm);
	confirm->setGeometry(0, 0, width(), height());
}

bool HudPlayersView::applyForTests(const std::string &key, const std::string &value)
{
	const auto rowIndex = [this](const std::string &text) -> nonstd::optional<size_t> {
		const size_t row = static_cast<size_t>(std::strtoul(text.c_str(), nullptr, 10));
		return row < rows.size() ? nonstd::optional<size_t>(row) : nonstd::nullopt;
	};
	if (key == "focus")
	{
		auto row = rowIndex(value);
		focus = row;
		return row.has_value();
	}
	if (key == "confirm")
	{
		auto row = rowIndex(value);
		if (!row.has_value() || allianceState(players[*row]) != ALLIANCE_FORMED)
		{
			return false;
		}
		askToBreakAlliance(players[*row]);
		return confirm != nullptr;
	}
	if (key == "menu")
	{
		const size_t colon = value.find(':');
		auto row = colon == std::string::npos ? nonstd::nullopt : rowIndex(value.substr(colon + 1));
		if (!row.has_value())
		{
			return false;
		}
		const std::string which = value.substr(0, colon);
		const uint32_t player = players[*row];
		// The button the menu opens under: the row's (Give, or "…")
		std::shared_ptr<WIDGET> anchor;
		const auto &children = rows[*row]->children();
		const auto findButton = [&children](const std::string &icon) -> std::shared_ptr<WIDGET> {
			for (const auto &child : children)
			{
				auto button = std::dynamic_pointer_cast<IntelButton>(child);
				if (button && button->iconName == icon && button->visible())
				{
					return button;
				}
			}
			return nullptr;
		};
		if (which == "give" && !compact)
		{
			anchor = findButton("gift");
			if (anchor && multiMenuShowsGifts(player))
			{
				openGiveMenu(player, anchor);
				return true;
			}
			return false;
		}
		if (which == "more")
		{
			anchor = findButton("more");
			if (anchor)
			{
				openMoreMenu(player, anchor);
				return true;
			}
		}
		return false;
	}
	return false;
}
