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
/** @file render_pass.h
 * `RenderPassDesc`, read descriptors, and `RenderPassContext` for graph pass recording.
 */

#pragma once

#include "attachment.h"
#include "pipeline_surfaces.h"
#include "render_pass_id.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <optional>
using std::optional;
using std::nullopt;

namespace gfx_api
{

struct abstract_texture;

/// Runtime index of a pass within a materialized `vector<RenderPassDesc>` (execution order).
using PassHandle = size_t;
static constexpr PassHandle INVALID_PASS_HANDLE = ~PassHandle(0);

/// <summary>
/// Which producer attachment a pass read refers to when `ReadSource::PassOutput`.
/// </summary>
enum class AttachmentRole : uint8_t
{
	/// Resolve target if MSAA resolve exists, else `colorAttachments[0]`.
	PrimaryColor,
	/// `colorAttachments[attachmentIndex]`.
	Color,
	/// `depthAttachment` (includes `arrayLayer` from producer).
	Depth,
	/// `resolveAttachment`.
	Resolve,
};

/// <summary>
/// How a `ReadDesc` input is resolved at compile / materialize time.
/// </summary>
enum class ReadSource : uint8_t
{
	/// Direct `abstract_texture*` on the read descriptor.
	ExplicitTexture,
	/// Named `PipelineSurfaceId` surface.
	PipelineSurface,
	/// Output of a prior pass in the same graph (`producerPass` + `AttachmentRole`).
	PassOutput,
};

/// <summary>
/// Declared input dependency for a pass (compile resolves to textures + layouts).
/// </summary>
struct ReadDesc
{
	ReadSource source = ReadSource::ExplicitTexture;
	/// Used when `source == ExplicitTexture`.
	abstract_texture* texture = nullptr;
	/// Used when `source == PipelineSurface`.
	PipelineSurfaceId pipelineSurface = PipelineSurfaceId::SceneColor;
	/// Used when `source == PassOutput` (materializer maps from `PassId` to this handle).
	PassHandle producerPass = INVALID_PASS_HANDLE;
	AttachmentRole producerRole = AttachmentRole::PrimaryColor;
	uint32_t attachmentIndex = 0;
	optional<uint32_t> arrayLayer;
	optional<uint32_t> mipLevel;
};

/// <summary>
/// Compile-time resolved read: concrete texture, subresource, and depth flag for barriers.
/// </summary>
struct ResolvedRead
{
	abstract_texture* texture = nullptr;
	uint32_t arrayLayer = 0;
	uint32_t mipLevel = 0;
	bool isDepth = false;
};

class RenderPassContext;

/// <summary>
/// Fully materialized render pass: GPU attachments, viewport, reads, and record callback.
///
/// Output of `BlueprintMaterializer`; consumed by `compilePassGraph` and
/// `executeCompiledRenderGraph`. `passId` retains stable `PassId` for debugging and callbacks.
/// </summary>
struct RenderPassDesc
{
	std::string debugName;

	/// Records draw calls for this pass; must only call gfx_api bind/draw while the pass is active.
	using RecordFunc = std::function<void(const RenderPassContext&)>;
	/// Draw callback; often populated from `RecordFuncTable` at materialize time.
	RecordFunc recordFunc;

	std::vector<AttachmentDesc> colorAttachments;
	optional<AttachmentDesc> depthAttachment;
	optional<AttachmentDesc> resolveAttachment;
	optional<std::pair<uint32_t, uint32_t>> viewportSize;
	std::vector<ReadDesc> reads;
	/// Stable pass identity; `PassId::Count` means unset.
	PassId passId = PassId::Count;
};

/// <summary>
/// Per-pass data passed to `RecordFunc` callbacks after reads are compile-resolved.
/// </summary>
class RenderPassContext
{
public:
	RenderPassContext(const RenderPassDesc& desc, std::vector<ResolvedRead> resolvedReads);

	/// Resolved input texture for `reads[index]` (barriers already emitted).
	abstract_texture* getRead(size_t index) const;
	/// Stable id from `RenderPassDesc::passId`.
	PassId passId() const { return _desc.passId; }

private:
	const RenderPassDesc& _desc;
	std::vector<ResolvedRead> _resolvedReads;
};

} // namespace gfx_api
