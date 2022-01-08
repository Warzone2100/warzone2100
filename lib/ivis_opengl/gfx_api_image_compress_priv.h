/*
	This file is part of Warzone 2100.
	Copyright (C) 2021-2022  Warzone 2100 Project

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

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

namespace gfx_api
{
	void initBestRealTimeCompressionFormats();

	// Determine the best available live compressed image format for the current system (+ textureType)
	optional<gfx_api::pixel_format> bestRealTimeCompressionFormatForImage(const iV_Image& image, gfx_api::texture_type textureType);

	// Compresses an iV_Image to the desired compressed image format (if possible)
	std::unique_ptr<iV_BaseImage> compressImage(const iV_Image& image, gfx_api::pixel_format desiredFormat);
}
