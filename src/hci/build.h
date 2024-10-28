/*
	This file is part of Warzone 2100.
	Copyright (C) 2021-2023  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_HCI_BUILD_INTERFACE_H__
#define __INCLUDED_SRC_HCI_BUILD_INTERFACE_H__

#include "objects_stats.h"

class BuildController: public BaseObjectsStatsController, public std::enable_shared_from_this<BuildController>
{
public:
	STRUCTURE_STATS *getObjectStatsAt(size_t objectIndex) const override;
	STRUCTURE_STATS *getStatsAt(size_t statsIndex) const override
	{
		ASSERT_OR_RETURN(nullptr, statsIndex < stats.size(), "Invalid stats index (%zu); max: (%zu)", statsIndex, stats.size());
		return stats[statsIndex];
	}

	size_t statsSize() const override
	{
		return stats.size();
	}

	bool shouldShowRedundantDesign() const
	{
		return intGetShouldShowRedundantDesign();
	}

	void setShouldShowRedundantDesign(bool value)
	{
		intSetShouldShowRedundantDesign(value);
		updateBuildOptionsList();
	}

	bool shouldShowFavorites() const
	{
		return BuildController::showFavorites;
	}

	void setShouldShowFavorite(bool value)
	{
		BuildController::showFavorites = value;
		updateBuildOptionsList();
	}

	size_t objectsSize() const override
	{
		return builders.size();
	}

	DROID *getObjectAt(size_t index) const override
	{
		ASSERT_OR_RETURN(nullptr, index < builders.size(), "Invalid object index (%zu); max: (%zu)", index, builders.size());
		return builders[index];
	}

	bool findObject(std::function<bool (BASE_OBJECT *)> iteration) const override
	{
		return BaseObjectsController::findObject(builders, iteration);
	}

	void updateData();
	void toggleFavorites(STRUCTURE_STATS *buildOption);
	void startBuildPosition(STRUCTURE_STATS *buildOption);
	bool showInterface() override;
	void refresh() override;
	void clearData() override;
	void toggleBuilderSelection(DROID *droid);
	std::shared_ptr<StatsForm> makeStatsForm() override;

	static void resetShowFavorites()
	{
		BuildController::showFavorites = false;
	}

	DROID *getHighlightedObject() const override
	{
		return highlightedBuilder;
	}

	void setHighlightedObject(BASE_OBJECT *object, bool jumpToHighlightedStatsObject) override;

	bool getQueuedJumpToHighlightedStatsObject() const override
	{
		return queuedJumpToHighlightedStatsObject;
	}

	void clearQueuedJumpToHighlightedStatsObject() override
	{
		queuedJumpToHighlightedStatsObject = false;
	}

private:
	void updateBuildersList();
	void updateBuildOptionsList();
	std::vector<STRUCTURE_STATS *> stats;
	std::vector<DROID *> builders;
	bool queuedJumpToHighlightedStatsObject = true;

	static bool showFavorites;
	static DROID *highlightedBuilder;
};

#endif // __INCLUDED_SRC_HCI_BUILD_INTERFACE_H__
