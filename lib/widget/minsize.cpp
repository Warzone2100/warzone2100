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
#include "minsize.h"

std::shared_ptr<MinSizeWidget> MinSize::wrap(std::shared_ptr<WIDGET> widget)
{
    auto wrap = std::make_shared<MinSizeWidget>(*this);
    wrap->attach(widget);
    return wrap;
}

void MinSizeWidget::geometryChanged()
{
    if (!children().empty())
    {
        children().front()->setGeometry(0, 0, width(), height());
    }
}

int32_t MinSizeWidget::idealWidth()
{
    return std::max(
		minSize.width.value_or(0),
		children().empty() ? 0 : children().front()->idealWidth()
	);
}

int32_t MinSizeWidget::idealHeight()
{
    return std::max(
		minSize.height.value_or(0),
		children().empty() ? 0 : children().front()->idealHeight()
	);
}
