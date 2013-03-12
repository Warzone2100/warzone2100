/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

#include "widget.h"


struct TabSelectionStyle
{
	TabSelectionStyle() {}
	TabSelectionStyle(Image tab, Image tabDown, Image tabHighlight, Image prev, Image prevDown, Image prevHighlight, Image next, Image nextDown, Image nextHighlight, int gap);

	QSize tabSize;
	QSize scrollTabSize;
	Image tabImage, tabImageDown, tabImageHighlight;
	Image prevScrollTabImage, prevScrollTabImageDown, prevScrollTabImageHighlight;
	Image nextScrollTabImage, nextScrollTabImageDown, nextScrollTabImageHighlight;
	int tabGap;
};

class TabSelectionWidget : public WIDGET
{
	Q_OBJECT

public:
	TabSelectionWidget(WIDGET *parent);

	void setHeight(int height);
	void addStyle(TabSelectionStyle const &style);

	int tabs() const { return tabButtons.size(); }

signals:
	void tabChanged(int);

public slots:
	void setTab(int tab);
	void setNumberOfTabs(int tabs);

private slots:
	void prevTabPage();
	void nextTabPage();

private:
	void doLayoutAll();

	std::vector<TabSelectionStyle> styles;
	unsigned currentTab;
	unsigned tabsAtOnce;
	std::vector<W_BUTTON *> tabButtons;
	W_BUTTON *prevTabPageButton;
	W_BUTTON *nextTabPageButton;
	class QSignalMapper *setTabMapper;
};

class ListWidget : public WIDGET
{
	Q_OBJECT

public:
	enum Order {RightThenDown, DownThenRight};

	ListWidget(WIDGET *parent);

	void setChildSize(int width, int height);  ///< Sets the size of all child widgets (applied by calling addWidgetToLayout).
	void setChildSpacing(int width, int height);  ///< Sets the distance between child widgets (applied by calling addWidgetToLayout).
	void setOrder(Order order);  ///< Sets whether subsequent child widgets are placed in horizontal or vertical lines (applied by calling addWidgetToLayout).
	void addWidgetToLayout(WIDGET *widget);  ///< Manages the geometry of widget, and shows/hides it when changing tabs.

	int currentPage() const { return currentPage_; }
	int pages() const { return std::max(((int)myChildren.size() - 1)/widgetsPerPage(), 0) + 1; }

signals:
	void currentPageChanged(int);
	void numberOfPagesChanged(int);

public slots:
	void setCurrentPage(int page);

private:
	void doLayoutAll();
	void doLayout(int num);

	int widgetsPerPage() const { return widgetsPerRow()*widgetsPerColumn(); }
	int widgetsPerRow() const { return std::max((width() + spacing.width())/widgetSkipX(), 1); }
	int widgetsPerColumn() const { return std::max((height() + spacing.height())/widgetSkipY(), 1); }
	int widgetSkipX() const { return childSize.width() + spacing.width(); }
	int widgetSkipY() const { return childSize.height() + spacing.height(); }

	QSize childSize;
	QSize spacing;
	unsigned currentPage_;
	std::vector<WIDGET *> myChildren;
	Order order;
};

class ListTabWidget : public WIDGET
{
	Q_OBJECT

public:
	enum TabPosition {Top, Bottom};

	ListTabWidget(WIDGET *parent);

	virtual void geometryChanged();

	void setChildSize(int width, int height) { widgets->setChildSize(width, height); }  ///< Sets the size of all child widgets (applied by calling addWidgetToLayout).
	void setChildSpacing(int width, int height) { widgets->setChildSpacing(width, height); }  ///< Sets the distance between child widgets (applied by calling addWidgetToLayout).
	void setOrder(ListWidget::Order order) { widgets->setOrder(order); }  ///< Sets whether subsequent child widgets are placed in horizontal or vertical lines (applied by calling addWidgetToLayout).
	void addWidgetToLayout(WIDGET *widget);  ///< Manages the geometry of widget, and shows/hides it when changing tabs.
	bool setCurrentPage(int page) { widgets->setCurrentPage(page); return widgets->currentPage() == page; }
	int currentPage() const { return widgets->currentPage(); }
	int pages() const { return widgets->pages(); }

	void setTabPosition(TabPosition pos);

	TabSelectionWidget *tabWidget() { return tabs; }
	ListWidget *listWidget() { return widgets; }

private:
	TabSelectionWidget *tabs;
	ListWidget *widgets;
	TabPosition tabPos;
};

#endif //THISISALISTWIDGET_H
