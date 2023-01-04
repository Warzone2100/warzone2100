/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2021  Warzone 2100 Project

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
 *  Functionality for the form widget.
 */

#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/math_ext.h"
#include "widget.h"
#include "widgint.h"
#include "form.h"
#include "tip.h"
#include "label.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"

#include <array>

#define WZ_FORM_MINIMIZED_LEFT_PADDING 5
#define WZ_FORM_MINIMIZED_MAXBUTTON_RIGHT_PADDING WZ_FORM_MINIMIZED_LEFT_PADDING

W_FORMINIT::W_FORMINIT()
	: disableChildren(false)
{}

W_FORM::W_FORM(W_FORMINIT const *init)
	: WIDGET(init, WIDG_FORM)
	, disableChildren(init->disableChildren)
{
	ASSERT((init->style & ~(WFORM_INVISIBLE | WFORM_CLICKABLE | WFORM_NOCLICKMOVE | WFORM_NOPRIMARY | WFORM_SECONDARY | WIDG_HIDDEN)) == 0, "Unknown style bit");
	ASSERT((init->style & WFORM_INVISIBLE) == 0 || (init->style & WFORM_CLICKABLE) == 0, "Cannot have an invisible clickable form");
	ASSERT((init->style & WFORM_CLICKABLE) != 0 || (init->style & (WFORM_NOPRIMARY | WFORM_SECONDARY)) == 0, "Cannot set keys if the form isn't clickable");
}

W_FORM::W_FORM()
	: WIDGET(WIDG_FORM)
	, disableChildren(false)
{}

W_CLICKFORM::W_CLICKFORM(W_FORMINIT const *init)
	: W_FORM(init)
	, state(WBUT_PLAIN)
	, pTip(init->pTip)
	, HilightAudioID(WidgGetHilightAudioID())
	, ClickedAudioID(WidgGetClickedAudioID())
	, AudioCallback(WidgGetAudioCallback())
{}

W_CLICKFORM::W_CLICKFORM()
	: W_FORM()
	, state(WBUT_PLAIN)
	, HilightAudioID(WidgGetHilightAudioID())
	, ClickedAudioID(WidgGetClickedAudioID())
	, AudioCallback(WidgGetAudioCallback())
{}

unsigned W_CLICKFORM::getState()
{
	return state & (WBUT_DISABLE | WBUT_LOCK | WBUT_CLICKLOCK | WBUT_FLASH | WBUT_DOWN | WBUT_HIGHLIGHT);
}

void W_CLICKFORM::setState(unsigned newState)
{
	ASSERT(!((newState & WBUT_LOCK) && (newState & WBUT_CLICKLOCK)), "Cannot have both WBUT_LOCK and WBUT_CLICKLOCK");

	unsigned mask = WBUT_DISABLE | WBUT_LOCK | WBUT_CLICKLOCK;
	state = (state & ~mask) | (newState & mask);
	dirty = true;
}

void W_CLICKFORM::setFlash(bool enable)
{
	if (enable)
	{
		state |= WBUT_FLASH;
	}
	else
	{
		state &= ~WBUT_FLASH;
	}
	dirty = true;
}

bool W_FORM::isUserMovable() const
{
	return userMovable || formState == FormState::MINIMIZED;
}

void W_FORM::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	dirty = true;
	if (isUserMovable() && key == WKEY_PRIMARY)
	{
		if (formState == FormState::MINIMIZED && (psContext->mx <= minimizedGeometry().x() + minimizedLeftButtonWidth))
		{
			return;
		}
		dragStart = Vector2i(psContext->mx, psContext->my);
	}
}

void W_FORM::released(W_CONTEXT *psContext, WIDGET_KEY)
{
	if (formState == FormState::MINIMIZED
		&& !dragStart.has_value()	// mousedown was on "re-open" button (drag is only started when mouse is over the rest of the minimized frame)
		&& (psContext->mx <= minimizedGeometry().x() + minimizedLeftButtonWidth)) // mouse is still over "re-open" button
	{
		// clicked + released mouse-button on "re-open" button
		toggleMinimized();
		return;
	}
	if (!isUserMovable() || !dragStart.has_value()) { return; }
	dragStart = nullopt;
	dirty = true;
}

void W_FORM::run(W_CONTEXT *psContext)
{
	if (!userMovable || !dragStart.has_value()) { return; }

	/* If the mouse is released *anywhere*, stop dragging */
	if (mouseReleased(MOUSE_LMB))
	{
		dragStart = nullopt;
		return;
	}

	Vector2i currentMousePos(psContext->mx, psContext->my);
	if (currentMousePos == dragStart.value()) { return; }
	Vector2i dragDelta(currentMousePos.x - dragStart.value().x, currentMousePos.y - dragStart.value().y);

	const WzRect *pRectToMove = &(geometry());
	if (formState == FormState::MINIMIZED)
	{
		pRectToMove = &(minimizedGeometry());
	}
	Vector2i newPosition(pRectToMove->x() + dragDelta.x, pRectToMove->y() + dragDelta.y);

	int maxPossibleX = pie_GetVideoBufferWidth() - pRectToMove->width();
	int maxPossibleY = pie_GetVideoBufferHeight() - pRectToMove->height();
	newPosition.x = clip<int>(newPosition.x, 0, maxPossibleX);
	newPosition.y = clip<int>(newPosition.y, 0, maxPossibleY);
	if (formState != FormState::MINIMIZED)
	{
		move(newPosition.x, newPosition.y);
	}
	else
	{
		minimizedRect = WzRect(newPosition.x, newPosition.y, minimizedRect.width(), minimizedRect.height());
	}
	dirty = true;
	dragStart = currentMousePos;
}

/* Respond to a mouse click */
void W_CLICKFORM::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	W_FORM::clicked(psContext, key);

	// Can't click a button if it is disabled or locked down.
	if ((state & (WBUT_DISABLE | WBUT_LOCK)) == 0)
	{
		// Check this is the correct key
		if ((!(style & WFORM_NOPRIMARY) && key == WKEY_PRIMARY) ||
		    ((style & WFORM_SECONDARY) && key == WKEY_SECONDARY))
		{
			state &= ~WBUT_FLASH;  // Stop it flashing
			state |= WBUT_DOWN;
			dirty = true;

			if (AudioCallback != nullptr)
			{
				AudioCallback(ClickedAudioID);
			}
		}
	}
}

void W_CLICKFORM::released(W_CONTEXT *, WIDGET_KEY key)
{
	if ((state & WBUT_DOWN) != 0)
	{
		// Check this is the correct key.
		if ((!(style & WFORM_NOPRIMARY) && key == WKEY_PRIMARY) ||
		    ((style & WFORM_SECONDARY) && key == WKEY_SECONDARY))
		{
			if (auto lockedScreen = screenPointer.lock())
			{
				lockedScreen->setReturn(shared_from_this());
			}
			state &= ~WBUT_DOWN;
			dirty = true;
		}
	}
}


/* Respond to a mouse moving over a form */
void W_CLICKFORM::highlight(W_CONTEXT *psContext)
{
	state |= WBUT_HIGHLIGHT;

	if (AudioCallback != nullptr)
	{
		AudioCallback(HilightAudioID);
	}
}


/* Respond to the mouse moving off a form */
void W_FORM::highlightLost()
{
	dirty = true;
}

void W_CLICKFORM::highlightLost()
{
	W_FORM::highlightLost();

	state &= ~(WBUT_DOWN | WBUT_HIGHLIGHT);
	dirty = true;
}

void W_FORM::display(int xOffset, int yOffset)
{
	if ((style & WFORM_INVISIBLE) == 0)
	{
		int x0 = x() + xOffset;
		int y0 = y() + yOffset;
		int x1 = x0 + width();
		int y1 = y0 + height();

		iV_ShadowBox(x0, y0, x1, y1, 1, WZCOL_FORM_LIGHT, WZCOL_FORM_DARK, WZCOL_FORM_BACKGROUND);
	}
}

void W_FORM::screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight)
{
	if (formState == FormState::MINIMIZED)
	{
		// must ensure that the minimizedRect is fully within the new screen bounds
		int newMinX0 = std::max(0, std::min(minimizedRect.x(), newWidth - minimizedRect.width()));
		int newMinY0 = std::max(0, std::min(minimizedRect.y(), newHeight - minimizedRect.height()));
		minimizedRect = WzRect(newMinX0, newMinY0, minimizedRect.width(), minimizedRect.height());
	}
	WIDGET::screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
}

bool W_FORM::hitTest(int x, int y)
{
	if (!minimizable || formState != FormState::MINIMIZED)
	{
		return WIDGET::hitTest(x, y);
	}
	// handle: minimized
	return minimizedGeometry().contains(x, y);
}

bool W_FORM::processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	if (!minimizable || formState != FormState::MINIMIZED)
	{
		return WIDGET::processClickRecursive(psContext, key, wasPressed);
	}
	// handle: minimized
	ASSERT(disableChildren, "disableChildren should be set to true while minimized");
	return WIDGET::processClickRecursive(psContext, key, wasPressed);
}

void W_FORM::displayRecursive(WidgetGraphicsContext const &context)
{
	if (!minimizable || formState != FormState::MINIMIZED)
	{
		WIDGET::displayRecursive(context);
		return;
	}

	// handle: minimized
	displayMinimized(context.getXOffset(), context.getYOffset());
}

void W_FORM::displayMinimized(int xOffset, int yOffset)
{
	auto minDim = minimizedGeometry();

	int x0 = minDim.x() + xOffset;
	int y0 = minDim.y() + yOffset;
	int x1 = x0 + minDim.width();
	int y1 = y0 + minDim.height();

	// draw "drop-shadow"
	int dropShadowX0 = std::max<int>(x0 - 5, 0);
	int dropShadowY0 = std::max<int>(y0 - 5, 0);
	int dropShadowX1 = std::min<int>(x1 + 5, pie_GetVideoBufferWidth());
	int dropShadowY1 = std::min<int>(y1 + 5, pie_GetVideoBufferHeight());
	pie_UniTransBoxFill((float)dropShadowX0, (float)dropShadowY0, (float)dropShadowX1, (float)dropShadowY1, pal_RGBA(0, 0, 0, 40));

	// draw background + border
	iV_TransBoxFill((float)x0, (float)y0, (float)x1, (float)y1);
	iV_Box(x0, y0, x1, y1, pal_RGBA(255, 255, 255, 150));

	// draw minimized title
	int titleY0 = y0 + (minDim.height() - minimizedTitle->height()) / 2;
	minimizedTitle->display(x0 + WZ_FORM_MINIMIZED_LEFT_PADDING, titleY0);

	if (isMouseOverWidget() && (mouseX() <= x0 + minimizedLeftButtonWidth))
	{
		// Draw "re-open" button border when mouse is over
		iV_Box(x0 + 1, y0 + 1, x0 + minimizedLeftButtonWidth, y1 - 1, pal_RGBA(255, 255, 255, 150));
	}
}

void W_FORM::enableMinimizing(const std::string& _minimizedTitle, PIELIGHT titleColour)
{
	minimizedTitle = std::make_shared<W_LABEL>();
	minimizedTitle->setFont(font_regular_bold, titleColour);
	WzString titleString = WzString("\u21F2  "); // ⇲
	if (!_minimizedTitle.empty())
	{
		titleString += WzString::fromUtf8(_minimizedTitle);
	}
	else
	{
		titleString += _("(untitled)");
	}
	minimizedLeftButtonWidth = WZ_FORM_MINIMIZED_LEFT_PADDING + iV_GetTextWidth("\u21F2", font_regular_bold) + WZ_FORM_MINIMIZED_MAXBUTTON_RIGHT_PADDING;
	minimizedTitle->setString(titleString);
	minimizedTitle->setGeometry(0, 0, minimizedTitle->getMaxLineWidth(), iV_GetTextLineSize(font_regular_bold));
	minimizable = true;
}

void W_FORM::disableMinimizing()
{
	minimizedTitle.reset();
	minimizable = false;
	if (formState == FormState::MINIMIZED)
	{
		setFormState(FormState::NORMAL);
	}
}

bool W_FORM::setFormState(FormState state)
{
	if (formState == state) { return true; }
	if (formState == FormState::MINIMIZED)
	{
		disableChildren = false;
	}
	switch(state)
	{
		case FormState::NORMAL:
			break;
		case FormState::MINIMIZED:
		{
			if (!minimizable)
			{
				return false;
			}
			auto minimizedSize = calcMinimizedSize();
			minimizedRect = WzRect(x(), y(), minimizedSize.x, minimizedSize.y);
			disableChildren = true;
			break;
		}
	}
	formState = state;
	return true;
}

bool W_FORM::toggleMinimized()
{
	FormState desiredState = FormState::MINIMIZED;
	if (formState == FormState::MINIMIZED)
	{
		desiredState = FormState::NORMAL;
	}
	return setFormState(desiredState);
}

Vector2i W_FORM::calcMinimizedSize() const
{
	Vector2i minimizedSize = {0,0};
	if (!minimizable) { return minimizedSize; }
	// width = width of the minimizedTitle (which includes the "re-open button" as a unicode character) + padding
	if (minimizedTitle != nullptr)
	{
		minimizedSize.x = minimizedTitle->getMaxLineWidth() + 15;
	}
	// height = height of the font's line height + padding
	minimizedSize.y = iV_GetTextLineSize(font_regular_bold) + 6;
	return minimizedSize;
}

const WzRect& W_FORM::minimizedGeometry() const
{
	return minimizedRect;
}

void W_CLICKFORM::setTip(std::string string)
{
	pTip = string;
}

bool W_CLICKFORM::isDown() const
{
	return (state & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
}

bool W_CLICKFORM::isHighlighted() const
{
	return (state & WBUT_HIGHLIGHT) != 0;
}

void W_CLICKFORM::display(int xOffset, int yOffset)
{
	ASSERT(false, "No default implementation exists for click forms");
}

// MARK: - W_FULLSCREENOVERLAY_CLICKFORM

W_FULLSCREENOVERLAY_CLICKFORM::W_FULLSCREENOVERLAY_CLICKFORM(W_FORMINIT const *init) : W_CLICKFORM(init) {}
W_FULLSCREENOVERLAY_CLICKFORM::W_FULLSCREENOVERLAY_CLICKFORM() : W_CLICKFORM() {}

std::shared_ptr<W_FULLSCREENOVERLAY_CLICKFORM> W_FULLSCREENOVERLAY_CLICKFORM::make(UDWORD formID)
{
	W_FORMINIT sInit;
	sInit.id = formID;
	sInit.style = WFORM_PLAIN | WFORM_CLICKABLE;
	sInit.x = 0;
	sInit.y = 0;
	sInit.width = screenWidth - 1;
	sInit.height = screenHeight - 1;
	sInit.calcLayout = LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(0, 0, screenWidth, screenHeight);
	});

	class make_shared_enabler: public W_FULLSCREENOVERLAY_CLICKFORM
	{
	public:
		make_shared_enabler(W_FORMINIT const *init): W_FULLSCREENOVERLAY_CLICKFORM(init) {}
	};
	return std::make_shared<make_shared_enabler>(&sInit);
}

void W_FULLSCREENOVERLAY_CLICKFORM::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	if (onClickedFunc)
	{
		onClickedFunc();
	}
}

void W_FULLSCREENOVERLAY_CLICKFORM::display(int xOffset, int yOffset)
{
	if (!visible())
	{
		// skip if hidden
		return;
	}

	if (backgroundColor.rgba == 0)
	{
		return;
	}

	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	auto strongCutoutWidget = cutoutWidget.lock();
	WzRect screenRect = screenGeometry();
	WzRect cutoutRect = (strongCutoutWidget) ? screenRect.intersectionWith(strongCutoutWidget->screenGeometry()) : WzRect();

	if (cutoutRect.width() <= 0 || cutoutRect.height() <= 0)
	{
		// simple path - draw background over everything
		pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), backgroundColor);
	}
	else
	{
		std::array<WzRect, 4> surroundingRects = {
			// left column until left edge of cutout rect
			WzRect({screenRect.left(), screenRect.top()}, {cutoutRect.left(), screenRect.bottom()}),
			// top band above cutout rect
			WzRect({cutoutRect.left(), screenRect.top()}, {cutoutRect.right(), cutoutRect.top()}),
			// right column after cutout rect
			WzRect({cutoutRect.right(), screenRect.top()}, {screenRect.right(), screenRect.bottom()}),
			// bottom band below cutout rect
			WzRect({cutoutRect.left(), cutoutRect.bottom()}, {cutoutRect.right(), screenRect.bottom()})
		};

		for (const auto& rect : surroundingRects)
		{
			if (rect.width() <= 0 || rect.height() <= 0)
			{
				continue;
			}
			pie_UniTransBoxFill(rect.left(), rect.top(), rect.right(), rect.bottom(), backgroundColor);
		}
	}
}

void W_FULLSCREENOVERLAY_CLICKFORM::run(W_CONTEXT *psContext)
{
	if (keyPressed(KEY_ESC))
	{
		if (onCancelPressed)
		{
			onCancelPressed();
		}
	}
	inputLoseFocus();	// clear the input buffer.
}

void displayChildDropShadows(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	PIELIGHT dropShadowColor = pal_RGBA(0, 0, 0, 40);
	const int widerPadding = 4;
	const int closerPadding = 2;
	int childXOffset = psWidget->x() + xOffset;
	int childYOffset = psWidget->y() + yOffset;
	for (auto& child : psWidget->children())
	{
		if (!child->visible()) { continue; }
		int childX0 = child->x() + childXOffset;
		int childY0 = child->y() + childYOffset;
		int childDropshadowWiderX0 = std::max(childX0 - widerPadding, 0);
		int childDropshadowWiderX1 = std::min(childX0 + child->width() + widerPadding, pie_GetVideoBufferWidth());
		int childDropshadowWiderY1 = std::min(childY0 + child->height() + widerPadding, pie_GetVideoBufferHeight());
		int childDropshadowCloserX0 = std::max(childX0 - closerPadding, 0);
		int childDropshadowCloserX1 = std::min(childX0 + child->width() + closerPadding, pie_GetVideoBufferWidth());
		int childDropshadowCloserY1 = std::min(childY0 + child->height() + closerPadding, pie_GetVideoBufferHeight());
		pie_UniTransBoxFill(childDropshadowWiderX0, childY0, childDropshadowWiderX1, childDropshadowWiderY1, dropShadowColor);
		pie_UniTransBoxFill(childDropshadowCloserX0, childY0, childDropshadowCloserX1, childDropshadowCloserY1, dropShadowColor);
	}
}
