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

#ifndef __INCLUDED_SRC_HCI_MANUFACTURE_INTERFACE_H__
#define __INCLUDED_SRC_HCI_MANUFACTURE_INTERFACE_H__

#include "objects_stats.h"

class ManufactureController: public BaseObjectsStatsController, public std::enable_shared_from_this<ManufactureController>
{
public:
	DROID_TEMPLATE *getObjectStatsAt(size_t objectIndex) const override;
	DROID_TEMPLATE *getStatsAt(size_t statsIndex) const override
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
		updateManufactureOptionsList();
	}

	size_t objectsSize() const override
	{
		return factories.size();
	}

	STRUCTURE *getObjectAt(size_t index) const override
	{
		ASSERT_OR_RETURN(nullptr, index < factories.size(), "Invalid object index (%zu); max: (%zu)", index, factories.size());
		return factories[index];
	}

	bool findObject(std::function<bool (BASE_OBJECT *)> iteration) const override
	{
		return BaseObjectsController::findObject(factories, iteration);
	}

	void updateData();
	void adjustFactoryProduction(DROID_TEMPLATE *manufactureOption, bool add);
	void adjustFactoryLoop(bool add);
	void releaseFactoryProduction(STRUCTURE *structure);
	void cancelFactoryProduction(STRUCTURE *structure);
	void startDeliveryPointPosition();
	bool showInterface() override;
	void refresh() override;
	void clearData() override;
	std::shared_ptr<StatsForm> makeStatsForm() override;

	STRUCTURE *getHighlightedObject() const override
	{
		return highlightedFactory;
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
	void updateFactoriesList();
	void updateManufactureOptionsList();
	std::vector<DROID_TEMPLATE *> stats;
	std::vector<STRUCTURE *> factories;
	bool queuedJumpToHighlightedStatsObject = true;
	static STRUCTURE *highlightedFactory;
};

#endif // __INCLUDED_SRC_HCI_MANUFACTURE_INTERFACE_H__
