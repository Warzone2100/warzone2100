/*
 * Button.h
 *
 * Definitions for edit box functions.
 */
#ifndef _button_h
#define _button_h

/* The widget heap */
extern OBJ_HEAP	*psButHeap;

/* Button states */
#define WBUTS_NORMAL	0x0000
#define WBUTS_DOWN		0x0001		// Button is down
#define WBUTS_GREY		0x0002		// Button is disabled
#define WBUTS_HILITE	0x0004		// Button is hilited
#define WBUTS_LOCKED	0x0008		// Button is locked down
#define WBUTS_CLICKLOCK	0x0010		// Button is locked but clickable
#define WBUTS_FLASH		0x0020		// Button flashing is enabled
#define WBUTS_FLASHON	0x0040		// Button is flashing

typedef struct _w_button
{
	/* The common widget data */
	WIDGET_BASE;

	UDWORD		state;				// The current button state
	STRING		*pText;				// The text for the button
	STRING		*pTip;				// The tool tip for the button
	SWORD HilightAudioID;				// Audio ID for form clicked sound
	SWORD ClickedAudioID;				// Audio ID for form hilighted sound
	WIDGET_AUDIOCALLBACK AudioCallback;	// Pointer to audio callback function
//	PROP_FONT	*psFont;			// button font
	int FontID;
} W_BUTTON;

/* Initialise the button module */
extern BOOL buttonStartUp(void);

/* Create a button widget data structure */
extern BOOL buttonCreate(W_BUTTON **ppsWidget, W_BUTINIT *psInit);

/* Free the memory used by a button */
extern void buttonFree(W_BUTTON *psWidget);

/* Initialise a button widget before running it */
extern void buttonInitialise(W_BUTTON *psWidget);

/* Run a button widget */
extern void buttonRun(W_BUTTON *psWidget);

/* Respond to a mouse click */
extern void buttonClicked(W_BUTTON *psWidget, UDWORD key);

/* Respond to a mouse button up */
extern void buttonReleased(W_BUTTON *psWidget, UDWORD key);

/* Respond to a mouse moving over a button */
extern void buttonHiLite(W_BUTTON *psWidget, W_CONTEXT *psContext);

/* Respond to the mouse moving off a button */
extern void buttonHiLiteLost(W_BUTTON *psWidget);

/* Get a button's state */
extern UDWORD buttonGetState(W_BUTTON *psButton);

/* Set a button's state */
extern void buttonSetState(W_BUTTON *psWidget, UDWORD state);

extern void buttonSetFlash(W_BUTTON *psButton);
extern void buttonClearFlash(W_BUTTON *psButton);

/* The button display function */
extern void buttonDisplay(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, UDWORD *pColours);

#endif

