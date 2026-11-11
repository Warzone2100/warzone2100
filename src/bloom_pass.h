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
/** @file bloom_pass.h
 * Bloom: a dual-filter chain on the floating point scene, before the tonemap (see `appendSceneReferredPasses`).
 * Six 13-tap downsamples to 1/64 of the scene, five tent upsamples back up,
 * then the tonemap adds the result onto the scene.
 */

#pragma once

#include "lib/ivis_opengl/gfx_api.h"
#include "lib/ivis_opengl/gfx_pipelines.h"

namespace bloom_pass
{

struct Level
{
	float intensity; ///< strength of the bloom added onto the scene (0 = off)
	float threshold; ///< scene brightness above which light blooms
};

/// The current setting's bloom level
Level level();
float intensity();
/// Gain on additive effects' bright cores for the current setting (0 while bloom is off)
float additiveCoreBoost();

/// Downsample step i: reads the scene (i = 0) or level i - 1, writes level i
void recordDownsample(const gfx_api::RenderPassContext& passCtx);
/// Upsample step i: reads level i + 1, adds onto level i
void recordUpsample(const gfx_api::RenderPassContext& passCtx);
/// Fills the tonemap's bloom constants from its read `readIndex` (bloom level 0)
void fillTonemapBloom(const gfx_api::RenderPassContext& passCtx, uint32_t readIndex,
	gfx_api::constant_buffer_type<SHADER_SCENE_TONEMAP>& constants);

} // namespace bloom_pass
