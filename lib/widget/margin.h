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

#ifndef __INCLUDED_LIB_WIDGET_MARGIN_H__
#define __INCLUDED_LIB_WIDGET_MARGIN_H__

#include "lib/widget/widget.h"

class MarginWidget;
struct Margin
{
public:
    Margin(int32_t top, int32_t right, int32_t bottom, int32_t left)
        : top(top)
        , right(right)
        , bottom(bottom)
        , left(left)
        {}

    Margin(int32_t vertical, int32_t horizontal): Margin(vertical, horizontal, vertical, horizontal) {}
    explicit Margin(int32_t value): Margin(value, value) {}

    std::shared_ptr<MarginWidget> wrap(std::shared_ptr<WIDGET> widget);

    int32_t top;
    int32_t right;
    int32_t bottom;
    int32_t left;
};

class MarginWidget: public WIDGET
{
public:
	explicit MarginWidget(Margin margin): WIDGET(), margin(margin)
	{
		setTransparentToMouse(true);
	}

protected:
    void geometryChanged() override;
    int32_t idealWidth() override;
    int32_t idealHeight() override;

private:
    Margin margin;
};

#endif // __INCLUDED_LIB_WIDGET_MARGIN_H__
