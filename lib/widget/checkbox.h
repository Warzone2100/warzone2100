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
/** @file
 *  Definitions for Bar Graph functions.
 */

#ifndef __INCLUDED_LIB_WIDGET_CHECKBOX_H__
#define __INCLUDED_LIB_WIDGET_CHECKBOX_H__

#include "button.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/framework/vector.h"
#include <string>

struct WzCheckboxButton : public W_BUTTON
{
public:
	WzCheckboxButton();

	void display(int xOffset, int yOffset);

	Vector2i calculateDesiredDimensions();

	bool getIsChecked() const { return isChecked; }
private:
	int checkboxSize()
	{
		wzText.setText(pText.toUtf8(), FontID);
		return wzText.lineSize() - 2;
	}
private:
	WzText wzText;
	bool isChecked = false;
};

#endif // __INCLUDED_LIB_WIDGET_CHECKBOX_H__
