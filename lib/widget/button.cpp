/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
#include "lib/framework/frameint.h"
#include "widget.h"
#include "widgint.h"
#include "button.h"
#include "form.h"
#include "tip.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/gamelib/gtime.h"


W_BUTINIT::W_BUTINIT()
	: pText(NULL)
	, pTip(NULL)
	, FontID(font_regular)
{}

W_BUTTON::W_BUTTON(W_BUTINIT const *init)
	: WIDGET(init, WIDG_BUTTON)
	, state(WBUT_PLAIN)
	, pText(QString::fromUtf8(init->pText))
	, pTip(QString::fromUtf8(init->pTip))
	, HilightAudioID(WidgGetHilightAudioID())
	, ClickedAudioID(WidgGetClickedAudioID())
	, AudioCallback(WidgGetAudioCallback())
	, FontID(init->FontID)
{
	ASSERT((init->style & ~(WBUT_PLAIN | WIDG_HIDDEN | WBUT_NOPRIMARY | WBUT_SECONDARY | WBUT_TXTCENTRE)) == 0, "unknown button style");
}

unsigned W_BUTTON::getState()
{
	return state & (WBUT_DISABLE | WBUT_LOCK | WBUT_CLICKLOCK | WBUT_FLASH | WBUT_DOWN | WBUT_HIGHLIGHT);
}

void W_BUTTON::setFlash(bool enable)
{
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
}

QString W_BUTTON::getString() const
{
	return pText;
}

void W_BUTTON::setString(QString string)
{
	pText = string;
}

void W_BUTTON::setTip(QString string)
{
	pTip = string;
}

void W_BUTTON::clicked(W_CONTEXT *, WIDGET_KEY key)
{
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
void W_BUTTON::released(W_CONTEXT *psContext, WIDGET_KEY key)
{
	W_SCREEN *psScreen = psContext->psScreen;
	if (state & WBUT_DOWN)
	{
		// Check this is the correct key
		if ((!(style & WBUT_NOPRIMARY) && key == WKEY_PRIMARY) ||
		    ((style & WBUT_SECONDARY) && key == WKEY_SECONDARY))
		{
			widgSetReturn(psScreen, this);
			state &= ~WBUT_DOWN;
		}
	}
}


/* Respond to a mouse moving over a button */
void W_BUTTON::highlight(W_CONTEXT *psContext)
{
	state |= WBUT_HIGHLIGHT;

	if (AudioCallback)
	{
		AudioCallback(HilightAudioID);
	}

	/* If there is a tip string start the tool tip */
	if (!pTip.isEmpty())
	{
		tipStart(this, pTip, psContext->psScreen->TipFontID,
		         psContext->psForm->aColours,
		         x() + psContext->xOffset, y() + psContext->yOffset,
		         width(), height());
	}
}


/* Respond to the mouse moving off a button */
void W_BUTTON::highlightLost()
{
	state &= ~(WBUT_DOWN | WBUT_HIGHLIGHT);
	if (!pTip.isEmpty())
	{
		tipStop(this);
	}
}

void W_BUTTON::display(int xOffset, int yOffset, PIELIGHT *pColours)
{
	if (displayFunction != NULL)
	{
		displayFunction(this, xOffset, yOffset, pColours);
		return;
	}

	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	bool haveText = !pText.isEmpty();
	QByteArray textBytes = pText.toUtf8();
	char const *textData = textBytes.constData();

	if (state & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK))
	{
		/* Display the button down */
		iV_ShadowBox(x0, y0, x1, y1, 0, pColours[WCOL_LIGHT], pColours[WCOL_DARK], pColours[WCOL_BKGRND]);

		if (haveText)
		{
			iV_SetFont(FontID);
			iV_SetTextColour(pColours[WCOL_TEXT]);
			int fw = iV_GetTextWidth(textData);
			int fx = x0 + (width() - fw) / 2;
			int fy = y0 + (height() - iV_GetTextLineSize()) / 2 - iV_GetTextAboveBase();
			iV_DrawText(textData, fx, fy);
		}

		if (state & WBUT_HIGHLIGHT)
		{
			/* Display the button hilite */
			iV_Box(x0 + 3, y0 + 3, x1 - 2, y1 - 2, pColours[WCOL_HILITE]);
		}
	}
	else if (state & WBUT_DISABLE)
	{
		/* Display the disabled button */
		iV_ShadowBox(x0, y0, x1, y1, 0, pColours[WCOL_LIGHT], pColours[WCOL_LIGHT], pColours[WCOL_BKGRND]);

		if (haveText)
		{
			iV_SetFont(FontID);
			int fw = iV_GetTextWidth(textData);
			int fx = x0 + (width() - fw) / 2;
			int fy = y0 + (height() - iV_GetTextLineSize()) / 2 - iV_GetTextAboveBase();
			iV_SetTextColour(pColours[WCOL_LIGHT]);
			iV_DrawText(textData, fx + 1, fy + 1);
			iV_SetTextColour(pColours[WCOL_DISABLE]);
			iV_DrawText(textData, fx, fy);
		}

		if (state & WBUT_HIGHLIGHT)
		{
			/* Display the button hilite */
			iV_Box(x0 + 2, y0 + 2, x1 - 3, y1 - 3, pColours[WCOL_HILITE]);
		}
	}
	else
	{
		/* Display the button up */
		iV_ShadowBox(x0, y0, x1, y1, 0, pColours[WCOL_LIGHT], pColours[WCOL_DARK], pColours[WCOL_BKGRND]);

		if (haveText)
		{
			iV_SetFont(FontID);
			iV_SetTextColour(pColours[WCOL_TEXT]);
			int fw = iV_GetTextWidth(textData);
			int fx = x0 + (width() - fw) / 2;
			int fy = y0 + (height() - iV_GetTextLineSize()) / 2 - iV_GetTextAboveBase();
			iV_DrawText(textData, fx, fy);
		}

		if (state & WBUT_HIGHLIGHT)
		{
			/* Display the button hilite */
			iV_Box(x0 + 2, y0 + 2, x1 - 3, y1 - 3, pColours[WCOL_HILITE]);
		}
	}
}
