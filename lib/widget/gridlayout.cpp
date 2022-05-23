/*
	This file is part of Warzone 2100.
	Copyright (C) 2021  Warzone 2100 Project

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

#include "gridlayout.h"
#include "widgbase.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include <algorithm>
#include <nonstd/optional.hpp>
#include <map>
#include <set>

GridLayout::GridLayout(): WIDGET()
{
}

void GridLayout::place(grid_allocation::slot column, grid_allocation::slot row, std::shared_ptr<WIDGET> widget)
{
	attach(widget);
	placements.emplace_back(GridLayout::Placement {column, row, widget});
	invalidateAllocation();
}

int32_t GridLayout::idealWidth()
{
	return getColumnAllocator().get_minimum_size();
}

int32_t GridLayout::idealHeight()
{
	return getRowAllocator().get_minimum_size();
}

nonstd::optional<std::vector<uint32_t>> GridLayout::getScrollSnapOffsets()
{
	updateLayout();
	return scrollSnapOffsets;
}

void GridLayout::displayRecursive(WidgetGraphicsContext const &context)
{
	updateLayout();
	WIDGET::displayRecursive(context);
}

void GridLayout::geometryChanged()
{
	invalidateLayout();
}

void GridLayout::updateLayout()
{
	if (!layoutDirty)
	{
		return;
	}

	layoutDirty = false;

	columnOffsets = getColumnAllocator().calculate_offsets(width());
	rowOffsets = getRowAllocator().calculate_offsets(height());
	scrollSnapOffsets.resize(rowOffsets.size());
	size_t i = 0;
	for (auto offset: rowOffsets)
	{
		scrollSnapOffsets[i++] = offset.second;
	}

	for (auto &placement: placements)
	{
		auto left = columnOffsets[placement.column.start];
		auto right = columnOffsets[placement.column.start + placement.column.span];
		auto top = rowOffsets[placement.row.start];
		auto bottom = rowOffsets[placement.row.start + placement.row.span];
		placement.widget->setGeometry(left, top, right - left, bottom - top);
	}
}

const std::map<uint32_t, int32_t> &GridLayout::getColumnOffsets()
{
	updateLayout();
	return columnOffsets;
}

const std::map<uint32_t, int32_t> &GridLayout::getRowOffsets()
{
	updateLayout();
	return rowOffsets;
}

void GridLayout::invalidateLayout()
{
	layoutDirty = true;
}

void GridLayout::invalidateAllocation()
{
	columnAllocation.reset();
	rowAllocation.reset();
	invalidateLayout();
}

std::vector<grid_allocation::item> GridLayout::getAllocationItems(PlacementTransformer placementTransformer)
{
	auto items = std::vector<grid_allocation::item>();
	for (const auto &placement: placements)
	{
		items.push_back(placementTransformer(placement));
	}

	return items;
}

grid_allocation::item GridLayout::placementColumn(GridLayout::Placement placement)
{
	return {placement.column, placement.widget->idealWidth()};
}

grid_allocation::item GridLayout::placementRow(GridLayout::Placement placement)
{
	return {placement.row, placement.widget->idealHeight()};
}

GridLayout::Placement::Placement(grid_allocation::slot column, grid_allocation::slot row, std::shared_ptr<WIDGET> widget)
	: column(column)
	, row(row)
	, widget(widget)
{
}

grid_allocation::allocator &GridLayout::getColumnAllocator()
{
	if (!columnAllocation)
	{
		columnAllocation.emplace(getAllocationItems(GridLayout::placementColumn));
	}

	return columnAllocation.value();
}

grid_allocation::allocator &GridLayout::getRowAllocator()
{
	if (!rowAllocation)
	{
		rowAllocation.emplace(getAllocationItems(GridLayout::placementRow));
	}

	return rowAllocation.value();
}
