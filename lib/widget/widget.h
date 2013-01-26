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
/** 
 *	@file lib/widget/widget.h
 *	Definitions for the Widget library
 *	@defgroup Widget Widget system
 *	Warzone uses a pixel layout, callback based widget code. While it has several
 *	fallbacks for drawing standard widgets, usually you want to specify your
 *	own drawing callbacks.
 *	@{
 */

#ifndef __INCLUDED_LIB_WIDGET_WIDGET_H__
#define __INCLUDED_LIB_WIDGET_WIDGET_H__

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/textdraw.h"
#include "widgbase.h"

/***********************************************************************************
 *
 * Widget style definitions - these control how the basic widget appears on screen
 */

#define WIDG_HIDDEN		0x8000	///< The widget is initially hidden

/************ Form styles ****************/

#define WFORM_PLAIN		0	///< Plain form
#define WFORM_TABBED		1	///< Tabbed form

/** Invisible (i.e. see through) form - can be used in conjunction with WFORM_PLAIN or WFORM_TABBED. */
#define WFORM_INVISIBLE		2

#define WFORM_CLICKABLE		4	///< Clickable form - return form id when the form is clicked
#define	WFORM_NOCLICKMOVE	8	///< Disable movement on a clickable form

/** 
 * Control whether the primary or secondary buttons work on a clickable form.
 * Primary works by default - this turns it off.
 */
#define WFORM_NOPRIMARY		0x10
#define WFORM_SECONDARY		0x20	///< Enable secondary buttons

/************ Label styles ***************/

#define WLAB_PLAIN		0	///< Plain text only label
#define WLAB_ALIGNLEFT		1	///< Align the text at the left of the box
#define WLAB_ALIGNCENTRE	2	///< Center the text
#define WLAB_ALIGNRIGHT		4	///< Align the text at the right of the box

/************ Button styles **************/

#define WBUT_PLAIN		0	///< Plain button (text with a box around it)
#define	WBUT_NOCLICKMOVE	8	///< Disable movement on a button


/**
 * Control whether the primary or secondary buttons work on a button. Primary works by default - 
 * this turns it off.
 */
#define WBUT_NOPRIMARY		0x10
#define WBUT_SECONDARY		0x20	///< Enable secondary buttons.
#define WBUT_TXTCENTRE		0x40	///< Text only buttons. centre the text?

/*********** Edit Box styles *************/

#define WEDB_PLAIN		0	///< Plain edit box (text with a box around it)

/*********** Bar Graph styles ************/

#define WBAR_PLAIN		0	////< Plain bar graph
#define WBAR_TROUGH		1	///< Bar graph with a trough showing empty percentage
#define WBAR_DOUBLE		2	///< Double bar graph, one on top of other

/*********** Slider styles ***************/

#define WSLD_PLAIN		0	///< Plain slider

/***********************************************************************************/

/** The basic initialisation structure */
struct W_INIT
{
	W_INIT();

	UDWORD                  formID;                 ///< ID number of form to put widget on. ID == 0 specifies the default form for the screen
	UWORD                   majorID, minorID;       ///< Which major and minor tab to put the widget on for a tabbed form
	UDWORD                  id;                     ///< Unique id number (chosen by user)
	UDWORD                  style;                  ///< widget style
	SWORD                   x, y;                   ///< screen location
	UWORD                   width, height;          ///< widget size
	WIDGET_DISPLAY          pDisplay;               ///< Optional display function
	WIDGET_CALLBACK         pCallback;              ///< Optional callback function
	void *                  pUserData;              ///< Optional user data pointer
	UDWORD                  UserData;               ///< User data (if any)
};

/*
 * Flags for controlling where the tabs appear on a form -
 * used in the majorPos and minorPos entries of the W_FORMINIT struct
 */
#define	WFORM_TABNONE		0		///< No tab
#define WFORM_TABTOP		1
#define WFORM_TABLEFT		2
#define WFORM_TABRIGHT		3
#define WFORM_TABBOTTOM		4

/*
 * Upper limits for major and minor tabs on a tab form.
 * Not the best way to do it I know, but it keeps the memory
 * management MUCH simpler.
 */

// The below define is max # of tabs we can have.
// It is set to 20  Look @  #define	MAXSTRUCTURES	200 in hci.h  Keep them in check!
// New routines really have no max limit. I am not sure what max # a legal user can have.
#define WFORM_MAXMAJOR		40	   // Maximum number of major tabs on a tab form
// we do NOT use MAX MINOR now, it is another way to draw the widgets.
#define WFORM_MAXMINOR		5	   //15		// Maximum number of minor tabs off a major
#define MAX_TAB_STD_SHOWN   4		// max # of tabs we can display using standard tab icons.
#define MAX_TAB_SMALL_SHOWN 8		// max # of tabs we can display using small tab icons.
#define TAB_SEVEN    7		//*with* tab scroll buttons, we can only (currently) show 7 max!
// NOTE: enable TAB_MINOR at your own risk.  Have NOT testest new rotuines with that.
#define TAB_MINOR 0	// Tab types passed into tab display callbacks.
#define TAB_MAJOR 1

typedef void (*TAB_DISPLAY)(WIDGET *psWidget, UDWORD TabType, UDWORD Position, UDWORD Number, bool Selected, bool Hilight, UDWORD x, UDWORD y, UDWORD Width, UDWORD Height);
typedef void (*FONT_DISPLAY)(UDWORD x, UDWORD y, char *String);

/** Form initialisation structure */
struct W_FORMINIT : public W_INIT
{
	W_FORMINIT();

	/* Data for a tabbed form */
	bool                    disableChildren;
	UWORD			majorPos, minorPos;		// Position of the tabs on the form
	UWORD			majorSize, minorSize;		// Size of the tabs (in pixels)
	SWORD			majorOffset, minorOffset;	// Tab start offset.
	SWORD			tabVertOffset;			///< Tab form overlap offset.
	SWORD			tabHorzOffset;			///< Tab form overlap offset.
	UWORD			tabMajorThickness;		///< The thickness of the tabs
	UWORD			tabMinorThickness;		///< The thickness of the tabs
	UWORD			tabMajorGap;			///< The space between tabs
	UWORD			tabMinorGap;			///< The space between tabs
	UWORD			numStats;			///< Number of "stats" (items) in list
	UWORD			numButtons;			///< Number of buttons per form
	UWORD			numMajor;			///< Number of major tabs
	UWORD			aNumMinors[WFORM_MAXMAJOR];	///< Number of minor tabs for each major
	SWORD			TabMultiplier;			///< Used to tell system we got lots of (virtual) tabs to display
	unsigned                maxTabsShown;                   ///< Maximum number of tabs shown at once.
	const char		*pTip;				///< Tool tip for the form itself
	char			*apMajorTips[WFORM_MAXMAJOR];	///< Tool tips for the major tabs
	char			*apMinorTips[WFORM_MAXMAJOR][WFORM_MAXMINOR];	///< Tool tips for the minor tabs
	TAB_DISPLAY		pTabDisplay;			///< Optional callback for displaying a tab.
	WIDGET_DISPLAY		pFormDisplay;			///< Optional callback to display the form.
};

/** Label initialisation structure */
struct W_LABINIT : public W_INIT
{
	W_LABINIT();

	const char		*pText;			///< label text
	const char		*pTip;			///< Tool tip for the label.
	enum iV_fonts           FontID;			///< ID of the IVIS font to use for this widget.
};

/** Button initialisation structure */
struct W_BUTINIT : public W_INIT
{
	W_BUTINIT();

	const char *pText;	///< Button text
	const char *pTip;	///< Tool tip text
	enum iV_fonts FontID;	//< ID of the IVIS font to use for this widget.
};

/** Edit box initialisation structure */
struct W_EDBINIT : public W_INIT
{
	W_EDBINIT();

	const char *pText;		///< initial contents of the edit box
	enum iV_fonts FontID;		///< ID of the IVIS font to use for this widget.
	WIDGET_DISPLAY pBoxDisplay;	///< Optional callback to display the form.
	FONT_DISPLAY pFontDisplay;	///< Optional callback to display a string.
};

/* Orientation flags for the bar graph */
enum WBAR_ORIENTATION
{
	WBAR_LEFT = 1,                  ///< Bar graph fills from left to right
	WBAR_RIGHT,                     ///< Bar graph fills from right to left
	WBAR_TOP,                       ///< Bar graph fills from top to bottom
	WBAR_BOTTOM,                    ///< Bar graph fills from bottom to top
};

/** Bar Graph initialisation structure */
struct W_BARINIT : public W_INIT
{
	W_BARINIT();

	WBAR_ORIENTATION orientation;           ///< Orientation of the bar on the widget
	UWORD		size;			///< Initial percentage of the graph that is filled
	UWORD		minorSize;		///< Percentage of second bar graph if there is one
	UWORD		iRange;			///< Maximum range
	int             denominator;            ///< Denominator, 1 by default.
	int             precision;              ///< Number of places after the decimal point to display, 0 by default.
	PIELIGHT	sCol;			///< Bar colour
	PIELIGHT	sMinorCol;		///< Minor bar colour
	const char	*pTip;			///< Tool tip text
};


/* Orientation of the slider */
enum WSLD_ORIENTATION
{
	WSLD_LEFT = 1,                  ///< Slider is horizontal and starts at left
	WSLD_RIGHT,                     ///< Slider is horizontal and starts at the right
	WSLD_TOP,                       ///< Slider is vertical and starts at the top
	WSLD_BOTTOM,                    ///< Slider is vertical and starts at the bottom
};

/** Slider initialisation structure */
struct W_SLDINIT : public W_INIT
{
	W_SLDINIT();

	WSLD_ORIENTATION orientation;           ///< Orientation of the slider
	UWORD		numStops;		///< Number of stops on the slider
	UWORD		barSize;		///< Size of the bar
	UWORD		pos;			///< Initial position of the slider bar
	const char	*pTip;			///< Tip string
};

/***********************************************************************************/

/** The maximum lenth of strings for the widget system */
#define WIDG_MAXSTR		80

/** The maximum value for bar graph size */
#define WBAR_SCALE		100

/** Initialise the widget module */
extern bool widgInitialise(void);

/** Reset the widget module */
extern void widgReset(void);

/** Shut down the widget module */
extern void widgShutDown(void);

/** Create an empty widget screen */
extern W_SCREEN* widgCreateScreen(void);

/** Release a screen and all its associated data */
extern void widgReleaseScreen(W_SCREEN *psScreen);

/** Set the tool tip font for a screen */
extern void widgSetTipFont(W_SCREEN *psScreen, enum iV_fonts FontID);

/** Add a form to the widget screen */
extern bool widgAddForm(W_SCREEN *psScreen, const W_FORMINIT* psInit);

/** Add a label to the widget screen */
extern bool widgAddLabel(W_SCREEN *psScreen, const W_LABINIT* psInit);

/** Add a button to a form */
extern bool widgAddButton(W_SCREEN *psScreen, const W_BUTINIT* psInit);

/** Add an edit box to a form */
extern bool widgAddEditBox(W_SCREEN *psScreen, const W_EDBINIT* psInit);

/** Add a bar graph to a form */
extern bool widgAddBarGraph(W_SCREEN *psScreen, const W_BARINIT* psInit);

/** Add a slider to a form */
extern bool widgAddSlider(W_SCREEN *psScreen, const W_SLDINIT* psInit);

/** Delete a widget from the screen */
extern void widgDelete(W_SCREEN *psScreen, UDWORD id);

/** Hide a widget */
extern void widgHide(W_SCREEN *psScreen, UDWORD id);

/** Reveal a widget */
extern void widgReveal(W_SCREEN *psScreen, UDWORD id);

/** Return a pointer to a buffer containing the current string of a widget if any.
 * This will always return a valid string pointer.
 * NOTE: The string must be copied out of the buffer
 */
extern const char *widgGetString(W_SCREEN *psScreen, UDWORD id);

/** Set the text in a widget */
extern void widgSetString(W_SCREEN *psScreen, UDWORD id, const char *pText);

/** Set the current tabs for a tab form */
extern void widgSetTabs(W_SCREEN *psScreen, UDWORD id, UWORD major, UWORD minor);

/** Get the current tabs for a tab form */
extern void widgGetTabs(W_SCREEN *psScreen, UDWORD id, UWORD *pMajor, UWORD *pMinor);

/** Get the number of major tab in a tab form. */
int widgGetNumTabMajor(W_SCREEN *psScreen, UDWORD id);

/** Get the number of minor tabs in a tab form. */
int widgGetNumTabMinor(W_SCREEN *psScreen, UDWORD id, UWORD pMajor);

/** Get the current position of a widget */
extern void widgGetPos(W_SCREEN *psScreen, UDWORD id, SWORD *pX, SWORD *pY);

/** Get the current position of a slider bar */
extern UDWORD widgGetSliderPos(W_SCREEN *psScreen, UDWORD id);

/** Set the current position of a slider bar */
extern void widgSetSliderPos(W_SCREEN *psScreen, UDWORD id, UWORD pos);

/** Set the current size of a bar graph */
extern void widgSetBarSize(W_SCREEN *psScreen, UDWORD id, UDWORD size);

/** Set the current size of a minor bar on a double graph */
extern void widgSetMinorBarSize(W_SCREEN *psScreen, UDWORD id, UDWORD size);

/** Return the ID of the widget the mouse was over this frame */
extern UDWORD widgGetMouseOver(W_SCREEN *psScreen);

/** Return the user data for a widget */
extern void *widgGetUserData(W_SCREEN *psScreen, UDWORD id);

/** Set the user data for a widget */
extern void widgSetUserData(W_SCREEN *psScreen, UDWORD id, void *UserData);

/** Return the user data for a widget */
extern UDWORD widgGetUserData2(W_SCREEN *psScreen, UDWORD id);

/** Set the user data for a widget */
extern void widgSetUserData2(W_SCREEN *psScreen, UDWORD id,UDWORD UserData);

/** Return the user data for the returned widget */
extern void *widgGetLastUserData(W_SCREEN *psScreen);

/** Get widget structure */
extern WIDGET *widgGetFromID(W_SCREEN *psScreen, UDWORD id);

/** Set tip string for a widget */
extern void widgSetTip(W_SCREEN *psScreen, UDWORD id, const char *pTip);
extern void widgSetTipText(WIDGET* psWidget, const char* pTip);

/** Colour numbers */
enum _w_colour
{
	WCOL_BKGRND,	///< Background colours
	WCOL_TEXT,	///< Text colour
	WCOL_LIGHT,	///< Light colour for 3D effects
	WCOL_DARK,	///< Dark colour for 3D effects
	WCOL_HILITE,	///< Hilite colour
	WCOL_CURSOR,	///< Edit Box cursor colour
	WCOL_TIPBKGRND,	///< Background for the tool tip window
	WCOL_DISABLE,	///< Text colour on a disabled button

	WCOL_MAX,	///< All colour numbers are less than this
};

/** Set a colour on a form */
extern void widgSetColour(W_SCREEN *psScreen, UDWORD id, UDWORD index, PIELIGHT colour);

/** Set the global toop tip text colour. */
extern void widgSetTipColour(PIELIGHT colour);

/* Possible states for a button */
#define WBUT_DISABLE	0x0001		///< Disable (grey out) a button
#define WBUT_LOCK	0x0002		///< Fix a button down
#define WBUT_CLICKLOCK	0x0004		///< Fix a button down but it is still clickable
#define WBUT_FLASH	0x0008		///< Make a button flash.

extern void widgSetButtonFlash(W_SCREEN *psScreen, UDWORD id);
extern void widgClearButtonFlash(W_SCREEN *psScreen, UDWORD id);

/** Get a button or clickable form's state */
extern UDWORD widgGetButtonState(W_SCREEN *psScreen, UDWORD id);

/** Set a button or clickable form's state */
extern void widgSetButtonState(W_SCREEN *psScreen, UDWORD id, UDWORD state);


/** Return which key was used to press the last returned widget */
extern UDWORD widgGetButtonKey_DEPRECATED(W_SCREEN *psScreen);

/** Initialise the set of widgets that make up a screen.
 * Call this once before calling widgRunScreen and widgDisplayScreen.
 * This should only be called once before calling Run and Display as many times
 * as is required.
 */
extern void widgStartScreen(W_SCREEN *psScreen);

/** Clean up after a screen has been run.
 * Call this after the widgRunScreen / widgDisplayScreen cycle.
 */
extern void widgEndScreen(W_SCREEN *psScreen);

/** Execute a set of widgets for one cycle.
 * Return the id of the widget that was activated, or 0 for none.
 */
extern UDWORD widgRunScreen(W_SCREEN *psScreen);

/** Display the screen's widgets in their current state
 * (Call after calling widgRunScreen, this allows the input
 *  processing to be seperated from the display of the widgets).
 */
extern void widgDisplayScreen(W_SCREEN *psScreen);


/** Set the current audio callback function and audio id's. */
extern void WidgSetAudio(WIDGET_AUDIOCALLBACK Callback,SWORD HilightID,SWORD ClickedID);

/** Get pointer to current audio callback function. */
extern WIDGET_AUDIOCALLBACK WidgGetAudioCallback(void);

/** Get current audio ID for hilight. */
extern SWORD WidgGetHilightAudioID(void);

/** Get current audio ID for clicked. */
extern SWORD WidgGetClickedAudioID(void);

/** Enable or disable all sliders. */
extern void sliderEnableDrag(bool Enable);

extern void setWidgetsStatus( bool var );
extern bool getWidgetsStatus( void );

extern void CheckpsMouseOverWidget( void *psWidget );
/** @} */

#endif // __INCLUDED_LIB_WIDGET_WIDGET_H__
