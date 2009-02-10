/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008-2009  Warzone Resurrection Project

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

#ifndef BUTTON_H_
#define BUTTON_H_

#include "widget.h"

/*
 * Forward declarations
 */
typedef struct _button button;
typedef struct _buttonVtbl buttonVtbl;

struct _buttonVtbl
{
	struct _widgetVtbl widgetVtbl;

	void    (*doNormalDraw)     (button *self);

	void    (*doDisabledDraw)   (button *self);

	void    (*doMouseOverDraw)  (button *self);

	void	(*doMouseDownDraw)  (button *self);
};

struct _button
{
	/**
	 * Parent
	 */
	struct _widget widget;

	/**
	 * Vtable
	 */
	struct _buttonVtbl *vtbl;
};

/*
 * Type information
 */
extern const classInfo buttonClassInfo;

/*
 * Helper macros
 */
#define BUTTON(self) (assert(widgetIsA(WIDGET(self), &buttonClassInfo)), \
		              (button *) (self))
#define BUTTON_GET_VTBL(self) (BUTTON(self)->vtbl)
#define BUTTON_CHECK_METHOD(self, method) (assert(BUTTON_GET_VTBL(self)->method))

/*
 * Protected methods
 */
void buttonInit(button *self, const char *id);
void buttonDestroyImpl(widget *self);
void buttonDoDrawImpl(widget *self);
void buttonDoDisabledDrawImpl(button *self);

/*
 * Protected virtual methods
 */

/**
 * Called the draw the button.
 *
 * @param self  The button to draw.
 */
void buttonDoNormalDraw(button *self);

/**
 * Called to draw the button when it is disabled (WIDGET->isEnabled == false).
 *
 * @param self  The button to draw.
 */
void buttonDoDisabledDraw(button *self);

/**
 * Called to draw the button when the mouse is over it.
 *
 * @param self  The button to draw.
 */
void buttonDoMouseOverDraw(button *self);

/**
 * Called to draw the button when a mouse button is depressed on it.
 *
 * @param self  The button to draw.
 */
void buttonDoMouseDownDraw(button *self);

#endif /* BUTTON_H_ */
