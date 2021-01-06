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
 * Functions for scroll bar.
 */

#include <memory>
#include "scrollbar.h"
#include "lib/ivis_opengl/pieblitfunc.h"

static void displayScrollBar(WIDGET *widget, UDWORD xOffset, UDWORD yOffset)
{
	auto slider = (W_SLIDER *)widget;

	int x0 = slider->x() + xOffset;
	int y0 = slider->y() + yOffset;

	pie_UniTransBoxFill(x0, y0, x0 + slider->width(), y0 + slider->height(), WZCOL_DBLUE);

	auto sliderY = slider->numStops > 0 ? (slider->height() - slider->barSize) * slider->pos / slider->numStops: 0;
	pie_UniTransBoxFill(x0, y0 + sliderY, x0 + slider->width(), y0 + sliderY + slider->barSize, slider->isEnabled() ? WZCOL_LBLUE : WZCOL_FORM_DISABLE);
}

void ScrollBarWidget::initialize()
{
	W_SLDINIT sliderInit;
	attach(slider = std::make_shared<W_SLIDER>(&sliderInit));
	slider->numStops = 0;
	slider->barSize = 0;
	slider->orientation = WSLD_TOP;
	slider->displayFunction = displayScrollBar;
}

void ScrollBarWidget::geometryChanged()
{
	slider->setGeometry(0, 0, width(), height());
}

uint16_t ScrollBarWidget::position() const
{
	return slider->pos;
}

void ScrollBarWidget::setPosition(uint16_t newPosition)
{
	slider->pos = std::min(newPosition, slider->numStops);
}

void ScrollBarWidget::incrementPosition(int32_t amount)
{
	if (isEnabled()) {
		auto pos = amount + slider->pos;
		CLIP(pos, 0, slider->numStops);
		slider->pos = pos;
	}
}

void ScrollBarWidget::setScrollableSize(uint16_t value)
{
	auto moveToBottom = stickToBottom && slider->pos == slider->numStops;
	scrollableSize = value;
	updateSlider();
	if (moveToBottom) {
		slider->pos = slider->numStops;
	}
}

void ScrollBarWidget::setViewSize(uint16_t value)
{
	viewSize = value;
	updateSlider();
}

void ScrollBarWidget::setStickToBottom(bool value)
{
	stickToBottom = value;
}

void ScrollBarWidget::updateSlider()
{
	slider->barSize = scrollableSize > 0 ? viewSize * height() / scrollableSize : 0;
	slider->numStops = MAX(0, scrollableSize - viewSize);
	slider->pos = std::min(slider->pos, slider->numStops);
}

void ScrollBarWidget::enable()
{
	slider->enable();
}

void ScrollBarWidget::disable()
{
	slider->disable();
}

bool ScrollBarWidget::isEnabled() const
{
	return slider->isEnabled();
}
