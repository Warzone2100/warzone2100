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
/** @file
 *  Implementation of a list widget. Useful for displaying lists of buttons and stuff.
 */

#include "listwidget.h"
#include "button.h"
#include "label.h"
#include "lib/framework/math_ext.h"

#include <algorithm>

TabSelectionStyle::TabSelectionStyle(AtlasImage tab, AtlasImage tabDown, AtlasImage tabHighlight, AtlasImage prev, AtlasImage prevDown, AtlasImage prevHighlight, AtlasImage next, AtlasImage nextDown, AtlasImage nextHighlight, int gap, TabAlignment alignment)
	: tabSize(tab.width(), tab.height())
	, scrollTabSize(0, 0)
	, tabImage(tab), tabImageDown(tabDown), tabImageHighlight(tabHighlight)
	, prevScrollTabImage(prev), prevScrollTabImageDown(prevDown), prevScrollTabImageHighlight(prevHighlight)
	, nextScrollTabImage(next), nextScrollTabImageDown(nextDown), nextScrollTabImageHighlight(nextHighlight)
	, tabGap(gap)
	, tabAlignment(alignment)
{
	if (!prev.isNull())
	{
		scrollTabSize = WzSize(prev.width(), prev.height());
	}
}

void TabSelectionWidget::initialize()
{
	attach(prevTabPageButton = std::make_shared<W_BUTTON>());
	attach(nextTabPageButton = std::make_shared<W_BUTTON>());

	prevTabPageButton->addOnClickHandler([](W_BUTTON& button) {
		auto pParent = std::static_pointer_cast<TabSelectionWidget>(button.parent());
		assert(pParent != nullptr);
		pParent->prevTabPage();
	});
	nextTabPageButton->addOnClickHandler([](W_BUTTON& button) {
		auto pParent = std::static_pointer_cast<TabSelectionWidget>(button.parent());
		assert(pParent != nullptr);
		pParent->nextTabPage();
	});

	prevTabPageButton->setTip(_("Tab Scroll left"));
	nextTabPageButton->setTip(_("Tab Scroll right"));

	setHeight(5);
	setNumberOfTabs(1);
}

void TabSelectionWidget::geometryChanged()
{
	doLayoutAll();
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

void TabSelectionWidget::setTab(size_t tab)
{
	size_t previousTab = currentTab;
	currentTab = clip<size_t>(tab, 0, tabButtons.size() - 1);
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

void TabSelectionWidget::setNumberOfTabs(size_t tabs)
{
	tabs = std::max<size_t>(tabs, 1);
	size_t previousSize = tabButtons.size();
	for (size_t n = tabs; n < tabButtons.size(); ++n)
	{
		detach(tabButtons[n]);
	}
	tabButtons.resize(tabs);
	for (size_t n = previousSize; n < tabButtons.size(); ++n)
	{
		attach(tabButtons[n] = std::make_shared<W_BUTTON>());
		tabButtons[n]->addOnClickHandler([n](W_BUTTON& button) {
			auto pParent = std::static_pointer_cast<TabSelectionWidget>(button.parent());
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
	tabsAtOnce = std::max<size_t>(tabs(), 1);
	if (tabs() == 0 || width() == 0 || height() == 0)
	{
		return;
	}
	tabStyle.tabSize = WzSize((tabs() > 0) ? (width() / static_cast<int>(tabs())) : 0, height());
	tabStyle.scrollTabSize = WzSize(0, 0);
	tabStyle.tabGap = 0;
	tabStyle.tabAlignment = TabAlignment::LeftAligned;
	for (size_t n = 0; n < styles.size(); ++n)
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
	nextTabPageButton->show((tabsAtOnce > 0) ? currentTab / tabsAtOnce < (tabs() - 1) / tabsAtOnce : false);
	size_t numPages = std::max<size_t>((tabsAtOnce > 0) ? (tabs() + (tabsAtOnce - 1)) / tabsAtOnce : 1, 1);
	size_t numTabsOnLastPage = (tabs() > 0) ? ((tabs()-1) % tabsAtOnce) + 1 : 0;
	for (size_t n = 0; n < tabButtons.size(); ++n)
	{
		int x0 = 0;
		size_t tabPage = n / tabsAtOnce;
		int tabPos = 0;
		switch (tabStyle.tabAlignment)
		{
			case TabAlignment::LeftAligned:
				x0 = scrollSpace + static_cast<int>(n % tabsAtOnce) * (tabStyle.tabSize.width() + tabStyle.tabGap);
				break;
			case TabAlignment::RightAligned:
				if (tabPage < (numPages - 1))
				{
					tabPos = static_cast<int>(tabsAtOnce) - static_cast<int>(n % tabsAtOnce);
				}
				else
				{
					// on last page
					tabPos = static_cast<int>(numTabsOnLastPage) - static_cast<int>(n % tabsAtOnce);
				}
				x0 = width() - scrollSpace - (tabPos * (tabStyle.tabSize.width() + tabStyle.tabGap));
				break;
		}
		tabButtons[n]->setGeometry(x0, 0, tabStyle.tabSize.width(), tabStyle.tabSize.height());
		tabButtons[n]->setImages(tabStyle.tabImage, tabStyle.tabImageDown, tabStyle.tabImageHighlight);
		tabButtons[n]->show(currentTab / tabsAtOnce == n / tabsAtOnce);
		tabButtons[n]->setState(n == currentTab ? WBUT_LOCK : 0);
	}
	if (tabButtons.size() == 1)
	{
		tabButtons[0]->hide();  // Don't show single tabs.
	}
}

ListWidget::ListWidget()
	: WIDGET()
	, childSize(10, 10)
	, spacing(4, 4)
	, currentPage_(0)
	, order(RightThenDown)
{}

void ListWidget::widgetLost(WIDGET *widget)
{
	for (auto childIt = myChildren.begin(); childIt != myChildren.end(); childIt++)
	{
		if (childIt->get() == widget) {
			myChildren.erase(childIt);
			doLayoutAll();
			updateNumberOfPages();
			break;
		}
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

void ListWidget::addWidgetToLayout(const std::shared_ptr<WIDGET> &widget)
{
	myChildren.push_back(widget);
	doLayout(myChildren.size() - 1);
	updateNumberOfPages();
}

void ListWidget::setCurrentPage(size_t page)
{
	size_t previousPage = currentPage_;
	size_t pp = widgetsPerPage();
	currentPage_ = clip<size_t>(page, 0, numberOfPages - 1);
	if (previousPage == currentPage_)
	{
		return;  // Nothing to do.
	}
	for (size_t n = pp * previousPage; n < pp * (previousPage + 1) && n < myChildren.size(); ++n)
	{
		myChildren[n]->hide();
	}
	for (size_t n = pp * currentPage_; n < pp * (currentPage_ + 1) && n < myChildren.size(); ++n)
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
	for (size_t n = 0; n < myChildren.size(); ++n)
	{
		doLayout(n);
	}
}

void ListWidget::doLayout(size_t num)
{
	size_t page = num / widgetsPerPage();
	size_t withinPage = num % widgetsPerPage();
	size_t column = 0, row = 0;
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
	myChildren[num]->setGeometry(static_cast<int>(column) * widgetSkipX(), static_cast<int>(row) * widgetSkipY(), childSize.width(), childSize.height());
	myChildren[num]->show(page == currentPage_);
}

size_t ListWidget::firstWidgetShownIndex() const
{
	return currentPage_ * widgetsPerPage();
}

size_t ListWidget::lastWidgetShownIndex() const
{
	return std::min(firstWidgetShownIndex() + widgetsPerPage() - 1, myChildren.size() - 1);
}

std::shared_ptr<WIDGET> ListWidget::getWidgetAtIndex(size_t index) const
{
	ASSERT_OR_RETURN(nullptr, index < myChildren.size(), "Invalid widget index: %zu", index);
	return myChildren[index];
}

void ListWidget::addOnCurrentPageChangedHandler(const W_LISTWIDGET_ON_CURRENTPAGECHANGED_FUNC& handlerFunc)
{
	onCurrentPageChangedHandlers.push_back(handlerFunc);
}

void ListWidget::addOnNumberOfPagesChangedHandler(const W_LISTWIDGET_ON_NUMBEROFPAGESCHANGED_FUNC& handlerFunc)
{
	onNumberOfPagesChangedHandlers.push_back(handlerFunc);
}

void ListWidget::updateNumberOfPages()
{
	auto newNumberOfPages = myChildren.empty() ? 1 : ((myChildren.size() - 1) / widgetsPerPage()) + 1;

	if (newNumberOfPages != numberOfPages)
	{
		numberOfPages = newNumberOfPages;
		setCurrentPage(currentPage_);
		/* Call all onNumberOfPagesChanged event handlers */
		for (auto it = onNumberOfPagesChangedHandlers.begin(); it != onNumberOfPagesChangedHandlers.end(); it++)
		{
			auto onNumberOfPagesChanged = *it;
			if (onNumberOfPagesChanged)
			{
				onNumberOfPagesChanged(*this, numberOfPages);
			}
		}
	}
}

void ListWidget::geometryChanged()
{
	doLayoutAll();
	updateNumberOfPages();
}

void ListTabWidget::initialize()
{
	attach(label = std::make_shared<W_LABEL>());
	label->setFont(font_small, WZCOL_TEXT_MEDIUM);
	label->setTransparentToMouse(true);
	attach(tabs = TabSelectionWidget::make());
	attach(widgets = std::make_shared<ListWidget>());
	tabs->addOnTabChangedHandler([](TabSelectionWidget& tabsWidget, size_t currentTab) {
		auto pParent = tabsWidget.parent();
		ASSERT_OR_RETURN(, pParent != nullptr, "pParent is nullptr");
		std::static_pointer_cast<ListTabWidget>(pParent)->setCurrentPage(currentTab);
	});
	widgets->addOnCurrentPageChangedHandler([](ListWidget& listWidget, size_t currentPage) {
		auto pParent = listWidget.parent();
		ASSERT_OR_RETURN(, pParent != nullptr, "pParent is nullptr");
		std::static_pointer_cast<ListTabWidget>(pParent)->tabs->setTab(currentPage);
	});
	widgets->addOnNumberOfPagesChangedHandler([](ListWidget& listWidget, size_t numberOfPages) {
		auto pParent = listWidget.parent();
		ASSERT_OR_RETURN(, pParent != nullptr, "pParent is nullptr");
		std::static_pointer_cast<ListTabWidget>(pParent)->tabs->setNumberOfTabs(numberOfPages);
	});
	tabs->setNumberOfTabs(widgets->pages());
}

#define LABEL_X_BETWEEN_PADDING 5
#define LABEL_PADDING_Y 0

void ListTabWidget::geometryChanged()
{
	int32_t label_width = static_cast<size_t>(std::max<int>(label->idealWidth(), 0));
	int label_x0 = 0;
	int label_y0 = LABEL_PADDING_Y;
	int tabs_x0 = label_x0 + label_width + LABEL_X_BETWEEN_PADDING;
	int tabs_width = width() - tabs_x0;
	switch (tabPos)
	{
	case Top:
		{
			label->setGeometry(label_x0, label_y0, label_width, label->height());
			tabs->setGeometry(tabs_x0, 0, tabs_width, tabs->height());
			int widgets_y0 = std::max(label->idealHeight(), tabs->height());
			widgets->setGeometry(0, widgets_y0, width(), height() - widgets_y0);
		}
		break;
	case Bottom:
		{
			int label_tabs_height = std::max(label->idealHeight() + (LABEL_PADDING_Y * 2), tabs->height());
			label_y0 = height() - label_tabs_height + LABEL_PADDING_Y;
			label->setGeometry(label_x0, label_y0, label_width, label->height());
			tabs->setGeometry(tabs_x0, height() - label_tabs_height, tabs_width, tabs->height());
			widgets->setGeometry(0, 0, width(), height() - tabs->height());
		}
		break;
	}
}

int32_t ListTabWidget::heightOfTabsLabel() const
{
	int widgets_y0 = std::max(LABEL_PADDING_Y + label->idealHeight(), tabs->height());
	return widgets_y0;
}

void ListTabWidget::setTitle(const WzString& string)
{
	label->setString(string);
	label->setGeometry(0, 0, label->idealWidth(), label->idealHeight());
	geometryChanged();
}

void ListTabWidget::addWidgetToLayout(const std::shared_ptr<WIDGET> &widget)
{
	if (widget->parent().get() == this)
	{
		detach(widget);
	}

	widgets->attach(widget);
	widgets->addWidgetToLayout(widget);
}

void ListTabWidget::setTabPosition(TabPosition pos)
{
	tabPos = pos;
	geometryChanged();
}
