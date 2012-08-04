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

#ifndef TABLE_H_
#define TABLE_H_

#include "widget.h"

/*
 * Forward declarations
 */
typedef struct _table table;
typedef struct _tableVtbl tableVtbl;

struct _tableVtbl
{
	struct _widgetVtbl widgetVtbl;
	
	// No additional virtual methods
};

struct _table
{
	/**
	 * Parent
	 */
	struct _widget widget;
	
	/**
	 * Our vtable
	 */
	tableVtbl *vtbl;
	
	/**
	 * Child layout properties, this must be kept in sync with widget->children
	 * array at all times. Any add/remove operation on one needs to be
	 * accompanied by a corresponding operation on the other
	 */
	vector *childPositions;
	
	/**
	 * The number of rows in the table
	 */
	int numRows;
	
	/**
	 * The number of columns in the table
	 */
	int numColumns;
	
	/**
	 * Default vertical alignment of new cells
	 */
	vAlign vAlignment;
	
	/**
	 * Default horizontal alignment of new cells
	 */
	hAlign hAlignment;
	
	/**
	 * Vertical cell (row) padding
	 */
	int rowPadding;
	
	/**
	 * Horizontal cell (column) padding
	 */
	int columnPadding;
};

/*
 * Type information
 */
extern const classInfo tableClassInfo;

/*
 * Helper macros
 */
#define TABLE(self) (assert(widgetIsA(WIDGET(self), &tableClassInfo)), \
                     (table *) (self))

/*
 * Protected methods
 */
void tableInit(table *self, const char *id);
void tableDestroyImpl(widget *self);
bool tableAddChildDummyImpl(widget *self, widget *child);
void tableRemoveChildImpl(widget *self, widget *child);
void tableDoDrawImpl(widget *self);
bool tableDoLayoutImpl(widget *self);
size tableGetMinSizeImpl(widget *self);
size tableGetMaxSizeImpl(widget *self);

/*
 * Public methods
 */

/**
 * Constructs a new table object and returns it.
 * 
 * @param id    The id of the widget.
 * @return A pointer to an table on success otherwise NULL.
 */
table *tableCreate(const char *id);

/**
 * Sets the default alignment scheme for newly added child widgets. This is the
 * horizontal/vertical alignment used if none is specified.
 *
 * It is important to note that changing these defaults will only affect
 * subsequent calls to tableAddChild* and will have no effect on existing cells.
 *
 * @param self  The table to set the default alignment of.
 * @param h The new horizontal alignment scheme to use.
 * @param v The new vertical alignment scheme to use.
 */
void tableSetDefaultAlign(table *self, hAlign h, vAlign v);

/**
 * Sets padding for all child widgets.
 *
 * @param self  The table to set the padding of.
 * @param h The new amount of horizontal padding to apply.
 * @param v The new amount of vertical padding to apply.
 */
bool tableSetPadding(table *self, int h, int v);

/**
 * Returns the number of distinct rows in the table.
 *
 * @param self  The table to return the number of rows of.
 * @return The number of rows in the table.
 */
int tableGetRowCount(const table *self);

/**
 * Returns the number of distinct columns in the table.
 *
 * @param self  The table to return the number of columns of.
 * @return The number of columns in the table.
 */
int tableGetColumnCount(const table *self);

/**
 * Returns the contents of the table at (row,column). Should the cell be empty,
 * NULL is returned. Row and column are subject to the following invariants:
 *  0 < row < tableGetRowCount(self) and
 *  0 < column < tableGetColumnCount(self).
 *
 * Cells which span multiple rows/columns are included. Therefore this method is
 * not suitable for iterating over the contents of a table, as a single widget
 * may be returned more than once.
 *
 * @param self  The table to return the cell of.
 * @param row   The row of the table cell.
 * @param column    The column of the table cell.
 * @return The widget at (row,column) should one exist; otherwise NULL.
 */
widget *tableGetCell(const table *self, int row, int column);

/**
 * Adds the widget, child, to the table, self, at (row,column). This method is
 * simply a convenience method which wraps around the more functional
 * tableAddChildWithSpanAlign. The cells alignment is that of the tables default
 * and the row-span and column span are both 1.
 *
 * @param self  The table to add the widget to.
 * @param child The child widget to add.
 * @param row   The row in the table to add the child to.
 * @param column    The column in the table add the child to.
 * @return true if child was successfully added, false otherwise.
 * @see tableAddChildWithSpan
 */
bool tableAddChild(table *self, widget *child, int row, int column);

/**
 * Adds the widget, child, to the table, at (row,column) with child spanning
 * rowspan rows and colspan columns.
 *
 * @param self  The table to add the widget to.
 * @param child The child widget to add.
 * @param row   The row in the table to add the child to.
 * @param rowspan   The number of rows spanned by the child.
 * @param column    The column in the table to add the child to.
 * @param colpsan   The number of columns spanned by the child.
 * @return true if the child was successfully added, false otherwise.
 * @see tableAddChildWithSpanAlign
 */
bool tableAddChildWithSpan(table *self, widget *child,
                           int row, int rowspan,
                           int column, int colspan);

/**
 * Adds the widget, child, to the table, self, at (row,column) with the child
 * having a vertical alignment, v and a horizontal alignment, h.
 *
 * @param self  The table to add the widget to.
 * @param child The child widget to add.
 * @param row   The row in the table to add the child to.
 * @param v	Vertical alignment scheme to use.
 * @param column    The column in the table to add the child to.
 * @param h Horizontal alignment scheme to use.
 * @return true if the child was successfully added, false otherwise.
 * @see tableAddChildWithSpanAlign
 */
bool tableAddChildWithAlign(table *self, widget *child,
                            int row, vAlign v,
                            int column, hAlign h);

/**
 * Adds the widget, child, to the table, self, at (row,column) with the child
 * spanning rowspan rows and colspan columns having a vertical alignment, v and
 * a horizontal alignment, h.
 *
 * This method is subject to the following constraints:
 *  - 0 < row <= tableGetRowCount(self) + 1;
 *  - 0 < column <= tableGetColumnCount(self) + 1;
 *  - the cells (row,column) ... (row+rowspan-1,column+colspan-1) must be empty,
 *    (i.e., tableGetCell(self, r, c) == NULL).
 *
 * @param self  The table to add the widget to.
 * @param child The child widget to add.
 * @param row   The row in the table to add the child to.
 * @param rowspan   The number of rows spanned by the child.
 * @param v Vertical alignment scheme to use.
 * @param column    The column in the table to add the child to.
 * @param colspan   The number of columns spanned by the child.
 * @param v Horizontal alignment of the child.
 * @return true if the child was successfully added, false otherwise.
 */
bool tableAddChildWithSpanAlign(table *self, widget *child,
                                int row, int rowspan, vAlign v,
                                int column, int colspan, hAlign h);

#endif /*TABLE_H_*/
