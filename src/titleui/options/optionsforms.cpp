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

#include "optionsforms.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/widget/label.h"
#include "lib/framework/input.h"
#include "../../warzoneconfig.h"
#include "../../urlhelpers.h"
#include "lib/framework/wzapp.h"

#include <functional>

// MARK: -

// Note: Currently does not support W_BUTTON's:
// - setImages functions
// - progressBorder functionality
class WzClippableButton : public W_BUTTON
{
public:
	void run(W_CONTEXT *psContext) override;
	void display(int xOffset, int yOffset) override;
	void displayRecursive(WidgetGraphicsContext const &context) override;
	void setString(WzString string) override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;
	void setFont(iV_fonts font);
	void setPadding(int32_t horizontalPadding, int32_t verticalPadding);
private:
	void displayImpl(int xOffset, int yOffset, optional<WzRect> clipRectScreenCoords = nullopt);
private:
	WzCachedText wzText;
	int32_t horizontalPadding = 15;
	int32_t verticalPadding = 10;
};

void WzClippableButton::setFont(iV_fonts font)
{
	FontID = font;
	wzText.setText(pText, FontID);
}

void WzClippableButton::setPadding(int32_t horizontalPadding_, int32_t verticalPadding_)
{
	horizontalPadding = horizontalPadding_;
	verticalPadding = verticalPadding_;
}

void WzClippableButton::setString(WzString string)
{
	W_BUTTON::setString(string);
	wzText.setText(string, FontID);
}

void WzClippableButton::run(W_CONTEXT *psContext)
{
	wzText.tick();
}

void WzClippableButton::display(int xOffset, int yOffset)
{
	displayImpl(xOffset, yOffset);
}

void WzClippableButton::displayImpl(int xOffset, int yOffset, optional<WzRect> clipRectScreenCoords)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int w = width();
	int h = height();

	bool isDown = (state & WBUT_DOWN) != 0;
	bool isLocked = (state & (WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (state & WBUT_DISABLE) != 0;
	bool isHighlight = (state & WBUT_HIGHLIGHT) != 0;

	// Draw the background (clipped if needed)
	if (isDown || isLocked || isHighlight)
	{
		PIELIGHT backgroundColor;
		if (isLocked)
		{
			backgroundColor = WZCOL_MENU_SCORE_BUILT;
		}
		else if (isDown)
		{
			backgroundColor = WZCOL_FORM_DARK;
		}
		else
		{
			backgroundColor = WZCOL_MENU_SCORE_BUILT;
			backgroundColor.byte.a = backgroundColor.byte.a / 2;
		}
		int backX0 = std::max<int>(x0, clipRectScreenCoords.has_value() ? clipRectScreenCoords.value().x() : 0);
		int backY0 = std::max<int>(y0, clipRectScreenCoords.has_value() ? clipRectScreenCoords.value().y() : 0);
		int backX1 = std::min<int>(x0 + w, clipRectScreenCoords.has_value() ? clipRectScreenCoords.value().right() : x0 + w);
		int backY1 = std::min<int>(y0 + h, clipRectScreenCoords.has_value() ? clipRectScreenCoords.value().bottom() : y0 + h);
		pie_UniTransBoxFill(backX0, backY0, backX1, backY1, backgroundColor);
	}

	// Draw the main text
	PIELIGHT textColor = (!isDisabled && (isLocked || isHighlight)) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM;
	if (isDisabled)
	{
		textColor.byte.a = (textColor.byte.a / 2);
	}

	int textWidth = wzText->width();
	int textX0 = x0 + (w - textWidth) / 2;
	int textY0 = y0 + (h - wzText->lineSize()) / 2 - wzText->aboveBase();

	if (clipRectScreenCoords.has_value())
	{
		wzText->renderClipped(Vector2f(textX0, textY0), textColor, clipRectScreenCoords.value());
	}
	else
	{
		wzText->render(Vector2f(textX0, textY0), textColor);
	}
}

void WzClippableButton::displayRecursive(WidgetGraphicsContext const &context)
{
	WzRect clipRectScreenCoords;
	if (context.clipIntersects(geometry(), &clipRectScreenCoords))
	{
		displayImpl(context.getXOffset(), context.getYOffset(), clipRectScreenCoords);
	}
	else
	{
		if (!context.allowChildDisplayRecursiveIfSelfClipped())
		{
			return;
		}
	}

	auto childrenContext = context.translatedBy(x(), y()).setAllowChildDisplayRecursiveIfSelfClipped(false);

	for (auto child : children())
	{
		if (child->visible())
		{
			child->displayRecursive(childrenContext);
		}
	}
}

int32_t WzClippableButton::idealWidth()
{
	return (horizontalPadding * 2) + wzText.getTextWidth();
}

int32_t WzClippableButton::idealHeight()
{
	return (verticalPadding * 2) + wzText.getTextLineSize();
}

// MARK: - HorizontalScrollButton

class HorizontalScrollButton : public W_BUTTON
{
protected:
	HorizontalScrollButton() { }
	void initialize(const WzString& c, iV_fonts buttonFontID);

public:
	static std::shared_ptr<HorizontalScrollButton> make(const WzString& c, iV_fonts buttonFontID);

	void display(int xOffset, int yOffset) override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;

private:
	WzText wzText;
	int32_t horizontalPadding = 5;
	int32_t verticalPadding = 5;
};

std::shared_ptr<HorizontalScrollButton> HorizontalScrollButton::make(const WzString& c, iV_fonts buttonFontID)
{
	class make_shared_enabler: public HorizontalScrollButton {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize(c, buttonFontID);
	return widget;
}

void HorizontalScrollButton::initialize(const WzString& c, iV_fonts buttonFontID)
{
	wzText.setText(c, buttonFontID);
}

int32_t HorizontalScrollButton::idealWidth()
{
	return (horizontalPadding * 2) + wzText.width();
}

int32_t HorizontalScrollButton::idealHeight()
{
	return (verticalPadding * 2) + wzText.lineSize();
}

void HorizontalScrollButton::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int w = width();
	int h = height();

	bool isDown = (state & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (state & WBUT_DISABLE) != 0;
	bool isHighlight = (state & WBUT_HIGHLIGHT) != 0;

	int backX0 = x0;
	int backY0 = y0;
	int backX1 = x0 + w;
	int backY1 = y0 + h;

	// Draw the background
	if (isDisabled)
	{
		// no background
	}
	else if (isDown)
	{
		pie_UniTransBoxFill(backX0, backY0, backX1, backY1, pal_RGBA(255, 255, 255, 200));
		pie_UniTransBoxFill(backX0 + 1, backY0 + 1, backX1 - 1, backY1 - 1, WZCOL_FORM_DARK);
	}
	else if (isHighlight)
	{
		pie_UniTransBoxFill(backX0, backY0, backX1, backY1, WZCOL_MENU_SCORE_BUILT);
	}
	else
	{
		pie_UniTransBoxFill(backX0, backY0, backX1, backY1, WZCOL_TRANSPARENT_BOX);
		pie_UniTransBoxFill(backX0, backY0, backX1, backY1, WZCOL_TRANSPARENT_BOX);
	}

	// Draw the main text
	PIELIGHT textColor = (!isDisabled && isHighlight) ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM;
	if (isDisabled)
	{
		textColor.byte.a = (textColor.byte.a / 3);
	}

	int textWidth = wzText.width();
	int textX0 = x0 + (w - textWidth) / 2;
	int textY0 = y0 + (h - wzText.lineSize()) / 2 - wzText.aboveBase();
	wzText.render(Vector2f(textX0, textY0), textColor);
}

// MARK: - HorizontalButtonsWidget

class HorizontalButtonsWidget : public WIDGET
{
public:
	typedef std::function<void(const WzString& untranslatedTitle)> OnSelectForm;
protected:
	HorizontalButtonsWidget() { }
	void initialize(const OnSelectForm& onSelectForm, iV_fonts buttonFontID, bool clickLock, int32_t horizontalButtonPadding, int32_t verticalButtonPadding);
public:
	static std::shared_ptr<HorizontalButtonsWidget> make(const OnSelectForm& onSelectForm, iV_fonts buttonFontID, bool clickLock = true, int32_t horizontalButtonPadding = 15, int32_t verticalButtonPadding = 10);
	void addButton(WzString untranslatedTitle);
	void selectItem(size_t buttonIdx);
	void clear();
	void setItemSpacing(int32_t itemSpacing);
	void setVerticalOuterPadding(int32_t padding);
	void setBackgroundColor(optional<PIELIGHT> bckColor);
	void informLanguageDidChange();
protected:
	virtual std::shared_ptr<WIDGET> findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;
	virtual void geometryChanged() override;
	virtual void display(int xOffset, int yOffset) override;
	virtual void displayRecursive(WidgetGraphicsContext const &context) override;
	virtual void highlightLost() override;
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;

	bool capturesMouseDrag(WIDGET_KEY) override;
	void mouseDragged(WIDGET_KEY, W_CONTEXT *start, W_CONTEXT *current) override;
	void released(W_CONTEXT *context, WIDGET_KEY key) override;
private:
	void updateLayout();
	void resizeChildren();
	void setCurrentXScrollPos(int32_t val);
	void scrollEnsureButtonVisible(size_t buttonIdx);
	size_t currentLeftMostFullyVisibleButtonIdx() const;
	size_t currentRightMostFullyVisibleButtonIdx() const;
	std::shared_ptr<WIDGET> processInternalButtonOver(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed);
	void updateScrollButtonsState();
private:
	OnSelectForm onSelectForm;
	iV_fonts buttonFontID;
	bool clickLock = false;
	int32_t horizontalButtonPadding = 15;
	int32_t verticalButtonPadding = 10;
	int32_t verticalOuterPadding = 0;
	optional<PIELIGHT> backgroundColor = nullopt;
	std::vector<WzString> optionsFormUntranslatedTitles;
	std::shared_ptr<ClipRectWidget> clipContainer;
	std::vector<std::shared_ptr<WzClippableButton>> optionsTitles;
	optional<size_t> currentButtonIdx = nullopt;
	std::shared_ptr<WIDGET> lastMouseOverInternalWidget;
	std::shared_ptr<HorizontalScrollButton> scrollLeftButton;
	std::shared_ptr<HorizontalScrollButton> scrollRightButton;
	int32_t scrollableWidth = 0;
	int32_t maxItemHeight = 0;
	int32_t itemSpacing = 0;
	int32_t clipContainerInnerPaddingX = 28;
	int32_t currentXScrollPos = 0;
	optional<int32_t> dragStartXScrollPos = nullopt;
	bool isHandlingDrag = false;
	bool layoutDirty = true;
};

std::shared_ptr<HorizontalButtonsWidget> HorizontalButtonsWidget::make(const OnSelectForm& onSelectForm, iV_fonts buttonFontID, bool clickLock, int32_t horizontalButtonPadding, int32_t verticalButtonPadding)
{
	ASSERT_OR_RETURN(nullptr, onSelectForm != nullptr, "Missing expected onSelectForm function");
	class make_shared_enabler: public HorizontalButtonsWidget {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize(onSelectForm, buttonFontID, clickLock, horizontalButtonPadding, verticalButtonPadding);
	return widget;
}

void HorizontalButtonsWidget::initialize(const OnSelectForm& onSelectForm_, iV_fonts buttonFontID_, bool clickLock_, int32_t horizontalButtonPadding_, int32_t verticalButtonPadding_)
{
	onSelectForm = onSelectForm_;
	buttonFontID = buttonFontID_;
	clickLock = clickLock_;
	horizontalButtonPadding = horizontalButtonPadding_;
	verticalButtonPadding = verticalButtonPadding_;

	clipContainer = std::make_shared<ClipRectWidget>();
	attach(clipContainer);

	scrollLeftButton = HorizontalScrollButton::make("«", font_medium_bold);
	attach(scrollLeftButton, ChildZPos::Front);
	scrollLeftButton->hide();

	scrollRightButton = HorizontalScrollButton::make("»", font_medium_bold);
	attach(scrollRightButton, ChildZPos::Front);
	scrollRightButton->hide();

	auto weakSelf = std::weak_ptr<HorizontalButtonsWidget>(std::static_pointer_cast<HorizontalButtonsWidget>(shared_from_this()));
	scrollLeftButton->addOnClickHandler([weakSelf](W_BUTTON&){
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "Null pointer");
		size_t currLeftMostFullButtonIdx = strongSelf->currentLeftMostFullyVisibleButtonIdx();
		if (currLeftMostFullButtonIdx > 0)
		{
			strongSelf->scrollEnsureButtonVisible(currLeftMostFullButtonIdx - 1);
		}
	});
	scrollRightButton->addOnClickHandler([weakSelf](W_BUTTON&){
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "Null pointer");
		size_t currRightMostFullButtonIdx = strongSelf->currentRightMostFullyVisibleButtonIdx();
		if (currRightMostFullButtonIdx + 1 < strongSelf->optionsTitles.size())
		{
			strongSelf->scrollEnsureButtonVisible(currRightMostFullButtonIdx + 1);
		}
	});
}

size_t HorizontalButtonsWidget::currentLeftMostFullyVisibleButtonIdx() const
{
	for (size_t i = 0; i < optionsTitles.size(); ++i)
	{
		int32_t buttonXPos = optionsTitles[i]->x();
		int32_t minRequiredVisibleXPos = buttonXPos - clipContainerInnerPaddingX;
		if (currentXScrollPos <= minRequiredVisibleXPos)
		{
			return i;
		}
	}
	return optionsTitles.size();
}

size_t HorizontalButtonsWidget::currentRightMostFullyVisibleButtonIdx() const
{
	if (optionsTitles.empty())
	{
		return 0;
	}
	for (size_t i = optionsTitles.size() - 1; i > 0; --i)
	{
		int32_t buttonXPos = optionsTitles[i]->x();
		int32_t buttonWidth = optionsTitles[i]->width();
		int32_t maxXPosVisible = currentXScrollPos + width();
		int32_t maxDesiredXPosVisible = (buttonXPos + buttonWidth + clipContainerInnerPaddingX);
		if (maxXPosVisible >= maxDesiredXPosVisible)
		{
			return i;
		}
	}
	return 0;
}

void HorizontalButtonsWidget::addButton(WzString untranslatedTitle)
{
	size_t newButtonIdx = optionsFormUntranslatedTitles.size();

	optionsFormUntranslatedTitles.push_back(untranslatedTitle);

	auto but = std::make_shared<WzClippableButton>();
	but->setPadding(horizontalButtonPadding, verticalButtonPadding);
	but->setFont(buttonFontID);
	but->setString(gettext(untranslatedTitle.toUtf8().c_str()));

	auto weakSelf = std::weak_ptr<HorizontalButtonsWidget>(std::static_pointer_cast<HorizontalButtonsWidget>(shared_from_this()));
	but->addOnClickHandler([weakSelf, newButtonIdx](W_BUTTON& but) {
		widgScheduleTask([weakSelf, newButtonIdx]() {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "Null pointer?");
			strongSelf->selectItem(newButtonIdx);
		});
	});

	clipContainer->attach(but);
	optionsTitles.emplace_back(but);
	layoutDirty = true;
}

void HorizontalButtonsWidget::informLanguageDidChange()
{
	// Must re-set all of the button text strings, and recalculate layout
	for (size_t i = 0; i < optionsFormUntranslatedTitles.size(); ++i)
	{
		optionsTitles[i]->setString(gettext(optionsFormUntranslatedTitles[i].toUtf8().c_str()));
	}
	layoutDirty = true;
}

void HorizontalButtonsWidget::selectItem(size_t buttonIdx)
{
	ASSERT_OR_RETURN(, buttonIdx < optionsFormUntranslatedTitles.size(), "Invalid buttonIdx: %zu", buttonIdx);
	currentButtonIdx = buttonIdx;

	onSelectForm(optionsFormUntranslatedTitles[buttonIdx]);

	if (clickLock)
	{
		for (size_t i = 0; i < optionsTitles.size(); ++i)
		{
			optionsTitles[i]->setState((i == buttonIdx) ? WBUT_CLICKLOCK : 0);
		}
	}

	scrollEnsureButtonVisible(buttonIdx);
}

void HorizontalButtonsWidget::clear()
{
	optionsFormUntranslatedTitles.clear();
	clipContainer->removeAllChildren();
	optionsTitles.clear();
	currentButtonIdx.reset();
	lastMouseOverInternalWidget.reset();
	currentXScrollPos = 0;
	layoutDirty = true;
}

void HorizontalButtonsWidget::setItemSpacing(int32_t spacing)
{
	itemSpacing = spacing;
	layoutDirty = true;
}

void HorizontalButtonsWidget::setVerticalOuterPadding(int32_t padding)
{
	verticalOuterPadding = padding;
	layoutDirty = true;
}

void HorizontalButtonsWidget::setBackgroundColor(optional<PIELIGHT> color)
{
	backgroundColor = color;
}

void HorizontalButtonsWidget::scrollEnsureButtonVisible(size_t buttonIdx)
{
	// scroll if needed to ensure that the full button is visible
	int32_t buttonXPos = optionsTitles[buttonIdx]->x();
	int32_t buttonWidth = optionsTitles[buttonIdx]->width();
	int32_t minRequiredVisibleXPos = buttonXPos - clipContainerInnerPaddingX;
	if (currentXScrollPos > minRequiredVisibleXPos)
	{
		setCurrentXScrollPos(minRequiredVisibleXPos);
	}
	else
	{
		int w = width();
		int32_t maxXPosVisible = currentXScrollPos + w - clipContainerInnerPaddingX;
		int32_t maxDesiredXPosVisible = (buttonXPos + buttonWidth + clipContainerInnerPaddingX);
		if (maxXPosVisible < maxDesiredXPosVisible)
		{
			setCurrentXScrollPos(currentXScrollPos + (maxDesiredXPosVisible - maxXPosVisible - clipContainerInnerPaddingX));
		}
	}
}

void HorizontalButtonsWidget::resizeChildren()
{
	scrollableWidth = 0;
	maxItemHeight = 0;
	int32_t nextXOffset = clipContainerInnerPaddingX;
	int32_t itemHeight = height() - (verticalOuterPadding * 2);
	for (auto& child : clipContainer->children())
	{
		if (!clipContainer->visible())
		{
			continue;
		}
		child->setGeometry(nextXOffset, 0, child->idealWidth(), itemHeight);
		scrollableWidth = nextXOffset + child->idealWidth();
		maxItemHeight = std::max(maxItemHeight, child->idealHeight());
		nextXOffset = scrollableWidth + itemSpacing;
	}
	scrollableWidth += clipContainerInnerPaddingX;
}

void HorizontalButtonsWidget::displayRecursive(WidgetGraphicsContext const &context)
{
	updateLayout();
	WIDGET::displayRecursive(context);
}

void HorizontalButtonsWidget::updateLayout()
{
	if (!layoutDirty) {
		return;
	}
	layoutDirty = false;
	resizeChildren();

	int h = height();
	int w = width();

	int clipContainerWidth = std::min<int>(w, scrollableWidth);
	int clipContainerX0 = (w - clipContainerWidth) / 2;
	int clipContainerX1 = clipContainerX0 + clipContainerWidth;
	int clipContainerHeight = h - (verticalOuterPadding * 2);
	clipContainer->setGeometry(clipContainerX0, verticalOuterPadding, clipContainerWidth, clipContainerHeight);

	scrollLeftButton->setGeometry(clipContainerX0, 0, clipContainerInnerPaddingX, h);
	scrollRightButton->setGeometry(clipContainerX1 - clipContainerInnerPaddingX, 0, clipContainerInnerPaddingX, h);

	if (currentButtonIdx.has_value())
	{
		scrollEnsureButtonVisible(currentButtonIdx.value());
	}

	updateScrollButtonsState();
}

void HorizontalButtonsWidget::geometryChanged()
{
	layoutDirty = true;
}

void HorizontalButtonsWidget::display(int xOffset, int yOffset)
{
	if (backgroundColor.has_value())
	{
		int x0 = x() + xOffset;
		int y0 = y() + yOffset;

		pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), backgroundColor.value());
	}
}

bool HorizontalButtonsWidget::capturesMouseDrag(WIDGET_KEY)
{
	return true;
}

/* How far the mouse has to move to start a drag */
constexpr int HorizontalButtonsWidgetDragThreshold = 5;

void HorizontalButtonsWidget::mouseDragged(WIDGET_KEY wkey, W_CONTEXT *psStartContext, W_CONTEXT *psContext)
{
	if (wkey != WKEY_PRIMARY)
	{
		return;
	}

	if (!isHandlingDrag)
	{
		if (ABSDIF(psStartContext->mx, psContext->mx) > HorizontalButtonsWidgetDragThreshold ||
			ABSDIF(psStartContext->my, psContext->my) > HorizontalButtonsWidgetDragThreshold)
		{
			// start processing as drag
			isHandlingDrag = true;
			dragStartXScrollPos = currentXScrollPos;
		}
	}

	if (isHandlingDrag)
	{
		int32_t deltaDragX = psContext->mx - psStartContext->mx;
		setCurrentXScrollPos(dragStartXScrollPos.value() - deltaDragX);
	}
}

void HorizontalButtonsWidget::released(W_CONTEXT *context, WIDGET_KEY key)
{
	if (!isHandlingDrag)
	{
		// Must forward this event to child buttons, as we aren't processing a drag (yet)
		// but widgRunScreen has custom handling when a widget captures mouse drag
		// (which this does) and won't ultimately call findMouseTargetRecursive, but instead
		// just mouseDragged() and released() on the capture widget.
		lastMouseOverInternalWidget = processInternalButtonOver(context, key, false);
	}
	isHandlingDrag = false;
	dragStartXScrollPos.reset();
}

void HorizontalButtonsWidget::setCurrentXScrollPos(int32_t val)
{
	int maxScrollXPos = (scrollableWidth > width()) ? scrollableWidth - width() : 0;
	if (maxScrollXPos > 0)
	{
		currentXScrollPos = std::clamp<int32_t>(val, 0, maxScrollXPos);
	}
	else
	{
		currentXScrollPos = 0;
	}
	clipContainer->setLeftOffset(currentXScrollPos);

	// Update scroll side buttons state
	updateScrollButtonsState();
}

void HorizontalButtonsWidget::updateScrollButtonsState()
{
	int maxScrollXPos = (scrollableWidth > width()) ? scrollableWidth - width() : 0;

	bool leftScrollButtonDisabled = (currentXScrollPos <= 0);
	bool rightScrollButtonDisabled = (currentXScrollPos >= maxScrollXPos);

	if (leftScrollButtonDisabled)
	{
		scrollLeftButton->setState(WBUT_DISABLE);
	}
	else if (scrollLeftButton->getState() & WBUT_DISABLE)
	{
		scrollLeftButton->setState(0);
	}

	if (rightScrollButtonDisabled)
	{
		scrollRightButton->setState(WBUT_DISABLE);
	}
	else if (scrollRightButton->getState() & WBUT_DISABLE)
	{
		scrollRightButton->setState(0);
	}

	bool buttonsVisible = (!leftScrollButtonDisabled || !rightScrollButtonDisabled);
	scrollLeftButton->show(buttonsVisible);
	scrollRightButton->show(buttonsVisible);
}

std::shared_ptr<WIDGET> HorizontalButtonsWidget::findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	setCurrentXScrollPos(currentXScrollPos + (getMouseWheelSpeed().x * 10));

	// skip processing of children - just process clicks for ourself

	if (!visible() || transparentToMouse())
	{
		return nullptr;
	}

	// however, forward certain events (highlight, click) to buttons if not dragging
	if (!isHandlingDrag)
	{
		lastMouseOverInternalWidget = processInternalButtonOver(psContext, key, wasPressed);
	}
	else
	{
		if (lastMouseOverInternalWidget)
		{
			lastMouseOverInternalWidget->highlightLost();
		}
		lastMouseOverInternalWidget = nullptr;
	}

	return shared_from_this();
}

void HorizontalButtonsWidget::highlightLost()
{
	if (lastMouseOverInternalWidget)
	{
		if (lastMouseOverInternalWidget != shared_from_this())
		{
			lastMouseOverInternalWidget->highlightLost();
		}
		lastMouseOverInternalWidget = nullptr;
	}
}

std::shared_ptr<WIDGET> HorizontalButtonsWidget::processInternalButtonOver(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	ASSERT_OR_RETURN(nullptr, psContext != nullptr, "Null context?");

	W_CONTEXT mouseOverWidgetCtx(psContext);
	auto mouseOverWidget = WIDGET::findMouseTargetRecursive(&mouseOverWidgetCtx, key, wasPressed);

	if (!mouseOverWidget || mouseOverWidget == shared_from_this())
	{
		if (lastMouseOverInternalWidget && (lastMouseOverInternalWidget != mouseOverWidget))
		{
			lastMouseOverInternalWidget->highlightLost();
		}
		return nullptr;
	}

	if (mouseOverWidget != lastMouseOverInternalWidget)
	{
		if (lastMouseOverInternalWidget)
		{
			lastMouseOverInternalWidget->highlightLost();
		}
		mouseOverWidget->highlight(&mouseOverWidgetCtx);
	}

	if (key != WKEY_NONE)
	{
		if (wasPressed)
		{
			mouseOverWidget->clicked(&mouseOverWidgetCtx, key);
		}
		else
		{
			mouseOverWidget->released(&mouseOverWidgetCtx, key);
		}
	}
	return mouseOverWidget;
}

int32_t HorizontalButtonsWidget::idealWidth()
{
	updateLayout();
	return scrollableWidth;
}

int32_t HorizontalButtonsWidget::idealHeight()
{
	updateLayout();
	return maxItemHeight + (verticalOuterPadding * 2);
}

// MARK: - OptionsBrowserForm

OptionsBrowserForm::OptionsBrowserForm()
{ }

void OptionsBrowserForm::initialize(const std::shared_ptr<WIDGET>& optionalBackButton_)
{
	optionalBackButton = optionalBackButton_;
	if (optionalBackButton)
	{
		attach(optionalBackButton);
	}

	auto weakSelf = std::weak_ptr<OptionsBrowserForm>(std::static_pointer_cast<OptionsBrowserForm>(shared_from_this()));
	auto formSwitcher = HorizontalButtonsWidget::make([weakSelf](const WzString& untranslatedTitle) {
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "No self?");
		strongSelf->displayOptionsForm(untranslatedTitle);
	}, font_medium_bold, true, 12, 7);
	formSwitcher->setItemSpacing(3);
	formSwitcher->setVerticalOuterPadding(7);
	attach(formSwitcher);
	optionsFormSwitcher = formSwitcher;

	auto sectionSwitcher = HorizontalButtonsWidget::make([weakSelf](const WzString& sectionId) {
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "No self?");
		strongSelf->displayOptionsFormSection(sectionId);
	}, font_regular, false, 9, 5);
	sectionSwitcher->setItemSpacing(1);
	sectionSwitcher->setVerticalOuterPadding(6);
	sectionSwitcher->setBackgroundColor(pal_RGBA(0,0,0,65));
	attach(sectionSwitcher);
	optionsFormSectionSwitcher = sectionSwitcher;

	auto openConfigurationDirectoryLinkTmp = std::make_shared<WzClippableButton>();
	openConfigurationDirectoryLinkTmp->setFont(font_small);
	openConfigurationDirectoryLinkTmp->setString(_("Open Configuration Directory"));
	openConfigurationDirectoryLinkTmp->setPadding(6, 6);
	attach(openConfigurationDirectoryLinkTmp);
	openConfigurationDirectoryLinkTmp->setGeometry(0, 0, openConfigurationDirectoryLinkTmp->width(), iV_GetTextLineSize(openConfigurationDirectoryLinkTmp->FontID) + 8);
	openConfigurationDirectoryLinkTmp->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = psWidget->parent();
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
		int h = psParent->height();
		int y0 = h - psWidget->height();
		psWidget->setGeometry(10, y0, psWidget->width(), psWidget->height());
	}));
	openConfigurationDirectoryLinkTmp->addOnClickHandler([](W_BUTTON&) {
		widgScheduleTask([]() {
			if (!openFolderInDefaultFileManager(PHYSFS_getWriteDir()))
			{
				// Failed to open write dir in default filesystem browser
				std::string configErrorMessage = _("Failed to open configuration directory in system default file browser.");
				configErrorMessage += "\n\n";
				configErrorMessage += _("Configuration directory is reported as:");
				configErrorMessage += "\n";
				configErrorMessage += PHYSFS_getWriteDir();
				configErrorMessage += "\n\n";
				configErrorMessage += _("If running inside a container / isolated environment, this may differ from the actual path on disk.");
				configErrorMessage += "\n";
				configErrorMessage += _("Please see the documentation for more information on how to locate it manually.");
				wzDisplayDialog(Dialog_Warning, _("Failed to open configuration directory"), configErrorMessage.c_str());
			}
		});
	});
	openConfigurationDirectoryLink = openConfigurationDirectoryLinkTmp;
}

void OptionsBrowserForm::showOpenConfigDirLink(bool show)
{
	if (openConfigurationDirectoryLink)
	{
		openConfigurationDirectoryLink->show(show);
	}
}

std::shared_ptr<OptionsBrowserForm> OptionsBrowserForm::make(const std::shared_ptr<WIDGET>& optionalBackButton)
{
	class make_shared_enabler: public OptionsBrowserForm {};
	auto widget = std::make_shared<make_shared_enabler>();
	widget->initialize(optionalBackButton);
	return widget;
}

void OptionsBrowserForm::addOptionsForm(OptionsBrowserForm::Modes mode, const CreateOptionsFormFunc& createFormFunc, const WzString& untranslatedTitle)
{
	auto switcher = std::static_pointer_cast<HorizontalButtonsWidget>(optionsFormSwitcher);
	switcher->addButton(untranslatedTitle);
	optionsForms.push_back({ mode, untranslatedTitle, createFormFunc, nullptr });
	if (!currentOptionsForm.has_value())
	{
		switcher->selectItem(0);
	}
}

void OptionsBrowserForm::displayOptionsForm(size_t idx)
{
	ASSERT_OR_RETURN(, idx < optionsForms.size(), "Invalid index: %zu", idx);
	if (currentOptionsForm.has_value())
	{
		const auto& oldForm = optionsForms[currentOptionsForm.value()].optionsForm;
		if (oldForm)
		{
			detach(oldForm);
		}
	}

	currentOptionsForm = idx;
	bool alreadyHadForm = true;
	if (!optionsForms[idx].optionsForm)
	{
		alreadyHadForm = false;
		optionsForms[idx].optionsForm = optionsForms[idx].createFormFunc();
		optionsForms[idx].optionsForm->setMaxListWidth(maxOptionsFormWidth);
	}
	attach(optionsForms[idx].optionsForm);
	if (alreadyHadForm)
	{
		// since the display scale may have changed (due to edits on the one option screen *or* various other conditions)
		// and non-attached options forms would not have received the screenSizeDidChange event, trigger it manually now
		optionsForms[idx].optionsForm->screenSizeDidChange(screenWidth, screenHeight, screenWidth, screenHeight);

		// trigger a refresh + row layout recalc
		optionsForms[idx].optionsForm->refreshOptions(true);
	}

	updateSectionSwitcher();

	positionCurrentOptionsForm();
}

void OptionsBrowserForm::updateSectionSwitcher()
{
	auto sectionSwitcher = std::static_pointer_cast<HorizontalButtonsWidget>(optionsFormSectionSwitcher);

	if (!currentOptionsForm.has_value())
	{
		sectionSwitcher->clear();
		sectionSwitcher->hide();
		return;
	}

	sectionSwitcher->clear();
	auto optionsFormSections = optionsForms[currentOptionsForm.value()].optionsForm->getSectionInfos();
	if (!optionsFormSections.empty())
	{
		for (const auto& section : optionsFormSections)
		{
			sectionSwitcher->addButton(section.sectionId());
		}
		sectionSwitcher->show();
	}
	else
	{
		sectionSwitcher->hide();
	}
}

bool OptionsBrowserForm::displayOptionsForm(const WzString& untranslatedTitle)
{
	auto it = std::find_if(optionsForms.begin(), optionsForms.end(), [untranslatedTitle](const auto& a){
		return a.untranslatedTitle == untranslatedTitle;
	});
	if (it == optionsForms.end())
	{
		return false;
	}
	size_t index = std::distance(optionsForms.begin(), it);
	displayOptionsForm(index);
	return true;
}

bool OptionsBrowserForm::displayOptionsFormSection(const WzString& sectionId)
{
	if (!currentOptionsForm.has_value())
	{
		return false;
	}
	return optionsForms[currentOptionsForm.value()].optionsForm->jumpToSectionId(sectionId);
}

void OptionsBrowserForm::positionCurrentOptionsForm()
{
	int w = width();
	int h = height();

	int bottomOfLastWidget = optionsFormSwitcher->y() + optionsFormSwitcher->height();

	if (optionsFormSectionSwitcher->visible())
	{
		int switcherWidth = w;
		int switcherX0 = 0;
		optionsFormSectionSwitcher->setGeometry(switcherX0, bottomOfLastWidget, switcherWidth, optionsFormSectionSwitcher->idealHeight());
		bottomOfLastWidget = optionsFormSectionSwitcher->y() + optionsFormSectionSwitcher->height();
	}

	int formY0 = bottomOfLastWidget + paddingBeforeOptionsForm;
	int formAvailableHeight = h;
	if (openConfigurationDirectoryLink && openConfigurationDirectoryLink->visible())
	{
		formAvailableHeight = openConfigurationDirectoryLink->y();
	}
	formAvailableHeight -= formY0 + bottomPadding;

	if (currentOptionsForm.has_value())
	{
		const auto& current = optionsForms[currentOptionsForm.value()].optionsForm;
		ASSERT_OR_RETURN(, current != nullptr, "Options form not instantiated?");
		int formWidth = w - 10;
		int formX0 = (w - formWidth) / 2;
		current->setGeometry(formX0, formY0, formWidth, formAvailableHeight);
	}
}

void OptionsBrowserForm::geometryChanged()
{
	int w = width();
	int h = height();

	int availableFormSwitcherWidth = w;
	int minSwitcherX0 = 0;
	if (optionalBackButton)
	{
		// position to left of the form switcher
		optionalBackButton->setGeometry(0, 0, optionalBackButton->idealWidth(), optionalBackButton->idealHeight());
		minSwitcherX0 = optionalBackButton->x() + optionalBackButton->width();
		availableFormSwitcherWidth -= minSwitcherX0;
	}

	int switcherWidth = std::min(optionsFormSwitcher->idealWidth(), availableFormSwitcherWidth);
	int switcherX0 = std::max<int32_t>((availableFormSwitcherWidth - switcherWidth) / 2, minSwitcherX0);
	optionsFormSwitcher->setGeometry(switcherX0, topPadding, switcherWidth, optionsFormSwitcher->idealHeight());

	if (openConfigurationDirectoryLink)
	{
		int configDirLinkHeight = openConfigurationDirectoryLink->idealHeight();
		int configDirLinkY0 = h - configDirLinkHeight - 1;
		openConfigurationDirectoryLink->setGeometry(leftOpenConfigDirLinkPadding, configDirLinkY0, openConfigurationDirectoryLink->idealWidth(), configDirLinkHeight);
	}

	positionCurrentOptionsForm();
}

void OptionsBrowserForm::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int h = height();

	pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + h, pal_RGBA(150,150,150,50));
	pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + h, pal_RGBA(0,0,0,70));
	pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + h, WZCOL_TRANSPARENT_BOX);

	// Draw a darker border underneath where the switcher is
	int switcherY0 = optionsFormSwitcher->y();
	int switcherY1 = switcherY0 + optionsFormSwitcher->height();
	pie_UniTransBoxFill(x0, y0 + switcherY0, x0 + width(), y0 + switcherY1, WZCOL_TRANSPARENT_BOX);
	pie_UniTransBoxFill(x0, y0 + switcherY0, x0 + width(), y0 + switcherY1, pal_RGBA(0,0,0,128));

	if (openConfigurationDirectoryLink && openConfigurationDirectoryLink->visible())
	{
		int configDirLinkY0 = openConfigurationDirectoryLink->y();
		int configDirLinkY1 = h;
		pie_UniTransBoxFill(x0, y0 + configDirLinkY0, x0 + width(), y0 + configDirLinkY1, WZCOL_TRANSPARENT_BOX);
		pie_UniTransBoxFill(x0, y0 + configDirLinkY0, x0 + width(), y0 + configDirLinkY1, pal_RGBA(0,0,0,128));
	}
}

#define MIN_IDEAL_OPTIONS_BROWSER_FORM_WIDTH 640
#define MIN_IDEAL_OPTIONS_BROWSER_FORM_HEIGHT 640

int32_t OptionsBrowserForm::idealWidth()
{
	return std::max(maxOptionsFormWidth, MIN_IDEAL_OPTIONS_BROWSER_FORM_WIDTH);
}

int32_t OptionsBrowserForm::idealHeight()
{
	int minCalculatedIdealHeight = (topPadding + bottomPadding) + optionsFormSwitcher->idealHeight() + paddingBeforeOptionsForm + 300;
	return std::max(minCalculatedIdealHeight, MIN_IDEAL_OPTIONS_BROWSER_FORM_HEIGHT);
}

bool OptionsBrowserForm::switchToOptionsForm(OptionsBrowserForm::Modes mode)
{
	auto it = std::find_if(optionsForms.begin(), optionsForms.end(), [mode](const OptionsFormDetails& details) { return details.mode == mode; });
	if (it == optionsForms.end())
	{
		return false;
	}
	size_t index = std::distance(optionsForms.begin(), it);
	auto switcher = std::static_pointer_cast<HorizontalButtonsWidget>(optionsFormSwitcher);
	switcher->selectItem(index);
	return true;
}

void OptionsBrowserForm::informLanguageDidChange()
{
	// Refresh switchers
	auto switcher = std::static_pointer_cast<HorizontalButtonsWidget>(optionsFormSwitcher);
	switcher->informLanguageDidChange();
	auto sectionSwitcher = std::static_pointer_cast<HorizontalButtonsWidget>(optionsFormSectionSwitcher);
	sectionSwitcher->informLanguageDidChange();

	// Refresh open configuration directory link text
	openConfigurationDirectoryLink->setString(_("Open Configuration Directory"));

	// Trigger layout recalc
	geometryChanged();
}

// MARK: -

std::shared_ptr<OptionsBrowserForm> createOptionsBrowser(bool inGame, const std::shared_ptr<WIDGET>& optionalBackButton)
{
	auto result = OptionsBrowserForm::make(optionalBackButton);
	result->showOpenConfigDirLink(!inGame);

	auto weakOptionsBrowserForm = std::weak_ptr<OptionsBrowserForm>(result);
	auto informOnLanguageChangeHandler = [weakOptionsBrowserForm]() {
		auto strongOptionsBrowserForm = weakOptionsBrowserForm.lock();
		ASSERT_OR_RETURN(, strongOptionsBrowserForm != nullptr, "No parent?");
		strongOptionsBrowserForm->informLanguageDidChange();
	};

	result->addOptionsForm(
		OptionsBrowserForm::Modes::Interface,
		[informOnLanguageChangeHandler]() { return makeInterfaceOptionsForm(informOnLanguageChangeHandler); },
		N_("Interface")
	);
	if (!inGame)
	{
		result->addOptionsForm(OptionsBrowserForm::Modes::Defaults, makeDefaultsOptionsForm, N_("Defaults"));
	}
	result->addOptionsForm(OptionsBrowserForm::Modes::Graphics, makeGraphicsOptionsForm, N_("Graphics"));
	result->addOptionsForm(OptionsBrowserForm::Modes::Audio, makeAudioOptionsForm, N_("Audio"));
	result->addOptionsForm(OptionsBrowserForm::Modes::Controls, makeControlsOptionsForm, N_("Controls"));
	result->addOptionsForm(OptionsBrowserForm::Modes::Window, makeWindowOptionsForm, N_("Window"));

	return result;
}
