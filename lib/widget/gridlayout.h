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

#ifndef __INCLUDED_LIB_WIDGET_GRIDLAYOUT_H__
#define __INCLUDED_LIB_WIDGET_GRIDLAYOUT_H__

#include <nonstd/optional.hpp>
#include <map>
#include <set>
#include "widget.h"
#include "gridallocation.h"

class GridLayout: public WIDGET
{
public:
	struct Placement
	{
	public:
		Placement(grid_allocation::slot column, grid_allocation::slot row, std::shared_ptr<WIDGET> widget);
		grid_allocation::slot column;
		grid_allocation::slot row;
		std::shared_ptr<WIDGET> widget;
	};

	GridLayout();
	void place(grid_allocation::slot columnAllocation, grid_allocation::slot rowAllocation, std::shared_ptr<WIDGET> widget);
	int32_t idealWidth() override;
	int32_t idealHeight() override;
	nonstd::optional<std::vector<uint32_t>> getScrollSnapOffsets() override;
	const std::map<uint32_t, int32_t> &getColumnOffsets();
	const std::map<uint32_t, int32_t> &getRowOffsets();

protected:
	void displayRecursive(WidgetGraphicsContext const &context) override;
	void geometryChanged() override;

private:
	typedef std::function<grid_allocation::item(const GridLayout::Placement)> PlacementTransformer;

	void updateLayout();
	void invalidateLayout();
	void invalidateAllocation();
	std::vector<grid_allocation::item> getAllocationItems(PlacementTransformer placementTransformer);
	static grid_allocation::item placementColumn(GridLayout::Placement placement);
	static grid_allocation::item placementRow(GridLayout::Placement placement);

	grid_allocation::allocator &getColumnAllocator();
	grid_allocation::allocator &getRowAllocator();

	nonstd::optional<grid_allocation::allocator> columnAllocation;
	nonstd::optional<grid_allocation::allocator> rowAllocation;
	std::vector<Placement> placements;
	bool layoutDirty = false;
	std::vector<uint32_t> scrollSnapOffsets;
	std::map<uint32_t, int32_t> columnOffsets;
	std::map<uint32_t, int32_t> rowOffsets;
};

#endif // __INCLUDED_LIB_WIDGET_GRIDLAYOUT_H__
