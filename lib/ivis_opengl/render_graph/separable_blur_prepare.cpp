// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
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
