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
/** @file ssao.h
 * Screen-space ambient occlusion (SSAO) game-side module.
 */

#pragma once

#include "lib/ivis_opengl/gfx_api.h"
#include "warzoneconfig.h"

namespace ssao
{

/// Discrete quality preset: on/off, sample/blur cost, and target-size divisors.
/// Analog look (radius, bias, intensity) stays in Tuning inside ssao.cpp.
struct SsaoSettings
{
	bool enabled = false;
	uint32_t generateDivisor = 1; // 1 = full res; 2 = half; 4 = quarter
	uint32_t blurDivisor = 1;     // >= generateDivisor (never upsample before blur)
	int sampleCount = 16;         // 8 or 16, <= gfx_api::SSAO_KERNEL_SIZE
	int blurTapPairs = 4;         // 2 → 5-tap, 4 → 9-tap
};

SsaoSettings settingsFor(SSAO_MODE mode);
SsaoSettings activeSettings();

void init();
void shutdown();

void recordGenerate(const gfx_api::RenderPassContext& passCtx);
void recordDownsample(const gfx_api::RenderPassContext& passCtx);
void recordBlurH(const gfx_api::RenderPassContext& passCtx);
void recordBlurV(const gfx_api::RenderPassContext& passCtx);
void recordCompose(const gfx_api::RenderPassContext& passCtx);

} // namespace ssao
