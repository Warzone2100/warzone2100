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
/** @file
 *  Light thrown by effects, configured from the data files.
 */

#pragma once

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/pietypes.h"

#include <nonstd/optional.hpp>

struct iIMDShape;
struct WEAPON_STATS;

/// The light a projectile throws while it is in flight
struct ProjectileLight
{
	PIELIGHT color;
	UDWORD range = 0;
	float intensity = 1.f;
};

bool loadEffectLights(const char *pFileName);
void effectLightsShutDown();

/// Re-reads the file the settings were last loaded from
void reloadEffectLights();

/// Return the light that a projectile of psStats (drawn with pIMD) throws, or nullopt when it throws none
nonstd::optional<ProjectileLight> resolveProjectileLight(const WEAPON_STATS *psStats, const iIMDShape *pIMD, bool modelIsGlowing);

/// Whether any part of the in-flight graphic is drawn as a glow (additive or premultiplied), so it lights the world.
/// Mirrors the additive decision in renderProjectile (which must stay in sync).
bool projectileGraphicGlows(const WEAPON_STATS *psStats, const iIMDShape *pFirstIMD);

/// The light thrown by an in-flight projectile of psStats, resolving glow and plume scale from its graphic.
/// Returns nullopt when the projectile throws no light.
nonstd::optional<ProjectileLight> resolveInFlightProjectileLight(const WEAPON_STATS *psStats, const iIMDShape *pFirstIMD);
