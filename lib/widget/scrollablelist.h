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
 * Definitions for scrollable list functions.
 */

#ifndef __INCLUDED_LIB_WIDGET_SCROLLABLELIST_H__
#define __INCLUDED_LIB_WIDGET_SCROLLABLELIST_H__

#include "widget.h"
#include "scrollbar.h"
#include "cliprect.h"

class ScrollableListWidget : public WIDGET
{
protected:
	ScrollableListWidget(): WIDGET() {}
	virtual void initialize();

public:
	static std::shared_ptr<ScrollableListWidget> make()
	{
		class make_shared_enabler: public ScrollableListWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize();
		return widget;
	}

	void run(W_CONTEXT *psContext) override;
	void addItem(const std::shared_ptr<WIDGET> &widget);
	void clear();
	bool processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;
	void enableScroll();
	void disableScroll();
	void setStickToBottom(bool value);
	void setPadding(Padding const &rect);
	void setSnapOffset(bool value);
	void setBackgroundColor(PIELIGHT const &color);
	void setItemSpacing(uint32_t value);
	uint32_t calculateListViewHeight() const;
	uint32_t calculateListViewWidth() const;
	void display(int xOffset, int yOffset) override;
	void displayRecursive(WidgetGraphicsContext const& context) override;
	int getScrollbarWidth() const;
	uint16_t getScrollPosition() const;
	void setScrollPosition(uint16_t newPosition);
	int32_t idealWidth() override;

protected:
	void geometryChanged() override;

private:
	std::shared_ptr<ScrollBarWidget> scrollBar;
	std::shared_ptr<ClipRectWidget> listView;
	uint32_t scrollableHeight = 0;
	bool snapOffset = true;
	bool layoutDirty = false;
	Padding padding = {0, 0, 0, 0};
	PIELIGHT backgroundColor;
	uint32_t itemSpacing = 0;

	uint32_t snappedOffset();
	void updateLayout();
	void resizeChildren(uint32_t width);
};

#endif // __INCLUDED_LIB_WIDGET_SCROLLABLELIST_H__
