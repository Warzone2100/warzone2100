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
#include "lib/ivis_opengl/pietypes.h"

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
	size_t addItem(const std::shared_ptr<WIDGET> &widget);
	size_t numItems() const;
	std::shared_ptr<WIDGET> getItemAtIdx(size_t itemNum) const;
	const std::vector<std::shared_ptr<WIDGET>>& getItems() const;
	void clear();
	std::shared_ptr<WIDGET> findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;
	void enableScroll();
	void disableScroll();
	void setStickToBottom(bool value);
	void setPadding(Padding const &rect);
	const Padding& getPadding() const;
	void setDrawRowLines(bool bEnabled);
	void setSnapOffset(bool value);
	void setBackgroundColor(PIELIGHT const &color);
	void setBorderColor(PIELIGHT const &color);
	void setItemSpacing(uint32_t value);
	uint32_t getItemSpacing() const { return itemSpacing; }
	uint32_t calculateListViewHeight() const;
	uint32_t calculateListViewWidth() const;
	void display(int xOffset, int yOffset) override;
	void displayRecursive(WidgetGraphicsContext const& context) override;
	int getScrollbarWidth() const;
	void setScrollbarWidth(int newWidth);
	void setExpandWhenScrollbarInvisible(bool expandWidth);
	uint16_t getScrollPosition() const;
	void setScrollPosition(uint16_t newPosition);
	void scrollToItem(size_t itemNum);
	void scrollEnsureItemVisible(size_t itemNum);
	bool isItemVisible(size_t itemNum);
	int32_t getCurrentYPosOfItem(size_t itemNum);
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
	void setListTransparentToMouse(bool hasMouseTransparency);
	void setLayoutDirty();
	std::shared_ptr<WIDGET> getItemAtYPos(int32_t yPos);
	optional<size_t> getItemIdxAtYPos(int32_t yPos);

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
	PIELIGHT borderColor;
	uint32_t itemSpacing = 0;
	int scrollbarWidth = 0;
	bool expandWidthWhenScrollbarInvisible = true;
	bool drawRowLines = false;
	PIELIGHT rowLinesColor;
	size_t topVisibleItemIdx = 0;
	std::vector<PIERECT_DrawRequest> lineDraws;

	uint32_t snappedOffset();
	void updateLayout();
	void resizeChildren(uint32_t width);
	uint32_t getScrollPositionForItem(size_t itemNum);
};

class ClickableScrollableList : public ScrollableListWidget
{
public:
	static std::shared_ptr<ClickableScrollableList> make();
	typedef std::function<void (ClickableScrollableList& list)> ClickableScrollableList_OnClick_Func;
	void setOnClickHandler(const ClickableScrollableList_OnClick_Func& _onClickFunc);
	typedef std::function<void (ClickableScrollableList& list, bool isHighlighted)> ClickableScrollableList_OnHighlight_Func;
	void setOnHighlightHandler(const ClickableScrollableList_OnHighlight_Func& _onHighlightFunc);
protected:
	void clicked(W_CONTEXT *, WIDGET_KEY) override;
	void released(W_CONTEXT *, WIDGET_KEY) override;
	void highlight(W_CONTEXT *) override;
	void highlightLost() override;
private:
	ClickableScrollableList_OnClick_Func onClickFunc;
	ClickableScrollableList_OnHighlight_Func onHighlightFunc;
	bool mouseDownOnList = false;
};

#endif // __INCLUDED_LIB_WIDGET_SCROLLABLELIST_H__
