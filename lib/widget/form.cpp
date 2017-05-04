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
 *  Functionality for the form widget.
 */

#include <string.h>

#include "lib/framework/frame.h"
#include "widget.h"
#include "widgint.h"
#include "form.h"
#if defined(WZ_CC_MSVC)
#include "form_moc.h"		// this is generated on the pre-build event.
#endif
#include "tip.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"


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

W_FORM::W_FORM(WIDGET *parent)
	: WIDGET(parent, WIDG_FORM)
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

W_CLICKFORM::W_CLICKFORM(WIDGET *parent)
	: W_FORM(parent)
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

void W_FORM::clicked(W_CONTEXT *, WIDGET_KEY)
{
	// Stop the tip if there is one.
	tipStop(this);
	dirty = true;
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
			screenPointer->setReturn(this);
			state &= ~WBUT_DOWN;
			dirty = true;
		}
	}
}


/* Respond to a mouse moving over a form */
void W_CLICKFORM::highlight(W_CONTEXT *psContext)
{
	state |= WBUT_HIGHLIGHT;

	// If there is a tip string start the tool tip.
	if (!pTip.isEmpty())
	{
		tipStart(this, pTip, screenPointer->TipFontID, x() + psContext->xOffset, y() + psContext->yOffset, width(), height());
	}

	if (AudioCallback != nullptr)
	{
		AudioCallback(HilightAudioID);
	}
}


/* Respond to the mouse moving off a form */
void W_FORM::highlightLost()
{
	// Clear the tool tip if there is one.
	tipStop(this);
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

void W_CLICKFORM::setTip(QString string)
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
