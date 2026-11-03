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
/** @file contact_shadows.h
 * Screen-space contact shadows game-side module (sun directional, lighting-prepare).
 */

#pragma once

#include "lib/ivis_opengl/gfx_api.h"
#include "warzoneconfig.h"

namespace contact_shadows
{

/// Discrete quality preset: on/off, march/blur cost, and target-size divisors.
/// Analog look (ray length, thickness, fades) stays in Tuning inside contact_shadows.cpp.
struct ContactShadowSettings
{
	bool enabled = false;
	uint32_t generateDivisor = 1;
	uint32_t blurDivisor = 1;
	int stepCount = 16;
	int blurTapPairs = 4;
};

ContactShadowSettings settingsFor(CONTACT_SHADOWS_MODE mode);
ContactShadowSettings activeSettings();

void init();
void shutdown();
void syncLightingConstant();
gfx_api::texture* dummyLitTexture();

void recordGenerate(const gfx_api::RenderPassContext& passCtx);
void recordDownsample(const gfx_api::RenderPassContext& passCtx);
void recordBlurH(const gfx_api::RenderPassContext& passCtx);
void recordBlurV(const gfx_api::RenderPassContext& passCtx);

} // namespace contact_shadows
