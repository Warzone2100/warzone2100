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


/* Store the position of a tab */
struct TAB_POS
{
	SDWORD		index;
	SDWORD		x, y;
	UDWORD		width, height;
	SDWORD		TabMultiplier;			//Added to keep track of tab scroll
};

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
	, TabMultiplier(0)
	, maxTabsShown(MAX_TAB_SMALL_SHOWN - 1)  // Equal to TAB_SEVEN, which is equal to 7.
	, pTip(NULL)
	// apMajorTips
	, pTabDisplay(NULL)
	, pFormDisplay(NULL)
{
	memset(apMajorTips, 0, sizeof(apMajorTips));
}

W_FORM::W_FORM(W_FORMINIT const *init)
	: WIDGET(init, WIDG_FORM)
	, disableChildren(init->disableChildren)
	, Ax0(0), Ay0(0), Ax1(0), Ay1(0)  // These assignments were previously done by a memset.
	, animCount(0)
	, startTime(0)                    // This assignment was previously done by a memset.
	, psLastHiLite(NULL)
	, psWidgets(NULL)
{
	if (display == NULL)
	{
		display = formDisplay;
	}

	aColours[WCOL_BKGRND]    = WZCOL_FORM_BACKGROUND;
	aColours[WCOL_TEXT]      = WZCOL_FORM_TEXT;
	aColours[WCOL_LIGHT]     = WZCOL_FORM_LIGHT;
	aColours[WCOL_DARK]      = WZCOL_FORM_DARK;
	aColours[WCOL_HILITE]    = WZCOL_FORM_HILITE;
	aColours[WCOL_CURSOR]    = WZCOL_FORM_CURSOR;
	aColours[WCOL_TIPBKGRND] = WZCOL_FORM_TIP_BACKGROUND;
	aColours[WCOL_DISABLE]   = WZCOL_FORM_DISABLE;

	ASSERT((init->style & ~(WFORM_TABBED | WFORM_INVISIBLE | WFORM_CLICKABLE | WFORM_NOCLICKMOVE | WFORM_NOPRIMARY | WFORM_SECONDARY | WIDG_HIDDEN)) == 0, "Unknown style bit");
	ASSERT((init->style & WFORM_TABBED) == 0 || (init->style & (WFORM_INVISIBLE | WFORM_CLICKABLE)) == 0, "Tabbed form cannot be invisible or clickable");
	ASSERT((init->style & WFORM_INVISIBLE) == 0 || (init->style & WFORM_CLICKABLE) == 0, "Cannot have an invisible clickable form");
	ASSERT((init->style & WFORM_CLICKABLE) != 0 || (init->style & (WFORM_NOPRIMARY | WFORM_SECONDARY)) == 0, "Cannot set keys if the form isn't clickable");
}

W_FORM::~W_FORM()
{
	widgReleaseWidgetList(psWidgets);
	CheckpsMouseOverWidget(this);  // clear global if needed
}

W_CLICKFORM::W_CLICKFORM(W_FORMINIT const *init)
	: W_FORM(init)
	, state(WCLICK_NORMAL)
	, pTip(QString::fromUtf8(init->pTip))
	, HilightAudioID(WidgGetHilightAudioID())
	, ClickedAudioID(WidgGetClickedAudioID())
	, AudioCallback(WidgGetAudioCallback())
{
	if (init->pDisplay == NULL)
	{
		display = formDisplayClickable;
	}
}

W_MAJORTAB::W_MAJORTAB()
	: psWidgets(NULL)
{
}

W_TABFORM::W_TABFORM(W_FORMINIT const *init)
	: W_FORM(init)
	, majorPos(init->majorPos)
	, majorSize(init->majorSize)
	, tabMajorThickness(init->tabMajorThickness)
	, tabMajorGap(init->tabMajorGap)
	, tabVertOffset(init->tabVertOffset)
	, tabHorzOffset(init->tabHorzOffset)
	, majorOffset(init->majorOffset)
	, majorT(0)
	, state(0)
	, tabHiLite(~0)
	, TabMultiplier(init->TabMultiplier)
	, maxTabsShown(init->maxTabsShown)
	, numStats(init->numStats)
	, numButtons(init->numButtons)
	, pTabDisplay(init->pTabDisplay)
{
	/* Allocate the memory for tool tips and copy them in */
	/* Set up the tab data.
	 * All widget pointers have been zeroed by the memset above.
	 */
	numMajor = init->numMajor;
	for (unsigned major = 0; major < init->numMajor; ++major)
	{
		/* Check for a tip for the major tab */
		asMajor[major].pTip = QString::fromUtf8(init->apMajorTips[major]);
	}

	if (init->pDisplay == NULL)
	{
		display = formDisplayTabbed;
	}

	ASSERT(init->numMajor != 0, "Must have at least one major tab on a tabbed form");
	ASSERT(init->numMajor < WFORM_MAXMAJOR, "Too many Major tabs");
}

/* Free a tabbed form widget */
W_TABFORM::~W_TABFORM()
{
	W_FORMGETALL sGetAll;
	formInitGetAllWidgets(this, &sGetAll);
	for (WIDGET *psCurr = formGetAllWidgets(&sGetAll); psCurr != NULL; psCurr = formGetAllWidgets(&sGetAll))
	{
		widgReleaseWidgetList(psCurr);
	}
}

/* Add a widget to a form */
bool formAddWidget(W_FORM *psForm, WIDGET *psWidget, W_INIT const *psInit)
{
	W_TABFORM	*psTabForm;
	WIDGET		**ppsList;
	W_MAJORTAB	*psMajor;

	ASSERT(psWidget != NULL,
	       "formAddWidget: Invalid widget pointer");

	if (psForm->style & WFORM_TABBED)
	{
		ASSERT(psForm != NULL,
		       "formAddWidget: Invalid tab form pointer");
		psTabForm = (W_TABFORM *)psForm;
		if (psInit->majorID >= psTabForm->numMajor)
		{
			ASSERT(false, "formAddWidget: Major tab does not exist");
			return false;
		}
		psMajor = psTabForm->asMajor + psInit->majorID;
		ppsList = &psMajor->psWidgets;
		psWidget->psNext = *ppsList;
		*ppsList = psWidget;
	}
	else
	{
		ASSERT(psForm != NULL,
		       "formAddWidget: Invalid form pointer");
		psWidget->psNext = psForm->psWidgets;
		psForm->psWidgets = psWidget;
	}

	return true;
}


static const unsigned clickFormflagMap[] = {WCLICK_GREY,      WBUT_DISABLE,
                                            WCLICK_LOCKED,    WBUT_LOCK,
                                            WCLICK_CLICKLOCK, WBUT_CLICKLOCK};

unsigned W_CLICKFORM::getState()
{
	unsigned retState = 0;
	for (unsigned i = 0; i < ARRAY_SIZE(clickFormflagMap); i += 2)
	{
		if ((state & clickFormflagMap[i]) != 0)
		{
			retState |= clickFormflagMap[i + 1];
		}
	}

	return retState;
}

void W_CLICKFORM::setState(unsigned newState)
{
	ASSERT(!((state & WBUT_LOCK) && (state & WBUT_CLICKLOCK)),
	       "widgSetButtonState: Cannot have WBUT_LOCK and WBUT_CLICKLOCK");

	for (unsigned i = 0; i < ARRAY_SIZE(clickFormflagMap); i += 2)
	{
		state &= ~clickFormflagMap[i];
		if ((newState & clickFormflagMap[i + 1]) != 0)
		{
			state |= clickFormflagMap[i];
		}
	}
}

/* Return the widgets currently displayed by a form */
WIDGET *formGetWidgets(W_FORM *psWidget)
{
	W_TABFORM	*psTabForm;
	W_MAJORTAB	*psMajor;

	if (psWidget->style & WFORM_TABBED)
	{
		psTabForm = (W_TABFORM *)psWidget;
		psMajor = psTabForm->asMajor + psTabForm->majorT;
		return psMajor->psWidgets;
	}
	else
	{
		return psWidget->psWidgets;
	}
}


/* Initialise the formGetAllWidgets function */
void formInitGetAllWidgets(W_FORM *psWidget, W_FORMGETALL *psCtrl)
{
	if (psWidget->style & WFORM_TABBED)
	{
		psCtrl->psGAWList = NULL;
		psCtrl->psGAWForm = (W_TABFORM *)psWidget;
		psCtrl->psGAWMajor = psCtrl->psGAWForm->asMajor;
		psCtrl->GAWMajor = 0;
	}
	else
	{
		psCtrl->psGAWList = psWidget->psWidgets;
		psCtrl->psGAWForm = NULL;
	}
}


/* Repeated calls to this function will return widget lists
 * until all widgets in a form have been returned.
 * When a NULL list is returned, all widgets have been seen.
 */
WIDGET *formGetAllWidgets(W_FORMGETALL *psCtrl)
{
	WIDGET	*psRetList;

	if (psCtrl->psGAWForm == NULL)
	{
		/* Not a tabbed form, return the list */
		psRetList = psCtrl->psGAWList;
		psCtrl->psGAWList = NULL;
	}
	else
	{
		/* Working with a tabbed form - search for the first widget list */
		psRetList = NULL;
		while (psRetList == NULL && psCtrl->GAWMajor < psCtrl->psGAWForm->numMajor)
		{
			psRetList = psCtrl->psGAWMajor->psWidgets;
			++psCtrl->GAWMajor;
			++psCtrl->psGAWMajor;
		}
	}

	return psRetList;
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

	psForm->majorT = major;
}

int widgGetNumTabMajor(W_SCREEN *psScreen, UDWORD id)
{
	W_TABFORM *const psForm = widgGetTabbedFormById(psScreen, id);

	ASSERT(psForm != NULL, "Couldn't find a tabbed form with ID %u", id);
	if (psForm == NULL)
	{
		return 0;
	}

	return psForm->numMajor;
}

/* Get the current tabs for a tab form */
void widgGetTabs(W_SCREEN *psScreen, UDWORD id, UWORD *pMajor)
{
	W_TABFORM	*psForm;

	psForm = (W_TABFORM *)widgGetFromID(psScreen, id);
	if (psForm == NULL || psForm->type != WIDG_FORM ||
	    !(psForm->style & WFORM_TABBED))
	{
		ASSERT(false, "widgGetTabs: couldn't find tabbed form from id");
		return;
	}
	ASSERT(psForm != NULL, "widgGetTabs: Invalid tab form pointer");
	ASSERT(psForm->majorT < psForm->numMajor, "widgGetTabs: invalid major id %u >= max %u", psForm->majorT, psForm->numMajor);

	*pMajor = psForm->majorT;
}


/* Set a colour on a form */
void widgSetColour(W_SCREEN *psScreen, UDWORD id, UDWORD index, PIELIGHT colour)
{
	W_TABFORM	*psForm = (W_TABFORM *)widgGetFromID(psScreen, id);

	ASSERT_OR_RETURN(, psForm && psForm->type == WIDG_FORM, "Could not find form from id %u", id);
	ASSERT_OR_RETURN(, index < WCOL_MAX, "Colour id %u out of range", index);
	psForm->aColours[index] = colour;
}


/* Return the origin on the form from which button locations are calculated */
void formGetOrigin(W_FORM *psWidget, SDWORD *pXOrigin, SDWORD *pYOrigin)
{
	W_TABFORM	*psTabForm;

	ASSERT(psWidget != NULL,
	       "formGetOrigin: Invalid form pointer");

	if (psWidget->style & WFORM_TABBED)
	{
		psTabForm = (W_TABFORM *)psWidget;
		if (psTabForm->majorPos == WFORM_TABTOP)
		{
			*pYOrigin = psTabForm->tabMajorThickness;
		}
		else
		{
			*pYOrigin = 0;
		}
		if (psTabForm->majorPos == WFORM_TABLEFT)
		{
			*pXOrigin = psTabForm->tabMajorThickness;
		}
		else
		{
			*pXOrigin = 0;
		}
	}
	else
	{
		*pXOrigin = 0;
		*pYOrigin = 0;
	}
}


// Currently in game, I can only find that warzone uses horizontal tabs.
// So ONLY this routine was modified.  Will have to modify the vert. tab
// routine if we ever use it.
// Choose a horizontal tab from a coordinate
static bool formPickHTab(TAB_POS *psTabPos,
        SDWORD x0, SDWORD y0,
        UDWORD width, UDWORD height, UDWORD gap,
        UDWORD number, SDWORD fx, SDWORD fy, unsigned maxTabsShown)
{
	SDWORD	x, y1;
	UDWORD	i;

#if NO_DISPLAY_SINGLE_TABS
	if (number == 1)
	{
		// Don't have single tabs
		return false;
	}
#endif

	x = x0;
	y1 = y0 + height;

	// We need to filter out some tabs, since we can only display 7 at a time, with
	// the scroll tabs in place, and 8 without. MAX_TAB_SMALL_SHOWN (currently 8) is the case
	// when we do NOT want scrolltabs, and are using smallTab icons.
	// Also need to check if the TabMultiplier is set or not, if not then it means
	// we have not yet added the code to display/handle the tab scroll buttons.
	// At this time, I think only the design screen has this limitation of only 8 tabs.
	if (number > maxTabsShown  && psTabPos->TabMultiplier) // of course only do this if we actually need >8 tabs.
	{
		number -= (psTabPos->TabMultiplier - 1) * maxTabsShown;
		number = std::min(number, maxTabsShown);  // is it still > than maxTabsShown?
	}
	else if (number > maxTabsShown + 1)
	{
		// we need to clip the tab count to max amount *without* the scrolltabs visible.
		// The reason for this, is that in design screen & 'feature' debug & others(?),
		// we can get over max # of tabs that the game originally supported.
		// This made it look bad.
		number = maxTabsShown + 1;
	}

	for (i = 0; i < number; i++)
	{
//		if (fx >= x && fx <= x + (SDWORD)(width - gap) &&
		if (fx >= x && fx <= x + (SDWORD)(width) &&
		    fy >= y0 && fy <= y1)
		{
			// found a tab under the coordinate
			if (psTabPos->TabMultiplier)	//Checks to see we need the extra tab scroll buttons
			{
				// holds the VIRTUAL tab #, since obviously, we can't display more than 7
				psTabPos->index = (i % maxTabsShown) + ((psTabPos->TabMultiplier - 1) * maxTabsShown);
			}
			else
			{
				// This is a normal request.
				psTabPos->index = i;
			}
			psTabPos->x = x ;
			psTabPos->y = y0;
			psTabPos->width = width;
			psTabPos->height = height;
			return true;
		}

		x += width + gap;
	}

	/* Didn't find any  */
	return false;
}

// NOTE: This routine is NOT modified to use the tab scroll buttons.
// Choose a vertical tab from a coordinate
static bool formPickVTab(TAB_POS *psTabPos,
        SDWORD x0, SDWORD y0,
        UDWORD width, UDWORD height, UDWORD gap,
        UDWORD number, SDWORD fx, SDWORD fy)
{
	SDWORD	x1, y;
	UDWORD	i;

#if NO_DISPLAY_SINGLE_TABS
	if (number == 1)
	{
		/* Don't have single tabs */
		return false;
	}
#endif

	x1 = x0 + width;
	y = y0;
	for (i = 0; i < number; i++)
	{
		if (fx >= x0 && fx <= x1 &&
		    fy >= y && fy <= y + (SDWORD)(height))
//			fy >= y && fy <= y + (SDWORD)(height - gap))
		{
			/* found a tab under the coordinate */
			psTabPos->index = i;
			psTabPos->x = x0;
			psTabPos->y = y;
			psTabPos->width = width;
			psTabPos->height = height;
			return true;
		}

		y += height + gap;
	}

	/* Didn't find any */
	return false;
}


/* Find which tab is under a form coordinate */
static bool formPickTab(W_TABFORM *psForm, UDWORD fx, UDWORD fy,
        TAB_POS *psTabPos)
{
	SDWORD		x0, y0, x1, y1;

	/* Get the basic position of the form */
	x0 = 0;
	y0 = 0;
	x1 = psForm->width;
	y1 = psForm->height;

	/* Adjust for where the tabs are */
	if (psForm->majorPos == WFORM_TABLEFT)
	{
		x0 += psForm->tabMajorThickness;
	}
	if (psForm->majorPos == WFORM_TABRIGHT)
	{
		x1 -= psForm->tabMajorThickness;
	}
	if (psForm->majorPos == WFORM_TABTOP)
	{
		y0 += psForm->tabMajorThickness;
	}
	if (psForm->majorPos == WFORM_TABBOTTOM)
	{
		y1 -= psForm->tabMajorThickness;
	}

	psTabPos->TabMultiplier = psForm->TabMultiplier;
	/* Check the major tabs */
	switch (psForm->majorPos)
	{
	case WFORM_TABTOP:
		if (formPickHTab(psTabPos, x0 + psForm->majorOffset, y0 - psForm->tabMajorThickness,
		        psForm->majorSize, psForm->tabMajorThickness, psForm->tabMajorGap,
		        psForm->numMajor, fx, fy, psForm->maxTabsShown))
		{
			return true;
		}
		break;
	case WFORM_TABBOTTOM:
		if (formPickHTab(psTabPos, x0 + psForm->majorOffset, y1,
		        psForm->majorSize, psForm->tabMajorThickness, psForm->tabMajorGap,
		        psForm->numMajor, fx, fy, psForm->maxTabsShown))
		{
			return true;
		}
		break;
	case WFORM_TABLEFT:
		if (formPickVTab(psTabPos, x0 - psForm->tabMajorThickness, y0 + psForm->majorOffset,
		        psForm->tabMajorThickness, psForm->majorSize, psForm->tabMajorGap,
		        psForm->numMajor, fx, fy))
		{
			return true;
		}
		break;
	case WFORM_TABRIGHT:
		if (formPickVTab(psTabPos, x1, y0 + psForm->majorOffset,
		        psForm->tabMajorThickness, psForm->majorSize, psForm->tabMajorGap,
		        psForm->numMajor, fx, fy))
		{
			return true;
		}
		break;
	case WFORM_TABNONE:
		ASSERT(false, "Cannot have a tabbed form with no major tabs");
		break;
	}

	return false;
}

extern UDWORD realTime;  // FIXME Include a header...

/* Run a form widget */
void W_TABFORM::run(W_CONTEXT *psContext)
{
	int mx = psContext->mx;
	int my = psContext->my;

	// If the mouse is over the form, see if any tabs need to be highlighted.
	if (mx < 0 || mx > width ||
	    my < 0 || my > height)
	{
		return;
	}

	TAB_POS sTabPos;
	memset(&sTabPos, 0x0, sizeof(TAB_POS));
	if (formPickTab(this, mx, my, &sTabPos))
	{
		if (tabHiLite != (UWORD)sTabPos.index)
		{
			// Got a new tab - start the tool tip if there is one.
			tabHiLite = (UWORD)sTabPos.index;
			QString pTip;
			pTip = asMajor[sTabPos.index].pTip;
			if (!pTip.isEmpty())
			{
				// Got a tip - start it off.
				tipStart(this, pTip, psContext->psScreen->TipFontID, aColours, sTabPos.x + psContext->xOffset, sTabPos.y + psContext->yOffset, sTabPos.width, sTabPos.height);
			}
			else
			{
				// No tip - clear any old tip.
				tipStop(this);
			}
		}
	}
	else
	{
		// No tab - clear the tool tip.
		tipStop(this);
		// And clear the highlight.
		tabHiLite = (UWORD)(-1);
	}
}

void W_CLICKFORM::run(W_CONTEXT *)
{
	if (state & WCLICK_FLASH)
	{
		if (((realTime / 250) % 2) == 0)
		{
			state &= ~WCLICK_FLASHON;
		}
		else
		{
			state |= WCLICK_FLASHON;
		}
	}
}

void W_CLICKFORM::setFlash(bool enable)
{
	if (enable)
	{
		state |= WCLICK_FLASH;
	}
	else
	{
		state &= ~(WCLICK_FLASH | WCLICK_FLASHON);
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
	if (!(state & (WCLICK_GREY | WCLICK_LOCKED)))
	{
		// Check this is the correct key
		if ((!(style & WFORM_NOPRIMARY) && key == WKEY_PRIMARY) ||
			((style & WFORM_SECONDARY) && key == WKEY_SECONDARY))
		{
			state &= ~WCLICK_FLASH;	// Stop it flashing
			state &= ~WCLICK_FLASHON;
			state |= WCLICK_DOWN;

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
	TAB_POS sTabPos;

	/* See if a tab has been clicked on */
	if (formPickTab(this, psContext->mx, psContext->my, &sTabPos))
	{
		// Clicked on a tab.
		ASSERT(majorT < numMajor, "invalid major id %u >= max %u", sTabPos.index, numMajor);
		majorT = sTabPos.index;
		widgSetReturn(psContext->psScreen, this);
	}
}

void W_CLICKFORM::released(W_CONTEXT *psContext, WIDGET_KEY key)
{
	if ((state & WCLICK_DOWN) != 0)
	{
		// Check this is the correct key.
		if ((!(style & WFORM_NOPRIMARY) && key == WKEY_PRIMARY) ||
		    ((style & WFORM_SECONDARY) && key == WKEY_SECONDARY))
		{
			widgSetReturn(psContext->psScreen, this);
			state &= ~WCLICK_DOWN;
		}
	}
}


/* Respond to a mouse moving over a form */
void W_CLICKFORM::highlight(W_CONTEXT *psContext)
{
	state |= WCLICK_HILITE;

	// If there is a tip string start the tool tip.
	if (!pTip.isEmpty())
	{
		tipStart(this, pTip, psContext->psScreen->TipFontID, psContext->psForm->aColours, x + psContext->xOffset, y + psContext->yOffset, width, height);
	}

	if (AudioCallback != NULL)
	{
		AudioCallback(HilightAudioID);
	}
}


/* Respond to the mouse moving off a form */
void W_FORM::highlightLost(W_CONTEXT *psContext)
{
	// If one of the widgets was highlighted that has to lose it as well.
	if (psLastHiLite != NULL)
	{
		psLastHiLite->highlightLost(psContext);
	}
	// Clear the tool tip if there is one.
	tipStop(this);
}

void W_TABFORM::highlightLost(W_CONTEXT *psContext)
{
	W_FORM::highlightLost(psContext);

	tabHiLite = (UWORD)(-1);
}

void W_CLICKFORM::highlightLost(W_CONTEXT *psContext)
{
	W_FORM::highlightLost(psContext);

	state &= ~(WCLICK_DOWN | WCLICK_HILITE);
}

/* Display a form */
void formDisplay(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD	x0, y0, x1, y1;

	if (!(psWidget->style & WFORM_INVISIBLE))
	{
		x0 = psWidget->x + xOffset;
		y0 = psWidget->y + yOffset;
		x1 = x0 + psWidget->width;
		y1 = y0 + psWidget->height;

		iV_ShadowBox(x0, y0, x1, y1, 1, pColours[WCOL_LIGHT], pColours[WCOL_DARK], pColours[WCOL_BKGRND]);
	}
}


/* Display a clickable form */
void formDisplayClickable(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD			x0, y0, x1, y1;
	W_CLICKFORM		*psForm;

	psForm = (W_CLICKFORM *)psWidget;
	x0 = psWidget->x + xOffset;
	y0 = psWidget->y + yOffset;
	x1 = x0 + psWidget->width;
	y1 = y0 + psWidget->height;

	/* Display the border */
	if (psForm->state & (WCLICK_DOWN | WCLICK_LOCKED | WCLICK_CLICKLOCK))
	{
		iV_ShadowBox(x0, y0, x1, y1, 1, pColours[WCOL_DARK], pColours[WCOL_LIGHT], pColours[WCOL_BKGRND]);
	}
	else
	{
		iV_ShadowBox(x0, y0, x1, y1, 1, pColours[WCOL_LIGHT], pColours[WCOL_DARK], pColours[WCOL_BKGRND]);
	}
}


/* Draw top tabs */
static void formDisplayTTabs(W_TABFORM *psForm, SDWORD x0, SDWORD y0,
        UDWORD width, UDWORD height,
        UDWORD number, UDWORD selected, UDWORD hilite,
        PIELIGHT *pColours, UDWORD TabGap)
{
	SDWORD	x, x1, y1;
	UDWORD	i, drawnumber;

#if NO_DISPLAY_SINGLE_TABS
	if (number == 1)
	{
		/* Don't display single tabs */
		return;
	}
#endif

	x = x0 + 2;
	x1 = x + width - 2;
	y1 = y0 + height;
	if (number > psForm->maxTabsShown)  //we can display 8 tabs fine with no extra voodoo.
	{
		// We do NOT want to draw all the tabs once we have drawn 7 tabs
		// Both selected & hilite are converted from virtual tab range, to a range
		// that is seen on the form itself.  This would be 0-6 (7 tabs)
		// We also fix drawnumber, so we don't display too many tabs since the pages
		// will be empty.
		drawnumber = (number - ((psForm->TabMultiplier - 1) * psForm->maxTabsShown));
		if (drawnumber > psForm->maxTabsShown)
		{
			drawnumber = psForm->maxTabsShown ;
		}
		selected = (selected % psForm->maxTabsShown);	//Go from Virtual range, to our range

		if (hilite != 65535)			//sigh.  Don't blame me for this!It is THEIR 'hack'.
		{
			hilite = hilite % psForm->maxTabsShown;    //we want to hilite tab 0 - 6.
		}
	}
	else
	{
		// normal draw
		drawnumber = number;
	}
	for (i = 0; i < drawnumber; i++)
	{
		if (psForm->pTabDisplay)
		{
			psForm->pTabDisplay(psForm, WFORM_TABTOP, i, i == selected, i == hilite, x, y0, width, height);
		}
		else
		{
			if (i == selected)
			{
				/* Fill in the tab */
				pie_BoxFill(x + 1, y0 + 1, x1 - 1, y1, pColours[WCOL_BKGRND]);
				/* Draw the outline */
				iV_Line(x, y0 + 2, x, y1 - 1, pColours[WCOL_LIGHT]);
				iV_Line(x, y0 + 2, x + 2, y0, pColours[WCOL_LIGHT]);
				iV_Line(x + 2, y0, x1 - 1, y0, pColours[WCOL_LIGHT]);
				iV_Line(x1, y0 + 1, x1, y1, pColours[WCOL_DARK]);
			}
			else
			{
				/* Fill in the tab */
				pie_BoxFill(x + 1, y0 + 2, x1 - 1, y1 - 1, pColours[WCOL_BKGRND]);
				/* Draw the outline */
				iV_Line(x, y0 + 3, x, y1 - 1, pColours[WCOL_LIGHT]);
				iV_Line(x, y0 + 3, x + 2, y0 + 1, pColours[WCOL_LIGHT]);
				iV_Line(x + 2, y0 + 1, x1 - 1, y0 + 1, pColours[WCOL_LIGHT]);
				iV_Line(x1, y0 + 2, x1, y1 - 1, pColours[WCOL_DARK]);
			}
			if (i == hilite)
			{
				/* Draw the hilite box */
				iV_Box(x + 2, y0 + 4, x1 - 3, y1 - 3, pColours[WCOL_HILITE]);
			}
		}
		x += width + TabGap;
		x1 += width + TabGap;
	}
}


/* Draw bottom tabs */
static void formDisplayBTabs(W_TABFORM *psForm, SDWORD x0, SDWORD y0,
        UDWORD width, UDWORD height,
        UDWORD number, UDWORD selected, UDWORD hilite,
        PIELIGHT *pColours, UDWORD TabGap)
{
	SDWORD	x, x1, y1;
	UDWORD	i;

#if NO_DISPLAY_SINGLE_TABS
	if (number == 1)
	{
		/* Don't display single tabs */
		return;
	}
#endif

	x = x0 + 2;
	x1 = x + width - 2;
	y1 = y0 + height;
	for (i = 0; i < number; i++)
	{
		if (psForm->pTabDisplay)
		{
			psForm->pTabDisplay(psForm, WFORM_TABBOTTOM, i, i == selected, i == hilite, x, y0, width, height);
		}
		else
		{
			if (i == selected)
			{
				/* Fill in the tab */
				pie_BoxFill(x + 1, y0, x1 - 1, y1 - 1, pColours[WCOL_BKGRND]);
				/* Draw the outline */
				iV_Line(x, y0, x, y1 - 1, pColours[WCOL_LIGHT]);
				iV_Line(x, y1, x1 - 3, y1, pColours[WCOL_DARK]);
				iV_Line(x1 - 2, y1, x1, y1 - 2, pColours[WCOL_DARK]);
				iV_Line(x1, y1 - 3, x1, y0 + 1, pColours[WCOL_DARK]);
			}
			else
			{
				/* Fill in the tab */
				pie_BoxFill(x + 1, y0 + 1, x1 - 1, y1 - 2, pColours[WCOL_BKGRND]);
				/* Draw the outline */
				iV_Line(x, y0 + 1, x, y1 - 1, pColours[WCOL_LIGHT]);
				iV_Line(x + 1, y1 - 1, x1 - 3, y1 - 1, pColours[WCOL_DARK]);
				iV_Line(x1 - 2, y1 - 1, x1, y1 - 3, pColours[WCOL_DARK]);
				iV_Line(x1, y1 - 4, x1, y0 + 1, pColours[WCOL_DARK]);
			}
			if (i == hilite)
			{
				/* Draw the hilite box */
				iV_Box(x + 2, y0 + 3, x1 - 3, y1 - 4, pColours[WCOL_HILITE]);
			}
		}
		x += width + TabGap;
		x1 += width + TabGap;
	}
}


/* Draw left tabs */
static void formDisplayLTabs(W_TABFORM *psForm, SDWORD x0, SDWORD y0,
        UDWORD width, UDWORD height,
        UDWORD number, UDWORD selected, UDWORD hilite,
        PIELIGHT *pColours, UDWORD TabGap)
{
	SDWORD	x1, y, y1;
	UDWORD	i;

#if NO_DISPLAY_SINGLE_TABS
	if (number == 1)
	{
		/* Don't display single tabs */
		return;
	}
#endif

	x1 = x0 + width;
	y = y0 + 2;
	y1 = y + height - 2;
	for (i = 0; i < number; i++)
	{
		if (psForm->pTabDisplay)
		{
			psForm->pTabDisplay(psForm, WFORM_TABLEFT, i, i == selected, i == hilite, x0, y, width, height);
		}
		else
		{
			if (i == selected)
			{
				/* Fill in the tab */
				pie_BoxFill(x0 + 1, y + 1, x1, y1 - 1, pColours[WCOL_BKGRND]);
				/* Draw the outline */
				iV_Line(x0, y, x1 - 1, y, pColours[WCOL_LIGHT]);
				iV_Line(x0, y + 1, x0, y1 - 2, pColours[WCOL_LIGHT]);
				iV_Line(x0 + 1, y1 - 1, x0 + 2, y1, pColours[WCOL_DARK]);
				iV_Line(x0 + 3, y1, x1, y1, pColours[WCOL_DARK]);
			}
			else
			{
				/* Fill in the tab */
				pie_BoxFill(x0 + 2, y + 1, x1 - 1, y1 - 1, pColours[WCOL_BKGRND]);
				/* Draw the outline */
				iV_Line(x0 + 1, y, x1 - 1, y, pColours[WCOL_LIGHT]);
				iV_Line(x0 + 1, y + 1, x0 + 1, y1 - 2, pColours[WCOL_LIGHT]);
				iV_Line(x0 + 2, y1 - 1, x0 + 3, y1, pColours[WCOL_DARK]);
				iV_Line(x0 + 4, y1, x1 - 1, y1, pColours[WCOL_DARK]);
			}
			if (i == hilite)
			{
				iV_Box(x0 + 4, y + 2, x1 - 2, y1 - 3, pColours[WCOL_HILITE]);
			}
		}
		y += height + TabGap;
		y1 += height + TabGap;
	}
}


/* Draw right tabs */
static void formDisplayRTabs(W_TABFORM *psForm, SDWORD x0, SDWORD y0,
        UDWORD width, UDWORD height,
        UDWORD number, UDWORD selected, UDWORD hilite,
        PIELIGHT *pColours, UDWORD TabGap)
{
	SDWORD	x1, y, y1;
	UDWORD	i;

#if NO_DISPLAY_SINGLE_TABS
	if (number == 1)
	{
		/* Don't display single tabs */
		return;
	}
#endif

	x1 = x0 + width;
	y = y0 + 2;
	y1 = y + height - 2;
	for (i = 0; i < number; i++)
	{
		if (psForm->pTabDisplay)
		{
			psForm->pTabDisplay(psForm, WFORM_TABRIGHT, i, i == selected, i == hilite, x0, y, width, height);
		}
		else
		{
			if (i == selected)
			{
				/* Fill in the tab */
				pie_BoxFill(x0, y + 1, x1 - 1, y1 - 1, pColours[WCOL_BKGRND]);
				/* Draw the outline */
				iV_Line(x0, y, x1 - 1, y, pColours[WCOL_LIGHT]);
				iV_Line(x1, y, x1, y1 - 2, pColours[WCOL_DARK]);
				iV_Line(x1 - 1, y1 - 1, x1 - 2, y1, pColours[WCOL_DARK]);
				iV_Line(x1 - 3, y1, x0, y1, pColours[WCOL_DARK]);
			}
			else
			{
				/* Fill in the tab */
				pie_BoxFill(x0 + 1, y + 1, x1 - 2, y1 - 1, pColours[WCOL_BKGRND]);
				/* Draw the outline */
				iV_Line(x0 + 1, y, x1 - 1, y, pColours[WCOL_LIGHT]);
				iV_Line(x1 - 1, y, x1 - 1, y1 - 2, pColours[WCOL_DARK]);
				iV_Line(x1 - 2, y1 - 1, x1 - 3, y1, pColours[WCOL_DARK]);
				iV_Line(x1 - 4, y1, x0 + 1, y1, pColours[WCOL_DARK]);
			}
			if (i == hilite)
			{
				iV_Box(x0 + 2, y + 2, x1 - 4, y1 - 3, pColours[WCOL_HILITE]);
			}
		}
		y += height + TabGap;
		y1 += height + TabGap;
	}
}


/* Display a tabbed form */
void formDisplayTabbed(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD		x0, y0, x1, y1;
	W_TABFORM	*psForm;

	psForm = (W_TABFORM *)psWidget;

	/* Get the basic position of the form */
	x0 = psForm->x + xOffset;
	y0 = psForm->y + yOffset;
	x1 = x0 + psForm->width;
	y1 = y0 + psForm->height;

	/* Adjust for where the tabs are */
	if (psForm->majorPos == WFORM_TABLEFT)
	{
		x0 += psForm->tabMajorThickness - psForm->tabHorzOffset;
	}
	if (psForm->majorPos == WFORM_TABRIGHT)
	{
		x1 -= psForm->tabMajorThickness - psForm->tabHorzOffset;
	}
	if (psForm->majorPos == WFORM_TABTOP)
	{
		y0 += psForm->tabMajorThickness - psForm->tabVertOffset;
	}
	if (psForm->majorPos == WFORM_TABBOTTOM)
	{
		y1 -= psForm->tabMajorThickness - psForm->tabVertOffset;
	}

	/* Draw the major tabs */
	switch (psForm->majorPos)
	{
	case WFORM_TABTOP:
		formDisplayTTabs(psForm, x0 + psForm->majorOffset, y0 - psForm->tabMajorThickness + psForm->tabVertOffset,
		        psForm->majorSize, psForm->tabMajorThickness,
		        psForm->numMajor, psForm->majorT, psForm->tabHiLite,
		        pColours, psForm->tabMajorGap);
		break;
	case WFORM_TABBOTTOM:
		formDisplayBTabs(psForm, x0 + psForm->majorOffset, y1 + psForm->tabVertOffset,
		        psForm->majorSize, psForm->tabMajorThickness,
		        psForm->numMajor, psForm->majorT, psForm->tabHiLite,
		        pColours, psForm->tabMajorGap);
		break;
	case WFORM_TABLEFT:
		formDisplayLTabs(psForm, x0 - psForm->tabMajorThickness + psForm->tabHorzOffset, y0 + psForm->majorOffset,
		        psForm->tabMajorThickness, psForm->majorSize,
		        psForm->numMajor, psForm->majorT, psForm->tabHiLite,
		        pColours, psForm->tabMajorGap);
		break;
	case WFORM_TABRIGHT:
		formDisplayRTabs(psForm, x1 - psForm->tabHorzOffset, y0 + psForm->majorOffset,
		        psForm->tabMajorThickness, psForm->majorSize,
		        psForm->numMajor, psForm->majorT, psForm->tabHiLite,
		        pColours, psForm->tabMajorGap);
		break;
	case WFORM_TABNONE:
		ASSERT(false, "formDisplayTabbed: Cannot have a tabbed form with no major tabs");
		break;
	}
}

void W_CLICKFORM::setTip(QString string)
{
	pTip = string;
}
