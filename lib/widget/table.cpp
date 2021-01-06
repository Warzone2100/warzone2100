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
 * Functions for scrollable table.
 */

#include "table.h"
#include "form.h"
#include "widgbase.h"
#include "lib/framework/input.h"
#include "lib/ivis_opengl/pieblitfunc.h"

#include <algorithm>
#include <numeric>

#define TABLE_ROW_PADDING 5
#define TABLE_COL_PADDING 10
#define TABLE_COL_HEADER_HEIGHT	(20 + (TABLE_ROW_PADDING * 2))

// MARK: - TableRow

TableRow::TableRow()
: W_BUTTON()
{
	AudioCallback = nullptr;
}

std::shared_ptr<TableRow> TableRow::make(const std::vector<std::shared_ptr<WIDGET>>& _columnWidgets, int rowHeight /*= 0*/)
{
	class make_shared_enabler: public TableRow {};
	auto result = std::make_shared<make_shared_enabler>();

	result->columnWidgets = _columnWidgets;

	if (rowHeight <= 0)
	{
		// expand row height to match maximum height of any contained widget
		rowHeight = 0;
		for (auto& widget : result->columnWidgets)
		{
			rowHeight = std::max(rowHeight, widget->height());
		}
	}
	else
	{
		// use fixed row height
		for (auto& widget : result->columnWidgets)
		{
			widget->setGeometry(widget->x(), widget->y(), widget->width(), rowHeight);
		}
	}
	result->setGeometry(result->x(), result->y(), result->width(), rowHeight);

	for (auto& widget : result->columnWidgets)
	{
		result->attach(widget);
	}

	return result;
}

void TableRow::setHighlightsOnMouseOver(bool value)
{
	highlightsOnMouseOver = value;
}

void TableRow::display(int xOffset, int yOffset)
{
	if (!highlightsOnMouseOver || !isMouseOverRowOrChildren()) { return; }
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();
	iV_TransBoxFill(x0, y0, x1, y1);
}

bool TableRow::processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	// if processClickRecursive was called for this row, it means the mouse is over it
	// (see WIDGET::processClickRecursive)
	lastFrameMouseIsOverRowOrChildren = frameGetFrameNumber();

	return WIDGET::processClickRecursive(psContext, key, wasPressed);
}

bool TableRow::isMouseOverRowOrChildren() const
{
	if (!lastFrameMouseIsOverRowOrChildren.has_value())
	{
		return false;
	}
	return lastFrameMouseIsOverRowOrChildren.value() == frameGetFrameNumber();
}

void TableRow::resizeColumns(const std::vector<size_t>& newColumnWidths, int columnPadding)
{
	columnPadding = std::max<int>(columnPadding, 0);
	int lastColumnEndX = -columnPadding;
	size_t colIdx = 0;
	for (; colIdx < std::min(newColumnWidths.size(), columnWidgets.size()); colIdx++)
	{
		columnWidgets[colIdx]->setGeometry(lastColumnEndX + columnPadding + columnPadding, columnWidgets[colIdx]->y(), static_cast<int>(newColumnWidths[colIdx]), columnWidgets[colIdx]->height());
		columnWidgets[colIdx]->show();
		lastColumnEndX = (columnWidgets[colIdx]->x() + columnWidgets[colIdx]->width());
	}
	// hide any additional column widgets
	for (; colIdx < columnWidgets.size(); colIdx++)
	{
		columnWidgets[colIdx]->hide();
	}
}

// MARK: - TableHeader

class TableHeader : public W_FORM
{
public:
	TableHeader(const std::shared_ptr<ScrollableTableWidget>& parentTable);
	virtual ~TableHeader() {}

protected:
	virtual void display(int xOffset, int yOffset) override;
	virtual void clicked(W_CONTEXT *, WIDGET_KEY) override;
	virtual void released(W_CONTEXT *, WIDGET_KEY) override;
	virtual void run(W_CONTEXT *psContext) override;
	virtual void geometryChanged() override;

public:
	void addColumn(const TableColumn& column);
	void changeColumnWidths(const std::vector<size_t>& newColumnWidths);
	bool isUserDraggingColumnHeader() const;

private:
	int columnWidgetHeight() const;

private:
	bool userResizableHeaders = true;
	std::vector<TableColumn> columns;
	optional<size_t> colBeingResized = nullopt;
	optional<Vector2i> dragStart = nullopt;
	std::weak_ptr<ScrollableTableWidget> parentTable;
};

TableHeader::TableHeader(const std::shared_ptr<ScrollableTableWidget>& _parentTable)
	: W_FORM()
{
	ASSERT(_parentTable != nullptr, "Null parent table");
	parentTable = _parentTable;
}

void TableHeader::addColumn(const TableColumn& column)
{
	attach(column.columnWidget);
	// Position it to the right of the prior column (child)
	int columnX = TABLE_COL_PADDING;
	int colHeaderY = 0;
	if (columns.size() > 0)
	{
		columnX += columns.back().columnWidget->x() + columns.back().columnWidget->width() + TABLE_COL_PADDING;
	}
	column.columnWidget->setGeometry(columnX, colHeaderY, column.columnWidget->width(), columnWidgetHeight());

	columns.push_back(column);
}

void TableHeader::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height() - 1;
	iV_TransBoxFill(x0, y0, x1, y1);
	iV_Line(x0, y1, x1, y1, WZCOL_MENU_SEPARATOR);

	// draw a line between every column widget
	for (auto& child : columns)
	{
		// column lines
		int xPos = x0 + child.columnWidget->x() + child.columnWidget->width() + TABLE_COL_PADDING;
		iV_Line(xPos, y0 + 5, xPos, y1 - 5, WZCOL_MENU_SEPARATOR);
	}
}

void TableHeader::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	if (userResizableHeaders && key == WKEY_PRIMARY && !columns.empty())
	{
		// determine the column that is being resized
		colBeingResized = nullopt;
		for (size_t colIdx = columns.size() - 1; /* termination handled in loop body */; colIdx--)
		{
			auto& columnWidget = columns[colIdx].columnWidget;
			int col_x1 = columnWidget->x() + columnWidget->width();
			if (psContext->mx > col_x1)
			{
				if (columns[colIdx].resizeBehavior != TableColumn::ResizeBehavior::FIXED_WIDTH)
				{
					colBeingResized = colIdx;
				}
				break;
			}
			if (colIdx == 0)
			{
				break;
			}
		}
		if (!colBeingResized.has_value()) { return; }
		dragStart = Vector2i(psContext->mx, psContext->my);
	}
}

void TableHeader::released(W_CONTEXT *, WIDGET_KEY)
{
	if (!userResizableHeaders || !dragStart.has_value()) { return; }
	colBeingResized = nullopt;
	dragStart = nullopt;
}

bool TableHeader::isUserDraggingColumnHeader() const
{
	return userResizableHeaders && dragStart.has_value();
}

void TableHeader::run(W_CONTEXT *psContext)
{
	if (!userResizableHeaders || !dragStart.has_value()) { return; }

	/* If the mouse is released *anywhere*, stop dragging */
	if (mouseReleased(MOUSE_LMB))
	{
		colBeingResized = nullopt;
		dragStart = nullopt;
		return;
	}

	Vector2i currentMousePos(psContext->mx, psContext->my);
	if (currentMousePos == dragStart.value()) { return; }
	Vector2i dragDelta(currentMousePos.x - dragStart.value().x, currentMousePos.y - dragStart.value().y);

	int priorColumnWidth = columns[colBeingResized.value()].columnWidget->width();
	size_t newProposedColumnWidth = static_cast<size_t>(std::max<int>(priorColumnWidth + dragDelta.x, 0));
	if (auto table = parentTable.lock())
	{
		auto result = table->header_changeColumnWidth(colBeingResized.value(), static_cast<size_t>(newProposedColumnWidth));
		if (result.has_value())
		{
			Vector2i currentDragLogicalPos = dragStart.value();
			currentDragLogicalPos.x += (static_cast<int>(result.value()) - priorColumnWidth);
			dragStart = currentDragLogicalPos;
		}
	}
}

void TableHeader::geometryChanged()
{
	// resize column header widgets height based on parent TableHeader height
	for(auto& column : columns)
	{
		auto& columnWidget = column.columnWidget;
		columnWidget->setGeometry(columnWidget->x(), columnWidget->y(), columnWidget->width(), columnWidgetHeight());
	}
}

int TableHeader::columnWidgetHeight() const
{
	return height() - 2;
}

void TableHeader::changeColumnWidths(const std::vector<size_t>& newColumnWidths)
{
	int columnX = TABLE_COL_PADDING;
	size_t i = 0;
	for(; i < std::min(newColumnWidths.size(), columns.size()); i++)
	{
		auto& columnWidget = columns[i].columnWidget;
		columnWidget->setGeometry(columnX, columnWidget->y(), static_cast<int>(newColumnWidths[i]), columnWidget->height());
		columnX += TABLE_COL_PADDING + columnWidget->width() + TABLE_COL_PADDING;
	}
	for(; i < columns.size(); i++)
	{
		auto& columnWidget = columns[i].columnWidget;
		columnWidget->setGeometry(columnX, columnWidget->y(), columnWidget->width(), columnWidget->height());
		columnX += TABLE_COL_PADDING + columnWidget->width() + TABLE_COL_PADDING;
	}
}

// MARK: - ScrollableTableWidget

ScrollableTableWidget::ScrollableTableWidget()
: WIDGET()
{ }

std::shared_ptr<ScrollableTableWidget> ScrollableTableWidget::make(const std::vector<TableColumn>& columns, int headerHeight /*= -1*/)
{
	class make_shared_enabler: public ScrollableTableWidget {};
	auto result = std::make_shared<make_shared_enabler>();

	// Add table header row
	if (headerHeight <= 0)
	{
		headerHeight = TABLE_COL_HEADER_HEIGHT;
	}
	result->header = std::make_shared<TableHeader>(result);
	result->attach(result->header);
	result->header->setGeometry(0, 0, result->width(), headerHeight); // initialize height
	result->header->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		if (auto psParent = psWidget->parent())
		{
			// set width to parent width
			psWidget->setGeometry(0, 0, psParent->width(), psWidget->height());
		}
	}));

	// Add columns
	result->tableColumns = columns;
	for (auto& column : result->tableColumns)
	{
		result->header->addColumn(column);
		result->columnWidths.push_back(column.columnWidget->width());
	}
	result->minColumnWidths.resize(result->tableColumns.size(), 0);

	// Create and add scrollableList
	result->scrollableList = ScrollableListWidget::make();
	result->attach(result->scrollableList);
	result->scrollableList->setBackgroundColor(WZCOL_TRANSPARENT_BOX);
	// Position scrollableList below column headers
	result->scrollableList->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		if (auto psParent = std::dynamic_pointer_cast<ScrollableTableWidget>(psWidget->parent()))
		{
			int y0 = psParent->header->y() + psParent->header->height();
			psWidget->setGeometry(0, y0, psParent->width(), psParent->height() - y0);
		}
	}));

	return result;
}

void ScrollableTableWidget::geometryChanged()
{
	// Trigger calcLayout on children
	header->callCalcLayout();
	scrollableList->callCalcLayout();
}

void ScrollableTableWidget::setBackgroundColor(PIELIGHT const &color)
{
	scrollableList->setBackgroundColor(color);
}

uint16_t ScrollableTableWidget::getScrollPosition() const
{
	return scrollableList->getScrollPosition();
}

void ScrollableTableWidget::setScrollPosition(uint16_t newPosition)
{
	scrollableList->setScrollPosition(newPosition);
}

void ScrollableTableWidget::addRow(const std::shared_ptr<TableRow> &row)
{
	ASSERT_OR_RETURN(, row != nullptr, "row is null");
	ASSERT_OR_RETURN(, row->numColumns() == columnWidths.size(), "Unexpected number of columns in row: %zu", row->numColumns());
	row->resizeColumns(columnWidths, TABLE_COL_PADDING);
	// Add row to internal ScrollableListWidget, and rows list
	scrollableList->addItem(row);
	rows.push_back(row);
}

void ScrollableTableWidget::clearRows()
{
	scrollableList->clear();
	rows.clear();
}

bool ScrollableTableWidget::changeColumnWidths(const std::vector<size_t>& newColumnWidths, bool overrideUserColumnResizing /*= false*/)
{
	ASSERT_OR_RETURN(false, newColumnWidths.size() == columnWidths.size(), "newColumnWidths.size (%zu) does not match existing number of columns (%zu)", newColumnWidths.size(), columnWidths.size());
	if (userDidResizeColumnWidths && !overrideUserColumnResizing)
	{
		return false;
	}
	if (!relayoutColumns(newColumnWidths))
	{
		debug(LOG_WZ, "The proposed column widths are not possible to achieve given the table width and layout constraints.");
		return false;
	}
	return true;
}

optional<size_t> ScrollableTableWidget::changeColumnWidth(size_t col, size_t newColumnWidth, bool overrideUserColumnResizing /*= false*/)
{
	ASSERT_OR_RETURN(nullopt, col < columnWidths.size(), "Invalid column index: %zu", col);
	if (userDidResizeColumnWidths && !overrideUserColumnResizing)
	{
		return nullopt;
	}
	auto newProposedColumnWidths = columnWidths;
	newProposedColumnWidths[col] = newColumnWidth;
	if (!relayoutColumns(newProposedColumnWidths, {col}))
	{
		return nullopt;
	}
	return columnWidths[col];
}

// Called specifically from the header to inform the Table that the user has resized the columns
optional<size_t> ScrollableTableWidget::header_changeColumnWidth(size_t col, size_t newColumnWidth)
{
	auto result = changeColumnWidth(col, newColumnWidth, true);
	userDidResizeColumnWidths = true;
	return result;
}

void ScrollableTableWidget::setMinimumColumnWidths(const std::vector<size_t>& newMinColumnWidths)
{
	ASSERT_OR_RETURN(, newMinColumnWidths.size() == columnWidths.size(), "newMinColumnWidths.size (%zu) does not match existing number of columns (%zu)", newMinColumnWidths.size(), columnWidths.size());
	minColumnWidths = newMinColumnWidths;
	relayoutColumns(columnWidths);
}

void ScrollableTableWidget::setMinimumColumnWidth(size_t col, size_t newMinColumnWidth)
{
	ASSERT_OR_RETURN(, col < minColumnWidths.size(), "Invalid column index: %zu", col);
	minColumnWidths[col] = newMinColumnWidth;
	relayoutColumns(columnWidths);
}

std::vector<size_t> ScrollableTableWidget::getShrinkableColumnIndexes(const std::vector<size_t>& currentColumnWidths)
{
	return getColumnIndexes([this, currentColumnWidths](size_t i, const TableColumn& col) {
		if (col.resizeBehavior == TableColumn::ResizeBehavior::FIXED_WIDTH)
		{
			return false;
		}
		if ((std::min(currentColumnWidths.size(), minColumnWidths.size()) > i) && (currentColumnWidths[i] <= minColumnWidths[i]))
		{
			return false;
		}
		return true;
	});
}

std::vector<size_t> ScrollableTableWidget::getExpandToFillColumnIndexes()
{
	return getColumnIndexes([](size_t, const TableColumn& col) {
		return (col.resizeBehavior == TableColumn::ResizeBehavior::RESIZABLE_AUTOEXPAND);
	});
}

bool ScrollableTableWidget::relayoutColumns(std::vector<size_t> proposedColumnWidths, const std::unordered_set<size_t>& priorityIndexes)
{
	// respect any minimum column widths (first pass)
	std::vector<size_t> colIndexesIncreasedToMinimumSize;
	for (size_t i = 0; i < std::min(proposedColumnWidths.size(), minColumnWidths.size()); i++)
	{
		if (proposedColumnWidths[i] < minColumnWidths[i])
		{
			proposedColumnWidths[i] = minColumnWidths[i];
			colIndexesIncreasedToMinimumSize.push_back(i);
		}
	}

	auto maxColumnWidthAvailable = getMaxColumnTotalWidth(proposedColumnWidths.size());
	auto totalColWidth = std::accumulate(proposedColumnWidths.begin(), proposedColumnWidths.end(), decltype(proposedColumnWidths)::value_type(0));
	if (totalColWidth > maxColumnWidthAvailable)
	{
		// need to shrink all shrinkable columns to fit
		size_t extraWidth = totalColWidth - maxColumnWidthAvailable;
		std::vector<size_t> allShrinkableColumnIndexes = getShrinkableColumnIndexes(proposedColumnWidths);
		std::vector<size_t> shrinkableColumnIndexes = allShrinkableColumnIndexes;
		shrinkableColumnIndexes.erase(
			std::remove_if(shrinkableColumnIndexes.begin(), shrinkableColumnIndexes.end(),
							[priorityIndexes](const size_t &colIdx) { return priorityIndexes.count(colIdx) > 0; }),
			shrinkableColumnIndexes.end());
		bool ignoringPriorityIndexes = false;
		if (shrinkableColumnIndexes.empty())
		{
			// ignore "priorityIndexes"
			shrinkableColumnIndexes = allShrinkableColumnIndexes;
			ignoringPriorityIndexes = true;
		}
		if (shrinkableColumnIndexes.empty())
		{
			ASSERT(!shrinkableColumnIndexes.empty(), "All columns are fixed width (or non-shrinkable) but desired sizes exceed maxColumnWidthAvailable: %zu", maxColumnWidthAvailable);
			return false;
		}
		auto shrinkColumnWidths = [this, &proposedColumnWidths](const std::vector<size_t>& shrinkableColumnIndexes, size_t extraWidth) -> size_t {
			size_t currentTotalColumnWidthReduction = 0;
			for (size_t i = 0; i < shrinkableColumnIndexes.size(); i++)
			{
				size_t colIdx = shrinkableColumnIndexes[i];
				size_t remainingExtraWidth = extraWidth - currentTotalColumnWidthReduction;
				size_t widthReductionAmount = (i < shrinkableColumnIndexes.size()-1) ? (remainingExtraWidth / (shrinkableColumnIndexes.size() - i)) : (remainingExtraWidth);
				size_t maxColumnWidthReduction = proposedColumnWidths[colIdx];
				if (minColumnWidths.size() > colIdx && proposedColumnWidths[colIdx] > minColumnWidths[colIdx])
				{
					maxColumnWidthReduction = proposedColumnWidths[colIdx] - minColumnWidths[colIdx];
				}
				widthReductionAmount = std::min(widthReductionAmount, maxColumnWidthReduction);
				proposedColumnWidths[colIdx] -= widthReductionAmount;
				currentTotalColumnWidthReduction += widthReductionAmount;
			}
			return currentTotalColumnWidthReduction;
		};
		size_t currentTotalColumnWidthReduction = shrinkColumnWidths(shrinkableColumnIndexes, extraWidth);
		if (currentTotalColumnWidthReduction < extraWidth)
		{
			if (!ignoringPriorityIndexes && !priorityIndexes.empty())
			{
				// One more thing we can do - try to shrink the priority column indexes
				shrinkableColumnIndexes.clear();
				shrinkableColumnIndexes.insert(shrinkableColumnIndexes.end(), priorityIndexes.begin(), priorityIndexes.end());
				currentTotalColumnWidthReduction += shrinkColumnWidths(shrinkableColumnIndexes, (extraWidth - currentTotalColumnWidthReduction));
			}
			if (currentTotalColumnWidthReduction < extraWidth)
			{
				// Unable to shrink other columns - reject this proposed set of column widths
				return false;
			}
		}
		ASSERT(currentTotalColumnWidthReduction == extraWidth, "Logic error");
		totalColWidth = std::accumulate(proposedColumnWidths.begin(), proposedColumnWidths.end(), decltype(proposedColumnWidths)::value_type(0));
	}
	if (totalColWidth < maxColumnWidthAvailable)
	{
		// expand any "expand to fill" columns to proportionally take up the extra space
		auto extraWidth = maxColumnWidthAvailable - totalColWidth;
		std::vector<size_t> expandToFillColumnIndexes = getExpandToFillColumnIndexes();
		size_t currentTotalColumnWidthIncrease = 0;
		for (size_t i = 0; i < expandToFillColumnIndexes.size(); i++)
		{
			size_t colIdx = expandToFillColumnIndexes[i];
			size_t widthIncreaseAmount = (i < expandToFillColumnIndexes.size()-1) ? (extraWidth / expandToFillColumnIndexes.size()) : (extraWidth - currentTotalColumnWidthIncrease);
			proposedColumnWidths[colIdx] += widthIncreaseAmount;
			currentTotalColumnWidthIncrease += widthIncreaseAmount;
		}
	}
	totalColWidth = std::accumulate(proposedColumnWidths.begin(), proposedColumnWidths.end(), decltype(proposedColumnWidths)::value_type(0));

	// store proposed column widths
	columnWidths = proposedColumnWidths;

	// actually resize header and row columns
	header->changeColumnWidths(columnWidths);
	for (auto& row : rows)
	{
		row->resizeColumns(columnWidths, TABLE_COL_PADDING);
	}

	return true;
}

size_t ScrollableTableWidget::getMaxColumnTotalWidth(size_t numColumns) const
{
	// TABLE_COL_PADDING is on all sides of all columns
	// also factor in scrollbar width
	size_t totalPaddingWidth = TABLE_COL_PADDING + ((numColumns - 1) * 2 * TABLE_COL_PADDING) + TABLE_COL_PADDING + scrollableList->getScrollbarWidth();
	return static_cast<size_t>(width()) - totalPaddingWidth;
}

bool ScrollableTableWidget::isUserDraggingColumnHeader() const
{
	return header->isUserDraggingColumnHeader();
}
