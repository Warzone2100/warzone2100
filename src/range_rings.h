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
/** @file range_rings.h
 * Instanced cone SDF range rings (generate + terrain composite).
 */

#pragma once

#include "lib/ivis_opengl/gfx_api.h"

namespace range_rings
{

bool init();
void shutdown();

void recordSdfSensor(const gfx_api::RenderPassContext& passCtx);
void recordSdfWeapon(const gfx_api::RenderPassContext& passCtx);
void recordSdfMin(const gfx_api::RenderPassContext& passCtx);
void recordComposite(const gfx_api::RenderPassContext& passCtx);

/// CPU hook for the unused debug API (weapon channel). radius <= 0 clears.
void setDebugRange(float centerX, float centerYGame, float radius);

} // namespace range_rings
