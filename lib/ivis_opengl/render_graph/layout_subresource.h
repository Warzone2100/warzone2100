// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
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
