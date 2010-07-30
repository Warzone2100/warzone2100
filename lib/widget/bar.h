/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
 *  Definitions for Bar Graph functions.
 */

#ifndef __INCLUDED_LIB_WIDGET_BAR_H__
#define __INCLUDED_LIB_WIDGET_BAR_H__

#include "widget.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

typedef struct _w_bargraph
{
	/* The common widget data */
	WIDGET_BASE;

	UWORD		barPos;				// Orientation of the bar on the widget
	UWORD		majorSize;			// Percentage of the main bar that is filled
	UWORD		minorSize;			// Percentage of the minor bar if there is one
	UWORD		iRange;				// Maximum range
	UWORD		iValue;				// Current value
	UWORD		iOriginal;			// hack to keep uncapped value around
	PIELIGHT	majorCol;			// Colour for the major bar
	PIELIGHT	minorCol;			// Colour for the minor bar
	const char	*pTip;				// The tool tip for the graph
} W_BARGRAPH;

/* Create a barGraph widget data structure */
extern W_BARGRAPH* barGraphCreate(const W_BARINIT* psInit);

/* Free the memory used by a barGraph */
extern void barGraphFree(W_BARGRAPH *psWidget);

/* Initialise a barGraph widget before running it */
extern void barGraphInitialise(W_BARGRAPH *psWidget);

#if 0
/* Run a barGraph widget */
extern void barGraphRun(W_BARGRAPH *psWidget);

/* Respond to a mouse click */
extern void barGraphClicked(W_BARGRAPH *psWidget);

/* Respond to a mouse up */
extern void barGraphReleased(W_BARGRAPH *psWidget);
#endif

/* Respond to a mouse moving over a barGraph */
extern void barGraphHiLite(W_BARGRAPH *psWidget, W_CONTEXT *psContext);

/* Respond to the mouse moving off a barGraph */
extern void barGraphHiLiteLost(W_BARGRAPH *psWidget);

/* The bar graph display function */
extern void barGraphDisplay(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

/* The double bar graph display function */
extern void barGraphDisplayDouble(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

/* The trough bar graph display function */
extern void barGraphDisplayTrough(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_LIB_WIDGET_BAR_H__
