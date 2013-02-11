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
 *  Slider bar interface definitions.
 */

#ifndef __INCLUDED_LIB_WIDGET_SLIDER_H__
#define __INCLUDED_LIB_WIDGET_SLIDER_H__

#include "widget.h"
#include "widgbase.h"

/* Slider state */
#define SLD_DRAG		0x0001		// Slider is being dragged
#define SLD_HILITE		0x0002		// Slider is hilited


struct W_SLIDER : public WIDGET
{
	W_SLIDER(W_SLDINIT const *init);

	void clicked(W_CONTEXT *psContext, WIDGET_KEY key);
	void released(W_CONTEXT *psContext, WIDGET_KEY key);
	void highlight(W_CONTEXT *psContext);
	void highlightLost(W_CONTEXT *psContext);
	void run(W_CONTEXT *psContext);

	WSLD_ORIENTATION orientation;                   // The orientation of the slider
	UWORD		numStops;			// Number of stop positions on the slider
	UWORD		barSize;			// Thickness of slider bar
	UWORD		pos;				// Current stop position of the slider
	UWORD		state;				// Slider state
	QString         pTip;                           // Tool tip
};

/* Create a slider widget data structure */
extern W_SLIDER *sliderCreate(const W_SLDINIT *psInit);

/* Free the memory used by a slider */
extern void sliderFree(W_SLIDER *psWidget);

/* Initialise a slider widget before running it */
extern void sliderInitialise(W_SLIDER *psWidget);

/* The slider display function */
extern void sliderDisplay(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

#endif // __INCLUDED_LIB_WIDGET_SLIDER_H__
