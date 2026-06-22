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
/** @file legacy_pass_layout_commit.h
 * Captures and commits legacy pass final layouts into `FrameLayoutTracker`.
 */

#pragma once

#if defined(WZ_VULKAN_ENABLED)

#include "vk/vulkan_hpp_include.h"

#include "render_graph/render_pass.h"
#include "vk/legacy_layout_final.h"
#include "vk/pass_layout_key.h"

#include <vector>

namespace gfx_api::vk
{

class FrameLayoutTracker;

/// <summary>
/// Captures and commits final attachment layouts for out-of-graph (legacy) passes.
///
/// Graph passes use `CompiledPass::postPassLayoutUpdates` instead. `captureFromPassKey`
/// runs in `beginPass` when `compiledPass == null`; `commitTo` runs in `endPass`.
/// </summary>
class LegacyPassLayoutCommit
{
public:
	void clear();
	/// Records final layouts from a freshly built `PassLayoutKey` (out-of-graph path only).
	void captureFromPassKey(const gfx_api::RenderPassDesc& pass, const PassLayoutKey& key);
	/// Commits captured finals via `FrameLayoutTracker::commitLegacyFinalLayouts` and clears.
	void commitTo(FrameLayoutTracker& tracker);

private:
	std::vector<LegacyLayoutFinal> _finals;
};

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
