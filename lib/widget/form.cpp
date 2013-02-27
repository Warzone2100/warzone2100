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
 *  Functionality for the form widget.
 */

#include <string.h>

#include "lib/framework/frame.h"
#include "widget.h"
#include "widgint.h"
#include "form.h"
#include "tip.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"

/* Control whether single tabs are displayed */
#define NO_DISPLAY_SINGLE_TABS 1


W_FORMINIT::W_FORMINIT()
	: disableChildren(false)
	, majorPos(0)
	, majorSize(0)
	, majorOffset(0)
	, tabVertOffset(0)
	, tabHorzOffset(0)
	, tabMajorThickness(0)
	, tabMajorGap(0)
	, numStats(0)
	, numButtons(0)
	, numMajor(0)
	, maxTabsShown(MAX_TAB_SMALL_SHOWN - 1)  // Equal to TAB_SEVEN, which is equal to 7.
	, pTip(NULL)
	, pTabDisplay(NULL)
{
}

W_FORM::W_FORM(W_FORMINIT const *init)
	: WIDGET(init, WIDG_FORM)
	, disableChildren(init->disableChildren)
	, Ax0(0), Ay0(0), Ax1(0), Ay1(0)  // These assignments were previously done by a memset.
	, animCount(0)
	, startTime(0)                    // This assignment was previously done by a memset.
{
	ASSERT((init->style & ~(WFORM_TABBED | WFORM_INVISIBLE | WFORM_CLICKABLE | WFORM_NOCLICKMOVE | WFORM_NOPRIMARY | WFORM_SECONDARY | WIDG_HIDDEN)) == 0, "Unknown style bit");
	ASSERT((init->style & WFORM_TABBED) == 0 || (init->style & (WFORM_INVISIBLE | WFORM_CLICKABLE)) == 0, "Tabbed form cannot be invisible or clickable");
	ASSERT((init->style & WFORM_INVISIBLE) == 0 || (init->style & WFORM_CLICKABLE) == 0, "Cannot have an invisible clickable form");
	ASSERT((init->style & WFORM_CLICKABLE) != 0 || (init->style & (WFORM_NOPRIMARY | WFORM_SECONDARY)) == 0, "Cannot set keys if the form isn't clickable");
}

W_FORM::W_FORM(WIDGET *parent)
	: WIDGET(parent, WIDG_FORM)
	, disableChildren(false)
	, Ax0(0), Ay0(0), Ax1(0), Ay1(0)
	, animCount(0)
	, startTime(0)
{}

W_CLICKFORM::W_CLICKFORM(W_FORMINIT const *init)
	: W_FORM(init)
	, state(WBUT_PLAIN)
	, pTip(QString::fromUtf8(init->pTip))
	, HilightAudioID(WidgGetHilightAudioID())
	, ClickedAudioID(WidgGetClickedAudioID())
	, AudioCallback(WidgGetAudioCallback())
{}

W_TABFORM::W_TABFORM(W_FORMINIT const *init)
	: W_FORM(init)
	, majorPos(init->majorPos)
	, majorSize(init->majorSize)
	, tabMajorThickness(init->tabMajorThickness)
	, tabMajorGap(init->tabMajorGap)
	, tabVertOffset(init->tabVertOffset)
	, tabHorzOffset(init->tabHorzOffset)
	, majorOffset(init->majorOffset)
	, state(0)
	, tabHiLite(~0)
	, maxTabsShown(init->maxTabsShown)
	, numStats(init->numStats)
	, numButtons(init->numButtons)
	, pTabDisplay(init->pTabDisplay)
	, currentTab(0)
{
	setNumTabs(init->numMajor);
}

bool W_TABFORM::setTab(int newTab)
{
	if (currentTab < childTabs.size())
	{
		childTabs[currentTab]->hide();
	}
	currentTab = clip(newTab, 0, childTabs.size() - 1);
	childTabs[currentTab]->show();
	return currentTab == (unsigned)newTab;
}

void W_TABFORM::setNumTabs(int numTabs)
{
	ASSERT(numTabs >= 1, "Must have at least one major tab on a tabbed form");

	unsigned prevNumTabs = childTabs.size();
	for (unsigned n = numTabs; n < prevNumTabs; ++n)
	{
		delete childTabs[n];
	}
	childTabs.resize(numTabs);
	for (unsigned n = prevNumTabs; n < childTabs.size(); ++n)
	{
		childTabs[n] = new WIDGET(this);
		childTabs[n]->show(n == currentTab);
	}
	setTab(currentTab);  // Ensure that currentTab is still valid.
	fixChildGeometry();
}

/* Add a widget to a form */
bool formAddWidget(W_FORM *psForm, WIDGET *psWidget, W_INIT const *psInit)
{
	ASSERT_OR_RETURN(false, psWidget != NULL && psForm != NULL, "Invalid pointer");

	if (psForm->style & WFORM_TABBED)
	{
		W_TABFORM *psTabForm = (W_TABFORM *)psForm;
		ASSERT_OR_RETURN(false, psInit->majorID < psTabForm->childTabs.size(), "Major tab does not exist");
		psTabForm->childTabs[psInit->majorID]->attach(psWidget);
	}
	else
	{
		psForm->attach(psWidget);
	}

	return true;
}

unsigned W_CLICKFORM::getState()
{
	return state & (WBUT_DISABLE | WBUT_LOCK | WBUT_CLICKLOCK | WBUT_FLASH | WBUT_DOWN | WBUT_HIGHLIGHT);
}

void W_CLICKFORM::setState(unsigned newState)
{
	ASSERT(!((newState & WBUT_LOCK) && (newState & WBUT_CLICKLOCK)), "Cannot have both WBUT_LOCK and WBUT_CLICKLOCK");

	unsigned mask = WBUT_DISABLE | WBUT_LOCK | WBUT_CLICKLOCK;
	state = (state & ~mask) | (newState & mask);
}

static W_TABFORM *widgGetTabbedFormById(W_SCREEN *const psScreen, const UDWORD id)
{
	W_TABFORM *const psForm = (W_TABFORM *)widgGetFromID(psScreen, id);

	if (psForm == NULL
	    || psForm->type != WIDG_FORM
	    || !(psForm->style & WFORM_TABBED))
	{
		return NULL;
	}

	return psForm;
}


/* Set the current tabs for a tab form */
void widgSetTabs(W_SCREEN *psScreen, UDWORD id, UWORD major)
{
	W_TABFORM *const psForm = widgGetTabbedFormById(psScreen, id);

	ASSERT(psForm != NULL, "widgSetTabs: Invalid tab form pointer");

	if (psForm == NULL)
	{
		return; /* make us work fine in no assert compiles */
	}

	ASSERT(major < widgGetNumTabMajor(psScreen, id), "widgSetTabs id=%u: invalid major id %u >= max %u", id,
	       major, widgGetNumTabMajor(psScreen, id));

	// Make sure to bail out when we've been passed out-of-bounds major or minor numbers
	if (major >= widgGetNumTabMajor(psScreen, id))
	{
		return;
	}

	psForm->setTab(major);
}

int widgGetNumTabMajor(W_SCREEN *psScreen, UDWORD id)
{
	W_TABFORM *const psForm = widgGetTabbedFormById(psScreen, id);

	ASSERT_OR_RETURN(0, psForm != NULL, "Couldn't find a tabbed form with ID %u", id);

	return psForm->numTabs();
}

/* Get the current tabs for a tab form */
void widgGetTabs(W_SCREEN *psScreen, UDWORD id, UWORD *pMajor)
{
	W_TABFORM *const psForm = widgGetTabbedFormById(psScreen, id);
	ASSERT_OR_RETURN(, psForm != NULL, "Couldn't find a tabbed form with ID %u", id);

	*pMajor = psForm->tab();
}

void W_TABFORM::fixChildGeometry()
{
	bool horizontal = majorPos == WFORM_TABTOP || majorPos == WFORM_TABBOTTOM;
	bool leftOrTop = majorPos == WFORM_TABLEFT || majorPos == WFORM_TABTOP;

	int dw = horizontal? 0 : -tabMajorThickness;
	int dh = horizontal? -tabMajorThickness : 0;
	int dx = -dw*leftOrTop;
	int dy = -dh*leftOrTop;

	for (Children::iterator i = childTabs.begin(); i != childTabs.end(); ++i)
	{
		(*i)->setGeometry(dx, dy, width() + dw, height() + dh);
	}
}

QRect W_TABFORM::tabPos(int index)
{
	if (NO_DISPLAY_SINGLE_TABS && numTabs() == 1)
	{
		return QRect();  // Don't display single tabs.
	}

	int n = index - tabPage()*maxTabsShown;
	if (n < 0 || (unsigned)n >= maxTabsShown)
	{
		return QRect();  // Tab icon is not currently visible.
	}

	int pos = majorOffset + n*(majorSize + tabMajorGap);

	switch (majorPos)
	{
	case WFORM_TABTOP:    return QRect(pos, 0,                            majorSize, tabMajorThickness);
	case WFORM_TABBOTTOM: return QRect(pos, height() - tabMajorThickness, majorSize, tabMajorThickness);
	case WFORM_TABLEFT:   return QRect(0,                           pos,  tabMajorThickness, majorSize);
	case WFORM_TABRIGHT:  return QRect(width() - tabMajorThickness, pos,  tabMajorThickness, majorSize);
	}
	ASSERT(false, "Cannot have a tabbed form with no major tabs");
	return QRect();
}

int W_TABFORM::pickTab(int clickX, int clickY)
{
	for (int i = 0; i < numTabs(); ++i)
	{
		QRect r = tabPos(i);
		if (r.isNull())
		{
			continue;
		}

		if (r.contains(clickX, clickY))
		{
			return i;
		}
	}
	return -1;
}

void W_TABFORM::run(W_CONTEXT *psContext)
{
	// If the mouse is over the form, see if any tabs need to be highlighted.
	tabHiLite = pickTab(psContext->mx, psContext->my);
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
}

void W_FORM::clicked(W_CONTEXT *, WIDGET_KEY)
{
	// Stop the tip if there is one.
	tipStop(this);
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

			if (AudioCallback != NULL)
			{
				AudioCallback(ClickedAudioID);
			}
		}
	}
}


/* Respond to a mouse form up */
void W_TABFORM::released(W_CONTEXT *psContext, WIDGET_KEY)
{
	int newTab = pickTab(psContext->mx, psContext->my);
	if (newTab >= 0)
	{
		// Clicked on a tab.
		setTab(newTab);
		screenPointer->setReturn(this);
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

	if (AudioCallback != NULL)
	{
		AudioCallback(HilightAudioID);
	}
}


/* Respond to the mouse moving off a form */
void W_FORM::highlightLost()
{
	// Clear the tool tip if there is one.
	tipStop(this);
}

void W_TABFORM::highlightLost()
{
	W_FORM::highlightLost();

	tabHiLite = (UWORD)(-1);
}

void W_CLICKFORM::highlightLost()
{
	W_FORM::highlightLost();

	state &= ~(WBUT_DOWN | WBUT_HIGHLIGHT);
}

void W_FORM::display(int xOffset, int yOffset)
{
	if (displayFunction != NULL)
	{
		displayFunction(this, xOffset, yOffset);
		return;
	}

	if ((style & WFORM_INVISIBLE) == 0)
	{
		int x0 = x() + xOffset;
		int y0 = y() + yOffset;
		int x1 = x0 + width();
		int y1 = y0 + height();

		iV_ShadowBox(x0, y0, x1, y1, 1, WZCOL_FORM_LIGHT, WZCOL_FORM_DARK, WZCOL_FORM_BACKGROUND);
	}
}

void W_CLICKFORM::display(int xOffset, int yOffset)
{
	if (displayFunction != NULL)
	{
		displayFunction(this, xOffset, yOffset);
		return;
	}

	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	/* Display the border */
	if ((state & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0)
	{
		iV_ShadowBox(x0, y0, x1, y1, 1, WZCOL_FORM_DARK, WZCOL_FORM_LIGHT, WZCOL_FORM_BACKGROUND);
	}
	else
	{
		iV_ShadowBox(x0, y0, x1, y1, 1, WZCOL_FORM_LIGHT, WZCOL_FORM_DARK, WZCOL_FORM_BACKGROUND);
	}
}

void W_TABFORM::display(int xOffset, int yOffset)
{
	if (displayFunction != NULL)
	{
		displayFunction(this, xOffset, yOffset);
		return;
	}

	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	for (int i = 0; i < numTabs(); ++i)
	{
		QRect r = tabPos(i);
		if (r.isNull())
		{
			continue;
		}

		bool isCurrent = (unsigned)i == currentTab;
		bool isHighlighted = (unsigned)i == tabHiLite;
		if (pTabDisplay)
		{
			pTabDisplay(this, majorPos, i - tabPage()*maxTabsShown, isCurrent, isHighlighted, x0 + r.x(), y0 + r.y(), r.width(), r.height());
		}
		else
		{
			int x = x0 + r.x();
			int y = y0 + r.y();
			int x1 = x + r.width();
			int y1 = y + r.height();
			if (isCurrent)
			{
				/* Fill in the tab */
				pie_BoxFill(x + 1, y + 1, x1 - 1, y1, WZCOL_FORM_BACKGROUND);
				/* Draw the outline */
				iV_Line(x, y + 2, x, y1 - 1, WZCOL_FORM_LIGHT);
				iV_Line(x, y + 2, x + 2, y, WZCOL_FORM_LIGHT);
				iV_Line(x + 2, y, x1 - 1, y, WZCOL_FORM_LIGHT);
				iV_Line(x1, y + 1, x1, y1, WZCOL_FORM_DARK);
			}
			else
			{
				/* Fill in the tab */
				pie_BoxFill(x + 1, y + 2, x1 - 1, y1 - 1, WZCOL_FORM_BACKGROUND);
				/* Draw the outline */
				iV_Line(x, y + 3, x, y1 - 1, WZCOL_FORM_LIGHT);
				iV_Line(x, y + 3, x + 2, y + 1, WZCOL_FORM_LIGHT);
				iV_Line(x + 2, y + 1, x1 - 1, y + 1, WZCOL_FORM_LIGHT);
				iV_Line(x1, y + 2, x1, y1 - 1, WZCOL_FORM_DARK);
			}
			if (isHighlighted)
			{
				/* Draw the highlight box */
				iV_Box(x + 2, y + 4, x1 - 3, y1 - 3, WZCOL_FORM_HILITE);
			}
		}
	}
}

void W_CLICKFORM::setTip(QString string)
{
	pTip = string;
}
