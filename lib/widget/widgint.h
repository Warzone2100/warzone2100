/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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

