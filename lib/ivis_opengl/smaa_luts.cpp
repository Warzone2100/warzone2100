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
/** @file smaa_luts.cpp
 * The precomputed SMAA area and search lookup textures, uploaded from the
 * byte arrays shipped with the SMAA reference implementation. Uses the
 * compact R8 / RG8 formats when supported and expands to RGBA8 otherwise.
 */

#include "smaa_luts.h"

#include "gfx_api.h"
#include "pietypes.h"

#include "3rdparty/AreaTex.h"
#include "3rdparty/SearchTex.h"

#include <vector>

namespace
{

class SmaaLutImage final : public iV_BaseImage
{
public:
	SmaaLutImage(unsigned int width, unsigned int height, gfx_api::pixel_format format, std::vector<unsigned char>&& data)
	: m_width(width), m_height(height), m_format(format), m_data(std::move(data))
	{ }

	unsigned int width() const override { return m_width; }
	unsigned int height() const override { return m_height; }
	gfx_api::pixel_format pixel_format() const override { return m_format; }
	const unsigned char* data() const override { return m_data.data(); }
	size_t data_size() const override { return m_data.size(); }
	unsigned int bufferRowLength() const override { return m_width; }
	unsigned int bufferImageHeight() const override { return m_height; }

private:
	unsigned int m_width;
	unsigned int m_height;
	gfx_api::pixel_format m_format;
	std::vector<unsigned char> m_data;
};

gfx_api::texture* createLutTexture(const unsigned char* bytes, unsigned int width, unsigned int height, unsigned int channels, gfx_api::pixel_format compactFormat, const char* debugName)
{
	auto& ctx = gfx_api::context::get();
	const size_t texelCount = static_cast<size_t>(width) * static_cast<size_t>(height);

	gfx_api::pixel_format format = compactFormat;
	std::vector<unsigned char> data;
	if (ctx.textureFormatIsSupported(gfx_api::pixel_format_target::texture_2d, compactFormat, gfx_api::pixel_format_usage::sampled_image))
	{
		data.assign(bytes, bytes + texelCount * channels);
	}
	else
	{
		format = gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8;
		data.resize(texelCount * 4, 0);
		for (size_t i = 0; i < texelCount; ++i)
		{
			for (unsigned int c = 0; c < channels; ++c)
			{
				data[i * 4 + c] = bytes[i * channels + c];
			}
			data[i * 4 + 3] = 255;
		}
	}

	SmaaLutImage image(width, height, format, std::move(data));
	gfx_api::texture* texture = gfx_api::context::get().create_texture(1, width, height, format, debugName);
	if (texture == nullptr)
	{
		return nullptr;
	}
	texture->upload(0, image);
	return texture;
}

gfx_api::texture* areaTexture = nullptr;
gfx_api::texture* searchTexture = nullptr;

} // anonymous namespace

gfx_api::texture* smaaGetAreaTexture()
{
	if (areaTexture == nullptr)
	{
		areaTexture = createLutTexture(areaTexBytes, AREATEX_WIDTH, AREATEX_HEIGHT, 2, gfx_api::pixel_format::FORMAT_RG8_UNORM, "mem::smaaAreaTex");
	}
	return areaTexture;
}

gfx_api::texture* smaaGetSearchTexture()
{
	if (searchTexture == nullptr)
	{
		searchTexture = createLutTexture(searchTexBytes, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, 1, gfx_api::pixel_format::FORMAT_R8_UNORM, "mem::smaaSearchTex");
	}
	return searchTexture;
}

void smaaFreeLutTextures()
{
	delete areaTexture;
	areaTexture = nullptr;
	delete searchTexture;
	searchTexture = nullptr;
}
