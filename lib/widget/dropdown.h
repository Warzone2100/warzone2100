/*
	This file is part of Warzone 2100.
	Copyright (C) 2020  Warzone 2100 Project

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
/**
 * @file
 * Definitions for scroll bar functions.
 */

#ifndef __INCLUDED_LIB_WIDGET_DROPDOWN_H__
#define __INCLUDED_LIB_WIDGET_DROPDOWN_H__

#include <optional-lite/optional.hpp>
#include "widget.h"
#include "scrollablelist.h"

class DropdownItemWrapper;
typedef std::function<void(std::shared_ptr<DropdownItemWrapper> item)> DropdownOnSelectHandler;

class DropdownItemWrapper: public WIDGET
{
protected:
	DropdownItemWrapper() {}

	void initialize(const std::shared_ptr<WIDGET> &newItem, DropdownOnSelectHandler newOnSelect);

public:
	static std::shared_ptr<DropdownItemWrapper> make(const std::shared_ptr<WIDGET> &item, DropdownOnSelectHandler onSelect)
	{
		class make_shared_enabler: public DropdownItemWrapper {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(item, onSelect);
		return widget;
	}

	bool processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;
	void geometryChanged() override;

	WIDGET &getItem() const
	{
		return *(item.get());
	}

	void setPadding(const Padding &value)
	{
		padding = value;
	}

	void setSelected(bool value)
	{
		selected = value;
	}

protected:
	void display(int xOffset, int yOffset) override;

private:
	std::shared_ptr<WIDGET> item;
	DropdownOnSelectHandler onSelect;
	Padding padding;
	bool selected = false;
};

class DropdownWidget : public WIDGET
{
public:
	DropdownWidget();

	void addItem(const std::shared_ptr<WIDGET> &widget);
	void display(int xOffset, int yOffset) override;
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void run(W_CONTEXT *) override;
	void geometryChanged() override;
	void open();
	void close();
	bool processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;
	void setListHeight(uint32_t value)
	{
		itemsList->setGeometry(itemsList->x(), itemsList->y(), itemsList->width(), value);
	}
	void setItemPadding(const Padding &value)
	{
		itemPadding = value;
	}
	void setSelectedIndex(size_t index)
	{
		ASSERT_OR_RETURN(, index < items.size(), "Invalid dropdown item index");
		select(items[index]);
	}
	void setOnChange(std::function<void(DropdownWidget&)> value)
	{
		onChange = value;
	}
	nonstd::optional<size_t> getSelectedIndex() const
	{
		if (selectedItem == nullptr)
		{
			return nonstd::nullopt;
		}

		for (auto item = items.begin(); item != items.end(); item++)
		{
			if (*item == selectedItem)
			{
				return item - items.begin();
			}
		}

		return nonstd::nullopt;
	}
	int getScrollbarWidth() const
	{
		ASSERT_OR_RETURN(0, itemsList != nullptr, "null itemsList");
		return itemsList->getScrollbarWidth();
	}

private:
	std::vector<std::shared_ptr<DropdownItemWrapper>> items;
	std::shared_ptr<ScrollableListWidget> itemsList;
	std::shared_ptr<W_SCREEN> overlayScreen;
	std::shared_ptr<DropdownItemWrapper> selectedItem;
	Padding itemPadding = {0, 0, 0, 0};
	std::function<void(DropdownWidget&)> onChange;

	void select(const std::shared_ptr<DropdownItemWrapper> &selected)
	{
		if (selectedItem == selected)
		{
			return;
		}

		if (selectedItem)
		{
			selectedItem->setSelected(false);
		}
		selectedItem = selected;
		selectedItem->setSelected(true);

		if (onChange)
		{
			onChange(*this);
		}
	}
};

#endif // __INCLUDED_LIB_WIDGET_DROPDOWN_H__
