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
/** @file pass_resolve.h
 * Pass attachment resolution, MSAA helpers, and execution batch assignment API.
 */

#pragma once

#include "attachment.h"
#include "compile.h"
#include "render_pass.h"

#include <unordered_set>

#include <optional>

namespace gfx_api
{

struct RenderPassDesc;

/// <summary>
/// Resolved view of one producer pass output (texture + subresource flags).
///
/// Returned by `getPassOutputAttachment` when resolving `ReadSource::PassOutput` reads.
/// </summary>
struct PassOutputView
{
	abstract_texture* texture = nullptr;
	uint32_t arrayLayer = 0;
	uint32_t mipLevel = 0;
	bool isDepth = false;
	bool isMultisampled = false;
};

/// <summary>
/// Materialize a `RenderPassDesc` for GPU recording: resolve attachment sources
/// and set `viewportSize` when inferrable.
///
/// Single entry point used by legacy pass factories and `compilePassGraph` (via
/// `resolvePassDescription` before compile). Mutates `pass` in place; returns false
/// if resolution fails (missing surface, invalid dimensions).
/// </summary>
bool resolvePassDescription(RenderPassDesc& pass);

/// Effective store op; unset attachments default to `Store`.
AttachmentStoreOp attachmentStoreOpOr(const AttachmentDesc& attachment);

/// True when the pass targets swapchain color (pre- or post-MSAA resolve).
bool passTargetsSwapchainColor(const RenderPassDesc& pass);

/// True when `candidate` can share one backend render-pass instance with `batchHead`.
/// Used only by assignExecutionBatches during compilePassGraph.
bool canExtendPassBatch(const CompiledPass& batchHead, const CompiledPass& candidate);

/// Partition non-skipped compiled passes into execution batches.
/// Called from compilePassGraph after planLayoutTimeline.
void assignExecutionBatches(const std::vector<CompiledPass>& passes,
	std::vector<ExecutionBatch>& outBatches);

/// True when the pass has no color attachments and a resolved depth attachment.
bool passIsDepthOnly(const RenderPassDesc& pass);

/// True when `resolveAttachment` is set and the first color attachment is multisampled.
bool passNeedsMsaaResolve(const RenderPassDesc& pass);

/// True when resolved depth includes stencil (scene depth, not shadow map).
bool attachmentDepthHasStencil(const AttachmentDesc& attachment);

/// True when the texture is the shadow-map pipeline surface.
bool isDepthShaderSampledSurface(abstract_texture* texture);

/// True when the texture is the swapchain presentable color surface.
bool isSwapchainPresentableColorSurface(abstract_texture* texture);

/// All non-null color, depth, and resolve textures referenced by the pass.
std::unordered_set<abstract_texture*> getPassAttachmentTextures(const RenderPassDesc& pass);

/// Resolve which texture/subresource a producer pass exposes for a given `AttachmentRole`.
std::optional<PassOutputView> getPassOutputAttachment(
	const RenderPassDesc& producer,
	AttachmentRole role,
	uint32_t colorIndex = 0);

} // namespace gfx_api
