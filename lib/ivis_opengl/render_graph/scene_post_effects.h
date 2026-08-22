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
#include <cstddef>

namespace gfx_api
{

/// One ordered shader input of a scene post-effect apply pass.
/// Fixed pass reads reuse the render graph's canonical BlueprintReadEdge shape;
/// IncomingColor is resolved dynamically from the previous enabled effect.
struct ScenePostEffectRead
{
	enum class Source : uint8_t
	{
		IncomingColor,
		PassOutput,
	};

	Source source = Source::IncomingColor;
	BlueprintReadEdge edge {};

	static constexpr ScenePostEffectRead incomingColor()
	{
		return {};
	}

	static constexpr ScenePostEffectRead passOutput(PassId producer,
		AttachmentRole role = AttachmentRole::PrimaryColor, uint32_t attachmentIndex = 0)
	{
		ScenePostEffectRead read;
		read.source = Source::PassOutput;
		read.edge.producerPass = producer;
		read.edge.producerRole = role;
		read.edge.attachmentIndex = attachmentIndex;
		return read;
	}
};

/// C++17-compatible immutable view over an exact-sized read declaration.
/// This deliberately has span semantics; ivis-opengl is not yet built as C++20,
/// so std::span cannot be used here.
class ScenePostEffectReadView
{
public:
	constexpr ScenePostEffectReadView() = default;

	template <size_t Size>
	constexpr ScenePostEffectReadView(const std::array<ScenePostEffectRead, Size>& reads)
		: data_(reads.data())
		, size_(Size)
	{}

	constexpr const ScenePostEffectRead* begin() const { return data_; }
	constexpr const ScenePostEffectRead* end() const { return size_ == 0 ? data_ : data_ + size_; }
	constexpr size_t size() const { return size_; }

private:
	const ScenePostEffectRead* data_ = nullptr;
	size_t size_ = 0;
};

/// One screen-space effect after opaque ScenePass and before forward transparents.
/// Table order of `applyPass` is the apply chain (SSAO compose -> fog -> rings).
/// FogApply intentionally belongs here: its sampled prepass depth identifies the
/// visible opaque surface only. Transparent layers use their own fragment distance
/// and must apply fog before blending; a later fullscreen pass cannot recover them.
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

	/// Ordered reads of `applyPass`; span index is the shader binding.
	/// Non-owning: backing storage must remain valid for the descriptor's lifetime.
	ScenePostEffectReadView applyReads {};
};

bool effectEnabled(const RenderTopologySnapshot& snapshot, ScenePostEffectId id);
/// True when at least one scene post-effect is enabled - i.e. something sits between opaque ScenePass and the transparents,
/// so the blueprint separates them into the SceneTransparent pass (and the prepass must provide depth).
bool anyScenePostEffectEnabled(const RenderTopologySnapshot& snapshot);
bool anyScenePostEffectEnabled(const SceneEffectSurfaces& cfg);
PrepassNeed prepassNeeds(const RenderTopologySnapshot& snapshot);
PrepassNeed prepassNeeds(const SceneEffectSurfaces& cfg);

void emitApplyPass(BlueprintBuilder& builder, const ScenePostEffectDesc& effect, PassId incomingColor);

extern const std::array<ScenePostEffectDesc, static_cast<size_t>(ScenePostEffectId::Count)> kScenePostEffects;

} // namespace gfx_api
