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

#include <nonstd/optional.hpp>
#include "widget.h"
#include "scrollablelist.h"
#include "lib/ivis_opengl/ivisdef.h"

class DropdownItemWrapper;
typedef std::function<void(std::shared_ptr<DropdownItemWrapper> item)> DropdownOnSelectHandler;
class DropdownWidget;

class DropdownItemWrapper: public WIDGET
{
protected:
	DropdownItemWrapper() {}

	void initialize(const std::shared_ptr<DropdownWidget>& parent, const std::shared_ptr<WIDGET> &newItem, DropdownOnSelectHandler newOnSelect);

public:
	static std::shared_ptr<DropdownItemWrapper> make(const std::shared_ptr<DropdownWidget>& parent, const std::shared_ptr<WIDGET> &item, DropdownOnSelectHandler onSelect)
	{
		class make_shared_enabler: public DropdownItemWrapper {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(parent, item, onSelect);
		return widget;
	}

	std::shared_ptr<WIDGET> findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;
	void geometryChanged() override;

	const std::shared_ptr<WIDGET>& getItem() const
	{
		return item;
	}

	void setSelected(bool value)
	{
		selected = value;
	}

	int32_t idealWidth() override
	{
		return item->idealWidth();
	}

	int32_t idealHeight() override
	{
		return item->idealHeight();
	}

protected:
	void display(int xOffset, int yOffset) override;

protected:
	friend class DropdownWidget;
	void clearMouseDownState();

private:
	std::shared_ptr<WIDGET> item;
	DropdownOnSelectHandler onSelect;
	std::weak_ptr<DropdownWidget> parent;
	bool selected = false;
	bool mouseDownOnWrapper = false;
};

class DropdownWidget : public WIDGET
{
public:
	DropdownWidget();

	void addItem(const std::shared_ptr<WIDGET> &widget);
	void clear();
	void display(int xOffset, int yOffset) override;
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void run(W_CONTEXT *) override;
	void geometryChanged() override;
	void open();
	bool isOpen() const;
	void close();
	std::shared_ptr<WIDGET> findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;
	void setListHeight(uint32_t value)
	{
		itemsList->setGeometry(itemsList->x(), itemsList->y(), itemsList->width(), value);
	}
	void setListBackgroundColor(const PIELIGHT& color);
	const Padding& getDropdownMenuOuterPadding() const;
	bool setSelectedIndex(size_t index)
	{
		ASSERT_OR_RETURN(false, index < items.size(), "Invalid dropdown item index");
		return select(items[index], index);
	}
	void setCanChange(std::function<bool(DropdownWidget&, size_t newIndex, std::shared_ptr<WIDGET>)> value)
	{
		canChange = value;
	}
	void setOnChange(std::function<void(DropdownWidget&)> value)
	{
		onChange = value;
	}
	void setOnOpen(std::function<void(DropdownWidget&)> value)
	{
		onOpen = value;
	}
	void setOnClose(std::function<void(DropdownWidget&)> value)
	{
		onClose = value;
	}
	std::shared_ptr<WIDGET> getItem(size_t idx) const
	{
		if (idx >= items.size())
		{
			return nullptr;
		}
		return items[idx]->getItem();
	}
	std::shared_ptr<WIDGET> getSelectedItem() const
	{
		if (selectedItem == nullptr)
		{
			return nullptr;
		}
		return selectedItem->getItem();
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

	// Show a dropdown caret image on the right side of the widget
	void setDropdownCaretImage(optional<AtlasImage> image, const WzSize& displaySize, const Padding& padding = {});

	void setDisabled(bool isDisabled);
	bool getIsDisabled() const;

	enum class DropdownMenuStyle
	{
		InPlace,	// dropdown menu appears *overtop* of the currently-selected item (i.e. in-place)
		Separate	// dropdown menu appears below (or above if not enough room) the parent DropdownWidget
	};
	void setStyle(DropdownMenuStyle menuStyle);

	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;

	int32_t getItemDisplayWidth() const;
	int32_t getCaretImageUsedWidth() const;

protected:
	friend class DropdownItemWrapper;
	void setMouseClickOnItem(std::shared_ptr<DropdownItemWrapper> item, WIDGET_KEY key, bool wasPressed);

	void drawDropdownCaretImage(int xOffset, int yOffset, PIELIGHT color);
	virtual void drawOpenedHighlight(int xOffset, int yOffset);
	virtual void drawSelectedItem(const std::shared_ptr<WIDGET>& item, const WzRect& screenDisplayArea);

	virtual void onSelectedItemChanged();

	virtual int calculateDropdownListScreenPosX() const;
	virtual int calculateDropdownListDisplayWidth() const;

	void callRunOnItems();

private:
	std::vector<std::shared_ptr<DropdownItemWrapper>> items;
	std::shared_ptr<ScrollableListWidget> itemsList;
	std::shared_ptr<W_SCREEN> overlayScreen;
	std::shared_ptr<DropdownItemWrapper> selectedItem;
	std::function<bool(DropdownWidget&, size_t newIndex, std::shared_ptr<WIDGET> newSelectedWidget)> canChange;
	std::function<void(DropdownWidget&)> onChange;
	std::function<void(DropdownWidget&)> onOpen;
	std::function<void(DropdownWidget&)> onClose;
	std::shared_ptr<DropdownItemWrapper> mouseOverItem;
	std::shared_ptr<DropdownItemWrapper> mouseDownItem;
	int32_t overlayYPosOffset = 0;
	optional<AtlasImage> dropdownCaretImage;
	WzSize dropdownCaretImageSize;
	Padding dropdownCaretImagePadding = {};
	DropdownMenuStyle menuStyle = DropdownMenuStyle::InPlace;
	bool isDisabled = false;

	bool select(const std::shared_ptr<DropdownItemWrapper> &selected, size_t selectedIndex);
	int calculateDropdownListScreenPosY() const;
};

#endif // __INCLUDED_LIB_WIDGET_DROPDOWN_H__
