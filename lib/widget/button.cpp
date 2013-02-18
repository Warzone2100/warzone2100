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
	, state(WBUTS_NORMAL)
	, pText(QString::fromUtf8(init->pText))
	, pTip(QString::fromUtf8(init->pTip))
	, HilightAudioID(WidgGetHilightAudioID())
	, ClickedAudioID(WidgGetClickedAudioID())
	, AudioCallback(WidgGetAudioCallback())
	, FontID(init->FontID)
{
	ASSERT((init->style & ~(WBUT_PLAIN | WIDG_HIDDEN | WFORM_NOCLICKMOVE | WBUT_NOPRIMARY | WBUT_SECONDARY | WBUT_TXTCENTRE)) == 0, "unknown button style");
}

static const unsigned buttonflagMap[] = {WBUTS_GREY,      WBUT_DISABLE,
                                         WBUTS_LOCKED,    WBUT_LOCK,
                                         WBUTS_CLICKLOCK, WBUT_CLICKLOCK};

unsigned W_BUTTON::getState()
{
	unsigned retState = 0;
	for (unsigned i = 0; i < ARRAY_SIZE(buttonflagMap); i += 2)
	{
		if ((state & buttonflagMap[i]) != 0)
		{
			retState |= buttonflagMap[i + 1];
		}
	}

	if (state & WBUTS_FLASH)
	{
		retState |= WBUT_FLASH;
	}

	return retState;
}

void W_BUTTON::setFlash(bool enable)
{
	if (enable)
	{
		state |= WBUTS_FLASH;
	}
	else
	{
		state &= ~(WBUTS_FLASH | WBUTS_FLASHON);
	}
}

void W_BUTTON::setState(unsigned newState)
{
	ASSERT(!((newState & WBUT_LOCK) && (newState & WBUT_CLICKLOCK)), "Cannot have both WBUT_LOCK and WBUT_CLICKLOCK");

	for (unsigned i = 0; i < ARRAY_SIZE(buttonflagMap); i += 2)
	{
		state &= ~buttonflagMap[i];
		if ((newState & buttonflagMap[i + 1]) != 0)
		{
			state |= buttonflagMap[i];
		}
	}
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

/* Run a button widget */
void W_BUTTON::run(W_CONTEXT *)
{
	if (state & WBUTS_FLASH)
	{
		if (((realTime / 250) % 2) == 0)
		{
			state &= ~WBUTS_FLASHON;
		}
		else
		{
			state |= WBUTS_FLASHON;
		}
	}
}


/* Respond to a mouse click */
void W_BUTTON::clicked(W_CONTEXT *, WIDGET_KEY key)
{
	/* Can't click a button if it is disabled or locked down */
	if (!(state & (WBUTS_GREY | WBUTS_LOCKED)))
	{
		// Check this is the correct key
		if ((!(style & WBUT_NOPRIMARY) && key == WKEY_PRIMARY) ||
		    ((style & WBUT_SECONDARY) && key == WKEY_SECONDARY))
		{
			if (AudioCallback)
			{
				AudioCallback(ClickedAudioID);
			}
			state &= ~WBUTS_FLASH;	// Stop it flashing
			state &= ~WBUTS_FLASHON;
			state |= WBUTS_DOWN;
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
	if (state & WBUTS_DOWN)
	{
		// Check this is the correct key
		if ((!(style & WBUT_NOPRIMARY) && key == WKEY_PRIMARY) ||
		    ((style & WBUT_SECONDARY) && key == WKEY_SECONDARY))
		{
			widgSetReturn(psScreen, this);
			state &= ~WBUTS_DOWN;
		}
	}
}


/* Respond to a mouse moving over a button */
void W_BUTTON::highlight(W_CONTEXT *psContext)
{
	state |= WBUTS_HILITE;

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
	state &= ~(WBUTS_DOWN | WBUTS_HILITE);
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

	if (state & (WBUTS_DOWN | WBUTS_LOCKED | WBUTS_CLICKLOCK))
	{
		/* Display the button down */
		iV_ShadowBox(x0, y0, x1, y1, 0, pColours[WCOL_LIGHT], pColours[WCOL_DARK], pColours[WCOL_BKGRND]);

		if (haveText)
		{
			iV_SetFont(FontID);
			iV_SetTextColour(pColours[WCOL_TEXT]);
			int fw = iV_GetTextWidth(textData);
			int fx, fy;
			if (style & WBUT_NOCLICKMOVE)
			{
				fx = x0 + (width() - fw) / 2 + 1;
				fy = y0 + 1 + (height() - iV_GetTextLineSize()) / 2 - iV_GetTextAboveBase();
			}
			else
			{
				fx = x0 + (width() - fw) / 2;
				fy = y0 + (height() - iV_GetTextLineSize()) / 2 - iV_GetTextAboveBase();
			}
			iV_DrawText(textData, fx, fy);
		}

		if (state & WBUTS_HILITE)
		{
			/* Display the button hilite */
			iV_Box(x0 + 3, y0 + 3, x1 - 2, y1 - 2, pColours[WCOL_HILITE]);
		}
	}
	else if (state & WBUTS_GREY)
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

		if (state & WBUTS_HILITE)
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

		if (state & WBUTS_HILITE)
		{
			/* Display the button hilite */
			iV_Box(x0 + 2, y0 + 2, x1 - 3, y1 - 3, pColours[WCOL_HILITE]);
		}
	}
}
