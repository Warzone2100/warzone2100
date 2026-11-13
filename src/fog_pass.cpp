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
/** @file fog_pass.cpp
 * Deferred distance-fog fullscreen pass.
 */

#include "fog_pass.h"

#include "display3d_render_graph.h"
#include "display3d_render_internal.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/gfx_api.h"
#include "lib/ivis_opengl/piestate.h"

#include <glm/gtc/matrix_inverse.hpp>

namespace fog_pass
{

void recordApply(const gfx_api::RenderPassContext& passCtx)
{
	ASSERT(passCtx.readCount() == 2, "FogApply: 0 color, 1 prepass depth");
	gfx_api::abstract_texture* scene = passCtx.getRead(0);
	gfx_api::abstract_texture* depth = passCtx.getRead(1);
	if (scene == nullptr || depth == nullptr
		|| !pie_IsInGame3DFrameContextReady())
	{
		return;
	}

	const RENDER_STATE renderState = getCurrentRenderState();
	const auto& fc = pie_GetInGame3DFrameContext();

	gfx_api::constant_buffer_type<SHADER_SCENE_FOG> constants {};
	constants.fogColor = pie_SceneColourToVec4(pie_GetFogColour());
	constants.invProjectionMatrix = glm::inverse(fc.perspectiveMatrix);
	display3d_fillPassReadUvScaleClamp(passCtx, 1, constants.uvScaleClamp);
	constants.fogBegin = renderState.fogBegin;
	constants.fogEnd = renderState.fogEnd;
	constants.linearLighting = gfx_api::context::get().linearLightingEnabled() ? 1.f : 0.f;
	display3d_drawFullscreenTriangle<gfx_api::SceneFogPSO>(constants, scene, depth);
}

} // namespace fog_pass
