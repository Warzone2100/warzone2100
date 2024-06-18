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
	scrollbarWidth = SCROLLBAR_WIDTH;
	backgroundColor.rgba = 0;
	borderColor.rgba = 0;
}

void ScrollableListWidget::geometryChanged()
{
	scrollBar->setGeometry(width() - scrollbarWidth, 0, scrollbarWidth, height());
	scrollBar->setViewSize(height());
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
		if (child->geometry().bottom() < scrollBar->position())
		{
			continue;
		}

		const auto childOffsets = child->getScrollSnapOffsets().value_or(std::vector<uint32_t>{0});
		for (const auto childOffset: childOffsets)
		{
			const auto y = child->y() + childOffset;
			if (y >= scrollBar->position())
			{
				return y;
			}
		}
	}

	return 0;
}

uint32_t ScrollableListWidget::getScrollPositionForItem(size_t itemNum)
{
	ASSERT_OR_RETURN(0, itemNum < listView->children().size(), "Invalid itemNum: %zu", itemNum);
	auto& child = listView->children()[itemNum];
	const auto childOffsets = child->getScrollSnapOffsets().value_or(std::vector<uint32_t>{0});
	for (const auto childOffset: childOffsets)
	{
		const auto y = child->y() + childOffset;
		return y;
	}
	return child->y();
}

int32_t ScrollableListWidget::getCurrentYPosOfItem(size_t itemNum)
{
	int currTopOffset = listView->getTopOffset();
	auto itemYPos = getScrollPositionForItem(itemNum);
	return itemYPos - currTopOffset;
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

size_t ScrollableListWidget::numItems() const
{
	return listView->children().size();
}

void ScrollableListWidget::updateLayout()
{
	if (!layoutDirty) {
		return;
	}
	layoutDirty = false;

	auto listViewWidthWithoutScrollBar = calculateListViewWidth();
	auto widthOfScrollbar = scrollBar->width();
	auto listViewWidthWithScrollBar = (widthOfScrollbar >= 0 && listViewWidthWithoutScrollBar > static_cast<uint32_t>(widthOfScrollbar)) ? listViewWidthWithoutScrollBar - static_cast<uint32_t>(widthOfScrollbar) : 0;
	auto listViewHeight = calculateListViewHeight();

	resizeChildren(listViewWidthWithScrollBar);

	scrollBar->show(scrollableHeight > listViewHeight);

	if (scrollBar->visible() || !expandWidthWhenScrollbarInvisible)
	{
		listView->setGeometry(padding.left, padding.top, listViewWidthWithScrollBar, listViewHeight);
	} else {
		resizeChildren(listViewWidthWithoutScrollBar);
		listView->setGeometry(padding.left, padding.top, listViewWidthWithoutScrollBar, listViewHeight);
	}

	scrollBar->setScrollableSize(scrollableHeight + padding.top + padding.bottom);
}

void ScrollableListWidget::resizeChildren(uint32_t width)
{
	scrollableHeight = 0;
	auto nextOffset = 0;
	for (auto& child : listView->children())
	{
		child->setGeometry(0, nextOffset, width, child->height());
		scrollableHeight = nextOffset + child->height();
		nextOffset = scrollableHeight + itemSpacing;
	}
}

uint32_t ScrollableListWidget::calculateListViewHeight() const
{
	int32_t result = height() - static_cast<int32_t>(padding.top) - static_cast<int32_t>(padding.bottom);
	return (result > 0) ? static_cast<uint32_t>(result) : 0;
}

uint32_t ScrollableListWidget::calculateListViewWidth() const
{
	int32_t result = width() - padding.left - padding.right;
	return (result > 0) ? static_cast<uint32_t>(result) : 0;
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

void ScrollableListWidget::setBorderColor(PIELIGHT const &color)
{
	borderColor = color;
}

void ScrollableListWidget::setSnapOffset(bool value)
{
	snapOffset = value;
}

void ScrollableListWidget::setItemSpacing(uint32_t value)
{
	if (value == itemSpacing)
	{
		return;
	}
	itemSpacing = value;
	layoutDirty = true;
}

void ScrollableListWidget::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	if (backgroundColor.rgba != 0)
	{
		pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), backgroundColor);
	}

	if (borderColor.rgba != 0)
	{
		int x1 = x0 + width();
		int y1 = y0 + height();
		iV_Box(x0, y0, x1, y1, borderColor);
	}
}

void ScrollableListWidget::displayRecursive(WidgetGraphicsContext const& context)
{
	updateLayout();
	WIDGET::displayRecursive(context);
}

int ScrollableListWidget::getScrollbarWidth() const
{
	return scrollbarWidth;
}

void ScrollableListWidget::setScrollbarWidth(int newWidth)
{
	scrollbarWidth = newWidth;
	geometryChanged();
	layoutDirty = true;
	updateLayout();
}

void ScrollableListWidget::setExpandWhenScrollbarInvisible(bool expandWidth)
{
	expandWidthWhenScrollbarInvisible = expandWidth;
}

uint16_t ScrollableListWidget::getScrollPosition() const
{
	return scrollBar->position();
}

void ScrollableListWidget::setScrollPosition(uint16_t newPosition)
{
	updateLayout();
	scrollBar->setPosition(newPosition);
	listView->setTopOffset(snapOffset ? snappedOffset() : scrollBar->position());
}

int32_t ScrollableListWidget::idealWidth()
{
	auto maxItemIdealWidth = 0;
	for (auto &item: listView->children())
	{
		maxItemIdealWidth = std::max(maxItemIdealWidth, item->idealWidth());
	}

	return maxItemIdealWidth + padding.left + padding.right + scrollbarWidth;
}

int32_t ScrollableListWidget::idealHeight()
{
	updateLayout();
	return scrollableHeight + padding.top + padding.bottom;
}

void ScrollableListWidget::scrollToItem(size_t itemNum)
{
	updateLayout();
	scrollBar->setPosition(getScrollPositionForItem(itemNum));
	listView->setTopOffset(snapOffset ? snappedOffset() : scrollBar->position());
}

bool ScrollableListWidget::isItemVisible(size_t itemNum)
{
	ASSERT_OR_RETURN(false, itemNum < listView->children().size(), "Invalid itemNum: %zu", itemNum);
	updateLayout();
	return listView->isChildVisible(listView->children()[itemNum]);
}

void ScrollableListWidget::scrollEnsureItemVisible(size_t itemNum)
{
	if (!visible() || width() <= 0 || height() <= 0)
	{
		// no-op if this isn't visible
		return;
	}
	if (!isItemVisible(itemNum))
	{
		auto scrollPosOfItem = getScrollPositionForItem(itemNum);
		if (scrollPosOfItem < scrollBar->position())
		{
			// scroll up the minimum amount
			scrollBar->setPosition(scrollPosOfItem);
		}
		else
		{
			// scroll down the minimum amount
			auto desiredScrollPos = std::max<int32_t>(scrollPosOfItem - listView->height() + listView->children()[itemNum]->height(), 0);
			scrollBar->setPosition(desiredScrollPos);
		}
		listView->setTopOffset(snapOffset ? snappedOffset() : scrollBar->position());
	}
}

void ScrollableListWidget::setListTransparentToMouse(bool hasMouseTransparency)
{
	listView->setTransparentToMouse(hasMouseTransparency);
}

// MARK: - ClickableScrollableList

std::shared_ptr<ClickableScrollableList> ClickableScrollableList::make()
{
	class make_shared_enabler: public ClickableScrollableList {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize(); // calls ScrollableListWidget::initialize
	widget->setListTransparentToMouse(true);
	return widget;
}

void ClickableScrollableList::setOnClickHandler(const ClickableScrollableList_OnClick_Func& _onClickFunc)
{
	onClickFunc = _onClickFunc;
}

void ClickableScrollableList::setOnHighlightHandler(const ClickableScrollableList_OnHighlight_Func& _onHighlightFunc)
{
	onHighlightFunc = _onHighlightFunc;
}

void ClickableScrollableList::clicked(W_CONTEXT *, WIDGET_KEY)
{
	mouseDownOnList = true;
}

void ClickableScrollableList::released(W_CONTEXT *, WIDGET_KEY)
{
	bool wasFullClick = mouseDownOnList;
	mouseDownOnList = false;
	if (wasFullClick)
	{
		if (onClickFunc)
		{
			onClickFunc(*this);
		}
	}
}

void ClickableScrollableList::highlight(W_CONTEXT *)
{
	if (onHighlightFunc)
	{
		onHighlightFunc(*this, true);
	}
}

void ClickableScrollableList::highlightLost()
{
	mouseDownOnList = false;
	if (onHighlightFunc)
	{
		onHighlightFunc(*this, false);
	}
}
