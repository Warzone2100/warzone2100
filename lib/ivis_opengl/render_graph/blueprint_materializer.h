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
/** @file blueprint_materializer.h
 * Turns a topology blueprint into executable `RenderPassDesc` lists for one frame.
 */

#pragma once

#include "blueprint.h"
#include "topology.h"

#include <unordered_map>
#include <utility>
#include <vector>

namespace gfx_api
{

/// <summary>
/// Turns a `PassGraphTopologyBlueprint` into executable `RenderPassDesc` list for one frame.
///
/// Binds blueprint attachment slots and viewport rules to concrete textures and pixel sizes
/// using a `RenderTopologySnapshot`, and attaches draw callbacks from `RecordFuncTable`.
/// Called on every materialize (including resize); does not cache results itself
/// (`CachedRenderGraph` owns caching).
/// </summary>
class BlueprintMaterializer
{
public:
	/// Snapshot supplies drawable/scene/shadow dimensions and backend epoch for this frame.
	explicit BlueprintMaterializer(const RenderTopologySnapshot& snapshot);

	/// Produce ordered pass descriptions ready for `compilePassGraph` / execution.
	std::vector<RenderPassDesc> materialize(const PassGraphTopologyBlueprint& blueprint,
		const RecordFuncTable& recordFuncs) const;

private:
	AttachmentDesc resolveAttachment(const BlueprintAttachment& attachment) const;
	std::pair<uint32_t, uint32_t> resolveViewport(const BlueprintPass& pass) const;

	const RenderTopologySnapshot& _snapshot;
};

} // namespace gfx_api
