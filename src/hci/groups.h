/*
	This file is part of Warzone 2100.
	Copyright (C) 2022-2023  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_HCI_GROUPS_H__
#define __INCLUDED_SRC_HCI_GROUPS_H__

#include "../intdisplay.h"
#include "objects_stats.h"

#include "../objmem.h"
#include "../input/keyconfig.h"
#include "../keybind.h"
#include "lib/widget/label.h"
#include "../selection.h"

void setGroupButtonEnabled(bool bNewState);
bool getGroupButtonEnabled();

class GroupButton;
class GroupsUIController;

class GroupsForum: public IntFormAnimated
{
private:
	typedef IntFormAnimated BaseWidget;
protected:
	void initialize();
public:
	void display(int xOffset, int yOffset) override;
	static std::shared_ptr<GroupsForum> make()
	{
		class make_shared_enabler: public GroupsForum {};
		auto widget = std::make_shared<make_shared_enabler>();
		// widget->controller = controller;
		widget->initialize();
		return widget;
	}
	void updateData();
	void updateSelectedGroup(size_t group);
	void addGroupDamageForCurrentTick(size_t group, uint64_t additionalDamage, bool unitKilled);
	void addCommanderGroupDamageForCurrentTick(const DROID *psCommander, uint64_t additionalDamage, bool unitKilled);

	WidgetHelp const * getHelp() const override
	{
		if (!help.has_value()) { return nullptr; }
		return &(help.value());
	}
	void setHelp(optional<WidgetHelp> help) override;
private:
	std::shared_ptr<GroupButton> makeGroupButton(size_t groupNumber);
	void addTabList();
private:
	std::shared_ptr<IntListTabWidget> groupsList;
	std::shared_ptr<GroupsUIController> groupsUIController;
	optional<WidgetHelp> help;
};

#endif // __INCLUDED_SRC_HCI_GROUPS_H__
