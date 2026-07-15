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
 *  Widgets displaying the research log from the game history log,
 *  one panel per player (or per team, for locked teams with shared research)
 */

#ifndef __INCLUDED_SRC_RESEARCHLOGVIEWER_H__
#define __INCLUDED_SRC_RESEARCHLOGVIEWER_H__

#include "lib/widget/widget.h"
#include "lib/framework/wzstring.h"
#include "lib/ivis_opengl/piepalette.h"

#include <memory>
#include <vector>

class ScrollableTableWidget;
class ScrollableListWidget;
class W_LABEL;

struct ResearchLogColumnDef
{
	WzString title;				///< player name, or "Team A" / "Team B" / ...
	PIELIGHT color;				///< title color
	size_t sourcePlayerIdx = 0;	///< whose events to filter from GameStoryLogger's research log
	WzString tooltip;			///< e.g. team member names (empty = no tooltip)
};

/// One def per displayed research log: per team for locked teams (shared research), per player otherwise
std::vector<ResearchLogColumnDef> buildResearchLogColumnDefs();

/**
 * A single research timeline panel: a title plus a scrollable table of
 * (game time badge, research name) rows, fed from GameStoryLogger's research log.
 */
class PlayerResearchLog : public WIDGET
{
public:
	static std::shared_ptr<PlayerResearchLog> make(const ResearchLogColumnDef& def);

	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;

private:
	void initialize(const ResearchLogColumnDef& def);

private:
	std::shared_ptr<W_LABEL> titleLabel;
	std::shared_ptr<ScrollableTableWidget> table;	// null if there are no entries
	std::shared_ptr<W_LABEL> noEntriesLabel;		// shown instead of the table when there are no entries
	int32_t cachedIdealWidth = 0;
	int badgeColumnWidth = 0;
	int nameColumnIdealWidth = 0;
};

/**
 * Displays one PlayerResearchLog per column def (player or team), tiled
 * left-to-right with as many per line as the width permits, with the lines
 * vertically scrollable when they exceed the widget's height.
 */
class GameResearchLogViewerWidget : public WIDGET
{
public:
	static std::shared_ptr<GameResearchLogViewerWidget> make();

	void geometryChanged() override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;

private:
	void initialize();
	int maxPanelIdealWidth() const;
	int columnsForWidth(int availableWidth) const;
	void rebuildLines(int columns);

private:
	std::shared_ptr<ScrollableListWidget> scrollableList;
	std::vector<std::shared_ptr<PlayerResearchLog>> panels;
	std::vector<std::shared_ptr<WIDGET>> lines;
	int currentColumns = 0;
};

#endif // __INCLUDED_SRC_RESEARCHLOGVIEWER_H__
