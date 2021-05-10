/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
 *  Functions for the bar graph widget
 */

#include "widget.h"
#include "widgint.h"
#include "tip.h"
#include "form.h"
#include "bar.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"

W_BARINIT::W_BARINIT()
	: orientation(WBAR_LEFT)
	, size(0)
	, minorSize(0)
	, iRange(100)
	, denominator(1)
	, precision(0)
	  //sCol
	  //sMinorCol
{
	sCol.rgba = 0;
	sMinorCol.rgba = 0;
}

W_BARGRAPH::W_BARGRAPH(W_BARINIT const *init)
	: WIDGET(init, WIDG_BARGRAPH)
	, barPos(init->orientation)
	, majorSize(init->size)
	, minorSize(init->minorSize)
	, iRange(init->iRange)
	, majorValue(majorSize * iRange / WBAR_SCALE)
	, minorValue(minorSize * iRange / WBAR_SCALE)
	, denominator(MAX(init->denominator, 1))
	, precision(init->precision)
	, majorCol(init->sCol)
	, minorCol(init->sMinorCol)
	, textCol(WZCOL_BLACK)
	, pTip(init->pTip)
	, backgroundColour(WZCOL_FORM_BACKGROUND)
{
	/* Set the minor colour if necessary */
	// Actually, this sets the major colour to the minor colour. The minor colour used to be left completely uninitialised... Wonder what it was for..?
	if (style & WBAR_DOUBLE)
	{
		majorCol = minorCol;
	}

	ASSERT((init->style & ~(WBAR_PLAIN | WBAR_TROUGH | WBAR_DOUBLE | WIDG_HIDDEN)) == 0, "Unknown bar graph style");
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (__GNUC__ < 7)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wlogical-op" // Older GCC (at least GCC 5.4) triggers a warning on this
#endif
	ASSERT(init->orientation >= WBAR_LEFT || init->orientation <= WBAR_BOTTOM, "Unknown orientation");
#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (__GNUC__ < 7)
# pragma GCC diagnostic pop
#endif
	ASSERT(init->size <= WBAR_SCALE, "Bar size out of range");
	ASSERT((init->style & WBAR_DOUBLE) == 0 || init->minorSize <= WBAR_SCALE, "Minor bar size out of range");
}

static W_BARGRAPH *getBarGraph(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id)
{
	ASSERT_OR_RETURN(nullptr, psScreen != nullptr, "Invalid screen pointer");
	W_BARGRAPH *psBGraph = (W_BARGRAPH *)widgGetFromID(psScreen, id);
	ASSERT_OR_RETURN(nullptr, psBGraph != nullptr, "Could not find widget from ID");
	ASSERT_OR_RETURN(nullptr, psBGraph->type == WIDG_BARGRAPH, "Wrong widget type");

	return psBGraph;
}

/* Set the current size of a bar graph */
void widgSetBarSize(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id, UDWORD iValue)
{
	if (auto psBGraph = getBarGraph(psScreen, id))
	{
		psBGraph->majorValue = iValue;
		psBGraph->sizesDirty = true;
	}
}


/* Set the current size of a minor bar on a double graph */
void widgSetMinorBarSize(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id, UDWORD iValue)
{
	if (auto psBGraph = getBarGraph(psScreen, id))
	{
		psBGraph->minorValue = iValue;
		psBGraph->sizesDirty = true;
	}
}


/** Set the range on a double graph */
void widgSetBarRange(const std::shared_ptr<W_SCREEN> &psScreen, UDWORD id, UDWORD iValue)
{
	if (auto psBGraph = getBarGraph(psScreen, id))
	{
		psBGraph->iRange = iValue;
		psBGraph->sizesDirty = true;
	}
}

void W_BARGRAPH::run(W_CONTEXT *context)
{
	if (sizesDirty)
	{
		majorSize = WBAR_SCALE * MIN(majorValue, iRange) / MAX(iRange, 1);
		minorSize = MIN(WBAR_SCALE * minorValue / MAX(iRange, 1), WBAR_SCALE);
		sizesDirty = false;
	}
}

static void barGraphDisplayText(W_BARGRAPH *barGraph, int x0, int x1, int y1)
{
	if (!barGraph->text.empty())
	{
		barGraph->wzCachedText.setText(barGraph->text, font_bar);
		int textWidth = barGraph->wzCachedText.width();
		Vector2i pos((x0 + x1 - textWidth) / 2, y1);
		// Add a shadow, to make it visible against any background.
		for (int dx = -2; dx <= 2; ++dx)
		{
			for (int dy = -2; dy <= 2; ++dy)
			{
				if (dx*dx + dy*dy <= 4)
				{
					barGraph->wzCachedText.render(pos.x + dx, pos.y + dy, WZCOL_BLACK);
				}
			}
		}
		barGraph->wzCachedText.render(pos.x, pos.y, barGraph->textCol);
	}
}

/* The simple bar graph display function */
static void barGraphDisplay(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	SDWORD		x0 = 0, y0 = 0, x1 = 0, y1 = 0;
	W_BARGRAPH	*psBGraph;

	psBGraph = (W_BARGRAPH *)psWidget;

	/* figure out which way the bar graph fills */
	switch (psBGraph->barPos)
	{
	case WBAR_LEFT:
		x0 = xOffset + psWidget->x();
		y0 = yOffset + psWidget->y();
		x1 = x0 + psWidget->width() * psBGraph->majorSize / WBAR_SCALE;
		y1 = y0 + psWidget->height();
		break;
	case WBAR_RIGHT:
		y0 = yOffset + psWidget->y();
		x1 = xOffset + psWidget->x() + psWidget->width();
		x0 = x1 - psWidget->width() * psBGraph->majorSize / WBAR_SCALE;
		y1 = y0 + psWidget->height();
		break;
	case WBAR_TOP:
		x0 = xOffset + psWidget->x();
		y0 = yOffset + psWidget->y();
		x1 = x0 + psWidget->width();
		y1 = y0 + psWidget->height() * psBGraph->majorSize / WBAR_SCALE;
		break;
	case WBAR_BOTTOM:
		x0 = xOffset + psWidget->x();
		x1 = x0 + psWidget->width();
		y1 = yOffset + psWidget->y() + psWidget->height();
		y0 = y1 - psWidget->height() * psBGraph->majorSize / WBAR_SCALE;
		break;
	}

	/* Now draw the graph */
	iV_ShadowBox(x0, y0, x1, y1, 0, WZCOL_FORM_LIGHT, WZCOL_FORM_DARK, psBGraph->majorCol);

	barGraphDisplayText(psBGraph, x0, x1, y1);
}


/* The double bar graph display function */
static void barGraphDisplayDouble(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	SDWORD		x0 = 0, y0 = 0, x1 = 0, y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0;
	W_BARGRAPH	*psBGraph = (W_BARGRAPH *)psWidget;

	/* figure out which way the bar graph fills */
	switch (psBGraph->barPos)
	{
	case WBAR_LEFT:
		/* Calculate the major bar */
		x0 = xOffset + psWidget->x();
		y0 = yOffset + psWidget->y();
		x1 = x0 + psWidget->width() * psBGraph->majorSize / WBAR_SCALE;
		y1 = y0 + 2 * psWidget->height() / 3;

		/* Calculate the minor bar */
		x2 = x0;
		y2 = y0 + psWidget->height() / 3;
		x3 = x2 + psWidget->width() * psBGraph->minorSize / WBAR_SCALE;
		y3 = y0 + psWidget->height();
		break;
	case WBAR_RIGHT:
		/* Calculate the major bar */
		y0 = yOffset + psWidget->y();
		x1 = xOffset + psWidget->x() + psWidget->width();
		x0 = x1 - psWidget->width() * psBGraph->majorSize / WBAR_SCALE;
		y1 = y0 + 2 * psWidget->height() / 3;

		/* Calculate the minor bar */
		x3 = x1;
		y2 = y0 + psWidget->height() / 3;
		x2 = x3 - psWidget->width() * psBGraph->minorSize / WBAR_SCALE;
		y3 = y0 + psWidget->height();
		break;
	case WBAR_TOP:
		/* Calculate the major bar */
		x0 = xOffset + psWidget->x();
		y0 = yOffset + psWidget->y();
		x1 = x0 + 2 * psWidget->width() / 3;
		y1 = y0 + psWidget->height() * psBGraph->majorSize / WBAR_SCALE;

		/* Calculate the minor bar */
		x2 = x0 + psWidget->width() / 3;
		y2 = y0;
		x3 = x0 + psWidget->width();
		y3 = y2 + psWidget->height() * psBGraph->minorSize / WBAR_SCALE;
		break;
	case WBAR_BOTTOM:
		/* Calculate the major bar */
		x0 = xOffset + psWidget->x();
		x1 = x0 + 2 * psWidget->width() / 3;
		y1 = yOffset + psWidget->y() + psWidget->height();
		y0 = y1 - psWidget->height() * psBGraph->majorSize / WBAR_SCALE;

		/* Calculate the minor bar */
		x2 = x0 + psWidget->width() / 3;
		x3 = x0 + psWidget->width();
		y3 = y1;
		y2 = y3 - psWidget->height() * psBGraph->minorSize / WBAR_SCALE;
		break;
	}

	/* Draw the minor bar graph */
	if (psBGraph->minorSize > 0)
	{
		iV_ShadowBox(x2, y2, x3, y3, 0, WZCOL_FORM_LIGHT, WZCOL_FORM_DARK, psBGraph->minorCol);
	}

	/* Draw the major bar graph */
	iV_ShadowBox(x0, y0, x1, y1, 0, WZCOL_FORM_LIGHT, WZCOL_FORM_DARK, psBGraph->majorCol);

	barGraphDisplayText(psBGraph, x0, x1, y1);
}


/* The trough bar graph display function */
void barGraphDisplayTrough(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	SDWORD		x0 = 0, y0 = 0, x1 = 0, y1 = 0;		// Position of the bar
	SDWORD		tx0 = 0, ty0 = 0, tx1 = 0, ty1 = 0;	// Position of the trough
	bool		showBar = true, showTrough = true;
	W_BARGRAPH	*psBGraph = (W_BARGRAPH *)psWidget;

	/* figure out which way the bar graph fills */
	switch (psBGraph->barPos)
	{
	case WBAR_LEFT:
		x0 = xOffset + psWidget->x();
		y0 = yOffset + psWidget->y();
		x1 = x0 + psWidget->width() * psBGraph->majorSize / WBAR_SCALE;
		y1 = y0 + psWidget->height();
		if (x0 == x1)
		{
			showBar = false;
		}
		tx0 = x1 + 1;
		ty0 = y0;
		tx1 = x0 + psWidget->width();
		ty1 = y1;
		if (tx0 >= tx1)
		{
			showTrough = false;
		}
		break;
	case WBAR_RIGHT:
		y0 = yOffset + psWidget->y();
		x1 = xOffset + psWidget->x() + psWidget->width();
		x0 = x1 - psWidget->width() * psBGraph->majorSize / WBAR_SCALE;
		y1 = y0 + psWidget->height();
		if (x0 == x1)
		{
			showBar = false;
		}
		tx0 = xOffset + psWidget->x();
		ty0 = y0;
		tx1 = x0 - 1;
		ty1 = y1;
		if (tx0 >= tx1)
		{
			showTrough = false;
		}
		break;
	case WBAR_TOP:
		x0 = xOffset + psWidget->x();
		y0 = yOffset + psWidget->y();
		x1 = x0 + psWidget->width();
		y1 = y0 + psWidget->height() * psBGraph->majorSize / WBAR_SCALE;
		if (y0 == y1)
		{
			showBar = false;
		}
		tx0 = x0;
		ty0 = y1 + 1;
		tx1 = x1;
		ty1 = y0 + psWidget->height();
		if (ty0 >= ty1)
		{
			showTrough = false;
		}
		break;
	case WBAR_BOTTOM:
		x0 = xOffset + psWidget->x();
		x1 = x0 + psWidget->width();
		y1 = yOffset + psWidget->y() + psWidget->height();
		y0 = y1 - psWidget->height() * psBGraph->majorSize / WBAR_SCALE;
		if (y0 == y1)
		{
			showBar = false;
		}
		tx0 = x0;
		ty0 = yOffset + psWidget->y();
		tx1 = x1;
		ty1 = y0 - 1;
		if (ty0 >= ty1)
		{
			showTrough = false;
		}
		break;
	}

	/* Now draw the graph */
	if (showBar)
	{
		pie_BoxFill(x0, y0, x1, y1, psBGraph->majorCol);
	}
	if (showTrough)
	{
		iV_ShadowBox(tx0, ty0, tx1, ty1, 0, WZCOL_FORM_DARK, WZCOL_FORM_LIGHT, psBGraph->backgroundColour);
	}

	barGraphDisplayText(psBGraph, x0, tx1, ty1);
}

void W_BARGRAPH::display(int xOffset, int yOffset)
{
	if (style & WBAR_TROUGH)
	{
		barGraphDisplayTrough(this, xOffset, yOffset);
	}
	else if (style & WBAR_DOUBLE)
	{
		barGraphDisplayDouble(this, xOffset, yOffset);
	}
	else
	{
		barGraphDisplay(this, xOffset, yOffset);
	}
}

void W_BARGRAPH::setTip(std::string string)
{
	pTip = string;
}
