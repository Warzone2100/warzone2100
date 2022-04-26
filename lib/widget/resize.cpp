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
#include "resize.h"

std::shared_ptr<ResizeWidget> Resize::wrap(std::shared_ptr<WIDGET> widget)
{
    auto wrap = std::make_shared<ResizeWidget>(*this);
    wrap->attach(widget);
    return wrap;
}

void ResizeWidget::geometryChanged()
{
    if (!children().empty())
    {
        children().front()->setGeometry(0, 0, width(), height());
    }
}

int32_t ResizeWidget::idealWidth()
{
	int32_t ideal = resize.minWidth.value_or(0);

	if (!children().empty())
	{
		ideal = std::max(ideal, children().front()->idealWidth());
		if (resize.maxWidth)
		{
			ideal = std::min(ideal, resize.maxWidth.value());
		}
	}

	return ideal;
}

int32_t ResizeWidget::idealHeight()
{
	int32_t ideal = resize.minHeight.value_or(0);

	if (!children().empty())
	{
		ideal = std::max(ideal, children().front()->idealHeight());
		if (resize.maxHeight)
		{
			ideal = std::min(ideal, resize.maxHeight.value());
		}
	}

	return ideal;
}
