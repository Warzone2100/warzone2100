// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

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
 * Functions for popover menu widget.
 */

#include "popovermenu.h"

// MARK: - MenuItemWrapper

MenuItemWrapper::MenuItemWrapper()
{ }

void MenuItemWrapper::initialize(const std::shared_ptr<PopoverMenuWidget>& psParent, const std::shared_ptr<WIDGET> &newItem, bool closeMenuOnClickFunc)
{
	item = newItem;
	parent = psParent;
	closeMenuOnClick = closeMenuOnClickFunc;
	attach(item);
	setGeometry(0, 0, idealWidth(), idealHeight());
}

std::shared_ptr<MenuItemWrapper> MenuItemWrapper::wrap(const std::shared_ptr<PopoverMenuWidget>& parent, const std::shared_ptr<WIDGET> &newItem, bool closeMenuOnClick)
{
	class make_shared_enabler: public MenuItemWrapper {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize(parent, newItem, closeMenuOnClick);
	return widget;
}

const std::shared_ptr<WIDGET>& MenuItemWrapper::getItem() const
{
	return item;
}

bool MenuItemWrapper::getCloseMenuOnClick() const
{
	return closeMenuOnClick;
}

std::shared_ptr<WIDGET> MenuItemWrapper::findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	W_CONTEXT inputContext(psContext);
	auto result = WIDGET::findMouseTargetRecursive(psContext, key, wasPressed);

	if (result && key != WKEY_NONE && this->hitTest(inputContext.mx, inputContext.my))
	{
		if (auto psStrongParent = parent.lock())
		{
			psStrongParent->handleMouseClickDownOnItem(std::static_pointer_cast<MenuItemWrapper>(shared_from_this()), key, wasPressed);
		}

		if (wasPressed)
		{
			mouseDownOnWrapper = true;
		}
		else
		{
			// if mouse down + mouse up on the item
			if (mouseDownOnWrapper)
			{
				mouseDownOnWrapper = false;
				if (auto psStrongParent = parent.lock())
				{
					psStrongParent->handleMouseReleasedOnItem(std::static_pointer_cast<MenuItemWrapper>(shared_from_this()), key);
				}
			}
		}
	}

	return result;
}

void MenuItemWrapper::geometryChanged()
{
	item->setGeometry(0, 0, width(), height());
}

void MenuItemWrapper::display(int xOffset, int yOffset)
{
	// no-op
}

int32_t MenuItemWrapper::idealWidth()
{
	return item->idealWidth();
}

int32_t MenuItemWrapper::idealHeight()
{
	return item->idealHeight();
}

void MenuItemWrapper::clearMouseDownState()
{
	mouseDownOnWrapper = false;
}

// MARK: - PopoverMenuWidget

PopoverMenuWidget::PopoverMenuWidget()
{ }

void PopoverMenuWidget::initialize()
{
	menuList = ScrollableListWidget::make();
	menuList->setPadding(Padding{5, 5, 5, 5});
	menuList->setBackgroundColor(pal_RGBA(20, 20, 20, 255));
	attach(menuList);
}

std::shared_ptr<PopoverMenuWidget> PopoverMenuWidget::make()
{
	class make_shared_enabler: public PopoverMenuWidget { };
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize();
	return widget;
}

void PopoverMenuWidget::addMenuItem(const std::shared_ptr<WIDGET>& widget, bool closesMenuOnClick)
{
	std::shared_ptr<PopoverMenuWidget> self = std::static_pointer_cast<PopoverMenuWidget>(shared_from_this());
	auto wrappedItem = MenuItemWrapper::wrap(self, widget, closesMenuOnClick);
	menuList->addItem(wrappedItem);
	maxItemHeight = std::max<int32_t>(maxItemHeight, wrappedItem->idealHeight());
}

size_t PopoverMenuWidget::numItems() const
{
	return menuList->numItems();
}

int PopoverMenuWidget::maxItemIdealHeight() const
{
	return maxItemHeight;
}

const Padding& PopoverMenuWidget::getPadding() const
{
	return menuList->getPadding();
}

std::shared_ptr<PopoverWidget> PopoverMenuWidget::openMenu(const std::shared_ptr<WIDGET>& parent, PopoverWidget::Alignment align, Vector2i positionOffset, const PopoverWidget::OnPopoverCloseHandlerFunc& onCloseHandler)
{
	if (auto strongCurrentPopover = currentPopover.lock())
	{
		strongCurrentPopover->close();
	}
	// Create a PopoverWidget, set this as its contents
	auto result = PopoverWidget::makePopover(parent, shared_from_this(), PopoverWidget::Style::Interactive, align, positionOffset, onCloseHandler);
	currentPopover = result;
	return result;
}

void PopoverMenuWidget::closeMenu()
{
	if (auto strongCurrentPopover = currentPopover.lock())
	{
		strongCurrentPopover->close();
		currentPopover.reset();
	}
}

int32_t PopoverMenuWidget::idealWidth()
{
	return menuList->idealWidth();
}

int32_t PopoverMenuWidget::idealHeight()
{
	return menuList->idealHeight();
}

void PopoverMenuWidget::geometryChanged()
{
	menuList->setGeometry(0, 0, width(), height());
}

void PopoverMenuWidget::handleMouseClickDownOnItem(std::shared_ptr<MenuItemWrapper> item, WIDGET_KEY key, bool wasPressed)
{
	if (item == mouseDownItem) { return; }

	if (mouseDownItem)
	{
		mouseDownItem->clearMouseDownState();
	}

	if (wasPressed)
	{
		mouseDownItem = item;
	}
	else
	{
		mouseDownItem.reset();
	}
}

void PopoverMenuWidget::handleMouseReleasedOnItem(std::shared_ptr<MenuItemWrapper> item, WIDGET_KEY)
{
	if (item != mouseDownItem)
	{
		if (mouseDownItem)
		{
			mouseDownItem->clearMouseDownState();
			mouseDownItem.reset();
		}
		return;
	}

	if (item && item->getCloseMenuOnClick())
	{
		closeMenu();
	}
	mouseDownItem.reset();
}
