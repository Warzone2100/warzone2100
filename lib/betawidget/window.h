/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008  Warzone Resurrection Project

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

/*
 * Forward declarations
 */
typedef struct _window window;
typedef struct _windowVtbl windowVtbl;

struct _windowVtbl
{
	widgetVtbl widgetVtbl;
	
	// No additional virtual methods
};

struct _window
{
	/**
	 * Parent
	 */
	widget widget;
	
	/**
	 * Our vtable
	 */
	windowVtbl *vtbl;
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
void windowInit(window *self, const char *id, int x, int y, int w, int h);
void windowDestroyImpl(widget *self);
bool windowDoLayoutImpl(widget *self);
void windowDoDrawImpl(widget *self);
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

#endif /*WINDOW_H_*/
