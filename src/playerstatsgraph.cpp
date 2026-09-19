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
 *  A graph widget displaying a per-player stat over time, from the game history log
 */

#include "playerstatsgraph.h"
#include "scores.h"
#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/widget/dropdown.h"
#include "lib/widget/label.h"
#include "lib/widget/margin.h"

#include <algorithm>

#include <fmt/core.h>

constexpr int STATSGRAPH_PADDING = 10;
constexpr int STATSGRAPH_DROPDOWN_TOP = 8;
constexpr int STATSGRAPH_DROPDOWN_HEIGHT = 24;
constexpr int STATSGRAPH_DROPDOWN_ITEM_HEIGHT = 20;
constexpr int STATSGRAPH_GRAPH_TOP = STATSGRAPH_DROPDOWN_TOP + STATSGRAPH_DROPDOWN_HEIGHT + 8;

constexpr PlayerStatsMetricDisplayFormat getDisplayFormatForPlayerStatsMetric(PlayerStatsMetric metric)
{
	switch (metric)
	{
		case PlayerStatsMetric::LabUse:
			return PlayerStatsMetricDisplayFormat::Float_2f_Percent;
		default:
			break;
	}
	return PlayerStatsMetricDisplayFormat::Integer;
}

const std::vector<PlayerStatsMetricDef>& getPlayerStatsMetricDefs()
{
	typedef GameStoryLogger::GameFrame::PlayerStats PlayerStats;
	static const std::vector<PlayerStatsMetricDef> defs = {
		{PlayerStatsMetric::Score, N_("Score"), [](const PlayerStats& s) { return static_cast<float>(s.score); }},
		{PlayerStatsMetric::Kills, N_("Kills"), [](const PlayerStats& s) { return static_cast<float>(s.kills); }},

		{PlayerStatsMetric::Droids, N_("Droids"), [](const PlayerStats& s) { return static_cast<float>(s.droids); }},
		{PlayerStatsMetric::DroidsBuilt, N_("Droids Built"), [](const PlayerStats& s) { return static_cast<float>(s.droidsBuilt); }},
		{PlayerStatsMetric::DroidsLost, N_("Droids Lost"), [](const PlayerStats& s) { return static_cast<float>(s.droidsLost); }},

		{PlayerStatsMetric::Structs, N_("Structs"), [](const PlayerStats& s) { return static_cast<float>(s.structs); }},
		{PlayerStatsMetric::StructuresBuilt, N_("Structs Built"), [](const PlayerStats& s) { return static_cast<float>(s.structuresBuilt); }},
		{PlayerStatsMetric::StructuresLost, N_("Structs Lost"), [](const PlayerStats& s) { return static_cast<float>(s.structuresLost); }},

		{PlayerStatsMetric::Power, N_("Power"), [](const PlayerStats& s) { return static_cast<float>(s.power); }},
		{PlayerStatsMetric::OilRigs, N_("Oil Rigs"), [](const PlayerStats& s) { return static_cast<float>(s.oilRigs); }},

		{PlayerStatsMetric::ResearchComplete, N_("Research Completed"), [](const PlayerStats& s) { return static_cast<float>(s.researchComplete); }},
		{PlayerStatsMetric::LabUse, N_("Lab Use"), [](const PlayerStats& s) { return (static_cast<float>(s.recentResearchPerformance) / static_cast<float>(s.recentResearchPotential)) * 100.f; }},
	};
	return defs;
}

static const PlayerStatsMetricDef& getMetricDef(PlayerStatsMetric metric)
{
	const auto& defs = getPlayerStatsMetricDefs();
	for (const auto& def : defs)
	{
		if (def.metric == metric)
		{
			return def;
		}
	}
	ASSERT(false, "Missing metric def: %d", static_cast<int>(metric));
	return defs.front();
}

void GameFramePlayerStatsWidget::setYAxisFormatterForMetric(PlayerStatsMetricDisplayFormat format)
{
	switch (format)
	{
		case PlayerStatsMetricDisplayFormat::Integer:
			setYAxisFormatter([](float value) {
				return WzString::number(static_cast<int>(value));
			});
			break;
		case PlayerStatsMetricDisplayFormat::Float_2f_Percent:
			setYAxisFormatter([](float value) {
				return WzString::fromUtf8(fmt::format("{:.2f}%", value));
			});
			break;
	}
}

std::shared_ptr<GameFramePlayerStatsWidget> GameFramePlayerStatsWidget::make(PlayerStatsMetric initialMetric)
{
	auto result = std::make_shared<GameFramePlayerStatsWidget>();
	result->setXAxisFormatter([](float value) {
		char timeText[20];
		getAsciiTimeN(timeText, sizeof(timeText), static_cast<unsigned>(std::max(value, 0.f)));
		return WzString::fromUtf8(timeText);
	});
	result->setYAxisFormatterForMetric(getDisplayFormatForPlayerStatsMetric(initialMetric));
	result->setYAxisMinTickStep(1.f);
	// time-friendly x tick intervals
	std::vector<float> tickSteps;
	for (unsigned seconds : {15, 30, 60, 2 * 60, 5 * 60, 10 * 60, 15 * 60, 30 * 60, 60 * 60, 2 * 60 * 60})
	{
		tickSteps.push_back(static_cast<float>(seconds * GAME_TICKS_PER_SEC));
	}
	result->setXAxisPreferredTickSteps(std::move(tickSteps));
	result->setMetric(initialMetric);
	return result;
}

void GameFramePlayerStatsWidget::setMetric(PlayerStatsMetric newMetric)
{
	metric = newMetric;
	rebuildSeries();
}

void GameFramePlayerStatsWidget::rebuildSeries()
{
	const auto& metricDef = getMetricDef(metric);
	const auto& frames = GameStoryLogger::instance().getGameFrames();
	const auto& playerAttrs = GameStoryLogger::instance().getFixedPlayerAttributes();

	setYAxisFormatterForMetric(getDisplayFormatForPlayerStatsMetric(metric));

	std::vector<Series> newSeries;
	for (size_t player = 0; player < playerAttrs.size(); ++player)
	{
		if (!playerAttrs[player].slotWasOccupied || playerAttrs[player].name.empty())
		{
			continue; // open / closed slot
		}
		Series s;
		s.label = WzString::fromUtf8(playerAttrs[player].name);
		s.color = pal_GetTeamColour(playerAttrs[player].colour);
		for (const auto& frame : frames)
		{
			if (player >= frame.playerData.size())
			{
				continue;
			}
			const auto& playerStats = frame.playerData[player];
			s.points.push_back(Vector2f(static_cast<float>(frame.currGameTime), metricDef.getValue(playerStats)));
			if (playerStats.playerLeftGameTime.has_value())
			{
				break; // end the line where the player left the game
			}
		}
		newSeries.push_back(std::move(s));
	}
	setSeries(std::move(newSeries));
}

static size_t getMetricDefIndex(PlayerStatsMetric metric)
{
	const auto& defs = getPlayerStatsMetricDefs();
	for (size_t i = 0; i < defs.size(); ++i)
	{
		if (defs[i].metric == metric)
		{
			return i;
		}
	}
	ASSERT(false, "Missing metric def: %d", static_cast<int>(metric));
	return 0;
}

std::shared_ptr<PlayerStatsGraphForm> PlayerStatsGraphForm::make(PlayerStatsMetric initialMetric)
{
	auto result = std::make_shared<PlayerStatsGraphForm>();
	result->initialize(initialMetric);
	return result;
}

void PlayerStatsGraphForm::initialize(PlayerStatsMetric initialMetric)
{
	graph = GameFramePlayerStatsWidget::make(initialMetric);
	attach(graph);

	// metric selector
	metricDropdown = std::make_shared<DropdownWidget>();
	attach(metricDropdown);
	const auto& metricDefs = getPlayerStatsMetricDefs();
	int maxItemWidth = 0;
	for (const auto& def : metricDefs)
	{
		auto label = std::make_shared<W_LABEL>();
		label->setFont(font_regular, WZCOL_FORM_TEXT);
		label->setString(gettext(def.displayName));
		label->setGeometry(0, 0, label->getMaxLineWidth(), STATSGRAPH_DROPDOWN_ITEM_HEIGHT);
		maxItemWidth = std::max(maxItemWidth, label->width());
		metricDropdown->addItem(Margin(10, 10).wrap(label));
	}
	metricDropdown->setListHeight(STATSGRAPH_DROPDOWN_ITEM_HEIGHT * std::min<uint32_t>(static_cast<uint32_t>(metricDefs.size()), 6));
	metricDropdown->setSelectedIndex(getMetricDefIndex(initialMetric));
	std::weak_ptr<GameFramePlayerStatsWidget> weakGraph = graph;
	metricDropdown->setOnChange([weakGraph](DropdownWidget& dropdown) {
		auto selectedIndex = dropdown.getSelectedIndex();
		auto strongGraph = weakGraph.lock();
		ASSERT_OR_RETURN(, strongGraph != nullptr, "No graph?");
		const auto& defs = getPlayerStatsMetricDefs();
		if (selectedIndex.has_value() && selectedIndex.value() < defs.size())
		{
			strongGraph->setMetric(defs[selectedIndex.value()].metric);
		}
	});
	metricDropdown->setGeometry(STATSGRAPH_PADDING, STATSGRAPH_DROPDOWN_TOP, maxItemWidth + 30, STATSGRAPH_DROPDOWN_HEIGHT);
}

void PlayerStatsGraphForm::setMetric(PlayerStatsMetric newMetric)
{
	graph->setMetric(newMetric);
	metricDropdown->setSelectedIndex(getMetricDefIndex(newMetric));
}

PlayerStatsMetric PlayerStatsGraphForm::currentMetric() const
{
	return graph->currentMetric();
}

void PlayerStatsGraphForm::geometryChanged()
{
	if (graph)
	{
		graph->setGeometry(STATSGRAPH_PADDING, STATSGRAPH_GRAPH_TOP,
		                   std::max(width() - STATSGRAPH_PADDING * 2, 1),
		                   std::max(height() - STATSGRAPH_GRAPH_TOP - STATSGRAPH_PADDING, 1));
	}
}

int32_t PlayerStatsGraphForm::idealWidth()
{
	return graph->idealWidth() + STATSGRAPH_PADDING * 2;
}

int32_t PlayerStatsGraphForm::idealHeight()
{
	return STATSGRAPH_GRAPH_TOP + graph->idealHeight() + STATSGRAPH_PADDING;
}

void PlayerStatsGraphForm::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	int dropdownBoxX0 = x0 + metricDropdown->x();
	int dropdownBoxY0 = y0 + metricDropdown->y();
	int dropdownBoxX1 = dropdownBoxX0 + metricDropdown->width();
	int dropdownBoxY1 = dropdownBoxY0 + metricDropdown->height();

	pie_UniTransBoxFill(dropdownBoxX0, dropdownBoxY0, dropdownBoxX1, dropdownBoxY1, pal_RGBA(0, 0, 0, 130));
	iV_Box(dropdownBoxX0, dropdownBoxY0, dropdownBoxX1, dropdownBoxY1, pal_RGBA(255, 255, 255, 50));
}
