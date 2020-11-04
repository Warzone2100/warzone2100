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
 * Definitions for clip rect functions.
 *
 * This widget will render only the childrens that are visible, and whose rectangle is contained in the clip rect's rectangle.
 */

#ifndef __INCLUDED_LIB_WIDGET_CLIPRECT_H__
#define __INCLUDED_LIB_WIDGET_CLIPRECT_H__

#include "widget.h"

class ClipRectWidget : public WIDGET
{
public:
	ClipRectWidget(WIDGET *parent) : WIDGET(parent) {}

	void display(int xOffset, int yOffset) override;
	void run(W_CONTEXT *psContext) override;
	bool processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;
	void displayRecursive(int xOffset, int yOffset) override;
	void setTopOffset(uint16_t value);
	void setLeftOffset(uint16_t value);

private:
	glm::ivec2 offset = {0, 0};
};

#endif // __INCLUDED_LIB_WIDGET_CLIPRECT_H__
