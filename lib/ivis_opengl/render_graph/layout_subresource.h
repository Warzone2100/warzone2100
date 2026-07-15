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
/** @file layout_subresource.h
 * `LayoutSubresourceKey` and helpers for per-slice image layout tracking.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_set>

namespace gfx_api
{

struct abstract_texture;
struct AttachmentDesc;
struct ResolvedRead;
struct RenderPassDesc;

/// Compile-time and runtime layout tracking key for one image subresource.
struct LayoutSubresourceKey
{
	abstract_texture* texture = nullptr;
	uint32_t arrayLayer = 0;
	uint32_t mipLevel = 0;
};

inline bool operator==(const LayoutSubresourceKey& a, const LayoutSubresourceKey& b)
{
	return a.texture == b.texture && a.arrayLayer == b.arrayLayer && a.mipLevel == b.mipLevel;
}

inline bool operator!=(const LayoutSubresourceKey& a, const LayoutSubresourceKey& b)
{
	return !(a == b);
}

struct LayoutSubresourceKeyHash
{
	size_t operator()(const LayoutSubresourceKey& key) const noexcept;
};

using LayoutSubresourceSet = std::unordered_set<LayoutSubresourceKey, LayoutSubresourceKeyHash>;

LayoutSubresourceKey layoutSubresourceKey(abstract_texture* texture, uint32_t arrayLayer = 0, uint32_t mipLevel = 0);
LayoutSubresourceKey layoutSubresourceKey(const AttachmentDesc& attachment);
LayoutSubresourceKey layoutSubresourceKey(const ResolvedRead& read);
bool isPassAttachmentSubresource(const RenderPassDesc& pass, const LayoutSubresourceKey& subresource);

} // namespace gfx_api
