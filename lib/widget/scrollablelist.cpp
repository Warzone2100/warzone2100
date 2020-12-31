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
 * Functions for scrollable list.
 */

#include "scrollablelist.h"
#include "lib/framework/input.h"
#include "lib/ivis_opengl/pieblitfunc.h"

static const auto SCROLLBAR_WIDTH = 15;

void ScrollableListWidget::initialize()
{
	attach(scrollBar = ScrollBarWidget::make());
	attach(listView = std::make_shared<ClipRectWidget>());
	scrollBar->show(false);
	backgroundColor.rgba = 0;
}

void ScrollableListWidget::geometryChanged()
{
	scrollBar->setGeometry(width() - SCROLLBAR_WIDTH, 0, SCROLLBAR_WIDTH, height());
	scrollBar->setViewSize(calculateListViewHeight());
	layoutDirty = true;
}

void ScrollableListWidget::run(W_CONTEXT *psContext)
{
	updateLayout();
	listView->setTopOffset(snapOffset ? snappedOffset() : scrollBar->position());
}

/**
 * Snap offset to first visible child.
 *
 * This wouldn't be necessary if it were possible to clip the rendering.
 */
uint32_t ScrollableListWidget::snappedOffset()
{
	for (auto child : listView->children())
	{
		if (child->y() >= scrollBar->position())
		{
			return child->y();
		}
	}

	return 0;
}

void ScrollableListWidget::addItem(const std::shared_ptr<WIDGET> &item)
{
	listView->attach(item);
	layoutDirty = true;
}

void ScrollableListWidget::clear()
{
	listView->removeAllChildren();
	layoutDirty = true;
	updateLayout();
	listView->setTopOffset(0);
}

void ScrollableListWidget::updateLayout()
{
	if (!layoutDirty) {
		return;
	}
	layoutDirty = false;

	auto listViewWidthWithoutScrollBar = calculateListViewWidth();
	auto listViewWidthWithScrollBar = listViewWidthWithoutScrollBar - scrollBar->width();
	auto listViewHeight = calculateListViewHeight();

	resizeChildren(listViewWidthWithScrollBar);

	scrollBar->show(scrollableHeight > listViewHeight);

	if (scrollBar->visible())
	{
		listView->setGeometry(padding.left, padding.top, listViewWidthWithScrollBar, listViewHeight);
	} else {
		resizeChildren(listViewWidthWithoutScrollBar);
		listView->setGeometry(padding.left, padding.top, listViewWidthWithoutScrollBar, listViewHeight);
	}

	scrollBar->setScrollableSize(scrollableHeight);
}

void ScrollableListWidget::resizeChildren(uint32_t width)
{
	scrollableHeight = 0;
	auto nextOffset = 0;
	for (auto child : listView->children())
	{
		child->setGeometry(0, nextOffset, width, child->height());
		scrollableHeight = nextOffset + child->height();
		nextOffset = scrollableHeight + itemSpacing;
	}
}

uint32_t ScrollableListWidget::calculateListViewHeight() const
{
	return height() - padding.top - padding.bottom;
}

uint32_t ScrollableListWidget::calculateListViewWidth() const
{
	return width() - padding.left - padding.right;
}

bool ScrollableListWidget::processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	scrollBar->incrementPosition(-getMouseWheelSpeed().y * 20);
	return WIDGET::processClickRecursive(psContext, key, wasPressed);
}

void ScrollableListWidget::enableScroll()
{
	scrollBar->enable();
}

void ScrollableListWidget::disableScroll()
{
	scrollBar->disable();
}

void ScrollableListWidget::setStickToBottom(bool value)
{
	scrollBar->setStickToBottom(value);
}

void ScrollableListWidget::setPadding(Padding const &rect)
{
	padding = rect;
	layoutDirty = true;
}

void ScrollableListWidget::setBackgroundColor(PIELIGHT const &color)
{
	backgroundColor = color;
}

void ScrollableListWidget::setSnapOffset(bool value)
{
	snapOffset = value;
}

void ScrollableListWidget::setItemSpacing(uint32_t value)
{
	itemSpacing = value;
}

void ScrollableListWidget::display(int xOffset, int yOffset)
{
	if (backgroundColor.rgba != 0)
	{
		int x0 = x() + xOffset;
		int y0 = y() + yOffset;
		pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), backgroundColor);
	}
}

void ScrollableListWidget::displayRecursive(WidgetGraphicsContext const& context)
{
	updateLayout();
	WIDGET::displayRecursive(context);
}
