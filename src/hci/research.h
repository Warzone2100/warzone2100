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

#ifndef __INCLUDED_SRC_HCI_RESEARCH_INTERFACE_H__
#define __INCLUDED_SRC_HCI_RESEARCH_INTERFACE_H__

#include "objects_stats.h"

class ResearchController: public BaseObjectsStatsController, public std::enable_shared_from_this<ResearchController>
{
public:
	RESEARCH *getObjectStatsAt(size_t objectIndex) const override;
	RESEARCH *getStatsAt(size_t statsIndex) const override
	{
		ASSERT_OR_RETURN(nullptr, statsIndex < stats.size(), "Invalid stats index (%zu); max: (%zu)", statsIndex, stats.size());
		return stats[statsIndex];
	}

	size_t statsSize() const override
	{
		return stats.size();
	}

	size_t objectsSize() const override
	{
		return facilities.size();
	}

	STRUCTURE *getObjectAt(size_t index) const override
	{
		ASSERT_OR_RETURN(nullptr, index < facilities.size(), "Invalid object index (%zu); max: (%zu)", index, facilities.size());
		return facilities[index];
	}

	bool findObject(std::function<bool (BASE_OBJECT *)> iteration) const override
	{
		return BaseObjectsController::findObject(facilities, iteration);
	}

	nonstd::optional<size_t> getHighlightedFacilityIndex();
	void updateData();
	bool showInterface() override;
	void refresh() override;
	void clearData() override;
	std::shared_ptr<StatsForm> makeStatsForm() override;
	void startResearch(RESEARCH &research);
	void cancelResearch(STRUCTURE *facility);
	void requestResearchCancellation(STRUCTURE *facility);

	STRUCTURE *getHighlightedObject() const override
	{
		return highlightedFacility;
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
	void updateFacilitiesList();
	void updateResearchOptionsList();
	std::vector<RESEARCH *> stats;
	std::vector<STRUCTURE *> facilities;
	bool queuedJumpToHighlightedStatsObject = true;
	static STRUCTURE *highlightedFacility;
};

#endif // __INCLUDED_SRC_HCI_RESEARCH_INTERFACE_H__
