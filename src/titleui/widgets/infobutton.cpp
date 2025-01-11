/*
	This file is part of Warzone 2100.
	Copyright (C) 2024  Warzone 2100 Project

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
/** \file
 *  Info Button
 */

#include "infobutton.h"
#include "src/frontend.h"
#include "src/frend.h"
#include "lib/widget/widget.h"
#include "lib/widget/label.h"
#include "lib/ivis_opengl/pieblitfunc.h"

std::shared_ptr<WzInfoButton> WzInfoButton::make()
{
	class make_shared_enabler: public WzInfoButton {};
	auto widget = std::make_shared<make_shared_enabler>();
	return widget;
}

void WzInfoButton::setImageDimensions(int imageSize)
{
	imageDimensions = imageSize;
}

int32_t WzInfoButton::idealHeight()
{
	return imageDimensions;
}

void WzInfoButton::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (getState() & WBUT_DISABLE) != 0;
	bool isHighlight = !isDisabled && ((getState() & WBUT_HIGHLIGHT) != 0);

	if (isHighlight)
	{
		// Display highlight rect
		int highlightRectDimensions = std::max(std::min<int>(width(), height()), imageDimensions + 2);
		int boxX0 = x0 + (width() - highlightRectDimensions) / 2;
		int boxY0 = y0 + (height() - highlightRectDimensions) / 2;
		iV_Box(boxX0, boxY0, boxX0 + highlightRectDimensions + 1, boxY0 + highlightRectDimensions + 1, (isDown) ? imageColorHighlighted : imageColor);
	}

	// Display the info icon, centered
	int imgPosX0 = x0 + (width() - imageDimensions) / 2;
	int imgPosY0 = y0 + (height() - imageDimensions) / 2;
	PIELIGHT imgColor = (isHighlight) ? imageColorHighlighted : imageColor;
	iV_DrawImageFileAnisotropicTint(FrontImages, IMAGE_INFO_CIRCLE, imgPosX0, imgPosY0, Vector2f(imageDimensions, imageDimensions), imgColor);
}
