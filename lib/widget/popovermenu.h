// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025-2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/**
 * @file
 * Definitions for popover menu widget.
 */

#ifndef __INCLUDED_LIB_WIDGET_POPOVER_MENU_H__
#define __INCLUDED_LIB_WIDGET_POPOVER_MENU_H__

#include "widget.h"
#include "scrollablelist.h"
#include "popover.h"

class PopoverMenuWidget;

class MenuItemWrapper: public WIDGET
{
public:
	typedef std::function<void(std::shared_ptr<MenuItemWrapper>)> OnClickHandler;
protected:
	MenuItemWrapper();
	void initialize(const std::shared_ptr<PopoverMenuWidget>& parent, const std::shared_ptr<WIDGET> &newItem, bool closeMenuOnClick);

public:
	static std::shared_ptr<MenuItemWrapper> wrap(const std::shared_ptr<PopoverMenuWidget>& parent, const std::shared_ptr<WIDGET> &item, bool closeMenuOnClick);

	const std::shared_ptr<WIDGET>& getItem() const;
	bool getCloseMenuOnClick() const;

	virtual std::shared_ptr<WIDGET> findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;
	virtual void geometryChanged() override;

	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;

protected:
	void display(int xOffset, int yOffset) override;

	friend class PopoverMenuWidget;
	void clearMouseDownState();

private:
	std::shared_ptr<WIDGET> item;
	std::weak_ptr<PopoverMenuWidget> parent;
	bool closeMenuOnClick = false;
	bool mouseDownOnWrapper = false;
};

class PopoverMenuWidget : public WIDGET
{
public:
	typedef std::function<void ()> DismissPopoverFunc;
protected:
	PopoverMenuWidget();
	void initialize();
public:
	static std::shared_ptr<PopoverMenuWidget> make();

	void addMenuItem(const std::shared_ptr<WIDGET>& widget, bool closesMenuOnClick = false);
	size_t numItems() const;
	int maxItemIdealHeight() const;
	const Padding& getPadding() const;

	std::shared_ptr<PopoverWidget> openMenu(const std::shared_ptr<WIDGET>& parent, PopoverWidget::Alignment align = PopoverWidget::Alignment::LeftOfParent, Vector2i positionOffset = Vector2i(0, 0), const PopoverWidget::OnPopoverCloseHandlerFunc& onCloseHandler = nullptr);
	void closeMenu();

	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
	virtual void geometryChanged() override;
protected:
	friend class MenuItemWrapper;
	void handleMouseClickDownOnItem(std::shared_ptr<MenuItemWrapper> item, WIDGET_KEY key, bool wasPressed);
	void handleMouseReleasedOnItem(std::shared_ptr<MenuItemWrapper> item, WIDGET_KEY key);
private:
	std::shared_ptr<ScrollableListWidget> menuList;
	std::weak_ptr<PopoverWidget> currentPopover;
	std::shared_ptr<MenuItemWrapper> mouseDownItem;
	int32_t maxItemHeight = 0;
};

#endif // __INCLUDED_LIB_WIDGET_POPOVER_MENU_H__
