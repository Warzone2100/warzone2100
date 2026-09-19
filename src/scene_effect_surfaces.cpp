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
/** @file scene_effect_surfaces.cpp
 * Commit current SSAO/fog/range-ring settings into the gfx pipeline-surface catalog.
 */

#include "scene_effect_surfaces.h"

#include "display3d.h"
#include "ssao.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/gfx_api.h"
#include "lib/ivis_opengl/piestate.h"

bool applySceneEffectSurfaces()
{
	const ssao::SsaoSettings ssaoSettings = ssao::activeSettings();
	gfx_api::SceneEffectSurfaces cfg;
	cfg.ssao = ssaoSettings.enabled;
	cfg.ssaoGenerateDivisor = ssaoSettings.generateDivisor;
	cfg.ssaoBlurDivisor = ssaoSettings.blurDivisor;
	cfg.fog = pie_GetFogEnabled();
	cfg.rangeRings = rangeOnScreen;
	if (!gfx_api::context::get().setSceneEffectSurfaces(cfg))
	{
		debug(LOG_ERROR, "Failed to sync pipeline surfaces after scene-effect config change (ssao=%d fog=%d rangeRings=%d)",
			static_cast<int>(cfg.ssao), static_cast<int>(cfg.fog), static_cast<int>(cfg.rangeRings));
		return false;
	}
	return true;
}
