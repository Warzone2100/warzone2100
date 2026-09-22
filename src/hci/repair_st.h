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

#ifndef __INCLUDED_SRC_HCI_REPAIRST_INTERFACE_H__
#define __INCLUDED_SRC_HCI_REPAIRST_INTERFACE_H__

#include "lib/framework/string_ext.h"
#include "objects_stats.h"
#include "src/structuredef.h"
#include <cstddef>

class RepairStationController: public BaseObjectsStatsController, public std::enable_shared_from_this<RepairStationController>
{
public:
	void startDeliveryPointPosition();
	bool showInterface() override;
	void refresh() override {};
	void clearData() override {};
	std::shared_ptr<StatsForm> makeStatsForm() override;

	//Must implement virtual methods, but otherwise will always have 1 station and 0 additional items.
	DROID_TEMPLATE *getObjectStatsAt(size_t objectIndex) const override {return nullptr;}
	DROID_TEMPLATE *getStatsAt(size_t statsIndex) const override {return nullptr;}
	size_t statsSize() const override {return 0;}
	size_t objectsSize() const override {return 1;}

	//We never need to interact with multiple stations at once.
	STRUCTURE *getObjectAt(size_t index) const override {return nullptr;}

	bool findObject(std::function<bool (BASE_OBJECT *)> iteration) const override
	{
		//Fast track returning the selected station if we already have one.
		if (highlightedStation && highlightedStation->selected)
		{
			return iteration(highlightedStation);
		}

		auto* intStrList = interfaceStructList();
		if (intStrList)
		{
			for (auto structure : *intStrList)
			{
				if (iteration(structure))
				{
					return true;
				}
			}
		}
		return false;
	}

	void setHighlightedObject(BASE_OBJECT *object, bool jumpToHighlightedStatsObject) override;


	STRUCTURE* getHighlightedObject() const override
	{
		return highlightedStation;
	}

	//Never need to jump to the repair station from the menu.
	bool getQueuedJumpToHighlightedStatsObject() const override {return false;}
	void clearQueuedJumpToHighlightedStatsObject() override {}

private:
	static STRUCTURE *highlightedStation;
};

#endif // __INCLUDED_SRC_HCI_MANUFACTURE_INTERFACE_H__
