/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/** @file
 *  Widgets displaying the research log from the game history log
 */

#include "researchlogviewer.h"

#include "lib/framework/frame.h"
#include "lib/widget/label.h"
#include "lib/widget/table.h"
#include "lib/widget/scrollablelist.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/textdraw.h"
#include "gamehistorylogger.h"
#include "research.h"
#include "statsdef.h"
#include "scores.h"
#include "ai.h"
#include "multiplay.h"

#include <algorithm>
#include <map>

constexpr int RESEARCHLOG_PADDING = 6;
constexpr int RESEARCHLOG_TITLE_HEIGHT = 20;
constexpr int RESEARCHLOG_TITLE_GAP = 4;
constexpr int RESEARCHLOG_ROW_HEIGHT = 18;
constexpr int RESEARCHLOG_BADGE_PADDING_X = 4;
constexpr int RESEARCHLOG_COLUMN_PADDING_X = 4;
constexpr int RESEARCHLOG_NAME_MAX_IDEAL_WIDTH = 200;
constexpr int RESEARCHLOG_PANEL_MIN_IDEAL_WIDTH = 160;
constexpr int RESEARCHLOG_PANEL_IDEAL_HEIGHT = 180;
constexpr int RESEARCHLOG_GRID_GAP = 10;
constexpr int RESEARCHLOG_IDEAL_MAX_PER_LINE = 3;

// MARK: - Column defs

std::vector<ResearchLogColumnDef> buildResearchLogColumnDefs()
{
	const auto& playerAttrs = GameStoryLogger::instance().getFixedPlayerAttributes();

	std::vector<size_t> activePlayers;
	for (size_t player = 0; player < playerAttrs.size(); ++player)
	{
		if (playerAttrs[player].slotWasOccupied && !playerAttrs[player].name.empty())
		{
			activePlayers.push_back(player);
		}
	}

	std::vector<ResearchLogColumnDef> result;

	if (game.alliance == ALLIANCES_TEAMS)
	{
		// locked teams with shared research - every team member has the same research events,
		// so display one log per team, sourced from the first member of each
		std::map<int32_t, std::vector<size_t>> teamMembers;
		for (size_t player : activePlayers)
		{
			teamMembers[playerAttrs[player].team].push_back(player);
		}
		size_t teamIdx = 0;
		for (const auto& team : teamMembers)
		{
			ResearchLogColumnDef def;
			std::string teamLetter(1, static_cast<char>('A' + (teamIdx % 26)));
			def.title = WzString::fromUtf8(astringf(_("Team %s"), teamLetter.c_str()));
			def.color = WZCOL_TEXT_BRIGHT;
			def.sourcePlayerIdx = team.second.front();
			WzString memberNames;
			for (size_t member : team.second)
			{
				if (!memberNames.isEmpty())
				{
					memberNames += "\n";
				}
				memberNames += WzString::fromUtf8(playerAttrs[member].name);
			}
			def.tooltip = memberNames;
			result.push_back(std::move(def));
			++teamIdx;
		}
	}
	else
	{
		for (size_t player : activePlayers)
		{
			ResearchLogColumnDef def;
			def.title = WzString::fromUtf8(playerAttrs[player].name);
			def.color = pal_GetTeamColour(playerAttrs[player].colour);
			def.sourcePlayerIdx = player;
			result.push_back(std::move(def));
		}
	}

	return result;
}

// MARK: - W_ResearchTimeBadge

class W_ResearchTimeBadge : public WIDGET
{
public:
	W_ResearchTimeBadge(const WzString& timeText, PIELIGHT accentColor)
	{
		text.setText(timeText, font_small);
		borderColor = accentColor;
		borderColor.byte.a = 128;
		setGeometry(0, 0, idealWidth(), idealHeight());
	}

	void display(int xOffset, int yOffset) override
	{
		int x0 = xOffset + x();
		int y0 = yOffset + y();
		pie_UniTransBoxFill(x0, y0 + 1, x0 + width(), y0 + height() - 1, pal_RGBA(0, 0, 0, 160));
		iV_Box(x0, y0 + 1, x0 + width(), y0 + height() - 1, borderColor);
		text.render(x0 + (width() - text.width()) / 2, y0 + (height() - text.lineSize()) / 2 - text.aboveBase(), WZCOL_TEXT_BRIGHT);
	}

	int32_t idealWidth() override
	{
		return text.width() + RESEARCHLOG_BADGE_PADDING_X * 2;
	}

	int32_t idealHeight() override
	{
		return RESEARCHLOG_ROW_HEIGHT;
	}

private:
	WzText text;
	PIELIGHT borderColor;
};

// MARK: - PlayerResearchLog

std::shared_ptr<PlayerResearchLog> PlayerResearchLog::make(const ResearchLogColumnDef& def)
{
	auto result = std::make_shared<PlayerResearchLog>();
	result->initialize(def);
	return result;
}

void PlayerResearchLog::initialize(const ResearchLogColumnDef& def)
{
	titleLabel = std::make_shared<W_LABEL>();
	attach(titleLabel);
	titleLabel->setFont(font_regular_bold, def.color);
	titleLabel->setString(def.title);
	titleLabel->setCanTruncate(true);
	if (!def.tooltip.isEmpty())
	{
		titleLabel->setTip(def.tooltip.toUtf8());
	}

	struct RowData
	{
		std::shared_ptr<W_ResearchTimeBadge> badge;
		std::shared_ptr<W_LABEL> nameLabel;
	};
	std::vector<RowData> rowData;
	int maxBadgeWidth = 0;
	int maxNameWidth = 0;
	for (const auto& event : GameStoryLogger::instance().getResearchLog())
	{
		if (event.player < 0 || static_cast<size_t>(event.player) != def.sourcePlayerIdx)
		{
			continue;
		}

		char timeText[20];
		getAsciiTimeN(timeText, sizeof(timeText), event.gameTime);

		// look up the localized research name (fall back to the raw id, e.g. if the log
		// was loaded from a file without the matching research stats loaded)
		WzString researchName = event.researchId;
		if (const RESEARCH* psResearch = getResearch(event.researchId.toUtf8().c_str()))
		{
			researchName = WzString::fromUtf8(getLocalizedStatsName(psResearch));
		}

		RowData row;
		row.badge = std::make_shared<W_ResearchTimeBadge>(WzString::fromUtf8(timeText), def.color);
		row.nameLabel = std::make_shared<W_LABEL>();
		row.nameLabel->setFont(font_small, WZCOL_TEXT_BRIGHT);
		row.nameLabel->setString(researchName);
		row.nameLabel->setCanTruncate(true);
		row.nameLabel->setTip(researchName.toUtf8());
		row.nameLabel->setGeometry(0, 0, row.nameLabel->getMaxLineWidth(), RESEARCHLOG_ROW_HEIGHT);
		maxBadgeWidth = std::max(maxBadgeWidth, row.badge->idealWidth());
		maxNameWidth = std::max(maxNameWidth, row.nameLabel->getMaxLineWidth());
		rowData.push_back(std::move(row));
	}

	if (rowData.empty())
	{
		noEntriesLabel = std::make_shared<W_LABEL>();
		attach(noEntriesLabel);
		noEntriesLabel->setFont(font_small, WZCOL_TEXT_MEDIUM);
		noEntriesLabel->setString(_("(no research)"));
		cachedIdealWidth = std::max({RESEARCHLOG_PANEL_MIN_IDEAL_WIDTH,
		                             titleLabel->getMaxLineWidth() + RESEARCHLOG_PADDING * 2,
		                             noEntriesLabel->getMaxLineWidth() + RESEARCHLOG_PADDING * 2});
		return;
	}

	auto makeColHeaderLabel = [](int initialWidth) {
		auto label = std::make_shared<W_LABEL>();
		label->setGeometry(0, 0, initialWidth, 0);
		return label;
	};
	std::vector<TableColumn> columns;
	columns.push_back({makeColHeaderLabel(maxBadgeWidth), TableColumn::ResizeBehavior::FIXED_WIDTH});
	columns.push_back({makeColHeaderLabel(std::min(maxNameWidth, RESEARCHLOG_NAME_MAX_IDEAL_WIDTH)), TableColumn::ResizeBehavior::RESIZABLE_AUTOEXPAND});
	table = ScrollableTableWidget::make(columns);
	attach(table);
	table->setHeaderVisible(false);
	table->setDrawColumnLines(false);
	table->setColumnPadding(Vector2i(RESEARCHLOG_COLUMN_PADDING_X, 0));
	// note: no column relayout here - the table is still zero-sized, and the column width
	// math underflows; the columns are laid out in geometryChanged once the table has a size
	badgeColumnWidth = maxBadgeWidth;
	nameColumnIdealWidth = std::min(maxNameWidth, RESEARCHLOG_NAME_MAX_IDEAL_WIDTH);

	for (auto& row : rowData)
	{
		table->addRow(TableRow::make({row.badge, row.nameLabel}, RESEARCHLOG_ROW_HEIGHT));
	}

	size_t neededContentWidth = static_cast<size_t>(maxBadgeWidth + std::min(maxNameWidth, RESEARCHLOG_NAME_MAX_IDEAL_WIDTH));
	cachedIdealWidth = std::max<int32_t>(RESEARCHLOG_PANEL_MIN_IDEAL_WIDTH,
	                                     static_cast<int32_t>(table->getTableWidthNeededForTotalColumnWidth(2, neededContentWidth)) + RESEARCHLOG_PADDING * 2);
}

void PlayerResearchLog::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();
	pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), pal_RGBA(0, 0, 0, 130));
	iV_Box(x0, y0, x0 + width(), y0 + height(), pal_RGBA(255, 255, 255, 60));
}

void PlayerResearchLog::geometryChanged()
{
	int innerWidth = std::max(width() - RESEARCHLOG_PADDING * 2, 1);
	if (titleLabel)
	{
		titleLabel->setGeometry(RESEARCHLOG_PADDING, RESEARCHLOG_PADDING, innerWidth, RESEARCHLOG_TITLE_HEIGHT);
	}
	int contentY = RESEARCHLOG_PADDING + RESEARCHLOG_TITLE_HEIGHT + RESEARCHLOG_TITLE_GAP;
	int contentHeight = std::max(height() - contentY - RESEARCHLOG_PADDING, 1);
	if (table)
	{
		table->setGeometry(RESEARCHLOG_PADDING, contentY, innerWidth, contentHeight);
		// relayout the columns against the new table width (the table does not do this automatically)
		table->changeColumnWidths({static_cast<size_t>(badgeColumnWidth), static_cast<size_t>(nameColumnIdealWidth)});
	}
	if (noEntriesLabel)
	{
		noEntriesLabel->setGeometry(RESEARCHLOG_PADDING, contentY, innerWidth, RESEARCHLOG_ROW_HEIGHT);
	}
}

int32_t PlayerResearchLog::idealWidth()
{
	return cachedIdealWidth;
}

int32_t PlayerResearchLog::idealHeight()
{
	return RESEARCHLOG_PANEL_IDEAL_HEIGHT;
}

// MARK: - ResearchLogLine

/**
 * A single line of PlayerResearchLog panels, side by side.
 *
 * Each line is a separate ScrollableListWidget item: widget display culling is
 * all-or-nothing (a widget that isn't fully inside the scroll view is skipped
 * along with all of its children), so the scrolled items must individually fit
 * within the view - one big flow-layout item taller than the view would never
 * display at all.
 */
class ResearchLogLine : public WIDGET
{
public:
	static std::shared_ptr<ResearchLogLine> make(const std::vector<std::shared_ptr<PlayerResearchLog>>& linePanels, int gridColumns, int lineHeight)
	{
		auto result = std::make_shared<ResearchLogLine>();
		for (const auto& panel : linePanels)
		{
			result->attach(panel);
			result->panels.push_back(panel);
		}
		result->gridColumns = std::max(gridColumns, 1);
		result->setGeometry(0, 0, 0, lineHeight);
		return result;
	}

	void geometryChanged() override
	{
		if (panels.empty())
		{
			return;
		}
		// size cells by the overall grid's column count (not this line's panel count),
		// so panels on a partial last line match the width of those on full lines
		int cellWidth = std::max((width() - (gridColumns - 1) * RESEARCHLOG_GRID_GAP) / gridColumns, 1);
		for (size_t i = 0; i < panels.size(); ++i)
		{
			panels[i]->setGeometry(static_cast<int>(i) * (cellWidth + RESEARCHLOG_GRID_GAP), 0, cellWidth, height());
		}
	}

private:
	std::vector<std::shared_ptr<PlayerResearchLog>> panels;
	int gridColumns = 1;
};

// MARK: - GameResearchLogViewerWidget

std::shared_ptr<GameResearchLogViewerWidget> GameResearchLogViewerWidget::make()
{
	auto result = std::make_shared<GameResearchLogViewerWidget>();
	result->initialize();
	return result;
}

void GameResearchLogViewerWidget::initialize()
{
	for (const auto& def : buildResearchLogColumnDefs())
	{
		panels.push_back(PlayerResearchLog::make(def));
	}
	scrollableList = ScrollableListWidget::make();
	attach(scrollableList);
	scrollableList->setItemSpacing(RESEARCHLOG_GRID_GAP);
}

int GameResearchLogViewerWidget::maxPanelIdealWidth() const
{
	int result = 0;
	for (const auto& panel : panels)
	{
		result = std::max(result, panel->idealWidth());
	}
	return result;
}

int GameResearchLogViewerWidget::columnsForWidth(int availableWidth) const
{
	if (panels.empty())
	{
		return 1;
	}
	int cols = (availableWidth + RESEARCHLOG_GRID_GAP) / (maxPanelIdealWidth() + RESEARCHLOG_GRID_GAP);
	return std::max(1, std::min<int>(cols, static_cast<int>(panels.size())));
}

void GameResearchLogViewerWidget::rebuildLines(int columns)
{
	// release the panels from their current lines, then re-bucket them onto new lines
	for (const auto& panel : panels)
	{
		if (auto previousLine = panel->parent())
		{
			previousLine->detach(panel);
		}
	}
	scrollableList->clear();
	lines.clear();
	size_t panelsPerLine = static_cast<size_t>(std::max(columns, 1));
	int lineHeight = std::max(height(), 1);
	for (size_t i = 0; i < panels.size(); i += panelsPerLine)
	{
		std::vector<std::shared_ptr<PlayerResearchLog>> linePanels(
			panels.begin() + i, panels.begin() + std::min(i + panelsPerLine, panels.size()));
		auto line = ResearchLogLine::make(linePanels, columns, lineHeight);
		scrollableList->addItem(line);
		lines.push_back(line);
	}
}

void GameResearchLogViewerWidget::geometryChanged()
{
	if (!scrollableList)
	{
		return;
	}
	scrollableList->setGeometry(0, 0, width(), height());
	if (panels.empty())
	{
		return;
	}
	// always reserve room for the scrollbar, so the column count doesn't
	// oscillate with the scrollbar's visibility
	int availableWidth = std::max(width() - scrollableList->getScrollbarWidth(), 1);
	int columns = columnsForWidth(availableWidth);
	if (columns != currentColumns)
	{
		currentColumns = columns;
		rebuildLines(columns);
		return; // rebuildLines already sized the lines to the current height
	}
	// each line fills the full view height, so scrolling pages whole lines
	int lineHeight = std::max(height(), 1);
	for (const auto& line : lines)
	{
		if (line->height() != lineHeight)
		{
			line->setGeometry(line->x(), line->y(), line->width(), lineHeight);
			scrollableList->setLayoutDirty();
		}
	}
}

int32_t GameResearchLogViewerWidget::idealWidth()
{
	if (panels.empty())
	{
		return RESEARCHLOG_PANEL_MIN_IDEAL_WIDTH;
	}
	int cols = std::min<int>(static_cast<int>(panels.size()), RESEARCHLOG_IDEAL_MAX_PER_LINE);
	return cols * maxPanelIdealWidth() + (cols - 1) * RESEARCHLOG_GRID_GAP + scrollableList->getScrollbarWidth();
}

int32_t GameResearchLogViewerWidget::idealHeight()
{
	// lines fill the full view height (scrolling pages whole lines), so the
	// ideal height is just a single line at the panels' ideal height
	return RESEARCHLOG_PANEL_IDEAL_HEIGHT;
}
