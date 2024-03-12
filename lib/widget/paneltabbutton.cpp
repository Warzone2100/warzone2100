/*
	This file is part of Warzone 2100.
	Copyright (C) 2023  Warzone 2100 Project

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

#include "lib/ivis_opengl/pieblitfunc.h"
#include "paneltabbutton.h"

constexpr int QCFORM_PANEL_TABS_PADDING = 20;
constexpr int QCFORM_PANEL_TABS_HEIGHT = 20;
const PIELIGHT WZCOL_PANELTABS_FILL_COLOR = pal_RGBA(25, 0, 110, 220);
const PIELIGHT WZCOL_PANELTABS_FILL_COLOR_DARK = pal_RGBA(10, 0, 70, 250);
const PIELIGHT WZCOL_PANELTABS_BORDER_LIGHT = pal_RGBA(255, 255, 255, 150);

std::shared_ptr<WzPanelTabButton> WzPanelTabButton::make(const WzString& text)
{
	auto button = std::make_shared<WzPanelTabButton>();
	button->setString(text);
	button->FontID = font_regular;
	int minButtonWidthForText = iV_GetTextWidth(text, button->FontID);
	button->setGeometry(0, 0, minButtonWidthForText + QCFORM_PANEL_TABS_PADDING, QCFORM_PANEL_TABS_HEIGHT);
	return button;
}

void WzPanelTabButton::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	bool haveText = !pText.isEmpty();

	bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (getState() & WBUT_DISABLE) != 0;
	bool isHighlight = !isDisabled && ((getState() & WBUT_HIGHLIGHT) != 0);

	// Display the button.
	auto light_border = WZCOL_PANELTABS_BORDER_LIGHT;
	auto fill_color = isDown || isDisabled ? WZCOL_PANELTABS_FILL_COLOR_DARK : WZCOL_PANELTABS_FILL_COLOR;
	iV_ShadowBox(x0, y0, x1, y1, 0, isDown || isHighlight ? light_border : WZCOL_FORM_DARK, isDown || isHighlight ? light_border : WZCOL_FORM_DARK, fill_color);

	if (haveText)
	{
		wzText.setText(pText, FontID);
		int fw = wzText.width();
		int fx = x0 + (width() - fw) / 2;
		int fy = y0 + (height() - wzText.lineSize()) / 2 - wzText.aboveBase();
		PIELIGHT textColor = (isDown) ? WZCOL_TEXT_BRIGHT : WZCOL_FORM_TEXT;
		if (isDisabled)
		{
			textColor.byte.a = (textColor.byte.a / 2);
		}
		wzText.render(fx, fy, textColor);
	}
}
