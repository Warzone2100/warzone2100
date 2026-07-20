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
/** @file render_pass_id.h
 * Stable `PassId` enum identifying pass slots across topology, callbacks, and caching.
 */

#pragma once

#include <cstdint>

namespace gfx_api
{

/// <summary>
/// Stable semantic identity for a render pass slot across topology, callbacks, and caching.
///
/// Pass vector index can change when optional passes are added (e.g. shadow cascades);
/// `PassId` is the key for `RecordFuncTable`, blueprint `BlueprintPass::id`, and
/// `RenderPassDesc::passId`. `Count` is the array bound and "unset" sentinel.
/// </summary>
enum class PassId : uint16_t
{
	Backdrop,
	ShadowCascade0,
	ShadowCascade1,
	ShadowCascade2,
	ShadowCascade3,
	ScenePass,
	SceneBlit,
	TargettingEffects,
	SceneOverlays,
	SceneDebugOverlays,
	GameStartFade,
	InGameUI,
	TitleUI,
	LoadingBackdrop,
	LoadingScreen,
	VideoPlayback,
	/// Array size and invalid/unset pass id (not a real pass).
	Count
};

/// Map shadow cascade index (`0` .. `WZ_SHADOW_CASCADES_COUNT` - 1) to the corresponding `PassId`.
inline PassId shadowCascadePassId(uint32_t cascadeIndex)
{
	constexpr uint32_t first = static_cast<uint32_t>(PassId::ShadowCascade0);
	constexpr uint32_t last = static_cast<uint32_t>(PassId::ShadowCascade3);
	const uint32_t value = first + cascadeIndex;
	if (value > last)
	{
		return PassId::Count;
	}
	return static_cast<PassId>(value);
}

/// Inverse of `shadowCascadePassId`; returns `UINT32_MAX` if `id` is outside the cascade `PassId` range.
inline uint32_t passIdToCascadeIndex(PassId id)
{
	const uint16_t base = static_cast<uint16_t>(PassId::ShadowCascade0);
	const uint16_t value = static_cast<uint16_t>(id);
	if (value < base || value > static_cast<uint16_t>(PassId::ShadowCascade3))
	{
		return UINT32_MAX;
	}
	return value - base;
}

} // namespace gfx_api
