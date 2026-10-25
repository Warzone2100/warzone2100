// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version, as published
	by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
	the top-level directory of the source distribution. This notice must
	be kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/


#include "hudpagecontrol.h"

#include "hudskin.h"

#include "lib/framework/frame.h"
#include "lib/framework/gamepad_input.h"
#include "lib/framework/input.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/button.h"
#include "lib/widget/listwidget.h"
#include "lib/widget/popover.h"

#include <algorithm>

namespace
{

// Inside the frame's 1 px border, page cells are 1 px apart (where a separator goes), all of the same width
constexpr int FRAME_BORDER = 1;
constexpr int SEPARATOR = 1;
constexpr int MIN_CELL_WIDTH = 10;
constexpr int CELL_PADDING = 4;
constexpr int ARROW_WIDTH = 7;
constexpr int PICKER_BUTTON_WIDTH = 28;
constexpr int PICKER_BUTTON_HEIGHT = 24;
constexpr size_t PICKER_COLUMNS = 4;
constexpr int PICKER_GAP = 4;
constexpr int PICKER_PADDING = 6;

bool touchForTests = false;

/// With touch or a gamepad: large page buttons to pick from
bool wantsPicker()
{
	return touchForTests || inputLastPointerInputWasTouch() || isGamepadActiveInput();
}

class PickerFrame : public WIDGET
{
protected:
	void display(int xOffset, int yOffset) override
	{
		currentHudSkin().drawPopoverFrame(WzRect(xOffset + x(), yOffset + y(), width(), height()));
	}
};

}

class HudPageControl::Segment : public W_BUTTON
{
public:
	void setVisualHeight(int height)
	{
		drawnHeight = height;
	}

	/// `cellLeft` and `cellWidth`: the part of it that it draws in, past the frame's border or the separator on its left
	void set(TabSelectionButtonRole newRole, size_t newPage, bool newSelected, bool newSeparator, bool newLarge, bool disabled,
	         int newCellLeft = 0, int newCellWidth = -1, size_t ofPages = 0)
	{
		summary = ofPages > 0;
		cellLeft = newCellLeft;
		cellWidth = newCellWidth;
		role = newRole;
		page = newPage;
		selected = newSelected;
		separator = newSeparator;
		large = newLarge;
		WzString text = WzString::number(page + 1);
		if (summary)
		{
			text = WzString::number(page + 1) + WzString::fromUtf8(" / ") + WzString::number(ofPages);
		}
		if (role != TabSelectionButtonRole::Tab)
		{
			text = WzString::fromUtf8(role == TabSelectionButtonRole::PreviousPage ? "‹" : "›");
		}
		if (text != label.getText())
		{
			label.setText(text, font_small);
		}
		const unsigned disabledState = disabled ? WBUT_DISABLE : 0;
		if ((getState() & WBUT_DISABLE) != disabledState)
		{
			setState(disabledState);
		}
	}

	TabSelectionButtonRole role = TabSelectionButtonRole::Tab;
	size_t page = 0;
	bool summary = false;

protected:
	void display(int xOffset, int yOffset) override
	{
		const int visualHeight = large ? height() : std::min(height(), drawnHeight);
		HudPageSegmentLook look;
		look.rect = WzRect(xOffset + x() + cellLeft, yOffset + y() + (height() - visualHeight) / 2, cellWidth >= 0 ? cellWidth : width(), visualHeight);
		look.role = role;
		look.selected = selected;
		look.hovered = (getState() & WBUT_HIGHLIGHT) != 0;
		look.disabled = (getState() & WBUT_DISABLE) != 0;
		look.separator = separator;
		look.large = large;
		const auto ink = currentHudSkin().drawPageSegment(look);
		if (!ink.has_value())
		{
			return;
		}
		// Numbers share the digits' baseline (WzText::renderCenteredOnInk)
		if (role == TabSelectionButtonRole::Tab && digits.getText().isEmpty())
		{
			digits.setText(WzString::fromUtf8("0123456789"), font_small);
		}
		label.renderCenteredOnInk(look.rect, ink.value(), role == TabSelectionButtonRole::Tab ? &digits : nullptr);
	}

private:
	bool selected = false;
	bool separator = false;
	bool large = false;
	int cellLeft = 0;
	int cellWidth = -1;
	int drawnHeight = 13;
	WzText label;
	WzText digits;
};

std::shared_ptr<HudPageControl> HudPageControl::make(const std::shared_ptr<ListTabWidget> &list)
{
	class make_shared_enabler: public HudPageControl {};
	auto control = std::make_shared<make_shared_enabler>();
	control->list = list;
	std::weak_ptr<HudPageControl> weakControl = control;
	list->listWidget()->addOnCurrentPageChangedHandler([weakControl](ListWidget &, size_t) {
		if (auto strongControl = weakControl.lock())
		{
			strongControl->layoutSegments();
		}
	});
	list->listWidget()->addOnNumberOfPagesChangedHandler([weakControl](ListWidget &, size_t) {
		if (auto strongControl = weakControl.lock())
		{
			strongControl->layoutSegments();
			if (strongControl->onPagesChanged)
			{
				strongControl->onPagesChanged(*strongControl);
			}
		}
	});
	control->layoutSegments();
	return control;
}

HudPageControl::~HudPageControl()
{
	if (picker != nullptr)
	{
		picker->close();
	}
}

size_t HudPageControl::pages() const
{
	auto strongList = list.lock();
	return strongList != nullptr ? strongList->pages() : 0;
}

void HudPageControl::setVisualHeight(int height)
{
	drawnHeight = height;
	for (const auto &segment : segments)
	{
		segment->setVisualHeight(height);
	}
}

void HudPageControl::setSingleSegment(bool single)
{
	if (single != singleSegment)
	{
		singleSegment = single;
		layoutSegments();
	}
}

void HudPageControl::geometryChanged()
{
	layoutSegments();
}

void HudPageControl::display(int xOffset, int yOffset)
{
	if (frameWidth > 0)
	{
		const int visualHeight = std::min(height(), drawnHeight);
		currentHudSkin().drawPageControlFrame(WzRect(xOffset + x() + width() - frameWidth, yOffset + y() + (height() - visualHeight) / 2, frameWidth, visualHeight));
	}
}

void HudPageControl::layoutSegments()
{
	auto strongList = list.lock();
	const size_t pageCount = strongList != nullptr ? strongList->pages() : 0;
	const size_t current = pageCount > 0 ? std::min(strongList->currentPage(), pageCount - 1) : 0;
	struct Slot
	{
		TabSelectionButtonRole role;
		size_t page;
		int width;
	};
	std::vector<Slot> slots;
	const int inside = width() - FRAME_BORDER * 2;
	auto cellsFor = [](int availableWidth, int count) { return (availableWidth - SEPARATOR * (count - 1)) / count; };
	if (pageCount > 1 && inside > 0 && singleSegment)
	{
		slots.push_back({TabSelectionButtonRole::Tab, current, inside});
	}
	else if (pageCount > 1 && inside > 0)
	{
		// All the pages when they fit, else the ones around the current page between arrows
		const int minimum = std::max(MIN_CELL_WIDTH, static_cast<int>(iV_GetTextWidth(WzString::number(pageCount), font_small)) + CELL_PADDING);
		size_t first = 0;
		size_t shown = pageCount;
		int pagesWidth = inside;
		bool arrows = false;
		if (cellsFor(inside, static_cast<int>(pageCount)) < minimum)
		{
			arrows = inside >= minimum + ARROW_WIDTH * 2;
			pagesWidth = arrows ? inside - ARROW_WIDTH * 2 : inside;
			shown = static_cast<size_t>(std::max(1, (pagesWidth + SEPARATOR) / (minimum + SEPARATOR)));
			first = std::min(current - std::min(current, shown / 2), pageCount - shown);
		}
		const int cell = cellsFor(pagesWidth, static_cast<int>(shown));
		if (arrows)
		{
			slots.push_back({TabSelectionButtonRole::PreviousPage, current, ARROW_WIDTH});
		}
		for (size_t n = 0; n < shown; ++n)
		{
			slots.push_back({TabSelectionButtonRole::Tab, first + n, cell});
		}
		if (arrows)
		{
			slots.push_back({TabSelectionButtonRole::NextPage, current, ARROW_WIDTH});
		}
	}

	std::weak_ptr<HudPageControl> weakSelf = std::static_pointer_cast<HudPageControl>(shared_from_this());
	while (segments.size() < slots.size())
	{
		auto segment = std::make_shared<Segment>();
		segment->setVisualHeight(drawnHeight);
		segment->addOnClickHandler([weakSelf](W_BUTTON &button) {
			if (auto self = weakSelf.lock())
			{
				self->pressed(static_cast<Segment &>(button));
			}
		});
		attach(segment);
		segments.push_back(segment);
	}
	// The frame takes what the cells need, at the right end (the remainder of an even share stays empty on the left)
	frameWidth = 0;
	for (size_t n = 0; n < slots.size(); ++n)
	{
		const bool gap = n > 0 && slots[n].role == TabSelectionButtonRole::Tab && slots[n - 1].role == TabSelectionButtonRole::Tab;
		frameWidth += slots[n].width + (gap ? SEPARATOR : 0);
	}
	frameWidth = slots.empty() ? 0 : frameWidth + FRAME_BORDER * 2;
	int x = width() - frameWidth;
	for (size_t n = 0; n < segments.size(); ++n)
	{
		if (n >= slots.size())
		{
			segments[n]->hide();
			continue;
		}
		const Slot &slot = slots[n];
		// Each takes the border or separator on its left for hits, the first also what is left of the frame,
		// the last its right border
		const int lead = n == 0 ? x + FRAME_BORDER : (slot.role == TabSelectionButtonRole::Tab && slots[n - 1].role == TabSelectionButtonRole::Tab ? SEPARATOR : 0);
		const int left = n == 0 ? 0 : x;
		const int trail = n + 1 == slots.size() ? FRAME_BORDER : 0;
		const bool disabled = (slot.role == TabSelectionButtonRole::PreviousPage && current == 0)
			|| (slot.role == TabSelectionButtonRole::NextPage && current + 1 >= pageCount);
		// Between two pages, neither of them the current one
		const bool separator = n > 0 && slot.role == TabSelectionButtonRole::Tab && slots[n - 1].role == TabSelectionButtonRole::Tab
			&& slot.page != current && slots[n - 1].page != current;
		segments[n]->set(slot.role, slot.page, !singleSegment && slot.role == TabSelectionButtonRole::Tab && slot.page == current, separator, false, disabled,
		                 lead, slot.width, singleSegment ? pageCount : 0);
		segments[n]->setGeometry(left, 0, lead + slot.width + trail, height());
		segments[n]->show();
		x = left + lead + slot.width;
	}
	show(pageCount > 1);
}

void HudPageControl::pressed(const Segment &segment)
{
	auto strongList = list.lock();
	if (strongList == nullptr)
	{
		return;
	}
	if (wantsPicker() || segment.summary)
	{
		openPicker();
		return;
	}
	const size_t current = strongList->currentPage();
	switch (segment.role)
	{
	case TabSelectionButtonRole::Tab:
		strongList->setCurrentPage(segment.page);
		break;
	case TabSelectionButtonRole::PreviousPage:
		if (current > 0)
		{
			strongList->setCurrentPage(current - 1);
		}
		break;
	case TabSelectionButtonRole::NextPage:
		if (current + 1 < strongList->pages())
		{
			strongList->setCurrentPage(current + 1);
		}
		break;
	}
}

void HudPageControl::openPicker()
{
	auto strongList = list.lock();
	if (strongList == nullptr || picker != nullptr)
	{
		return;
	}
	const size_t pageCount = strongList->pages();
	const size_t current = strongList->currentPage();
	const size_t columns = std::max<size_t>(1, std::min(PICKER_COLUMNS, pageCount));
	const size_t rows = (pageCount + columns - 1) / columns;
	auto frame = std::make_shared<PickerFrame>();
	frame->setGeometry(0, 0, PICKER_PADDING * 2 + static_cast<int>(columns) * (PICKER_BUTTON_WIDTH + PICKER_GAP) - PICKER_GAP,
	                   PICKER_PADDING * 2 + static_cast<int>(rows) * (PICKER_BUTTON_HEIGHT + PICKER_GAP) - PICKER_GAP);
	std::weak_ptr<HudPageControl> weakSelf = std::static_pointer_cast<HudPageControl>(shared_from_this());
	for (size_t page = 0; page < pageCount; ++page)
	{
		auto button = std::make_shared<Segment>();
		button->set(TabSelectionButtonRole::Tab, page, page == current, false, true, false);
		frame->attach(button);
		button->setGeometry(PICKER_PADDING + static_cast<int>(page % columns) * (PICKER_BUTTON_WIDTH + PICKER_GAP),
		                    PICKER_PADDING + static_cast<int>(page / columns) * (PICKER_BUTTON_HEIGHT + PICKER_GAP),
		                    PICKER_BUTTON_WIDTH, PICKER_BUTTON_HEIGHT);
		button->addOnClickHandler([weakSelf, page](W_BUTTON &) {
			auto self = weakSelf.lock();
			if (self == nullptr)
			{
				return;
			}
			if (auto selfList = self->list.lock())
			{
				selfList->setCurrentPage(page);
			}
			if (self->picker != nullptr)
			{
				self->picker->close();
			}
		});
	}
	picker = PopoverWidget::makePopover(shared_from_this(), frame, PopoverWidget::Style::Interactive, PopoverWidget::Alignment::RightOfParent,
	                                    Vector2i(0, 2), [weakSelf]() {
		if (auto self = weakSelf.lock())
		{
			self->picker.reset();
		}
	});
}

void hudSetTouchInputForTests(bool on)
{
	touchForTests = on;
}

bool hudPointerIsTouch()
{
	return touchForTests || inputLastPointerInputWasTouch();
}
