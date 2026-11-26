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
	additional terms, or (at your option) any later version, as published
	by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
	the top-level directory of the source distribution. This notice must
	be kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#include "hudranks.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/gfx_api.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/vectorimage.h"

#include "../vectorimageloader.h"

#include <array>
#include <cmath>
#include <memory>
#include <vector>

namespace
{

constexpr unsigned RANKS = 8;
constexpr int SIZES = 2;
/// The shapes' width at 100%, where they sit on the pixel grid: the star's, and the others' (right-aligned to the star)
constexpr float STAR_WIDTH[SIZES] = {8.f, 6.f};
constexpr float WIDTH[SIZES] = {7.f, 5.f};
const char *const SUFFIX[SIZES] = {"", "-small"};
/// The Classic images' colors, rank by rank
constexpr uint8_t COLORS[RANKS][3] = {
	{255, 71, 39}, {255, 71, 39}, {255, 159, 67}, {240, 170, 56}, {250, 210, 72}, {255, 241, 96}, {255, 255, 255}, {250, 210, 72},
};
constexpr uint8_t OUTLINE = 10;
constexpr float OUTLINE_WIDTH = 1.f;
constexpr int GAP = 1;

struct Cell
{
	Vector2i position;  ///< (in the texture, padding included)
	Vector2i size;
	int padding = 0;
};

struct Atlas
{
	std::unique_ptr<gfx_api::texture> texture;
	Vector2i size;
	float scale = 0.f;
	bool failed = false;
	std::array<std::array<Cell, RANKS>, SIZES> cells;
};

Atlas atlas;
std::vector<TextureRegionDraw> queued;

float width100(unsigned rank, int size)
{
	return rank == RANKS - 1 ? STAR_WIDTH[size] : WIDTH[size];
}

std::string iconPath(unsigned rank, int size)
{
	return "images/hud/icons/rank-" + std::to_string(rank + 1) + SUFFIX[size] + ".svg";
}

/// Each rank in both sizes, rasterized for the current scale: the outline (the shape grown) under the shape, in color
bool build()
{
	const float scale = iV_VectorImagesScale();
	atlas = Atlas();
	atlas.scale = scale;
	struct Rendered
	{
		VectorImageCoverage outline;
		VectorImageCoverage shape;
	};
	std::array<std::array<Rendered, RANKS>, SIZES> rendered;
	Vector2i cursor(0, 0);
	int rowHeight = 0;
	int width = 0;
	for (int size = 0; size < SIZES; ++size)
	{
		cursor = Vector2i(0, cursor.y + rowHeight + (size ? GAP : 0));
		rowHeight = 0;
		for (unsigned rank = 0; rank < RANKS; ++rank)
		{
			const std::string path = iconPath(rank, size);
			if (!loadGameVectorImage(path))
			{
				return false;
			}
			const float shapeWidth = width100(rank, size);
			const int w = static_cast<int>(std::lround(shapeWidth * scale));
			const int h = static_cast<int>(std::lround(shapeWidth / gameVectorImageAspect(path) * scale));
			auto outline = iV_VectorImageCoverage(path, w, h, OUTLINE_WIDTH * scale);
			auto shape = iV_VectorImageCoverage(path, w, h);
			if (!outline || !shape)
			{
				return false;
			}
			Cell &cell = atlas.cells[size][rank];
			cell.position = cursor;
			cell.size = Vector2i(outline->width, outline->height);
			cell.padding = outline->padding;
			cursor.x += cell.size.x + GAP;
			rowHeight = std::max(rowHeight, cell.size.y);
			width = std::max(width, cursor.x);
			rendered[size][rank] = {std::move(*outline), std::move(*shape)};
		}
	}
	atlas.size = Vector2i(width, cursor.y + rowHeight);

	iV_Image bitmap;
	if (!bitmap.allocate(static_cast<unsigned>(atlas.size.x), static_cast<unsigned>(atlas.size.y), 4, true))
	{
		return false;
	}
	unsigned char *out = bitmap.bmp_w();
	for (int size = 0; size < SIZES; ++size)
	{
		for (unsigned rank = 0; rank < RANKS; ++rank)
		{
			const Cell &cell = atlas.cells[size][rank];
			const Rendered &r = rendered[size][rank];
			const uint8_t *color = COLORS[rank];
			for (int y = 0; y < cell.size.y; ++y)
			{
				for (int x = 0; x < cell.size.x; ++x)
				{
					const float back = r.outline.coverage[static_cast<size_t>(y) * r.outline.width + x] / 255.f;
					const int sx = x - cell.padding;
					const int sy = y - cell.padding;
					const float front = (sx >= 0 && sy >= 0 && sx < r.shape.width && sy < r.shape.height)
						? r.shape.coverage[static_cast<size_t>(sy) * r.shape.width + sx] / 255.f : 0.f;
					// The shape over its outline, in straight alpha
					const float alpha = front + back * (1.f - front);
					unsigned char *px = out + (static_cast<size_t>(cell.position.y + y) * atlas.size.x + cell.position.x + x) * 4;
					if (alpha <= 0.f)
					{
						continue;
					}
					const auto mix = [&](uint8_t c) {
						return static_cast<unsigned char>(std::lround((c * front + OUTLINE * back * (1.f - front)) / alpha));
					};
					px[0] = mix(color[0]);
					px[1] = mix(color[1]);
					px[2] = mix(color[2]);
					px[3] = static_cast<unsigned char>(std::lround(alpha * 255.f));
				}
			}
		}
	}
	atlas.texture.reset(gfx_api::context::get().createTextureForCompatibleImageUploads(1, bitmap, "hudranks"));
	if (atlas.texture == nullptr || !atlas.texture->upload(0, bitmap))
	{
		atlas.texture.reset();
		return false;
	}
	return true;
}

bool ready()
{
	if (atlas.texture != nullptr && atlas.scale == iV_VectorImagesScale())
	{
		return true;
	}
	if (atlas.failed && atlas.scale == iV_VectorImagesScale())
	{
		return false;
	}
	if (!build())
	{
		atlas = Atlas();
		atlas.scale = iV_VectorImagesScale();
		atlas.failed = true;
		return false;
	}
	return true;
}

}

bool hudQueueRank(unsigned level, float x, float y, HudRankSize size)
{
	if (level < 1 || level > RANKS || !ready())
	{
		return false;
	}
	const Cell &cell = atlas.cells[static_cast<int>(size)][level - 1];
	const float scale = atlas.scale;
	x += STAR_WIDTH[static_cast<int>(size)] - width100(level - 1, static_cast<int>(size));
	// The top left on a whole physical pixel, so the shapes stay on the pixel grid
	const float left = (std::round(x * scale) - static_cast<float>(cell.padding)) / scale;
	const float top = (std::round(y * scale) - static_cast<float>(cell.padding)) / scale;
	queued.push_back({Vector2f(left, top), Vector2f(cell.size.x / scale, cell.size.y / scale), cell.position, cell.size, pal_RGBA(255, 255, 255, 255)});
	return true;
}

void hudDrawQueuedRanks()
{
	if (!queued.empty() && atlas.texture != nullptr)
	{
		iV_DrawTextureRegions(*atlas.texture, atlas.size, queued);
	}
	queued.clear();
}

bool hudDrawRank(unsigned level, float x, float y, HudRankSize size)
{
	if (!hudQueueRank(level, x, y, size))
	{
		return false;
	}
	hudDrawQueuedRanks();
	return true;
}

void hudReleaseRanks()
{
	queued.clear();
	atlas = Atlas();
}
