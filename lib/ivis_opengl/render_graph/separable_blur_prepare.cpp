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
/** @file separable_blur_prepare.cpp
 * Shared downsample + H/V blur chain for lighting-prepare and post-effects.
 */

#include "separable_blur_prepare.h"

namespace gfx_api
{

void emitSeparableBlurPasses(BlueprintBuilder& builder, const RenderTopologySnapshot& snapshot,
	const SeparableBlurPrepareChainDesc& desc)
{
	// Routing is common to SSAO, SSR, and contact shadows.
	// Formats, clear values, validity, accumulation, and compose contracts remain caller-owned.
	const bool downsample = (snapshot.features & desc.downsampleFeature) != 0;
	PassId horizontalInput = desc.generatePass;
	PipelineSurfaceId verticalOutput = desc.rawSurface;
	if (downsample)
	{
		builder.beginPass(desc.downsamplePass, desc.downsampleName)
			.color(desc.coarseSurface, desc.blurLoadOp, AttachmentStoreOp::Store, desc.clearValue)
			.viewport(ViewportRule::ColorTarget)
			.readFrom(desc.generatePass, AttachmentRole::PrimaryColor);
		horizontalInput = desc.downsamplePass;
		verticalOutput = desc.coarseSurface;
	}

	builder.beginPass(desc.horizontalPass, desc.horizontalName)
		.color(desc.horizontalSurface, desc.blurLoadOp, AttachmentStoreOp::Store, desc.clearValue)
		.viewport(ViewportRule::ColorTarget)
		.readFrom(horizontalInput, AttachmentRole::PrimaryColor)
		.readFrom(PassId::ScenePrepass, AttachmentRole::Depth);

	builder.beginPass(desc.verticalPass, desc.verticalName)
		.color(verticalOutput, desc.blurLoadOp, AttachmentStoreOp::Store, desc.clearValue)
		.viewport(ViewportRule::ColorTarget)
		.readFrom(desc.horizontalPass, AttachmentRole::PrimaryColor)
		.readFrom(PassId::ScenePrepass, AttachmentRole::Depth);
}

} // namespace gfx_api
