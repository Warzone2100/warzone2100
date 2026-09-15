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
#include "lib/gamelib/gtime.h"
#include <cmath>

static const auto SCROLLBAR_WIDTH = 15;

void ScrollableListWidget::initialize()
{
	attach(scrollBar = ScrollBarWidget::make());
	attach(listView = std::make_shared<ClipRectWidget>());
	scrollBar->show(false);
	scrollbarWidth = SCROLLBAR_WIDTH;
	backgroundColor.clear();
	borderColor.clear();
	rowLinesColor = pal_RGBA(0,0,0,255);
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
	runTouchScroll();
	// (while a touch moves it, the content follows the finger: an item crossing the edge is hidden until it settles)
	listView->setTopOffset((snapOffset && !touchScrollMoving()) ? snappedOffset() : scrollBar->position());
}

WzDragAxes ScrollableListWidget::touchDragAxes()
{
	updateLayout();
	return (scrollBar->isEnabled() && scrollBar->visible()) ? WzDragAxes::Vertical : WzDragAxes::None;
}

bool ScrollableListWidget::interceptsTouchPress()
{
	if (flingVelocity == 0.f && !settling)
	{
		return false;
	}
	// A press stops the scrolling where it is, and isn't a press on the item under it
	flingVelocity = 0.f;
	settling = false;
	touchPressTaken = true;
	return true;
}

void ScrollableListWidget::mouseDragged(WIDGET_KEY, W_CONTEXT *start, W_CONTEXT *current)
{
	if (!touchDragging)
	{
		touchDragging = true;
		flingVelocity = 0.f;
		settling = false;
		touchDragStartPosition = scrollBar->position();
		touchSamples.clear();
	}
	touchPosition = std::clamp(static_cast<float>(touchDragStartPosition) - static_cast<float>(current->my - start->my), 0.f, static_cast<float>(scrollBar->maxPosition()));
	scrollBar->setPosition(static_cast<uint32_t>(std::lround(touchPosition)));
	// The finger's last 100 ms, for the flick's speed
	touchSamples.push_back({realTime, current->my});
	while (touchSamples.size() > 2 && realTime - touchSamples.front().time > 100)
	{
		touchSamples.erase(touchSamples.begin());
	}
}

void ScrollableListWidget::released(W_CONTEXT *, WIDGET_KEY)
{
	endTouchScroll();
}

bool ScrollableListWidget::endTouchScroll()
{
	const bool wasTouchScroll = touchDragging || touchPressTaken;
	if (touchDragging)
	{
		float velocity = 0.f;
		if (touchSamples.size() >= 2 && realTime - touchSamples.back().time < 60)
		{
			const auto elapsed = touchSamples.back().time - touchSamples.front().time;
			if (elapsed > 0)
			{
				velocity = -static_cast<float>(touchSamples.back().y - touchSamples.front().y) / static_cast<float>(elapsed);
			}
		}
		if (std::abs(velocity) > 0.15f)
		{
			flingVelocity = std::clamp(velocity, -6.f, 6.f);
			flingLastTime = realTime;
		}
		else
		{
			settleTouchScroll();
		}
	}
	else if (touchPressTaken)
	{
		settleTouchScroll();
	}
	touchDragging = false;
	touchPressTaken = false;
	touchSamples.clear();
	return wasTouchScroll;
}

bool ScrollableListWidget::isTouchScrolling() const
{
	return touchScrollMoving();
}

bool ScrollableListWidget::touchScrollMoving() const
{
	return touchDragging || touchPressTaken || flingVelocity != 0.f || settling;
}

void ScrollableListWidget::runTouchScroll()
{
	if (flingVelocity != 0.f)
	{
		const float elapsed = static_cast<float>(std::min<uint32_t>(realTime - flingLastTime, 100));
		flingLastTime = realTime;
		const float maxPosition = static_cast<float>(scrollBar->maxPosition());
		touchPosition += flingVelocity * elapsed;
		flingVelocity *= std::exp(-elapsed / 325.f);
		if (touchPosition <= 0.f || touchPosition >= maxPosition || std::abs(flingVelocity) < 0.02f)
		{
			touchPosition = std::clamp(touchPosition, 0.f, maxPosition);
			flingVelocity = 0.f;
		}
		scrollBar->setPosition(static_cast<uint32_t>(std::lround(touchPosition)));
		if (flingVelocity == 0.f)
		{
			settleTouchScroll();
		}
	}
	else if (settling)
	{
		const float t = std::min(1.f, static_cast<float>(realTime - settleStart) / 150.f);
		const float eased = 1.f - (1.f - t) * (1.f - t) * (1.f - t);
		const float position = static_cast<float>(settleFrom) + (static_cast<float>(settleTo) - static_cast<float>(settleFrom)) * eased;
		scrollBar->setPosition(static_cast<uint32_t>(std::lround(position)));
		if (t >= 1.f)
		{
			settling = false;
		}
	}
}

/* At rest after a touch, the list settles on the nearest item edge (as it would snap) */
void ScrollableListWidget::settleTouchScroll()
{
	settling = false;
	if (!snapOffset)
	{
		return;
	}
	settleFrom = scrollBar->position();
	settleTo = nearestSnapPosition(settleFrom);
	if (settleTo != settleFrom)
	{
		settling = true;
		settleStart = realTime;
	}
}

uint32_t ScrollableListWidget::nearestSnapPosition(uint32_t position)
{
	uint32_t nearest = 0;
	uint32_t nearestDistance = position;
	for (const auto& child : listView->children())
	{
		const auto childOffsets = child->getScrollSnapOffsets().value_or(std::vector<uint32_t>{0});
		for (const auto childOffset : childOffsets)
		{
			const auto y = static_cast<uint32_t>(std::max(0, child->y() + static_cast<int>(childOffset)));
			const auto distance = (y > position) ? y - position : position - y;
			if (distance < nearestDistance)
			{
				nearest = y;
				nearestDistance = distance;
			}
		}
	}
	return std::min(nearest, scrollBar->maxPosition());
}

/**
 * Snap offset to first visible child.
 *
 * This wouldn't be necessary if it were possible to clip the rendering.
 */
uint32_t ScrollableListWidget::snappedOffset()
{
	const auto& items = listView->children();
	for (size_t idx = 0; idx < items.size(); ++idx)
	{
		const auto& child = items[idx];
		if (child->geometry().bottom() < static_cast<int32_t>(scrollBar->position()))
		{
			continue;
		}

		const auto childOffsets = child->getScrollSnapOffsets().value_or(std::vector<uint32_t>{0});
		for (const auto childOffset: childOffsets)
		{
			const auto y = child->y() + childOffset;
			if (y >= scrollBar->position())
			{
				topVisibleItemIdx = idx;
				return y;
			}
		}
	}

	topVisibleItemIdx = 0;
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
	return (itemYPos - currTopOffset) + listView->y();
}

size_t ScrollableListWidget::addItem(const std::shared_ptr<WIDGET> &item)
{
	size_t newItemIdx = listView->children().size();
	listView->attach(item);
	layoutDirty = true;
	return newItemIdx;
}

void ScrollableListWidget::clear()
{
	listView->removeAllChildren();
	layoutDirty = true;
	updateLayout();
	listView->setTopOffset(0);
	topVisibleItemIdx = 0;
}

size_t ScrollableListWidget::numItems() const
{
	return listView->children().size();
}

std::shared_ptr<WIDGET> ScrollableListWidget::getItemAtIdx(size_t itemNum) const
{
	ASSERT_OR_RETURN(nullptr, itemNum < listView->children().size(), "itemNum out of bounds - number of items: %zu", listView->children().size());
	return listView->children()[itemNum];
}

const std::vector<std::shared_ptr<WIDGET>>& ScrollableListWidget::getItems() const
{
	return listView->children();
}

void ScrollableListWidget::updateLayout()
{
	if (!layoutDirty) {
		return;
	}
	layoutDirty = false;

	auto listViewWidthWithoutScrollBar = calculateListViewWidth();
	auto widthOfScrollbar = scrollBar->width();
	auto listViewWidthWithScrollBar = listViewWidthWithoutScrollBar;
	if (widthOfScrollbar > padding.right)
	{
		listViewWidthWithScrollBar = (widthOfScrollbar >= 0 && listViewWidthWithoutScrollBar > static_cast<uint32_t>(widthOfScrollbar)) ? listViewWidthWithoutScrollBar - static_cast<uint32_t>(widthOfScrollbar) : 0;
	}
	auto listViewHeight = calculateListViewHeight();

	bool needsScrollbar = true;
	if (expandWidthWhenScrollbarInvisible)
	{
		resizeChildren(listViewWidthWithoutScrollBar);
		needsScrollbar = scrollableHeight > listViewHeight;
	}
	if (needsScrollbar)
	{
		resizeChildren(listViewWidthWithScrollBar);
		needsScrollbar = scrollableHeight > listViewHeight;
	}
	scrollBar->show(needsScrollbar);

	const uint32_t listViewWidth = (needsScrollbar || !expandWidthWhenScrollbarInvisible)
		? listViewWidthWithScrollBar : listViewWidthWithoutScrollBar;
	listView->setGeometry(padding.left, padding.top, listViewWidth, listViewHeight);

	scrollBar->setScrollableSize(scrollableHeight + padding.top + padding.bottom);
}

void ScrollableListWidget::resizeChildren(uint32_t width)
{
	scrollableHeight = 0;
	auto nextOffset = 0;
	for (auto& child : listView->children())
	{
		if (!child->visible())
		{
			continue;
		}
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

bool ScrollableListWidget::canConsumeWheelScroll()
{
	updateLayout();
	return scrollBar->isEnabled() && scrollBar->visible();
}

std::shared_ptr<WIDGET> ScrollableListWidget::findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	auto result = WIDGET::findMouseTargetRecursive(psContext, key, wasPressed);

	// scroll with the mouse wheel only if no nested widget (between this widget and the
	// mouse target) handles the wheel itself - the innermost wheel consumer under the mouse wins
	bool nestedScrollableHandlesWheel = false;
	for (auto widget = result; widget != nullptr && widget.get() != this; widget = widget->parent())
	{
		if (widget->canConsumeWheelScroll())
		{
			nestedScrollableHandlesWheel = true;
			break;
		}
	}
	if (!nestedScrollableHandlesWheel)
	{
		scrollBar->incrementPosition(-getMouseWheelSpeed().y * 20);
	}

	return result;
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

const Padding& ScrollableListWidget::getPadding() const
{
	return padding;
}

void ScrollableListWidget::setDrawRowLines(bool bEnabled)
{
	drawRowLines = bEnabled;
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

	if (!backgroundColor.isTransparent())
	{
		pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), backgroundColor);
	}

	if (!borderColor.isTransparent())
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

	// *after* displaying all children, draw the row lines (if desired)
	if (!drawRowLines || itemSpacing == 0)
	{
		return;
	}

	if (!context.clipContains(geometry())) {
		return;
	}

	lineDraws.clear();

	int xOffset = context.getXOffset();
	int yOffset = context.getYOffset();

	int x0 = x() + xOffset;
	int y0 = y() + yOffset + listView->y();
	int x1 = x0 + listView->width();
	int y1 = y0 + listView->height();

	int listViewTopOffset = listView->getTopOffset();
	for (auto& child : listView->children())
	{
		int childY = child->y();
		if (childY < listViewTopOffset)
		{
			continue;
		}
		int childDisplayY0 = childY - static_cast<int>(listViewTopOffset);
		int rowLineY0 = y0 + childDisplayY0 - itemSpacing;
		if (rowLineY0 > y1 || rowLineY0 < y0)
		{
			continue;
		}
		lineDraws.emplace_back(x0, rowLineY0, x1, rowLineY0 + itemSpacing, rowLinesColor);
	}

	pie_DrawMultiRect(lineDraws);
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

void ScrollableListWidget::setScrollbarMinimumSliderSize(uint32_t value)
{
	scrollBar->setMinimumSliderSize(value);
}

void ScrollableListWidget::setExpandWhenScrollbarInvisible(bool expandWidth)
{
	expandWidthWhenScrollbarInvisible = expandWidth;
}

uint32_t ScrollableListWidget::getScrollPosition() const
{
	return scrollBar->position();
}

void ScrollableListWidget::setScrollPosition(uint32_t newPosition)
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
			// scroll down the minimum amount (signed intermediate: the
			// unsigned scroll pos minus the view height can be negative)
			int64_t desiredScrollPos = std::max<int64_t>(static_cast<int64_t>(scrollPosOfItem) - listView->height() + listView->children()[itemNum]->height(), 0);
			scrollBar->setPosition(static_cast<uint32_t>(desiredScrollPos));
		}
		listView->setTopOffset(snapOffset ? snappedOffset() : scrollBar->position());
	}
}

void ScrollableListWidget::setListTransparentToMouse(bool hasMouseTransparency)
{
	listView->setTransparentToMouse(hasMouseTransparency);
}

void ScrollableListWidget::setLayoutDirty()
{
	layoutDirty = true;
}

// yPos is widget coordinates
std::shared_ptr<WIDGET> ScrollableListWidget::getItemAtYPos(int32_t yPos)
{
	auto itemIdx = getItemIdxAtYPos(yPos);
	if (!itemIdx.has_value())
	{
		return nullptr;
	}
	const auto& items = listView->children();
	return items[itemIdx.value()];
}

// yPos is widget coordinates
optional<size_t> ScrollableListWidget::getItemIdxAtYPos(int32_t yPos)
{
	auto listViewY0 = listView->y();
	auto listViewHeight = listView->height();
	auto listViewY1 = listViewY0 + listViewHeight;
	if (yPos < listViewY0 || yPos > listViewY1)
	{
		return nullopt;
	}
	const auto& items = listView->children();
	for (size_t idx = topVisibleItemIdx; idx < items.size(); ++idx)
	{
		auto itemCurrentY0 = getCurrentYPosOfItem(idx);
		if (itemCurrentY0 >= listViewHeight)
		{
			break;
		}
		const auto& itemWidget = items[idx];
		auto itemCurrentY1 = itemCurrentY0 + itemWidget->height();
		if (yPos >= itemCurrentY0 && yPos < itemCurrentY1)
		{
			return idx;
		}
	}
	return nullopt;
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
	bool wasFullClick = mouseDownOnList && !endTouchScroll();
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
