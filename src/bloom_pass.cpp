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
/** @file bloom_pass.cpp
 * Bloom record functions (see bloom_pass.h and bloom_filters.glsl).
 */

#include "bloom_pass.h"

#include "display3d_render_internal.h"
#include "warzoneconfig.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/gfx_pipelines.h"
#include "lib/ivis_opengl/render_graph/render_pass_id.h"

#include <algorithm>

namespace bloom_pass
{

// Tent radius, in texels of the level being read
constexpr float UPSAMPLE_RADIUS = 1.0f;

static glm::vec2 inverseTextureSize(gfx_api::abstract_texture* texture)
{
	const auto dims = gfx_api::context::get().getRenderTargetDimensions(texture)
		.value_or(std::pair<uint32_t, uint32_t>{1, 1});
	return glm::vec2(1.f / static_cast<float>(std::max<uint32_t>(dims.first, 1)),
		1.f / static_cast<float>(std::max<uint32_t>(dims.second, 1)));
}

Level level()
{
	// High's lower threshold lets the brightest sunlit highlights bloom too
	switch (war_getBloomMode())
	{
		case BLOOM_MODE::OFF: return {0.f, 1.f};
		case BLOOM_MODE::LOW: return {0.4f, 1.f};
		case BLOOM_MODE::MEDIUM: return {0.8f, 1.f};
		case BLOOM_MODE::HIGH: return {1.2f, 0.85f};
	}
	return {0.f, 1.f};
}

float intensity()
{
	return level().intensity;
}

float additiveCoreBoost()
{
	return (intensity() > 0.f) ? 1.5f : 0.f;
}

void recordDownsample(const gfx_api::RenderPassContext& passCtx)
{
	ASSERT_OR_RETURN(, passCtx.readCount() == 1, "BloomDownsample: expected one read");
	gfx_api::abstract_texture* source = passCtx.getRead(0);
	if (source == nullptr)
	{
		return;
	}
	gfx_api::constant_buffer_type<SHADER_BLOOM_DOWNSAMPLE> constants {};
	display3d_fillPassReadUvScaleClamp(passCtx, 0, constants.uvScaleClamp);
	const glm::vec2 texel = inverseTextureSize(source);
	// Only the first step, reading the scene, thresholds
	const bool first = passCtx.passId() == gfx_api::PassId::BloomDownsample0;
	constants.params = glm::vec4(texel.x, texel.y, 0.f, first ? level().threshold : 0.f);
	display3d_drawFullscreenTriangle<gfx_api::BloomDownsamplePSO>(constants, source);
}

void recordUpsample(const gfx_api::RenderPassContext& passCtx)
{
	ASSERT_OR_RETURN(, passCtx.readCount() == 1, "BloomUpsample: expected one read");
	gfx_api::abstract_texture* source = passCtx.getRead(0);
	if (source == nullptr)
	{
		return;
	}
	gfx_api::constant_buffer_type<SHADER_BLOOM_UPSAMPLE> constants {};
	display3d_fillPassReadUvScaleClamp(passCtx, 0, constants.uvScaleClamp);
	const glm::vec2 texel = inverseTextureSize(source);
	constants.params = glm::vec4(texel.x, texel.y, UPSAMPLE_RADIUS, 0.f);
	display3d_drawFullscreenTriangle<gfx_api::BloomUpsamplePSO>(constants, source);
}

void fillTonemapBloom(const gfx_api::RenderPassContext& passCtx, uint32_t readIndex,
	gfx_api::constant_buffer_type<SHADER_SCENE_TONEMAP>& constants)
{
	gfx_api::abstract_texture* level0 = passCtx.getRead(readIndex);
	if (level0 == nullptr)
	{
		return;
	}
	display3d_fillPassReadUvScaleClamp(passCtx, readIndex, constants.bloomUvScaleClamp);
	const glm::vec2 texel = inverseTextureSize(level0);
	// Level 0 holds the sum of every level
	constants.bloomParams = glm::vec4(texel.x, texel.y, UPSAMPLE_RADIUS,
		intensity() / static_cast<float>(gfx_api::BLOOM_LEVELS));
}

} // namespace bloom_pass
