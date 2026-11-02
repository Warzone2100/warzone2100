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
/** @file separable_blur_prepare.h
 * Shared downsample + H/V blur chain for lighting-prepare and post-effects.
 */

#pragma once

#include "blueprint.h"
#include "pipeline_surfaces.h"
#include "render_pass_id.h"
#include "topology.h"

namespace gfx_api
{

struct SeparableBlurPrepareChainDesc
{
	PassId generatePass;
	PassId downsamplePass;
	PassId horizontalPass;
	PassId verticalPass;
	PipelineSurfaceId rawSurface;
	PipelineSurfaceId horizontalSurface;
	PipelineSurfaceId coarseSurface;
	const char* downsampleName;
	const char* horizontalName;
	const char* verticalName;
	ClearValue clearValue;
	uint32_t downsampleFeature;
	/// Generate is emitted by the caller. This covers downsample + H + V.
	/// SSAO/contact keep Clear (unoccluded 1 if a pixel is skipped). SSR overwrites every pixel.
	AttachmentLoadOp blurLoadOp = AttachmentLoadOp::Clear;
};

void emitSeparableBlurPasses(BlueprintBuilder& builder, const RenderTopologySnapshot& snapshot,
	const SeparableBlurPrepareChainDesc& desc);

} // namespace gfx_api
