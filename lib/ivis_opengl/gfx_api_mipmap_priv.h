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
/** @file gfx_api_mipmap_priv.h
 * Shared mipmap helpers for gfx_api texture loading.
 */

#pragma once

#include "gfx_api.h"

#include <cmath>
#include <memory>
#include <vector>

inline size_t calcMipmapLevelsForUncompressedImage(const iV_BaseImage& image, gfx_api::texture_type textureType)
{
	bool generateMipMaps = (textureType != gfx_api::texture_type::user_interface);
	size_t mipmap_levels = 1;
	if (generateMipMaps)
	{
		mipmap_levels = 1 + static_cast<size_t>(floor(log2(std::max(image.width(), image.height()))));
	}
	return mipmap_levels;
}

inline std::vector<std::unique_ptr<iV_Image>> generateMipMapsFromUncompressedImage(const iV_Image& image, size_t mipmap_levels, gfx_api::texture_type textureType)
{
	std::vector<std::unique_ptr<iV_Image>> results;

	const iV_Image *pLastImage = &image;
	for (size_t i = 1; i < mipmap_levels; i++)
	{
		optional<int> alphaChannelOverride;
		if (textureType == gfx_api::texture_type::alpha_mask)
		{
			alphaChannelOverride = 0;
		}

		unsigned int output_w = std::max<unsigned int>(1, pLastImage->width() >> 1);
		unsigned int output_h = std::max<unsigned int>(1, pLastImage->height() >> 1);

		iV_Image *pNewLevel = new iV_Image();
		pNewLevel->resizedFromOther(*pLastImage, output_w, output_h, alphaChannelOverride);

		results.push_back(std::unique_ptr<iV_Image>(pNewLevel));

		pLastImage = pNewLevel;
	}

	return results;
}
