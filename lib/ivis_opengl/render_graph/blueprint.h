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
/** @file blueprint.h
 * Topology-stable pass graph blueprint types, `BlueprintBuilder`, and `RecordFuncTable`.
 */

#pragma once

#include "render_pass.h"
#include "render_pass_id.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <optional>
using std::optional;
using std::nullopt;

namespace gfx_api
{

/// <summary>
/// Draw-callback registry keyed by stable pass identity (`PassId`).
///
/// Attaches record callbacks to passes without storing them in
/// `PassGraphTopologyBlueprint`, so topology can be cached, hashed,
/// rematerialized on resize, and debug-dumped independently of game draw code.
///
/// `BlueprintMaterializer` merges this table with the blueprint at
/// materialize time; `CachedRenderGraph` keeps it across rematerializations.
/// </summary>
struct RecordFuncTable
{
	/// Per `PassId` record callbacks; empty slots are skipped at execute time.
	std::array<RenderPassDesc::RecordFunc, static_cast<size_t>(PassId::Count)> funcs{};

	/// Register the draw callback for a pass slot.
	void set(PassId id, RenderPassDesc::RecordFunc func)
	{
		funcs[static_cast<size_t>(id)] = std::move(func);
	}
};

/// <summary>
/// Declarative attachment slot for a blueprint pass (load/store/clear), before GPU textures exist.
///
/// Describes *what kind* of attachment to bind, not a resolved `AttachmentDesc`.
/// `BlueprintMaterializer` turns each slot into a concrete texture using the current
/// `RenderTopologySnapshot` (pipeline surfaces) or transient pool (offscreen targets).
/// </summary>
struct BlueprintAttachment
{
	/// How the materializer resolves this slot to a texture.
	enum class Target : uint8_t
	{
		/// Named surface from `PipelineSurfaceId` (scene, swapchain, shadow map, …).
		PipelineSurface,
		/// Per-frame pooled color target; dimensions come from the pass viewport.
		TransientColor,
		/// Per-frame pooled depth target; dimensions come from the pass viewport.
		TransientDepth,
	};

	Target target = Target::PipelineSurface;
	/// Used when `target == PipelineSurface`; ignored for transient targets.
	PipelineSurfaceId surfaceId = PipelineSurfaceId::SceneColor;
	AttachmentLoadOp loadOp = AttachmentLoadOp::Clear;
	AttachmentStoreOp storeOp = AttachmentStoreOp::Store;
	/// Shadow-map cascade layer; only for depth pipeline surfaces that use array layers.
	optional<uint32_t> arrayLayer;
	ClearValue clearValue = ClearValue::colorClear();
};

/// <summary>
/// Cross-pass read dependency declared in topology form (producer `PassId`, not pass index).
///
/// At materialize time, `BlueprintMaterializer` maps `producerPass` to the producer's
/// runtime `PassHandle` and emits a `ReadDesc` on the consumer pass. The compiler /
/// backend use that to insert barriers and to resolve inputs in `RenderPassContext`.
/// </summary>
struct BlueprintReadEdge
{
	/// Stable identity of the pass that wrote the attachment being read.
	PassId producerPass = PassId::Count;
	/// Which output of the producer (color, depth, resolve, …).
	AttachmentRole producerRole = AttachmentRole::PrimaryColor;
	/// Index into the producer's `colorAttachments` when `producerRole == Color`.
	uint32_t attachmentIndex = 0;
};

/// <summary>
/// Rule for choosing pass viewport width/height at materialize time.
///
/// Blueprint stores the rule only; pixel sizes come from `RenderTopologySnapshot`
/// (`drawableW`/`drawableH`, `sceneW`/`sceneH`, or shadow-map dimensions).
/// </summary>
enum class ViewportRule : uint8_t
{
	/// Swapchain / drawable size (`drawableW` x `drawableH`).
	Drawable,
	/// Offscreen scene color target size (`sceneW` x `sceneH`).
	SceneColorTarget,
	/// Shadow cascade map size; use `BlueprintPass::viewportCascadeIndex` to pick the cascade.
	DepthCascade,
};

/// <summary>
/// One node in the pass graph: attachments, viewport rule, and inbound read edges.
///
/// Carries a stable `PassId` and debug name but no draw callbacks (those live in
/// `RecordFuncTable`). Becomes one `RenderPassDesc` when materialized.
/// </summary>
struct BlueprintPass
{
	/// Stable pass identity; used to look up `RecordFuncTable` entries.
	PassId id = PassId::Count;
	std::string debugName;
	std::vector<BlueprintAttachment> colorAttachments;
	optional<BlueprintAttachment> depthAttachment;
	optional<BlueprintAttachment> resolveAttachment;
	ViewportRule viewportRule = ViewportRule::Drawable;
	/// Cascade index when `viewportRule == DepthCascade`.
	uint32_t viewportCascadeIndex = 0;
	/// Producer passes this pass reads from (compile-time topology edges).
	std::vector<BlueprintReadEdge> reads;
};

/// <summary>
/// Immutable pass-graph topology: pass list, attachments, and read edges only.
///
/// Contains no lambdas, viewport pixel sizes, or backend handles. Built from
/// `RenderTopologySnapshot` (see `fromSnapshot`) or `BlueprintBuilder`, then cached
/// by `topologyHash` in `CachedRenderGraph`. Rematerialized separately when
/// `materializeHash` changes (resize, backend epoch).
/// </summary>
class PassGraphTopologyBlueprint
{
public:
	/// Ordered pass sequence for this frame topology.
	const std::vector<BlueprintPass>& passes() const { return _passes; }
	/// Hash of pass ids, attachment layout, reads, and viewport rules (not dimensions).
	uint64_t topologyHash() const;

	/// Build the standard topology for a screen kind / feature set (see frame_blueprint.cpp).
	static PassGraphTopologyBlueprint fromSnapshot(const struct RenderTopologySnapshot& snapshot);

private:
	friend class BlueprintBuilder;
	std::vector<BlueprintPass> _passes;
};

/// Human-readable dump of pass order, attachments, and read edges (debug logging).
std::string dumpBlueprint(const PassGraphTopologyBlueprint& blueprint);

/// <summary>
/// Fluent builder for `PassGraphTopologyBlueprint` (structure only).
///
/// Call `beginPass`, attach color/depth/resolve slots, set viewport rule and
/// `readFrom` edges, then `build()`. Used by frame blueprint helpers and custom
/// topology setup; does not register `RecordFuncTable` callbacks.
/// </summary>
class BlueprintBuilder
{
public:
	/// Start a new pass; subsequent calls append to this pass until the next `beginPass`.
	BlueprintBuilder& beginPass(PassId id, std::string debugName);
	/// Add a color attachment bound to a pipeline surface.
	BlueprintBuilder& color(PipelineSurfaceId surface, AttachmentLoadOp load, AttachmentStoreOp store,
		ClearValue clear = ClearValue::colorClear());
	/// Add a depth attachment; `arrayLayer` selects a shadow-map cascade slice when needed.
	BlueprintBuilder& depth(PipelineSurfaceId surface, AttachmentLoadOp load, AttachmentStoreOp store,
		optional<uint32_t> arrayLayer = nullopt,
		ClearValue clear = ClearValue::depthStencilClear());
	/// Add an MSAA resolve target attachment.
	BlueprintBuilder& resolve(PipelineSurfaceId surface, AttachmentLoadOp load, AttachmentStoreOp store,
		ClearValue clear = ClearValue::colorClear());
	/// Set viewport sizing rule for the current pass.
	BlueprintBuilder& viewport(ViewportRule rule, uint32_t cascadeIndex = 0);
	/// Declare that the current pass reads an output from a prior pass (by `PassId`).
	BlueprintBuilder& readFrom(PassId producer, AttachmentRole role = AttachmentRole::PrimaryColor,
		uint32_t attachmentIndex = 0);

	/// Finish building; invalidates the builder's current-pass cursor.
	PassGraphTopologyBlueprint build();

private:
	std::vector<BlueprintPass> _passes;
	BlueprintPass* _current = nullptr;
};

/// Append the standard offscreen scene pass (MSAA or single-sample) to `builder`.
void addScenePassToBuilder(BlueprintBuilder& builder, PassId id, bool sceneMsaa);
/// Append a swapchain-target pass (MSAA resolve when enabled) with shared depth setup.
void addSwapchainPassToBuilder(BlueprintBuilder& builder, PassId id, std::string debugName,
	bool swapchainMsaa, AttachmentLoadOp colorLoad,
	optional<AttachmentLoadOp> depthLoadOverride = nullopt);

} // namespace gfx_api
