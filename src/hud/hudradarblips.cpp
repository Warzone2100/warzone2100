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

#include "hudradarblips.h"
#include "hudskin.h"

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

/// The shapes, each as wide and high (logical pixels) as it's drawn: the ring's three steps, the read dot,
/// the flame's three
struct Shape
{
	const char *path;
	float size;
};
constexpr std::array<Shape, 7> SHAPES = {{
	{"images/hud/icons/radar-blip-1.svg", 15.f},
	{"images/hud/icons/radar-blip-2.svg", 15.f},
	{"images/hud/icons/radar-blip-3.svg", 15.f},
	{"images/hud/icons/radar-blip-read.svg", 5.f},
	{"images/hud/icons/radar-blip-flame.svg", 9.f},
	{"images/hud/icons/radar-blip-flame.svg", 10.f},
	{"images/hud/icons/radar-blip-flame.svg", 11.f},
}};
constexpr size_t READ = 3;
constexpr size_t FLAME = 4;
/// A dark edge around each shape
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
	std::array<Cell, SHAPES.size()> cells;
};

Atlas atlas;
std::vector<TextureRegionDraw> queued;

/// The shapes in one row, white over their dark outline (straight alpha), to be tinted by the draws
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
	std::array<Rendered, SHAPES.size()> rendered;
	int x = 0;
	int height = 0;
	for (size_t i = 0; i < SHAPES.size(); ++i)
	{
		if (!loadGameVectorImage(SHAPES[i].path))
		{
			return false;
		}
		const int side = std::max(1, static_cast<int>(std::lround(SHAPES[i].size * scale)));
		auto outline = iV_VectorImageCoverage(SHAPES[i].path, side, side, OUTLINE_WIDTH * scale);
		auto shape = iV_VectorImageCoverage(SHAPES[i].path, side, side);
		if (!outline || !shape)
		{
			return false;
		}
		Cell &cell = atlas.cells[i];
		cell.position = Vector2i(x, 0);
		cell.size = Vector2i(outline->width, outline->height);
		cell.padding = outline->padding;
		x += cell.size.x + GAP;
		height = std::max(height, cell.size.y);
		rendered[i] = {std::move(*outline), std::move(*shape)};
	}
	atlas.size = Vector2i(x, height);

	iV_Image bitmap;
	if (!bitmap.allocate(static_cast<unsigned>(atlas.size.x), static_cast<unsigned>(atlas.size.y), 4, true))
	{
		return false;
	}
	unsigned char *out = bitmap.bmp_w();
	for (size_t i = 0; i < SHAPES.size(); ++i)
	{
		const Cell &cell = atlas.cells[i];
		const Rendered &r = rendered[i];
		for (int y = 0; y < cell.size.y; ++y)
		{
			for (int px = 0; px < cell.size.x; ++px)
			{
				const float back = r.outline.coverage[static_cast<size_t>(y) * r.outline.width + px] / 255.f;
				const int sx = px - cell.padding;
				const int sy = y - cell.padding;
				const float front = (sx >= 0 && sy >= 0 && sx < r.shape.width && sy < r.shape.height)
					? r.shape.coverage[static_cast<size_t>(sy) * r.shape.width + sx] / 255.f : 0.f;
				const float alpha = front + back * (1.f - front);
				if (alpha <= 0.f)
				{
					continue;
				}
				unsigned char *pixel = out + (static_cast<size_t>(cell.position.y + y) * atlas.size.x + cell.position.x + px) * 4;
				const auto value = static_cast<unsigned char>(std::lround((255.f * front + OUTLINE * back * (1.f - front)) / alpha));
				pixel[0] = value;
				pixel[1] = value;
				pixel[2] = value;
				pixel[3] = static_cast<unsigned char>(std::lround(alpha * 255.f));
			}
		}
	}
	atlas.texture.reset(gfx_api::context::get().createTextureForCompatibleImageUploads(1, bitmap, "hudradarblips"));
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

PIELIGHT blipColor(HudRadarBlip kind)
{
	HudSkin &skin = currentHudSkin();
	switch (kind)
	{
	case HudRadarBlip::Enemy: return skin.toneColor(HudTone::Bad);
	case HudRadarBlip::Resource: return pal_RGBA(70, 200, 224, 255);
	case HudRadarBlip::Artifact: return skin.toneColor(HudTone::Ok);
	case HudRadarBlip::BurningResource: return pal_RGBA(255, 138, 58, 255);
	}
	return skin.toneColor(HudTone::Neutral);
}

}

bool hudQueueRadarBlip(HudRadarBlip kind, int frame, float x, float y)
{
	if (frame > 2 || !ready())
	{
		return false;
	}
	size_t shape = READ;
	if (frame >= 0)
	{
		shape = (kind == HudRadarBlip::BurningResource ? FLAME : 0) + static_cast<size_t>(frame);
	}
	const Cell &cell = atlas.cells[shape];
	const float scale = atlas.scale;
	const float side = SHAPES[shape].size;
	// Centered, the top left on a whole physical pixel, so the shapes stay sharp where the radar doesn't turn
	const float left = (std::round((x - side / 2.f) * scale) - static_cast<float>(cell.padding)) / scale;
	const float top = (std::round((y - side / 2.f) * scale) - static_cast<float>(cell.padding)) / scale;
	queued.push_back({Vector2f(left, top), Vector2f(cell.size.x / scale, cell.size.y / scale), cell.position, cell.size, blipColor(kind)});
	return true;
}

void hudDrawQueuedRadarBlips(const glm::mat4 &modelViewProjection)
{
	if (!queued.empty() && atlas.texture != nullptr)
	{
		iV_DrawTextureRegions(*atlas.texture, atlas.size, queued, modelViewProjection);
	}
	queued.clear();
}

void hudReleaseRadarBlips()
{
	queued.clear();
	atlas = Atlas();
}
