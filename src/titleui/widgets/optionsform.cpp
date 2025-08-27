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
 *  Widgets for displaying a list of changeable options
 */

#include "optionsform.h"
#include "src/frend.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/widget/form.h"
#include "lib/widget/label.h"
#include "lib/widget/scrollablelist.h"
#include "lib/widget/paragraph.h"
#include "lib/widget/dropdown.h"
#include "lib/widget/gridlayout.h"
#include "lib/widget/margin.h"
#include "lib/widget/popover.h"
#include "lib/netplay/netplay.h"
#include "src/multiplay.h"
#include "src/main.h"
#include "src/intimage.h"

// MARK: OptionsSlider

static void displayBigSlider(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	W_SLIDER *Slider = (W_SLIDER *)psWidget;
	int x = xOffset + psWidget->x();
	int y = yOffset + psWidget->y();

	iV_DrawImage(IntImages, IMAGE_SLIDER_BIG, x, y);			// draw bdrop

	int sx = (Slider->width() - 3 - Slider->barSize) * Slider->pos / std::max<int>(Slider->numStops, 1);  // determine pos.
	iV_DrawImage(IntImages, IMAGE_SLIDER_BIGBUT, x + 3 + sx, y + 3);								//draw amount
}

OptionsSlider::OptionsSlider()
{ }

std::shared_ptr<OptionsSlider> OptionsSlider::make(int32_t minValue, int32_t maxValue, int32_t stepValue, CurrentValueFunc currentValFunc, OnChangeValueFunc onChange, bool needsStateUpdates)
{
	ASSERT_OR_RETURN(nullptr, minValue < maxValue, "minValue is not < maxValue");
	ASSERT_OR_RETURN(nullptr, currentValFunc != nullptr, "Invalid currentVal func");
	ASSERT_OR_RETURN(nullptr, IntImages != nullptr, "IntImages not loaded?");

	uint32_t totalRange = static_cast<uint32_t>(maxValue - minValue);
	UWORD numStops = (maxValue - minValue) / stepValue;
	UWORD currPos = (currentValFunc() - minValue) / stepValue;

	W_SLDINIT sSldInit;
	sSldInit.formID		= 0;
	sSldInit.id			= 0;
	sSldInit.width		= iV_GetImageWidth(IntImages, IMAGE_SLIDER_BIG);
	sSldInit.height		= iV_GetImageHeight(IntImages, IMAGE_SLIDER_BIG);
	sSldInit.numStops	= numStops;
	sSldInit.barSize	= iV_GetImageHeight(IntImages, IMAGE_SLIDER_BIG);
	sSldInit.pos		= currPos;
	sSldInit.pDisplay	= displayBigSlider;

	auto sliderWidget = std::make_shared<W_SLIDER>(&sSldInit);

	class make_shared_enabler : public OptionsSlider { };
	auto result = std::make_shared<make_shared_enabler>();
	result->sliderWidget = sliderWidget;
	result->minValue = minValue;
	result->maxValue = maxValue;
	result->totalRange = totalRange;
	result->stepValue = stepValue;
	result->currentValFunc = currentValFunc;
	result->onChangeValueFunc = onChange;
	result->needsStateUpdates = needsStateUpdates;

	auto weakParent = std::weak_ptr<OptionsSlider>(result);
	result->sliderWidget->addOnChange([weakParent](W_SLIDER& s) {
		auto strongParent = weakParent.lock();
		ASSERT_OR_RETURN(, strongParent != nullptr, "Null parent");
		strongParent->handleSliderPosChanged();
	});
	result->attach(result->sliderWidget);

	result->cachedIdealSliderWidth = iV_GetImageWidth(IntImages, IMAGE_SLIDER_BIG);
	result->updateSliderValueText();

	return result;
}

int32_t OptionsSlider::currentValue() const
{
	return (static_cast<int32_t>(sliderWidget->pos) * stepValue) + minValue;
}

void OptionsSlider::handleSliderPosChanged()
{
	auto newValue = currentValue();
	if (onChangeValueFunc)
	{
		onChangeValueFunc(newValue);
	}
	updateSliderValueText();
}

void OptionsSlider::updateSliderValueText()
{
	text = WzString::number(currentValue());
	cacheIdealTextSize();
}

size_t OptionsSlider::maxNumValueCharacters()
{
	WzString minValueStr = WzString::number(minValue);
	WzString maxValueStr = WzString::number(maxValue);
	return std::max<size_t>(minValueStr.length(), maxValueStr.length());
}

void OptionsSlider::cacheIdealTextSize()
{
	auto tmpStr = WzString("444444");
	tmpStr.truncate(maxNumValueCharacters());
	cachedIdealTextWidth = iV_GetTextWidth(tmpStr.toUtf8().c_str(), FontID) + 2;
	cachedIdealTextLineSize = iV_GetTextLineSize(FontID);
}

void OptionsSlider::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int w = width();
	int h = height();

	bool isHighlight = false;

	iV_fonts fontID = wzText.getFontID();
	if (fontID == font_count || lastWidgetWidth != width() || wzText.getText() != text)
	{
		fontID = FontID;
		wzText.setText(text, fontID);
	}

	const int maxTextDisplayableWidth  = w - cachedIdealSliderWidth - (horizontalPadding * 3);
	if (wzText->width() > maxTextDisplayableWidth)
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
			wzText.setText(text, fontID);
		} while (wzText->width() > maxTextDisplayableWidth);
	}
	lastWidgetWidth = width();

	// Draw the main text
	PIELIGHT textColor = (isHighlight) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM;
	if (isDisabled)
	{
		textColor.byte.a = (textColor.byte.a / 2);
	}

	int maxDisplayableMainTextWidth = maxTextDisplayableWidth;

	int textX0 = x0 + w - horizontalPadding - std::min(maxDisplayableMainTextWidth, wzText->width());
	int textY0 = y0 + (h - wzText->lineSize()) / 2 - wzText->aboveBase();

	isTruncated = maxDisplayableMainTextWidth < wzText->width();
	if (isTruncated)
	{
		maxDisplayableMainTextWidth -= (iV_GetEllipsisWidth(wzText.getFontID()) + 2);
	}
	wzText->render(textX0, textY0, textColor, 0.0f, maxDisplayableMainTextWidth);
	if (isTruncated)
	{
		// Render ellipsis
		iV_DrawEllipsis(wzText.getFontID(), Vector2f(textX0 + maxDisplayableMainTextWidth + 2, textY0), textColor);
	}
}

void OptionsSlider::geometryChanged()
{
	int w = width();
	int h = height();

	if (w == 0 || h == 0)
	{
		return;
	}

	int sliderX0 = horizontalPadding;
	int sliderY0 = (h - sliderWidget->height()) / 2;
	int maxSliderWidth = w - (horizontalPadding * 2);
	int sliderWidth = std::min<int>(maxSliderWidth, cachedIdealSliderWidth);
	sliderWidget->setGeometry(sliderX0, sliderY0, sliderWidth, sliderWidget->height());
}

int32_t OptionsSlider::idealWidth()
{
	return horizontalPadding + cachedIdealSliderWidth + horizontalPadding + cachedIdealTextWidth + horizontalPadding;
}

int32_t OptionsSlider::idealHeight()
{
	return (outerPaddingY * 2) + iV_GetTextLineSize(font_regular);
}

void OptionsSlider::update(bool force)
{
	if (!force && !needsStateUpdates)
	{
		return;
	}

	sliderWidget->pos = (currentValFunc() - minValue) / stepValue;
}

void OptionsSlider::informAvailable(bool isAvailable)
{
	// TODO: Set disabled flag so display can disable the slider appearance?
}

void OptionsSlider::addOnChangeHandler(std::function<void(WIDGET&)> handler)
{
	sliderWidget->addOnChange(handler);
}

// MARK: -

WzOptionsDropdownWidget::WzOptionsDropdownWidget()
{
	if (FrontImages != nullptr)
	{
		setDropdownCaretImage(AtlasImage(FrontImages, IMAGE_CARET_DOWN_FILL), WzSize(12, 12), Padding{0, 8, 0, 0});
	}
	else
	{
		// FUTURE TODO: Different image for in-game options? (For now, no image)
	}
	setStyle(DropdownWidget::DropdownMenuStyle::Separate);
	setListBackgroundColor(pal_RGBA(20,20,20,255));
}

void WzOptionsDropdownWidget::setTextAlignment(WzTextAlignment align)
{
	style &= ~(WLAB_ALIGNLEFT | WLAB_ALIGNCENTRE | WLAB_ALIGNRIGHT);
	style |= align;
}

std::shared_ptr<WIDGET> WzOptionsDropdownWidget::findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	// deliberately skip the default DropdownWidget::findMouseTargetRecursive behavior (which forwards highlighting events to the selected item)
	return WIDGET::findMouseTargetRecursive(psContext, key, wasPressed);
}

void WzOptionsDropdownWidget::display(int xOffset, int yOffset)
{
	if (isOpen() || isHighlighted)
	{
		drawOpenedHighlight(xOffset, yOffset);
	}

	if (!getIsDisabled())
	{
		drawDropdownCaretImage(xOffset, yOffset, WZCOL_TEXT_MEDIUM);
	}
}

void WzOptionsDropdownWidget::geometryChanged()
{
	updateCurrentChoiceDisplayWidgetGeometry();
	DropdownWidget::geometryChanged();
}

void WzOptionsDropdownWidget::drawOpenedHighlight(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();
	int w = width();
	int h = height();

	if (currentOptionChoiceDisplayWidg)
	{
		PIELIGHT backgroundColor;
		if (isHighlighted)
		{
			backgroundColor = WZCOL_MENU_SCORE_BUILT;
		}
		else
		{
			backgroundColor = WZCOL_FORM_DARK;
			backgroundColor.byte.a = std::min<uint8_t>(backgroundColor.byte.a, 60);
		}
		int backX0 = x0;
		int backX1 = backX0 + w;
		if (style & WLAB_ALIGNRIGHT)
		{
			int highlightWidth = currentOptionChoiceDisplayWidg->width() + paddingLeft;
			int itemWidth = getItemDisplayWidth();
			backX0 = (x0 + itemWidth) - highlightWidth;
		}
		int backY0 = y0;
		int backY1 = backY0 + h;
		pie_UniTransBoxFill(backX0, backY0, backX1, backY1, backgroundColor);
	}
}

void WzOptionsDropdownWidget::drawSelectedItem(const std::shared_ptr<WIDGET>& item, const WzRect& displayArea)
{
	// no-op because we handle this with the currentOptionChoiceDisplayWidg
}

void WzOptionsDropdownWidget::highlight(W_CONTEXT *psContext)
{
	isHighlighted = true;
	if (currentOptionChoiceDisplayWidg)
	{
		currentOptionChoiceDisplayWidg->setDisplayHighlighted(true);
	}
}

void WzOptionsDropdownWidget::highlightLost()
{
	isHighlighted = false;
	if (currentOptionChoiceDisplayWidg)
	{
		currentOptionChoiceDisplayWidg->setDisplayHighlighted(false);
	}
}

int WzOptionsDropdownWidget::calculateDropdownListScreenPosX() const
{
	// align with left side of current displayed text label
	return currentOptionChoiceDisplayWidg->screenPosX();
}

int WzOptionsDropdownWidget::calculateDropdownListDisplayWidth() const
{
	return std::max<int>(calculateCurrentUsedWidth(), DropdownWidget::calculateDropdownListDisplayWidth());
}

int WzOptionsDropdownWidget::calculateCurrentUsedWidth() const
{
	int result = getCaretImageUsedWidth();
	if (currentOptionChoiceDisplayWidg)
	{
		result += currentOptionChoiceDisplayWidg->idealWidth();
	}
	return result;
}

void WzOptionsDropdownWidget::onSelectedItemChanged()
{
	auto item = getSelectedItem();
	if (!currentOptionChoiceDisplayWidg)
	{
		currentOptionChoiceDisplayWidg = WzOptionsChoiceWidget::make(font_regular_bold);
		currentOptionChoiceDisplayWidg->setPadding(10, 8);
		currentOptionChoiceDisplayWidg->setTransparentToMouse(true);
		currentOptionChoiceDisplayWidg->setHighlightBackgroundColor(pal_RGBA(0,0,0,0));
		attach(currentOptionChoiceDisplayWidg);
	}
	currentOptionChoiceDisplayWidg->setString(item->getString());
	updateCurrentChoiceDisplayWidgetGeometry();

	DropdownWidget::onSelectedItemChanged();
}

void WzOptionsDropdownWidget::updateCurrentChoiceDisplayWidgetGeometry()
{
	if (currentOptionChoiceDisplayWidg)
	{
		int maxItemWidth = getItemDisplayWidth();
		int itemWidth = std::min<int>(maxItemWidth, currentOptionChoiceDisplayWidg->idealWidth());
		int itemHeight = currentOptionChoiceDisplayWidg->idealHeight();
		int itemX0 = 0;
		if (style & WLAB_ALIGNRIGHT)
		{
			itemX0 = maxItemWidth - itemWidth;
		}
		else
		{
			itemX0 = 0;
		}
		int itemY0 = (height() - itemHeight) / 2;
		currentOptionChoiceDisplayWidg->setGeometry(itemX0, itemY0, itemWidth, itemHeight);
	}
}

int32_t WzOptionsDropdownWidget::idealWidth()
{
	return calculateCurrentUsedWidth() + paddingLeft;
}

int32_t WzOptionsDropdownWidget::idealHeight()
{
	return std::max<int32_t>((currentOptionChoiceDisplayWidg) ? currentOptionChoiceDisplayWidg->idealHeight() : 0, DropdownWidget::idealHeight());
}

// MARK: - WzOptionsChoiceWidget

WzOptionsChoiceWidget::WzOptionsChoiceWidget(iV_fonts desiredFontID)
: FontID(desiredFontID)
{
	highlightBackgroundColor = WZCOL_MENU_SCORE_BUILT;
}

std::shared_ptr<WzOptionsChoiceWidget> WzOptionsChoiceWidget::make(iV_fonts desiredFontID)
{
	class make_shared_enabler: public WzOptionsChoiceWidget {
	public:
		make_shared_enabler(iV_fonts desiredFontID)
		: WzOptionsChoiceWidget(desiredFontID)
		{ }
	};
	auto widget = std::make_shared<make_shared_enabler>(desiredFontID);
	return widget;
}

void WzOptionsChoiceWidget::recalcIconDrawSize()
{
	auto scaledIconSize = scaleIconSizeFor(width(), height());
	iconDrawWidth = scaledIconSize.width();
	iconDrawHeight = scaledIconSize.height();
}

WzSize WzOptionsChoiceWidget::scaleIconSizeFor(optional<int> width, optional<int> height)
{
	WzSize result;
	if (!icon)
	{
		return result;
	}

	int iconMaxWidth = width.value_or(INT_MAX) - (horizontalPadding * 2);
	int iconMaxHeight = height.value_or(INT_MAX) - (verticalPadding * 2);

	result.setWidth(icon->Width);
	result.setHeight(icon->Height);
	if (result.width() <= iconMaxWidth && result.height() <= iconMaxHeight)
	{
		return result;
	}

	float widthScale = 1.f;
	float heightScale = 1.f;
	if (result.width() > iconMaxWidth)
	{
		widthScale = float(iconMaxWidth) / result.width();
	}
	if (result.height() > iconMaxHeight)
	{
		heightScale = float(iconMaxHeight) / result.height();
	}
	float scale = std::min<float>(widthScale, heightScale);
	result.setWidth(static_cast<int>(result.width() * scale));
	result.setHeight(static_cast<int>(result.height() * scale));
	return result;
}

int32_t WzOptionsChoiceWidget::idealWidth()
{
	int result = (horizontalPadding * 2) + wzText.getTextWidth();
	if (icon)
	{
		result += (scaleIconSizeFor(nullopt, idealHeight()).width() + iconPaddingAfter);
	}
	return result;
}

int32_t WzOptionsChoiceWidget::idealHeight()
{
	return (verticalPadding * 2) + wzText.getTextLineSize();
}

void WzOptionsChoiceWidget::setString(WzString string)
{
	text = string;
	wzText.setText(string, FontID); // any time the string is changed, set wzText to the desired string & FontID, so we can calculate the ideal width + height
}

void WzOptionsChoiceWidget::setIcon(const AtlasImageDef *icon_)
{
	icon = icon_;
}

void WzOptionsChoiceWidget::setDisabled(bool value)
{
	isDisabled = value;
}

bool WzOptionsChoiceWidget::getIsDisabled() const
{
	return isDisabled;
}

void WzOptionsChoiceWidget::setDisplayHighlighted(bool highlighted)
{
	forceDisplayHighlight = highlighted;
}

void WzOptionsChoiceWidget::setPadding(int horizontal, int vertical)
{
	horizontalPadding = horizontal;
	verticalPadding = vertical;
}

void WzOptionsChoiceWidget::setTextAlignment(WzTextAlignment align)
{
	style &= ~(WLAB_ALIGNLEFT | WLAB_ALIGNCENTRE | WLAB_ALIGNRIGHT);
	style |= align;
}

void WzOptionsChoiceWidget::setHighlightBackgroundColor(PIELIGHT color)
{
	highlightBackgroundColor = color;
}

std::string WzOptionsChoiceWidget::getTip()
{
	if (!isTruncated) {
		return "";
	}
	return text.toUtf8();
}

WzString WzOptionsChoiceWidget::getString() const
{
	return text;
}

void WzOptionsChoiceWidget::highlight(W_CONTEXT *psContext)
{
	isHighlighted = true;
}

void WzOptionsChoiceWidget::highlightLost()
{
	isHighlighted = false;
}

void WzOptionsChoiceWidget::geometryChanged()
{
	recalcIconDrawSize();
}

void WzOptionsChoiceWidget::run(W_CONTEXT *)
{
	wzText.tick();
}

void WzOptionsChoiceWidget::screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight)
{
	// use as a signal that we should re-check any font scaling or layout calculations (as display scaling setting may have changed)
	if (wzText.getFontID() != FontID)
	{
		wzText.setText(text, FontID);
	}
	wzText.resetCachedDimensions();
}

void WzOptionsChoiceWidget::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int w = width();
	int h = height();

	bool isHighlight = !isDisabled && (isHighlighted || forceDisplayHighlight);

	iV_fonts fontID = wzText.getFontID();
	if (fontID == font_count || lastWidgetWidth != width() || wzText.getText() != text)
	{
		fontID = FontID;
		wzText.setText(text, fontID);
	}

	int availableButtonTextWidth = w - (horizontalPadding * 2);
	if (iconDrawWidth > 0)
	{
		availableButtonTextWidth -= (iconDrawWidth + iconPaddingAfter);
	}
	if (wzText->width() > availableButtonTextWidth)
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
			wzText.setText(text, fontID);
		} while (wzText->width() > availableButtonTextWidth);
	}
	lastWidgetWidth = width();

	// Draw background (if highlighted)
	if (isHighlight && !highlightBackgroundColor.isTransparent())
	{
		pie_UniTransBoxFill(x0, y0, x0 + w, y0 + h, highlightBackgroundColor);
	}

	// Draw the main text
	PIELIGHT textColor = WZCOL_TEXT_BRIGHT;
	if (isDisabled)
	{
		textColor.byte.a = (textColor.byte.a / 2);
	}

	int maxDisplayableMainTextWidth = availableButtonTextWidth;

	int textX0 = 0;
	if (style & WLAB_ALIGNRIGHT)
	{
		textX0 = x0 + w - horizontalPadding - std::min(maxDisplayableMainTextWidth, wzText->width());
	}
	else
	{
		textX0 = x0 + horizontalPadding;
		if (iconDrawWidth > 0)
		{
			textX0 += (iconDrawWidth + iconPaddingAfter);
		}
	}
	int textY0 = y0 + (h - wzText->lineSize()) / 2 - wzText->aboveBase();

	isTruncated = maxDisplayableMainTextWidth < wzText->width();
	if (isTruncated)
	{
		maxDisplayableMainTextWidth -= (iV_GetEllipsisWidth(wzText.getFontID()) + 2);
	}
	wzText->render(textX0, textY0, textColor, 0.0f, maxDisplayableMainTextWidth);
	if (isTruncated)
	{
		// Render ellipsis
		iV_DrawEllipsis(wzText.getFontID(), Vector2f(textX0 + maxDisplayableMainTextWidth + 2, textY0), textColor);
	}

	if (icon)
	{
		int iconX0 = x0 + horizontalPadding;
		if (style & WLAB_ALIGNRIGHT)
		{
			iconX0 = x0 + w - horizontalPadding - std::min(availableButtonTextWidth, wzText->width()) - (iconDrawWidth + iconPaddingAfter);
		}
		int iconY0 = y0 + (h - iconDrawHeight) / 2;
		iV_DrawImage2(icon, iconX0, iconY0, iconDrawWidth, iconDrawHeight);
	}
}

typedef WzOptionsChoiceWidget WzOptionsLabelWidget;

// MARK: - WzClickableOptionsChoiceWidget

WzClickableOptionsChoiceWidget::WzClickableOptionsChoiceWidget(iV_fonts desiredFontID)
: WzOptionsChoiceWidget(desiredFontID)
{ }

std::shared_ptr<WzClickableOptionsChoiceWidget> WzClickableOptionsChoiceWidget::make(iV_fonts desiredFontID)
{
	class make_shared_enabler: public WzClickableOptionsChoiceWidget {
	public:
		make_shared_enabler(iV_fonts desiredFontID)
		: WzClickableOptionsChoiceWidget(desiredFontID)
		{ }
	};
	auto widget = std::make_shared<make_shared_enabler>(desiredFontID);
	return widget;
}

void WzClickableOptionsChoiceWidget::addOnClickHandler(const OnClickHandler& onClickFunc)
{
	onClickHandlers.push_back(onClickFunc);
}

void WzClickableOptionsChoiceWidget::clicked(W_CONTEXT *, WIDGET_KEY key)
{
	if (getIsDisabled())
	{
		return;
	}

	if (key == WKEY_PRIMARY || key == WKEY_SECONDARY)
	{
		mouseDownOnWidget = true;
	}
}

/* Respond to a mouse button up */
void WzClickableOptionsChoiceWidget::released(W_CONTEXT *, WIDGET_KEY key)
{
	if (getIsDisabled())
	{
		mouseDownOnWidget = false;
		return;
	}

	if (key == WKEY_PRIMARY || key == WKEY_SECONDARY)
	{
		if (mouseDownOnWidget)
		{
			/* Call all onClick event handlers */
			for (auto it = onClickHandlers.begin(); it != onClickHandlers.end(); it++)
			{
				auto onClickHandler = *it;
				if (onClickHandler)
				{
					onClickHandler(*this, key);
				}
			}
			mouseDownOnWidget = false;
		}
	}
}

/* Respond to a mouse moving over a button */
void WzClickableOptionsChoiceWidget::highlight(W_CONTEXT *psContext)
{
	WzOptionsChoiceWidget::highlight(psContext);
}

/* Respond to the mouse moving off a button */
void WzClickableOptionsChoiceWidget::highlightLost()
{
	mouseDownOnWidget = false;
	WzOptionsChoiceWidget::highlightLost();
}

// MARK: - OptionValueChangerInterface

OptionValueChangerInterface::~OptionValueChangerInterface()
{ }

// MARK: - OptionInfo

OptionInfo::OptionInfo(const WzString& optionId, const WzString& displayName, const WzString& helpDescription)
: optionId(optionId)
, displayName(displayName)
, helpDescription(helpDescription)
{ }

OptionInfo& OptionInfo::addAvailabilityCondition(const OptionAvailabilityCondition& condition)
{
	availabilityConditions.push_back(condition);
	return *this;
}

OptionInfo& OptionInfo::setRequiresRestart(bool val)
{
	bRequiresRestart = val;
	return *this;
}

WzString OptionInfo::getTranslatedDisplayName() const
{
	if (displayName.isEmpty()) { return {}; }
	return gettext(displayName.toUtf8().c_str());
}

WzString OptionInfo::getTranslatedHelpDescription() const
{
	if (helpDescription.isEmpty()) { return {}; }
	return gettext(helpDescription.toUtf8().c_str());
}

bool OptionInfo::isAvailable() const
{
	return std::all_of(availabilityConditions.begin(), availabilityConditions.end(), [&](const OptionAvailabilityCondition& condition) -> bool {
		return condition(*this).available;
	});
}

std::vector<OptionInfo::AvailabilityResult> OptionInfo::getAvailabilityResults() const
{
	std::vector<AvailabilityResult> result;
	result.reserve(availabilityConditions.size());
	for (const auto& condition : availabilityConditions)
	{
		result.push_back(condition(*this));
	}
	return result;
}

bool OptionInfo::requiresRestart() const
{
	return bRequiresRestart;
}

// MARK: - OptionAvailabilityConditions

OptionInfo::AvailabilityResult IsNotInGame(const OptionInfo&)
{
	OptionInfo::AvailabilityResult result;
	result.available = GetGameMode() != GS_NORMAL;
	result.localizedUnavailabilityReason = _("Not editable while in a game.");
	return result;
}

OptionInfo::AvailabilityResult IsNotMultiplayer(const OptionInfo&)
{
	OptionInfo::AvailabilityResult result;
	const bool bIsTrueMultiplayerGame = bMultiPlayer && NetPlay.bComms;
	result.available = !bIsTrueMultiplayerGame;
	result.localizedUnavailabilityReason = _("Not editable while in a multiplayer game.");
	return result;
}

// MARK: - OptionsSection

OptionsSection::OptionsSection(const WzString& displayName, const WzString& helpDescription)
: displayName(displayName)
, helpDescription(helpDescription)
{ }

const WzString& OptionsSection::sectionId() const
{
	// just use the original (base language) displayName as the sectionId
	return displayName;
}

WzString OptionsSection::getTranslatedDisplayName() const
{
	if (displayName.isEmpty()) { return {}; }
	return gettext(displayName.toUtf8().c_str());
}

WzString OptionsSection::getTranslatedHelpDescription() const
{
	if (helpDescription.isEmpty()) { return {}; }
	return gettext(helpDescription.toUtf8().c_str());
}

// MARK: - OptionsValueChangerWidgetWrapper

class OptionsValueChangerWidgetWrapper : public W_FORM
{
public:
	static std::shared_ptr<OptionsValueChangerWidgetWrapper> wrap(const std::shared_ptr<WIDGET>& optionValueChangerWidget);

	int32_t idealWidth() override;
	int32_t idealHeight() override;

	void enable(bool value = true);
	void disable();

	std::shared_ptr<WIDGET> getOptionValueChangerWidget() const { return optionValueChangerWidget; };

protected:
	void display(int xOffset, int yOffset) override;
	void displayRecursive(WidgetGraphicsContext const &context) override; // for handling disabled overlay
	void geometryChanged() override;
private:
	std::shared_ptr<WIDGET> optionValueChangerWidget;
};

std::shared_ptr<OptionsValueChangerWidgetWrapper> OptionsValueChangerWidgetWrapper::wrap(const std::shared_ptr<WIDGET>& optionValueChangerWidget)
{
	auto result = std::make_shared<OptionsValueChangerWidgetWrapper>();
	result->optionValueChangerWidget = optionValueChangerWidget;
	result->attach(optionValueChangerWidget);
	return result;
}

int32_t OptionsValueChangerWidgetWrapper::idealWidth()
{
	return optionValueChangerWidget->idealWidth();
}

int32_t OptionsValueChangerWidgetWrapper::idealHeight()
{
	return optionValueChangerWidget->idealHeight();
}

void OptionsValueChangerWidgetWrapper::geometryChanged()
{
	// The optionValueChangerWidget should be right-aligned inside the wrapper, and a maximum width of its idealWidth()
	auto w = width();
	auto h = height();
	auto idealWidgetWidth = optionValueChangerWidget->idealWidth();

	int widgX0 = (idealWidgetWidth < w) ? (w - idealWidgetWidth) : 0;
	int widgY0 = 0;
	int widgWidth = std::min(idealWidgetWidth, w);
	int widgHeight = h;
	optionValueChangerWidget->setGeometry(widgX0, widgY0, widgWidth, widgHeight);
}

void OptionsValueChangerWidgetWrapper::display(int xOffset, int yOffset)
{
	// no-op
}

void OptionsValueChangerWidgetWrapper::displayRecursive(WidgetGraphicsContext const &context)
{
	// call parent displayRecursive
	W_FORM::displayRecursive(context);

	if (disableChildren)
	{
		// when children are disabled on a W_FORM or subclass, WIDGET::displayRecursive does not actually draw children - but in this case we actually want to
		auto childrenContext = context.translatedBy(x(), y()).setAllowChildDisplayRecursiveIfSelfClipped(false);
		// Display the widgets on this widget.
		// NOTE: Draw them in the opposite order that clicks are processed, so behavior matches the visual.
		// i.e. Since findMouseTargetRecursive handles processing children in decreasing z-order (i.e. "top-down")
		//      we want to draw things in list order (bottom-up) so the "top-most" is drawn last
		for (auto const &child: children())
		{
			if (child->visible())
			{
				child->displayRecursive(childrenContext);
			}
		}

		bool widgetIsClipped = !context.clipContains(geometry());
		if (!widgetIsClipped)
		{
			// "over-draw" with the disabled color over the contained changer widget
			int x0 = context.getXOffset() + x();
			int y0 = context.getYOffset() + y();
			int widgX0 = x0 + optionValueChangerWidget->x();
			int widgY0 = y0 + optionValueChangerWidget->y();
			int widgX1 = widgX0 + optionValueChangerWidget->width();
			int widgY1 = widgY0 + optionValueChangerWidget->height();
			pie_UniTransBoxFill(widgX0, widgY0, widgX1, widgY1, WZCOL_TRANSPARENT_BOX);
		}
	}
}

void OptionsValueChangerWidgetWrapper::enable(bool enableValue)
{
	disableChildren = !enableValue;
}

void OptionsValueChangerWidgetWrapper::disable()
{
	disableChildren = true; // skips processing clicks / mouse for all children
}

// MARK: - Options Row Widgets

class OptionsRowWidgetBase: public WIDGET
{
public:
	virtual void setIsHighlighted(bool val) = 0;
};

class OptionsPaddingRow: public OptionsRowWidgetBase
{
public:
	static std::shared_ptr<OptionsPaddingRow> make(int paddingHeight);

	void setIsHighlighted(bool val) override;

	int32_t idealWidth() override;
	int32_t idealHeight() override;
protected:
	void display(int xOffset, int yOffset) override;
private:
	int32_t paddingHeight = 0;
};

std::shared_ptr<OptionsPaddingRow> OptionsPaddingRow::make(int paddingHeight)
{
	class make_shared_enabler: public OptionsPaddingRow {};
	auto result = std::make_shared<make_shared_enabler>();
	result->paddingHeight = paddingHeight;
	result->setGeometry(0, 0, result->idealWidth(), result->idealHeight());
	return result;
}

void OptionsPaddingRow::setIsHighlighted(bool)
{
	// no-op
}

int32_t OptionsPaddingRow::idealWidth()
{
	return 100;
}

int32_t OptionsPaddingRow::idealHeight()
{
	return paddingHeight;
}

void OptionsPaddingRow::display(int xOffset, int yOffset)
{
	// no-op
}

// MARK: OptionsSectionRow

class OptionsSectionRow: public OptionsRowWidgetBase
{
public:
	static std::shared_ptr<OptionsSectionRow> make(const WzString& sectionName);

	void setIsHighlighted(bool val) override;
	void setShowBottomBorder(bool show) { showBottomBorder = show; }
	void setSectionName(const WzString& sectionName);

	int32_t idealWidth() override;
	int32_t idealHeight() override;
	void geometryChanged() override;

	void flashHighlight();
protected:
	void display(int xOffset, int yOffset) override;
private:
	std::shared_ptr<W_LABEL> m_labelWidget;
	bool showBottomBorder = false;
	optional<uint32_t> flashHighlightStart = nullopt;
	const int32_t outerPaddingX = 1;
	const int32_t outerPaddingY = 8;
	const int32_t innerPaddingX = 4;
};

std::shared_ptr<OptionsSectionRow> OptionsSectionRow::make(const WzString& sectionName)
{
	class make_shared_enabler: public OptionsSectionRow {};
	auto result = std::make_shared<make_shared_enabler>();

	result->m_labelWidget = std::make_shared<W_LABEL>();
	result->m_labelWidget->setFont(font_regular_bold, WZCOL_TEXT_MEDIUM);
	result->m_labelWidget->setString(sectionName);
	result->m_labelWidget->setTextAlignment(WLAB_ALIGNLEFT);
	result->m_labelWidget->setGeometry(0, 0, result->m_labelWidget->idealWidth(), result->m_labelWidget->idealHeight());
	result->attach(result->m_labelWidget);

	result->setGeometry(0, 0, result->idealWidth(), result->idealHeight());

	return result;
}

void OptionsSectionRow::setIsHighlighted(bool)
{
	// no-op
}

void OptionsSectionRow::setSectionName(const WzString& sectionName)
{
	m_labelWidget->setString(sectionName);
}

int32_t OptionsSectionRow::idealWidth()
{
	return (outerPaddingX * 2) + (innerPaddingX * 2) + m_labelWidget->idealWidth();
}

int32_t OptionsSectionRow::idealHeight()
{
	return (outerPaddingY * 2) + m_labelWidget->idealHeight();
}

void OptionsSectionRow::geometryChanged()
{
	m_labelWidget->setGeometry(outerPaddingX + innerPaddingX, outerPaddingY, width() - (outerPaddingX * 2) - (innerPaddingX * 2), height() - (outerPaddingY * 2));
}

void OptionsSectionRow::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();
	int x1 = x0 + width();
	int y1 = y0 + height();

	if (flashHighlightStart.has_value())
	{
		auto delta = realTime - flashHighlightStart.value();
		if (delta < 500)
		{
			PIELIGHT flashColor = WZCOL_MENU_SCORE_BUILT;
			uint8_t alphaValue = 180 - static_cast<uint8_t>((float(delta) / 500.f) * 180);
			flashColor.byte.a = alphaValue;
			pie_UniTransBoxFill(x0, y0, x1 - 1, y1 - ((showBottomBorder) ? 1 : 0), flashColor);
		}
		else
		{
			flashHighlightStart.reset();
		}
	}

	if (showBottomBorder)
	{
		int bottomBorderX0 = x0 + outerPaddingX;
		int bottomBorderX1 = x1 - outerPaddingX;
		int bottomBorderY0 = y1 - 1;
		int bottomBorderY1 = bottomBorderY0 + 1;
		PIELIGHT lineColor = WZCOL_TEXT_MEDIUM;
		lineColor.byte.a = (lineColor.byte.a / 2);
		pie_UniTransBoxFill(bottomBorderX0, bottomBorderY0, bottomBorderX1, bottomBorderY1, lineColor);
	}
}

void OptionsSectionRow::flashHighlight()
{
	flashHighlightStart = realTime;
}

// MARK: OptionRow

class OptionRow: public OptionsRowWidgetBase
{
protected:
	OptionRow() { }
public:
	static std::shared_ptr<OptionRow> make(const WzString& optionId, const std::shared_ptr<WzOptionsLabelWidget>& labelWidget, const std::shared_ptr<OptionsValueChangerWidgetWrapper>& wrappedOptionValueChangerWidget, int16_t indentLevel = 0);

	void setParentOptionsForm(const std::shared_ptr<OptionsForm>& parentOptionsForm);
	void setIsHighlighted(bool val) override;
	void setShowBottomBorder(bool show) { showBottomBorder = show; }
	void flashHighlight();

	int32_t idealWidth() override;
	int32_t idealHeight() override;
	void geometryChanged() override;

	const WzString& optionId() const { return m_optionId; }
	std::shared_ptr<WzOptionsLabelWidget> getLabelWidget() const { return m_labelWidget; }
	std::shared_ptr<OptionsValueChangerWidgetWrapper> getWrappedOptionValueChangerWidget() const { return m_wrappedOptionValueChangerWidget; }

protected:
	void display(int xOffset, int yOffset) override;
private:
	inline int32_t leftIndent() { return static_cast<int32_t>(m_indentLevel) * indentPaddingPerLevel; }
private:
	WzString m_optionId;
	std::shared_ptr<WzOptionsLabelWidget> m_labelWidget;
	std::shared_ptr<OptionsValueChangerWidgetWrapper> m_wrappedOptionValueChangerWidget;
	std::weak_ptr<OptionsForm> m_weakParentForm;
	int16_t m_indentLevel = 0;
	optional<uint32_t> flashHighlightStart = nullopt;
	bool showBottomBorder = false;
	bool isHighlighted = false;
	const int32_t outerPaddingX = 1;
	const int32_t outerPaddingY = 0;
	const int32_t innerPaddingX = 6;
	const int32_t indentPaddingPerLevel = 10;
	const int32_t minimumLabelWidth = 100;
};

std::shared_ptr<OptionRow> OptionRow::make(const WzString& optionId, const std::shared_ptr<WzOptionsLabelWidget>& labelWidget, const std::shared_ptr<OptionsValueChangerWidgetWrapper>& wrappedOptionValueChangerWidget, int16_t indentLevel)
{
	class make_shared_enabler: public OptionRow {};
	auto result = std::make_shared<make_shared_enabler>();
	result->m_optionId = optionId;
	result->m_indentLevel = indentLevel;

	result->m_labelWidget = labelWidget;
	result->attach(result->m_labelWidget);

	result->m_wrappedOptionValueChangerWidget = wrappedOptionValueChangerWidget;
	result->attach(result->m_wrappedOptionValueChangerWidget);

	return result;
}

int32_t OptionRow::idealWidth()
{
	return (outerPaddingX * 2) + leftIndent() + std::max<int>(m_labelWidget->idealWidth(), minimumLabelWidth) + innerPaddingX + m_wrappedOptionValueChangerWidget->idealWidth();
}

int32_t OptionRow::idealHeight()
{
	return (outerPaddingY * 2) + std::max<int32_t>(m_labelWidget->idealHeight(), m_wrappedOptionValueChangerWidget->idealHeight()) + (showBottomBorder ? 1 : 0);
}

void OptionRow::geometryChanged()
{
	int32_t w = width();
	int32_t h = height();

	if (w == 0 || h == 0)
	{
		return;
	}

	int32_t leftIndentWidth = leftIndent();
	int32_t availableWidthForWidgets = w - (outerPaddingX * 2) - leftIndentWidth - innerPaddingX;
	int32_t availableWidgetHeight = h - (outerPaddingY * 2) - (showBottomBorder ? 1 : 0);

	int32_t labelWidth = m_labelWidget->idealWidth();
	int32_t valueChangerWidth = m_wrappedOptionValueChangerWidget->idealWidth();

	if (labelWidth + valueChangerWidth > availableWidthForWidgets)
	{
		// Must shrink label widget below its ideal width
		int32_t shrinkByTotalWidth = (labelWidth + valueChangerWidth) - availableWidthForWidgets;
		int32_t shrinkLabelByWidth = std::min<int32_t>(shrinkByTotalWidth, (labelWidth > minimumLabelWidth) ? labelWidth - minimumLabelWidth : 0);
		labelWidth -= shrinkLabelByWidth;
		shrinkByTotalWidth -= shrinkLabelByWidth;
		if (shrinkByTotalWidth > 0)
		{
			// Must also shrink the valueChangerWidget?
			valueChangerWidth -= shrinkByTotalWidth;
			if (valueChangerWidth < minimumLabelWidth)
			{
				valueChangerWidth = minimumLabelWidth;
				debug(LOG_WZ, "Dimensions too small");
			}
		}
	}

	// label is left-aligned, with left padding based on indentLevel
	int32_t labelX0 = outerPaddingX + leftIndentWidth;
	int32_t labelHeight = std::min<int32_t>(m_labelWidget->idealHeight(), availableWidgetHeight);
	int32_t labelY0 = outerPaddingY + ((availableWidgetHeight - labelHeight) / 2);
	m_labelWidget->setGeometry(labelX0, labelY0, labelWidth, labelHeight);

	// valueChanger is right-aligned
	int32_t valueChangerX0 = w - outerPaddingX - valueChangerWidth;
	int32_t valueChangerHeight = std::min<int32_t>(m_wrappedOptionValueChangerWidget->idealHeight(), availableWidgetHeight);
	int32_t valueChangerY0 = outerPaddingY + ((availableWidgetHeight - valueChangerHeight) / 2);
	m_wrappedOptionValueChangerWidget->setGeometry(valueChangerX0, valueChangerY0, valueChangerWidth, valueChangerHeight);
}

void OptionRow::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();
	int x1 = x0 + width();
	int y1 = y0 + height();

	if (isHighlighted)
	{
		PIELIGHT highlightColor = WZCOL_MENU_SCORE_BUILT;
		highlightColor.byte.a = highlightColor.byte.a / 2;
		pie_UniTransBoxFill(x0, y0, x1 - 1, y1 - ((showBottomBorder) ? 1 : 0), highlightColor);
	}

	if (flashHighlightStart.has_value())
	{
		auto delta = realTime - flashHighlightStart.value();
		if (delta < 500)
		{
			PIELIGHT flashColor = pal_RGBA(16, 150, 2, 255);
			uint8_t alphaValue = 180 - static_cast<uint8_t>((float(delta) / 500.f) * 180);
			flashColor.byte.a = alphaValue;
			pie_UniTransBoxFill(x0, y0, x1 - 1, y1 - ((showBottomBorder) ? 1 : 0), flashColor);
		}
		else
		{
			flashHighlightStart.reset();
		}
	}

	if (showBottomBorder)
	{
		int bottomBorderX0 = x0 + outerPaddingX;
		int bottomBorderX1 = x1 - outerPaddingX;
		int bottomBorderY0 = y1 - 1;
		int bottomBorderY1 = bottomBorderY0 + 1;
		PIELIGHT lineColor = WZCOL_TEXT_MEDIUM;
		lineColor.byte.a = (lineColor.byte.a / 2);
		pie_UniTransBoxFill(bottomBorderX0, bottomBorderY0, bottomBorderX1, bottomBorderY1, lineColor);
	}
}

void OptionRow::setParentOptionsForm(const std::shared_ptr<OptionsForm>& parentOptionsForm)
{
	m_weakParentForm = parentOptionsForm;
}

void OptionRow::setIsHighlighted(bool newValue)
{
	isHighlighted = newValue;
	m_labelWidget->setDisplayHighlighted(isHighlighted);
}

void OptionRow::flashHighlight()
{
	flashHighlightStart = realTime;
}

// MARK: - OptionsForm

std::shared_ptr<OptionsForm> OptionsForm::make()
{
	class make_shared_enabler : public OptionsForm { };
	auto result = std::make_shared<make_shared_enabler>();
	result->initialize();
	return result;
}

OptionsForm::OptionsForm()
{ }

OptionsForm::~OptionsForm()
{
	if (currentHelpPopoverWidget)
	{
		currentHelpPopoverWidget->close();
		currentHelpPopoverWidget.reset();
	}
}

void OptionsForm::initialize()
{
	optionsList = ScrollableListWidget::make();
	optionsList->setPadding({1, 0, 1, 0});
	optionsList->setItemSpacing(0);
	attach(optionsList);
}

int32_t OptionsForm::idealWidth()
{
	return optionsList->idealWidth();
}

int32_t OptionsForm::idealHeight()
{
	return optionsList->idealHeight();
}

class WzHelpPopoverWidget : public WIDGET
{
protected:
	WzHelpPopoverWidget();
	bool initialize(const OptionInfo& optionInfo, const std::vector<OptionChoiceHelpDescription>& choiceHelpDescriptions, int32_t paragraphWidth);
public:
	static std::shared_ptr<WzHelpPopoverWidget> make(const OptionInfo& optionInfo, const std::vector<OptionChoiceHelpDescription>& choiceHelpDescriptions, int32_t paragraphWidth);
	int32_t idealWidth() override;
	int32_t idealHeight() override;
protected:
	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;
private:
	std::shared_ptr<Paragraph> paragraph;
	const int32_t outerPadding = 10;
};

std::shared_ptr<WzHelpPopoverWidget> WzHelpPopoverWidget::make(const OptionInfo& optionInfo, const std::vector<OptionChoiceHelpDescription>& choiceHelpDescriptions, int32_t paragraphWidth)
{
	class make_shared_enabler : public WzHelpPopoverWidget { };
	auto result = std::make_shared<make_shared_enabler>();
	if (!result->initialize(optionInfo, choiceHelpDescriptions, paragraphWidth))
	{
		return nullptr;
	}
	result->setGeometry(0, 0, result->idealWidth(), result->idealHeight());
	return result;
}

WzHelpPopoverWidget::WzHelpPopoverWidget()
{ }

bool WzHelpPopoverWidget::initialize(const OptionInfo& optionInfo, const std::vector<OptionChoiceHelpDescription>& choiceHelpDescriptions, int32_t paragraphWidth)
{
	auto availabilityResults = optionInfo.getAvailabilityResults();
	bool optionIsAvailable = std::all_of(availabilityResults.begin(), availabilityResults.end(), [](const OptionInfo::AvailabilityResult& result) -> bool {
		return result.available;
	});

	// Build description paragraph
	paragraph = std::make_shared<Paragraph>();
	bool wroteALine = false;
	paragraph->setLineSpacing(1);

	// Add general option info text description
	paragraph->setFont(font_small);
	paragraph->setFontColour(WZCOL_TEXT_BRIGHT);
	WzString optionHelpDescription = optionInfo.getTranslatedHelpDescription();
	if (!optionHelpDescription.isEmpty())
	{
		paragraph->addText(optionHelpDescription);
		wroteALine = true;
	}

	if (optionIsAvailable)
	{
		for (const auto& choiceHelpDesc : choiceHelpDescriptions)
		{
			if (choiceHelpDesc.helpDescription.isEmpty())
			{
				continue;
			}

			if (wroteALine)
			{
				paragraph->addText("\n \n");
			}

			// Add a bold string with the displayName and then append the choice help description
			if (!choiceHelpDesc.displayName.isEmpty())
			{
				paragraph->setFont(font_bar); // font_small_bold
				paragraph->addText(choiceHelpDesc.displayName + ": ");
			}
			paragraph->setFont(font_small);
			paragraph->addText(choiceHelpDesc.helpDescription);

			wroteALine = true;
		}

		if (optionInfo.requiresRestart())
		{
			if (wroteALine)
			{
				paragraph->addText("\n \n");
			}

			paragraph->setFont(font_bar); // font_small_bold
			paragraph->addText(WzString("* ") + _("Takes effect on game restart"));
			wroteALine = true;

			paragraph->setFont(font_small);
		}
	}
	else
	{
		for (const auto& result : availabilityResults)
		{
			if (result.available)
			{
				continue;
			}

			if (wroteALine)
			{
				paragraph->addText("\n \n");
			}

			// Add a bold string with the
			if (!result.localizedUnavailabilityReason.isEmpty())
			{
				paragraph->setFont(font_bar); // font_small_bold
				paragraph->addText(result.localizedUnavailabilityReason);
				wroteALine = true;
			}
		}
	}

	if (!wroteALine)
	{
		return false;
	}

	attach(paragraph);
	paragraph->setGeometry(0, 0, paragraphWidth, 100); // paragraph calculates actual height based on width

	return true;
}

int32_t WzHelpPopoverWidget::idealWidth()
{
	return (outerPadding * 2) + paragraph->width();
}

int32_t WzHelpPopoverWidget::idealHeight()
{
	return (outerPadding * 2) + paragraph->height();
}

void WzHelpPopoverWidget::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();
	// display background
	pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), pal_RGBA(20, 20, 20, 200));
}

void WzHelpPopoverWidget::geometryChanged()
{
	int w = width();
	int h = height();

	int desiredParagraphWidth = w - (outerPadding * 2);
	int maxParagraphHeight = h - (outerPadding * 2);
	paragraph->setGeometry(outerPadding, outerPadding, desiredParagraphWidth, maxParagraphHeight);
}

void OptionsForm::openHelpPopover(const OptionInfo& optionInfo, const std::vector<OptionChoiceHelpDescription>& choiceHelpDescriptions, const std::shared_ptr<WIDGET>& parent)
{
	closeHelpPopover();

	int32_t paragraphWidth = std::min<int32_t>(240, std::max<int32_t>(width() - 100, 240));
	auto helpWidget = WzHelpPopoverWidget::make(optionInfo, choiceHelpDescriptions, paragraphWidth);
	if (!helpWidget)
	{
		// nothing to display
		return;
	}

	// Then open a popover with the paragraph as the widget
	currentHelpPopoverWidget = PopoverWidget::makePopover(parent, helpWidget, PopoverWidget::Style::NonInteractive, PopoverWidget::Alignment::RightOfParent, Vector2i(0, 10));
}

void OptionsForm::closeHelpPopover()
{
	if (!currentHelpPopoverWidget)
	{
		return;
	}
	currentHelpPopoverWidget->close();
	currentHelpPopoverWidget.reset();
}

void OptionsForm::handleStartedRowEditing(const WzString& optionId)
{
	closeHelpPopover();

	auto it = optionDetailsMap.find(optionId);
	ASSERT_OR_RETURN(, it != optionDetailsMap.end(), "Could not find OptionRow for: %s", optionId.toUtf8().c_str());
	auto row = std::dynamic_pointer_cast<OptionRow>(optionsList->getItemAtIdx(it->second.idxInOptionsList));
	ASSERT_OR_RETURN(, row != nullptr, "Could not find OptionRow for: %s", optionId.toUtf8().c_str());

	capturedEditingRowWidget = row;
}

void OptionsForm::handleEndedRowEditing(const WzString& optionId)
{
	auto it = optionDetailsMap.find(optionId);
	ASSERT_OR_RETURN(, it != optionDetailsMap.end(), "Could not find OptionRow for: %s", optionId.toUtf8().c_str());
	auto row = std::dynamic_pointer_cast<OptionRow>(optionsList->getItemAtIdx(it->second.idxInOptionsList));
	ASSERT_OR_RETURN(, row != nullptr, "Could not find OptionRow for: %s", optionId.toUtf8().c_str());

	if (row != capturedEditingRowWidget)
	{
		debug(LOG_INFO, "Ignorning call for non-current captured editing row widget: %s", optionId.toUtf8().c_str());
	}

	capturedEditingRowWidget = nullptr;
}

void OptionsForm::display(int xOffset, int yOffset)
{
	// no-op
}

bool OptionsForm::hasOptionsThatRequireRestart() const
{
	for (auto& i : optionDetailsMap)
	{
		if (i.second.info.requiresRestart())
		{
			return true;
		}
	}
	return false;
}

bool OptionsForm::jumpToSectionId(const WzString& sectionId)
{
	auto it = sectionDetailsMap.find(sectionId);
	if (it == sectionDetailsMap.end())
	{
		return false;
	}
	auto idx = it->second.idxInOptionsList;
	optionsList->scrollToItem(idx);
	if (auto sectionRow = std::dynamic_pointer_cast<OptionsSectionRow>(optionsList->getItemAtIdx(idx)))
	{
		sectionRow->flashHighlight();
	}
	return true;
}

bool OptionsForm::jumpToOptionId(const WzString& optionId)
{
	auto it = optionDetailsMap.find(optionId);
	if (it == optionDetailsMap.end())
	{
		return false;
	}
	jumpToItemIdx(it->second.idxInOptionsList, true);
	return true;
}

size_t OptionsForm::numSections() const
{
	return optionDetailsMap.size();
}

std::vector<OptionsSection> OptionsForm::getSectionInfos() const
{
	std::vector<OptionsSection> result;
	for (const auto& sectionId : orderedSectionIds)
	{
		auto it = sectionDetailsMap.find(sectionId);
		if (it == sectionDetailsMap.end())
		{
			continue;
		}
		result.push_back(it->second.info);
	}
	return result;
}

void OptionsForm::setMaxListWidth(int32_t maxListWidth_)
{
	maxListWidth = maxListWidth_;
	geometryChanged();
}

void OptionsForm::setRefreshOptionsOnScreenSizeChange(bool enabled)
{
	refreshOptionsOnScreenSizeChange = enabled;
}

bool OptionsForm::jumpToItemIdx(size_t idx, bool flash)
{
	optionsList->scrollEnsureItemVisible(idx);
	if (flash)
	{
		if (auto row = std::dynamic_pointer_cast<OptionRow>(optionsList->getItemAtIdx(idx)))
		{
			row->flashHighlight();
		}
	}
	return true;
}

void OptionsForm::addSection(const OptionsSection& optionsSection, bool bottomBorder /*= false*/)
{
	if (optionsList->numItems() > 0)
	{
		// Add "padding" row above the section row
		optionsList->addItem(OptionsPaddingRow::make(20));
	}

	auto optionsSectionWidget = OptionsSectionRow::make(optionsSection.getTranslatedDisplayName());
	optionsSectionWidget->setShowBottomBorder(bottomBorder);

	// Store mapping
	auto insertResult = sectionDetailsMap.insert({optionsSection.sectionId(), {optionsSection, optionsSectionWidget, 0}});
	ASSERT_OR_RETURN(, insertResult.second, "Option section already added to OptionsForm: %s", optionsSection.sectionId().toUtf8().c_str());
	orderedSectionIds.push_back(optionsSection.sectionId());

	// Add options section widget to list
	size_t itemListIdx = optionsList->addItem(optionsSectionWidget);
	insertResult.first->second.idxInOptionsList = itemListIdx;
}

static WzString getOptionInfoTranslatedDisplayName(const OptionInfo& optionInfo)
{
	WzString result = optionInfo.getTranslatedDisplayName();
	if (optionInfo.requiresRestart())
	{
		result.append("*");
	}
	return result;
}

void OptionsForm::addOptionInternal(const OptionInfo& optionInfo, const std::shared_ptr<WIDGET>& optionValueChangerWidget, bool bottomBorder /*= false*/, int16_t indentLevel /*= 0*/)
{
	auto pWeakOptionsForm = std::weak_ptr<OptionsForm>(std::dynamic_pointer_cast<OptionsForm>(shared_from_this()));

	// Create left side widget for displaying the Option label / display name
	auto optionLabelWidget = WzOptionsLabelWidget::make(font_regular);
	optionLabelWidget->setString(getOptionInfoTranslatedDisplayName(optionInfo));
	optionLabelWidget->setPadding(4, 8);
	optionLabelWidget->setTextAlignment(WLAB_ALIGNLEFT);
	optionLabelWidget->setGeometry(0, 0, optionLabelWidget->idealWidth(), optionLabelWidget->idealHeight());
	optionLabelWidget->setHighlightBackgroundColor(pal_RGBA(0,0,0,0));

	// Add an onChangeHandler to the optionValueChangerWidget
	auto pOptionValueChangerInterface = std::dynamic_pointer_cast<OptionValueChangerInterface>(optionValueChangerWidget);
	ASSERT_OR_RETURN(, pOptionValueChangerInterface != nullptr, "optionValueChangerWidget does not implement appropriate interface");
	WzString optionIdCopy = optionInfo.optionId;
	pOptionValueChangerInterface->addOnChangeHandler([pWeakOptionsForm, optionIdCopy](WIDGET&) {
		auto strongOptionsForm = pWeakOptionsForm.lock();
		ASSERT_OR_RETURN(, strongOptionsForm != nullptr, "Invalid pointer");
		strongOptionsForm->handleOptionValueChanged(optionIdCopy);
	});
	pOptionValueChangerInterface->addStartingCapturedEditingHandler([pWeakOptionsForm, optionIdCopy](WIDGET&) {
		auto strongOptionsForm = pWeakOptionsForm.lock();
		ASSERT_OR_RETURN(, strongOptionsForm != nullptr, "Invalid pointer");
		strongOptionsForm->handleStartedRowEditing(optionIdCopy);
	});
	pOptionValueChangerInterface->addEndingCapturedEditingHandler([pWeakOptionsForm, optionIdCopy](WIDGET&) {
		auto strongOptionsForm = pWeakOptionsForm.lock();
		ASSERT_OR_RETURN(, strongOptionsForm != nullptr, "Invalid pointer");
		strongOptionsForm->handleEndedRowEditing(optionIdCopy);
	});

	// Wrap the optionValueChangerWidget in an OptionsValueChangerWidgetWrapper
	auto wrappedOptionValueChangerWidget = OptionsValueChangerWidgetWrapper::wrap(optionValueChangerWidget);
	bool isAvailable = optionInfo.isAvailable();
	if (!isAvailable)
	{
		wrappedOptionValueChangerWidget->disable();
	}
	pOptionValueChangerInterface->informAvailable(isAvailable);

	// Store mapping
	auto insertResult = optionDetailsMap.insert({optionInfo.optionId, {optionInfo, optionLabelWidget, wrappedOptionValueChangerWidget, 0}});
	ASSERT_OR_RETURN(, insertResult.second, "Option id already added to OptionsForm: %s", optionInfo.optionId.toUtf8().c_str());

	// Add both widgets to an OptionRow, insert into list
	auto row = OptionRow::make(optionInfo.optionId, optionLabelWidget, wrappedOptionValueChangerWidget, indentLevel);
	row->setGeometry(0, 0, row->idealWidth(), row->idealHeight());
	if (bottomBorder)
	{
		row->setShowBottomBorder(true);
	}
	size_t itemListIdx = optionsList->addItem(row);
	insertResult.first->second.idxInOptionsList = itemListIdx;
}

std::shared_ptr<WIDGET> OptionsForm::findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	if (key == WKEY_NONE) // called this way for highlight processing by WIDGET::findMouseTargetRecursive, only if the mouse is actually over this widget (i.e. if hit-testing passes)
	{
		// mouse is over the options form this frame
		W_CONTEXT shiftedContext(psContext);
		shiftedContext.mx = psContext->mx - x();
		shiftedContext.my = psContext->my - y();
		shiftedContext.xOffset = psContext->xOffset + x();
		shiftedContext.yOffset = psContext->yOffset + y();
		mouseOverFormPosThisFrame = Vector2i(shiftedContext.mx, shiftedContext.my); // consumed by run()
	}

	// forward to parent
	return W_FORM::findMouseTargetRecursive(psContext, key, wasPressed);
}

void OptionsForm::run(W_CONTEXT *psContext)
{
	std::shared_ptr<WIDGET> currentHoverWidget = nullptr;
	if (capturedEditingRowWidget)
	{
		currentHoverWidget = capturedEditingRowWidget;
	}
	else if (mouseOverFormPosThisFrame.has_value())
	{
		const auto& listPadding = optionsList->getPadding();
		if ((mouseOverFormPosThisFrame.value().x >= listPadding.left)
			&& (mouseOverFormPosThisFrame.value().x <= (optionsList->width() - listPadding.right)))
		{
			// determine which row the mouse is currently over
			currentHoverWidget = std::dynamic_pointer_cast<OptionsRowWidgetBase>(optionsList->getItemAtYPos(mouseOverFormPosThisFrame.value().y));
		}
		else
		{
			currentHoverWidget = nullptr;
		}
		mouseOverFormPosThisFrame.reset(); // consume it
	}

	if (currentHoverWidget)
	{
		handleMouseIsOverRow(currentHoverWidget);
	}
	else if (priorMouseOverRowWidget)
	{
		handleMouseIsOverRow(nullptr);
	}
}

void OptionsForm::geometryChanged()
{
	int32_t w = width();
	int32_t h = height();

	int32_t listHeight = std::min<int32_t>(optionsList->idealHeight(), h);
	uint32_t listPaddingX = (maxListWidth > 0 && maxListWidth < w) ? static_cast<uint32_t>((w - maxListWidth) / 2) : 0;
	optionsList->setPadding(Padding{0, listPaddingX, 0, listPaddingX});
	optionsList->setGeometry(0, 0, w, listHeight);
}

void OptionsForm::screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight)
{
	if (refreshOptionsOnScreenSizeChange)
	{
		refreshOptions(true);
	}
	W_FORM::screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
}

void OptionsForm::handleMouseIsOverRow(const std::shared_ptr<WIDGET>& rowWidget)
{
	if (!rowWidget)
	{
		if (priorMouseOverRowWidget)
		{
			std::dynamic_pointer_cast<OptionsRowWidgetBase>(priorMouseOverRowWidget)->setIsHighlighted(false);
			priorMouseOverRowWidget.reset();
		}
		closeHelpPopover();
		return;
	}

	if (rowWidget != priorMouseOverRowWidget)
	{
		if (priorMouseOverRowWidget)
		{
			std::dynamic_pointer_cast<OptionsRowWidgetBase>(priorMouseOverRowWidget)->setIsHighlighted(false);
		}
		std::dynamic_pointer_cast<OptionsRowWidgetBase>(rowWidget)->setIsHighlighted(true);
		auto psOptionRowWidget = std::dynamic_pointer_cast<OptionRow>(rowWidget);
		bool rowIsInCapturedEditMode = capturedEditingRowWidget != nullptr;
		if (psOptionRowWidget && !rowIsInCapturedEditMode)
		{
			// it's an option row - display the help popover (as long as not in captured editing mode)
			auto it = optionDetailsMap.find(psOptionRowWidget->optionId());
			if (it != optionDetailsMap.end())
			{
				const auto& wrappedOptionValueChangerWidget = psOptionRowWidget->getWrappedOptionValueChangerWidget();
				auto pOptionValueChangerInterface = std::dynamic_pointer_cast<OptionValueChangerInterface>(wrappedOptionValueChangerWidget->getOptionValueChangerWidget());
				std::vector<OptionChoiceHelpDescription> choiceHelpDescriptions;
				if (pOptionValueChangerInterface)
				{
					auto optHelpDescriptions = pOptionValueChangerInterface->getHelpDescriptions();
					if (optHelpDescriptions.has_value())
					{
						choiceHelpDescriptions = std::move(optHelpDescriptions.value());
					}
				}
				openHelpPopover(it->second.info, choiceHelpDescriptions, wrappedOptionValueChangerWidget);
			}
			else
			{
				closeHelpPopover();
			}
		}
		else
		{
			closeHelpPopover();
		}
		priorMouseOverRowWidget = rowWidget;
	}
}

void OptionsForm::handleOptionValueChanged(const WzString& optionId)
{
	// Call update on all other options
	refreshOptionsInternal({optionId});

	// Update row layout (if needed)
	auto it = optionDetailsMap.find(optionId);
	if (it != optionDetailsMap.end())
	{
		auto row = std::dynamic_pointer_cast<OptionRow>(optionsList->getItemAtIdx(it->second.idxInOptionsList));
		if (row)
		{
			row->geometryChanged();
		}
	}
}

void OptionsForm::refreshOptionsInternal(const std::unordered_set<WzString>& except)
{
	if (isInRefreshOptionsInternal)
	{
		return;
	}
	isInRefreshOptionsInternal = true;
	const char* currentTranslationLocale = getLanguage();
	bool changedTranslationLocale = lastConfiguredTranslationLocale.compare(currentTranslationLocale) != 0;
	if (changedTranslationLocale)
	{
		lastConfiguredTranslationLocale = currentTranslationLocale;

		for (auto& i : sectionDetailsMap)
		{
			// Update the section row text
			auto pSectionRow = std::dynamic_pointer_cast<OptionsSectionRow>(i.second.optionSectionWidget);
			if (pSectionRow)
			{
				pSectionRow->setSectionName(i.second.info.getTranslatedDisplayName());
			}
		}
	}
	bool forceUpdate = changedTranslationLocale;
	for (auto& i : optionDetailsMap)
	{
		auto pOptionLabel = std::dynamic_pointer_cast<WzOptionsLabelWidget>(i.second.optionLabelWidget);
		if (pOptionLabel && forceUpdate)
		{
			pOptionLabel->setString(getOptionInfoTranslatedDisplayName(i.second.info));
		}
		if (except.count(i.first))
		{
			continue;
		}
		bool isAvailable = i.second.info.isAvailable();
		auto wrappedOptionValueChangerWidget = std::static_pointer_cast<OptionsValueChangerWidgetWrapper>(i.second.optionValueChangerWidgetWrapper);
		auto pOptionValueChangerInterface = std::dynamic_pointer_cast<OptionValueChangerInterface>(wrappedOptionValueChangerWidget->getOptionValueChangerWidget());
		if (pOptionValueChangerInterface)
		{
			pOptionValueChangerInterface->update(forceUpdate);
			pOptionValueChangerInterface->informAvailable(isAvailable);
		}
		wrappedOptionValueChangerWidget->enable(isAvailable);
		if (changedTranslationLocale)
		{
			auto row = std::dynamic_pointer_cast<OptionRow>(optionsList->getItemAtIdx(i.second.idxInOptionsList));
			if (row)
			{
				row->geometryChanged();
			}
		}
	}
	isInRefreshOptionsInternal = false;
}

void OptionsForm::refreshOptions(bool forceRowLayoutUpdates /*= false*/)
{
	refreshOptionsInternal({});
	if (forceRowLayoutUpdates)
	{
		for (auto& i : optionDetailsMap)
		{
			auto row = std::dynamic_pointer_cast<OptionRow>(optionsList->getItemAtIdx(i.second.idxInOptionsList));
			if (row)
			{
				row->geometryChanged();
			}
		}
	}
}

void OptionsForm::refreshOptionsLayoutCalc()
{
	// trigger screenSizeDidChange to ensure all widgets have a chance to respond to display scale changes that occurred while not part of the active widget hierarchy
	WIDGET::screenSizeDidChange(screenWidth, screenHeight, screenWidth, screenHeight);

	// call geometryChanged() on rows to recalculate layout based on child widget ideal width (etc)
	for (auto& i : optionDetailsMap)
	{
		auto row = std::dynamic_pointer_cast<OptionRow>(optionsList->getItemAtIdx(i.second.idxInOptionsList));
		if (row)
		{
			row->geometryChanged();
		}
	}
	for (auto& i : sectionDetailsMap)
	{
		auto row = std::dynamic_pointer_cast<OptionsSectionRow>(optionsList->getItemAtIdx(i.second.idxInOptionsList));
		if (row)
		{
			row->geometryChanged();
		}
	}
}

void OptionsForm::refreshOptionAvailability()
{
	for (auto& i : optionDetailsMap)
	{
		bool isAvailable = i.second.info.isAvailable();
		auto wrappedOptionValueChangerWidget = std::static_pointer_cast<OptionsValueChangerWidgetWrapper>(i.second.optionValueChangerWidgetWrapper);
		wrappedOptionValueChangerWidget->enable(isAvailable);
		auto pOptionValueChangerInterface = std::dynamic_pointer_cast<OptionValueChangerInterface>(wrappedOptionValueChangerWidget->getOptionValueChangerWidget());
		if (pOptionValueChangerInterface)
		{
			pOptionValueChangerInterface->informAvailable(isAvailable);
		}
	}
}
