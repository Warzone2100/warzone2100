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
/** @file
 *  Implementation of a list widget. Useful for displaying lists of buttons and stuff.
 */

#include "listwidget.h"
#include "button.h"
#include <QtCore/QSignalMapper>


TabSelectionStyle::TabSelectionStyle(Image tab, Image tabDown, Image tabHighlight, Image prev, Image prevDown, Image prevHighlight, Image next, Image nextDown, Image nextHighlight, int gap)
	: tabSize(tab.width(), tab.height())
	, scrollTabSize(0, 0)
	, tabImage(tab), tabImageDown(tabDown), tabImageHighlight(tabHighlight)
	, prevScrollTabImage(prev), prevScrollTabImageDown(prevDown), prevScrollTabImageHighlight(prevHighlight)
	, nextScrollTabImage(next), nextScrollTabImageDown(nextDown), nextScrollTabImageHighlight(nextHighlight)
	, tabGap(gap)
{
	if (!prev.isNull())
	{
		scrollTabSize = QSize(prev.width(), prev.height());
	}
}

TabSelectionWidget::TabSelectionWidget(WIDGET *parent)
	: WIDGET(parent)
	, currentTab(0)
	, tabsAtOnce(1)
	, prevTabPageButton(new W_BUTTON(this))
	, nextTabPageButton(new W_BUTTON(this))
	, setTabMapper(new QSignalMapper(this))
{
	connect(setTabMapper, SIGNAL(mapped(int)), this, SLOT(setTab(int)));
	connect(prevTabPageButton, SIGNAL(clicked()), this, SLOT(prevTabPage()));
	connect(nextTabPageButton, SIGNAL(clicked()), this, SLOT(nextTabPage()));

	prevTabPageButton->setTip(_("Tab Scroll left"));
	nextTabPageButton->setTip(_("Tab Scroll right"));

	setHeight(5);
	setNumberOfTabs(1);
}

void TabSelectionWidget::setHeight(int height)
{
	setGeometry(x(), y(), width(), height);
}

void TabSelectionWidget::addStyle(TabSelectionStyle const &style)
{
	styles.push_back(style);

	doLayoutAll();
}

void TabSelectionWidget::setTab(int tab)
{
	unsigned previousTab = currentTab;
	currentTab = clip(tab, 0, tabButtons.size() - 1);
	if (previousTab == currentTab)
	{
		return;  // Nothing to do.
	}
	doLayoutAll();
	emit tabChanged(currentTab);
}

void TabSelectionWidget::setNumberOfTabs(int tabs)
{
	ASSERT_OR_RETURN(, tabs >= 0, "?");

	unsigned previousSize = tabButtons.size();
	for (unsigned n = tabs; n < previousSize; ++n)
	{
		delete tabButtons[n];
	}
	tabButtons.resize(tabs);
	for (unsigned n = previousSize; n < tabButtons.size(); ++n)
	{
		tabButtons[n] = new W_BUTTON(this);
		connect(tabButtons[n], SIGNAL(clicked()), setTabMapper, SLOT(map()));
		setTabMapper->setMapping(tabButtons[n], n);
	}

	doLayoutAll();
}

void TabSelectionWidget::prevTabPage()
{
	setTab(currentTab - tabsAtOnce);
}

void TabSelectionWidget::nextTabPage()
{
	setTab(currentTab + tabsAtOnce);
}

void TabSelectionWidget::doLayoutAll()
{
	TabSelectionStyle style;
	int scrollSpace = 0;
	tabsAtOnce = tabs();
	style.tabSize = QSize(width()/tabs(), height());
	style.scrollTabSize = QSize(0, 0);
	style.tabGap = 0;
	for (unsigned n = 0; n < styles.size(); ++n)
	{
		bool haveScroll_ = !styles[n].scrollTabSize.isEmpty();
		int scrollSpace_ = haveScroll_? styles[n].scrollTabSize.width() + styles[n].tabGap : 0;
		int tabsAtOnce_ = (width() + styles[n].tabGap - scrollSpace_*2)/(styles[n].tabSize.width() + styles[n].tabGap);
		if (haveScroll_ || tabs() <= tabsAtOnce_)
		{
			scrollSpace = scrollSpace_;
			tabsAtOnce = tabsAtOnce_;
			style = styles[n];
			break;
		}
	}
	prevTabPageButton->setGeometry(0, 0, style.scrollTabSize.width(), style.scrollTabSize.height());
	prevTabPageButton->setImages(style.prevScrollTabImage, style.prevScrollTabImageDown, style.prevScrollTabImageHighlight);
	prevTabPageButton->show(currentTab >= tabsAtOnce);
	nextTabPageButton->setGeometry(width() - style.scrollTabSize.width(), 0, style.scrollTabSize.width(), style.scrollTabSize.height());
	nextTabPageButton->setImages(style.nextScrollTabImage, style.nextScrollTabImageDown, style.nextScrollTabImageHighlight);
	nextTabPageButton->show(currentTab/tabsAtOnce < (tabs() - 1)/tabsAtOnce);
	for (unsigned n = 0; n < tabButtons.size(); ++n)
	{
		tabButtons[n]->setGeometry(scrollSpace + n%tabsAtOnce*(style.tabSize.width() + style.tabGap), 0, style.tabSize.width(), style.tabSize.height());
		tabButtons[n]->setImages(style.tabImage, style.tabImageDown, style.tabImageHighlight);
		tabButtons[n]->show(currentTab/tabsAtOnce == n/tabsAtOnce);
		tabButtons[n]->setState(n == currentTab? WBUT_LOCK : 0);
	}
	if (tabButtons.size() == 1)
	{
		tabButtons[0]->hide();  // Don't show single tabs.
	}
}

ListWidget::ListWidget(WIDGET *parent)
	: WIDGET(parent)
	, childSize(10, 10)
	, spacing(4, 4)
	, currentPage_(0)
	, order(RightThenDown)
{}

void ListWidget::setChildSize(int width, int height)
{
	childSize = QSize(width, height);
	doLayoutAll();
}

void ListWidget::setChildSpacing(int width, int height)
{
	spacing = QSize(width, height);
	doLayoutAll();
}

void ListWidget::setOrder(Order order_)
{
	order = order_;
	doLayoutAll();
}

void ListWidget::addWidgetToLayout(WIDGET *widget)
{
	int oldNumPages = pages();

	myChildren.push_back(widget);
	doLayout(myChildren.size() - 1);

	int numPages = pages();
	if (oldNumPages != numPages)
	{
		emit numberOfPagesChanged(numPages);
	}
}

void ListWidget::setCurrentPage(int page)
{
	unsigned previousPage = currentPage_;
	int pp = widgetsPerPage();
	currentPage_ = clip(page, 0, pages() - 1);
	if (previousPage == currentPage_)
	{
		return;  // Nothing to do.
	}
	for (unsigned n = pp*previousPage; n < pp*(previousPage + 1) && n < myChildren.size(); ++n)
	{
		myChildren[n]->hide();
	}
	for (unsigned n = pp*currentPage_; n < pp*(currentPage_ + 1) && n < myChildren.size(); ++n)
	{
		myChildren[n]->show();
	}
	emit currentPageChanged(currentPage_);
}

void ListWidget::doLayoutAll()
{
	for (unsigned n = 0; n < myChildren.size(); ++n)
	{
		doLayout(n);
	}
}

void ListWidget::doLayout(int num)
{
	unsigned page = num/widgetsPerPage();
	int withinPage = num%widgetsPerPage();
	int column = 0, row = 0;
	switch (order)
	{
	case RightThenDown:
		column = withinPage%widgetsPerRow();
		row = withinPage/widgetsPerRow();
		break;
	case DownThenRight:
		column = withinPage/widgetsPerColumn();
		row = withinPage%widgetsPerColumn();
		break;
	}
	myChildren[num]->setGeometry(column*widgetSkipX(), row*widgetSkipY(), childSize.width(), childSize.height());
	myChildren[num]->show(page == currentPage_);
}

ListTabWidget::ListTabWidget(WIDGET *parent)
	: WIDGET(parent)
	, tabs(new TabSelectionWidget(this))
	, widgets(new ListWidget(this))
{
	connect(tabs, SIGNAL(tabChanged(int)), widgets, SLOT(setCurrentPage(int)));
	connect(widgets, SIGNAL(currentPageChanged(int)), tabs, SLOT(setTab(int)));
	connect(widgets, SIGNAL(numberOfPagesChanged(int)), tabs, SLOT(setNumberOfTabs(int)));
	tabs->setNumberOfTabs(widgets->pages());
}

void ListTabWidget::geometryChanged()
{
	tabs->setGeometry(0, 0, width(), tabs->height());
	widgets->setGeometry(0, tabs->height(), width(), height() - tabs->height());
}

void ListTabWidget::addWidgetToLayout(WIDGET *widget)
{
	if (widget->parent() == this)
	{
		detach(widget);
		widgets->attach(widget);
	}
	widgets->addWidgetToLayout(widget);
}
