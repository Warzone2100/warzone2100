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
 * Functions for clip rect.
 *
 * This widget will render only the childrens that are visible, and whose rectangle is contained in the clip rect's rectangle.
 */

#include "cliprect.h"
#include "lib/ivis_opengl/pieblitfunc.h"

void ClipRectWidget::runRecursive(W_CONTEXT *psContext)
{
	W_CONTEXT newContext(psContext);
	newContext.xOffset = psContext->xOffset + x();
	newContext.yOffset = psContext->yOffset + y();
	newContext.mx = psContext->mx - x();
	newContext.my = psContext->my - y() + offset.y;

	WIDGET::runRecursive(&newContext);
}

std::shared_ptr<WIDGET> ClipRectWidget::findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	W_CONTEXT newContext(psContext);
	newContext.xOffset = psContext->xOffset - offset.x;
	newContext.yOffset = psContext->yOffset - offset.y;
	newContext.mx = psContext->mx + offset.x;
	newContext.my = psContext->my + offset.y;

	auto result = WIDGET::findMouseTargetRecursive(&newContext, key, wasPressed);
	if (result != nullptr)
	{
		*psContext = newContext; // bubble-up the matching hit's context
	}
	return result;
}

void ClipRectWidget::displayRecursive(const WidgetGraphicsContext &context)
{
	if (context.clipContains(geometry()))
	{
		display(context.getXOffset(), context.getYOffset());
	}

	auto childrenContext = context
		.translatedBy(x() - offset.x, + y() - offset.y)
		.clippedBy(WzRect(offset.x, offset.y, width(), height()));

	for (auto child : children())
	{
		if (child->visible())
		{
			child->displayRecursive(childrenContext);
		}
	}
}

bool ClipRectWidget::isChildVisible(const std::shared_ptr<WIDGET>& child)
{
	ASSERT_OR_RETURN(false, child->parent() == shared_from_this(), "Not a child of this widget?");
	WidgetGraphicsContext childrenContext = WidgetGraphicsContext()
		.translatedBy(screenPosX(), screenPosY())
		.clippedBy(WzRect(offset.x, offset.y, width(), height()));
	return childrenContext.clipContains(child->geometry());
}

bool ClipRectWidget::setTopOffset(uint16_t value)
{
	if (value == offset.y)
	{
		return false;
	}
	offset.y = value;
	return true;
}

uint16_t ClipRectWidget::getTopOffset()
{
	return offset.y;
}

bool ClipRectWidget::setLeftOffset(uint16_t value)
{
	if (value == offset.x)
	{
		return false;
	}
	offset.x = value;
	return true;
}

int ClipRectWidget::parentRelativeXOffset(int coord) const
{
	return x() - offset.x + coord;
}

int ClipRectWidget::parentRelativeYOffset(int coord) const
{
	return y() - offset.y + coord;
}
