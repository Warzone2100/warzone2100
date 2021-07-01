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

#ifndef __INCLUDED_LIB_WIDGET_ALIGNMENT_H__
#define __INCLUDED_LIB_WIDGET_ALIGNMENT_H__

#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "widget.h"

enum class VerticalAlignment
{
    Top,
    Center,
    Bottom
};

enum class HorizontalAlignment
{
    Left,
    Center,
    Right
};

class AlignmentWidget: public WIDGET
{
public:
    AlignmentWidget(VerticalAlignment verticalAlignment, HorizontalAlignment horizontalAlignment)
        : verticalAlignment(verticalAlignment)
        , horizontalAlignment(horizontalAlignment)
    {
    }

protected:
    void geometryChanged() override
    {
        if (!children().empty())
        {
            auto &child = children().front();
            auto childLeft = 0;
            if (horizontalAlignment == HorizontalAlignment::Center)
            {
                childLeft = std::max(0, width() - child->idealWidth()) / 2;
            }
            else if (horizontalAlignment == HorizontalAlignment::Right)
            {
                childLeft = std::max(0, width() - child->idealWidth());
            }
            auto childTop = 0;
            if (verticalAlignment == VerticalAlignment::Center)
            {
                childTop = std::max(0, height() - child->idealHeight()) / 2;
            }
            else if (verticalAlignment == VerticalAlignment::Bottom)
            {
                childTop = std::max(0, height() - child->idealHeight());
            }
            child->setGeometry(
                childLeft,
                childTop,
                std::min(width(), child->idealWidth()),
                std::min(height(), child->idealHeight())
            );
        }
    }

    int32_t idealWidth() override
    {
        return children().empty() ? WIDGET::idealWidth(): children().front()->idealWidth();
    }

    int32_t idealHeight() override
    {
        return children().empty() ? WIDGET::idealHeight(): children().front()->idealHeight();
    }

private:
    VerticalAlignment verticalAlignment;
    HorizontalAlignment horizontalAlignment;
};

#endif // __INCLUDED_LIB_WIDGET_ALIGNMENT_H__
