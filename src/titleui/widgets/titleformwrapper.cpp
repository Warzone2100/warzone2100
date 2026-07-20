// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

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
 *  TitleFormWrapper
 */

#include "../../frontend.h"
#include "../../multiint.h"
#include "../../hci.h"
#include "../../frend.h"
#include "../../intdisplay.h"
#include "titleformwrapper.h"

// MARK: - IntFormAnimatedWrapper

class IntFormAnimatedWrapper : public IntFormAnimated
{
protected:
	IntFormAnimatedWrapper()
	: IntFormAnimated(true)
	{ }
	void initialize(const std::shared_ptr<WIDGET>& displayWidget);
public:
	static std::shared_ptr<IntFormAnimatedWrapper> make(const std::shared_ptr<WIDGET>& displayWidget);
public:
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
	virtual void geometryChanged() override;
private:

private:
	std::shared_ptr<WIDGET> displayWidget;
};

void IntFormAnimatedWrapper::initialize(const std::shared_ptr<WIDGET>& displayWidget_)
{
	displayWidget = displayWidget_;
	attach(displayWidget);
}

std::shared_ptr<IntFormAnimatedWrapper> IntFormAnimatedWrapper::make(const std::shared_ptr<WIDGET>& displayWidget)
{
	class make_shared_enabler: public IntFormAnimatedWrapper {};
	auto result = std::make_shared<make_shared_enabler>();
	result->initialize(displayWidget);
	return result;
}

int32_t IntFormAnimatedWrapper::idealWidth()
{
	return 2 + displayWidget->idealWidth();
}

int32_t IntFormAnimatedWrapper::idealHeight()
{
	return 2 + displayWidget->idealHeight();
}

void IntFormAnimatedWrapper::geometryChanged()
{
	int w = width();
	int h = height();
	displayWidget->setGeometry(1, 1, w - 2, h - 2);
}

// MARK: - TitleFormSideText

class TitleFormSideText : public WIDGET
{
protected:
	TitleFormSideText() { }
	void initialize(iV_fonts font, const WzString& text);
public:
	static std::shared_ptr<TitleFormSideText> make(iV_fonts font, const WzString& text);

	virtual void display(int xOffset, int yOffset) override;
	virtual WzString getString() const override;
	virtual void setString(WzString string) override;
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
private:
	WzString text;
	iV_fonts fontID;
	WzText wzText;
	int32_t horizontalPadding = 0;
	int32_t verticalPadding = 0;
	int32_t lastWidgetWidth = 0;
	bool isTruncated = false;
};

std::shared_ptr<TitleFormSideText> TitleFormSideText::make(iV_fonts font, const WzString& text)
{
	class make_shared_enabler: public TitleFormSideText {};
	auto result = std::make_shared<make_shared_enabler>();
	result->initialize(font, text);
	return result;
}

void TitleFormSideText::initialize(iV_fonts font, const WzString& str)
{
	fontID = font;
	setString(str);
}

void TitleFormSideText::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int w = width();
	int h = height();

	iV_fonts currFontID = wzText.getFontID();
	if (fontID == font_count || lastWidgetWidth != width() || wzText.getText() != text)
	{
		currFontID = fontID;
	}
	wzText.setText(text, currFontID);

	const int availableButtonTextHeight = h - (verticalPadding * 2);
	if (wzText.width() > availableButtonTextHeight)
	{
		// text would exceed the bounds of the button area
		// try to shrink font so it fits
		do {
			if (currFontID == font_small || currFontID == font_bar)
			{
				break;
			}
			auto result = iV_ShrinkFont(currFontID);
			if (!result.has_value())
			{
				break;
			}
			currFontID = result.value();
			wzText.setText(text, currFontID);
		} while (wzText.width() > availableButtonTextHeight);
	}
	lastWidgetWidth = width();

	int maxTextDisplayableHeight = availableButtonTextHeight;
	isTruncated = maxTextDisplayableHeight < wzText.width();
	if (isTruncated)
	{
		maxTextDisplayableHeight -= (iV_GetEllipsisWidth(wzText.getFontID()) + 2);
	}

	int textX0 = static_cast<int>(x0 + (w - wzText.lineSize()) / 2 - float(wzText.aboveBase()));
	int textY0 = y0 + std::min(wzText.width(), availableButtonTextHeight);

	wzText.render(textX0 + 2, textY0 + 2, WZCOL_GREY, 270.f, maxTextDisplayableHeight);
	wzText.render(textX0, textY0, WZCOL_TEXT_BRIGHT, 270.f, maxTextDisplayableHeight);

	if (isTruncated)
	{
		// Render ellipsis
		iV_DrawEllipsis(wzText.getFontID(), Vector2f(textX0, textY0 + maxTextDisplayableHeight + 2), WZCOL_TEXT_BRIGHT, 270.f);
	}
}

WzString TitleFormSideText::getString() const
{
	return text;
}

void TitleFormSideText::setString(WzString string)
{
	text = string;
	wzText.setText(text, fontID);
}

int32_t TitleFormSideText::idealWidth()
{
	// is the height of the text
	return (horizontalPadding * 2) + wzText.lineSize();
}

int32_t TitleFormSideText::idealHeight()
{
	// is the width of the text
	return (verticalPadding * 2) + wzText.width();
}

// MARK: - TitleFormWrapper

class TitleFormWrapper : public WIDGET
{
protected:
	TitleFormWrapper() { }
	void initialize(const GetTitleStringFunc& getTitleStringFunc, const std::shared_ptr<WIDGET>& displayWidget);
public:
	static std::shared_ptr<TitleFormWrapper> make(const GetTitleStringFunc& getTitleStringFunc, const std::shared_ptr<WIDGET>& displayWidget);

	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;
	virtual void geometryChanged() override;
	virtual void screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight) override;
private:
	GetTitleStringFunc getTitleStringFunc;
	std::shared_ptr<TitleFormSideText> sideText;
	std::shared_ptr<IntFormAnimatedWrapper> formWrapper;
	int32_t outerPaddingX = 10;
	int32_t titleFormSpacing = 10;
	int32_t outerPaddingY = 20;
};

void TitleFormWrapper::initialize(const GetTitleStringFunc& getTitleStringFunc_, const std::shared_ptr<WIDGET>& displayWidget_)
{
	getTitleStringFunc = getTitleStringFunc_;

	sideText = TitleFormSideText::make(font_large, getTitleStringFunc());
	attach(sideText);

	formWrapper = IntFormAnimatedWrapper::make(displayWidget_);
	attach(formWrapper);
}

std::shared_ptr<TitleFormWrapper> TitleFormWrapper::make(const GetTitleStringFunc& getTitleStringFunc, const std::shared_ptr<WIDGET>& displayWidget)
{
	class make_shared_enabler: public TitleFormWrapper {};
	auto result = std::make_shared<make_shared_enabler>();
	result->initialize(getTitleStringFunc, displayWidget);
	return result;
}

int32_t TitleFormWrapper::idealWidth()
{
	int32_t outerTitleWidth = outerPaddingX + sideText->idealWidth() + titleFormSpacing;
	return (outerTitleWidth * 2) + formWrapper->idealWidth();
}

int32_t TitleFormWrapper::idealHeight()
{
	return (outerPaddingY * 2) + std::max(sideText->idealHeight(), formWrapper->idealHeight());
}

void TitleFormWrapper::geometryChanged()
{
	int w = width();
	int h = height();

	if (w == 0 || h == 0)
	{
		return;
	}

	int32_t sideTextWidth = sideText->idealWidth();
	int32_t sideTextHeight = sideText->idealHeight();

	int32_t formUnavailableWidth = (outerPaddingX + sideTextWidth + titleFormSpacing) * 2;

	int32_t maxAvailableFormWidth = w - formUnavailableWidth;
	int32_t formWidth = std::min(maxAvailableFormWidth, formWrapper->idealWidth());

	int32_t maxAvailableHeight = h - (outerPaddingY * 2);
	int32_t formHeight = std::min(maxAvailableHeight, formWrapper->idealHeight());

	int childY0 = (h - formHeight) / 2;
	int32_t formX0 = (w - formWidth) / 2;
	formWrapper->setGeometry(formX0, childY0, formWidth, formHeight);

	int32_t sideTextX0 = formX0 - (sideTextWidth + titleFormSpacing);
	sideText->setGeometry(sideTextX0, childY0, sideTextWidth, sideTextHeight);
}

void TitleFormWrapper::screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight)
{
	// refresh the side text (in case the current language or display scaling changed)
	sideText->setString(getTitleStringFunc());

	WIDGET::screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);

	if (oldWidth == newWidth && oldHeight == newHeight)
	{
		geometryChanged(); // may need to relayout title text anyway if it changed
	}
}

// MARK: -

std::shared_ptr<WIDGET> makeTitleFormWrapper(const GetTitleStringFunc& getTitleStringFunc, const std::shared_ptr<WIDGET>& displayWidget)
{
	return TitleFormWrapper::make(getTitleStringFunc, displayWidget);
}
