/*
	This file is part of Warzone 2100.
	Copyright (C) 2022  Warzone 2100 Project

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

#pragma once

#include "pietypes.h"
#include "gfx_api_formats_def.h"

#include <memory>
#include <vector>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

#define WZ_BASIS_UNCOMPRESSED_FORMAT gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8

namespace gfx_api
{
	struct texture; // forward-declare

	void initBasisTranscoder();

	optional<gfx_api::pixel_format> getBestAvailableTranscodeFormatForBasis(gfx_api::pixel_format_target target, gfx_api::texture_type textureType);

	std::vector<std::unique_ptr<iV_BaseImage>> loadiVImagesFromFile_Basis(const std::string& filename, gfx_api::texture_type textureType, gfx_api::pixel_format_target target,  optional<gfx_api::pixel_format> desiredFormat = nullopt, uint32_t maxWidth = UINT32_MAX, uint32_t maxHeight = UINT32_MAX);

	gfx_api::texture* loadImageTextureFromFile_KTX2(const std::string& filename, gfx_api::texture_type textureType, int maxWidth = -1, int maxHeight = -1);
	std::unique_ptr<iV_Image> loadUncompressedImageFromFile_KTX2(const std::string& filename, gfx_api::texture_type textureType, gfx_api::pixel_format_target target, int maxWidth = -1, int maxHeight = -1);
}
