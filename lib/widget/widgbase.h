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
 *  Definitions for the basic widget types.
 */

#ifndef __INCLUDED_LIB_WIDGET_WIDGBASE_H__
#define __INCLUDED_LIB_WIDGET_WIDGBASE_H__

#include "lib/framework/frame.h"
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/textdraw.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

/* Button colours */
#define WBUTC_TEXT		0			// Colour for button text
#define WBUTC_BKGRND	1			// Colour for button background
#define WBUTC_BORDER1	2			// Colour for button border
#define WBUTC_BORDER2	3			// 2nd border colour
#define	WBUTC_HILITE	4			// Hilite colour


/* Forward definitions */
struct _widget;
struct _w_context;

/* The display function prototype */
typedef void (*WIDGET_DISPLAY)(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

/* The optional user callback function */
typedef void (*WIDGET_CALLBACK)(struct _widget *psWidget, struct _w_context *psContext);
typedef void (*WIDGET_AUDIOCALLBACK)(int AudioID);


/* The common widget data */
#define WIDGET_BASE \
	UDWORD			formID;			/* ID of the widgets base form. */ \
	UDWORD			id;				/* The user set ID number for the widget */ \
									/* This is returned when e.g. a button is pressed */ \
	WIDGET_TYPE		type;			/* The widget type */ \
	UDWORD			style;			/* The style of the widget */ \
	SWORD			x,y;			/* The location of the widget */ \
	UWORD			width,height;	/* The size of the widget */ \
	WIDGET_DISPLAY	display;		/* Display the widget */\
	WIDGET_CALLBACK	callback;		/* User callback (if any) */\
	void			*pUserData;		/* Pointer to a user data block (if any) */\
	UDWORD			UserData;		/* User data (if any) */\
	\
	struct _widget	*psNext			/* Pointer to the next widget in the screen list */


/* The different base types of widget */
typedef enum _widget_type
{
	WIDG_FORM,
	WIDG_LABEL,
	WIDG_BUTTON,
	WIDG_EDITBOX,
	WIDG_BARGRAPH,
	WIDG_SLIDER,
} WIDGET_TYPE;


/* The base widget data type */
typedef struct _widget
{
	/* The common widget data */
	WIDGET_BASE;
} WIDGET;


/* The screen structure which stores all info for a widget screen */
typedef struct _w_screen
{
	WIDGET*          psForm;        ///< The root form of the screen
	WIDGET*          psFocus;       ///< The widget that has keyboard focus
	enum iV_fonts    TipFontID;     ///< ID of the IVIS font to use for tool tips.
	WIDGET*          psRetWidget;   ///< The widget to be returned by widgRunScreen
} W_SCREEN;

/* Context information to pass into the widget functions */
typedef struct _w_context
{
	W_SCREEN	*psScreen;			// Parent screen of the widget
	struct _w_form	*psForm;			// Parent form of the widget
	SDWORD		xOffset,yOffset;	// Screen offset of the parent form
	SDWORD		mx,my;				// mouse position on the form
} W_CONTEXT;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_LIB_WIDGET_WIDGBASE_H__
