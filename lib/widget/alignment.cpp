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
	int targetWidth = 0;
	int targetHeight = 0;
	if (alignment.resizing == Alignment::Resizing::Shrink)
	{
		targetWidth = std::min(width(), child->idealWidth());
		targetHeight = std::min(height(), child->idealHeight());
	}
	else if (alignment.resizing == Alignment::Resizing::Fit)
	{
		auto fitRatio = std::min(width() / (float)child->idealWidth(), height() / (float)child->idealHeight());
		targetWidth = static_cast<int>(child->idealWidth() * fitRatio);
		targetHeight = static_cast<int>(child->idealHeight() * fitRatio);
	}

	WzRect childRect(0, 0, 0, 0);
	if (alignment.horizontal == Alignment::Horizontal::Center)
	{
		childRect.setX(std::max(0, width() - targetWidth) / 2);
	}
	else if (alignment.horizontal == Alignment::Horizontal::Right)
	{
		childRect.setX(std::max(0, width() - targetWidth));
	}

	if (alignment.vertical == Alignment::Vertical::Center)
	{
		childRect.setY(std::max(0, height() - targetHeight) / 2);
	}
	else if (alignment.vertical == Alignment::Vertical::Bottom)
	{
		childRect.setY(std::max(0, height() - targetHeight));
	}

	// must set width/height **after** setX/setY
	childRect.setWidth(targetWidth);
	childRect.setHeight(targetHeight);

	child->setGeometry(childRect);
}

int32_t AlignmentWidget::idealWidth()
{
	return children().empty() ? WIDGET::idealWidth(): children().front()->idealWidth();
}

int32_t AlignmentWidget::idealHeight()
{
	return children().empty() ? WIDGET::idealHeight(): children().front()->idealHeight();
}
