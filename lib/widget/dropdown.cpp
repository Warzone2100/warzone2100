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
	DropdownOverlay(std::function<void ()> onClickedFunc): onClickedFunc(onClickedFunc)
	{
		setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			psWidget->setGeometry(0, 0, screenWidth - 1, screenHeight - 1);
		}));
	}

	void clicked(W_CONTEXT *, WIDGET_KEY = WKEY_PRIMARY) override
	{
		onClickedFunc();
	}

	std::function<void ()> onClickedFunc;
};

void DropdownItemWrapper::initialize(const std::shared_ptr<WIDGET> &newItem, DropdownOnSelectHandler newOnSelect)
{
	item = newItem;
	setGeometry(item->geometry());
	attach(item);
	onSelect = newOnSelect;
}

bool DropdownItemWrapper::processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	auto result = WIDGET::processClickRecursive(psContext, key, wasPressed);

	if (key != WKEY_NONE && wasPressed && item->isMouseOverWidget())
	{
		onSelect(std::static_pointer_cast<DropdownItemWrapper>(shared_from_this()));
	}

	return result;
}

void DropdownItemWrapper::geometryChanged()
{
	item->setGeometry(
		padding.left,
		padding.top,
		width() - padding.left - padding.right,
		height() - padding.top - padding.bottom
	);
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
		itemsList->setGeometry(screenPosX(), screenPosY() + height(), width(), itemsList->height());

		if (keyPressed(KEY_ESC))
		{
			inputLoseFocus();	// clear the input buffer.
			close();
		}
	}
}

void DropdownWidget::geometryChanged()
{
	for(auto& item : items)
	{
		item->setGeometry(0, 0, width(), height());
	}
}

void DropdownWidget::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	open();
}

void DropdownWidget::open()
{
	if (overlayScreen != nullptr) { return; }
	std::weak_ptr<DropdownWidget> pWeakThis(std::dynamic_pointer_cast<DropdownWidget>(shared_from_this()));
	widgScheduleTask([pWeakThis]() {
		if (auto dropdownWidget = pWeakThis.lock())
		{
			dropdownWidget->overlayScreen = W_SCREEN::make();
			widgRegisterOverlayScreenOnTopOfScreen(dropdownWidget->overlayScreen, dropdownWidget->screenPointer.lock());
			dropdownWidget->itemsList->setGeometry(dropdownWidget->screenPosX(), dropdownWidget->screenPosY() + dropdownWidget->height(), dropdownWidget->width(), dropdownWidget->itemsList->height());
			dropdownWidget->overlayScreen->psForm->attach(dropdownWidget->itemsList);
			dropdownWidget->overlayScreen->psForm->attach(std::make_shared<DropdownOverlay>([pWeakThis]() { if (auto dropdownWidget = pWeakThis.lock()) { dropdownWidget->close(); } }));
		}
	});
}

void DropdownWidget::close()
{
	std::weak_ptr<DropdownWidget> pWeakThis(std::dynamic_pointer_cast<DropdownWidget>(shared_from_this()));
	widgScheduleTask([pWeakThis]() {
		if (auto dropdownWidget = pWeakThis.lock())
		{
			widgRemoveOverlayScreen(dropdownWidget->overlayScreen);
			dropdownWidget->overlayScreen->psForm->detach(dropdownWidget->itemsList);
			dropdownWidget->overlayScreen = nullptr;
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
		selectedItem->getItem().displayRecursive(context);
	}
}

void DropdownWidget::addItem(const std::shared_ptr<WIDGET> &item)
{
	auto itemOnSelect = [this](std::shared_ptr<DropdownItemWrapper> selected) {
		if (!overlayScreen) {
			open();
			return;
		}

		select(selected);
		close();
	};

	auto wrapper = DropdownItemWrapper::make(item, itemOnSelect);
	wrapper->setPadding(itemPadding);
	wrapper->setGeometry(0, 0, width(), height());

	items.push_back(wrapper);
	itemsList->addItem(wrapper);
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
