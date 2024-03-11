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
 * Functions for dropdown.
 */

#include "dropdown.h"
#include "label.h"
#include "widgbase.h"
#include "form.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/framework/input.h"

class DropdownItemWrapper;

class DropdownOverlay: public WIDGET
{
public:
	DropdownOverlay(const std::shared_ptr<WIDGET>& parentDropdownWidget, std::function<void ()> onClickedFunc)
	: onClickedFunc(onClickedFunc)
	, parentDropdownWidget(parentDropdownWidget)
	{
		setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			psWidget->setGeometry(0, 0, screenWidth - 1, screenHeight - 1);
		}));
	}

	void clicked(W_CONTEXT *, WIDGET_KEY = WKEY_PRIMARY) override
	{
		onClickedFunc();
	}

	void display(int xOffset, int yOffset) override
	{
		if (parentDropdownWidget)
		{
			int x0 = std::max<int>(0, parentDropdownWidget->screenPosX() - 8);
			int y0 = parentDropdownWidget->screenPosY();
			int w = parentDropdownWidget->width();
			int h = parentDropdownWidget->height();
			pie_UniTransBoxFill(x0, y0, x0 + w + 16, y0 + h, WZCOL_MENU_SCORE_BUILT);
		}

		displayChildDropShadows(this, xOffset, yOffset);
	}

private:
	std::function<void ()> onClickedFunc;
	std::shared_ptr<WIDGET> parentDropdownWidget;
};

void DropdownItemWrapper::initialize(const std::shared_ptr<DropdownWidget>& newParent, const std::shared_ptr<WIDGET> &newItem, DropdownOnSelectHandler newOnSelect)
{
	item = newItem;
	attach(item);
	onSelect = newOnSelect;
	parent = newParent;
}

bool DropdownItemWrapper::processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	auto result = WIDGET::processClickRecursive(psContext, key, wasPressed);

	if (key != WKEY_NONE && this->hitTest(psContext->mx, psContext->my))
	{
		if (auto psStrongParent = parent.lock())
		{
			psStrongParent->setMouseClickOnItem(std::dynamic_pointer_cast<DropdownItemWrapper>(shared_from_this()), key, wasPressed);
		}

		if (wasPressed)
		{
			mouseDownOnWrapper = true;
		}
		else
		{
			// if mouse down + mouse up on the item
			if (mouseDownOnWrapper)
			{
				mouseDownOnWrapper = false;
				onSelect(std::static_pointer_cast<DropdownItemWrapper>(shared_from_this()));
			}
		}
	}

	return result;
}

void DropdownItemWrapper::clearMouseDownState()
{
	mouseDownOnWrapper = false;
}

void DropdownItemWrapper::geometryChanged()
{
	item->setGeometry(0, 0, width(), height());
}

void DropdownItemWrapper::display(int xOffset, int yOffset)
{
	if (selected)
	{
		auto x0 = xOffset + x();
		auto y0 = yOffset + y();
		pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), WZCOL_MENU_SCORE_BUILT);
	}
}

DropdownWidget::DropdownWidget()
{
	itemsList = ScrollableListWidget::make();
	itemsList->setBackgroundColor(WZCOL_MENU_BACKGROUND);
	setListHeight(100);
}

void DropdownWidget::run(W_CONTEXT *psContext)
{
	if (overlayScreen)
	{
		itemsList->setGeometry(screenPosX(), calculateDropdownListScreenPosY(), width(), itemsList->height());

		if (keyPressed(KEY_ESC))
		{
			inputLoseFocus();	// clear the input buffer.
			close();
		}
	}
}

void DropdownWidget::geometryChanged()
{
	itemsList->setGeometry(itemsList->x(), itemsList->y(), width(), itemsList->height());
	for(auto& item : items)
	{
		item->setGeometry(0, 0, width(), height());
	}
}

void DropdownWidget::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	open();
}

int DropdownWidget::calculateDropdownListScreenPosY() const
{
	int dropDownOverlayPosY = screenPosY() - overlayYPosOffset;
	if (dropDownOverlayPosY + itemsList->height() > screenHeight)
	{
		// Positioning the dropdown below would cause it to appear partially or fully offscreen
		// So, instead, position it above
		dropDownOverlayPosY = screenPosY() - itemsList->height();
		if (dropDownOverlayPosY < 0)
		{
			// Well, this would be off-screen too...
			// For now: Just position it at the top
			dropDownOverlayPosY = 0;
		}
	}
	return dropDownOverlayPosY;
}

void DropdownWidget::open()
{
	if (overlayScreen != nullptr) { return; }
	mouseDownItem.reset();
	std::weak_ptr<DropdownWidget> pWeakThis(std::dynamic_pointer_cast<DropdownWidget>(shared_from_this()));
	widgScheduleTask([pWeakThis]() {
		if (auto dropdownWidget = pWeakThis.lock())
		{
			dropdownWidget->overlayScreen = W_SCREEN::make();

			// calculate the ideal position so that the dropdown appears *overtop* of the currently-selected item (in-place)
			size_t selectedItemIndex = dropdownWidget->getSelectedIndex().value_or(0);
			size_t listViewTopIndex = 0;
			if (dropdownWidget->itemsList->idealHeight() > dropdownWidget->itemsList->height())
			{
				size_t maxOffset = std::min<size_t>(2, dropdownWidget->itemsList->numItems());
				if (selectedItemIndex > maxOffset)
				{
					listViewTopIndex = selectedItemIndex - maxOffset;
				}
				dropdownWidget->itemsList->scrollToItem(listViewTopIndex);
			}
			dropdownWidget->overlayYPosOffset = dropdownWidget->itemsList->getCurrentYPosOfItem(selectedItemIndex);

			auto newRootFrm = std::make_shared<DropdownOverlay>(
				dropdownWidget,
				[pWeakThis]() { if (auto dropdownWidget = pWeakThis.lock()) { dropdownWidget->close(); } }
			);
			dropdownWidget->overlayScreen->psForm->attach(newRootFrm);

			dropdownWidget->itemsList->setGeometry(dropdownWidget->screenPosX(), dropdownWidget->calculateDropdownListScreenPosY(), dropdownWidget->width(), dropdownWidget->itemsList->height());
			newRootFrm->attach(dropdownWidget->itemsList);

			widgRegisterOverlayScreenOnTopOfScreen(dropdownWidget->overlayScreen, dropdownWidget->screenPointer.lock());
		}
	});
}

void DropdownWidget::close()
{
	mouseDownItem.reset();
	std::weak_ptr<DropdownWidget> pWeakThis(std::dynamic_pointer_cast<DropdownWidget>(shared_from_this()));
	widgScheduleTask([pWeakThis]() {
		if (auto dropdownWidget = pWeakThis.lock())
		{
			if (dropdownWidget->overlayScreen != nullptr)
			{
				widgRemoveOverlayScreen(dropdownWidget->overlayScreen);
				if (dropdownWidget->overlayScreen->psForm)
				{
					dropdownWidget->overlayScreen->psForm->detach(dropdownWidget->itemsList);
				}
				dropdownWidget->overlayScreen = nullptr;
			}
		}
	});
}

void DropdownWidget::display(int xOffset, int yOffset)
{
	auto x0 = xOffset + x();
	auto y0 = yOffset + y();

	if (overlayScreen)
	{
		pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), WZCOL_MENU_SCORE_BUILT);
	}

	if (selectedItem)
	{
		WidgetGraphicsContext context;
		context = context.translatedBy(x0, y0);
		selectedItem->getItem()->displayRecursive(context);
	}
}

void DropdownWidget::addItem(const std::shared_ptr<WIDGET> &item)
{
	std::weak_ptr<DropdownWidget> pWeakThis(std::dynamic_pointer_cast<DropdownWidget>(shared_from_this()));
	size_t newItemIndex = items.size();
	auto itemOnSelect = [newItemIndex, pWeakThis](std::shared_ptr<DropdownItemWrapper> selected) {
		auto psStrongDropdown = pWeakThis.lock();
		ASSERT_OR_RETURN(, psStrongDropdown != nullptr, "DropdownWidget no longer exists?");
		if (!psStrongDropdown->overlayScreen) {
			psStrongDropdown->open();
			return;
		}

		if (psStrongDropdown->select(selected, newItemIndex))
		{
			psStrongDropdown->close();
		}
	};

	auto wrapper = DropdownItemWrapper::make(std::dynamic_pointer_cast<DropdownWidget>(shared_from_this()), item, itemOnSelect);
	wrapper->setGeometry(0, 0, width(), height());

	items.push_back(wrapper);
	itemsList->addItem(wrapper);
}

void DropdownWidget::clear()
{
	items.clear();
	itemsList->clear();
	selectedItem.reset();
	mouseOverItem.reset();
	mouseDownItem.reset();
}

bool DropdownWidget::processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	if (!overlayScreen && selectedItem)
	{
		W_CONTEXT shiftedContext(psContext);
		auto deltaX = x() - selectedItem->x();
		auto deltaY = y() - selectedItem->y();
		shiftedContext.mx = psContext->mx - deltaX;
		shiftedContext.my = psContext->my - deltaY;
		shiftedContext.xOffset = psContext->xOffset + deltaX;
		shiftedContext.yOffset = psContext->yOffset + deltaY;
		selectedItem->processClickRecursive(&shiftedContext, key, wasPressed);
	}

	return WIDGET::processClickRecursive(psContext, key, wasPressed);
}

void DropdownWidget::setMouseClickOnItem(std::shared_ptr<DropdownItemWrapper> item, WIDGET_KEY key, bool wasPressed)
{
	if (item == mouseDownItem) { return; }

	if (mouseDownItem)
	{
		mouseDownItem->clearMouseDownState();
	}

	if (wasPressed)
	{
		mouseDownItem = item;
	}
	else
	{
		mouseDownItem.reset();
	}
}
