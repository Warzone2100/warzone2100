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

/** @file terrain_bake.cpp
 *  See terrain_bake.h. Field encodings (must match terrain_tess.glsl):
 *   height texture (R16_UNORM):  (height + 2048) * 16 / 65535
 *   offset texture (RG16_UNORM): (offset + 64) / 128
 *   normal texture (RG8_UNORM):  normal xz * 0.5 + 0.5
 *  (Tile heights are int32 and CAN be negative in real map data, so the height
 *  encoding is biased. [-2048, 2048) at 1/16-unit precision covers everything.)
 *  Every encoding is a linear map, so hardware bilinear filtering of the
 *  encoded texels filters the decoded fields exactly.
 *  One texel per bake-grid lattice point: world (x, y) = texel * (TILE_UNITS / BAKE_SAMPLES_PER_TILE).
 */

#include "terrain_bake.h"
#include "terrain_surface.h"
#include "map.h"
#include "lib/framework/frame.h"
#include "lib/framework/math_ext.h"
#include "lib/framework/wzapp.h"
#include "lib/ivis_opengl/gfx_api.h"
#include "lib/ivis_opengl/pietypes.h"

#include <algorithm>
#include <vector>

namespace
{
	constexpr int BAKE = terrainBake::BAKE_SAMPLES_PER_TILE;
	static_assert(TILE_UNITS % BAKE == 0, "bake-grid step must be an exact integer");
	constexpr float BAKE_STEP = static_cast<float>(TILE_UNITS) / BAKE;
	constexpr float HEIGHT_SCALE = 16.f;              // height16 = (height + HEIGHT_BIAS) * 16
	constexpr float HEIGHT_BIAS = 2048.f;             // heights encoded from [-2048, 2048)
	constexpr float OFFSET_BIAS = TILE_UNITS / 2.f;   // offsets encoded from [-64, 64)
	/// worldNormalAt's sampling radius (0.5 tiles) in bake texels - the in-grid
	/// finite difference below reproduces it exactly at this spacing
	constexpr int NORMAL_EPS_TEXELS = BAKE / 2;

	/// A raw (owned-buffer) image for uploading non-8-bit-per-channel formats
	class WzRawFieldImage final : public iV_BaseImage
	{
	public:
		bool allocate(unsigned int width, unsigned int height, gfx_api::pixel_format format)
		{
			m_width = width;
			m_height = height;
			m_format = format;
			m_data.resize(gfx_api::format_memory_size(format, width, height));
			return !m_data.empty();
		}
		unsigned int width() const override { return m_width; }
		unsigned int height() const override { return m_height; }
		gfx_api::pixel_format pixel_format() const override { return m_format; }
		const unsigned char* data() const override { return m_data.data(); }
		size_t data_size() const override { return m_data.size(); }
		unsigned int bufferRowLength() const override { return m_width; }
		unsigned int bufferImageHeight() const override { return m_height; }
		unsigned char* dataWritable() { return m_data.data(); }
	private:
		unsigned int m_width = 0;
		unsigned int m_height = 0;
		gfx_api::pixel_format m_format = gfx_api::pixel_format::invalid;
		std::vector<unsigned char> m_data;
	};

	gfx_api::texture* heightTex = nullptr;
	gfx_api::texture* offsetTex = nullptr;
	gfx_api::texture* normalTex = nullptr;
	int bakedTexWidth = 0;
	int bakedTexHeight = 0;

	inline uint16_t encode16(float value01)
	{
		return static_cast<uint16_t>(clip<int>(static_cast<int>(value01 * 65535.f + 0.5f), 0, 65535));
	}

	inline unsigned char encodeSNorm8(float value)
	{
		return static_cast<unsigned char>(clip<int>(static_cast<int>((value * 0.5f + 0.5f) * 255.f + 0.5f), 0, 255));
	}

	/// Bake the inclusive texel rect [gx0,gx1]x[gy0,gy1]. The destination
	/// pointers reference the rect's (gx0, gy0) texel. rowStride is the
	/// destination row pitch in texels. Pure function of the (immutable during
	/// a bake) map lattice - safe to run on disjoint rects concurrently, and
	/// the output is band-independent: the rect-border normal fallback below
	/// is identical to the in-grid path.
	void bakeTexelRect(const WorldMapState& mapState, uint16_t* heightOut, uint16_t* offsetOut, unsigned char* normalOut,
					   size_t rowStride, int gx0, int gy0, int gx1, int gy1)
	{
		const int w = gx1 - gx0 + 1;
		const int h = gy1 - gy0 + 1;

		// pass 1: heights (kept in full precision for the normal pass) + offsets
		std::vector<float> heights(static_cast<size_t>(w) * h);
		for (int gy = gy0; gy <= gy1; gy++)
		{
			const float wy = gy * BAKE_STEP;
			for (int gx = gx0; gx <= gx1; gx++)
			{
				const float wx = gx * BAKE_STEP;
				const float height = terrainSurface::heightAt(mapState, wx, wy, terrainSurface::HeightMode::Ground);
				const Vector2f off = terrainSurface::outlineOffsetAt(mapState, wx, wy);
				heights[static_cast<size_t>(gy - gy0) * w + (gx - gx0)] = height;

				const size_t out = static_cast<size_t>(gy - gy0) * rowStride + (gx - gx0);
				heightOut[out] = encode16((height + HEIGHT_BIAS) * (HEIGHT_SCALE / 65535.f));
				offsetOut[out * 2 + 0] = encode16((off.x + OFFSET_BIAS) / TILE_UNITS);
				offsetOut[out * 2 + 1] = encode16((off.y + OFFSET_BIAS) / TILE_UNITS);
			}
		}

		// pass 2: normals. In the rect interior the fixed-radius central
		// difference lands exactly on baked height texels. Near the rect border
		// fall back to evaluating the surface directly (identical math).
		const float eps = NORMAL_EPS_TEXELS * BAKE_STEP;
		for (int gy = gy0; gy <= gy1; gy++)
		{
			for (int gx = gx0; gx <= gx1; gx++)
			{
				const size_t out = static_cast<size_t>(gy - gy0) * rowStride + (gx - gx0);
				if (gx - gx0 >= NORMAL_EPS_TEXELS && gx1 - gx >= NORMAL_EPS_TEXELS &&
					gy - gy0 >= NORMAL_EPS_TEXELS && gy1 - gy >= NORMAL_EPS_TEXELS)
				{
					const size_t row = static_cast<size_t>(gy - gy0) * w + (gx - gx0);
					const float hx = (heights[row + NORMAL_EPS_TEXELS] - heights[row - NORMAL_EPS_TEXELS]) / (2.f * eps);
					const float hy = (heights[row + static_cast<size_t>(NORMAL_EPS_TEXELS) * w] - heights[row - static_cast<size_t>(NORMAL_EPS_TEXELS) * w]) / (2.f * eps);
					const Vector3f n = glm::normalize(Vector3f(-hx, 1.f, hy));
					normalOut[out * 2 + 0] = encodeSNorm8(n.x);
					normalOut[out * 2 + 1] = encodeSNorm8(n.z);
				}
				else
				{
					const Vector3f n = terrainSurface::worldNormalAt(mapState, gx * BAKE_STEP, gy * BAKE_STEP, terrainSurface::HeightMode::Ground);
					normalOut[out * 2 + 0] = encodeSNorm8(n.x);
					normalOut[out * 2 + 1] = encodeSNorm8(n.z);
				}
			}
		}
	}

	/// Sanity check: decoded baked height must match the surface within the
	/// encoding's precision at a scattering of lattice points.
	void verifyBake(WorldMapState& mapState, const WzRawFieldImage& heightImg)
	{
		const uint16_t* texels = reinterpret_cast<const uint16_t*>(heightImg.data());
		float maxErr = 0.f;
		float maxErrExpected = 0.f, maxErrDecoded = 0.f;
		int maxErrGx = 0, maxErrGy = 0;
		for (int gy = 0; gy < bakedTexHeight; gy += 37)
		{
			for (int gx = 0; gx < bakedTexWidth; gx += 41)
			{
				const float decoded = texels[static_cast<size_t>(gy) * bakedTexWidth + gx] / HEIGHT_SCALE - HEIGHT_BIAS;
				const float expected = terrainSurface::heightAt(mapState, gx * BAKE_STEP, gy * BAKE_STEP, terrainSurface::HeightMode::Ground);
				if (std::abs(decoded - expected) > maxErr)
				{
					maxErr = std::abs(decoded - expected);
					maxErrExpected = expected;
					maxErrDecoded = decoded;
					maxErrGx = gx;
					maxErrGy = gy;
				}
			}
		}
		if (maxErr > 1.f / HEIGHT_SCALE + 0.001f)
		{
			debug(LOG_INFO, "bake mismatch at texel (%d,%d) world (%f,%f): expected %f decoded %f",
				  maxErrGx, maxErrGy, maxErrGx * BAKE_STEP, maxErrGy * BAKE_STEP, maxErrExpected, maxErrDecoded);
		}
		ASSERT(maxErr <= 1.f / HEIGHT_SCALE + 0.001f, "baked height field deviates from the surface (max err: %f)", maxErr);
		debug(LOG_TERRAIN, "bake verification: max height decode error %f (limit %f)", maxErr, 1.f / HEIGHT_SCALE);
	}
} // anonymous namespace

bool terrainBake::bakeFields(WorldMapState& mapState)
{
	const uint32_t startTime = wzGetTicks();
	bakedTexWidth = mapState.width * BAKE + 1;
	bakedTexHeight = mapState.height * BAKE + 1;

	WzRawFieldImage heightImg, offsetImg, normalImg;
	if (!heightImg.allocate(bakedTexWidth, bakedTexHeight, gfx_api::pixel_format::FORMAT_R16_UNORM)
		|| !offsetImg.allocate(bakedTexWidth, bakedTexHeight, gfx_api::pixel_format::FORMAT_RG16_UNORM)
		|| !normalImg.allocate(bakedTexWidth, bakedTexHeight, gfx_api::pixel_format::FORMAT_RG8_UNORM))
	{
		debug(LOG_ERROR, "Failed to allocate terrain bake buffers (%d x %d)", bakedTexWidth, bakedTexHeight);
		return false;
	}

	// Bake row bands in parallel: every texel is a pure function of the
	// (immutable during load) map lattice, and band boundaries produce
	// identical results to a single-threaded bake (see bakeTexelRect).
	// Keep bands a few times taller than the normal-sampling radius so the
	// band-border fallback path stays a small fraction of the work.
	const size_t numBands = std::max<size_t>(1, std::min<size_t>({ wzGetLogicalCPUCount(), 16, static_cast<size_t>(bakedTexHeight / 64) }));
	uint16_t* heightOut = reinterpret_cast<uint16_t*>(heightImg.dataWritable());
	uint16_t* offsetOut = reinterpret_cast<uint16_t*>(offsetImg.dataWritable());
	unsigned char* normalOut = normalImg.dataWritable();
	std::vector<wz::thread> workers;
	workers.reserve(numBands - 1);
	int bandY0 = 0;
	for (size_t band = 0; band < numBands; band++)
	{
		const int bandY1 = (band + 1 == numBands) ? (bakedTexHeight - 1) : static_cast<int>(static_cast<size_t>(bakedTexHeight) * (band + 1) / numBands - 1);
		uint16_t* bandHeight = &heightOut[static_cast<size_t>(bandY0) * bakedTexWidth];
		uint16_t* bandOffset = &offsetOut[static_cast<size_t>(bandY0) * bakedTexWidth * 2];
		unsigned char* bandNormal = &normalOut[static_cast<size_t>(bandY0) * bakedTexWidth * 2];
		if (band + 1 == numBands)
		{
			// the main thread bakes the last band itself
			bakeTexelRect(mapState, bandHeight, bandOffset, bandNormal, bakedTexWidth, 0, bandY0, bakedTexWidth - 1, bandY1);
		}
		else
		{
			workers.emplace_back([&mapState, bandHeight, bandOffset, bandNormal, bandY0, bandY1]() {
				bakeTexelRect(mapState, bandHeight, bandOffset, bandNormal, static_cast<size_t>(bakedTexWidth), 0, bandY0, bakedTexWidth - 1, bandY1);
			});
		}
		bandY0 = bandY1 + 1;
	}
	for (auto& worker : workers)
	{
		worker.join();
	}

	delete heightTex;
	delete offsetTex;
	delete normalTex;
	heightTex = gfx_api::context::get().create_texture(1, bakedTexWidth, bakedTexHeight, gfx_api::pixel_format::FORMAT_R16_UNORM, "mem::terrainBakedHeight");
	offsetTex = gfx_api::context::get().create_texture(1, bakedTexWidth, bakedTexHeight, gfx_api::pixel_format::FORMAT_RG16_UNORM, "mem::terrainBakedOffset");
	normalTex = gfx_api::context::get().create_texture(1, bakedTexWidth, bakedTexHeight, gfx_api::pixel_format::FORMAT_RG8_UNORM, "mem::terrainBakedNormal");
	heightTex->upload(0, heightImg);
	offsetTex->upload(0, offsetImg);
	normalTex->upload(0, normalImg);

	debug(LOG_INFO, "Baked terrain fields: %d x %d texels (%d samples/tile) in %u ms (%zu threads)",
		  bakedTexWidth, bakedTexHeight, BAKE, wzGetTicks() - startTime, numBands);
	verifyBake(mapState, heightImg);
	return true;
}

void terrainBake::rebakeTileRegion(WorldMapState& mapState, int minTileX, int minTileY, int maxTileX, int maxTileY)
{
	ASSERT_OR_RETURN(, heightTex != nullptr && offsetTex != nullptr && normalTex != nullptr, "rebake before bake?");
	// expand to the surface's influence radius: the bicubic/fillet lattice
	// fields reach [i-2, i+1] tiles around a changed corner, plus the normal
	// sampling radius. 3 tiles covers both.
	constexpr int influenceTiles = 3;
	const int gx0 = clip<int>((minTileX - influenceTiles) * BAKE, 0, bakedTexWidth - 1);
	const int gy0 = clip<int>((minTileY - influenceTiles) * BAKE, 0, bakedTexHeight - 1);
	const int gx1 = clip<int>((maxTileX + 1 + influenceTiles) * BAKE, 0, bakedTexWidth - 1);
	const int gy1 = clip<int>((maxTileY + 1 + influenceTiles) * BAKE, 0, bakedTexHeight - 1);

	WzRawFieldImage heightImg, offsetImg, normalImg;
	if (!heightImg.allocate(gx1 - gx0 + 1, gy1 - gy0 + 1, gfx_api::pixel_format::FORMAT_R16_UNORM)
		|| !offsetImg.allocate(gx1 - gx0 + 1, gy1 - gy0 + 1, gfx_api::pixel_format::FORMAT_RG16_UNORM)
		|| !normalImg.allocate(gx1 - gx0 + 1, gy1 - gy0 + 1, gfx_api::pixel_format::FORMAT_RG8_UNORM))
	{
		debug(LOG_ERROR, "Failed to allocate terrain re-bake buffers");
		return;
	}
	bakeTexelRect(mapState, reinterpret_cast<uint16_t*>(heightImg.dataWritable()), reinterpret_cast<uint16_t*>(offsetImg.dataWritable()), normalImg.dataWritable(),
				  heightImg.width(), gx0, gy0, gx1, gy1);
	heightTex->upload_sub(0, gx0, gy0, heightImg);
	offsetTex->upload_sub(0, gx0, gy0, offsetImg);
	normalTex->upload_sub(0, gx0, gy0, normalImg);
	debug(LOG_TERRAIN, "re-baked terrain fields for tiles (%d,%d)-(%d,%d): texel rect (%d,%d)-(%d,%d)",
		  minTileX, minTileY, maxTileX, maxTileY, gx0, gy0, gx1, gy1);
}

void terrainBake::shutdown()
{
	delete heightTex;
	heightTex = nullptr;
	delete offsetTex;
	offsetTex = nullptr;
	delete normalTex;
	normalTex = nullptr;
	bakedTexWidth = 0;
	bakedTexHeight = 0;
}

gfx_api::texture* terrainBake::heightTexture()
{
	return heightTex;
}

gfx_api::texture* terrainBake::offsetTexture()
{
	return offsetTex;
}

gfx_api::texture* terrainBake::normalTexture()
{
	return normalTex;
}
