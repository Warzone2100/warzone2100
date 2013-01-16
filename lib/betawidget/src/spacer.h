/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008-2013  Warzone 2100 Project

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

#ifndef SPACER_H_
#define SPACER_H_

#include "widget.h"

/*
 * Forward declarations
 */
typedef struct _spacer spacer;
typedef struct _spacerVtbl spacerVtbl;

/*
 * The directions a spacer can act in
 */
typedef enum 
{
	SPACER_DIRECTION_HORIZONTAL,
	SPACER_DIRECTION_VERTICAL
} spacerDirection;

struct _spacerVtbl
{
	struct _widgetVtbl widgetVtbl;
	
	// No additional virtual methods
};

struct _spacer
{
	/*
	 * Parent
	 */
	struct _widget widget;
	
	/*
	 * Our vtable
	 */
	spacerVtbl *vtbl;
	
	/*
	 * Which direction we should act in
	 */
	spacerDirection direction;
};

/*
 * Type information
 */
extern const classInfo spacerClassInfo;

/*
 * Helper macros
 */
#define SPACER(self) (assert(widgetIsA(WIDGET(self), &spacerClassInfo)), \
                      (spacer *) (self))

/*
 * Protected methods
 */
void spacerInit(spacer *self, const char *id, spacerDirection direction);
void spacerDestroyImpl(widget *self);
void spacerDoDrawImpl(widget *self);
bool spacerDoLayoutImpl(widget *self);
bool spacerAddChildDummyImpl(widget *self, widget *child);
size spacerGetMinSizeImpl(widget *self);
size spacerGetMaxSizeImpl(widget *self);

/*
 * Public methods
 */

/**
 * Constructs a new spacer object and returns it.
 * 
 * @param id    The id of the widget.
 * @param direction The direction of the spacer.
 * @return A pointer to an spacer on success otherwise NULL.
 */
spacer *spacerCreate(const char *id, spacerDirection direction);

#endif /*SPACER_H_*/
