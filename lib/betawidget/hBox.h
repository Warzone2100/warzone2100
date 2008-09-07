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

#ifndef HBOX_H_
#define HBOX_H_

#include "widget.h"

/*
 * Forward declarations
 */
typedef struct _hBox hBox;
typedef struct _hBoxVtbl hBoxVtbl;

struct _hBoxVtbl
{
	struct _widgetVtbl widgetVtbl;
	
	// No additional virtual methods
};

/*
 * 
 */
struct _hBox
{
	/*
	 * Parent
	 */
	struct _widget widget;
	
	/*
	 * Our vtable
	 */
	hBoxVtbl *vtbl;
	
	/*
	 * Child alignment
	 */
	vAlign vAlignment;
};

/*
 * Type information
 */
extern const classInfo hBoxClassInfo;

/*
 * Helper macros
 */
#define HBOX(self) (assert(widgetIsA(WIDGET(self), &hBoxClassInfo)), \
                    (hBox *) (self))

/*
 * Protected methods
 */
void hBoxInit(hBox *instance, const char *id);
void hBoxDestroyImpl(widget *instance);
bool hBoxDoLayoutImpl(widget *self);
void hBoxDoDrawImpl(widget *self);
size hBoxGetMinSizeImpl(widget *self);
size hBoxGetMaxSizeImpl(widget *self);

/*
 * Public methods
 */

/**
 * Constructs a new hBox object and returns it.
 * 
 * @param id    The id of the widget.
 * @return A pointer to an hBox on success otherwise NULL.
 */
hBox *hBoxCreate(const char *id);

/**
 * Sets the alignment of child widgets of self. This comes into effect when the
 * maximum vertical size of the child widgets is less than that or self.
 * 
 * @param self  The hBox to set the alignment properties of.
 * @param v     The vertical alignment scheme to use.
 */
void hBoxSetVAlign(hBox *self, vAlign v);

#endif /*HBOX_H_*/
