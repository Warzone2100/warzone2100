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
// FIXME Direct iVis implementation include!
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
	, pTip(NULL)
{}

W_SLIDER::W_SLIDER(W_SLDINIT const *init)
	: WIDGET(init, WIDG_SLIDER)
	, orientation(init->orientation)
	, numStops(init->numStops)
	, barSize(init->barSize)
	, pos(init->pos)
	, state(0)
	, pTip(QString::fromUtf8(init->pTip))
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
	ASSERT(psWidget != NULL,
	       "widgGetSliderPos: couldn't find widget from id");
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
	ASSERT(psWidget != NULL,
	       "widgGetSliderPos: couldn't find widget from id");
	if (psWidget)
	{
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

/* Return the current position of the slider bar on the widget */
static void sliderGetBarBox(W_SLIDER *psSlider, int *pX, int *pY, int *pWidth, int *pHeight)
{
	switch (psSlider->orientation)
	{
	case WSLD_LEFT:
		*pX = (psSlider->width() - psSlider->barSize)
		        * psSlider->pos / psSlider->numStops;
		*pY = 0;
		*pWidth = psSlider->barSize;
		*pHeight = psSlider->height();
		break;
	case WSLD_RIGHT:
		*pX = psSlider->width() - psSlider->barSize
		        - (psSlider->width() - psSlider->barSize)
		        * psSlider->pos / psSlider->numStops;
		*pY = 0;
		*pWidth = psSlider->barSize;
		*pHeight = psSlider->height();
		break;
	case WSLD_TOP:
		*pX = 0;
		*pY = (psSlider->height() - psSlider->barSize)
		        * psSlider->pos / psSlider->numStops;
		*pWidth = psSlider->width();
		*pHeight = psSlider->barSize;
		break;
	case WSLD_BOTTOM:
		*pX = 0;
		*pY = psSlider->height() - psSlider->barSize
		        - (psSlider->height() - psSlider->barSize)
		        * psSlider->pos / psSlider->numStops;
		*pWidth = psSlider->width();
		*pHeight = psSlider->barSize;
		break;
	}
}


/* Run a slider widget */
void W_SLIDER::run(W_CONTEXT *psContext)
{
	if ((state & SLD_DRAG) && !mouseDown(MOUSE_LMB))
	{
		state &= ~SLD_DRAG;
		screenPointer->setReturn(this);
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
		state |= SLD_DRAG;
	}
}

void W_SLIDER::highlight(W_CONTEXT *)
{
	state |= SLD_HILITE;
}


/* Respond to the mouse moving off a slider */
void W_SLIDER::highlightLost()
{
	state &= ~SLD_HILITE;
}

void W_SLIDER::display(int xOffset, int yOffset)
{
	if (displayFunction != NULL)
	{
		displayFunction(this, xOffset, yOffset);
		return;
	}

	int x0, y0, x1, y1, bbwidth = 0, bbheight = 0;

	switch (orientation)
	{
	case WSLD_LEFT:
	case WSLD_RIGHT:
		/* Draw the line */
		x0 = x() + xOffset + barSize / 2;
		y0 = y() + yOffset + height() / 2;
		x1 = x0 + width() - barSize;
		iV_Line(x0, y0, x1, y0, WZCOL_FORM_DARK);
		iV_Line(x0, y0 + 1, x1, y0 + 1, WZCOL_FORM_LIGHT);

		/* Now Draw the bar */
		sliderGetBarBox(this, &x0, &y0, &bbwidth, &bbheight);
		x0 = x0 + x() + xOffset;
		y0 = y0 + y() + yOffset;
		x1 = x0 + bbwidth;
		y1 = y0 + bbheight;
		iV_ShadowBox(x0, y0, x1, y1, 0, WZCOL_FORM_LIGHT, WZCOL_FORM_DARK, WZCOL_FORM_BACKGROUND);
		break;
	case WSLD_TOP:
	case WSLD_BOTTOM:
		/* Draw the line */
		x0 = x() + xOffset + width() / 2;
		y0 = y() + yOffset + barSize / 2;
		y1 = y0 + height() - barSize;
		iV_Line(x0, y0, x0, y1, WZCOL_FORM_DARK);
		iV_Line(x0 + 1, y0, x0 + 1, y1, WZCOL_FORM_LIGHT);

		/* Now Draw the bar */
		sliderGetBarBox(this, &x0, &y0, &bbwidth, &bbheight);
		x0 = x0 + x() + xOffset;
		y0 = y0 + y() + yOffset;
		x1 = x0 + bbwidth;
		y1 = y0 + bbheight;
		iV_ShadowBox(x0, y0, x1, y1, 0, WZCOL_FORM_LIGHT, WZCOL_FORM_DARK, WZCOL_FORM_BACKGROUND);
		break;
	}

	if ((state & SLD_HILITE) != 0)
	{
		x0 = x() + xOffset - 2;
		y0 = y() + yOffset - 2;
		x1 = x0 + width() + 4;
		y1 = y0 + height() + 4;
		iV_Box(x0, y0, x1, y1, WZCOL_FORM_HILITE);
	}
}

void W_SLIDER::setTip(QString string)
{
	pTip = string;
}
