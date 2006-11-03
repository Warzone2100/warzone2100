/*
 * WidgInt.h
 *
 * Internal widget library definitions
 */
#ifndef _widgint_h
#define _widgint_h

#include "lib/framework/input.h"

/* Control whether to use malloc for widgets */
#define W_USE_MALLOC	FALSE

/* Control whether the internal widget string heap should be used */
#define W_USE_STRHEAP	FALSE

/* Context information to pass into the widget functions */
typedef struct _w_context
{
	W_SCREEN	*psScreen;			// Parent screen of the widget
	struct _w_form	*psForm;			// Parent form of the widget
	SDWORD		xOffset,yOffset;	// Screen offset of the parent form
	SDWORD		mx,my;				// mouse position on the form
} W_CONTEXT;

/* Set the id number for widgRunScreen to return */
extern void widgSetReturn(WIDGET *psWidget);

/* Find a widget in a screen from its ID number */
extern WIDGET *widgGetFromID(W_SCREEN *psScreen, UDWORD id);

/* Get a string from the string heap */
extern BOOL widgAllocString(char **ppStr);

/* Get a string from the heap and copy in some data.
 * The string to copy will be truncated if it is too long.
 */
extern BOOL widgAllocCopyString(char **ppDest, char *pSrc);

/* Copy one string to another
 * The string to copy will be truncated if it is longer than WIDG_MAXSTR.
 */
extern void widgCopyString(char *pDest, char *pSrc);

/* Return a string to the string heap */
extern void widgFreeString(char *pStr);

/* Release a list of widgets */
extern void widgReleaseWidgetList(WIDGET *psWidgets);

/* Call the correct function for mouse over */
extern void widgHiLite(WIDGET *psWidget, W_CONTEXT *psContext);

/* Call the correct function for mouse moving off */
extern void widgHiLiteLost(WIDGET *psWidget, W_CONTEXT *psContext);

/* Call the correct function for loss of focus */
extern void widgFocusLost(WIDGET *psWidget);

/* Set the keyboard focus for the screen */
extern void screenSetFocus(W_SCREEN *psScreen, WIDGET *psWidget);

/* Clear the keyboard focus */
extern void screenClearFocus(W_SCREEN *psScreen);

#endif

