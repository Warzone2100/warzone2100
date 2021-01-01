/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2021  Warzone 2100 Project

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
/**
 * @file
 * Definitions for scrollable table functions.
 */

#ifndef __INCLUDED_LIB_WIDGET_TABLE_H__
#define __INCLUDED_LIB_WIDGET_TABLE_H__

#include "widget.h"
#include "button.h"
#include "scrollablelist.h"

#include <unordered_set>

class TableRow: public W_BUTTON
{
protected:
	TableRow();
public:
	// Make a new TableRow, given a vector of widgets (for the columns)
	// If rowHeight <= 0, expand the TableRow height to match maximum height of any contained (column) widget
	// If rowHeight > 0, use it as a fixed row height, and set all contained (column) widgets to that height
	//
	// NOTE: A column widget should expect its width to be adjustable, unless it is associated with a TableColumn
	//       (in its parent ScrollableTableWidget) that has ResizeBehavior::FIXED_WIDTH.
	static std::shared_ptr<TableRow> make(const std::vector<std::shared_ptr<WIDGET>>& _columnWidgets, int rowHeight = 0);

	// Set whether the row background highlights on mouse-over
	void setHighlightsOnMouseOver(bool value);
protected:
	virtual void display(int, int) override;
public:
	virtual bool processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;
protected:
	friend class ScrollableTableWidget;
	virtual void resizeColumns(const std::vector<size_t>& columnWidths, int columnPadding);
	inline size_t numColumns() const { return columnWidgets.size(); }
private:
	bool isMouseOverRowOrChildren() const;
private:
	bool highlightsOnMouseOver = false;
	std::vector<std::shared_ptr<WIDGET>> columnWidgets;
	optional<UDWORD> lastFrameMouseIsOverRowOrChildren = nullopt;
};

class TableHeader; // forward-declare

struct TableColumn
{
	enum class ResizeBehavior
	{
		RESIZABLE,				// user-resizable column
		RESIZABLE_AUTOEXPAND,	// user-resizable column, that also automatically expands to fill extra space
		FIXED_WIDTH
	};

	std::shared_ptr<WIDGET> columnWidget;
	ResizeBehavior resizeBehavior;
};

class ScrollableTableWidget: public WIDGET
{
protected:
	ScrollableTableWidget();

public:
	// Make a new ScrollableTableWidget
	//
	// Note: The TableColumn.columnWidget widgets are expected to have pre-set widths, which are used to
	//       initialize the ScrollableTableWidget's columnWidths.
	static std::shared_ptr<ScrollableTableWidget> make(const std::vector<TableColumn>& columns, int headerHeight = -1);

	// Add a new table row
	// See: ``TableRow``
	void addRow(const std::shared_ptr<TableRow> &row);
	void clearRows();

	// Get the maximum width that can be used by the column widths passed to changeColumnWidths, based on the current widget size (minus padding)
	size_t getMaxColumnTotalWidth(size_t numColumns) const;

	// Get the current column widths
	const std::vector<size_t>& getColumnWidths() const { return columnWidths; }

	// Propose a new set of column widths
	// The ScrollableTableWidget will automatically adjust the submitted widths for various constraints
	// (like the minimum column widths, total maximum width, which columns can be resized / expanded-to-fill-space, fixed width sizes, etc)
	// It is expected that newColumnWidths.size() will be equal to the number of columns in the table.
	bool changeColumnWidths(const std::vector<size_t>& newColumnWidths);

	// Change a single column width
	// The ScrollableTableWidget will preferentially re-layout / flow the other column widths according to the table's column constraints.
	// However, it will fallback to adjusting the newColumnWidth for col if necessary to satisfy all constraints.
	// Returns:
	// - `nullopt` on failure (if there was no way to change the specified column to the newColumnWidth and layout the other columns)
	// - the actual newColumnWidth, on success, which may differ from the input newColumnWidth if required to satisfy layout constraints
	optional<size_t> changeColumnWidth(size_t col, size_t newColumnWidth);

	// Get the current minimum column width layout constraints
	const std::vector<size_t>& getMinimumColumnWidths() const { return minColumnWidths; }

	// Add minimum column width layout constraints for all columns
	void setMinimumColumnWidths(const std::vector<size_t>& newMinColumnWidths);

	// Add or update a minimum column width layout constraint for a single column
	// Note: This triggers a relayout, so if you are setting constraints on multiple columns you should use:
	// setMinimumColumnWidths(const std::vector<size_t>& newMinColumnWidths)
	void setMinimumColumnWidth(size_t col, size_t newMinColumnWidth);

	// Change the table background color
	void setBackgroundColor(PIELIGHT const &color);

protected:
	virtual void geometryChanged() override;

private:
	bool relayoutColumns(std::vector<size_t> proposedColumnWidths, const std::unordered_set<size_t>& priorityIndexes = {});
	std::vector<size_t> getShrinkableColumnIndexes(const std::vector<size_t>& currentColumnWidths);
	std::vector<size_t> getExpandToFillColumnIndexes();
	inline std::vector<size_t> getColumnIndexes(const std::function<bool (size_t idx, const TableColumn&)>& func)
	{
		std::vector<size_t> columnIndexes;
		for (size_t i = 0; i < tableColumns.size(); i++)
		{
			if (func(i, tableColumns[i]))
			{
				columnIndexes.push_back(i);
			}
		}
		return columnIndexes;
	}

private:
	std::vector<TableColumn> tableColumns;
	std::vector<size_t> columnWidths;
	std::vector<size_t> minColumnWidths;

	std::shared_ptr<TableHeader> header;
	std::shared_ptr<ScrollableListWidget> scrollableList;

	std::vector<std::shared_ptr<TableRow>> rows;
};

#endif // __INCLUDED_LIB_WIDGET_TABLE_H__
