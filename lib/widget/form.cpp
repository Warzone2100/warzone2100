/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

static inline void formFreeTips(W_TABFORM *psForm);

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
	, majorPos(0), minorPos(0)
	, majorSize(0), minorSize(0)
	, majorOffset(0), minorOffset(0)
	, tabVertOffset(0)
	, tabHorzOffset(0)
	, tabMajorThickness(0)
	, tabMinorThickness(0)
	, tabMajorGap(0)
	, tabMinorGap(0)
	, numStats(0)
	, numButtons(0)
	, numMajor(0)
	// aNumMinors
	, TabMultiplier(0)
	, maxTabsShown(MAX_TAB_SMALL_SHOWN - 1)  // Equal to TAB_SEVEN, which is equal to 7.
	, pTip(NULL)
	// apMajorTips
	// apMinorTips
	, pTabDisplay(NULL)
	, pFormDisplay(NULL)
{
	memset(aNumMinors, 0, sizeof(aNumMinors));
	memset(apMajorTips, 0, sizeof(apMajorTips));
	memset(apMinorTips, 0, sizeof(apMinorTips));
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
}


/* Create a plain form widget */
static W_FORM *formCreatePlain(const W_FORMINIT *psInit)
{
	/* Allocate the required memory */
	W_FORM *psWidget = new W_FORM(psInit);
	if (psWidget == NULL)
	{
		debug(LOG_FATAL, "formCreatePlain: Out of memory");
		abort();
		return NULL;
	}

	return psWidget;
}


/* Free a plain form widget */
static void formFreePlain(W_FORM *psWidget)
{
	ASSERT(psWidget != NULL, "Invalid form pointer");

	widgReleaseWidgetList(psWidget->psWidgets);
	CheckpsMouseOverWidget(psWidget);			// clear global if needed
	delete psWidget;
}


W_CLICKFORM::W_CLICKFORM(W_FORMINIT const *init)
	: W_FORM(init)
	, state(WCLICK_NORMAL)
	, pTip(init->pTip)
	, HilightAudioID(WidgGetHilightAudioID())
	, ClickedAudioID(WidgGetClickedAudioID())
	, AudioCallback(WidgGetAudioCallback())
{
	if (init->pDisplay == NULL)
	{
		display = formDisplayClickable;
	}
}

/* Create a plain form widget */
static W_CLICKFORM *formCreateClickable(const W_FORMINIT *psInit)
{
	/* Allocate the required memory */
	W_CLICKFORM *psWidget = new W_CLICKFORM(psInit);
	if (psWidget == NULL)
	{
		debug(LOG_FATAL, "formCreateClickable: Out of memory");
		abort();
		return NULL;
	}

	return psWidget;
}


/* Free a plain form widget */
static void formFreeClickable(W_CLICKFORM *psWidget)
{
	ASSERT(psWidget != NULL,
	       "formFreePlain: Invalid form pointer");

	widgReleaseWidgetList(psWidget->psWidgets);

	delete psWidget;
}


W_TABFORM::W_TABFORM(W_FORMINIT const *init)
	: W_FORM(init)
	, majorPos(init->majorPos), minorPos(init->minorPos)
	, majorSize(init->majorSize), minorSize(init->minorSize)
	, tabMajorThickness(init->tabMajorThickness)
	, tabMinorThickness(init->tabMinorThickness)
	, tabMajorGap(init->tabMajorGap)
	, tabMinorGap(init->tabMinorGap)
	, tabVertOffset(init->tabVertOffset)
	, tabHorzOffset(init->tabHorzOffset)
	, majorOffset(init->majorOffset)
	, minorOffset(init->minorOffset)
	, majorT(0), minorT(0)
	, state(0)  // This assignment was previously done by a memset.
	, tabHiLite(~0)
	, TabMultiplier(init->TabMultiplier)
	, maxTabsShown(init->maxTabsShown)
	, numStats(init->numStats)
	, numButtons(init->numButtons)
	, pTabDisplay(init->pTabDisplay)
{
	memset(asMajor, 0, sizeof(asMajor));

	/* Allocate the memory for tool tips and copy them in */
	/* Set up the tab data.
	 * All widget pointers have been zeroed by the memset above.
	 */
	numMajor = init->numMajor;
	for (unsigned major = 0; major < init->numMajor; ++major)
	{
		/* Check for a tip for the major tab */
		asMajor[major].pTip = init->apMajorTips[major];
		asMajor[major].lastMinor = 0;

		/* Check for tips for the minor tab */
		asMajor[major].numMinor = init->aNumMinors[major];
		for (unsigned minor = 0; minor < init->aNumMinors[major]; ++minor)
		{
			asMajor[major].asMinor[minor].pTip = init->apMinorTips[major][minor];
		}
	}

	if (init->pDisplay == NULL)
	{
		display = formDisplayTabbed;
	}
}

/* Create a tabbed form widget */
static W_TABFORM *formCreateTabbed(const W_FORMINIT *psInit)
{
	if (psInit->numMajor == 0)
	{
		ASSERT(false, "formCreateTabbed: Must have at least one major tab on a tabbed form");
		return NULL;
	}
	if (psInit->majorPos != 0
	    && psInit->majorPos == psInit->minorPos)
	{
		ASSERT(false, "formCreateTabbed: Cannot have major and minor tabs on same side");
		return NULL;
	}
	if (psInit->numMajor >= WFORM_MAXMAJOR)
	{
		ASSERT(false, "formCreateTabbed: Too many Major tabs");
		return NULL;
	}
	for (unsigned major = 0; major < psInit->numMajor; ++major)
	{
		if (psInit->aNumMinors[major] >= WFORM_MAXMINOR)
		{
			ASSERT(false, "formCreateTabbed: Too many Minor tabs for Major %u", major);
			return NULL;
		}
		if (psInit->aNumMinors[major] == 0)
		{
			ASSERT(false, "formCreateTabbed: Must have at least one Minor tab for each major");
			return NULL;
		}
	}

	/* Allocate the required memory */
	W_TABFORM *psWidget = new W_TABFORM(psInit);
	if (psWidget == NULL)
	{
		debug(LOG_FATAL, "formCreateTabbed: Out of memory");
		abort();
		return NULL;
	}

	return psWidget;
}

/* Free the tips strings for a tabbed form */
static inline void formFreeTips(W_TABFORM *)
{
}

/* Free a tabbed form widget */
static void formFreeTabbed(W_TABFORM *psWidget)
{
	WIDGET			*psCurr;
	W_FORMGETALL	sGetAll;

	ASSERT(psWidget != NULL,
	       "formFreeTabbed: Invalid form pointer");

	formFreeTips(psWidget);

	formInitGetAllWidgets((W_FORM *)psWidget, &sGetAll);
	psCurr = formGetAllWidgets(&sGetAll);
	while (psCurr)
	{
		widgReleaseWidgetList(psCurr);
		psCurr = formGetAllWidgets(&sGetAll);
	}
	delete psWidget;
}


/* Create a form widget data structure */
W_FORM *formCreate(const W_FORMINIT *psInit)
{
	/* Check the style bits are OK */
	if (psInit->style & ~(WFORM_TABBED | WFORM_INVISIBLE | WFORM_CLICKABLE
	        | WFORM_NOCLICKMOVE | WFORM_NOPRIMARY | WFORM_SECONDARY
	        | WIDG_HIDDEN))
	{
		ASSERT(false, "formCreate: Unknown style bit");
		return NULL;
	}
	if ((psInit->style & WFORM_TABBED)
	    && (psInit->style & (WFORM_INVISIBLE | WFORM_CLICKABLE)))
	{
		ASSERT(false, "formCreate: Tabbed form cannot be invisible or clickable");
		return NULL;
	}
	if ((psInit->style & WFORM_INVISIBLE)
	    && (psInit->style & WFORM_CLICKABLE))
	{
		ASSERT(false, "formCreate: Cannot have an invisible clickable form");
		return NULL;
	}
	if (!(psInit->style & WFORM_CLICKABLE)
	    && ((psInit->style & WFORM_NOPRIMARY)
	        || (psInit->style & WFORM_SECONDARY)))
	{
		ASSERT(false, "formCreate: Cannot set keys if the form isn't clickable");
		return NULL;
	}

	/* Create the correct type of form */
	if (psInit->style & WFORM_TABBED)
	{
		return (W_FORM *)formCreateTabbed(psInit);
	}
	else if (psInit->style & WFORM_CLICKABLE)
	{
		return (W_FORM *)formCreateClickable(psInit);
	}
	else
	{
		return formCreatePlain(psInit);
	}

	return NULL;
}


/* Free the memory used by a form */
void formFree(W_FORM *psWidget)
{
	if (psWidget->style & WFORM_TABBED)
	{
		formFreeTabbed((W_TABFORM *)psWidget);
	}
	else if (psWidget->style & WFORM_CLICKABLE)
	{
		formFreeClickable((W_CLICKFORM *)psWidget);
	}
	else
	{
		formFreePlain(psWidget);
	}
}


/* Add a widget to a form */
bool formAddWidget(W_FORM *psForm, WIDGET *psWidget, W_INIT *psInit)
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
		if (psInit->minorID >= psMajor->numMinor)
		{
			ASSERT(false, "formAddWidget: Minor tab does not exist");
			return false;
		}
		ppsList = &(psMajor->asMinor[psInit->minorID].psWidgets);
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


/* Get the button state of a click form */
UDWORD formGetClickState(W_CLICKFORM *psForm)
{
	UDWORD State = 0;

	if (psForm->state & WCLICK_GREY)
	{
		State |= WBUT_DISABLE;
	}

	if (psForm->state & WCLICK_LOCKED)
	{
		State |= WBUT_LOCK;
	}

	if (psForm->state & WCLICK_CLICKLOCK)
	{
		State |= WBUT_CLICKLOCK;
	}

	return State;
}


/* Set the button state of a click form */
void formSetClickState(W_CLICKFORM *psForm, UDWORD state)
{
	ASSERT(!((state & WBUT_LOCK) && (state & WBUT_CLICKLOCK)),
	       "widgSetButtonState: Cannot have WBUT_LOCK and WBUT_CLICKLOCK");

	if (state & WBUT_DISABLE)
	{
		psForm->state |= WCLICK_GREY;
	}
	else
	{
		psForm->state &= ~WCLICK_GREY;
	}
	if (state & WBUT_LOCK)
	{
		psForm->state |= WCLICK_LOCKED;
	}
	else
	{
		psForm->state &= ~WCLICK_LOCKED;
	}
	if (state & WBUT_CLICKLOCK)
	{
		psForm->state |= WCLICK_CLICKLOCK;
	}
	else
	{
		psForm->state &= ~WCLICK_CLICKLOCK;
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
		return psMajor->asMinor[psTabForm->minorT].psWidgets;
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
		psCtrl->GAWMinor = 0;
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
			psRetList = psCtrl->psGAWMajor->asMinor[psCtrl->GAWMinor++].psWidgets;
			if (psCtrl->GAWMinor >= psCtrl->psGAWMajor->numMinor)
			{
				psCtrl->GAWMinor = 0;
				psCtrl->GAWMajor ++;
				psCtrl->psGAWMajor ++;
			}
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
void widgSetTabs(W_SCREEN *psScreen, UDWORD id, UWORD major, UWORD minor)
{
	W_TABFORM *const psForm = widgGetTabbedFormById(psScreen, id);

	ASSERT(psForm != NULL, "widgSetTabs: Invalid tab form pointer");

	if (psForm == NULL)
	{
		return; /* make us work fine in no assert compiles */
	}

	ASSERT(major < widgGetNumTabMajor(psScreen, id), "widgSetTabs id=%u: invalid major id %u >= max %u", id,
	       major, widgGetNumTabMajor(psScreen, id));

	ASSERT(minor < widgGetNumTabMinor(psScreen, id, major), "widgSetTabs id=%u: invalid minor id %u >= max %u", id,
	       minor, widgGetNumTabMinor(psScreen, id, major));

	// Make sure to bail out when we've been passed out-of-bounds major or minor numbers
	if (major >= widgGetNumTabMajor(psScreen, id)
	    || minor >= widgGetNumTabMinor(psScreen, id, major))
	{
		return;
	}

	psForm->majorT = major;
	psForm->minorT = minor;
	psForm->asMajor[major].lastMinor = minor;
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


int widgGetNumTabMinor(W_SCREEN *psScreen, UDWORD id, UWORD pMajor)
{
	W_TABFORM *const psForm = widgGetTabbedFormById(psScreen, id);

	ASSERT(psForm != NULL, "Couldn't find a tabbed form with ID %u", id);
	if (psForm == NULL)
	{
		return 0;
	}

	return psForm->asMajor[pMajor].numMinor;
}


/* Get the current tabs for a tab form */
void widgGetTabs(W_SCREEN *psScreen, UDWORD id, UWORD *pMajor, UWORD *pMinor)
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
	ASSERT(psForm->minorT < psForm->asMajor[psForm->majorT].numMinor, "widgGetTabs: invalid minor id %u >= max %u",
	       psForm->minorT, psForm->asMajor[psForm->majorT].numMinor);

	*pMajor = psForm->majorT;
	*pMinor = psForm->minorT;
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
		else if (psTabForm->minorPos == WFORM_TABTOP)
		{
			*pYOrigin = psTabForm->tabMinorThickness;
		}
		else
		{
			*pYOrigin = 0;
		}
		if (psTabForm->majorPos == WFORM_TABLEFT)
		{
			*pXOrigin = psTabForm->tabMajorThickness;
		}
		else if (psTabForm->minorPos == WFORM_TABLEFT)
		{
			*pXOrigin = psTabForm->tabMinorThickness;
		}
		else
		{
			*pXOrigin = 0;
		}
//		if ((psTabForm->majorPos == WFORM_TABTOP) ||
//			(psTabForm->minorPos == WFORM_TABTOP))
//		{
//			*pYOrigin = psTabForm->tabThickness;
//		}
//		else
//		{
//			*pYOrigin = 0;
//		}
//		if ((psTabForm->majorPos == WFORM_TABLEFT) ||
//			(psTabForm->minorPos == WFORM_TABLEFT))
//		{
//			*pXOrigin = psTabForm->tabThickness;
//		}
//		else
//		{
//			*pXOrigin = 0;
//		}
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
	W_MAJORTAB	*psMajor;
	SDWORD	xOffset, yOffset;
	SDWORD	xOffset2, yOffset2;

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
	else if (psForm->minorPos == WFORM_TABLEFT)
	{
		x0 += psForm->tabMinorThickness;
	}
	if (psForm->majorPos == WFORM_TABRIGHT)
	{
		x1 -= psForm->tabMajorThickness;
	}
	else if (psForm->minorPos == WFORM_TABRIGHT)
	{
		x1 -= psForm->tabMinorThickness;
	}
	if (psForm->majorPos == WFORM_TABTOP)
	{
		y0 += psForm->tabMajorThickness;
	}
	else if (psForm->minorPos == WFORM_TABTOP)
	{
		y0 += psForm->tabMinorThickness;
	}
	if (psForm->majorPos == WFORM_TABBOTTOM)
	{
		y1 -= psForm->tabMajorThickness;
	}
	else if (psForm->minorPos == WFORM_TABBOTTOM)
	{
		y1 -= psForm->tabMinorThickness;
	}

//		/* Adjust for where the tabs are */
//	if (psForm->majorPos == WFORM_TABLEFT || psForm->minorPos == WFORM_TABLEFT)
//	{
//		x0 += psForm->tabThickness;
//	}
//	if (psForm->majorPos == WFORM_TABRIGHT || psForm->minorPos == WFORM_TABRIGHT)
//	{
//		x1 -= psForm->tabThickness;
//	}
//	if (psForm->majorPos == WFORM_TABTOP || psForm->minorPos == WFORM_TABTOP)
//	{
//		y0 += psForm->tabThickness;
//	}
//	if (psForm->majorPos == WFORM_TABBOTTOM || psForm->minorPos == WFORM_TABBOTTOM)
//	{
//		y1 -= psForm->tabThickness;
//	}


	xOffset = yOffset = 0;
	switch (psForm->minorPos)
	{
	case WFORM_TABTOP:
		yOffset = psForm->tabVertOffset;
		break;

	case WFORM_TABLEFT:
		xOffset = psForm->tabHorzOffset;
		break;

	case WFORM_TABBOTTOM:
		yOffset = psForm->tabVertOffset;
		break;

	case WFORM_TABRIGHT:
		xOffset = psForm->tabHorzOffset;
		break;
	}

	xOffset2 = yOffset2 = 0;
	psTabPos->TabMultiplier = psForm->TabMultiplier;
	/* Check the major tabs */
	switch (psForm->majorPos)
	{
	case WFORM_TABTOP:
		if (formPickHTab(psTabPos, x0 + psForm->majorOffset - xOffset, y0 - psForm->tabMajorThickness,
		        psForm->majorSize, psForm->tabMajorThickness, psForm->tabMajorGap,
		        psForm->numMajor, fx, fy, psForm->maxTabsShown))
		{
			return true;
		}
		yOffset2 = -psForm->tabVertOffset;
		break;
	case WFORM_TABBOTTOM:
		if (formPickHTab(psTabPos, x0 + psForm->majorOffset - xOffset, y1,
		        psForm->majorSize, psForm->tabMajorThickness, psForm->tabMajorGap,
		        psForm->numMajor, fx, fy, psForm->maxTabsShown))
		{
			return true;
		}
		break;
	case WFORM_TABLEFT:
		if (formPickVTab(psTabPos, x0 - psForm->tabMajorThickness, y0 + psForm->majorOffset - yOffset,
		        psForm->tabMajorThickness, psForm->majorSize, psForm->tabMajorGap,
		        psForm->numMajor, fx, fy))
		{
			return true;
		}
		xOffset2 = psForm->tabHorzOffset;
		break;
	case WFORM_TABRIGHT:
		if (formPickVTab(psTabPos, x1, y0 + psForm->majorOffset - yOffset,
		        psForm->tabMajorThickness, psForm->majorSize, psForm->tabMajorGap,
		        psForm->numMajor, fx, fy))
		{
			return true;
		}
		break;
	case WFORM_TABNONE:
		ASSERT(false, "formDisplayTabbed: Cannot have a tabbed form with no major tabs");
		break;
	}

	/* Draw the minor tabs */
	psMajor = psForm->asMajor + psForm->majorT;
	switch (psForm->minorPos)
	{
	case WFORM_TABTOP:
		if (formPickHTab(psTabPos, x0 + psForm->minorOffset - xOffset2, y0 - psForm->tabMinorThickness,
		        psForm->minorSize, psForm->tabMinorThickness, psForm->tabMinorGap,
		        psMajor->numMinor, fx, fy, psForm->maxTabsShown))
		{
			psTabPos->index += psForm->numMajor;
			return true;
		}
		break;
	case WFORM_TABBOTTOM:
		if (formPickHTab(psTabPos, x0 + psForm->minorOffset - xOffset2, y1,
		        psForm->minorSize, psForm->tabMinorThickness, psForm->tabMinorGap,
		        psMajor->numMinor, fx, fy, psForm->maxTabsShown))
		{
			psTabPos->index += psForm->numMajor;
			return true;
		}
		break;
	case WFORM_TABLEFT:
		if (formPickVTab(psTabPos, x0 + xOffset - psForm->tabMinorThickness, y0 + psForm->minorOffset - yOffset2,
		        psForm->tabMinorThickness, psForm->minorSize, psForm->tabMinorGap,
		        psMajor->numMinor, fx, fy))
		{
			psTabPos->index += psForm->numMajor;
			return true;
		}
		break;
	case WFORM_TABRIGHT:
		if (formPickVTab(psTabPos, x1 + xOffset, y0 + psForm->minorOffset - yOffset2,
		        psForm->tabMinorThickness, psForm->minorSize, psForm->tabMinorGap,
		        psMajor->numMinor, fx, fy))
		{
			psTabPos->index += psForm->numMajor;
			return true;
		}
		break;
		/* case WFORM_TABNONE - no minor tabs so nothing to display */
	}

	return false;
}

extern UDWORD realTime;  // FIXME Include a header...

/* Run a form widget */
void formRun(W_FORM *psWidget, W_CONTEXT *psContext)
{
	SDWORD		mx, my;
	TAB_POS		sTabPos;
	char		*pTip;
	W_TABFORM	*psTabForm;

	memset(&sTabPos, 0x0, sizeof(TAB_POS));
	if (psWidget->style & WFORM_CLICKABLE)
	{
		if (((W_CLICKFORM *)psWidget)->state & WCLICK_FLASH)
		{
			if (((realTime / 250) % 2) == 0)
			{
				((W_CLICKFORM *)psWidget)->state &= ~WCLICK_FLASHON;
			}
			else
			{
				((W_CLICKFORM *)psWidget)->state |= WCLICK_FLASHON;
			}
		}
	}

	if (psWidget->style & WFORM_TABBED)
	{
		mx = psContext->mx;
		my = psContext->my;
		psTabForm = (W_TABFORM *)psWidget;

		/* If the mouse is over the form, see if any tabs need to be hilited */
		if (mx >= 0 && mx <= psTabForm->width &&
		    my >= 0 && my <= psTabForm->height)
		{
			if (formPickTab(psTabForm, mx, my, &sTabPos))
			{
				if (psTabForm->tabHiLite != (UWORD)sTabPos.index)
				{
					/* Got a new tab - start the tool tip if there is one */
					psTabForm->tabHiLite = (UWORD)sTabPos.index;
					if (sTabPos.index >= psTabForm->numMajor)
					{
						pTip = psTabForm->asMajor[psTabForm->majorT].
						       asMinor[sTabPos.index - psTabForm->numMajor].pTip;
					}
					else
					{
						pTip = psTabForm->asMajor[sTabPos.index].pTip;
					}
					if (pTip)
					{
						/* Got a tip - start it off */
						tipStart((WIDGET *)psTabForm, pTip,
						        psContext->psScreen->TipFontID, psTabForm->aColours,
						        sTabPos.x + psContext->xOffset,
						        sTabPos.y + psContext->yOffset,
						        sTabPos.width, sTabPos.height);
					}
					else
					{
						/* No tip - clear any old tip */
						tipStop((WIDGET *)psWidget);
					}
				}
			}
			else
			{
				/* No tab - clear the tool tip */
				tipStop((WIDGET *)psWidget);
				/* And clear the hilite */
				psTabForm->tabHiLite = (UWORD)(-1);
			}
		}
	}
}


void formSetFlash(W_FORM *psWidget)
{
	if (psWidget->style & WFORM_CLICKABLE)
	{
		((W_CLICKFORM *)psWidget)->state |= WCLICK_FLASH;
	}
}


void formClearFlash(W_FORM *psWidget)
{
	if (psWidget->style & WFORM_CLICKABLE)
	{
		((W_CLICKFORM *)psWidget)->state &= ~WCLICK_FLASH;
		((W_CLICKFORM *)psWidget)->state &= ~WCLICK_FLASHON;
	}
}


/* Respond to a mouse click */
void formClicked(W_FORM *psWidget, UDWORD key)
{
	W_CLICKFORM		*psClickForm;

	/* Stop the tip if there is one */
	tipStop((WIDGET *)psWidget);

	if (psWidget->style & WFORM_CLICKABLE)
	{
		/* Can't click a button if it is disabled or locked down */
		if (!(((W_CLICKFORM *)psWidget)->state & (WCLICK_GREY | WCLICK_LOCKED)))
		{
			// Check this is the correct key
			if ((!(psWidget->style & WFORM_NOPRIMARY) && key == WKEY_PRIMARY) ||
			    ((psWidget->style & WFORM_SECONDARY) && key == WKEY_SECONDARY))
			{
				((W_CLICKFORM *)psWidget)->state &= ~WCLICK_FLASH;	// Stop it flashing
				((W_CLICKFORM *)psWidget)->state &= ~WCLICK_FLASHON;
				((W_CLICKFORM *)psWidget)->state |= WCLICK_DOWN;

				psClickForm = (W_CLICKFORM *)psWidget;

				if (psClickForm->AudioCallback)
				{
					psClickForm->AudioCallback(psClickForm->ClickedAudioID);
				}
			}
		}
	}
}


/* Respond to a mouse form up */
void formReleased(W_FORM *psWidget, UDWORD key, W_CONTEXT *psContext)
{
	W_TABFORM	*psTabForm;
	W_CLICKFORM	*psClickForm;
	TAB_POS		sTabPos;

	if (psWidget->style & WFORM_TABBED)
	{
		psTabForm = (W_TABFORM *)psWidget;
		/* See if a tab has been clicked on */
		if (formPickTab(psTabForm, psContext->mx, psContext->my, &sTabPos))
		{
			if (sTabPos.index >= psTabForm->numMajor)
			{
				/* Clicked on a minor tab */
				psTabForm->minorT = (UWORD)(sTabPos.index - psTabForm->numMajor);
				psTabForm->asMajor[psTabForm->majorT].lastMinor = psTabForm->minorT;
				widgSetReturn(psContext->psScreen, (WIDGET *)psWidget);
			}
			else
			{
				/* Clicked on a major tab */
				ASSERT(psTabForm->majorT < psTabForm->numMajor,
				       "formReleased: invalid major id %u >= max %u", sTabPos.index, psTabForm->numMajor);
				psTabForm->majorT = (UWORD)sTabPos.index;
				psTabForm->minorT = psTabForm->asMajor[sTabPos.index].lastMinor;
				widgSetReturn(psContext->psScreen, (WIDGET *)psWidget);
			}
		}
	}
	else if (psWidget->style & WFORM_CLICKABLE)
	{
		psClickForm = (W_CLICKFORM *)psWidget;
		if (psClickForm->state & WCLICK_DOWN)
		{
			// Check this is the correct key
			if ((!(psWidget->style & WFORM_NOPRIMARY) && key == WKEY_PRIMARY) ||
			    ((psWidget->style & WFORM_SECONDARY) && key == WKEY_SECONDARY))
			{
				widgSetReturn(psContext->psScreen, (WIDGET *)psClickForm);
				psClickForm->state &= ~WCLICK_DOWN;
			}
		}
	}
}


/* Respond to a mouse moving over a form */
void formHiLite(W_FORM *psWidget, W_CONTEXT *psContext)
{
	W_CLICKFORM		*psClickForm;

	if (psWidget->style & WFORM_CLICKABLE)
	{
		psClickForm = (W_CLICKFORM *)psWidget;

		psClickForm->state |= WCLICK_HILITE;

		/* If there is a tip string start the tool tip */
		if (psClickForm->pTip)
		{
			tipStart((WIDGET *)psClickForm, psClickForm->pTip,
			        psContext->psScreen->TipFontID, psContext->psForm->aColours,
			        psWidget->x + psContext->xOffset, psWidget->y + psContext->yOffset,
			        psWidget->width, psWidget->height);
		}

		if (psClickForm->AudioCallback)
		{
			psClickForm->AudioCallback(psClickForm->HilightAudioID);
		}
	}
}


/* Respond to the mouse moving off a form */
void formHiLiteLost(W_FORM *psWidget, W_CONTEXT *psContext)
{
	/* If one of the widgets were hilited that has to loose it as well */
	if (psWidget->psLastHiLite != NULL)
	{
		widgHiLiteLost(psWidget->psLastHiLite, psContext);
	}
	if (psWidget->style & WFORM_TABBED)
	{
		((W_TABFORM *)psWidget)->tabHiLite = (UWORD)(-1);
	}
	if (psWidget->style & WFORM_CLICKABLE)
	{
		((W_CLICKFORM *)psWidget)->state &= ~(WCLICK_DOWN | WCLICK_HILITE);
	}
	/* Clear the tool tip if there is one */
	tipStop((WIDGET *)psWidget);
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
        PIELIGHT *pColours, UDWORD TabType, UDWORD TabGap)
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
			psForm->pTabDisplay((WIDGET *)psForm, TabType, WFORM_TABTOP, i, i == selected, i == hilite, x, y0, width, height);
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
        PIELIGHT *pColours, UDWORD TabType, UDWORD TabGap)
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
			psForm->pTabDisplay((WIDGET *)psForm, TabType, WFORM_TABBOTTOM, i, i == selected, i == hilite, x, y0, width, height);
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
        PIELIGHT *pColours, UDWORD TabType, UDWORD TabGap)
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
			psForm->pTabDisplay((WIDGET *)psForm, TabType, WFORM_TABLEFT, i, i == selected, i == hilite, x0, y, width, height);
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
        PIELIGHT *pColours, UDWORD TabType, UDWORD TabGap)
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
			psForm->pTabDisplay((WIDGET *)psForm, TabType, WFORM_TABRIGHT, i, i == selected, i == hilite, x0, y, width, height);
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
	W_MAJORTAB	*psMajor;

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
	else if (psForm->minorPos == WFORM_TABLEFT)
	{
		x0 += psForm->tabMinorThickness - psForm->tabHorzOffset;
	}
	if (psForm->majorPos == WFORM_TABRIGHT)
	{
		x1 -= psForm->tabMajorThickness - psForm->tabHorzOffset;
	}
	else if (psForm->minorPos == WFORM_TABRIGHT)
	{
		x1 -= psForm->tabMinorThickness - psForm->tabHorzOffset;
	}
	if (psForm->majorPos == WFORM_TABTOP)
	{
		y0 += psForm->tabMajorThickness - psForm->tabVertOffset;
	}
	else if (psForm->minorPos == WFORM_TABTOP)
	{
		y0 += psForm->tabMinorThickness - psForm->tabVertOffset;
	}
	if (psForm->majorPos == WFORM_TABBOTTOM)
	{
		y1 -= psForm->tabMajorThickness - psForm->tabVertOffset;
	}
	else if (psForm->minorPos == WFORM_TABBOTTOM)
	{
		y1 -= psForm->tabMinorThickness - psForm->tabVertOffset;
	}

	/* Draw the major tabs */
	switch (psForm->majorPos)
	{
	case WFORM_TABTOP:
		formDisplayTTabs(psForm, x0 + psForm->majorOffset, y0 - psForm->tabMajorThickness + psForm->tabVertOffset,
		        psForm->majorSize, psForm->tabMajorThickness,
		        psForm->numMajor, psForm->majorT, psForm->tabHiLite,
		        pColours, TAB_MAJOR, psForm->tabMajorGap);
		break;
	case WFORM_TABBOTTOM:
		formDisplayBTabs(psForm, x0 + psForm->majorOffset, y1 + psForm->tabVertOffset,
		        psForm->majorSize, psForm->tabMajorThickness,
		        psForm->numMajor, psForm->majorT, psForm->tabHiLite,
		        pColours, TAB_MAJOR, psForm->tabMajorGap);
		break;
	case WFORM_TABLEFT:
		formDisplayLTabs(psForm, x0 - psForm->tabMajorThickness + psForm->tabHorzOffset, y0 + psForm->majorOffset,
		        psForm->tabMajorThickness, psForm->majorSize,
		        psForm->numMajor, psForm->majorT, psForm->tabHiLite,
		        pColours, TAB_MAJOR, psForm->tabMajorGap);
		break;
	case WFORM_TABRIGHT:
		formDisplayRTabs(psForm, x1 - psForm->tabHorzOffset, y0 + psForm->majorOffset,
		        psForm->tabMajorThickness, psForm->majorSize,
		        psForm->numMajor, psForm->majorT, psForm->tabHiLite,
		        pColours, TAB_MAJOR, psForm->tabMajorGap);
		break;
	case WFORM_TABNONE:
		ASSERT(false, "formDisplayTabbed: Cannot have a tabbed form with no major tabs");
		break;
	}

	/* Draw the minor tabs */
	psMajor = psForm->asMajor + psForm->majorT;
	switch (psForm->minorPos)
	{
	case WFORM_TABTOP:
		formDisplayTTabs(psForm, x0 + psForm->minorOffset, y0 - psForm->tabMinorThickness + psForm->tabVertOffset,
		        psForm->minorSize, psForm->tabMinorThickness,
		        psMajor->numMinor, psForm->minorT,
		        psForm->tabHiLite - psForm->numMajor, pColours, TAB_MINOR, psForm->tabMinorGap);
		break;
	case WFORM_TABBOTTOM:
		formDisplayBTabs(psForm, x0 + psForm->minorOffset, y1 + psForm->tabVertOffset,
		        psForm->minorSize, psForm->tabMinorThickness,
		        psMajor->numMinor, psForm->minorT,
		        psForm->tabHiLite - psForm->numMajor, pColours, TAB_MINOR, psForm->tabMinorGap);
		break;
	case WFORM_TABLEFT:
		formDisplayLTabs(psForm, x0 - psForm->tabMinorThickness + psForm->tabHorzOffset + psForm->minorOffset,
		        y0 + psForm->minorOffset,
		        psForm->tabMinorThickness, psForm->minorSize,
		        psMajor->numMinor, psForm->minorT,
		        psForm->tabHiLite - psForm->numMajor, pColours, TAB_MINOR, psForm->tabMinorGap);
		break;
	case WFORM_TABRIGHT:
		formDisplayRTabs(psForm, x1 + psForm->tabHorzOffset, y0 + psForm->minorOffset,
		        psForm->tabMinorThickness, psForm->minorSize,
		        psMajor->numMinor, psForm->minorT,
		        psForm->tabHiLite - psForm->numMajor, pColours, TAB_MINOR, psForm->tabMinorGap);
		break;
		/* case WFORM_TABNONE - no minor tabs so nothing to display */
	}
}
