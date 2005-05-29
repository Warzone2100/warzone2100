/*
 * WidgBase.h
 *
 * Definitions for the basic widget types.
 */
#ifndef _widgbase_h
#define _widgbase_h

#include "frame.h"

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

/* Button colours */
#define WBUTC_TEXT		0			// Colour for button text
#define WBUTC_BKGRND	1			// Colour for button background
#define WBUTC_BORDER1	2			// Colour for button border
#define WBUTC_BORDER2	3			// 2nd border colour
#define	WBUTC_HILITE	4			// Hilite colour

/* The display function prototype */
struct _widget;
typedef void (*WIDGET_DISPLAY)(struct _widget *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

/* The optional user callback function */
struct _w_context;
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


/* The base widget data type */
typedef struct _widget
{
	/* The common widget data */
	WIDGET_BASE;
} WIDGET;


/* The screen structure which stores all info for a widget screen */
typedef struct _w_screen
{
	WIDGET		*psForm;			// The root form of the screen
	WIDGET		*psFocus;			// The widget that has keyboard focus
//	PROP_FONT	*psTipFont;			// The font for tool tips
	int			TipFontID;			// ID of the IVIS font to use for tool tips.
} W_SCREEN;

#endif

