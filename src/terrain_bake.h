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

/** @file terrain_bake.h
 *  Bakes the smooth terrain surface's outputs (height, horizontal outline
 *  offset, normal) into textures for the hardware-tessellation terrain
 *  strategy. The CPU surface (terrain_surface.*) stays the single source of
 *  truth: the tessellation evaluation shader is a trivial sampler of these
 *  fields, so unit ground-settle parity holds by construction.
 */

#ifndef __INCLUDED_SRC_TERRAIN_BAKE_H__
#define __INCLUDED_SRC_TERRAIN_BAKE_H__

struct WorldMapState;
namespace gfx_api { struct texture; }

namespace terrainBake
{
	/// Bake-grid density. Must match WZ_BAKE_SAMPLES_PER_TILE in the
	/// terrain_tess.glsl shaders. TILE_UNITS must be divisible by this.
	constexpr int BAKE_SAMPLES_PER_TILE = 8;

	/// Full bake of the whole map. (Re)creates the field textures.
	/// Returns false on allocation failure.
	bool bakeFields(WorldMapState& mapState);

	/// Re-bake the field texels influenced by changes to the inclusive tile
	/// rect, and sub-upload them. The rect is expanded internally to the
	/// surface's influence radius, so pass just the changed tiles.
	void rebakeTileRegion(WorldMapState& mapState, int minTileX, int minTileY, int maxTileX, int maxTileY);

	/// Free the field textures (safe to call when never baked).
	void shutdown();

	// The baked field textures. All encodings are linear maps, so hardware
	// bilinear filtering of the encoded texels filters the decoded fields
	// exactly (the TES uses single filtered fetches).
	/// R16_UNORM: (height + 2048) / 4096
	gfx_api::texture* heightTexture();
	/// RG16_UNORM: (offset + 64) / 128 for the horizontal outline offset x/y
	gfx_api::texture* offsetTexture();
	/// RG8_UNORM: normal xz * 0.5 + 0.5 (y reconstructed in the shader)
	gfx_api::texture* normalTexture();
}

#endif // __INCLUDED_SRC_TERRAIN_BAKE_H__
