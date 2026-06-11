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

#ifndef __INCLUDED_SRC_PLAYERSTATSGRAPH_H__
#define __INCLUDED_SRC_PLAYERSTATSGRAPH_H__

#include "lib/widget/datagraph.h"
#include "gamehistorylogger.h"

#include <memory>
#include <vector>

class DropdownWidget;

enum class PlayerStatsMetric
{
	Score,
	Kills,

	Droids,
	DroidsBuilt,
	DroidsLost,

	Structs,
	StructuresBuilt,
	StructuresLost,

	Power,
	OilRigs,

	ResearchComplete,
	LabUse
};

enum class PlayerStatsMetricDisplayFormat
{
	Integer,
	Float_2f_Percent, // ##.##%
};

struct PlayerStatsMetricDef
{
	PlayerStatsMetric metric;
	const char* displayName;	// untranslated, pass to _() at point of use
	std::function<float (const GameStoryLogger::GameFrame::PlayerStats&)> getValue;
};

const std::vector<PlayerStatsMetricDef>& getPlayerStatsMetricDefs();

/**
 * A DataGraphWidget that displays a single GameFrame playerStats metric for
 * all players across game time, fed from GameStoryLogger's game frames.
 */
class GameFramePlayerStatsWidget : public DataGraphWidget
{
public:
	static std::shared_ptr<GameFramePlayerStatsWidget> make(PlayerStatsMetric initialMetric = PlayerStatsMetric::Score);

	void setMetric(PlayerStatsMetric newMetric);
	PlayerStatsMetric currentMetric() const { return metric; }

private:
	void rebuildSeries();
	void setYAxisFormatterForMetric(PlayerStatsMetricDisplayFormat format);

private:
	PlayerStatsMetric metric = PlayerStatsMetric::Score;
};

/**
 * A form combining a metric-selector dropdown with a GameFramePlayerStatsWidget
 * displaying the selected metric.
 */
class PlayerStatsGraphForm : public WIDGET
{
public:
	static std::shared_ptr<PlayerStatsGraphForm> make(PlayerStatsMetric initialMetric = PlayerStatsMetric::Score);

	void setMetric(PlayerStatsMetric newMetric);
	PlayerStatsMetric currentMetric() const;

	void geometryChanged() override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;
	void display(int xOffset, int yOffset) override;

private:
	void initialize(PlayerStatsMetric initialMetric);

private:
	std::shared_ptr<DropdownWidget> metricDropdown;
	std::shared_ptr<GameFramePlayerStatsWidget> graph;
};

#endif // __INCLUDED_SRC_PLAYERSTATSGRAPH_H__
