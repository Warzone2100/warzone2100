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
/** @file scene_post_effect_id.h
 * Ids and capability flags for post-lighting in-game screen-space effects.
 *
 * Kept free of blueprint/topology types so the surface catalog can include it
 * without pulling `BlueprintBuilder`.
 */

#pragma once

#include <cstdint>

namespace gfx_api
{

/// Ordered chain after opaque `ScenePass` and before `SceneTransparent`. Array index matches the id.
enum class ScenePostEffectId : uint8_t
{
	Ssao,
	Ssr,
	Fog,
	RangeRings,
	Count
};

/// ScenePrepass attachments required by an enabled post-effect.
enum class PrepassNeed : uint8_t
{
	None    = 0,
	Depth   = 1u << 0,
	Normals = 1u << 1,
};

constexpr PrepassNeed operator|(PrepassNeed a, PrepassNeed b)
{
	return static_cast<PrepassNeed>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

constexpr PrepassNeed operator&(PrepassNeed a, PrepassNeed b)
{
	return static_cast<PrepassNeed>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

constexpr bool hasFlag(PrepassNeed value, PrepassNeed flag)
{
	return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0u;
}

} // namespace gfx_api
