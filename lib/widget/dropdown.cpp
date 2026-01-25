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
	DropdownOverlay(std::function<void ()> onClickedFunc)
	: onClickedFunc(onClickedFunc)
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
		displayChildDropShadows(this, xOffset, yOffset);
	}

private:
	std::function<void ()> onClickedFunc;
};

void DropdownItemWrapper::initialize(const std::shared_ptr<DropdownWidget>& newParent, const std::shared_ptr<WIDGET> &newItem, DropdownOnSelectHandler newOnSelect)
{
	item = newItem;
	attach(item);
	onSelect = newOnSelect;
	parent = newParent;
}

std::shared_ptr<WIDGET> DropdownItemWrapper::findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	W_CONTEXT inputContext(psContext);
	auto result = WIDGET::findMouseTargetRecursive(psContext, key, wasPressed);

	if (key != WKEY_NONE && this->hitTest(inputContext.mx, inputContext.my))
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

				// Instead of calling the onSelect handler immediately, schedule a task.
				//
				// (The onSelect handler may call close(), and we want that to be delayed by an event processing cycle
				// so the mouse event gets properly attributed to the overlay screen by other code - such as the game
				// event processing loop, which currently usese isMouseOverScreenOverlayChild)
				auto weakSelf = std::weak_ptr<DropdownItemWrapper>(std::static_pointer_cast<DropdownItemWrapper>(shared_from_this()));
				widgScheduleTask([weakSelf]() {
					auto strongSelf = weakSelf.lock();
					ASSERT_OR_RETURN(, strongSelf != nullptr, "DropdownItemWrapper disappeared?");
					strongSelf->onSelect(strongSelf);
				});
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
		itemsList->setGeometry(calculateDropdownListScreenPosX(), calculateDropdownListScreenPosY(), itemsList->width(), itemsList->height());

		if (keyPressed(KEY_ESC))
		{
			inputLoseFocus();	// clear the input buffer.
			close();
		}
	}
	else
	{
		callRunOnItems();
	}
}

void DropdownWidget::callRunOnItems()
{
	// Some child widgets might use run() to clear cached textures (etc), so call run regularly to allow them
	for (const auto& item : items)
	{
		auto ctx = W_CONTEXT::ZeroContext();
		item->item->runRecursive(&ctx);
		item->item->manuallyCallRun(&ctx);
	}
}

void DropdownWidget::geometryChanged()
{
	int w = width();
	int h = height();
	if (w == 0 || h == 0)
	{
		return;
	}
	int itemWidth = getItemDisplayWidth();
	itemsList->setGeometry(itemsList->x(), itemsList->y(), calculateDropdownListDisplayWidth(), itemsList->height());
	switch (menuStyle)
	{
		case DropdownMenuStyle::InPlace:
			for(auto& item : items)
			{
				item->setGeometry(0, 0, itemWidth, h);
			}
			break;
		case DropdownMenuStyle::Separate:
			break;
	}
}

void DropdownWidget::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	open();
}

int DropdownWidget::calculateDropdownListScreenPosX() const
{
	return screenPosX();
}

int DropdownWidget::calculateDropdownListDisplayWidth() const
{
	switch (menuStyle)
	{
		case DropdownMenuStyle::InPlace:
			return getItemDisplayWidth();
		case DropdownMenuStyle::Separate:
			return itemsList->idealWidth();
	}
	return 0; // silence warning
}

void DropdownWidget::setListBackgroundColor(const PIELIGHT& color)
{
	itemsList->setBackgroundColor(color);
}

const Padding& DropdownWidget::getDropdownMenuOuterPadding() const
{
	return itemsList->getPadding();
}

int DropdownWidget::calculateDropdownListScreenPosY() const
{
	int yPosOffset = 0;
	switch (menuStyle)
	{
		case DropdownMenuStyle::InPlace:
			yPosOffset = -overlayYPosOffset;
			break;
		case DropdownMenuStyle::Separate:
			yPosOffset = height();
			break;
	}

	int dropDownOverlayPosY = screenPosY() + yPosOffset;
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
			if (dropdownWidget->onOpen)
			{
				dropdownWidget->onOpen(*dropdownWidget);
			}

			dropdownWidget->overlayScreen = W_SCREEN::make();

			switch (dropdownWidget->menuStyle)
			{
			case DropdownMenuStyle::InPlace:
				{
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
				}
				break;
			case DropdownMenuStyle::Separate:
				break;
			}

			auto newRootFrm = std::make_shared<DropdownOverlay>(
				[pWeakThis]() { if (auto dropdownWidget = pWeakThis.lock()) { dropdownWidget->close(); } }
			);
			dropdownWidget->overlayScreen->psForm->attach(newRootFrm);

			dropdownWidget->itemsList->setGeometry(dropdownWidget->calculateDropdownListScreenPosX(), dropdownWidget->calculateDropdownListScreenPosY(), dropdownWidget->calculateDropdownListDisplayWidth(), dropdownWidget->itemsList->height());
			newRootFrm->attach(dropdownWidget->itemsList);

			widgRegisterOverlayScreenOnTopOfScreen(dropdownWidget->overlayScreen, dropdownWidget->screenPointer.lock());
		}
	});
}

bool DropdownWidget::isOpen() const
{
	return overlayScreen != nullptr;
}

void DropdownWidget::close()
{
	ASSERT_OR_RETURN(, isOpen(), "Dropdown isn't open");
	mouseDownItem.reset();
	std::weak_ptr<DropdownWidget> pWeakThis(std::dynamic_pointer_cast<DropdownWidget>(shared_from_this()));
	widgScheduleTask([pWeakThis]() {
		if (auto dropdownWidget = pWeakThis.lock())
		{
			if (dropdownWidget->overlayScreen != nullptr)
			{
				if (dropdownWidget->onClose)
				{
					dropdownWidget->onClose(*dropdownWidget);
				}

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

int32_t DropdownWidget::getItemDisplayWidth() const
{
	return std::max(width() - getCaretImageUsedWidth(), 0);
}

int32_t DropdownWidget::getCaretImageUsedWidth() const
{
	return dropdownCaretImageSize.width() + dropdownCaretImagePadding.left + dropdownCaretImagePadding.right;
}

void DropdownWidget::display(int xOffset, int yOffset)
{
	auto x0 = xOffset + x();
	auto y0 = yOffset + y();

	if (isOpen())
	{
		drawOpenedHighlight(xOffset, yOffset);
	}

	auto itemWidth = getItemDisplayWidth();
	drawDropdownCaretImage(xOffset, yOffset, WZCOL_TEXT_MEDIUM);

	if (selectedItem)
	{
		WzRect displayArea(x0, y0, itemWidth, height());
		drawSelectedItem(selectedItem->getItem(), displayArea);
	}
}

void DropdownWidget::drawDropdownCaretImage(int xOffset, int yOffset, PIELIGHT color)
{
	if (!dropdownCaretImage.has_value())
	{
		return;
	}
	auto x0 = xOffset + x();
	auto y0 = yOffset + y();
	int caretX0 = x0 + getItemDisplayWidth() + dropdownCaretImagePadding.left;
	int caretY0 = y0 + ((height() - dropdownCaretImageSize.height()) / 2);
	iV_DrawImageFileAnisotropicTint(dropdownCaretImage.value().images, dropdownCaretImage.value().id, caretX0, caretY0, Vector2f{dropdownCaretImageSize.width(), dropdownCaretImageSize.height()}, color);
}

void DropdownWidget::drawOpenedHighlight(int xOffset, int yOffset)
{
	int x0 = std::max<int>(0, (xOffset + x()) - 8);
	int y0 = yOffset + y();
	int w = width();
	int h = height();
	pie_UniTransBoxFill(x0, y0, x0 + w + 16, y0 + h, WZCOL_MENU_SCORE_BUILT);
}

void DropdownWidget::drawSelectedItem(const std::shared_ptr<WIDGET>& item, const WzRect& displayArea)
{
	WidgetGraphicsContext context;
	context = context.translatedBy(displayArea.x(), displayArea.y());
	item->displayRecursive(context);
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
	switch (menuStyle)
	{
		case DropdownMenuStyle::InPlace:
			wrapper->setGeometry(0, 0, std::max(getItemDisplayWidth(), 0), height());
			break;
		case DropdownMenuStyle::Separate:
			wrapper->setGeometry(0, 0, std::max(item->width(), 0), item->height());
			break;
	}

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

std::shared_ptr<WIDGET> DropdownWidget::findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	if (!overlayScreen && selectedItem && !isDisabled && key == WKEY_NONE) // only forward highlighting events
	{
		W_CONTEXT shiftedContext(psContext);
		auto deltaX = x() - selectedItem->x();
		auto deltaY = y() - selectedItem->y();
		shiftedContext.mx = psContext->mx - deltaX;
		shiftedContext.my = psContext->my - deltaY;
		shiftedContext.xOffset = psContext->xOffset + deltaX;
		shiftedContext.yOffset = psContext->yOffset + deltaY;
		auto highlightedSelectedItemChildWidget = selectedItem->findMouseTargetRecursive(&shiftedContext, key, wasPressed);
		if (highlightedSelectedItemChildWidget)
		{
			*psContext = shiftedContext; // bubble-up the matching hit's context
			return highlightedSelectedItemChildWidget;
		}
	}

	return WIDGET::findMouseTargetRecursive(psContext, key, wasPressed);
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

void DropdownWidget::setDropdownCaretImage(optional<AtlasImage> image, const WzSize& displaySize, const Padding& padding)
{
	dropdownCaretImage = image;
	dropdownCaretImageSize = WzSize(0, 0);
	dropdownCaretImagePadding = Padding{};

	if (dropdownCaretImage.has_value())
	{
		if (!dropdownCaretImage.value().images)
		{
			dropdownCaretImage.reset();
			return;
		}
		if (dropdownCaretImage.value().id >= dropdownCaretImage.value().images->numImages())
		{
			dropdownCaretImage.reset();
			return;
		}
		dropdownCaretImageSize = displaySize;
		dropdownCaretImagePadding = padding;
	}
}

void DropdownWidget::setDisabled(bool _isDisabled)
{
	isDisabled = _isDisabled;
}

bool DropdownWidget::getIsDisabled() const
{
	return isDisabled;
}

void DropdownWidget::setStyle(DropdownMenuStyle _menuStyle)
{
	menuStyle = _menuStyle;
	switch (menuStyle)
	{
		case DropdownMenuStyle::InPlace:
			itemsList->setPadding(Padding{});
			break;
		case DropdownMenuStyle::Separate:
			itemsList->setPadding(Padding{5,0,5,0});
			break;
	}
}

int32_t DropdownWidget::idealWidth()
{
	int32_t result = itemsList->idealWidth();
	result += getCaretImageUsedWidth();
	return result;
}

int32_t DropdownWidget::idealHeight()
{
	auto max = 0;
	for (auto const &item: items)
	{
		max = std::max(max, item->idealHeight());
	}

	return max;
}

bool DropdownWidget::select(const std::shared_ptr<DropdownItemWrapper> &selected, size_t selectedIndex)
{
	if (selectedItem == selected)
	{
		return true;
	}

	if (canChange)
	{
		if (!canChange(*this, selectedIndex, (selected) ? selected->getItem() : nullptr))
		{
			// abort change
			return false;
		}
	}

	if (selectedItem)
	{
		selectedItem->setSelected(false);
	}
	selectedItem = selected;
	if (menuStyle == DropdownMenuStyle::InPlace)
	{
		selectedItem->setSelected(true);
	}

	onSelectedItemChanged();
	if (onChange)
	{
		onChange(*this);
	}

	return true;
}

void DropdownWidget::onSelectedItemChanged()
{
	// currently, no-op - intended for subclasses to override
}
