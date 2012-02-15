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

#ifndef HBOX_H_
#define HBOX_H_

#include "table.h"

/*
 * Forward declarations
 */
typedef struct _hBox hBox;
typedef struct _hBoxVtbl hBoxVtbl;

struct _hBoxVtbl
{
	struct _tableVtbl tableVtbl;

	// No additional virtual methods
};

/*
 *
 */
struct _hBox
{
	/**
	 * Parent
	 */
	struct _table table;

	/**
	 * Vtable
	 */
	struct _hBoxVtbl *vtbl;
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
bool hBoxAddChildImpl(widget *self, widget *child);

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

/**
 * Sets the padding (spacing) between child widgets. Since changing the padding
 * affects the minimum/maximum size of the hBox a call to this method causes the
 * window layout to be redone.
 *
 * Should the new padding result in an untenable layout then the padding is
 * restored to its previous value and the layout restored.
 *
 * @param self  The hBox to set the padding property of.
 * @param padding   The amount of padding to apply.
 * @return true if the padding was changed successfully; false otherwise.
 */
bool hBoxSetPadding(hBox *self, int padding);

#endif /*HBOX_H_*/
