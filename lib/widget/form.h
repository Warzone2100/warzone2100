/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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
 *  Definitions for the form functions.
 */

#ifndef __INCLUDED_LIB_WIDGET_FORM_H__
#define __INCLUDED_LIB_WIDGET_FORM_H__

#include "lib/widget/widget.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

/* The basic form data */
#define FORM_BASE \
	WIDGET_BASE;				/* The common widget data */ \
	\
	BOOL		disableChildren;	/* Disable all child widgets if true */ \
	UWORD		Ax0,Ay0,Ax1,Ay1; 	/* Working coords for animations. */ \
	UDWORD		animCount; 			/* Animation counter. */ \
	UDWORD		startTime;			/* Animation start time */ \
	PIELIGHT	aColours[WCOL_MAX];		/* Colours for the form and its widgets */ \
	WIDGET		*psLastHiLite;	/* The last widget to be hilited */ \
								/* This is used to track when the mouse moves */ \
								/* off something */ \
	WIDGET		*psWidgets		/* The widgets on the form */


/* The standard form */
typedef struct _w_form
{
	/* The common form data */
	FORM_BASE;
} W_FORM;

/* Information for a minor tab */
typedef struct _w_minortab
{
	/* Graphics data for the tab will go here */
	WIDGET		*psWidgets;			// Widgets on the tab
	char		*pTip;				// Tool tip
} W_MINORTAB;

/* Information for a major tab */
typedef struct _w_majortab
{
	/* Graphics data for the tab will go here */
	UWORD			lastMinor;					// Store which was the last selected minor tab
	UWORD			numMinor;
	W_MINORTAB		asMinor[WFORM_MAXMINOR];	// Minor tab information
	char			*pTip;
} W_MAJORTAB;

/* The tabbed form data structure */
typedef struct _w_tabform
{
	/* The common form data */
	FORM_BASE;

	UWORD		majorPos, minorPos;		// Position of the tabs on the form
	UWORD		majorSize,minorSize;	// the size of tabs horizontally and vertically
	UWORD		tabMajorThickness;			// The thickness of the tabs
	UWORD		tabMinorThickness;			// The thickness of the tabs
	UWORD		tabMajorGap;					// The gap between tabs
	UWORD		tabMinorGap;					// The gap between tabs
	SWORD		tabVertOffset;				// Tab form overlap offset.
	SWORD		tabHorzOffset;				// Tab form overlap offset.
	SWORD		majorOffset;			// Tab start offset.
	SWORD		minorOffset;			// Tab start offset.
	UWORD		majorT,minorT;			// which tab is selected
	UWORD		state;					// Current state of the widget
	UWORD		tabHiLite;				// which tab is hilited.
	/* NOTE: If tabHiLite is (UWORD)(-1) then there is no hilite.  A bit of a hack I know */
	/*       but I don't really have the energy to change it.  (Don't design stuff after  */
	/*       beers at lunch-time :-)                                                      */

	UWORD		numMajor;				// The number of major tabs
	SWORD		TabMultiplier;				//used to tell system we got lots of tabs to display
	UWORD		numStats;				//# of 'stats' (items) in list
	UWORD		numButtons;				//# of buttons per form
	W_MAJORTAB	asMajor[WFORM_MAXMAJOR];	// The major tab information
	TAB_DISPLAY pTabDisplay;			// Optional callback for display tabs.
} W_TABFORM;


/* Button states for a clickable form */
#define WCLICK_NORMAL		0x0000
#define WCLICK_DOWN			0x0001		// Button is down
#define WCLICK_GREY			0x0002		// Button is disabled
#define WCLICK_HILITE		0x0004		// Button is hilited
#define WCLICK_LOCKED		0x0008		// Button is locked down
#define WCLICK_CLICKLOCK	0x0010		// Button is locked but clickable
#define WCLICK_FLASH		0x0020		// Button flashing is enabled
#define WCLICK_FLASHON		0x0040		// Button is flashing

/* The clickable form data structure */
typedef struct _w_clickform
{
	/* The common form data */
	FORM_BASE;

	UDWORD		state;					// Button state of the form
	const char	*pTip;					// Tip for the form
	SWORD HilightAudioID;				// Audio ID for form clicked sound
	SWORD ClickedAudioID;				// Audio ID for form hilighted sound
	WIDGET_AUDIOCALLBACK AudioCallback;	// Pointer to audio callback function
} W_CLICKFORM;

extern void formClearFlash(W_FORM *psWidget);

/* Create a form widget data structure */
extern W_FORM* formCreate(const W_FORMINIT* psInit);

/* Free the memory used by a form */
extern void formFree(W_FORM *psWidget);

/* Add a widget to a form */
extern BOOL formAddWidget(W_FORM *psForm, WIDGET *psWidget, W_INIT *psInit);

/* Initialise a form widget before running it */
extern void formInitialise(W_FORM *psWidget);

/* Return the widgets currently displayed by a form */
extern WIDGET *formGetWidgets(W_FORM *psWidget);

/* Return the origin on the form from which button locations are calculated */
extern void formGetOrigin(W_FORM *psWidget, SDWORD *pXOrigin, SDWORD *pYOrigin);

/* Variables for the formGetAllWidgets functions */
typedef struct _w_formgetall
{
	WIDGET		*psGAWList;
	W_TABFORM	*psGAWForm;
	W_MAJORTAB	*psGAWMajor;
	UDWORD		GAWMajor, GAWMinor;
} W_FORMGETALL;

/* Initialise the formGetAllWidgets function */
extern void formInitGetAllWidgets(W_FORM *psWidget, W_FORMGETALL *psCtrl);

/* Repeated calls to this function will return widget lists
 * until all widgets in a form have been returned.
 * When a NULL list is returned, all widgets have been seen.
 */
extern WIDGET *formGetAllWidgets(W_FORMGETALL *psCtrl);

/* Get the button state of a click form */
extern UDWORD formGetClickState(W_CLICKFORM *psForm);

extern void formSetFlash(W_FORM *psWidget);

/* Set the button state of a click form */
extern void formSetClickState(W_CLICKFORM *psForm, UDWORD state);

/* Run a form widget */
extern void formRun(W_FORM *psWidget, W_CONTEXT *psContext);

/* Respond to a mouse click */
extern void formClicked(W_FORM *psWidget, UDWORD key);

/* Respond to a mouse form up */
extern void formReleased(W_FORM *psWidget, UDWORD key, W_CONTEXT *psContext);

/* Respond to a mouse moving over a form */
extern void formHiLite(W_FORM *psWidget, W_CONTEXT *psContext);

/* Respond to the mouse moving off a form */
extern void formHiLiteLost(W_FORM *psWidget, W_CONTEXT *psContext);

/* Display function prototypes */
extern void formDisplay(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
extern void formDisplayClickable(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
extern void formDisplayTabbed(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_LIB_WIDGET_FORM_H__
