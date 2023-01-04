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
	optional<gfx_api::pixel_format> bestRealTimeCompressionFormatForImage(gfx_api::pixel_format_target target, const iV_Image& image, gfx_api::texture_type textureType);
	optional<gfx_api::pixel_format> bestRealTimeCompressionFormat(gfx_api::pixel_format_target target, gfx_api::texture_type textureType);

	// Compresses an iV_Image to the desired compressed image format (if possible)
	std::unique_ptr<iV_BaseImage> compressImage(const iV_Image& image, gfx_api::pixel_format desiredFormat);
}

// An image in a compressed format
// Not thread-safe
class iV_CompressedImage final : public iV_BaseImage
{
private:
	unsigned int m_width = 0, m_height = 0;
	unsigned int m_bufferRowLength = 0, m_bufferImageHeight = 0;
	gfx_api::pixel_format m_format = gfx_api::pixel_format::invalid;
	uint64_t *m_data = nullptr;
	size_t m_data_size = 0;

public:
	virtual unsigned int width() const override;
	virtual unsigned int height() const override;

	// Get the current image pixel format
	virtual gfx_api::pixel_format pixel_format() const override;

	// Get a pointer to the image data that can be read
	virtual const unsigned char* data() const override;
	virtual size_t data_size() const override;

	unsigned int bufferRowLength() const override;
	unsigned int bufferImageHeight() const override;

public:
	// Allocate a new iV_CompressedImage buffer
	bool allocate(gfx_api::pixel_format format, size_t data_size, unsigned int bufferRowLength = 0, unsigned int bufferImageHeight = 0, unsigned int width = 0, unsigned int height = 0, bool zeroMemory = false);

	// Clear (free) the image contents / data
	void clear();

	// Get a pointer to the image data that can be written to
	uint64_t* uint64_w();

public:
	// iV_Image is non-copyable
	iV_CompressedImage(const iV_CompressedImage&) = delete;
	iV_CompressedImage& operator= (const iV_CompressedImage&) = delete;

	// iV_Image is movable
	iV_CompressedImage(iV_CompressedImage&& other);
	iV_CompressedImage& operator=(iV_CompressedImage&& other);

public:
	iV_CompressedImage() = default;
	~iV_CompressedImage();
};
