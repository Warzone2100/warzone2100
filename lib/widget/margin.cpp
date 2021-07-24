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

#include "lib/widget/margin.h"

std::shared_ptr<MarginWidget> Margin::wrap(std::shared_ptr<WIDGET> widget)
{
    auto wrap = std::make_shared<MarginWidget>(*this);
    wrap->attach(widget);
    return wrap;
}

void MarginWidget::geometryChanged()
{
	if (!children().empty())
	{
		auto &child = children().front();
		child->setGeometry(
			margin.left,
			margin.top,
			std::max(0, width() - margin.left - margin.right),
			std::max(0, height() - margin.top - margin.bottom)
		);
	}
}

int32_t MarginWidget::idealWidth()
{
	auto innerWidth = children().empty() ? 0: children().front()->idealWidth();
	return innerWidth + margin.left + margin.right;
}

int32_t MarginWidget::idealHeight()
{
	auto innerHeight = children().empty() ? 0: children().front()->idealHeight();
	return innerHeight + margin.top + margin.bottom;
}
