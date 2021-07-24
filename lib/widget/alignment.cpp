/*
	This file is part of Warzone 2100.
	Copyright (C) 2021  Warzone 2100 Project

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

#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "widget.h"
#include "alignment.h"

std::shared_ptr<AlignmentWidget> Alignment::wrap(std::shared_ptr<WIDGET> widget)
{
    auto wrap = std::make_shared<AlignmentWidget>(*this);
    wrap->attach(widget);
    return wrap;
}

void AlignmentWidget::geometryChanged()
{
    if (children().empty())
    {
        return;
    }

    auto &child = children().front();
    auto childLeft = 0;
    if (alignment.horizontal == Alignment::Horizontal::Center)
    {
        childLeft = std::max(0, width() - child->idealWidth()) / 2;
    }
    else if (alignment.horizontal == Alignment::Horizontal::Right)
    {
        childLeft = std::max(0, width() - child->idealWidth());
    }

    auto childTop = 0;
    if (alignment.vertical == Alignment::Vertical::Center)
    {
        childTop = std::max(0, height() - child->idealHeight()) / 2;
    }
    else if (alignment.vertical == Alignment::Vertical::Bottom)
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

int32_t AlignmentWidget::idealWidth()
{
    return children().empty() ? WIDGET::idealWidth(): children().front()->idealWidth();
}

int32_t AlignmentWidget::idealHeight()
{
    return children().empty() ? WIDGET::idealHeight(): children().front()->idealHeight();
}
