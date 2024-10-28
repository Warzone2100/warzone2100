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

#ifndef __INCLUDED_SRC_HCI_COMMANDER_INTERFACE_H__
#define __INCLUDED_SRC_HCI_COMMANDER_INTERFACE_H__

#include "objects_stats.h"

class CommanderController: public BaseObjectsController, public std::enable_shared_from_this<CommanderController>
{
public:
	STRUCTURE_STATS *getObjectStatsAt(size_t objectIndex) const override;

	STRUCTURE *getAssignedFactoryAt(size_t objectIndex) const;

	size_t objectsSize() const override
	{
		return commanders.size();
	}

	DROID *getObjectAt(size_t index) const override
	{
		ASSERT_OR_RETURN(nullptr, index < commanders.size(), "Invalid object index (%zu); max: (%zu)", index, commanders.size());
		return commanders[index];
	}

	bool findObject(std::function<bool (BASE_OBJECT *)> iteration) const override
	{
		return BaseObjectsController::findObject(commanders, iteration);
	}

	void updateData();
	bool showInterface() override;
	void refresh() override;
	void clearData() override;
	void displayOrderForm();

	DROID *getHighlightedObject() const override
	{
		return highlightedCommander;
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
	void updateCommandersList();
	std::vector<DROID *> commanders;
	bool queuedJumpToHighlightedStatsObject = true;
	static DROID *highlightedCommander;
};

#endif // __INCLUDED_SRC_HCI_COMMANDER_INTERFACE_H__
