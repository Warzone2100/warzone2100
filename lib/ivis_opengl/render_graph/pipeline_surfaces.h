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
/** @file pipeline_surfaces.h
 * Named persistent render targets (`PipelineSurfaceId`) and backend surface registry.
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "attachment.h"
#include "../gfx_api_formats_def.h"

#include <nonstd/optional.hpp>

namespace gfx_api
{

struct abstract_texture;

/// <summary>
/// Named persistent render targets shared across passes (scene, swapchain, shadow map).
///
/// Backends register live `abstract_texture*` instances in `PipelineSurfaceRegistry`.
/// Blueprints and `BlueprintMaterializer` refer to surfaces by id, not raw pointers.
/// </summary>
enum class PipelineSurfaceId : uint8_t
{
	SceneColor,
	SceneMSAAColor,
	SceneDepth,
	ShadowMap,
	SwapchainColor,
	SwapchainMSAAColor,
	SwapchainDepth,
	/// Array size and invalid id sentinel.
	Count
};

/// <summary>
/// Role of a pipeline surface (drives format defaults and layout/barrier hints).
/// </summary>
enum class PipelineSurfaceUsage : uint8_t
{
	ColorResolve,
	ColorMSAA,
	DepthStencil,
	DepthOnly,
	/// Swapchain / default framebuffer presentable color (not shader-sampled).
	SwapchainPresent,
};

/// <summary>
/// Static description of a pipeline surface: usage class, pixel format, and sample count.
/// </summary>
struct PipelineSurfaceMeta
{
	PipelineSurfaceUsage usage = PipelineSurfaceUsage::ColorResolve;
	pixel_format format = pixel_format::invalid;
	/// Default 1; MSAA surfaces may override via `PipelineSurfaceRegistry::setSurfaceSamples`.
	uint32_t samples = 1;
};

/// Built-in defaults per `PipelineSurfaceId` (samples may be overridden at registration).
PipelineSurfaceMeta getPipelineSurfaceMeta(PipelineSurfaceId id);

/// <summary>
/// Fixed-size registry of persistent pipeline render targets owned by the gfx backend.
///
/// Populated at swapchain/scene/shadow setup (`registerSurface`); read during
/// `resolvePassDescription` and `BlueprintMaterializer`. Lives on `gfx_api::context`.
/// </summary>
class PipelineSurfaceRegistry
{
public:
	/// Bind a backend texture to a surface slot (non-owning pointer).
	void registerSurface(PipelineSurfaceId id, abstract_texture* surface);
	/// Clear a slot (e.g. on swapchain teardown).
	void invalidateSurface(PipelineSurfaceId id);
	abstract_texture* get(PipelineSurfaceId id) const;

	/// Runtime sample count override (e.g. `SceneMSAAColor` / `SceneDepth` when MSAA is on).
	void setSurfaceSamples(PipelineSurfaceId id, uint32_t samples);
	/// Effective meta: static defaults merged with `_samplesOverride`.
	PipelineSurfaceMeta meta(PipelineSurfaceId id) const;

	/// Reverse lookup for attachment classification (depth-sampled vs swapchain present).
	nonstd::optional<PipelineSurfaceId> findSurfaceId(abstract_texture* texture) const;

private:
	std::array<abstract_texture*, static_cast<size_t>(PipelineSurfaceId::Count)> _surfaces {};
	std::array<uint32_t, static_cast<size_t>(PipelineSurfaceId::Count)> _samplesOverride {};
};

/// Build an `AttachmentDesc` from a registered pipeline surface and load/clear ops.
AttachmentDesc makePipelineSurfaceAttachment(PipelineSurfaceId id,
	AttachmentLoadOp op = AttachmentLoadOp::Clear,
	ClearValue clear = ClearValue::colorClear());

} // namespace gfx_api
