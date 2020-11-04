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
 * Definitions for scroll bar functions.
 */

#ifndef __INCLUDED_LIB_WIDGET_SCROLLBAR_H__
#define __INCLUDED_LIB_WIDGET_SCROLLBAR_H__

#include "widget.h"
#include "slider.h"

class ScrollBarWidget : public WIDGET
{
public:
	ScrollBarWidget(WIDGET *parent);

	uint16_t position() const;
	void setScrollableSize(uint16_t value);
	void setViewSize(uint16_t value);
	void setStickToBottom(bool value);
	void incrementPosition(int32_t amount);
	void enable();
	void disable();
	bool isEnabled() const;

protected:
	void geometryChanged() override;

private:
	W_SLIDER *slider;
	uint16_t scrollableSize = 1;
	uint16_t viewSize = 1;
	bool stickToBottom = false;

	void updateSlider();
};

#endif // __INCLUDED_LIB_WIDGET_SCROLLBAR_H__
