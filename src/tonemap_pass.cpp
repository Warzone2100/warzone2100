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
/** @file tonemap_pass.cpp
 * Brings scene referred color down to display range.
 *
 * Emitted only when the scene color format is floating point, which the HDR Lighting option selects.
 */

#include "tonemap_pass.h"

#include "bloom_pass.h"
#include "display3d_render_graph.h"
#include "display3d_render_internal.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/gfx_pipelines.h"

namespace tonemap_pass
{

void recordApply(const gfx_api::RenderPassContext& passCtx)
{
	ASSERT(passCtx.readCount() == 1 || passCtx.readCount() == 2, "SceneTonemap: expected the scene color (and the bloom)");
	gfx_api::abstract_texture* scene = passCtx.getRead(0);
	if (scene == nullptr)
	{
		return;
	}

	gfx_api::constant_buffer_type<SHADER_SCENE_TONEMAP> constants {};
	display3d_fillPassReadUvScaleClamp(passCtx, 0, constants.uvScaleClamp);
	// Where the shoulder starts: low enough to bend toward white, high enough to leave most of it untouched
	constants.tonemapKnee = 0.75f;
	constants.encodeOutput = gfx_api::context::get().linearLightingEnabled() ? 1.f : 0.f;
	gfx_api::abstract_texture* bloom = nullptr;
	if (passCtx.readCount() == 2)
	{
		bloom_pass::fillTonemapBloom(passCtx, 1, constants);
		bloom = passCtx.getRead(1);
	}
	display3d_drawFullscreenTriangle<gfx_api::SceneTonemapPSO>(constants, scene, bloom);
}

} // namespace tonemap_pass
