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
 * @file memory_pool.h
 * MemoryPool provides efficient allocation and deallocation of fixed-size memory blocks using
 * multiple power-of-2 size classes.
 * Works best for relatively small block sizes (<=256 KiB).
 */
#pragma once

#include "lib/framework/frame.h" // for ASSERT

#include <cstddef>
#include <cstdint>
#include <memory>
#include <cassert>
#include <queue>
#include <list>
#include <utility>
#include <vector>

/// <summary>
/// Options for MemoryPool controlling how many size classes there are,
/// how block sizes are distributed and how much elements each individual
/// size class has.
/// </summary>
struct MemoryPoolOptions
{
	// Minimal supported block size (in bytes). This needs to be a power-of-2.
	// The very first (smallest one in terms of block size) size class will have this block size.
	size_t minimal_supported_block_size = 8;
	// The largest block size required to be supported by the MemoryPool (in bytes).
	// MemoryPool may allocate size classes that have block size at least this much bytes.
	size_t largest_required_block_size = 65536;
	// The very first (the smallest one in terms of block size) size class will have this much
	// elements initially allocated in the first memory chunk.
	size_t required_capacity_for_smallest_sub_pool = 512;
	// Each size class is required to have at least this much elements in a single memory chunk.
	size_t minimal_required_capacity = 8;
};

/// <summary>
/// MemoryPool provides efficient allocation and deallocation of fixed-size memory blocks using
/// multiple power-of-2 size classes.
/// Works best for relatively small block sizes (<=256 KiB).
///
/// MemoryPool consists of a set of sub-pools.
///
/// Each sub-pool is responsible for serving fixed-sized allocations (equal to its block size)
/// from a single contiguous chunk of memory. If the capacity of a sub-pool is exceeded,
/// it will automatically replenish it by allocating a new chunk of memory having its capacity
/// to be twice the capacity of the previous chunk.
/// Every memory chunk manages its own free list for fast block reuse.
///
/// On deallocation, the sub-pool will automatically reclaim and deallocate smaller chunks
/// (if the current chunk has become empty) in favor of larger chunks (if there's any).
/// This helps to keep memory consumption better overall.
///
/// Each subsequent sub-pool will have block size twice as large than the previous one.
/// </summary>
class MemoryPool
{
public:

	explicit MemoryPool(const MemoryPoolOptions& opts);

	~MemoryPool() = default;
	MemoryPool(const MemoryPool&) = delete;
	MemoryPool& operator=(const MemoryPool&) = delete;

	/// <summary>
	/// Allocate a block of at least the requested size and alignment.
	/// </summary>
	void* allocate(size_t size, size_t alignment);

	/// <summary>
	/// Deallocate a previously allocated block. Finds the sub-pool by size and alignment.
	///
	/// Will try to automatically deallocate the "now-empty" chunk if there's a larger one
	/// available in the current sub-pool.
	/// </summary>
	void deallocate(void* ptr, size_t bytes, size_t alignment);

	size_t largest_supported_block_size() const;

private:

	struct Chunk
	{
		size_t blockSize;
		size_t capacity;
		size_t freeBlocksCount;
		size_t nextFreeIndex = 0;  // Next index for a block that has never been allocated before
		std::unique_ptr<uint8_t[]> buffer;

		size_t head = 0; // Dequeue index for the freelist
		size_t tail = 0; // Enqueue index for the freelist
		// Contiguous buffer of pointers to freelist blocks.
		//
		// This always has `capacity + 1` length so that in case the freelist has exactly
		// `capacity` elements, `head` and `tail` pointers won't become equal and will be
		// separated by a special "sentinel" element.
		std::unique_ptr<void*[]> freeListBuf;

		Chunk(size_t bSize, size_t cap)
			: blockSize(bSize)
			, capacity(cap)
			, freeBlocksCount(cap)
			, nextFreeIndex(0)
			, buffer(std::make_unique<uint8_t[]>(bSize * cap))
			, freeListBuf(std::make_unique<void*[]>(cap + 1))
		{}

		bool has_freelist_blocks() const
		{
			return head != tail;
		}

		void push_free_block(void* ptr);
		void* pop_free_block();
	};

	struct SubPool
	{
		explicit SubPool() = default;
		SubPool(SubPool&& other) = default;

		using ChunkStorage = std::list<Chunk>;

		size_t blockSize = 0;
		size_t totalFreeBlocks = 0;
		ChunkStorage chunks;

		// Returns the owning chunk for a pointer
		ChunkStorage::iterator get_owning_chunk(void* ptr);

		/// <summary>
		/// Helper to allocate a block from a chunk, update counters, and check alignment.
		/// </summary>
		/// <param name="chunk">Reference to the chunk to allocate from.</param>
		/// <param name="alignment">Alignment requirement for the memory block.</param>
		/// <returns>Pointer to the allocated memory block.</returns>
		void* allocate_new_block(Chunk& chunk, size_t alignment);

		/// <summary>
		/// Helper to allocate a block from the freelist of a given chunk.
		/// </summary>
		/// <param name="pool">Pointer to the chunk containing the freelist.</param>
		/// <param name="alignment">Alignment requirement for the memory block.</param>
		/// <returns>Pointer to the allocated memory block.</returns>
		void* allocate_from_freelist(Chunk& chunk, size_t alignment);
	};

	/// <summary>
	/// Allocates sub-pools for supported size classes.
	/// </summary>
	void allocate_sub_pools(size_t numSizeClasses);

	/// <summary>
	/// Finds a suitable sub-pool for the specified size and alignment.
	/// </summary>
	/// <param name="bytes">Size of the memory block to find a pool for.</param>
	/// <param name="alignment">Alignment requirement for the memory block.</param>
	/// <returns>Pointer to the appropriate SubPool, or nullptr if no suitable pool is found.</returns>
	SubPool* find_pool(size_t bytes, size_t alignment);

	MemoryPoolOptions opts_;
	std::vector<SubPool> subPools_;
};

/// <summary>
/// Provides a default singleton memory pool instance for use throughout the application.
/// </summary>
/// <returns>Reference to the default memory pool instance.</returns>
MemoryPool& defaultMemoryPool();
