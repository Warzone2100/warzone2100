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

/// Ordered chain of post-`ScenePass` effects (before SMAA/blit). Array index matches the id.
enum class ScenePostEffectId : uint8_t
{
	Ssao,
	Fog,
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

/// What the apply pass samples. List index is the shader binding index.
enum class ApplyInput : uint8_t
{
	/// ScenePass color, or the previous effect's apply output.
	IncomingColor,
	PrepassDepth,
	PrepassNormals,
	/// Primary color of `ScenePostEffectDesc::preparedColorPass` (for example, blurred AO).
	PreparedOutput,
};

} // namespace gfx_api
