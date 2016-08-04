/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
 *  Functions for the button widget
 */

#include "lib/framework/frame.h"
#include "widget.h"
#include "widgint.h"
#include "button.h"
#if defined(WZ_CC_MSVC)
#include "button_moc.h"		// this is generated on the pre-build event.
#endif
#include "form.h"
#include "tip.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/gamelib/gtime.h"


W_BUTINIT::W_BUTINIT()
	: pText(NULL)
	, FontID(font_regular)
{}

W_BUTTON::W_BUTTON(W_BUTINIT const *init)
	: WIDGET(init, WIDG_BUTTON)
	, state(WBUT_PLAIN)
	, pText(QString::fromUtf8(init->pText))
	, pTip(init->pTip)
	, HilightAudioID(WidgGetHilightAudioID())
	, ClickedAudioID(WidgGetClickedAudioID())
	, AudioCallback(WidgGetAudioCallback())
	, FontID(init->FontID)
{
	ASSERT((init->style & ~(WBUT_PLAIN | WIDG_HIDDEN | WBUT_NOPRIMARY | WBUT_SECONDARY | WBUT_TXTCENTRE)) == 0, "unknown button style");
}

W_BUTTON::W_BUTTON(WIDGET *parent)
	: WIDGET(parent, WIDG_BUTTON)
	, state(WBUT_PLAIN)
	, HilightAudioID(WidgGetHilightAudioID())
	, ClickedAudioID(WidgGetClickedAudioID())
	, AudioCallback(WidgGetAudioCallback())
	, FontID(font_regular)
{}

unsigned W_BUTTON::getState()
{
	return state & (WBUT_DISABLE | WBUT_LOCK | WBUT_CLICKLOCK | WBUT_FLASH | WBUT_DOWN | WBUT_HIGHLIGHT);
}

void W_BUTTON::setFlash(bool enable)
{
	dirty = true;
	if (enable)
	{
		state |= WBUT_FLASH;
	}
	else
	{
		state &= ~WBUT_FLASH;
	}
}

void W_BUTTON::setState(unsigned newState)
{
	ASSERT(!((newState & WBUT_LOCK) && (newState & WBUT_CLICKLOCK)), "Cannot have both WBUT_LOCK and WBUT_CLICKLOCK");

	unsigned mask = WBUT_DISABLE | WBUT_LOCK | WBUT_CLICKLOCK;
	state = (state & ~mask) | (newState & mask);
	dirty = true;
}

QString W_BUTTON::getString() const
{
	return pText;
}

void W_BUTTON::setString(QString string)
{
	pText = string;
	dirty = true;
}

void W_BUTTON::setTip(QString string)
{
	pTip = string;
}

void W_BUTTON::clicked(W_CONTEXT *, WIDGET_KEY key)
{
	dirty = true;

	/* Can't click a button if it is disabled or locked down */
	if ((state & (WBUT_DISABLE | WBUT_LOCK)) == 0)
	{
		// Check this is the correct key
		if ((!(style & WBUT_NOPRIMARY) && key == WKEY_PRIMARY) ||
		    ((style & WBUT_SECONDARY) && key == WKEY_SECONDARY))
		{
			if (AudioCallback)
			{
				AudioCallback(ClickedAudioID);
			}
			state &= ~WBUT_FLASH;	// Stop it flashing
			state |= WBUT_DOWN;
		}
	}

	/* Kill the tip if there is one */
	if (!pTip.isEmpty())
	{
		tipStop(this);
	}
}

/* Respond to a mouse button up */
void W_BUTTON::released(W_CONTEXT *, WIDGET_KEY key)
{
	if (state & WBUT_DOWN)
	{
		// Check this is the correct key
		if ((!(style & WBUT_NOPRIMARY) && key == WKEY_PRIMARY) ||
		    ((style & WBUT_SECONDARY) && key == WKEY_SECONDARY))
		{
			emit clicked();
			screenPointer->setReturn(this);
			state &= ~WBUT_DOWN;
			dirty = true;
		}
	}
}


/* Respond to a mouse moving over a button */
void W_BUTTON::highlight(W_CONTEXT *psContext)
{
	if ((state & WBUT_HIGHLIGHT) == 0)
	{
		state |= WBUT_HIGHLIGHT;
		dirty = true;
	}
	if (AudioCallback)
	{
		AudioCallback(HilightAudioID);
	}
	/* If there is a tip string start the tool tip */
	if (!pTip.isEmpty())
	{
		tipStart(this, pTip, screenPointer->TipFontID, x() + psContext->xOffset, y() + psContext->yOffset, width(), height());
	}
}


/* Respond to the mouse moving off a button */
void W_BUTTON::highlightLost()
{
	state &= ~(WBUT_DOWN | WBUT_HIGHLIGHT);
	dirty = true;
	if (!pTip.isEmpty())
	{
		tipStop(this);
	}
}

void W_BUTTON::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	bool haveText = !pText.isEmpty();

	bool isDown = (state & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (state & WBUT_DISABLE) != 0;
	bool isHighlight = (state & WBUT_HIGHLIGHT) != 0;

	// Display the button.
	if (!images.normal.isNull())
	{
		iV_DrawImage(images.normal, x0, y0);
		if (isDown && !images.down.isNull())
		{
			iV_DrawImage(images.down, x0, y0);
		}
		if (isDisabled && !images.disabled.isNull())
		{
			iV_DrawImage(images.disabled, x0, y0);
		}
		if (isHighlight && !images.highlighted.isNull())
		{
			iV_DrawImage(images.highlighted, x0, y0);
		}
	}
	else
	{
		iV_ShadowBox(x0, y0, x1, y1, 0, WZCOL_FORM_LIGHT, isDisabled ? WZCOL_FORM_LIGHT : WZCOL_FORM_DARK, WZCOL_FORM_BACKGROUND);
		if (isHighlight)
		{
			iV_Box(x0 + 2, y0 + 2, x1 - 3, y1 - 3, WZCOL_FORM_HILITE);
		}
	}

	if (haveText)
	{
		QByteArray textBytes = pText.toUtf8();

		iV_SetFont(FontID);
		int fw = iV_GetTextWidth(textBytes.constData());
		int fx = x0 + (width() - fw) / 2;
		int fy = y0 + (height() - iV_GetTextLineSize()) / 2 - iV_GetTextAboveBase();
		if (isDisabled)
		{
			iV_SetTextColour(WZCOL_FORM_LIGHT);
			iV_DrawText(textBytes.constData(), fx + 1, fy + 1);
			iV_SetTextColour(WZCOL_FORM_DISABLE);
		}
		else
		{
			iV_SetTextColour(WZCOL_FORM_TEXT);
		}
		iV_DrawText(textBytes.constData(), fx, fy);
	}

	if (isDisabled && !images.normal.isNull() && images.disabled.isNull())
	{
		// disabled, render something over it!
		iV_TransBoxFill(x0, y0, x0 + width(), y0 + height());
	}
}

void W_BUTTON::setImages(Images const &images_)
{
	images = images_;
	dirty = true;
	if (!images.normal.isNull())
	{
		setGeometry(x(), y(), images.normal.width(), images.normal.height());
	}
}

void W_BUTTON::setImages(Image image, Image imageDown, Image imageHighlight, Image imageDisabled)
{
	dirty = true;
	setImages(Images(image, imageDown, imageHighlight, imageDisabled));
}

void StateButton::setState(int state)
{
	if (currentState == state)
	{
		return;
	}
	dirty = true;
	currentState = state;
	std::map<int, Images>::const_iterator image = imageSets.find(state);
	if (image != imageSets.end())
	{
		W_BUTTON::setImages(image->second);
	}
	std::map<int, QString>::const_iterator tip = tips.find(state);
	if (tip != tips.end())
	{
		W_BUTTON::setTip(tip->second);
	}
}

void StateButton::setTip(int state, QString string)
{
	tips[state] = string;
	if (currentState == state)
	{
		W_BUTTON::setTip(string);
	}
}

void StateButton::setTip(int state, char const *stringUtf8)
{
	setTip(state, QString::fromUtf8(stringUtf8));
}

void StateButton::setImages(int state, Images const &images)
{
	imageSets[state] = images;
	dirty = true;
	if (currentState == state)
	{
		W_BUTTON::setImages(images);
	}
}
