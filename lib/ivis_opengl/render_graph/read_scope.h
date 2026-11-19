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
/** @file read_scope.h
 * `ReadProducerScope` classification (in-graph vs external) for image barrier planning.
 */

#pragma once

#include "layout_subresource.h"
#include "render_pass.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace gfx_api
{

enum class CompileImageLayout : uint8_t;
struct SubresourceSyncState;

/// Whether a sampled read's producer is tracked in the compile-time layout timeline.
enum class ReadProducerScope : uint8_t
{
	/// Prior in-graph producer exists in the layout timeline.
	InGraph,
	/// Explicit barrier path; old layout from runtime `FrameLayoutTracker` (out-of-graph / legacy reads).
	External,
};

/// Compile-time layout state keyed by texture subresource; used by `planLayoutTimeline` and `classifyReadProducerScope`.
using LayoutStateMap = std::unordered_map<LayoutSubresourceKey, SubresourceSyncState, LayoutSubresourceKeyHash>;

/// Classify a read when planning barriers. Uses ReadDesc + layout timeline state.
/// Does NOT mutate ResolvedRead — scope is stored on ImageBarrierOp only.
ReadProducerScope classifyReadProducerScope(const ReadDesc& read,
	const std::vector<RenderPassDesc>& descs, size_t consumerIndex,
	const LayoutStateMap& layoutStateAtRead);

} // namespace gfx_api
