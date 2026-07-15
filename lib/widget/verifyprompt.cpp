// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project

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
 *  Functions for the verify prompts
 */

#include "verifyprompt.h"

#include "label.h"
#include "editbox.h"
#include "slider.h"
#include "button.h"

#include "lib/framework/crc.h"
#include "lib/ivis_opengl/pieblitfunc.h"

namespace WzVerifyPrompt {

IvImageProviderInterface::~IvImageProviderInterface()
{ }

VerifyPromptData::~VerifyPromptData()
{ }

// MARK: - Helpers

static double getTextureDimensionsScaleRatioForMax(const gfx_api::texture2dDimensions& dimensions, size_t maxWidth, size_t maxHeight)
{
	if ((maxWidth == 0 || dimensions.width <= maxWidth) && (maxHeight == 0 || dimensions.height <= maxHeight))
	{
		return 1.0;
	}

	double scalingRatio;
	double widthRatio = (double)maxWidth / (double)dimensions.width;
	double heightRatio = (double)maxHeight / (double)dimensions.height;
	if (maxWidth > 0 && maxHeight > 0)
	{
		scalingRatio = std::min<double>(widthRatio, heightRatio);
	}
	else
	{
		scalingRatio = (maxWidth > 0) ? widthRatio : heightRatio;
	}

	return scalingRatio;
}

// MARK: - PromptActionButtonWidget

class PromptActionButtonWidget : public W_BUTTON
{
protected:
	PromptActionButtonWidget() {}
	void initialize(const WzString& text);
public:
	static std::shared_ptr<PromptActionButtonWidget> make(const WzString& text)
	{
		class make_shared_enabler: public PromptActionButtonWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(text);
		return widget;
	}
protected:
	void display(int xOffset, int yOffset) override;
private:
	WzText wzText;
	const int InternalPadding = 12;
};

void PromptActionButtonWidget::initialize(const WzString& text)
{
	setString(text);
	FontID = font_regular_bold;
	int minButtonWidthForText = iV_GetTextWidth(pText, FontID);
	setGeometry(0, 0, minButtonWidthForText + InternalPadding, iV_GetTextLineSize(FontID) + InternalPadding);
}

void PromptActionButtonWidget::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	bool haveText = !pText.isEmpty();

	bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (getState() & WBUT_DISABLE) != 0;
	bool isHighlight = !isDisabled && ((getState() & WBUT_HIGHLIGHT) != 0);

	PIELIGHT colour;
	if (isDisabled)								// unavailable
	{
		colour = WZCOL_TEXT_DARK;
	}
	else										// available
	{
		if (isHighlight)						// hilight
		{
			colour = WZCOL_TEXT_BRIGHT;
		}
		else									// don't highlight
		{
			colour = WZCOL_TEXT_MEDIUM;
		}
	}

	if (isHighlight || isDown)
	{
		PIELIGHT fillClr = pal_RGBA(255, 255, 255, 30);
		pie_UniTransBoxFill(x0, y0, x1, y1, fillClr);
	}
	iV_Box(x0, y0, x1, y1, colour);

	if (haveText)
	{
		wzText.setText(pText, FontID);
		int fw = wzText.width();
		int fx = x0 + (width() - fw) / 2;
		int fy = y0 + (height() - wzText.lineSize()) / 2 - wzText.aboveBase();
		wzText.render(fx, fy, colour);
	}
}

// MARK: - CaptchaPromptWidget

class CaptchaPromptWidget : public WIDGET
{
protected:
	CaptchaPromptWidget();
	~CaptchaPromptWidget();
	bool initialize(CaptchaVerifyPromptData&& promptData, const CaptchaVerifyResponseFunc& responseCallback);

public:
	static std::shared_ptr<CaptchaPromptWidget> make(CaptchaVerifyPromptData&& promptData, const CaptchaVerifyResponseFunc& responseCallback);

	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;

	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;

private:
	void recalcLayout();
	void submitEntry(const WzString& value);

private:
	CaptchaVerifyPromptData promptData;
	CaptchaVerifyResponseFunc responseCallback;

	std::shared_ptr<W_LABEL> promptText;
	gfx_api::texture *pImageTexture = nullptr;
	double imageScalingRatio = 1.0;

	std::shared_ptr<W_EDITBOX> inputBox;
	std::shared_ptr<PromptActionButtonWidget> okayButton;

	bool submittedEntry = false;

	const iV_fonts textFontId = font_regular_bold;
	const int32_t innerPaddingY = 10;
	const int32_t minimumInputAreaWidth = 200;
};

CaptchaPromptWidget::CaptchaPromptWidget()
{ }

CaptchaPromptWidget::~CaptchaPromptWidget()
{
	if (pImageTexture)
	{
		delete pImageTexture;
		pImageTexture = nullptr;
	}
}

std::shared_ptr<CaptchaPromptWidget> CaptchaPromptWidget::make(CaptchaVerifyPromptData&& promptData, const CaptchaVerifyResponseFunc& responseCallback)
{
	class make_shared_enabler: public CaptchaPromptWidget {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize(std::move(promptData), responseCallback);
	return widget;
}

bool CaptchaPromptWidget::initialize(CaptchaVerifyPromptData&& _promptData, const CaptchaVerifyResponseFunc& _responseCallback)
{
	promptData = std::move(_promptData);
	responseCallback = _responseCallback;

	promptText = std::make_shared<W_LABEL>();
	promptText->setFont(textFontId, WZCOL_FORM_LIGHT);
	promptText->setString(_("Please enter the text below"));
	promptText->setCanTruncate(true);
	promptText->setTransparentToClicks(true);
	attach(promptText);

	ASSERT_OR_RETURN(false, promptData.image != nullptr, "No image to load");
	auto image = promptData.image->GetImage();
	ASSERT_OR_RETURN(false, image.has_value(), "Failed to load image");

	pImageTexture = gfx_api::context::get().loadTextureFromUncompressedImage(std::move(image.value()), gfx_api::texture_type::user_interface, "slide_image");
	ASSERT_OR_RETURN(false, pImageTexture != nullptr, "Failed to load image texture");

	inputBox = std::make_shared<W_EDITBOX>();
	attach(inputBox);
	inputBox->setGeometry(0, 0, 280, 24);
	inputBox->setBoxColours(WZCOL_WHITE, WZCOL_WHITE, WZCOL_MENU_BACKGROUND);
	inputBox->setMaxStringSize(16);
	inputBox->setPlaceholder(_("Enter the text"));

	auto weakParent = std::weak_ptr<CaptchaPromptWidget>(std::static_pointer_cast<CaptchaPromptWidget>(shared_from_this()));
	inputBox->setOnReturnHandler([weakParent](W_EDITBOX& widg) {
		WzString str = widg.getString();
		widgScheduleTask([str, weakParent]() {
			auto strongParent = weakParent.lock();
			ASSERT_OR_RETURN(, strongParent != nullptr, "No parent?");
			strongParent->submitEntry(str);
		});
	});

	// Add OK button
	okayButton = PromptActionButtonWidget::make(_("OK"));
	attach(okayButton);
	okayButton->addOnClickHandler([weakParent](W_BUTTON&) {
		auto strongParent = weakParent.lock();
		ASSERT_OR_RETURN(, strongParent != nullptr, "No parent?");
		strongParent->submitEntry(strongParent->inputBox->getString());
	});

	return true;
}

void CaptchaPromptWidget::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();

	const auto imageDimensions = pImageTexture->get_dimensions();
	const auto scaledImageDimensions = Vector2f(imageDimensions.width * imageScalingRatio, imageDimensions.height * imageScalingRatio);

	const int imageAreaXPadding = (width() - static_cast<int>(std::ceil(scaledImageDimensions.x))) / 2;
	const int imageBoxX0 = x0 + imageAreaXPadding;
	const int imageBoxY0 = y0 + promptText->y() + promptText->height() + innerPaddingY;

	// Draw white background behind the pImageTexture
	pie_UniTransBoxFill(imageBoxX0, imageBoxY0, imageBoxX0 + scaledImageDimensions.x, imageBoxY0 + scaledImageDimensions.y, WZCOL_WHITE);

	// Draw the pImageTexture
	iV_DrawImageAnisotropic(*pImageTexture, Vector2i(imageBoxX0, imageBoxY0), Vector2f(0.f, 0.f), scaledImageDimensions, 0.f, WZCOL_WHITE);

	if (submittedEntry)
	{
		pie_UniTransBoxFill(imageBoxX0, imageBoxY0, imageBoxX0 + scaledImageDimensions.x, imageBoxY0 + scaledImageDimensions.y, WZCOL_TRANSPARENT_BOX);
	}
}

void CaptchaPromptWidget::geometryChanged()
{
	recalcLayout();
}

void CaptchaPromptWidget::recalcLayout()
{
	int w = width();
	int h = height();

	promptText->setGeometry(0, 0, w, promptText->idealHeight());

	const int inputBoxAreaHeight = std::max<int>(inputBox->height(), okayButton->height());
	const int neededHeightForNonImageWidgets = promptText->height() + innerPaddingY + innerPaddingY + inputBoxAreaHeight;
	const int availableHeightForImage = h - neededHeightForNonImageWidgets;

	imageScalingRatio = getTextureDimensionsScaleRatioForMax(pImageTexture->get_dimensions(), w, availableHeightForImage);

	const auto imageDimensions = pImageTexture->get_dimensions();
	const int scaledImageWidth = static_cast<int>(std::ceil(imageDimensions.width * imageScalingRatio));

	const int imageBoxY0 = promptText->y() + promptText->height() + innerPaddingY;
	const int imageBoxY1 = imageBoxY0 + static_cast<int32_t>(imageDimensions.height * imageScalingRatio);

	const int inputBoxAreaY0 = imageBoxY1 + innerPaddingY;
	const int availableInputBoxAreaWidth = std::min<int>(std::max<int>(scaledImageWidth, minimumInputAreaWidth), w);
	const int inputBoxAreaXPadding = (w - availableInputBoxAreaWidth) / 2;

	int buttonX0 = w - inputBoxAreaXPadding - okayButton->width();
	okayButton->setGeometry(buttonX0, inputBoxAreaY0, okayButton->width(), inputBoxAreaHeight);

	int inputBoxWidth = std::max<int>(std::max<int>(buttonX0 - 5, 0) - inputBoxAreaXPadding, 0);
	inputBox->setGeometry(inputBoxAreaXPadding, inputBoxAreaY0, inputBoxWidth, inputBoxAreaHeight);
}

int32_t CaptchaPromptWidget::idealWidth()
{
	int32_t result = promptText->idealWidth();
	result = std::max<int32_t>(static_cast<int32_t>(pImageTexture->get_dimensions().width), result);
	result = std::max<int32_t>(minimumInputAreaWidth, result);
	return result;
}

int32_t CaptchaPromptWidget::idealHeight()
{
	int32_t result = std::abs(iV_GetTextAboveBase(textFontId));
	result += promptText->idealHeight() + innerPaddingY;
	result += static_cast<int32_t>(pImageTexture->get_dimensions().height) + innerPaddingY;
	result += std::max<int32_t>(inputBox->height(), okayButton->height());

	return result;
}

void CaptchaPromptWidget::submitEntry(const WzString& value)
{
	ASSERT_OR_RETURN(, responseCallback != nullptr, "No response callback??");

	if (submittedEntry)
	{
		return;
	}

	responseCallback(value);

	if (promptText)
	{
		promptText->setFont(textFontId, WZCOL_TEXT_DARK);
	}
	if (inputBox)
	{
		inputBox->setState(WEDBS_DISABLE);
		inputBox->setBoxColours(WZCOL_MENU_BORDER, WZCOL_MENU_BORDER, WZCOL_MENU_BACKGROUND);
	}
	if (okayButton)
	{
		okayButton->setState(WBUT_DISABLE);
	}
	submittedEntry = true;
}

// MARK: - VerifySlideWidget

class VerifySlideWidget : public W_SLIDER
{
protected:
	VerifySlideWidget(W_SLDINIT const *init)
	: W_SLIDER(init)
	{ }
public:
	static std::shared_ptr<VerifySlideWidget> make();

	void released(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void display(int xOffset, int yOffset) override;

	typedef std::function<void(VerifySlideWidget&)> OnReleasedCallback;
	void setOnReleasedCallback(const OnReleasedCallback& newOnReleasedCallback)
	{
		onReleasedCallback = newOnReleasedCallback;
	}

private:
	OnReleasedCallback onReleasedCallback;

	WzText rightArrowText;
};

std::shared_ptr<VerifySlideWidget> VerifySlideWidget::make()
{
	class make_shared_enabler: public VerifySlideWidget {
	public:
		make_shared_enabler(W_SLDINIT const *init)
		: VerifySlideWidget(init)
		{ }
	};

	W_SLDINIT sSldInit;
	sSldInit.orientation = WSLD_LEFT;

	auto widget = std::make_shared<make_shared_enabler>(&sSldInit);
	widget->barSize = 48;
	return widget;
}

void VerifySlideWidget::released(W_CONTEXT *psContext, WIDGET_KEY key)
{
	W_SLIDER::released(psContext, key);

	if (onReleasedCallback)
	{
		onReleasedCallback(*this);
	}
}

void VerifySlideWidget::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();
	int w = width();
	int h = height();

	// Draw bar

	const int barHeight = std::max<int>(h / 2, 1);
	int barY0 = y0 + ((h - barHeight) / 2);
	pie_UniTransBoxFill(x0, barY0, x0 + w, barY0 + barHeight, WZCOL_DBLUE);

	// Draw slider box

	auto sliderX = numStops > 0 ? (w - barSize) * pos / numStops : 0;
	int sliderDrawX0 = x0 + sliderX+2;
	int sliderDrawX1 = x0 + sliderX + std::max<int>(barSize,5)-2;
	if (sliderDrawX1 - sliderDrawX0 <= 2)
	{
		--sliderDrawX0;
		++sliderDrawX1;
	}
	pie_UniTransBoxFill(sliderDrawX0, y0, sliderDrawX1, y0 + h, isEnabled() ? WZCOL_LBLUE : WZCOL_DBLUE);

	// Draw right-facing arrow centered in slider box // "\u2192" = →

	rightArrowText.setText("\u2192", font_medium_bold);
	float arrowTextX0 = sliderDrawX0 + (((sliderDrawX1 - sliderDrawX0) - rightArrowText.width()) / 2);
	float arrowTextY0 = y0 + (h - rightArrowText.lineSize()) / 2 - rightArrowText.aboveBase();
	rightArrowText.render(arrowTextX0, arrowTextY0, isEnabled() ? WZCOL_WHITE : WZCOL_TEXT_DARK);
}

// MARK: - SlidePromptWidget

class SlidePromptWidget : public WIDGET
{
protected:
	SlidePromptWidget();
	~SlidePromptWidget();
	bool initialize(SlideVerifyPromptData&& promptData, bool dragDrop, const SlideVerifyResponseFunc& responseCallback);

public:
	static std::shared_ptr<SlidePromptWidget> make(SlideVerifyPromptData&& promptData, bool dragDrop, const SlideVerifyResponseFunc& responseCallback);

	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;

	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;

	bool capturesMouseDrag(WIDGET_KEY) override { return dragDrop && !submittedResponse; }
	void released(W_CONTEXT *psContext, WIDGET_KEY key) override;

protected:
	// handling mouse drag
	void mouseDragged(WIDGET_KEY, W_CONTEXT *start, W_CONTEXT *current) override;

private:
	void recalcLayout();
	void handleSliderPosChanged();
	void handleSliderInputReleased();
	WzRect calcTileRect() const;

private:
	SlideVerifyPromptData promptData;
	bool dragDrop = false;
	SlideVerifyResponseFunc responseCallback;

	std::shared_ptr<W_LABEL> promptText;
	gfx_api::texture *pImageTexture = nullptr;
	double imageScalingRatio = 1.0;
	uint32_t imageWidth = 0;
	uint32_t imageHeight = 0;

	gfx_api::texture *pTileTexture = nullptr;
	uint32_t currTileX = 0;
	uint32_t currTileY = 0;

	std::shared_ptr<VerifySlideWidget> slider;
	optional<Vector2i> handlingDragTileStartMouseOffset = nullopt;

	bool submittedResponse = false;

	const iV_fonts textFontId = font_regular_bold;
	const int32_t innerPaddingY = 10;
	const int32_t sliderHeight = 28;
	const int32_t minimumInputAreaWidth = 200;
};

SlidePromptWidget::SlidePromptWidget()
{ }

SlidePromptWidget::~SlidePromptWidget()
{
	if (pImageTexture)
	{
		delete pImageTexture;
		pImageTexture = nullptr;
	}

	if (pTileTexture)
	{
		delete pTileTexture;
		pTileTexture = nullptr;
	}
}

std::shared_ptr<SlidePromptWidget> SlidePromptWidget::make(SlideVerifyPromptData&& promptData, bool dragDrop, const SlideVerifyResponseFunc& responseCallback)
{
	class make_shared_enabler: public SlidePromptWidget {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize(std::move(promptData), dragDrop, responseCallback);
	return widget;
}

bool SlidePromptWidget::initialize(SlideVerifyPromptData&& _promptData, bool _dragDrop, const SlideVerifyResponseFunc& _responseCallback)
{
	promptData = std::move(_promptData);
	dragDrop = _dragDrop;
	responseCallback = _responseCallback;

	promptText = std::make_shared<W_LABEL>();
	promptText->setFont(textFontId, WZCOL_FORM_LIGHT);
	if (!dragDrop)
	{
		promptText->setString(_("Please drag the slider to verify"));
	}
	else
	{
		promptText->setString(_("Please drag the tile to verify"));
	}
	promptText->setCanTruncate(true);
	promptText->setTransparentToClicks(true);
	attach(promptText);

	ASSERT_OR_RETURN(false, promptData.image != nullptr, "No image to load");
	auto image = promptData.image->GetImage();
	ASSERT_OR_RETURN(false, image.has_value(), "Failed to load image");

	ASSERT_OR_RETURN(false, promptData.tile != nullptr, "No tile to load");
	auto tile = promptData.tile->GetImage();
	ASSERT_OR_RETURN(false, tile.has_value(), "Failed to load tile");

	imageWidth = image->width();
	imageHeight = image->height();

	pImageTexture = gfx_api::context::get().loadTextureFromUncompressedImage(std::move(image.value()), gfx_api::texture_type::user_interface, "slide_image");
	ASSERT_OR_RETURN(false, pImageTexture != nullptr, "Failed to load image texture");

	pTileTexture = gfx_api::context::get().loadTextureFromUncompressedImage(std::move(tile.value()), gfx_api::texture_type::user_interface, "slide_tile");
	ASSERT_OR_RETURN(false, pTileTexture != nullptr, "Failed to load tile texture");

	currTileX = promptData.tile_x;
	currTileY = promptData.tile_y;

	auto weakParent = std::weak_ptr<SlidePromptWidget>(std::static_pointer_cast<SlidePromptWidget>(shared_from_this()));

	if (!dragDrop)
	{
		// Make slider with custom drawing function that draws a slider bar + box with -> arrow in it
		slider = VerifySlideWidget::make();
		slider->addOnChange([weakParent](W_SLIDER &) {
			auto strongParent = weakParent.lock();
			ASSERT_OR_RETURN(, strongParent != nullptr, "Null parent");
			strongParent->handleSliderPosChanged();
		});
		slider->setOnReleasedCallback([weakParent](VerifySlideWidget &) {
			auto strongParent = weakParent.lock();
			ASSERT_OR_RETURN(, strongParent != nullptr, "Null parent");
			strongParent->handleSliderInputReleased();
		});
		slider->numStops = static_cast<uint16_t>(static_cast<uint32_t>(pImageTexture->get_dimensions().width) - (promptData.tile_x + promptData.tile_width));
		slider->pos = 0;
		attach(slider);
	}

	return true;
}

void SlidePromptWidget::handleSliderPosChanged()
{
	currTileX = promptData.tile_x + slider->pos;
}

void SlidePromptWidget::handleSliderInputReleased()
{
	ASSERT_OR_RETURN(, responseCallback != nullptr, "No response callback??");

	if (submittedResponse)
	{
		return;
	}

	responseCallback(currTileX, currTileY);

	if (promptText)
	{
		promptText->setFont(textFontId, WZCOL_TEXT_DARK);
	}
	if (slider)
	{
		slider->disable();
	}

	submittedResponse = true;

	recalcLayout();
}

void SlidePromptWidget::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();

	const auto imageDimensions = pImageTexture->get_dimensions();
	const auto scaledImageDimensions = Vector2f(imageDimensions.width * imageScalingRatio, imageDimensions.height * imageScalingRatio);

	const int imageAreaXPadding = (width() - static_cast<int>(std::ceil(scaledImageDimensions.x))) / 2;
	const int imageBoxX0 = x0 + imageAreaXPadding;
	const int imageBoxY0 = y0 + promptText->y() + promptText->height() + innerPaddingY;

	// Draw the pImageTexture
	iV_DrawImageAnisotropic(*pImageTexture, Vector2i(imageBoxX0, imageBoxY0), Vector2f(0.f, 0.f), scaledImageDimensions, 0.f, WZCOL_WHITE);

	// Draw the pTileTexture at (currTileX, currTileY)
	iV_DrawImageAnisotropic(*pTileTexture, Vector2i(imageBoxX0, imageBoxY0), Vector2f(currTileX * imageScalingRatio, currTileY * imageScalingRatio), Vector2f(promptData.tile_width * imageScalingRatio, promptData.tile_height * imageScalingRatio), 0.f, WZCOL_WHITE);

	if (submittedResponse)
	{
		pie_UniTransBoxFill(imageBoxX0, imageBoxY0, imageBoxX0 + scaledImageDimensions.x, imageBoxY0 + scaledImageDimensions.y, WZCOL_TRANSPARENT_BOX);
	}
}

void SlidePromptWidget::geometryChanged()
{
	recalcLayout();
}

WzRect SlidePromptWidget::calcTileRect() const
{
	const int w = width();

	const auto imageDimensions = pImageTexture->get_dimensions();
	const int scaledImageWidth = static_cast<int>(std::ceil(imageDimensions.width * imageScalingRatio));

	const int imageAreaXPadding = (w - scaledImageWidth) / 2;
	const int imageBoxX0 = imageAreaXPadding;
	const int imageBoxY0 = promptText->y() + promptText->height() + innerPaddingY;

	const int tileX0 = imageBoxX0 + static_cast<int>(std::ceil(currTileX * imageScalingRatio));
	const int tileY0 = imageBoxY0 + static_cast<int>(std::ceil(currTileY * imageScalingRatio));

	const int tileDisplayWidth = static_cast<int>(std::ceil(promptData.tile_width * imageScalingRatio));
	const int tileDisplayHeight = static_cast<int>(std::ceil(promptData.tile_height * imageScalingRatio));

	return WzRect(tileX0, tileY0, tileDisplayWidth, tileDisplayHeight);
}

void SlidePromptWidget::recalcLayout()
{
	int w = width();
	int h = height();

	promptText->setGeometry(0, 0, w, promptText->idealHeight());

	const int neededHeightForNonImageWidgets = promptText->height() + innerPaddingY + ((dragDrop) ? 0 : (innerPaddingY + sliderHeight));
	const int availableHeightForImage = h - neededHeightForNonImageWidgets;

	imageScalingRatio = getTextureDimensionsScaleRatioForMax(pImageTexture->get_dimensions(), w, availableHeightForImage);

	const auto imageDimensions = pImageTexture->get_dimensions();
	const int scaledImageWidth = static_cast<int>(std::ceil(imageDimensions.width * imageScalingRatio));

//	const int imageAreaXPadding = (w - scaledImageWidth) / 2;
//	const int imageBoxX0 = imageAreaXPadding;
	const int imageBoxY0 = promptText->y() + promptText->height() + innerPaddingY;
	const int imageBoxY1 = imageBoxY0 + static_cast<int32_t>(imageDimensions.height * imageScalingRatio);

	if (slider)
	{
		const int availableInputAreaWidth = std::min<int>(std::max<int>(scaledImageWidth, minimumInputAreaWidth), w);
		const int inputAreaXPadding = (w - availableInputAreaWidth) / 2;

		const int sliderY0 = imageBoxY1 + innerPaddingY;

		slider->setGeometry(inputAreaXPadding, sliderY0, availableInputAreaWidth, sliderHeight);
	}
}

int32_t SlidePromptWidget::idealWidth()
{
	int32_t result = promptText->idealWidth();
	result = std::max<int32_t>(static_cast<int32_t>(pImageTexture->get_dimensions().width), result);
	result = std::max<int32_t>(minimumInputAreaWidth, result);
	return result;
}

int32_t SlidePromptWidget::idealHeight()
{
	int32_t result = std::abs(iV_GetTextAboveBase(textFontId));
	result += promptText->idealHeight() + innerPaddingY;
	result += static_cast<int32_t>(pImageTexture->get_dimensions().height);
	if (!dragDrop)
	{
		result += innerPaddingY + sliderHeight;
	}

	return result;
}

void SlidePromptWidget::released(W_CONTEXT *psContext, WIDGET_KEY key)
{
	WIDGET::released(psContext, key);

	if (handlingDragTileStartMouseOffset.has_value())
	{
		if (dragDrop)
		{
			handleSliderInputReleased();
		}
		handlingDragTileStartMouseOffset.reset();
	}
}

void SlidePromptWidget::mouseDragged(WIDGET_KEY, W_CONTEXT *start, W_CONTEXT *current)
{
	if (!handlingDragTileStartMouseOffset.has_value())
	{
		auto tileRect = calcTileRect();
		int startMx = start->mx - x();
		int startMy = start->my - y();
		if (!tileRect.contains(startMx, startMy))
		{
			return;
		}

		// click started inside the tile rect - process drag
		handlingDragTileStartMouseOffset = Vector2i(startMx - tileRect.left(), startMy - tileRect.top());
	}

	const int w = width();

	const auto imageDimensions = pImageTexture->get_dimensions();
	const int scaledImageWidth = static_cast<int>(std::ceil(imageDimensions.width * imageScalingRatio));

	const int imageAreaXPadding = (w - scaledImageWidth) / 2;
	const int imageBoxX0 = imageAreaXPadding;
	const int imageBoxY0 = promptText->y() + promptText->height() + innerPaddingY;

	int mx = current->mx - x();
	int my = current->my - y();
	Vector2i tileDragPosAdjusted(mx - handlingDragTileStartMouseOffset.value().x - imageBoxX0, my - handlingDragTileStartMouseOffset.value().y - imageBoxY0);

	// convert to logical tile coordinates
	auto proposedNewCurrTileX = static_cast<int64_t>(std::round(tileDragPosAdjusted.x / imageScalingRatio));
	auto proposedNewCurrTileY = static_cast<int64_t>(std::round(tileDragPosAdjusted.y / imageScalingRatio));
	currTileX = static_cast<uint32_t>(std::clamp<int64_t>(proposedNewCurrTileX, 0, imageWidth - promptData.tile_width));
	currTileY = static_cast<uint32_t>(std::clamp<int64_t>(proposedNewCurrTileY, 0, imageHeight - promptData.tile_height));
}

// MARK: - Centered Wrapper Widget

class CaptchaWrapperWidget : public WIDGET
{
protected:
	CaptchaWrapperWidget() {}
	void initialize(const std::shared_ptr<WIDGET>& wrappedCaptchaWidget);

public:
	static std::shared_ptr<CaptchaWrapperWidget> make(const std::shared_ptr<WIDGET>& wrappedCaptchaWidget);

	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;

	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;

private:
	void recalcLayout();

private:
	std::shared_ptr<WIDGET> wrappedCaptchaWidget;
	int32_t cachedIdealWidth = 0;
	int32_t cachedIdealHeight = 0;
	const int32_t innerPaddingX = 20;
	const int32_t innerPaddingY = 20;
};

std::shared_ptr<CaptchaWrapperWidget> CaptchaWrapperWidget::make(const std::shared_ptr<WIDGET>& wrappedCaptchaWidget)
{
	class make_shared_enabler: public CaptchaWrapperWidget {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize(wrappedCaptchaWidget);
	return widget;
}

void CaptchaWrapperWidget::initialize(const std::shared_ptr<WIDGET>& _wrappedCaptchaWidget)
{
	wrappedCaptchaWidget = _wrappedCaptchaWidget;
	attach(wrappedCaptchaWidget);

	cachedIdealWidth = wrappedCaptchaWidget->idealWidth();
	cachedIdealHeight = wrappedCaptchaWidget->idealHeight();
}

void CaptchaWrapperWidget::display(int xOffset, int yOffset)
{
	int x0 = xOffset + x();
	int y0 = yOffset + y();

	// Draw background
	pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), pal_RGBA(0,0,0,35));
}

void CaptchaWrapperWidget::geometryChanged()
{
	recalcLayout();
}

int32_t CaptchaWrapperWidget::idealWidth()
{
	return cachedIdealWidth + (innerPaddingX * 2);
}

int32_t CaptchaWrapperWidget::idealHeight()
{
	return cachedIdealHeight + (innerPaddingY * 2);
}

void CaptchaWrapperWidget::recalcLayout()
{
	int w = width();
	int h = height();

	// position wrappedCaptchaWidget centered in the parent
	int wrappedWidth = std::min<int>(w - (innerPaddingX * 2), cachedIdealWidth);
	int wrappedHeight = std::min<int>(h - (innerPaddingY * 2), cachedIdealHeight);
	int wrappedX0 = (w - wrappedWidth) / 2;
	int wrappedY0 = (h - wrappedHeight) / 2;

	wrappedCaptchaWidget->setGeometry(wrappedX0, wrappedY0, wrappedWidth, wrappedHeight);
}

// MARK: - Public API

std::shared_ptr<WIDGET> makeCaptchaWidget(CaptchaVerifyPromptData&& promptData, const CaptchaVerifyResponseFunc& responseCallback)
{
	return CaptchaWrapperWidget::make(CaptchaPromptWidget::make(std::move(promptData), responseCallback));
}

std::shared_ptr<WIDGET> makeSlideWidget(SlideVerifyPromptData&& promptData, bool dragDrop, const SlideVerifyResponseFunc& responseCallback)
{
	return CaptchaWrapperWidget::make(SlidePromptWidget::make(std::move(promptData), dragDrop, responseCallback));
}

} // namespace WzVerifyPrompt
