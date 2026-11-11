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
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
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
