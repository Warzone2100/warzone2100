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
 *  Definitions for Bar Graph functions.
 */

#ifndef __INCLUDED_LIB_WIDGET_BAR_H__
#define __INCLUDED_LIB_WIDGET_BAR_H__

#include "widget.h"


struct W_BARGRAPH : public WIDGET
{
	W_BARGRAPH(W_BARINIT const *init);

	void highlight(W_CONTEXT *psContext);
	void highlightLost(W_CONTEXT *);

	void setTip(QString string);

	WBAR_ORIENTATION barPos;                        // Orientation of the bar on the widget
	UWORD		majorSize;			// Percentage of the main bar that is filled
	UWORD		minorSize;			// Percentage of the minor bar if there is one
	UWORD		iRange;				// Maximum range
	UWORD		iValue;				// Current value
	UWORD		iOriginal;			// hack to keep uncapped value around
	int             denominator;                    // Denominator, 1 by default.
	int             precision;                      // Number of places after the decimal point to display, 0 by default.
	PIELIGHT	majorCol;			// Colour for the major bar
	PIELIGHT	minorCol;			// Colour for the minor bar
	PIELIGHT        textCol;                        // Colour for the text on the bar.
	QString         pTip;                           // The tool tip for the graph
	QString         text;                           // Text on the bar.
};

/* The bar graph display function */
extern void barGraphDisplay(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

/* The double bar graph display function */
extern void barGraphDisplayDouble(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

/* The trough bar graph display function */
extern void barGraphDisplayTrough(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

#endif // __INCLUDED_LIB_WIDGET_BAR_H__
