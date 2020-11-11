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

struct Padding
{
	uint32_t top;
	uint32_t right;
	uint32_t bottom;
	uint32_t left;
};

class ScrollableListWidget : public WIDGET
{
public:
	ScrollableListWidget(WIDGET *parent);

	void initializeLayout();
	void run(W_CONTEXT *psContext) override;
	void addItem(WIDGET *widget);
	bool processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;
	void enableScroll();
	void disableScroll();
	void setStickToBottom(bool value);
	void setPadding(Padding const &rect);
	void setSnapOffset(bool value);
	uint32_t calculateListViewHeight() const;
	uint32_t calculateListViewWidth() const;
	void displayRecursive(WidgetGraphicsContext const& context) override;

protected:
	void geometryChanged() override;

private:
	ScrollBarWidget *scrollBar;
	ClipRectWidget *listView;
	uint16_t scrollableHeight = 0;
	bool snapOffset = true;
	bool layoutDirty = false;
	Padding padding = {0, 0, 0, 0};

	uint16_t snappedOffset();
	void updateLayout();
	void resizeChildren(uint32_t width);
};

#endif // __INCLUDED_LIB_WIDGET_SCROLLABLELIST_H__
