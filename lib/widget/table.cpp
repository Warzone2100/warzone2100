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
, highlightColor(WZCOL_TRANSPARENT_BOX)
, disabledColor(WZCOL_FORM_DISABLE)
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
		// vertically center any column widgets that are less than the rowHeight
		for (auto& widget : result->columnWidgets)
		{
			if (widget->height() < rowHeight)
			{
				widget->setGeometry(widget->x(), (rowHeight - widget->height()) / 2, widget->width(), widget->height());
			}
		}
	}
	else
	{
		// use fixed row height
		for (auto& widget : result->columnWidgets)
		{
			widget->setGeometry(widget->x(), 0, widget->width(), rowHeight);
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

void TableRow::setHighlightColor(PIELIGHT newHighlightColor)
{
	highlightColor = newHighlightColor;
}

void TableRow::setDrawBorder(optional<PIELIGHT> newBorderColor)
{
	borderColor = newBorderColor;
}

void TableRow::setBackgroundColor(optional<PIELIGHT> newBackgroundColor)
{
	backgroundColor = newBackgroundColor;
}

// Set whether row is "disabled"
void TableRow::setDisabled(bool disabled)
{
	disabledRow = disabled;
	W_BUTTON::setState((disabled) ? WBUT_DISABLE : 0);
}

// Set row disable overlay color
void TableRow::setDisabledColor(PIELIGHT newDisabledColor)
{
	disabledColor = newDisabledColor;
}

int32_t TableRow::getColumnTotalContentIdealWidth()
{
	int32_t totalContentIdealWidth = 0;
	for (auto& column : columnWidgets)
	{
		totalContentIdealWidth += column->idealWidth();
	}
	return totalContentIdealWidth;
}

void TableRow::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	if (highlightsOnMouseOver && isMouseOverRowOrChildren() && !disabledRow)
	{
		pie_UniTransBoxFill(x0, y0, x1, y1, highlightColor);
	}
	else if (backgroundColor.has_value())
	{
		pie_UniTransBoxFill(x0, y0, x1, y1, backgroundColor.value());
	}

	if (borderColor.has_value())
	{
		iV_Box(x0, y0, x1, y1, borderColor.value());
	}
}

void TableRow::displayRecursive(WidgetGraphicsContext const& context)
{
	WIDGET::displayRecursive(context);

	// *after* displaying all children, draw the disabled overlay (if needed)
	if (!disabledRow)
	{
		return;
	}

	if (!context.clipContains(geometry())) {
		return;
	}

	int xOffset = context.getXOffset();
	int yOffset = context.getYOffset();

	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), disabledColor);
}

bool TableRow::hitTest(int x, int y) const
{
	if (x >= maxDisplayedColumnX1) { return false; }
	return W_BUTTON::hitTest(x, y);
}

std::shared_ptr<WIDGET> TableRow::findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	// if findMouseTargetRecursive was called for this row, it means the mouse is over it
	// (see WIDGET::findMouseTargetRecursive)
	lastFrameMouseIsOverRowOrChildren = frameGetFrameNumber();


	if (disabledRow)
	{
		return shared_from_this();
	}
	return WIDGET::findMouseTargetRecursive(psContext, key, wasPressed);
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
	maxDisplayedColumnX1 = lastColumnEndX;
	// hide any additional column widgets
	for (; colIdx < columnWidgets.size(); colIdx++)
	{
		columnWidgets[colIdx]->hide();
	}
}

std::shared_ptr<WIDGET> TableRow::getWidgetAtColumn(size_t col) const
{
	ASSERT_OR_RETURN(nullptr, col < columnWidgets.size(), "Invalid column index: %zu", col);
	return columnWidgets[col];
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
	virtual bool capturesMouseDrag(WIDGET_KEY) override;
	virtual void mouseDragged(WIDGET_KEY, W_CONTEXT *start, W_CONTEXT *current) override;

public:
	void addColumn(const TableColumn& column);
	void changeColumnWidths(const std::vector<size_t>& newColumnWidths);
	bool isUserDraggingColumnHeader() const;
	void setColumnPadding(Vector2i padding);

private:
	int columnWidgetHeight() const;

private:
	bool userResizableHeaders = true;
	Vector2i columnPadding;
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
	columnPadding = _parentTable->getColumnPadding();
}

void TableHeader::addColumn(const TableColumn& column)
{
	attach(column.columnWidget);
	// Position it to the right of the prior column (child)
	int columnX = columnPadding.x;
	int colHeaderY = 0;
	if (columns.size() > 0)
	{
		columnX += columns.back().columnWidget->x() + columns.back().columnWidget->width() + columnPadding.x;
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
		int xPos = x0 + child.columnWidget->x() + child.columnWidget->width() + columnPadding.x;
		iV_Line(xPos, y0 + 5, xPos, y1 - 5, WZCOL_MENU_SEPARATOR);
	}
}

bool TableHeader::capturesMouseDrag(WIDGET_KEY)
{
	return userResizableHeaders;
}

void TableHeader::mouseDragged(WIDGET_KEY key, W_CONTEXT *start, W_CONTEXT *current)
{
	if (!userResizableHeaders || key != WKEY_PRIMARY || columns.empty())
	{
		return;
	}

	if (!dragStart.has_value())
	{
		dragStart = Vector2i(start->mx, start->my);

		// determine the column that is being resized
		colBeingResized = nullopt;
		for (size_t colIdx = columns.size() - 1; /* termination handled in loop body */; colIdx--)
		{
			auto& columnWidget = columns[colIdx].columnWidget;
			int col_x1 = columnWidget->x() + columnWidget->width();
			if (start->mx > col_x1)
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
	}

	if (dragStart.has_value() && colBeingResized.has_value())
	{
		// handle column resize while dragging
		Vector2i currentMousePos(current->mx, current->my);
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
}

void TableHeader::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	// currently, no-op
}

void TableHeader::released(W_CONTEXT *, WIDGET_KEY)
{
	colBeingResized = nullopt;
	dragStart = nullopt;
}

bool TableHeader::isUserDraggingColumnHeader() const
{
	return userResizableHeaders && dragStart.has_value();
}

void TableHeader::setColumnPadding(Vector2i padding)
{
	columnPadding = padding;

	// Reposition all column widgets
	int lastColumnX = columnPadding.x;
	for (size_t columnIdx = 0; columnIdx < columns.size(); ++columnIdx)
	{
		// Position it to the right of the prior column (child)
		int columnX = lastColumnX;
		int colHeaderY = 0;
		if (columnIdx > 0)
		{
			columnX += columns[columnIdx-1].columnWidget->x() + columns[columnIdx-1].columnWidget->width() + columnPadding.x;
		}
		columns[columnIdx].columnWidget->setGeometry(columnX, colHeaderY, columns[columnIdx].columnWidget->width(), columnWidgetHeight());
	}
}

void TableHeader::run(W_CONTEXT *psContext)
{
	// currently, no-op
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
	int columnX = columnPadding.x;
	size_t i = 0;
	for(; i < std::min(newColumnWidths.size(), columns.size()); i++)
	{
		auto& columnWidget = columns[i].columnWidget;
		columnWidget->setGeometry(columnX, columnWidget->y(), static_cast<int>(newColumnWidths[i]), columnWidget->height());
		columnX += columnPadding.x + columnWidget->width() + columnPadding.x;
	}
	for(; i < columns.size(); i++)
	{
		auto& columnWidget = columns[i].columnWidget;
		columnWidget->setGeometry(columnX, columnWidget->y(), columnWidget->width(), columnWidget->height());
		columnX += columnPadding.x + columnWidget->width() + columnPadding.x;
	}
}

// MARK: - ScrollableTableWidget

ScrollableTableWidget::ScrollableTableWidget()
: WIDGET()
, columnPadding(TABLE_COL_PADDING, 0)
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
			int y0 = 0;
			if (psParent->header->visible())
			{
				y0 = psParent->header->y() + psParent->header->height() + psParent->scrollableList->getItemSpacing();
			}
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

int32_t ScrollableTableWidget::idealHeight()
{
	ASSERT_OR_RETURN(0, scrollableList != nullptr, "scrollableList not yet initialized?");
	return scrollableList->y() + scrollableList->idealHeight();
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
	row->resizeColumns(columnWidths, columnPadding.x);
	// Add row to internal ScrollableListWidget, and rows list
	scrollableList->addItem(row);
	rows.push_back(row);
}

void ScrollableTableWidget::clearRows()
{
	scrollableList->clear();
	rows.clear();
}

void ScrollableTableWidget::setHeaderVisible(bool visible)
{
	if (visible == header->visible())
	{
		return;
	}
	header->show(visible);
	geometryChanged();
}

void ScrollableTableWidget::setRowDisabled(size_t row, bool disabled)
{
	ASSERT_OR_RETURN(, row < rows.size(), "Invalid row idx: %zu", row);
	rows[row]->setDisabled(disabled);
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

// Get the maximum idealWidth() returned by any of the row widgets in the specified column
int32_t ScrollableTableWidget::getColumnMaxContentIdealWidth(size_t col)
{
	int32_t maxIdealWidth = 0;
	for (auto& row : rows)
	{
		auto pColWidget = row->getWidgetAtColumn(col);
		if (!pColWidget) { continue; }
		maxIdealWidth = std::max(maxIdealWidth, pColWidget->idealWidth());
	}
	return maxIdealWidth;
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

void ScrollableTableWidget::updateColumnWidths()
{
	// actually resize header and row columns
	header->changeColumnWidths(columnWidths);
	for (auto& row : rows)
	{
		row->resizeColumns(columnWidths, columnPadding.x);
	}
}

bool ScrollableTableWidget::relayoutColumns(std::vector<size_t> proposedColumnWidths, std::unordered_set<size_t> priorityIndexes)
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
		auto shrinkColumnWidths = [this, &proposedColumnWidths](std::vector<size_t> shrinkableColumnIndexes, size_t extraWidth) -> size_t {
			size_t currentTotalColumnWidthReduction = 0;
			std::vector<size_t> stillShrinkableColumnIndexes;
			// order shrinkableColumnIndexes by maxColumnWidthReduction for associated column
			std::sort(shrinkableColumnIndexes.begin(), shrinkableColumnIndexes.end(), [this, &proposedColumnWidths](size_t colIndexA, size_t coldIndexB) -> bool {
				size_t maxColumnWidthReductionA = proposedColumnWidths[colIndexA];
				if (minColumnWidths.size() > colIndexA)
				{
					maxColumnWidthReductionA = (proposedColumnWidths[colIndexA] > minColumnWidths[colIndexA]) ? (proposedColumnWidths[colIndexA] - minColumnWidths[colIndexA]) : 0;
				}
				size_t maxColumnWidthReductionB = proposedColumnWidths[coldIndexB];
				if (minColumnWidths.size() > coldIndexB)
				{
					maxColumnWidthReductionB = (proposedColumnWidths[coldIndexB] > minColumnWidths[coldIndexB]) ? (proposedColumnWidths[coldIndexB] - minColumnWidths[coldIndexB]) : 0;
				}
				return maxColumnWidthReductionA < maxColumnWidthReductionB;
			});
			// try to shrink all shrinkable columns proportionally
			for (size_t i = 0; i < shrinkableColumnIndexes.size(); i++)
			{
				size_t colIdx = shrinkableColumnIndexes[i];
				size_t remainingExtraWidth = extraWidth - currentTotalColumnWidthReduction;
				size_t widthReductionAmount = (i < shrinkableColumnIndexes.size()-1) ? (remainingExtraWidth / (shrinkableColumnIndexes.size() - i)) : (remainingExtraWidth);
				size_t maxColumnWidthReduction = proposedColumnWidths[colIdx];
				if (minColumnWidths.size() > colIdx)
				{
					maxColumnWidthReduction = (proposedColumnWidths[colIdx] > minColumnWidths[colIdx]) ? (proposedColumnWidths[colIdx] - minColumnWidths[colIdx]) : 0;
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
	updateColumnWidths();

	return true;
}

size_t ScrollableTableWidget::totalPaddingWidthFor(size_t numColumns) const
{
	// TABLE_COL_PADDING is on all sides of all columns
	// also factor in scrollbar width
	size_t totalPaddingWidth = columnPadding.x + ((numColumns - 1) * 2 * columnPadding.x) + columnPadding.x + scrollableList->getScrollbarWidth();
	return totalPaddingWidth;
}

size_t ScrollableTableWidget::getMaxColumnTotalWidth(size_t numColumns) const
{
	return static_cast<size_t>(width()) - totalPaddingWidthFor(numColumns);
}

size_t ScrollableTableWidget::getTableWidthNeededForTotalColumnWidth(size_t numColumns, size_t totalMinimumColumnWidth) const
{
	return totalPaddingWidthFor(numColumns) + totalMinimumColumnWidth;
}

bool ScrollableTableWidget::isUserDraggingColumnHeader() const
{
	return header->isUserDraggingColumnHeader();
}

void ScrollableTableWidget::setColumnPadding(Vector2i newPadding)
{
	if (newPadding == columnPadding)
	{
		return;
	}
	columnPadding = newPadding;

	// Need to inform header + resize columns and also resize columns in list
	header->setColumnPadding(newPadding);
	updateColumnWidths();
}

const Vector2i& ScrollableTableWidget::getColumnPadding()
{
	return columnPadding;
}

void ScrollableTableWidget::setDrawColumnLines(bool bEnabled)
{
	drawColumnLines = bEnabled;
}

void ScrollableTableWidget::setItemSpacing(uint32_t value)
{
	scrollableList->setItemSpacing(value);
}

void ScrollableTableWidget::displayRecursive(WidgetGraphicsContext const& context)
{
	WIDGET::displayRecursive(context);

	// *after* displaying all children, draw the column lines (if desired)
	if (!drawColumnLines)
	{
		return;
	}

	if (!context.clipContains(geometry())) {
		return;
	}

	int xOffset = context.getXOffset();
	int yOffset = context.getYOffset();

	int x0 = x() + xOffset;
	int y0 = y() + yOffset + scrollableList->y();
	int y1 = y0 + scrollableList->height();

	lines.clear();
	lines.reserve(columnWidths.size());

	// draw a line between every column widget
	int lastColumnEndX = -columnPadding.x;
	for (size_t colIdx = 0; colIdx < columnWidths.size(); ++colIdx)
	{
		// column lines
		int columnXPos = lastColumnEndX + columnPadding.x + columnPadding.x;
		int columRightLineXPos = columnXPos + static_cast<int>(columnWidths[colIdx]) + columnPadding.x;

		lines.emplace_back(x0 + columRightLineXPos, y0, x0 + columRightLineXPos, y1);
		lastColumnEndX = columnXPos + static_cast<int>(columnWidths[colIdx]);
	}

	iV_Lines(lines, WZCOL_MENU_SEPARATOR);
}
