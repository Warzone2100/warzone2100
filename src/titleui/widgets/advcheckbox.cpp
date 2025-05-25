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
 *  Advanced Fancy Checkbox
 */

#include "advcheckbox.h"
#include "src/frontend.h"
#include "src/frend.h"
#include "lib/ivis_opengl/pieblitfunc.h"

WzAdvCheckbox::WzAdvCheckbox()
{ }

std::shared_ptr<WzAdvCheckbox> WzAdvCheckbox::make(const WzString& displayName, const WzString& description)
{
	class make_shared_enabler: public WzAdvCheckbox {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize(displayName, description);
	return widget;
}

void WzAdvCheckbox::recalcIdealWidth()
{
	cachedIdealWidth = outerHorizontalPadding + imageDimensions + innerHorizontalPadding + iV_GetTextWidth(pText, FontID) + outerHorizontalPadding;
}

void WzAdvCheckbox::initialize(const WzString& displayName, const WzString& description)
{
	setDescription(description);
	setString(displayName);
}

int32_t WzAdvCheckbox::idealWidth()
{
	return cachedIdealWidth;
}

int32_t WzAdvCheckbox::idealHeight()
{
	int result = outerVerticalPadding + std::max<int32_t>(wzText.lineSize(), imageDimensions);

	int descriptionHeight = descriptionWidget->height();
	if (descriptionHeight > 0)
	{
		result += innerVerticalPadding + descriptionWidget->height();
	}

	result += outerVerticalPadding;
	return result;
}

void WzAdvCheckbox::highlight(W_CONTEXT *psContext)
{
	W_BUTTON::highlight(psContext);
	PIELIGHT descriptionHighlightColor = WZCOL_TEXT_BRIGHT;
	descriptionHighlightColor.byte.a = static_cast<UBYTE>(static_cast<int>(descriptionHighlightColor.byte.a * 3) / 4);
	descriptionWidget->forceSetAllFontColor(descriptionHighlightColor);
}

void WzAdvCheckbox::highlightLost()
{
	W_BUTTON::highlightLost();
	descriptionWidget->forceSetAllFontColor(WZCOL_TEXT_MEDIUM);
}

void WzAdvCheckbox::setImageDimensions(int imageSize)
{
	imageDimensions = imageSize;
	recalcIdealWidth();
}

bool WzAdvCheckbox::isChecked() const
{
	return checked;
}

void WzAdvCheckbox::setIsChecked(bool val)
{
	checked = val;
}

void WzAdvCheckbox::setOuterHorizontalPadding(int padding)
{
	outerHorizontalPadding = padding;
	recalcIdealWidth();
}

void WzAdvCheckbox::setInnerHorizontalPadding(int padding)
{
	innerHorizontalPadding = padding;
	recalcIdealWidth();
}

void WzAdvCheckbox::setOuterVerticalPadding(int padding)
{
	outerVerticalPadding = padding;
	recalcIdealWidth();
}

void WzAdvCheckbox::setInnerVerticalPadding(int padding)
{
	innerVerticalPadding = padding;
	recalcIdealWidth();
}

/* Respond to a mouse button up */
void WzAdvCheckbox::released(W_CONTEXT *context, WIDGET_KEY key)
{
	bool clickAndReleaseOnThisButton = ((state & WBUT_DOWN) != 0); // relies on W_BUTTON handling to properly set WBUT_DOWN

	if (clickAndReleaseOnThisButton)
	{
		// toggle click-lock ("checked") state
		setIsChecked(!isChecked());
	}

	W_BUTTON::released(context, key);
}

void WzAdvCheckbox::setString(WzString string)
{
	W_BUTTON::setString(string);
	recalcIdealWidth();
}

void WzAdvCheckbox::setDescription(const WzString& description)
{
	if (descriptionWidget)
	{
		detach(descriptionWidget);
	}
	descriptionWidget = std::make_shared<Paragraph>();
	descriptionWidget->setFont(font_small);
	descriptionWidget->setFontColour(WZCOL_TEXT_MEDIUM);
	descriptionWidget->setGeometry(0, 0, 400, 40);
	descriptionWidget->addText(description);
	descriptionWidget->setTransparentToMouse(true);
	attach(descriptionWidget);
}

void WzAdvCheckbox::geometryChanged()
{
	// reposition description paragraph beneath the image and line of text
	int descX0 = outerHorizontalPadding + imageDimensions + innerHorizontalPadding;
	int descY0 = outerVerticalPadding + std::max<int32_t>(wzText.lineSize(), imageDimensions) + innerVerticalPadding;
	descriptionWidget->setGeometry(descX0, descY0, width() - descX0 - outerHorizontalPadding, height() - descY0 - outerVerticalPadding); // while we specify a height, the paragraph widget will calculate and set its to its needed full height, given the supplied width
}

void WzAdvCheckbox::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int w = width();

	bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isChecked = checked;
	bool isDisabled = (getState() & WBUT_DISABLE) != 0;
	bool isHighlight = !isDisabled && isMouseOverWidget(); //!isDisabled && ((getState() & WBUT_HIGHLIGHT) != 0);

	iV_fonts fontID = wzText.getFontID();
	if (fontID == font_count || lastWidgetWidth != width() || wzText.getText() != pText)
	{
		fontID = FontID;
	}
	wzText.setText(pText, fontID);

	int availableButtonTextWidth = w - (outerHorizontalPadding * 2) - (imageDimensions + innerHorizontalPadding);
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

	// TODO: Draw background, if configured

	// Draw the main text
	PIELIGHT textColor = (isHighlight) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM;
	if (isDisabled)
	{
		textColor.byte.a = (textColor.byte.a / 2);
	}

	int textXOffset = outerHorizontalPadding + imageDimensions + innerHorizontalPadding;
	int textX0 = x0 + textXOffset;
	int textY0 = y0 + outerVerticalPadding - wzText.aboveBase();

	const int maxTextDisplayableWidth = w - textXOffset - outerHorizontalPadding;
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

	// Draw the image to the left
	int imgPosX0 = x0 + outerHorizontalPadding;
	int imgPosY0 = y0 + outerVerticalPadding + (wzText.lineSize() - imageDimensions) / 2;
	PIELIGHT imgColor = (isHighlight) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM;
	if (isDisabled)
	{
		imgColor.byte.a = (imgColor.byte.a / 2);
	}
	else if (isDown)
	{
		imgColor = WZCOL_TEXT_DARK;
	}
	UWORD imageID = (isChecked) ? IMAGE_CHECK_SQUARE_FILL : IMAGE_CHECK_SQUARE_EMPTY;
	iV_DrawImageFileAnisotropicTint(FrontImages, imageID, imgPosX0, imgPosY0, Vector2f(imageDimensions, imageDimensions), imgColor);
}
