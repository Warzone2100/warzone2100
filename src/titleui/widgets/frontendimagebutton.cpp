/*
	This file is part of Warzone 2100.
	Copyright (C) 2024-2025  Warzone 2100 Project

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
 *  Frontend Image Button
 */

#include "infobutton.h"
#include "src/frontend.h"
#include "src/frend.h"
#include "lib/widget/widget.h"
#include "lib/widget/label.h"
#include "lib/ivis_opengl/pieblitfunc.h"

WzFrontendImageButton::WzFrontendImageButton()
{ }

std::shared_ptr<WzFrontendImageButton> WzFrontendImageButton::make(optional<UWORD> frontendImgID)
{
	class make_shared_enabler: public WzFrontendImageButton {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->setImage(frontendImgID);
	return widget;
}

void WzFrontendImageButton::setImage(optional<UWORD> newFrontendImgID)
{
	missingImage = false;

	// validate newFrontendImgID value
	if (newFrontendImgID.has_value() && newFrontendImgID.value() >= FrontImages->numImages())
	{
		// out of bounds value (likely a mod has an incomplete / older frontend.img file)
		debug(LOG_ERROR, "frontend.img missing expected image id \"%u\" (num loaded: %zu) - remove any mods", static_cast<unsigned>(newFrontendImgID.value()), FrontImages->numImages());
		newFrontendImgID.reset();
		missingImage = true;
	}

	frontendImgID = newFrontendImgID;
	recalcIdealWidth();
}

void WzFrontendImageButton::setImageDimensions(int imageSize)
{
	if (imageSize == imageDimensions)
	{
		return;
	}
	imageDimensions = imageSize;
	recalcIdealWidth();
}

void WzFrontendImageButton::setPadding(int newHorizPadding, int newVertPadding)
{
	horizontalPadding = newHorizPadding;
	verticalPadding = newVertPadding;
	recalcIdealWidth();
}

void WzFrontendImageButton::setImageHorizontalOffset(int xOffset)
{
	imageHorizontalOffset = xOffset;
}

void WzFrontendImageButton::setCustomTextColors(optional<PIELIGHT> textColor, optional<PIELIGHT> highlightedTextColor)
{
	customTextColor = textColor;
	customTextHighlightColor = highlightedTextColor;
}

void WzFrontendImageButton::setCustomImageColor(optional<PIELIGHT> color)
{
	customImgColor = color;
}

void WzFrontendImageButton::setBackgroundColor(optional<PIELIGHT> color)
{
	customBackgroundColor = color;
}

void WzFrontendImageButton::setString(WzString string)
{
	W_BUTTON::setString(string);
	recalcIdealWidth();
}

bool WzFrontendImageButton::shouldDisplayImage() const
{
	return (frontendImgID.has_value() || missingImage) && imageDimensions > 0;
}

void WzFrontendImageButton::recalcIdealWidth()
{
	cachedIdealWidth = horizontalPadding;
	if (shouldDisplayImage())
	{
		cachedIdealWidth += imageDimensions + horizontalPadding;
	}
	auto textWidth = iV_GetTextWidth(pText, FontID);
	if (textWidth > 0)
	{
		cachedIdealWidth += textWidth + horizontalPadding;
	}
}

int32_t WzFrontendImageButton::idealWidth()
{
	return cachedIdealWidth;
}

int32_t WzFrontendImageButton::idealHeight()
{
	return (verticalPadding * 2) + std::max<int32_t>(imageDimensions, iV_GetTextLineSize(FontID));
}

void WzFrontendImageButton::setBorderDrawMode(BorderDrawMode mode)
{
	borderDrawMode = mode;
}

void WzFrontendImageButton::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int w = width();

	bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (getState() & WBUT_DISABLE) != 0;
	bool isHighlight = !isDisabled && ((getState() & WBUT_HIGHLIGHT) != 0);
	bool hasImage = shouldDisplayImage();

	PIELIGHT textColor = (isHighlight) ? customTextHighlightColor.value_or(WZCOL_TEXT_BRIGHT) : customTextColor.value_or(WZCOL_TEXT_MEDIUM);
	if (isDisabled)
	{
		textColor.byte.a = (textColor.byte.a / 2);
	}

	if (customBackgroundColor.has_value())
	{
		int boxX0 = x0;
		int boxY0 = y0;
		int boxX1 = boxX0 + width();
		int boxY1 = boxY0 + height();
		pie_UniTransBoxFill(boxX0, boxY0, boxX1, boxY1, customBackgroundColor.value());
	}

	switch (borderDrawMode)
	{
		case BorderDrawMode::Never:
			break;
		case BorderDrawMode::Highlighted:
		{
			int highlightRectDimensions = std::max(std::min<int>(width(), height()), imageDimensions + 2);
			int boxX0 = x0 + (width() - highlightRectDimensions) / 2;
			int boxY0 = y0 + (height() - highlightRectDimensions) / 2;
			if (isDown)
			{
				pie_UniTransBoxFill(boxX0 + 1, boxY0 + 1, boxX0 + highlightRectDimensions, boxY0 + highlightRectDimensions, WZCOL_FORM_DARK);
			}
			if ((isHighlight || isDown) && !isDisabled)
			{
				// Display highlight rect
				PIELIGHT borderColor = (isDown) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM;
				iV_Box(boxX0 + 1, boxY0 + 1, boxX0 + highlightRectDimensions, boxY0 + highlightRectDimensions, borderColor);
			}
			break;
		}
		case BorderDrawMode::Always:
		{
			// Display outer border
			PIELIGHT borderColor = (isDown) ? WZCOL_TEXT_DARK : ((isHighlight) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM);
			if (isDisabled)
			{
				borderColor.byte.a = (borderColor.byte.a / 2);
			}
			int boxX0 = x0;
			int boxY0 = y0;
			int boxX1 = boxX0 + width();
			int boxY1 = boxY0 + height();
			iV_Box(boxX0 + 1, boxY0 + 1, boxX1, boxY1, borderColor);
			break;
		}
	}

	auto bHasText = !pText.isEmpty();
	if (bHasText)
	{
		// Display text
		iV_fonts fontID = wzText.getFontID();
		if (fontID == font_count || lastWidgetWidth != width() || wzText.getText() != pText)
		{
			fontID = FontID;
		}
		wzText.setText(pText, fontID);

		int availableButtonTextWidth = w - (horizontalPadding * 2) - ((hasImage) ? (imageDimensions + horizontalPadding + imageHorizontalOffset) : 0);
		if (wzText.width() > availableButtonTextWidth)
		{
			// text would exceed the bounds of the button area
			// try to shrink font so it fits
			do {
				if (fontID == font_small || fontID == font_bar)
				{
					break;
				}
				auto result = iV_ShrinkFont(fontID);
				if (!result.has_value())
				{
					break;
				}
				fontID = result.value();
				wzText.setText(pText, fontID);
			} while (wzText.width() > availableButtonTextWidth);
		}
		lastWidgetWidth = width();

		// Draw the main text
		int textX0 = x0 + horizontalPadding + ((hasImage) ? imageDimensions + horizontalPadding + imageHorizontalOffset : 0);
		int textY0 = static_cast<int>(y0 + (height() - wzText.lineSize()) / 2 - float(wzText.aboveBase()));

		const int maxTextDisplayableWidth = w - (horizontalPadding * 2);
		int maxDisplayableMainTextWidth = maxTextDisplayableWidth;
		bool isTruncated = maxDisplayableMainTextWidth < wzText.width();
		if (isTruncated)
		{
			maxDisplayableMainTextWidth -= (iV_GetEllipsisWidth(wzText.getFontID()) + 2);
		}
		wzText.render(textX0, textY0, textColor, 0.0f, maxDisplayableMainTextWidth);
		if (isTruncated)
		{
			// Render ellipsis
			iV_DrawEllipsis(wzText.getFontID(), Vector2f(textX0 + maxDisplayableMainTextWidth + 2, textY0), textColor);
		}
	}

	int imgPosX0;
	if (bHasText)
	{
		// Display the image to the left
		imgPosX0 = x0 + horizontalPadding + imageHorizontalOffset;
	}
	else
	{
		// Display the image horizontally centered
		imgPosX0 = x0 + (width() - imageDimensions) / 2;
	}
	int imgPosY0 = y0 + (height() - imageDimensions) / 2;

	if (frontendImgID.has_value())
	{
		PIELIGHT imgColor = customImgColor.value_or(textColor);
		iV_DrawImageFileAnisotropicTint(FrontImages, frontendImgID.value(), imgPosX0, imgPosY0, Vector2f(imageDimensions, imageDimensions), imgColor);
	}
	else if (missingImage)
	{
		pie_UniTransBoxFill(imgPosX0, imgPosY0, imgPosX0 + imageDimensions, imgPosY0 + imageDimensions, WZCOL_RED);
	}
}
