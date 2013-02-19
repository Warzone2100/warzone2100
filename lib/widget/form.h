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
 *  Definitions for the form functions.
 */

#ifndef __INCLUDED_LIB_WIDGET_FORM_H__
#define __INCLUDED_LIB_WIDGET_FORM_H__

#include "lib/widget/widget.h"


/* The standard form */
struct W_FORM : public WIDGET
{
	W_FORM(W_FORMINIT const *init);

	void widgetLost(WIDGET *widget);

	void clicked(W_CONTEXT *psContext, WIDGET_KEY key);
	void highlightLost();
	void display(int xOffset, int yOffset, PIELIGHT *pColours);

	bool            disableChildren;        ///< Disable all child widgets if true
	UWORD           Ax0, Ay0, Ax1, Ay1;     ///< Working coords for animations.
	UDWORD          animCount;              ///< Animation counter.
	UDWORD          startTime;              ///< Animation start time
	PIELIGHT        aColours[WCOL_MAX];     ///< Colours for the form and its widgets
	WIDGET         *psLastHiLite;           ///< The last widget to be hilited. This is used to track when the mouse moves off something.
	WIDGET         *psWidgets;              ///< The widgets on the form
};

/* Information for a major tab */
struct W_MAJORTAB
{
	W_MAJORTAB();

	/* Graphics data for the tab will go here */
	WIDGET *                psWidgets;              // Widgets on the tab
};

/* The tabbed form data structure */
struct W_TABFORM : public W_FORM
{
	W_TABFORM(W_FORMINIT const *init);

	void released(W_CONTEXT *psContext, WIDGET_KEY key);
	void highlightLost();
	void run(W_CONTEXT *psContext);
	void display(int xOffset, int yOffset, PIELIGHT *pColours);

	int tab() const { return currentTab; }
	bool setTab(int newTab);  ///< Sets the tab, clamped to a valid range. Returns true iff newTab was in the valid range.
	bool scrollDeltaTab(int delta) { return setTab(tab() + delta); }
	bool scrollNextTab() { return scrollDeltaTab(1); }
	bool scrollPreviousTab() { return scrollDeltaTab(-1); }
	int numTabs() const { return childTabs.size(); }
	void setNumTabs(int numTabs);  ///< Adds/removes tabs. If removing tabs, widgets on the removed tabs will be deleted.
	WIDGET *tabWidget() { return childTabs[currentTab]; }

	UWORD           majorPos;                       // Position of the tabs on the form
	UWORD           majorSize;                      // the size of tabs horizontally and vertically
	UWORD		tabMajorThickness;			// The thickness of the tabs
	UWORD		tabMajorGap;					// The gap between tabs
	SWORD		tabVertOffset;				// Tab form overlap offset.
	SWORD		tabHorzOffset;				// Tab form overlap offset.
	SWORD		majorOffset;			// Tab start offset.
	UWORD		state;					// Current state of the widget
	UWORD		tabHiLite;				// which tab is hilited.
	/* NOTE: If tabHiLite is (UWORD)(-1) then there is no hilite.  A bit of a hack I know */
	/*       but I don't really have the energy to change it.  (Don't design stuff after  */
	/*       beers at lunch-time :-)                                                      */

	SWORD		TabMultiplier;				//used to tell system we got lots of tabs to display
	unsigned        maxTabsShown;                   ///< Maximum number of tabs shown at once.
	UWORD		numStats;				//# of 'stats' (items) in list
	UWORD		numButtons;				//# of buttons per form
	Children        childTabs;                      // The major tab information
	TAB_DISPLAY pTabDisplay;			// Optional callback for display tabs.

private:
	void fixChildGeometry();
	void displayTabs(int x0, int y0, int width, int height, int number, int selected, int highlight, PIELIGHT *pColours, int tabGap);

	unsigned currentTab;
};

/* The clickable form data structure */
struct W_CLICKFORM : public W_FORM
{
	W_CLICKFORM(W_FORMINIT const *init);

	void clicked(W_CONTEXT *psContext, WIDGET_KEY key);
	void released(W_CONTEXT *psContext, WIDGET_KEY key);
	void highlight(W_CONTEXT *psContext);
	void highlightLost();
	void display(int xOffset, int yOffset, PIELIGHT *pColours);

	unsigned getState();
	void setState(unsigned state);
	void setFlash(bool enable);
	void setTip(QString string);

	UDWORD		state;					// Button state of the form
	QString         pTip;                   // Tip for the form
	SWORD HilightAudioID;				// Audio ID for form clicked sound
	SWORD ClickedAudioID;				// Audio ID for form hilighted sound
	WIDGET_AUDIOCALLBACK AudioCallback;	// Pointer to audio callback function
};

/* Add a widget to a form */
bool formAddWidget(W_FORM *psForm, WIDGET *psWidget, W_INIT const *psInit);

/* Return the widgets currently displayed by a form */
WIDGET::Children const &formGetWidgets(W_FORM *psWidget);

/* Variables for the formGetAllWidgets functions */
struct W_FORMGETALL
{
	WIDGET::Children::const_iterator tabBegin, tabEnd, begin, end;
};

/* Initialise the formGetAllWidgets function */
extern void formInitGetAllWidgets(W_FORM *psWidget, W_FORMGETALL *psCtrl);

/* Repeated calls to this function will return widget lists
 * until all widgets in a form have been returned.
 * When a NULL list is returned, all widgets have been seen.
 */
extern WIDGET *formGetAllWidgets(W_FORMGETALL *psCtrl);

#endif // __INCLUDED_LIB_WIDGET_FORM_H__
