/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
/*!
 * \file listwidget.h
 * \brief A list widget. Useful for displaying lists of buttons and stuff.
 */

#ifndef THISISALISTWIDGET_H
#define THISISALISTWIDGET_H

#include "lib/ivis_opengl/ivisdef.h"

#include "widget.h"
#include <functional>
#include <algorithm>


struct TabSelectionStyle
{
	TabSelectionStyle() {}
	TabSelectionStyle(Image tab, Image tabDown, Image tabHighlight, Image prev, Image prevDown, Image prevHighlight, Image next, Image nextDown, Image nextHighlight, int gap);

	WzSize tabSize;
	WzSize scrollTabSize;
	Image tabImage, tabImageDown, tabImageHighlight;
	Image prevScrollTabImage, prevScrollTabImageDown, prevScrollTabImageHighlight;
	Image nextScrollTabImage, nextScrollTabImageDown, nextScrollTabImageHighlight;
	int tabGap;
};

class TabSelectionWidget : public WIDGET
{
protected:
	TabSelectionWidget(): WIDGET(), currentTab(0), tabsAtOnce(1) {}
	virtual void initialize();

public:
	static std::shared_ptr<TabSelectionWidget> make()
	{
		class make_shared_enabler: public TabSelectionWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize();
		return widget;
	}

	void setHeight(int height);
	void addStyle(TabSelectionStyle const &tabStyle);

	size_t tabs() const
	{
		return tabButtons.size();
	}

	/* The optional "onTabChanged" callback function */
	typedef std::function<void (TabSelectionWidget& widget, size_t currentTab)> W_TABSELECTION_ON_TAB_CHANGED_FUNC;

	void addOnTabChangedHandler(const W_TABSELECTION_ON_TAB_CHANGED_FUNC& onTabChangedFunc);

public:
	void setTab(size_t tab);
	void setNumberOfTabs(size_t tabs);

private:
	void prevTabPage();
	void nextTabPage();

private:
	void doLayoutAll();

	std::vector<TabSelectionStyle> styles;
	size_t currentTab;
	size_t tabsAtOnce;
	std::vector<std::shared_ptr<W_BUTTON>> tabButtons;
	std::shared_ptr<W_BUTTON> prevTabPageButton;
	std::shared_ptr<W_BUTTON> nextTabPageButton;
	std::vector<W_TABSELECTION_ON_TAB_CHANGED_FUNC> onTabChangedHandlers;
};

class ListWidget : public WIDGET
{

public:
	enum Order {RightThenDown, DownThenRight};

	ListWidget();

	void widgetLost(WIDGET *widget) override;

	void setChildSize(int width, int height);  ///< Sets the size of all child widgets (applied by calling addWidgetToLayout).
	void setChildSpacing(int width, int height);  ///< Sets the distance between child widgets (applied by calling addWidgetToLayout).
	void setOrder(Order order);  ///< Sets whether subsequent child widgets are placed in horizontal or vertical lines (applied by calling addWidgetToLayout).
	void addWidgetToLayout(const std::shared_ptr<WIDGET> &widget);  ///< Manages the geometry of widget, and shows/hides it when changing tabs.

	size_t currentPage() const
	{
		return currentPage_;
	}
	size_t pages() const
	{
		return numberOfPages;
	}
	size_t childrenSize() const
	{
		return myChildren.size();
	}

	/* The optional "onCurrentPageChanged" callback function */
	typedef std::function<void (ListWidget& psWidget, size_t currentPage)> W_LISTWIDGET_ON_CURRENTPAGECHANGED_FUNC;

	/* The optional "onNumberOfPagesChanged" callback function */
	typedef std::function<void (ListWidget& psWidget, size_t numberOfPages)> W_LISTWIDGET_ON_NUMBEROFPAGESCHANGED_FUNC;

	void addOnCurrentPageChangedHandler(const W_LISTWIDGET_ON_CURRENTPAGECHANGED_FUNC& handlerFunc);
	void addOnNumberOfPagesChangedHandler(const W_LISTWIDGET_ON_NUMBEROFPAGESCHANGED_FUNC& handlerFunc);

public:
	void setCurrentPage(size_t page);

private:
	void doLayoutAll();
	void doLayout(size_t num);

	size_t widgetsPerPage() const
	{
		return widgetsPerRow() * widgetsPerColumn();
	}
	size_t widgetsPerRow() const
	{
		return static_cast<size_t>(std::max((width() + spacing.width()) / widgetSkipX(), 1));
	}
	size_t widgetsPerColumn() const
	{
		return static_cast<size_t>(std::max((height() + spacing.height()) / widgetSkipY(), 1));
	}
	int widgetSkipX() const
	{
		return childSize.width() + spacing.width();
	}
	int widgetSkipY() const
	{
		return childSize.height() + spacing.height();
	}
	void updateNumberOfPages();

	size_t numberOfPages = 1;
	WzSize childSize;
	WzSize spacing;
	size_t currentPage_;
	std::vector<std::shared_ptr<WIDGET>> myChildren;
	Order order;
	std::vector<W_LISTWIDGET_ON_CURRENTPAGECHANGED_FUNC> onCurrentPageChangedHandlers;
	std::vector<W_LISTWIDGET_ON_NUMBEROFPAGESCHANGED_FUNC> onNumberOfPagesChangedHandlers;
};

class ListTabWidget : public WIDGET
{
protected:
	ListTabWidget(): WIDGET(), tabPos(Top) {}
	virtual void initialize();

public:
	enum TabPosition {Top, Bottom};

	void geometryChanged() override;

	void setChildSize(int width, int height)
	{
		widgets->setChildSize(width, height);    ///< Sets the size of all child widgets (applied by calling addWidgetToLayout).
	}
	void setChildSpacing(int width, int height)
	{
		widgets->setChildSpacing(width, height);    ///< Sets the distance between child widgets (applied by calling addWidgetToLayout).
	}
	void setOrder(ListWidget::Order order)
	{
		widgets->setOrder(order);    ///< Sets whether subsequent child widgets are placed in horizontal or vertical lines (applied by calling addWidgetToLayout).
	}
	void addWidgetToLayout(const std::shared_ptr<WIDGET> &widget);  ///< Manages the geometry of widget, and shows/hides it when changing tabs.
	bool setCurrentPage(size_t page)
	{
		widgets->setCurrentPage(page);
		return widgets->currentPage() == page;
	}
	size_t currentPage() const
	{
		return widgets->currentPage();
	}
	size_t pages() const
	{
		return widgets->pages();
	}
	size_t childrenSize() const
	{
		return widgets->childrenSize();
	}

	void setTabPosition(TabPosition pos);

	TabSelectionWidget *tabWidget()
	{
		return tabs.get();
	}
	ListWidget *listWidget()
	{
		return widgets.get();
	}

private:
	std::shared_ptr<TabSelectionWidget> tabs;
	std::shared_ptr<ListWidget> widgets;
	TabPosition tabPos;
};

#endif //THISISALISTWIDGET_H
