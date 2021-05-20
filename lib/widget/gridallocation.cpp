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

#include "lib/framework/types.h"
#include "gridallocation.h"
#include <algorithm>
#include <map>
#include <set>
#include <numeric>

grid_allocation::allocator::allocator(const std::vector<grid_allocation::item> &items)
{
	if (!items.empty())
	{
		initialize_steps(items);
		auto normalized_items = normalize_items(items);
		initialize_expandable_map(normalized_items);
		initialize_base_sizes(normalized_items);
	}
}

std::map<uint32_t, int32_t> grid_allocation::allocator::calculate_offsets(int32_t space)
{
	if (steps.empty())
	{
		return {};
	}

	auto sizes = base_sizes;
	expand(sizes, space, 0, sizes.size());
	shrink(sizes, space, 0, sizes.size());

	std::map<uint32_t, int32_t> offsets{{0, 0}};
	int32_t current_offset = 0;
	for (size_t i = 0; i < sizes.size(); i++)
	{
		current_offset += sizes[i];
		offsets[steps[i + 1]] = current_offset;
	}

	return offsets;
}

void grid_allocation::allocator::expand(std::vector<int32_t>& sizes, int32_t space, size_t start, size_t end)
{
	auto current_size = std::accumulate(sizes.begin() + start, sizes.begin() + end, 0);
	if (current_size >= space)
	{
		return;
	}

	auto space_to_expand = space - current_size;

	int32_t slots_to_expand = 0;
	for (size_t i = start; i < end; i++)
	{
		if (expandable_map[i])
		{
			slots_to_expand += steps[i + 1] - steps[i];
		}
	}

	auto has_expandable = slots_to_expand > 0;
	if (!has_expandable)
	{
		slots_to_expand = steps[end] - steps[start];
	}

	for (size_t i = start; i < end; i++)
	{
		if (!has_expandable || expandable_map[i])
		{
			auto span = steps[i + 1] - steps[i];
			auto increment = space_to_expand * span / slots_to_expand;
			sizes[i] += increment;
			space_to_expand -= increment;
			slots_to_expand -= span;
		}
	}
}

void grid_allocation::allocator::shrink(std::vector<int32_t>& sizes, int32_t space, size_t start, size_t end)
{
	auto current_size = std::accumulate(sizes.begin() + start, sizes.begin() + end, 0);
	if (current_size <= space)
	{
		return;
	}

	auto space_to_reduce = current_size - space;
	for (size_t i = start; i < end && current_size > 0; i++)
	{
		auto decrement = space_to_reduce * sizes[i] / current_size;
		current_size -= sizes[i];
		sizes[i] -= decrement;
		space_to_reduce -= decrement;
	}
}

int grid_allocation::allocator::get_minimum_size()
{
	return std::accumulate(base_sizes.begin(), base_sizes.end(), 0);
}

void grid_allocation::allocator::initialize_steps(const std::vector<grid_allocation::item> &items)
{
	std::set<uint32_t> unique_steps;
	for (const auto &item: items)
	{
		unique_steps.insert(item.start());
		unique_steps.insert(item.end());
	}

	slots_size = unique_steps.size() - 1;
	steps = {unique_steps.begin(), unique_steps.end()};
	for (size_t i = 0; i < steps.size(); i++)
	{
		steps_index[steps[i]] = static_cast<uint32_t>(i);
	}
}

std::vector<grid_allocation::item> grid_allocation::allocator::normalize_items(const std::vector<grid_allocation::item> &items)
{
	typedef std::pair<uint32_t, uint32_t> items_map_key;
	std::map<items_map_key, size_t> items_map;
	std::vector<item> normalized_items;

	for (auto item: items)
	{
		item.slot.span = steps_index[item.end()] - steps_index[item.start()];
		item.slot.start = steps_index[item.start()];

		items_map_key key{item.start(), item.span()};
		auto existing_iterator = items_map.find(key);
		if (existing_iterator == items_map.end())
		{
			items_map[key] = normalized_items.size();
			normalized_items.push_back(item);
			continue;
		}

		auto& existing = normalized_items[existing_iterator->second];
		existing.slot.expandable = existing.slot.expandable && item.slot.expandable;
		existing.ideal_size = std::max(existing.ideal_size, item.ideal_size);
	}

	std::sort(normalized_items.begin(), normalized_items.end(), [](item& a, item &b) {
		if (a.span() == b.span())
		{
			return a.start() < b.start();
		}

		return a.span() < b.span();
	});

	return normalized_items;
}

void grid_allocation::allocator::initialize_expandable_map(const std::vector<item>& items)
{
	expandable_map.resize(slots_size, true);
	for (const auto &item: items)
	{
		if (!item.expandable())
		{
			for (size_t slot = item.start(); slot < item.end(); slot++)
			{
				expandable_map[slot] = false;
			}
		}
	}

	for (auto item: items)
	{
		if (item.expandable())
		{
			auto begin = expandable_map.begin() + item.start();
			auto end = expandable_map.begin() + item.end();
			item.slot.expandable = std::find(begin, end, false) == end;
		}
	}
}

void grid_allocation::allocator::initialize_base_sizes(const std::vector<item>& items)
{
	std::vector<std::set<const item*>> slot_items(slots_size);
	base_sizes.resize(slots_size);
	for (auto& item: items)
	{
		auto allocated = std::accumulate(base_sizes.begin() + item.start(), base_sizes.begin() + item.end(), 0);
		if (allocated >= item.ideal_size)
		{
			continue;
		}

		for (size_t slot = item.start(); slot < item.end(); slot++)
		{
			slot_items[slot].insert(&item);
		}

		for (size_t slot = item.start(); slot < item.end() && allocated < item.ideal_size; slot++)
		{
			auto& current_unit = slot_items[slot];

			if (current_unit.size() == 1)
			{
				auto allocatable = item.ideal_size - allocated;
				allocated += allocatable;
				base_sizes[slot] += allocatable;
				break;
			}

			auto borrow_items = slot_items[slot];
			borrow_items.erase(&item);

			for (size_t borrow_slot = 0; borrow_slot < item.start(); borrow_slot++)
			{
				if (base_sizes[borrow_slot] == 0 || slot_items[borrow_slot] != borrow_items)
				{
					continue;
				}

				auto allocatable = std::min(item.ideal_size - allocated, base_sizes[borrow_slot]);
				base_sizes[borrow_slot] -= allocatable;
				allocated += allocatable;
				base_sizes[slot] += allocatable;
			}
		}

		expand(base_sizes, item.ideal_size, item.start(), item.end());
	}
}

grid_allocation::slot::slot(uint32_t start, uint32_t span, bool expandable)
	: start(start)
	, span(span)
	, expandable(expandable)
{
}

uint32_t grid_allocation::item::start() const
{
	return slot.start;
}

uint32_t grid_allocation::item::end() const
{
	return slot.start + slot.span;
}

uint32_t grid_allocation::item::span() const
{
	return slot.span;
}

bool grid_allocation::item::expandable() const
{
	return slot.expandable;
}
