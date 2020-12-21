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

void ClipRectWidget::run(W_CONTEXT *psContext)
{
	W_CONTEXT newContext(psContext);
	newContext.xOffset = psContext->xOffset + x();
	newContext.yOffset = psContext->yOffset + y();
	newContext.mx = psContext->mx - x();
	newContext.my = psContext->my - y() + offset.y;

	runRecursive(&newContext);
}

bool ClipRectWidget::processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	W_CONTEXT newContext(psContext);
	newContext.my = psContext->my + offset.y;
	return WIDGET::processClickRecursive(&newContext, key, wasPressed);
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

void ClipRectWidget::setTopOffset(uint16_t value)
{
	offset.y = value;
}

void ClipRectWidget::setLeftOffset(uint16_t value)
{
	offset.x = value;
}
