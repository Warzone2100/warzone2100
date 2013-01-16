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

#include "table.h"

#include <string.h>

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
	
	/// Vertical alignment
	vAlign vAlignment;
	
	/// Horizontal alignment
	hAlign hAlignment;
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

/**
 * Computes the sum of array between [u...u+v].
 *
 * @param array The array to compute the partial sum of.
 * @param u Starting index.
 * @param v Number of elements to count.
 * @return The sum between u and u+v
 */
static int tablePartialSum(const int *array, int u, int v)
{
	int i;
	int sum = 0;
	
	for (i = u; i < u + v; i++)
	{
		sum += array[i];
	}
	
	return sum;
}

/**
 * Removes any rows from the table which no cells occupy.
 *
 * @param self  The table to remove empty rows from.
 */
static void tableRemoveEmptyRows(table *self)
{
	int i, j, k;
	const int columnCount = tableGetColumnCount(self);
	const int numChildren = vectorSize(WIDGET(self)->children);
	
	// Iterate over each row of the table
	for (i = 1; i <= tableGetRowCount(self); i++)
	{
		// See if any of the columns in the row are occupied
		for (j = 1; j <= columnCount; j++)
		{
			if (tableGetCell(self, i, j))
			{
				break;
			}
		}
		
		// If the cells are all empty, we can remove the row
		if (j > columnCount)
		{
			// Find all cells in rows after the one to be removed
			for (k = 0; k < numChildren; k++)
			{
				childPositionInfo *pos = vectorAt(self->childPositions, k);
				
				// Decrease the row number of all rows > i by 1
				if (pos->row > i)
				{
					pos->row--;
				}
			}
			
			// The table now has one fewer rows in it
			self->numRows--;
			i--;
		}
	}
}

/**
 * Removes any columns from the table which no cells occupy.
 *
 * @param self  The table to remove empty columns from.
 */
static void tableRemoveEmptyColumns(table *self)
{
	int i, j, k;
	const int rowCount = tableGetRowCount(self);
	const int numChildren = vectorSize(WIDGET(self)->children);
	
	// Iterate over each column of the table
	for (i = 1; i <= tableGetColumnCount(self); i++)
	{
		// See if any of the rows in the column are occupied
		for (j = 1; j <= rowCount; j++)
		{
			if (tableGetCell(self, j, i))
			{
				break;
			}
		}
		
		// If the cells are all empty, we can remove the column
		if (j > rowCount)
		{
			// Find all cells in columns after the one to be removed
			for (k = 0; k < numChildren; k++)
			{
				childPositionInfo *pos = vectorAt(self->childPositions, k);
				
				// Decrease the column number of all columns > i by 1
				if (pos->column > i)
				{
					pos->column--;
				}
			}
			
			// The table now has one fewer columns in it
			self->numColumns--;
			i--;
		}
	}
}

/**
 * Returns the (spanning) row with the greatest difference between childSize.y
 * and the sum of the rows it spans (divided by the number of rows it spans).
 *
 * This function is integral to solving the problem about how to decide which
 * rows to increase the height of when handing spanning rows. This is because
 * the order in which we increase the size of rows affects the final layout.
 *
 * @param self  The table to determine the most undersized row of.
 * @param rowHeight An array containing the current row heights of the rows in
 *                  the table.
 * @param childSize The desired (min/max) size of the children in the table.
 * @return The offset of the most undersized row in the table in the
 *         WIDGET(self)->children array, or -1 if there are no such rows.
 */
static int tableGetMostOversizedRow(const table *self, const int *rowHeight,
                                    const size *childSize)
{
	int i;
	float maxDelta = 0.0f;
	int maxDeltaIndex = -1;
	const int numChildren = vectorSize(WIDGET(self)->children);
	
	for (i = 0; i < numChildren; i++)
	{
		const childPositionInfo *pos = vectorAt(self->childPositions, i);
		
		// See if the row is a spanning row
		if (pos->rowspan != 1)
		{
			float sum = tablePartialSum(rowHeight, pos->row - 1, pos->rowspan);
			
			// If the row is higher than the sum of its spanned rows
			if (childSize[i].y > sum)
			{
				float delta = (childSize[i].y - sum) / (float) pos->rowspan;
				
				if (delta > maxDelta)
				{
					maxDelta = delta;
					maxDeltaIndex = i;
				}
			}
		}
	}
	
	return maxDeltaIndex;
}

/**
 * Returns the (spanning) column with the greatest difference between
 * childSize.x and the sum of the columns it spans (divided by the number of
 * columns it spans).
 *
 * @param self  The table to determine the most undersized column of.
 * @param rowHeight An array containing the current column heights of the column
 *                  in the table.
 * @param childSize The desired (min/max) size of the children in the table.
 * @return The offset of the most undersized column in the table in the
 *         WIDGET(self)->children array, or -1 if there are no such rows.
 * @see tableGetMostOversizedRow
 */
static int tableGetMostOversizedColumn(const table *self,
                                       const int *columnWidth,
                                       const size *childSize)
{
	int i;
	float maxDelta = 0.0f;
	int maxDeltaIndex = -1;
	const int numChildren = vectorSize(WIDGET(self)->children);
	
	for (i = 0; i < numChildren; i++)
	{
		const childPositionInfo *pos = vectorAt(self->childPositions, i);
		
		// See if the column is a spanning column
		if (pos->colspan != 1)
		{
			float sum = tablePartialSum(columnWidth, pos->column - 1, pos->colspan);
			
			// If the column is wider than the sum of its spanned columns
			if (childSize[i].x > sum)
			{
				float delta = (childSize[i].x - sum) / (float) pos->colspan;
				
				if (delta > maxDelta)
				{
					maxDelta = delta;
					maxDeltaIndex = i;
				}
			}
		}
	}
	
	return maxDeltaIndex;
}

/**
 * Computes the minimum row/column size for each row/column in the table. It is
 * important to note that the widths/heights are inclusive of padding. Therefore
 * all but the rightmost column and bottom row will have either self->rowPadding
 * or self->columnPadding added onto their sizes.
 *
 * @param self  The table to get the minimum cell sizes of.
 * @param minRowHeight  The array to place the minimum row heights for the table
 *                      into; assumed to be tableGetRowCount(self) in size.
 * @param minColumnWidth    The array to place the minimum column widths for the
 *                          table into; assumed to be tableGetColumnCount(self)
 *                          in size.
 */
static void tableGetMinimumCellSizes(const table *self, int *minRowHeight,
                                     int *minColumnWidth)
{
	int i;
	int spanningIndex;
	
	const int numChildren = vectorSize(WIDGET(self)->children);
	const int numRows = tableGetRowCount(self);
	const int numColumns = tableGetColumnCount(self);
	
	size *minChildSize = alloca(sizeof(size) * numChildren);
	
	// Zero the min row/column sizes
	memset(minRowHeight, '\0', sizeof(int) * numRows);
	memset(minColumnWidth, '\0', sizeof(int) * numColumns);
	
	// Get the minimum row/column sizes for single-spanning cells
	for (i = 0; i < numChildren; i++)
	{
		const childPositionInfo *pos = vectorAt(self->childPositions, i);
		const int row = pos->row - 1;
		const int col  = pos->column - 1;
		
		// Get the minmum size of the cell
		minChildSize[i] = widgetGetMinSize(vectorAt(WIDGET(self)->children, i));
		
		// Take cell padding into account where appropriate
		if (pos->column < numColumns)
		{
			minChildSize[i].x += self->columnPadding;
		}
		
		if (pos->row < numRows)
		{
			minChildSize[i].y += self->rowPadding;
		}
				
		// If the row has a rowspan of 1; see if it is the highest thus far
		if (pos->rowspan == 1)
		{
			minRowHeight[row] = MAX(minRowHeight[row], minChildSize[i].y);
		}
		
		// If the column has a colspan of 1; see if it is the widest thus far
		if (pos->colspan == 1)
		{
			minColumnWidth[col] = MAX(minColumnWidth[col], minChildSize[i].x);
		}
	}
	
	// Handle spanning rows
	while ((spanningIndex = tableGetMostOversizedRow(self,
	                                                 minRowHeight,
	                                                 minChildSize)) != -1)
	{
		int i;
		const childPositionInfo *pos = vectorAt(self->childPositions, spanningIndex);
		
		// Calculate how much larger we need to make the spanned rows
		int delta = minChildSize[spanningIndex].y - tablePartialSum(minRowHeight,
																	pos->row - 1,
																	pos->rowspan);
		
		// Loop over the rows spanned increasing their height by 1
		for (i = pos->row; delta; i = pos->row + (i + 1) % pos->rowspan, delta--)
		{
			minRowHeight[i]++;
		}
	}
	
	// Handle spanning columns
	while ((spanningIndex = tableGetMostOversizedColumn(self,
	                                                    minColumnWidth,
	                                                    minChildSize)) != -1)
	{
		int i;
		const childPositionInfo *pos = vectorAt(self->childPositions, spanningIndex);
		
		// Calculate how much larger we need to make the spanned columns
		int delta = minChildSize[spanningIndex].x - tablePartialSum(minColumnWidth, 
		                                                            pos->column - 1,
		                                                            pos->colspan);
		
		for (i = pos->column; delta; i = pos->column + (i + 1) % pos->colspan, delta--)
		{
			minColumnWidth[i]++;
		}
	}
}

/**
 * Computes the maximum row/column size for each row/column in the table. It is
 * important to note that the widths/heights are inclusive of padding. Therefore
 * all but the rightmost column and bottom row will have either self->rowPadding
 * or self->columnPadding added onto their sizes.
 *
 * @param self  The table to get the minimum cell sizes of.
 * @param maxRowHeight  The array to place the maximum row heights for the table
 *                      into; assumed to be tableGetRowCount(self) in size.
 * @param maxColumnWidth    The array to place the maximum column widths for the
 *                          table into; assumed to be tableGetColumnCount(self)
 *                          in size.
 */
static void tableGetMaximumCellSizes(const table *self, int *maxRowHeight,
                                     int *maxColumnWidth)
{
	int i;
	int spanningIndex;
	
	const int numChildren = vectorSize(WIDGET(self)->children);
	const int numRows = tableGetRowCount(self);
	const int numColumns = tableGetColumnCount(self);
	
	size *maxChildSize = alloca(sizeof(size) * numChildren);
	
	// Zero the min row/column sizes
	memset(maxRowHeight, '\0', sizeof(int) * numRows);
	memset(maxColumnWidth, '\0', sizeof(int) * numColumns);
	
	// Get the maximum row/column sizes for single-spanning cells
	for (i = 0; i < numChildren; i++)
	{
		const childPositionInfo *pos = vectorAt(self->childPositions, i);
		const int row = pos->row - 1;
		const int col  = pos->column - 1;
		
		// Get the maximum size of the cell
		maxChildSize[i] = widgetGetMaxSize(vectorAt(WIDGET(self)->children, i));
		
		// If the row has a rowspan of 1; see if it is the highest thus far
		if (pos->rowspan == 1)
		{
			maxRowHeight[row] = MAX(maxRowHeight[row], maxChildSize[i].y);
		}
		
		// If the column has a colspan of 1; see if it is the widest thus far
		if (pos->colspan == 1)
		{
			maxColumnWidth[col] = MAX(maxColumnWidth[col], maxChildSize[i].x);
		}
	}
	
	// Handle spanning rows
	while ((spanningIndex = tableGetMostOversizedRow(self,
	                                                 maxRowHeight,
	                                                 maxChildSize)) != -1)
	{
		int i;
		const childPositionInfo *pos = vectorAt(self->childPositions, spanningIndex);
		
		// Calculate how much larger we need to make the spanned rows
		int delta = maxChildSize[spanningIndex].y - tablePartialSum(maxRowHeight,
																	pos->row - 1,
																	pos->rowspan);
		
		// Loop over the rows spanned increasing their height by 1
		for (i = pos->row; delta; i = pos->row + (i + 1) % pos->rowspan, delta--)
		{
			maxRowHeight[i]++;
		}
	}
	
	// Handle spanning columns
	while ((spanningIndex = tableGetMostOversizedColumn(self,
	                                                    maxColumnWidth,
	                                                    maxChildSize)) != -1)
	{
		int i;
		const childPositionInfo *pos = vectorAt(self->childPositions, spanningIndex);
		
		// Calculate how much larger we need to make the spanned columns
		int delta = maxChildSize[spanningIndex].x - tablePartialSum(maxColumnWidth, 
		                                                            pos->column - 1,
		                                                            pos->colspan);
		
		for (i = pos->column; delta; i = pos->column + (i + 1) % pos->colspan, delta--)
		{
			maxColumnWidth[i]++;
		}
	}
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
		vtbl.widgetVtbl.removeChild = tableRemoveChildImpl;
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
	
	// Default alignment is TOP LEFT
	tableSetDefaultAlign(self, TOP, LEFT);
	
	// Padding is 0
	self->rowPadding = self->columnPadding = 0;
}

void tableDestroyImpl(widget *self)
{
	// Destroy child positional info
	vectorMapAndDestroy(TABLE(self)->childPositions, free);
	
	// Call our parents destructor
	widgetDestroyImpl(self);
}

void tableSetDefaultAlign(table *self, hAlign h, vAlign v)
{
	// Set the alignment
	self->hAlignment = h;
	self->vAlignment = v;
}

bool tableSetPadding(table *self, int h, int v)
{
	const int oldColPadding = self->columnPadding;
	const int oldRowPadding = self->rowPadding;
	
	// Set the padding
	self->columnPadding = h;
	self->rowPadding = v;
	
	// If applicable re-layout the window
	if (WIDGET(self)->size.x != -1 && WIDGET(self)->size.y != -1)
	{
		// If laying out the window with the new padding fails
		if (!widgetDoLayout(widgetGetRoot(WIDGET(self))))
		{
			// Restore the old padding
			self->columnPadding = oldColPadding;
			self->rowPadding = oldRowPadding;
			
			widgetDoLayout(widgetGetRoot(WIDGET(self)));
			
			return false;
		}
	}
	
	// Either widgetDoLayout succeeded or we did not need laying out
	return true;
}

int tableGetRowCount(const table *self)
{
	return self->numRows;
}

int tableGetColumnCount(const table *self)
{
	return self->numColumns;
}

widget *tableGetCell(const table *self, int row, int column)
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

void tableRemoveChildImpl(widget *self, widget *child)
{
	// If we are not the childs parent, delegate to widgetRemoveChildImpl
	if (self != child->parent)
	{
		widgetRemoveChildImpl(self, child);
	}
	// We are the childs parent, so there is some bookkeeping to tend to
	else
	{
		int i;
		
		// Find the index of the child
		for (i = 0; i < vectorSize(self->children); i++)
		{
			if (child == vectorAt(self->children, i))
			{
				break;
			}
		}
		
		// Call the child's destructor and remove it
		widgetDestroy(vectorAt(self->children, i));
		vectorRemoveAt(self->children, i);
		
		// Remove the childs positional information
		free(vectorAt(TABLE(self)->childPositions, i));
		vectorRemoveAt(TABLE(self)->childPositions, i);
		
		// Remove any empty rows/columns
		tableRemoveEmptyRows(TABLE(self));
		tableRemoveEmptyColumns(TABLE(self));
		
		// Redo the layout of the window
		if (self->size.x != -1 && self->size.y != -1)
		{
			widgetDoLayout(widgetGetRoot(self));
		}
	}
}

bool tableAddChildDummyImpl(widget *self, widget *child)
{
	assert(!"widgetAddChild can not be called directly on tables; "
		   "please use tableAddChild instead");
	return false;
}

bool tableAddChild(table *self, widget *child, int row, int column)
{
	// Map onto tableAddChildWithSpan with rowspan and colspan as 1
	return tableAddChildWithSpan(self, child, row, 1, column, 1);
}

bool tableAddChildWithSpan(table *self, widget *child,
                           int row, int rowspan,
                           int column, int colspan)
{
	// Map onto tableAddChildWithSpanAlign using the default v and h alignments
	return tableAddChildWithSpanAlign(self, child,
	                                  row, rowspan, self->vAlignment,
	                                  column, colspan, self->hAlignment);
}

bool tableAddChildWithAlign(table *self, widget *child,
                            int row, vAlign v,
                            int column, hAlign h)
{
	// Map onto tablrAddChildWithSpanAlign with rowspan and colspan as 1
	return tableAddChildWithSpanAlign(self, child, row, 1, v, column, 1, h);
}

bool tableAddChildWithSpanAlign(table *self, widget *child,
                                int row, int rowspan, vAlign v,
                                int column, int colspan, hAlign h)
{
	int i, j;
	const int rowCount = tableGetRowCount(self);
	const int columnCount = tableGetColumnCount(self);
	childPositionInfo *pos;
	
	// First, check the row and column is valid
	assert(row > 0 && row <= tableGetRowCount(self) + 1);
	assert(column > 0 && column <= tableGetColumnCount(self) + 1);
	
	// Second, check all of the cells spanned are empty
	for (i = row; i < row + rowspan && i <= rowCount; i++)
	{
		for (j = column; j < column + colspan && j <= columnCount; j++)
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
	
	pos->vAlignment = v;
	pos->hAlignment = h;
	
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

size tableGetMinSizeImpl(widget *self)
{
	size minSize;
	
	const int numRows = tableGetRowCount(TABLE(self));
	const int numColumns = tableGetColumnCount(TABLE(self));
	
	int *minRowHeight = alloca(sizeof(int) * numRows);
	int *minColumnWidth = alloca(sizeof(int) * numColumns);

	tableGetMinimumCellSizes(TABLE(self), minRowHeight, minColumnWidth);
	
	// Sum up the widths and the heights to get the table size
	minSize.x = tablePartialSum(minColumnWidth, 0, numColumns);
	minSize.y = tablePartialSum(minRowHeight, 0, numRows);
	
	return minSize;
}

size tableGetMaxSizeImpl(widget *self)
{
	size maxSize;
	
	const int numRows = tableGetRowCount(TABLE(self));
	const int numColumns = tableGetColumnCount(TABLE(self));
	
	int *maxRowHeight = alloca(sizeof(int) * numRows);
	int *maxColumnWidth = alloca(sizeof(int) * numColumns);
	
	tableGetMaximumCellSizes(TABLE(self), maxRowHeight, maxColumnWidth);
	
	// Sum up the widths and the heights to get the table size
	maxSize.x = tablePartialSum(maxColumnWidth, 0, numColumns);
	maxSize.y = tablePartialSum(maxRowHeight, 0, numRows);
	
	return maxSize;
}

bool tableDoLayoutImpl(widget *self)
{
	table *selfTable = TABLE(self);
	
	const int numChildren = vectorSize(self->children);
	
	const int numRows = tableGetRowCount(selfTable);
	const int numColumns = tableGetColumnCount(selfTable);
	
	int i;
	int dx, dy;
	
	int *minColumnWidth = alloca(sizeof(int) * numColumns);
	int *maxColumnWidth = alloca(sizeof(int) * numColumns);
	
	int *minRowHeight = alloca(sizeof(int) * numRows);
	int *maxRowHeight = alloca(sizeof(int) * numRows);
	
	// Syntatic sugar
	int *columnWidth = minColumnWidth;
	int *rowHeight = minRowHeight;
	
	// Get the minimum and maximum cell sizes
	tableGetMinimumCellSizes(selfTable, minRowHeight, minColumnWidth);
	tableGetMaximumCellSizes(selfTable, maxRowHeight, maxColumnWidth);
	
	// Calculate how much space we have left to fill
	dx = self->size.x - tablePartialSum(minColumnWidth, 0, numColumns);
	dy = self->size.y - tablePartialSum(minRowHeight, 0, numRows);
	
	// Increase the column size until we run out of space
	for (i = 0; dx; i = (i + 1) % numColumns)
	{
		// If the column is not maxed out, increases its size by one
		if (columnWidth[i] < maxColumnWidth[i])
		{
			columnWidth[i]++;
			dx--;
		}
	}
	
	// Increase the row size until we run out of space
	for (i = 0; dy; i = (i + 1) % numRows)
	{
		// If the row is not maxed out, increase its size by one
		if (rowHeight[i] < minRowHeight[i])
		{
			rowHeight[i]++;
			dy--;
		}
	}
	
	// Now we need to position the children, taking padding into account
	for (i = 0; i < numChildren; i++)
	{
		// Get the child and its position info
		widget *child = vectorAt(self->children, i);
		const childPositionInfo *pos = vectorAt(selfTable->childPositions, i);
		size maxChildSize = widgetGetMaxSize(child);
		
		// left is the sum of all of the preceding columns
		int left = tablePartialSum(columnWidth, 0, pos->column);
		
		// top is the sum of all of the preceding rows
		int top = tablePartialSum(rowHeight, 0, pos->row);
		
		// cellWidth is the sum of the columns we span
		int cellWidth = tablePartialSum(columnWidth, pos->column - 1, pos->colspan);
		
		// cellHeight is the sum of the rows we span
		int cellHeight = tablePartialSum(rowHeight, pos->row - 1, pos->rowspan);
		
		// Final width and height of the child
		int w, h;
		
		// Final x,y offsets of the child
		int x, y;
		
		// If we are not the last row/column, subtract the row/column padding
		if (pos->column + pos->colspan - 1 != numColumns)
		{
			cellWidth -= selfTable->columnPadding;
		}
		if (pos->row + pos->rowspan - 1 != numRows)
		{
			cellHeight -= selfTable->rowPadding;
		}
		
		// Compute the final width and height of the child
		w = MIN(cellWidth, maxChildSize.x);
		h = MIN(cellHeight, maxChildSize.y);
		
		// Pad out any remaining space
		switch (pos->hAlignment)
		{
			case LEFT:
				x = left;
				break;
			case CENTRE:
				x = left + (cellWidth - w) / 2;
				break;
			case RIGHT:
				x = left + cellWidth - w;
				break;
		}
		
		switch (pos->vAlignment)
		{
			case TOP:
				y = top;
				break;
			case MIDDLE:
				y = top + (cellHeight - h) / 2;
				break;
			case BOTTOM:
				y = top + cellHeight - h;
				break;
		}
		
		// Resize and reposition the widget
		widgetResize(child, w, h);
		widgetReposition(child, x, y);
	}
	
	return true;
}

void tableDoDrawImpl(widget *self)
{
	// NO-OP
	(void) self;
}
