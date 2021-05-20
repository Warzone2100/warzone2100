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

#ifndef __INCLUDED_LIB_WIDGET_GRIDALLOCATION_H__
#define __INCLUDED_LIB_WIDGET_GRIDALLOCATION_H__

#include <map>
#include <set>
#include <vector>

namespace grid_allocation
{
	struct slot
	{
		slot(uint32_t start = 0, uint32_t span = 1, bool expandable = true);
		uint32_t start;
		uint32_t span;
		bool expandable;
	};

	struct item
	{
		grid_allocation::slot slot;
		int32_t ideal_size;

		uint32_t start() const;
		uint32_t end() const;
		uint32_t span() const;
		bool expandable() const;
	};

	class allocator
	{
	public:
		allocator(const std::vector<item> &items);
		std::map<uint32_t, int32_t> calculate_offsets(int32_t available_space);
		int get_minimum_size();

	private:
		void initialize_steps(const std::vector<grid_allocation::item> &items);
		std::vector<item> normalize_items(const std::vector<grid_allocation::item> &items);
		void initialize_expandable_map(const std::vector<item>& items);
		void initialize_base_sizes(const std::vector<item>& items);
		void expand(std::vector<int32_t>& sizes, int32_t space, size_t start, size_t end);
		void shrink(std::vector<int32_t>& sizes, int32_t space, size_t start, size_t end);

		size_t slots_size;
		std::map<uint32_t, uint32_t> steps_index;
		std::vector<uint32_t> steps;
		std::vector<int32_t> base_sizes;
		std::vector<bool> expandable_map;
	};
}

#endif // __INCLUDED_LIB_WIDGET_GRIDALLOCATION_H__
