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
/** @file gfx_api_legacy_pass_compat.h
 * `RenderPassDesc` factories for legacy shadow, scene, and swapchain pass entry points.
 */


#pragma once

#include "render_graph/render_pass.h"

namespace gfx_api
{

/// <summary>
/// Factories for standard frame passes expressed as `RenderPassDesc` (shadow, scene, swapchain).
///
/// Shared by Vulkan and OpenGL legacy `begin*Pass` entry points until the game frame path
/// fully migrates to `CachedRenderGraph` / blueprint materialization. Outputs use
/// `PipelineSurfaceId` attachment sources; callers run `resolvePassDescription` before `beginPass`.
/// </summary>
namespace legacy_pass
{

/// Shadow-map depth pass for cascade `cascadeIndex` (`ShadowMap` surface, array layer = cascade).
RenderPassDesc buildShadowCascadePassDesc(size_t cascadeIndex);

/// Scene color (+ optional MSAA resolve) and depth (`SceneMSAAColor`/`SceneColor`, `SceneDepth`).
RenderPassDesc buildScenePassDesc();

/// Final swapchain pass; `colorLoad`/`depthLoad` control clear vs load (UI overlay uses `Load`).
RenderPassDesc buildSwapchainPassDesc(AttachmentLoadOp colorLoad, AttachmentLoadOp depthLoad);

} // namespace legacy_pass
} // namespace gfx_api
