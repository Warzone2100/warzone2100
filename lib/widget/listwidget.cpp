/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
#include "lib/framework/math_ext.h"

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
		scrollTabSize = WzSize(prev.width(), prev.height());
	}
}

TabSelectionWidget::TabSelectionWidget(WIDGET *parent)
	: WIDGET(parent)
	, currentTab(0)
	, tabsAtOnce(1)
	, prevTabPageButton(new W_BUTTON(this))
	, nextTabPageButton(new W_BUTTON(this))
{
	prevTabPageButton->addOnClickHandler([](W_BUTTON& button) {
		TabSelectionWidget* pParent = static_cast<TabSelectionWidget*>(button.parent());
		assert(pParent != nullptr);
		pParent->prevTabPage();
	});
	nextTabPageButton->addOnClickHandler([](W_BUTTON& button) {
		TabSelectionWidget* pParent = static_cast<TabSelectionWidget*>(button.parent());
		assert(pParent != nullptr);
		pParent->nextTabPage();
	});

	prevTabPageButton->setTip(_("Tab Scroll left"));
	nextTabPageButton->setTip(_("Tab Scroll right"));

	setHeight(5);
	setNumberOfTabs(1);
}

void TabSelectionWidget::setHeight(int height)
{
	setGeometry(x(), y(), width(), height);
}

void TabSelectionWidget::addStyle(TabSelectionStyle const &tabStyle)
{
	styles.push_back(tabStyle);

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

	/* Call all onTabChanged event handlers */
	for (auto it = onTabChangedHandlers.begin(); it != onTabChangedHandlers.end(); it++)
	{
		auto onTabChanged = *it;
		if (onTabChanged)
		{
			onTabChanged(*this, currentTab);
		}
	}
}

void TabSelectionWidget::addOnTabChangedHandler(const W_TABSELECTION_ON_TAB_CHANGED_FUNC& onTabChangedFunc)
{
	onTabChangedHandlers.push_back(onTabChangedFunc);
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
		tabButtons[n]->addOnClickHandler([n](W_BUTTON& button) {
			TabSelectionWidget* pParent = static_cast<TabSelectionWidget*>(button.parent());
			assert(pParent != nullptr);
			pParent->setTab(n);
		});
	}

	doLayoutAll();
}

void TabSelectionWidget::prevTabPage()
{
	setTab(currentTab - currentTab % tabsAtOnce - 1);
}

void TabSelectionWidget::nextTabPage()
{
	setTab(currentTab + (tabsAtOnce - currentTab % tabsAtOnce));
}

void TabSelectionWidget::doLayoutAll()
{
	TabSelectionStyle tabStyle;
	int scrollSpace = 0;
	tabsAtOnce = tabs();
	tabStyle.tabSize = WzSize(width() / tabs(), height());
	tabStyle.scrollTabSize = WzSize(0, 0);
	tabStyle.tabGap = 0;
	for (unsigned n = 0; n < styles.size(); ++n)
	{
		bool haveScroll_ = !styles[n].scrollTabSize.isEmpty();
		int scrollSpace_ = haveScroll_ ? styles[n].scrollTabSize.width() + styles[n].tabGap : 0;
		int tabsAtOnce_ = (width() + styles[n].tabGap - scrollSpace_ * 2) / (styles[n].tabSize.width() + styles[n].tabGap);
		if (haveScroll_ || tabs() <= tabsAtOnce_)
		{
			scrollSpace = scrollSpace_;
			tabsAtOnce = tabsAtOnce_;
			tabStyle = styles[n];
			break;
		}
	}
	prevTabPageButton->setGeometry(0, 0, tabStyle.scrollTabSize.width(), tabStyle.scrollTabSize.height());
	prevTabPageButton->setImages(tabStyle.prevScrollTabImage, tabStyle.prevScrollTabImageDown, tabStyle.prevScrollTabImageHighlight);
	prevTabPageButton->show(currentTab >= tabsAtOnce);
	nextTabPageButton->setGeometry(width() - tabStyle.scrollTabSize.width(), 0, tabStyle.scrollTabSize.width(), tabStyle.scrollTabSize.height());
	nextTabPageButton->setImages(tabStyle.nextScrollTabImage, tabStyle.nextScrollTabImageDown, tabStyle.nextScrollTabImageHighlight);
	nextTabPageButton->show(currentTab / tabsAtOnce < (tabs() - 1) / tabsAtOnce);
	for (unsigned n = 0; n < tabButtons.size(); ++n)
	{
		tabButtons[n]->setGeometry(scrollSpace + n % tabsAtOnce * (tabStyle.tabSize.width() + tabStyle.tabGap), 0, tabStyle.tabSize.width(), tabStyle.tabSize.height());
		tabButtons[n]->setImages(tabStyle.tabImage, tabStyle.tabImageDown, tabStyle.tabImageHighlight);
		tabButtons[n]->show(currentTab / tabsAtOnce == n / tabsAtOnce);
		tabButtons[n]->setState(n == currentTab ? WBUT_LOCK : 0);
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

void ListWidget::widgetLost(WIDGET *widget)
{
	std::vector<WIDGET *>::iterator i = std::find(myChildren.begin(), myChildren.end(), widget);
	if (i != myChildren.end())
	{
		myChildren.erase(i);
		doLayoutAll();
	}
}

void ListWidget::setChildSize(int width, int height)
{
	childSize = WzSize(width, height);
	doLayoutAll();
}

void ListWidget::setChildSpacing(int width, int height)
{
	spacing = WzSize(width, height);
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
		/* Call all onNumberOfPagesChanged event handlers */
		for (auto it = onNumberOfPagesChangedHandlers.begin(); it != onNumberOfPagesChangedHandlers.end(); it++)
		{
			auto onNumberOfPagesChanged = *it;
			if (onNumberOfPagesChanged)
			{
				onNumberOfPagesChanged(*this, numPages);
			}
		}
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
	for (unsigned n = pp * previousPage; n < pp * (previousPage + 1) && n < myChildren.size(); ++n)
	{
		myChildren[n]->hide();
	}
	for (unsigned n = pp * currentPage_; n < pp * (currentPage_ + 1) && n < myChildren.size(); ++n)
	{
		myChildren[n]->show();
	}

	/* Call all onCurrentPageChanged event handlers */
	for (auto it = onCurrentPageChangedHandlers.begin(); it != onCurrentPageChangedHandlers.end(); it++)
	{
		auto onCurrentPageChanged = *it;
		if (onCurrentPageChanged)
		{
			onCurrentPageChanged(*this, currentPage_);
		}
	}
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
	unsigned page = num / widgetsPerPage();
	int withinPage = num % widgetsPerPage();
	int column = 0, row = 0;
	switch (order)
	{
	case RightThenDown:
		column = withinPage % widgetsPerRow();
		row = withinPage / widgetsPerRow();
		break;
	case DownThenRight:
		column = withinPage / widgetsPerColumn();
		row = withinPage % widgetsPerColumn();
		break;
	}
	myChildren[num]->setGeometry(column * widgetSkipX(), row * widgetSkipY(), childSize.width(), childSize.height());
	myChildren[num]->show(page == currentPage_);
}

void ListWidget::addOnCurrentPageChangedHandler(const W_LISTWIDGET_ON_CURRENTPAGECHANGED_FUNC& handlerFunc)
{
	onCurrentPageChangedHandlers.push_back(handlerFunc);
}

void ListWidget::addOnNumberOfPagesChangedHandler(const W_LISTWIDGET_ON_NUMBEROFPAGESCHANGED_FUNC& handlerFunc)
{
	onNumberOfPagesChangedHandlers.push_back(handlerFunc);
}

ListTabWidget::ListTabWidget(WIDGET *parent)
	: WIDGET(parent)
	, tabs(new TabSelectionWidget(this))
	, widgets(new ListWidget(this))
	, tabPos(Top)
{
	tabs->addOnTabChangedHandler([](TabSelectionWidget& tabsWidget, int currentTab) {
		ListTabWidget* pParent = static_cast<ListTabWidget*>(tabsWidget.parent());
		assert(pParent != nullptr);
		pParent->setCurrentPage(currentTab);
	});
	widgets->addOnCurrentPageChangedHandler([](ListWidget& listWidget, int currentPage) {
		ListTabWidget* pParent = static_cast<ListTabWidget*>(listWidget.parent());
		assert(pParent != nullptr);
		pParent->tabs->setTab(currentPage);
	});
	widgets->addOnNumberOfPagesChangedHandler([](ListWidget& listWidget, int numberOfPages) {
		ListTabWidget* pParent = static_cast<ListTabWidget*>(listWidget.parent());
		assert(pParent != nullptr);
		pParent->tabs->setNumberOfTabs(numberOfPages);
	});
	tabs->setNumberOfTabs(widgets->pages());
}

void ListTabWidget::geometryChanged()
{
	switch (tabPos)
	{
	case Top:
		tabs->setGeometry(0, 0, width(), tabs->height());
		widgets->setGeometry(0, tabs->height(), width(), height() - tabs->height());
		break;
	case Bottom:
		tabs->setGeometry(0, height() - tabs->height(), width(), tabs->height());
		widgets->setGeometry(0, 0, width(), height() - tabs->height());
		break;
	}
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

void ListTabWidget::setTabPosition(TabPosition pos)
{
	tabPos = pos;
	geometryChanged();
}
