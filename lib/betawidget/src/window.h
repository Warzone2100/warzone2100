/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008-2012  Warzone 2100 Project

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

#ifndef WINDOW_H_
#define WINDOW_H_

#include "widget.h"
#include "internal-cairo.h"

#include "patternManager.h"

/*
 * Forward declarations
 */
typedef struct _window window;
typedef struct _windowVtbl windowVtbl;

struct _windowVtbl
{
	struct _widgetVtbl widgetVtbl;
	
	// No additional virtual methods
};

struct _window
{
	/**
	 * Parent
	 */
	struct _widget widget;
	
	/**
	 * Our vtable
	 */
	windowVtbl *vtbl;

	/**
	 * Window gradient to use when drawing the window.
	 */
	pattern* windowPattern;
};

/*
 * Type information
 */
extern const classInfo windowClassInfo;

/*
 * Helper macros
 */
#define WINDOW(self) (assert(widgetIsA(WIDGET(self), &windowClassInfo)), \
(window *) (self))

/*
 * Protected methods
 */
void windowInit(window *self, const char *id, int w, int h);
void windowDestroyImpl(widget *self);
bool windowDoLayoutImpl(widget *self);
void windowDoDrawImpl(widget *self);
void windowDoDrawMaskImpl(widget *self);
bool windowAddChildImpl(widget *self, widget *child);
size windowGetMinSizeImpl(widget *self);
size windowGetMaxSizeImpl(widget *self);

/*
 * Public static methods
 */

/**
 * Sets the internal window-vector pointer to v. Whenever a new window is
 * created it is automatically added to the vecotor. Likewise, when a window is
 * destroyed it is removed from the vector.
 *
 * It is legal for an application to maintain more than one window-vector, e.g.,
 * for separating the windows created by a specific mod. It is, however, an
 * error to invoke windowDestroy on a window which is not currently in the
 * vector.
 *
 * @param v The vector to use for windows.
 */
void windowSetWindowVector(vector *v);

/**
 * Fetches the current window-vector pointer.
 *
 * @return The window-vector pointer currently in-use.
 */
vector *windowGetWindowVector(void);

/**
 * Passes the event evt to each window in the current window vector.
 *
 * @param evt   The event to handle.
 */
void windowHandleEventForWindowVector(const event *evt);

/**
 * Sets the size of the screen to (w,h). It is important that the screen size
 * be set before any calls to windowRepositionFromScreen are made.
 *
 * @param w The width of the screen, in pixels.
 * @param h The height of the screen, in pixels.
 */
void windowSetScreenSize(int w, int h);

/*
 * Public methods
 */

/**
 * Positions the window relative to the screen. The screens dimensions must be
 * set prior to calling this method; this can be done by calling
 * windowSetScreenSize.
 *
 * @param self  The window to position.
 * @param hAlign    The horizontal alignment to use for the x-coord.
 * @param xOffset   The offset to apply to the final x-coord.
 * @param vAlign    The vertical alignment to use for the y-coord.
 * @param yOffset   The offset to apply to the final y-coord.
 */
void windowRepositionFromScreen(window *self, hAlign hAlign, int xOffset,
                                              vAlign vAlign, int yOffset);

/**
 * Positions the window relative to the position of another window, anchor.
 *
 * @param self  The window to position.
 * @param anchor    The window to position self relative to.
 * @param hAlign    The horizontal alignment to use for the x-coord.
 * @param xOffset   The offset to apply to the final x-coord.
 * @param vAlign    The vertical alignment to use for the y-coord.
 * @param yOffset   The offset to apply to the final y-coord.
 */
void windowRepositionFromAnchor(window *self, const window *anchor,
                                hAlign hAlign, int xOffset,
                                vAlign vAlign, int yOffset);

/**
 * Sets the background pattern
 *
 * @param self The window to set the pattern
 * @param patternId The Id of the desired pattern
 */
void windowSetBackgroundPattern(window *self, const char *patternId);

#endif /*WINDOW_H_*/
