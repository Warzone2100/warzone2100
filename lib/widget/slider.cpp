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
 *  Slide bar widget definitions.
 */

#include "widget.h"
#include "widgint.h"
#include "slider.h"
#if defined(WZ_CC_MSVC)
#include "slider_moc.h"		// this is generated on the pre-build event.
#endif
#include "lib/ivis_opengl/pieblitfunc.h"

static bool DragEnabled = true;

void sliderEnableDrag(bool Enable)
{
	DragEnabled = Enable;
}

W_SLDINIT::W_SLDINIT()
	: orientation(WSLD_LEFT)
	, numStops(0)
	, barSize(0)
	, pos(0)
{}

W_SLIDER::W_SLIDER(W_SLDINIT const *init)
	: WIDGET(init, WIDG_SLIDER)
	, orientation(init->orientation)
	, numStops(init->numStops)
	, barSize(init->barSize)
	, pos(init->pos)
	, state(0)
	, pTip(init->pTip)
{
	ASSERT((init->style & ~(WBAR_PLAIN | WIDG_HIDDEN)) == 0, "Unknown style");
	ASSERT(init->orientation >= WSLD_LEFT || init->orientation <= WSLD_BOTTOM, "Unknown orientation");
	bool horizontal = init->orientation == WSLD_LEFT || init->orientation == WSLD_RIGHT;
	bool vertical = init->orientation == WSLD_TOP || init->orientation == WSLD_BOTTOM;
	ASSERT((!horizontal || init->numStops <= init->width  - init->barSize) &&
	       (!vertical   || init->numStops <= init->height - init->barSize), "Too many stops for slider length");
	ASSERT(init->pos <= init->numStops, "Slider position greater than stops (%d/%d)", init->pos, init->numStops);
	ASSERT((!horizontal || init->barSize <= init->width) &&
	       (!vertical   || init->barSize <= init->height), "Slider bar is larger than slider width");
}

/* Get the current position of a slider bar */
UDWORD widgGetSliderPos(W_SCREEN *psScreen, UDWORD id)
{
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	ASSERT(psWidget != NULL, "Could not find widget from id %u", id);
	if (psWidget)
	{
		return ((W_SLIDER *)psWidget)->pos;
	}
	return 0;
}

/* Set the current position of a slider bar */
void widgSetSliderPos(W_SCREEN *psScreen, UDWORD id, UWORD pos)
{
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	ASSERT(psWidget != NULL, "Could not find widget from id %u", id);
	if (psWidget && pos != ((W_SLIDER *)psWidget)->pos)
	{
		psWidget->dirty = true;
		if (pos > ((W_SLIDER *)psWidget)->numStops)
		{
			((W_SLIDER *)psWidget)->pos = ((W_SLIDER *)psWidget)->numStops;
		}
		else
		{
			((W_SLIDER *)psWidget)->pos = pos;
		}
	}
}

/* Run a slider widget */
void W_SLIDER::run(W_CONTEXT *psContext)
{
	if ((state & SLD_DRAG) && !mouseDown(MOUSE_LMB))
	{
		state &= ~SLD_DRAG;
		screenPointer->setReturn(this);
		dirty = true;
	}
	else if (!(state & SLD_DRAG) && mouseDown(MOUSE_LMB))
	{
		clicked(psContext, WKEY_NONE);
	}
	if (state & SLD_DRAG)
	{
		/* Figure out where the drag box should be */
		int mx = psContext->mx - x();
		int my = psContext->my - y();
		int stopSize;
		switch (orientation)
		{
		case WSLD_LEFT:
			if (mx <= barSize / 2)
			{
				pos = 0;
			}
			else if (mx >= width() - barSize / 2)
			{
				pos = numStops;
			}
			else
			{
				/* Mouse is in the middle of the slider, calculate which stop */
				stopSize = (width() - barSize) / numStops;
				pos = (mx + stopSize / 2 - barSize / 2)
				        * numStops
				        / (width() - barSize);
			}
			break;
		case WSLD_RIGHT:
			if (mx <= barSize / 2)
			{
				pos = numStops;
			}
			else if (mx >= width() - barSize / 2)
			{
				pos = 0;
			}
			else
			{
				/* Mouse is in the middle of the slider, calculate which stop */
				stopSize = (width() - barSize) / numStops;
				pos = numStops
				        - (mx + stopSize / 2 - barSize / 2)
				        * numStops
				        / (width() - barSize);
			}
			break;
		case WSLD_TOP:
			if (my <= barSize / 2)
			{
				pos = 0;
			}
			else if (my >= height() - barSize / 2)
			{
				pos = numStops;
			}
			else
			{
				/* Mouse is in the middle of the slider, calculate which stop */
				stopSize = (height() - barSize) / numStops;
				pos = (my + stopSize / 2 - barSize / 2)
				        * numStops
				        / (height() - barSize);
			}
			break;
		case WSLD_BOTTOM:
			if (my <= barSize / 2)
			{
				pos = numStops;
			}
			else if (my >= height() - barSize / 2)
			{
				pos = 0;
			}
			else
			{
				/* Mouse is in the middle of the slider, calculate which stop */
				stopSize = (height() - barSize) / numStops;
				pos = numStops
				        - (my + stopSize / 2 - barSize / 2)
				        * numStops
				        / (height() - barSize);
			}
			break;
		}
	}
}

void W_SLIDER::clicked(W_CONTEXT *psContext, WIDGET_KEY)
{
	if (DragEnabled && geometry().contains(psContext->mx, psContext->my))
	{
		dirty = true;
		state |= SLD_DRAG;
	}
}

void W_SLIDER::highlight(W_CONTEXT *)
{
	state |= SLD_HILITE;
	dirty = true;
}


/* Respond to the mouse moving off a slider */
void W_SLIDER::highlightLost()
{
	state &= ~SLD_HILITE;
	dirty = true;
}

void W_SLIDER::setTip(QString string)
{
	pTip = string;
}

void W_SLIDER::display(int xOffset, int yOffset)
{
	ASSERT(false, "No default implementation exists for sliders");
}
