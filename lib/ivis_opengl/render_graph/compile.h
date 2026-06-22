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
/** @file compile.h
 * Compiled pass artifacts, execution batches, and `compilePassGraph` API.
 */

#pragma once

#include "read_scope.h"
#include "render_pass.h"

#include <cstdint>
#include <vector>

#include <optional>

namespace gfx_api
{

/// <summary>
/// Backend-agnostic image layout state tracked across pass graph compilation.
///
/// Drives `ImageBarrierOp` planning and Vulkan `PassLayoutKey` final layouts.
/// </summary>
enum class CompileImageLayout : uint8_t
{
	Undefined,
	ShaderReadOnly,
	DepthReadOnly,
	ColorAttachment,
	DepthAttachment,
	Present,
};

/// Layout transition to emit before a pass records (per subresource).
struct ImageBarrierOp
{
	abstract_texture* texture = nullptr;
	uint32_t arrayLayer = 0;
	uint32_t mipLevel = 0;
	bool isDepth = false;
	/// Whether the producer of `texture` is tracked in the compile-time layout timeline.
	/// External: backend resolves `oldLayout` from the runtime tracker and emits an
	/// execution/memory barrier even when old == new layout. InGraph: prior layout is
	/// known at compile time; render-pass subpass dependencies cover matching layouts.
	ReadProducerScope producerScope = ReadProducerScope::InGraph;
	CompileImageLayout oldLayout = CompileImageLayout::Undefined;
	CompileImageLayout newLayout = CompileImageLayout::Undefined;
};

/// Layout state to record after a pass completes (feeds next pass barrier planning).
struct LayoutStateUpdate
{
	abstract_texture* texture = nullptr;
	uint32_t arrayLayer = 0;
	uint32_t mipLevel = 0;
	CompileImageLayout layout = CompileImageLayout::Undefined;
};

/// <summary>
/// Per-pass attachment final layouts derived at compile time.
///
/// Stored on `CompiledPass` and consumed by Vulkan backend when building render passes.
/// </summary>
struct CompiledPassLayoutMetadata
{
	/// Layout each attachment is in as the pass begins (RP initialLayout).
	/// Undefined => contents discarded (Clear/DontCare) or first use.
	CompileImageLayout depthInitialLayout = CompileImageLayout::Undefined;
	std::vector<CompileImageLayout> colorInitialLayouts;
	std::optional<CompileImageLayout> resolveInitialLayout;

	/// Layout each attachment is left in once the pass ends (RP finalLayout).
	CompileImageLayout depthFinalLayout = CompileImageLayout::DepthAttachment;
	std::vector<CompileImageLayout> colorFinalLayouts;
	std::optional<CompileImageLayout> resolveFinalLayout;
};

/// <summary>
/// One pass after graph compilation: resolved reads, barriers, and layout metadata.
///
/// When `skipped == false`, `desc` is the fully resolved `RenderPassDesc` (equals
/// `descs[graphIndex]` after compilePassGraph). When `skipped == true`, `desc` is
/// default-empty and the pass is omitted from executionBatches; it must not be referenced
/// by any successfully compiled PassOutput read.
/// </summary>
struct CompiledPass
{
	RenderPassDesc desc;
	/// Index in the original `descs` vector passed to `compilePassGraph`.
	size_t graphIndex = 0;
	bool skipped = false;
	/// Inputs resolved from `desc.reads` for `RenderPassContext::getRead`.
	std::vector<ResolvedRead> resolvedReads;
	std::vector<ImageBarrierOp> prePassBarriers;
	std::vector<LayoutStateUpdate> postPassLayoutUpdates;
	CompiledPassLayoutMetadata renderPassLayouts;
};

/// Contiguous slice of `PassGraphCompileResult::passes` executed as one backend render pass.
/// Indices refer to the compiled pass vector (includes skipped passes at their materialize indices,
/// but skipped passes never appear in any batch).
struct ExecutionBatch
{
	size_t startIndex = 0;
	size_t count = 0;
};

/// <summary>
/// Compiled render-graph artifact produced once per topology/materialize rebuild.
///
/// `passes` mirrors the materialized `RenderPassDesc` list (same order and count); each
/// `CompiledPass` holds pre-resolved reads, layout barriers, and attachment final layouts
/// so execution does not repeat that work every frame. Consumed by `warmCompiledRenderGraph`
/// (Vulkan warm render-pass layout ids on `VkRoot`) and `executeCompiledRenderGraph`
/// (fast path when no transients). Cached on `CachedRenderGraph` and reused until
/// `compilePassGraph` runs again.
/// </summary>
struct PassGraphCompileResult
{
	std::vector<CompiledPass> passes;
	/// Precomputed execution batches; populated by assignExecutionBatches in compilePassGraph.
	/// Empty when all passes are skipped or compile failed before assignment.
	std::vector<ExecutionBatch> executionBatches;
};

/// Resolve attachments, plan layout transitions, and fill `out` (one `CompiledPass` per input desc).
bool compilePassGraph(std::vector<RenderPassDesc>& descs, PassGraphCompileResult& out);

/// Build the `RenderPassContext` passed to a pass `RecordFunc` at execute time.
RenderPassContext buildRenderPassContext(const CompiledPass& compiledPass);

} // namespace gfx_api
