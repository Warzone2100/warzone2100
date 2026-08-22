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
/** @file scene_post_effects.h
 * Descriptor table for screen-space effects after opaque ScenePass and before SceneTransparent.
 */

#pragma once

#include "blueprint.h"
#include "pipeline_surfaces.h"
#include "render_pass_id.h"
#include "scene_post_effect_id.h"
#include "topology.h"

#include <array>

namespace gfx_api
{

/// One screen-space effect after opaque ScenePass and before forward transparents.
/// Table order of `applyPass` is the apply chain (SSAO compose -> fog -> rings).
///
/// Two phases, both optional:
/// - emitPreparePasses: offscreen subgraph that writes intermediates (AO, packed SDF).
///   Does not write scene color. Fog leaves this null.
/// - applyPass: fullscreen pass that samples IncomingColor plus extras.
struct ScenePostEffectDesc
{
	ScenePostEffectId id = ScenePostEffectId::Count;

	/// ScenePrepass attachments this effect needs when enabled (OR'd across the table).
	PrepassNeed prepassNeed = PrepassNeed::None;

	/// Optional subgraph after opaque ScenePass and before this effect's apply pass.
	/// Writes intermediate surfaces the apply pass samples; does not write scene color.
	void (*emitPreparePasses)(BlueprintBuilder&, const RenderTopologySnapshot&) = nullptr;

	/// Fullscreen pass that applies the effect to incoming scene color. `PassId::Count` = none.
	PassId applyPass = PassId::Count;
	/// `BlueprintPass::debugName` / `beginPass` string for `applyPass`.
	const char* applyDebugName = nullptr;
	/// Color attachment the apply pass writes; becomes the next incoming scene color.
	PipelineSurfaceId applyOutput = PipelineSurfaceId::Count;

	/// Ordered samples of `applyPass`; index is the shader binding.
	std::array<ApplyInput, 4> applyInputs {};
	uint8_t applyInputCount = 0;

	/// Pass whose primary color is sampled for `ApplyInput::PreparedOutput`.
	PassId preparedColorPass = PassId::Count;
};

bool effectEnabled(const RenderTopologySnapshot& snapshot, ScenePostEffectId id);
PrepassNeed prepassNeeds(const RenderTopologySnapshot& snapshot);
PrepassNeed prepassNeeds(const SceneEffectSurfaces& cfg);

void emitApplyPass(BlueprintBuilder& builder, const ScenePostEffectDesc& effect, PassId incomingColor);

extern const std::array<ScenePostEffectDesc, static_cast<size_t>(ScenePostEffectId::Count)> kScenePostEffects;

} // namespace gfx_api
