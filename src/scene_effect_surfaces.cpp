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
/** @file scene_effect_surfaces.cpp
 * Commit current SSAO/SSR/fog/range-ring settings into the gfx pipeline-surface catalog.
 */

#include "scene_effect_surfaces.h"

#include "bloom_pass.h"
#include "display3d.h"
#include "ssao.h"
#include "ssr.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/gfx_api.h"
#include "lib/ivis_opengl/piestate.h"

bool applySceneEffectSurfaces()
{
	const ssao::SsaoSettings ssaoSettings = ssao::activeSettings();
	const ssr::SsrSettings ssrSettings = ssr::activeSettings();
	gfx_api::SceneEffectSurfaces cfg;
	cfg.ssao = ssaoSettings.enabled;
	cfg.ssaoResolution = {ssaoSettings.generateDivisor, ssaoSettings.blurDivisor};
	cfg.ssr = ssrSettings.enabled;
	cfg.ssrResolution = {ssrSettings.generateDivisor, ssrSettings.blurDivisor};
	cfg.fog = pie_GetFogEnabled();
	cfg.rangeRings = rangeOnScreen;
	cfg.bloom = bloom_pass::intensity() > 0.f;
	pie_SetAdditiveCoreBoost(bloom_pass::additiveCoreBoost());
	if (!gfx_api::context::get().setSceneEffectSurfaces(cfg))
	{
		debug(LOG_ERROR, "Failed to sync pipeline surfaces after scene-effect config change (ssao=%d ssr=%d fog=%d rangeRings=%d)",
			static_cast<int>(cfg.ssao), static_cast<int>(cfg.ssr),
			static_cast<int>(cfg.fog), static_cast<int>(cfg.rangeRings));
		return false;
	}
	return true;
}
