// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** @file gfx_api_frame_resource_cache.h
 * Per-frame pooled caches for dynamic framebuffers/FBOs.
 */

#pragma once

#include <cstddef>
#include <cstdint>

#include "lib/framework/wzglobal.h"
#include "gfx_api_formats_def.h"
#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <nonstd/optional.hpp>

namespace gfx_api
{

struct abstract_texture;

/// <summary>
/// Per-frame pool of reusable GPU resources keyed by attachment/layout.
///
/// Backends own one or more specializations (`FramebufferResourceCache`, `DynamicFBOCache`, …).
/// Lifecycle (callers must preserve this ordering):
/// 1. releaseAll() at frame graph reset (start of accumulation).
/// 2. acquire() while recording.
/// 3. submitFrame() on the backend.
/// 4. purgeUnused() after submit - drop resources not acquired this frame.
/// </summary>
template <typename Key, typename Storage, typename Handle>
class PooledResourceCache
{
public:
	using PurgeHandleFn = std::function<void(Handle)>;

	/// Return a handle for `key`, creating storage via `createFn` when the pool is exhausted.
	template <typename CreateFn>
	Handle acquire(const Key& key, CreateFn&& createFn)
	{
		auto cacheIt = std::find_if(_cache.begin(), _cache.end(),
			[&key](const auto& entry) { return entry.first == key; });

		if (cacheIt == _cache.end())
		{
			_cache.emplace_back(key, CacheEntry {});
			cacheIt = std::prev(_cache.end());
		}

		CacheEntry& cacheEntry = cacheIt->second;
		if (cacheEntry.usedCount + 1 > cacheEntry.resources.size())
		{
			Storage resource = std::forward<CreateFn>(createFn)();
			ASSERT(isValidStorage(resource), "Failed to create pooled frame resource");
			cacheEntry.resources.emplace_back(std::move(resource));
		}

		return toHandle(cacheEntry.resources[cacheEntry.usedCount++]);
	}

	/// Reset per-key use counts at the start of frame accumulation.
	void releaseAll()
	{
		for (auto& cacheEntry : _cache)
		{
			cacheEntry.second.usedCount = 0;
		}
	}

	/// After submit, shrink pools and optionally destroy dropped handles via `onPurge`.
	void purgeUnused(const PurgeHandleFn& onPurge = PurgeHandleFn())
	{
		for (auto it = _cache.begin(); it != _cache.end(); )
		{
			CacheEntry& cacheEntry = it->second;
			dropExcess(cacheEntry.resources, cacheEntry.usedCount, onPurge);
			if (cacheEntry.usedCount == 0)
			{
				it = _cache.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	/// Tear down all entries (e.g. device lost / context destroy).
	void clear(const PurgeHandleFn& onPurge = PurgeHandleFn())
	{
		if (onPurge)
		{
			for (auto& cacheEntry : _cache)
			{
				for (Storage& resource : cacheEntry.second.resources)
				{
					if (isValidStorage(resource))
					{
						onPurge(toHandle(resource));
					}
				}
			}
		}
		_cache.clear();
	}

private:

	struct CacheEntry
	{
		size_t usedCount = 0;
		std::vector<Storage> resources;
	};

	static bool isValidStorage(const Storage& resource);
	static Handle toHandle(Storage& resource);
	static Handle toHandle(const Storage& resource);
	static void dropExcess(std::vector<Storage>& resources, size_t usedCount, const PurgeHandleFn& onPurge);

	std::vector<std::pair<Key, CacheEntry>> _cache;
};

template<typename Key, typename Storage, typename Handle>
bool PooledResourceCache<Key, Storage, Handle>::isValidStorage(const Storage& resource)
{
	if constexpr (std::is_pointer_v<Handle>)
	{
		return toHandle(resource) != nullptr;
	}
	else
	{
		return resource != Storage {};
	}
}

template<typename Key, typename Storage, typename Handle>
Handle PooledResourceCache<Key, Storage, Handle>::toHandle(Storage& resource)
{
	if constexpr (std::is_same_v<Storage, std::unique_ptr<abstract_texture>>)
	{
		return resource.get();
	}
	else
	{
		return resource;
	}
}

template<typename Key, typename Storage, typename Handle>
Handle PooledResourceCache<Key, Storage, Handle>::toHandle(const Storage& resource)
{
	if constexpr (std::is_same_v<Storage, std::unique_ptr<abstract_texture>>)
	{
		return resource.get();
	}
	else
	{
		return resource;
	}
}

template<typename Key, typename Storage, typename Handle>
void PooledResourceCache<Key, Storage, Handle>::dropExcess(std::vector<Storage>& resources, size_t usedCount,
	const PurgeHandleFn& onPurge)
{
	ASSERT(usedCount <= resources.size(), "Pooled resource cache usedCount exceeds resource count");
	while (resources.size() > usedCount)
	{
		Storage& dropped = resources.back();
		if (onPurge && isValidStorage(dropped))
		{
			onPurge(toHandle(dropped));
		}
		resources.pop_back();
	}
}

/// <summary>
/// Key for pooled Vulkan framebuffer instances (render pass + attachment views + size).
///
/// `attachmentViewHandles` are stored as `uint64_t` so this header stays backend-neutral;
/// Vulkan `beginPass` fills the key and passes it to `_framebufferCache`.
/// </summary>
struct FramebufferResourceKey
{
	/// Cached Vulkan render-pass layout index from warm-up on `VkRoot::_warmEntries`.
	size_t renderPassId = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	/// VkImageView handles stored as uint64_t for backend-neutral cache code.
	std::vector<uint64_t> attachmentViewHandles;

	bool operator<(const FramebufferResourceKey& other) const
	{
		return std::tie(renderPassId, width, height, attachmentViewHandles)
			< std::tie(other.renderPassId, other.width, other.height, other.attachmentViewHandles);
	}

	bool operator==(const FramebufferResourceKey& other) const
	{
		return renderPassId == other.renderPassId
			&& width == other.width
			&& height == other.height
			&& attachmentViewHandles == other.attachmentViewHandles;
	}
};

using FramebufferResourceCache = PooledResourceCache<FramebufferResourceKey, uint64_t, uint64_t>;

/// <summary>
/// Key for pooled OpenGL FBO instances (attachment object ids + layers + size).
///
/// Built from a resolved `RenderPassDesc` in `gl_context::beginPass` when not targeting
/// the default framebuffer.
/// </summary>
struct DynamicFBOKey
{
	struct Slot
	{
		uint32_t objectId = 0;
		uint32_t arrayLayer = 0;
		bool isRenderbuffer = false;
		/// True when the attachment is the window default framebuffer (FBO 0).
		bool isDefaultFramebuffer = false;

		bool operator<(const Slot& other) const
		{
			return std::tie(objectId, arrayLayer, isRenderbuffer, isDefaultFramebuffer)
				< std::tie(other.objectId, other.arrayLayer, other.isRenderbuffer, other.isDefaultFramebuffer);
		}

		bool operator==(const Slot& other) const
		{
			return objectId == other.objectId
				&& arrayLayer == other.arrayLayer
				&& isRenderbuffer == other.isRenderbuffer
				&& isDefaultFramebuffer == other.isDefaultFramebuffer;
		}
	};

	uint32_t width = 0;
	uint32_t height = 0;
	std::vector<Slot> colorSlots;
	nonstd::optional<Slot> depthSlot;

	bool operator<(const DynamicFBOKey& other) const
	{
		return std::tie(width, height, colorSlots, depthSlot)
			< std::tie(other.width, other.height, other.colorSlots, other.depthSlot);
	}

	bool operator==(const DynamicFBOKey& other) const
	{
		return width == other.width
			&& height == other.height
			&& colorSlots == other.colorSlots
			&& depthSlot == other.depthSlot;
	}
};

/// GL-only counterpart to `FramebufferResourceCache`.
using DynamicFBOCache = PooledResourceCache<DynamicFBOKey, uint32_t, uint32_t>;

} // namespace gfx_api
