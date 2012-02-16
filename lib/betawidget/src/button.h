/*
	This file is part of Warzone 2100.
	Copyright (C) 2009  Elio Gubser
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

#ifndef BUTTON_H_
#define BUTTON_H_

#include "widget.h"
#include "patternManager.h"

/*
 * Forward declarations
 */
typedef struct _button button;
typedef struct _buttonVtbl buttonVtbl;

typedef enum
{
	BUTTON_STATE_NORMAL=0,
	BUTTON_STATE_DISABLED,
	BUTTON_STATE_MOUSEOVER,
	BUTTON_STATE_MOUSEDOWN,

	/// Must be the last member
	BUTTON_STATE_COUNT
} buttonState;

struct _buttonVtbl
{
	struct _widgetVtbl widgetVtbl;

	void	(*doDrawNormal)     (button *self);

	void	(*doDrawDisabled)   (button *self);

	void	(*doDrawMouseOver)  (button *self);

	void	(*doDrawMouseDown)  (button *self);

	void	(*doButtonPath)     (button *self, cairo_t *cr);
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

	/**
	 * The current button state
	 */
	buttonState state;

	/**
	 * Each button state has a fill and contour pattern
	 */
	pattern *fillPatterns[BUTTON_STATE_COUNT];
	pattern *contourPatterns[BUTTON_STATE_COUNT];
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
button *buttonCreate(const char *id, int w, int h);
void buttonInit(button *self, const char *id, int w, int h);
void buttonDestroyImpl(widget *self);
void buttonDoDrawImpl(widget *self);
void buttonDoDrawMaskImpl(widget *self);
void buttonDoDrawNormalImpl(button *self);
void buttonDoDrawDisabledImpl(button *self);
void buttonDoDrawMouseOverImpl(button *self);
void buttonDoDrawMouseDownImpl(button *self);
void buttonDoButtonPathImpl(button *self, cairo_t *cr);
size buttonGetMinSizeImpl(widget *self);
size buttonGetMaxSizeImpl(widget *self);

/*
 * Protected virtual methods
 */
/**
 * Called the draw the button.
 *
 * @param self  The button to draw.
 */
void buttonDoDrawNormal(button *self);

/**
 * Called to draw the button when it is disabled (WIDGET->isEnabled == false).
 *
 * @param self  The button to draw.
 */
void buttonDoDrawDisabled(button *self);

/**
 * Called to draw the button when the mouse is over it.
 *
 * @param self  The button to draw.
 */
void buttonDoDrawMouseOver(button *self);

/**
 * Called to draw the button when a mouse button is depressed on it.
 *
 * @param self  The button to draw.
 */
void buttonDoDrawMouseDown(button *self);

/**
 * Called to set the button's shape path
 *
 * @param self The button for which the shape is created.
 * @param cr The cairo context on which the path is created.
 */
void buttonDoButtonPath(button *self, cairo_t *cr);

/*
 * Public methods
 */
/**
 * Sets the contour and fill patterns for a given button state.
 *
 * @param self The button for which we set the patterns.
 * @param state The button's state for which we set the patterns.
 * @param fillPatternId The pattern used to fill the button. May be NULL or "" to leave the old one.
 * @param contourPatternId The pattern used to outline the button. May be NULL or "" to leave the old one.
 */
void buttonSetPatternsForState(button *self, buttonState state, const char *fillPatternId, const char *contourPatternId);

#endif /* BUTTON_H_ */
