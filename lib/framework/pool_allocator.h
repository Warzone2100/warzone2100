// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

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
 * @file pool_allocator.h
 * STL-compatible allocator that uses MemoryPool for memory management.
 */
#pragma once

#include "lib/framework/memory_pool.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <unordered_set>

/// <summary>
/// STL-compatible allocator that uses MemoryPool for memory management.
/// </summary>
template <typename T, typename PoolT = MemoryPool>
class PoolAllocator
{
public:
	using value_type = T;
	using pointer = T*;
	using const_pointer = const T*;
	using reference = T&;
	using const_reference = const T&;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	template <typename U>
	struct rebind { using other = PoolAllocator<U, PoolT>; };

	explicit PoolAllocator() noexcept : pool_(defaultMemoryPool()) {}
	explicit PoolAllocator(PoolT& pool) noexcept : pool_(pool) {}

	PoolAllocator(const PoolAllocator& other) noexcept
		: pool_(other.pool_),
		defaultAlloc_(other.defaultAlloc_),
		oversizedAllocs_(other.oversizedAllocs_)
	{}

	template <typename U>
	PoolAllocator(const PoolAllocator<U, PoolT>& other) noexcept
		: pool_(other.pool_),
		defaultAlloc_(other.defaultAlloc_),
		oversizedAllocs_(other.oversizedAllocs_)
	{}

	pointer allocate(size_type n)
	{
		if (pool_.largest_supported_block_size() < std::max(n * sizeof(T), alignof(T)))
		{
			// Allocate via default new-delete allocator if there's no suitable block size available in the memory pool.
			T* ret = defaultAlloc_.allocate(n);
			oversizedAllocs_.emplace(ret);
			return ret;
		}
		T* ret = static_cast<pointer>(pool_.allocate(n * sizeof(T), alignof(T)));
		if (!ret)
		{
			// This could only happen if the underlying memory pool failed to replenish its capacity (failed to allocate a new chunk).
			// Not much we can do in this particular situation.
			throw std::bad_alloc();
		}
		return ret;
	}

	void deallocate(pointer p, size_type n)
	{
		if (pool_.largest_supported_block_size() < std::max(n * sizeof(T), alignof(T)))
		{
			auto oversizedIt = oversizedAllocs_.find(p);
			ASSERT(oversizedIt != oversizedAllocs_.end(), "Block %p not allocated by the current allocator.", static_cast<void*>(p));
			if (oversizedIt != oversizedAllocs_.end())
			{
				oversizedAllocs_.erase(oversizedIt);
				defaultAlloc_.deallocate(p, n);
			}
			return;
		}
		pool_.deallocate(static_cast<void*>(p), n * sizeof(T), alignof(T));
	}

	template <typename U, typename... Args>
	void construct(U* p, Args&&... args)
	{
		::new ((void*)p) U(std::forward<Args>(args)...);
	}

	template <typename U>
	void destroy(U* p)
	{
		p->~U();
	}

	// Comparison operators
	bool operator==(const PoolAllocator& other) const noexcept { return &pool_ == &other.pool_ && defaultAlloc_ == other.defaultAlloc_; }
	bool operator!=(const PoolAllocator& other) const noexcept { return !(*this == other); }

	PoolT& pool_;
	std::allocator<T> defaultAlloc_;
	std::unordered_set<void*> oversizedAllocs_;
};
