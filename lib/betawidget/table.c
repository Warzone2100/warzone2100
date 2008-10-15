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

#include "table.h"

/**
 * Child bookkeeping information.
 */
typedef struct
{
	/// What row the child is in
	int row;
	
	/// What column the child is in
	int column;
	
	/// How many rows the child spans
	int rowspan;
	
	/// How many columns the child spans
	int colspan;
} childPositionInfo;

static tableVtbl vtbl;

const classInfo tableClassInfo =
{
	&widgetClassInfo,
	"table"
};

table *tableCreate(const char *id)
{
	table *instance = malloc(sizeof(table));
	
	if (instance == NULL)
	{
		return NULL;
	}
	
	// Call the constructor
	tableInit(instance, id);
	
	// Return the new object
	return instance;
}

static void tableInitVtbl(table *self)
{
	static bool initialised = false;
	
	if (!initialised)
	{
		// Copy our parents vtable into ours
		vtbl.widgetVtbl = *(WIDGET(self)->vtbl);
		
		// Overload widget's destroy, draw and size methods
		vtbl.widgetVtbl.destroy     = tableDestroyImpl;
		vtbl.widgetVtbl.addChild    = tableAddChildDummyImpl;
		vtbl.widgetVtbl.doDraw      = tableDoDrawImpl;
		vtbl.widgetVtbl.doLayout    = tableDoLayoutImpl;
		vtbl.widgetVtbl.getMinSize  = tableGetMinSizeImpl;
		vtbl.widgetVtbl.getMaxSize  = tableGetMaxSizeImpl;
		
		initialised = true;
	}
	
	// Replace our parents vtable with our own
	WIDGET(self)->vtbl = &vtbl.widgetVtbl;
	
	// Set our vtable
	self->vtbl = &vtbl;
}

void tableInit(table *self, const char *id)
{
	// Init our parent
	widgetInit(WIDGET(self), id);
	
	// Prepare our vtable
	tableInitVtbl(self);
	
	// Set our type
	WIDGET(self)->classInfo = &tableClassInfo;
	
	// Prepare our child postional info container
	self->childPositions = vectorCreate();
	
	// We have no children and therefore no rows/columns
	self->numRows = self->numColumns = 0;
}

void tableDestroyImpl(widget *self)
{
	// Destroy child positional info
	vectorMapAndDestroy(TABLE(self)->childPositions, free);
	
	// Call our parents destructor
	widgetDestroyImpl(self);
}

int tableGetRowCount(table *self)
{
	return self->numRows;
}

int tableGetColumnCount(table *self)
{
	return self->numColumns;
}

widget *tableGetCell(table *self, int row, int column)
{
	int i;
	const int numChildren = vectorSize(WIDGET(self)->children);

	// Ensure that row and column are valid
	assert(row > 0 && row <= tableGetRowCount(self));
	assert(column > 0 && column <= tableGetColumnCount(self));
	
	// Search through the list of children until we find one matching
	for (i = 0; i < numChildren; i++)
	{
		// Legal as widget->children and table->childPositions are siblings
		const childPositionInfo *pos = vectorAt(self->childPositions, i);
		
		// See if it is a match
		if (row >= pos->row && row < pos->row + pos->rowspan
		 && column >= pos->column && column < pos->column + pos->colspan)
		{
			// We've found the widget
			return vectorAt(WIDGET(self)->children, i);
		}
	}
	
	// If the cell is empty, return NULL
	return NULL;
}

bool tableAddChildDummyImpl(widget *self, widget *child)
{
	assert(!"widgetAddChild can not be called directly on tables; "
		   "please use tableAddChild instead");
	return false;
}

bool tableAddChild(table *self, widget *child, int row, int column)
{
	// Map onto tableAddSpannedChild with rowspan and colspan as 1
	return tableAddSpannedChild(self, child, row, 1, column, 1);
}

bool tableAddSpannedChild(table *self, widget *child, int row, int rowspan,
                          int column, int colspan)
{
	int i, j;
	const int rowCount = tableGetRowCount(self);
	const int columnCount = tableGetColumnCount(self);
	childPositionInfo *pos;
	
	// First, check the row and column is valid
	assert(row > 0 && row <= tableGetRowCount(self) + 1);
	assert(column > 0 && column <= tableGetColumnCount(self) + 1);
	
	// Second, check all of the cells spanned are empty
	for (i = row; i < row + rowspan - 1 && i < rowCount; i++)
	{
		for (j = column; j < column + colspan - 1 && j < columnCount; j++)
		{
			assert(tableGetCell(self, i, j) == NULL);
		}
	}
	
	// Update the row and column counts
	self->numRows = MAX(rowCount, row + rowspan - 1);
	self->numColumns = MAX(columnCount, column + colspan - 1);
	
	// Add positional information regarding the child to our list
	pos = malloc(sizeof(childPositionInfo));
	
	pos->row = row;
	pos->rowspan = rowspan;
	
	pos->column = column;
	pos->colspan = colspan;
	
	vectorAdd(self->childPositions, pos);
	
	// Call widgetAddChildImpl, which will add the child and re-do the layout
	if (widgetAddChildImpl(WIDGET(self), child))
	{
		return true;
	}
	// Problem adding the child; positional information needs to be restored
	else
	{
		vectorRemoveAt(self->childPositions, vectorSize(self->childPositions) - 1);
		
		// Release the memory we malloc'ed earlier
		free(pos);
		
		// Restore the row and column counts
		self->numRows = rowCount;
		self->numColumns = columnCount;
		
		return false;
	}
}

void tableDoDrawImpl(widget *self)
{
	// NO-OP
	(void) self;
}
