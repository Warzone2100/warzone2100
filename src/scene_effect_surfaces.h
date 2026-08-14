// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/
/** @file scene_effect_surfaces.h
 * Commit current SSAO/fog settings into the gfx pipeline-surface catalog.
 */

#pragma once

/// Push current SSAO + fog settings into the gfx catalog and sync once.
/// Call after any contributing setting changes (options, keybind, init3DView).
bool applySceneEffectSurfaces();
