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

#pragma once

#include "display3d_render_internal.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/gfx_api.h"

#include <algorithm>
#include <utility>

namespace post_effect_blur
{

enum class Axis
{
	Horizontal,
	Vertical,
};

template<SHADER_MODE Shader, typename BlurPSO>
void recordDepthAwareBlur(const gfx_api::RenderPassContext& passCtx, Axis axis,
	float depthSigma, uint32_t tapPairs, const char* debugName)
{
	ASSERT(passCtx.readCount() == 2, "%s: value + depth", debugName);
	gfx_api::abstract_texture* value = passCtx.getRead(0);
	gfx_api::abstract_texture* depth = passCtx.getRead(1);
	if (value == nullptr || depth == nullptr)
	{
		return;
	}

	const auto used = passCtx.writeViewportSize().value_or(std::pair<uint32_t, uint32_t>{1, 1});
	const float usedW = static_cast<float>(std::max(used.first, 1u));
	const float usedH = static_cast<float>(std::max(used.second, 1u));

	gfx_api::constant_buffer_type<Shader> constants {};
	constants.blurDirection = axis == Axis::Horizontal
		? glm::vec2(1.0f / usedW, 0.0f)
		: glm::vec2(0.0f, 1.0f / usedH);
	constants.depthSigma = depthSigma;
	constants.tapPairs = static_cast<float>(tapPairs);
	display3d_fillPassReadUvScaleClamp(passCtx, 0, constants.valueUvScaleClamp);
	display3d_fillPassReadUvScaleClamp(passCtx, 1, constants.depthUvScaleClamp);

	display3d_drawFullscreenTriangle<BlurPSO>(constants, value, depth);
}

inline void recordBilinearResample(const gfx_api::RenderPassContext& passCtx, const char* debugName)
{
	ASSERT(passCtx.readCount() == 1, "%s: source value", debugName);
	gfx_api::abstract_texture* source = passCtx.getRead(0);
	if (source == nullptr)
	{
		return;
	}

	gfx_api::constant_buffer_type<SHADER_BILINEAR_RESAMPLE> constants {};
	display3d_fillPassReadUvScaleClamp(passCtx, 0, constants.uvScaleClamp);
	display3d_drawFullscreenTriangle<gfx_api::BilinearResamplePSO>(constants, source);
}

} // namespace post_effect_blur
