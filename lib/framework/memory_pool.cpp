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
#include "lib/framework/memory_pool.h"
#include "lib/framework/frame.h" // FOR ASSERT, ASSERT_OR_RETURN

namespace
{

size_t num_size_classes(const MemoryPoolOptions& opts)
{
	size_t n = 1;
	size_t sz = opts.minimal_supported_block_size;
	while (sz < opts.largest_required_block_size)
	{
		sz <<= 1;
		++n;
	}
	return n;
}

size_t initial_chunk_capacity(const MemoryPoolOptions& opts, size_t blockSize)
{
	// Sub-pools corresponding to smaller blocks will get more capacity by default.
	// Always allocate at least `opts.minimal_required_capacity` blocks for any given `blockSize`.
	return std::max(
		(opts.required_capacity_for_smallest_sub_pool * opts.minimal_supported_block_size) / blockSize,
		opts.minimal_required_capacity);
}

bool is_pow_2(size_t x)
{
	return (x & (x - 1)) == 0;
}

// Round up `x` to the nearest power-of-2
size_t bit_ceil(size_t x)
{
	if (x <= 1)
	{
		return 1;
	}
	size_t i = 1;
	while ((size_t(1) << i) < x)
	{
		++i;
	}
	return size_t(1) << i;
}

// Provide some validation of initial memory pool options + fixup the values, if needed, to match implementation expectations.
MemoryPoolOptions fixup_options(const MemoryPoolOptions& opts)
{
	ASSERT(is_pow_2(opts.minimal_supported_block_size) && is_pow_2(opts.largest_required_block_size), "Block size should be a power of 2");
	ASSERT(opts.minimal_required_capacity > 0, "Invalid minimal required size class capacity");
	ASSERT(opts.required_capacity_for_smallest_sub_pool >= opts.minimal_required_capacity, "Invalid required capacity for smallest sub-pool");

	MemoryPoolOptions res;

	// Default to 8 if minimal_required_capacity is set to 0 by user.
	res.minimal_required_capacity = opts.minimal_required_capacity > 0 ? opts.minimal_required_capacity : 8;
	// Use x2 multiplier (relative to the minimal required capacity) heuristic for the worst case.
	res.required_capacity_for_smallest_sub_pool =
		opts.required_capacity_for_smallest_sub_pool >= opts.minimal_required_capacity ?
		opts.required_capacity_for_smallest_sub_pool :
		opts.minimal_required_capacity * 2;
	// Round up block size limits to the nearest powers-of-2
	res.minimal_supported_block_size = bit_ceil(opts.minimal_supported_block_size);
	res.largest_required_block_size = bit_ceil(opts.largest_required_block_size);

	return res;
}

} // anonymous namespace

MemoryPool::MemoryPool(const MemoryPoolOptions& opts)
	: opts_(fixup_options(opts))
{
	allocate_sub_pools(num_size_classes(opts_));
}

void* MemoryPool::allocate(size_t size, size_t alignment)
{
	if (size == 0 || size > largest_supported_block_size())
	{
		ASSERT(false, "Invalid allocation size (doesn't fit into any available size class): %zu", size);
		return nullptr;
	}
	SubPool* pool = find_pool(size, alignment);
	ASSERT_OR_RETURN(nullptr, pool != nullptr, "No available blocks in any pool for size %zu and alignment %zu", size, alignment);

	// If there are free blocks, serve from freelist or from chunk's nextFreeIndex
	if (pool->totalFreeBlocks != 0)
	{
		for (auto& chunk : pool->chunks)
		{
			// Serve from the chunk's freelist if it's not empty
			if (chunk.has_freelist_blocks())
			{
				return pool->allocate_from_freelist(chunk, alignment);
			}
			// Otherwise, find the first chunk with nextFreeIndex < capacity
			if (chunk.nextFreeIndex < chunk.capacity)
			{
				return pool->allocate_new_block(chunk, alignment);
			}
		}
		// Should not reach here if totalFreeBlocks is correct
		ASSERT(false, "totalFreeBlocks > 0 but no available block found");
		return nullptr;
	}

	// If no free blocks, allocate a new chunk (capacities increasing in a geometric progression)
	// and serve the allocation from it
	size_t newChunkCapacity = pool->chunks.back().capacity * 2;
	auto& newChunk = pool->chunks.emplace_back(pool->blockSize, newChunkCapacity);
	pool->totalFreeBlocks += newChunkCapacity;
	return pool->allocate_new_block(newChunk, alignment);
}

void MemoryPool::deallocate(void* ptr, size_t bytes, size_t alignment)
{
	if (!ptr)
	{
		return;
	}
	SubPool* pool = find_pool(bytes, alignment);
	ASSERT_OR_RETURN(, pool != nullptr, "No sub-pool found for deallocation (bytes=%zu, alignment=%zu)", bytes, alignment);

	// Find the chunk that owns this pointer
	auto chunkIt = pool->get_owning_chunk(ptr);
	ASSERT_OR_RETURN(, chunkIt != pool->chunks.end(), "Pointer %p not from this pool", ptr);

	++pool->totalFreeBlocks;

	// Each subsequent chunk in the list is larger than the previous one
	// Always prefer larger chunks over smaller ones (with the smaller one being automatically reclaimed)
	if ((chunkIt->freeBlocksCount + 1) == chunkIt->capacity && std::next(chunkIt) != pool->chunks.end())
	{
		// Reclaim the now-empty chunk
		const auto reclaimedCapacity = chunkIt->capacity;
		pool->chunks.erase(chunkIt);
		// Fixup the total sub-pool capacity to account for the just reclaimed chunk
		pool->totalFreeBlocks -= reclaimedCapacity;
	}
	else
	{
		// Either this is _the_ largest chunk available in the sub-pool, or it's not empty yet
		// Push the block to the chunk's freelist
		chunkIt->push_free_block(ptr);
	}
}

size_t MemoryPool::largest_supported_block_size() const
{
	return opts_.largest_required_block_size;
}

void MemoryPool::allocate_sub_pools(size_t numSizeClasses)
{
	subPools_.reserve(numSizeClasses);

	size_t block_size = opts_.minimal_supported_block_size;
	for (size_t i = 0; i < numSizeClasses; ++i)
	{
		SubPool sp;
		sp.blockSize = block_size;
		// Create initial chunk
		size_t initial_capacity = initial_chunk_capacity(opts_, block_size);

		sp.chunks.emplace_back(block_size, initial_capacity);
		sp.totalFreeBlocks = initial_capacity;

		subPools_.emplace_back(std::move(sp));

		block_size <<= 1;
	}
}

MemoryPool::SubPool* MemoryPool::find_pool(size_t bytes, size_t alignment)
{
	for (auto& p : subPools_)
	{
		if (p.blockSize >= bytes && p.blockSize >= alignment && p.blockSize % alignment == 0)
		{
			return &p;
		}
	}
	return nullptr;
}

void MemoryPool::Chunk::push_free_block(void* ptr)
{
	ASSERT_OR_RETURN(, ptr != nullptr, "Unexpected null pointer in memory pool");
	ASSERT_OR_RETURN(, freeBlocksCount < capacity, "Free list is full");
	freeListBuf[tail] = ptr;
	tail = (tail + 1) % (capacity + 1);
	++freeBlocksCount;
}

void* MemoryPool::Chunk::pop_free_block()
{
	ASSERT_OR_RETURN(nullptr, freeBlocksCount > 0 && head != tail, "Free list is empty");
	void* ptr = freeListBuf[head];
	head = (head + 1) % (capacity + 1);
	--freeBlocksCount;
	return ptr;
}

MemoryPool::SubPool::ChunkStorage::iterator MemoryPool::SubPool::get_owning_chunk(void* ptr)
{
	uint8_t* p = static_cast<uint8_t*>(ptr);
	for (auto it = chunks.begin(), end = chunks.end(); it != end; ++it)
	{
		Chunk& chunk = *it;
		uint8_t* buf = chunk.buffer.get();
		size_t total_size = chunk.blockSize * chunk.capacity;
		if (p >= buf && p < buf + total_size)
		{
			size_t offset = static_cast<size_t>(p - buf);
			ASSERT_OR_RETURN(chunks.end(), offset % chunk.blockSize == 0, "Pointer is not aligned to block size");
			return it;
		}
	}
	return chunks.end();
}

void* MemoryPool::SubPool::allocate_new_block(Chunk& chunk, size_t alignment)
{
	void* block = chunk.buffer.get() + chunk.nextFreeIndex * chunk.blockSize;
	++chunk.nextFreeIndex;
	--chunk.freeBlocksCount;
	--totalFreeBlocks;
	ASSERT_OR_RETURN(nullptr, reinterpret_cast<uintptr_t>(block) % alignment == 0, "Block returned is not properly aligned");
	return block;
}

void* MemoryPool::SubPool::allocate_from_freelist(Chunk& chunk, size_t alignment)
{
	void* block = chunk.pop_free_block();
	--totalFreeBlocks;
	ASSERT_OR_RETURN(nullptr, reinterpret_cast<uintptr_t>(block) % alignment == 0, "Block returned from free list is not properly aligned");
	return block;
}

MemoryPool& defaultMemoryPool()
{
	static MemoryPool instance({});
	return instance;
}
