/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
 *  The tool tip display system.
 */

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/math_ext.h"
#include "lib/ivis_opengl/screen.h"
#include "widget.h"
#include "widgint.h"
#include "tip.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <string>

/* Time delay before showing the tool tip */
#define TIP_PAUSE	200

/* How long to display the tool tip */
#define TIP_TIME	4000

/* Size of border around tip text */
#define TIP_HGAP	6
#define TIP_VGAP	3


/* The tool tip state */
static enum _tip_state
{
	TIP_NONE,			// No tip, and no button hilited
	TIP_WAIT,			// A button is hilited, but not yet ready to show the tip
	TIP_ACTIVE,			// A tip is being displayed
} tipState;

struct TipDisplayCache {
	std::vector<WzText> wzTip;
};

static SDWORD		startTime;			// When the tip was created
static SDWORD		mx, my;				// Last mouse coords
static SDWORD		wx, wy, ww, wh;		// Position and size of button to place tip by
static SDWORD		tx, ty, tw, th;		// Position and size of the tip box
static SDWORD		fx, fy;				// Position of the text
static int              lineHeight;
static std::vector<std::string>  pTip;	// Tip text
static WIDGET		*psWidget;			// The button the tip is for
static enum iV_fonts FontID = font_regular;	// ID for the Ivis Font.
static PIELIGHT TipColour;
static TipDisplayCache displayCache;

/* Initialise the tool tip module */
void tipInitialise(void)
{
	tipState = TIP_NONE;
	TipColour = WZCOL_WHITE;
}

// Set the global toop tip text colour.
void widgSetTipColour(PIELIGHT colour)
{
	TipColour = colour;
}

static std::vector<std::string> splitString(const std::string& str, char delimiter)
{
	std::vector<std::string> strings;
	std::istringstream str_stream(str);
	std::string s;
	while (getline(str_stream, s, delimiter)) {
		strings.push_back(s);
	}
	return strings;
}

/*
 * Setup a tool tip.
 * The tip module will then wait until the correct points to
 * display and then remove the tool tip.
 * i.e. The tip will not be displayed immediately.
 * Calling this while another tip is being displayed will restart
 * the tip system.
 * psSource is the widget that started the tip.
 * x,y,width,height - specify the position of the button to place the
 * tip by.
 */
void tipStart(WIDGET *psSource, const std::string& pNewTip, iV_fonts NewFontID, int x, int y, int width, int height)
{
	ASSERT(psSource != nullptr, "Invalid widget pointer");

	tipState = TIP_WAIT;
	startTime = wzGetTicks();
	mx = mouseX();
	my = mouseY();
	wx = x; wy = y;
	ww = width; wh = height;
	pTip = splitString(pNewTip, '\n');
	psWidget = psSource;
	FontID = NewFontID;
	displayCache.wzTip.clear();
}


/* Stop a tool tip (e.g. if the hilite is lost on a button).
 * psSource should be the same as the widget that started the tip.
 */
void tipStop(WIDGET *psSource)
{
	ASSERT(psSource != nullptr,
	       "tipStop: Invalid widget pointer");

	if (tipState != TIP_NONE && psSource == psWidget)
	{
		tipState = TIP_NONE;
	}
}

/* Update and possibly display the tip */
void tipDisplay()
{
	SDWORD		newMX, newMY;
	SDWORD		currTime;
	SDWORD		fw, topGap;

	switch (tipState)
	{
	case TIP_WAIT:
		/* See if the tip has to be shown */
		newMX = mouseX();
		newMY = mouseY();
		currTime = wzGetTicks();
		if (newMX == mx &&
		    newMY == my &&
		    (currTime - startTime > TIP_PAUSE))
		{
			/* Activate the tip */
			tipState = TIP_ACTIVE;

			/* Calculate the size of the tip box */
			topGap = TIP_VGAP;

			lineHeight = iV_GetTextLineSize(FontID);

			fw = 0;
			displayCache.wzTip.resize(pTip.size());
			for (int n = 0; n < pTip.size(); ++n)
			{
				displayCache.wzTip[n].setText(pTip[n], FontID);
				fw = std::max<int>(fw, displayCache.wzTip[n].width());
			}
			tw = fw + TIP_HGAP * 2;
			th = topGap * 2 + lineHeight * pTip.size() + iV_GetTextBelowBase(FontID);

			/* Position the tip box */
			tx = clip(wx + ww / 2, 0, screenWidth - tw - 1);
			ty = std::max(wy + wh + TIP_VGAP, 0);
			if (ty + th >= (int)screenHeight)
			{
				/* Position the tip above the button */
				ty = wy - th - TIP_VGAP;
			}

			/* Position the text */
			fx = tx + TIP_HGAP;
			fy = ty + (th - lineHeight * pTip.size()) / 2 - iV_GetTextAboveBase(FontID);

			/* Note the time */
			startTime = wzGetTicks();
		}
		else if (newMX != mx ||
		         newMY != my ||
		         mousePressed(MOUSE_LMB))
		{
			mx = newMX;
			my = newMY;
			startTime = currTime;
		}
		break;
	case TIP_ACTIVE:
		{
			/* Draw the tool tip */
			iV_ShadowBox(tx - 2, ty - 2, tx + tw + 2, ty + th + 2, 1, WZCOL_FORM_LIGHT, WZCOL_FORM_DARK, WZCOL_FORM_TIP_BACKGROUND);
			size_t n = 0;
			for (auto it = displayCache.wzTip.begin(); it != displayCache.wzTip.end(); ++it)
			{
				it->render(fx, fy + lineHeight * n, TipColour);
				++n;
			}
		}

		break;
	default:
		break;
	}
}
