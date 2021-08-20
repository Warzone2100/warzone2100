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
 *  Functions for the bar graph widget
 */

#include "checkbox.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"

WzCheckboxButton::WzCheckboxButton()
: W_BUTTON()
{
	addOnClickHandler([](W_BUTTON& button) {
		WzCheckboxButton& self = static_cast<WzCheckboxButton&>(button);
		self.isChecked = !self.isChecked;
	});
}

Vector2i WzCheckboxButton::calculateDesiredDimensions()
{
	int cbSize = checkboxSize();
	Vector2i checkboxPos{x(), y()};
	int textLeftPos = checkboxPos.x + cbSize + 7;

	// TODO: Incorporate padding?
	return Vector2i(textLeftPos + wzText.width(), std::max(wzText.lineSize(), cbSize));
}

void WzCheckboxButton::display(int xOffset, int yOffset)
{
	wzText.setText(pText.toUtf8(), FontID);

	int x0 = xOffset + x();
	int y0 = yOffset + y();

	bool down = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool grey = (getState() & WBUT_DISABLE) != 0;

	// calculate checkbox dimensions
	int cbSize = checkboxSize();
	Vector2i checkboxOffset{0, (height() - cbSize) / 2}; // left-align, center vertically
	Vector2i checkboxPos{x0 + checkboxOffset.x, y0 + checkboxOffset.y};

	// draw checkbox border
	PIELIGHT notifyBoxAddColor = WZCOL_NOTIFICATION_BOX;
	notifyBoxAddColor.byte.a = uint8_t(float(notifyBoxAddColor.byte.a) * 0.7f);
	pie_UniTransBoxFill(checkboxPos.x, checkboxPos.y, checkboxPos.x + cbSize, checkboxPos.y + cbSize, notifyBoxAddColor);
	iV_Box2(checkboxPos.x, checkboxPos.y, checkboxPos.x + cbSize, checkboxPos.y + cbSize, WZCOL_TEXT_MEDIUM, WZCOL_TEXT_MEDIUM);

	if (down || isChecked)
	{
		// draw checkbox "checked" inside
		#define CB_INNER_INSET 2
		PIELIGHT checkBoxInsideColor = WZCOL_TEXT_MEDIUM;
		checkBoxInsideColor.byte.a = 200;
		pie_UniTransBoxFill(checkboxPos.x + CB_INNER_INSET, checkboxPos.y + CB_INNER_INSET, checkboxPos.x + cbSize - (CB_INNER_INSET), checkboxPos.y + cbSize - (CB_INNER_INSET), checkBoxInsideColor);
	}

	if (grey)
	{
		// disabled, render something over it!
		iV_TransBoxFill(x0, y0, x0 + width(), y0 + height());
	}

	// Display text to the right of the checkbox image
	int textLeftPos = checkboxPos.x + cbSize + 7;
	int fx = textLeftPos;
	int fw = wzText.width();
	int fy = yOffset + y() + (height() - wzText.lineSize()) / 2 - wzText.aboveBase();

	if (style & WBUT_TXTCENTRE)							//check for centering, calculate offset.
	{
		fx = textLeftPos + ((width() - fw) / 2);
	}

	wzText.render(fx, fy, WZCOL_TEXT_MEDIUM);
}
