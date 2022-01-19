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

#include "gfx_api_image_compress_priv.h"
#include "gfx_api.h"

#include <vector>
#include <array>
#include <algorithm>

// Return the formats for which real-time compression support has been included in this executable
static std::vector<gfx_api::pixel_format> builtInRealTimeFormatCompressors =
{
#if defined(ETCPAK_ENABLED)
		gfx_api::pixel_format::FORMAT_RGB8_ETC1,
		gfx_api::pixel_format::FORMAT_RGB8_ETC2,
		gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC,
		gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM, // DXT1
		gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM, // DXT5
#endif
};

static inline bool hasBuiltInRealTimeFormatCompressor(gfx_api::pixel_format desiredFormat)
{
	return std::any_of(builtInRealTimeFormatCompressors.begin(), builtInRealTimeFormatCompressors.end(), [desiredFormat](const gfx_api::pixel_format& builtIn) -> bool {
		return builtIn == desiredFormat;
	});
}

static optional<gfx_api::pixel_format> bestAvailableCompressionFormat_GameTextureRGBA = nullopt;
static optional<gfx_api::pixel_format> bestAvailableCompressionFormat_GameTextureRGB = nullopt;

void gfx_api::initBestRealTimeCompressionFormats()
{
	// gfx_api::texture_type::game_texture: a RGB / RGBA texture, possibly stored in a compressed format
	// Overall quality ranking:
	//   FORMAT_ASTC_4x4_UNORM > FORMAT_RGBA_BPTC_UNORM > FORMAT_RGBA8_ETC2_EAC (/ FORMAT_RGB8_ETC2) > FORMAT_RGBA_BC3_UNORM (DXT5) / FORMAT_RGB_BC1_UNORM (for RGB - 4bpp) > ETC1 (only RGB) > PVRTC (is generally the lowest quality)
	constexpr std::array<gfx_api::pixel_format, 2> qualityOrderRGBA = { gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC, gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM };
	constexpr std::array<gfx_api::pixel_format, 3> qualityOrderRGB = { gfx_api::pixel_format::FORMAT_RGB8_ETC2, gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM, gfx_api::pixel_format::FORMAT_RGB8_ETC1 };

	for ( auto format : qualityOrderRGBA )
	{
		if (!hasBuiltInRealTimeFormatCompressor(format))
		{
			continue;
		}
		if (gfx_api::context::get().texture2DFormatIsSupported(format, gfx_api::pixel_format_usage::sampled_image))
		{
			bestAvailableCompressionFormat_GameTextureRGBA = format;
			break;
		}
	}
	for ( auto format : qualityOrderRGB )
	{
		if (!hasBuiltInRealTimeFormatCompressor(format))
		{
			continue;
		}
		if (gfx_api::context::get().texture2DFormatIsSupported(format, gfx_api::pixel_format_usage::sampled_image))
		{
			bestAvailableCompressionFormat_GameTextureRGB = format;
			break;
		}
	}
}

// MARK: - iV_CompressedImage

iV_CompressedImage::~iV_CompressedImage()
{
	clear();
}

iV_CompressedImage::iV_CompressedImage(iV_CompressedImage&& other)
: m_width(0)
, m_height(0)
, m_format(gfx_api::pixel_format::invalid)
, m_data(nullptr)
, m_data_size(0)
{
	std::swap(m_width, other.m_width);
	std::swap(m_height, other.m_height);
	std::swap(m_format, other.m_format);
	std::swap(m_data, other.m_data);
	std::swap(m_data_size, other.m_data_size);
}

iV_CompressedImage& iV_CompressedImage::operator=(iV_CompressedImage&& other)
{
	if(&other != this)
	{
		clear();
		std::swap(m_width, other.m_width);
		std::swap(m_height, other.m_height);
		std::swap(m_format, other.m_format);
		std::swap(m_data, other.m_data);
		std::swap(m_data_size, other.m_data_size);
	}
	return *this;
}

// Allocate a new iV_CompressedImage buffer
bool iV_CompressedImage::allocate(gfx_api::pixel_format format, size_t data_size, unsigned int newWidth, unsigned int newHeight, bool zeroMemory)
{
	if (m_data)
	{
		free(m_data);
		m_data = nullptr;
		m_data_size = 0;
	}
	if (format != gfx_api::pixel_format::invalid && data_size > 0 && newWidth > 0 && newHeight > 0)
	{
		m_data = (unsigned char*)malloc(data_size);
		if (!m_data)
		{
			// TODO: ASSERT FAILURE TO ALLOCATE MEMORY!!
			return false;
		}
		if (zeroMemory)
		{
			memset(m_data, 0, data_size);
		}
	}
	m_width = newWidth;
	m_height = newHeight;
	m_format = format;
	m_data_size = data_size;
	return true;
}

void iV_CompressedImage::clear()
{
	allocate(gfx_api::pixel_format::invalid,0,0,0);
}

unsigned int iV_CompressedImage::width() const { return m_width; }
unsigned int iV_CompressedImage::height() const { return m_height; }

unsigned int iV_CompressedImage::channels() const
{
	switch (m_format)
	{
		// UNCOMPRESSED FORMATS
		case gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8:
		case gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8:
			return 4;
		case gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8:
			return 3;
		case gfx_api::pixel_format::FORMAT_RG8_UNORM:
			return 2;
		case gfx_api::pixel_format::FORMAT_R8_UNORM:
			return 1;
		// COMPRESSED FORMAT
		case gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM:
			return 3;
		case gfx_api::pixel_format::FORMAT_RGBA_BC2_UNORM:
			return 4;
		case gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM:
			return 4;
		case gfx_api::pixel_format::FORMAT_R_BC4_UNORM:
			return 1;
		case gfx_api::pixel_format::FORMAT_RG_BC5_UNORM:
			return 2;
		case gfx_api::pixel_format::FORMAT_RGBA_BPTC_UNORM:
			return 4;
		case gfx_api::pixel_format::FORMAT_RGB8_ETC1:
		case gfx_api::pixel_format::FORMAT_RGB8_ETC2:
			return 3;
		case gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC:
			return 4;
		case gfx_api::pixel_format::FORMAT_R11_EAC:
			return 1;
		case gfx_api::pixel_format::FORMAT_RG11_EAC:
			return 2;
		default:
			debug(LOG_FATAL, "Unrecognised pixel format");
	}

	return 0;
}

gfx_api::pixel_format iV_CompressedImage::pixel_format() const
{
	return m_format;
}

// Get a pointer to the bitmap data that can be read
const unsigned char* iV_CompressedImage::data() const
{
	return m_data;
}

size_t iV_CompressedImage::data_size() const
{
	return m_data_size;
}

// Get a pointer to the bitmap data that can be written to
unsigned char* iV_CompressedImage::data_w()
{
	return m_data;
}

// MARK: - EtcPak implementation

#if defined(ETCPAK_ENABLED)

#include <ProcessDxtc.hpp>
#include <ProcessRGB.hpp>

static std::unique_ptr<iV_CompressedImage> compressImageEtcPak(const iV_Image& image, gfx_api::pixel_format desiredFormat)
{
	ASSERT_OR_RETURN(nullptr, image.width() > 0 && image.height() > 0, "Image has 0 width or height (%u x %u)", image.width(), image.height());

	// Check that image width and height are multiple of 4
	if (image.width() % 4 != 0 || image.height() % 4 != 0)
	{
		debug(LOG_ERROR, "Unable to compress image with dimensions: %u x %u", image.width(), image.height());
		return nullptr;
	}

	const iV_Image* pSourceImage = &image;
	iV_Image dupSourceImage;

	// Convert input format to RGBA for compression with EtcPak
	if (image.channels() != 4)
	{
		dupSourceImage.duplicate(image);
		if (!dupSourceImage.convert_to_rgba())
		{
			debug(LOG_ERROR, "Failed to convert input image to RGBA?");
			return nullptr;
		}
		pSourceImage = &dupSourceImage;
	}

	uint32_t linesToProcess = pSourceImage->height();
	uint32_t blocks = pSourceImage->width() * linesToProcess / 16;

	size_t outputSize = pSourceImage->width() * pSourceImage->height() / 2;
	if(desiredFormat == gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC || desiredFormat == gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM) outputSize *= 2;

	std::unique_ptr<iV_CompressedImage> compressedOutput = std::unique_ptr<iV_CompressedImage>(new iV_CompressedImage());
	if (!compressedOutput->allocate(desiredFormat, outputSize, pSourceImage->width(), pSourceImage->height(), false))
	{
		debug(LOG_ERROR, "Failed to allocate memory for buffer");
		return nullptr;
	}

	switch (desiredFormat)
	{
		case gfx_api::pixel_format::FORMAT_RGB8_ETC1:
			CompressEtc1RgbDither(reinterpret_cast<const uint32_t*>(pSourceImage->bmp()), reinterpret_cast<uint64_t*>(compressedOutput->data_w()), blocks, pSourceImage->width());
			break;
		case gfx_api::pixel_format::FORMAT_RGB8_ETC2:
			CompressEtc2Rgb(reinterpret_cast<const uint32_t*>(pSourceImage->bmp()), reinterpret_cast<uint64_t*>(compressedOutput->data_w()), blocks, pSourceImage->width());
			break;
		case gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC:
			CompressEtc2Rgba(reinterpret_cast<const uint32_t*>(pSourceImage->bmp()), reinterpret_cast<uint64_t*>(compressedOutput->data_w()), blocks, pSourceImage->width());
			break;
		case gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM:
			CompressDxt1Dither(reinterpret_cast<const uint32_t*>(pSourceImage->bmp()), reinterpret_cast<uint64_t*>(compressedOutput->data_w()), blocks, pSourceImage->width());
			break;
		case gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM:
			CompressDxt5(reinterpret_cast<const uint32_t*>(pSourceImage->bmp()), reinterpret_cast<uint64_t*>(compressedOutput->data_w()), blocks, pSourceImage->width());
			break;
		default:
			compressedOutput->clear();
			debug(LOG_ERROR, "Unsupported compressed image format: %u", (unsigned)desiredFormat);
			return nullptr;
	}

	return compressedOutput;
}

#endif

// MARK: - Live compression functions

// Determine the best available live compressed image format for the current system (+ textureType)
optional<gfx_api::pixel_format> gfx_api::bestRealTimeCompressionFormatForImage(const iV_Image& image, gfx_api::texture_type textureType)
{
	if (image.width() % 4 != 0 || image.height() % 4 != 0)
	{
		// Image width + height should be a multiple of 4 to do runtime compression
		return nullopt;
	}

	// Only certain formats can be computed real-time - much more limited set
	switch (textureType)
	{
		case gfx_api::texture_type::user_interface:
			// POSSIBLE FUTURE TODO: Real-time: FORMAT_RGBA8_ETC2_EAC
			break;
		case gfx_api::texture_type::game_texture: // a RGB / RGBA texture, possibly stored in a compressed format
		{
			switch (image.pixel_format())
			{
				case gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8:
					return bestAvailableCompressionFormat_GameTextureRGBA;
				case gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8:
					return bestAvailableCompressionFormat_GameTextureRGB;
				default:
					break;
			}
		}
			break;
		case gfx_api::texture_type::alpha_mask:	// a single-channel texture, containing the alpha values
			// Do not run-time compress this - just store in a single-channel uncompressed texture
			break;
		default:
			// unsupported
			break;
	}

	return nullopt;
}

// Compresses an iV_Image to the desired compressed image format (if possible)
std::unique_ptr<iV_BaseImage> gfx_api::compressImage(const iV_Image& image, gfx_api::pixel_format desiredFormat)
{
	// Runtime compress to the desired format
	switch (desiredFormat)
	{
		case gfx_api::pixel_format::FORMAT_RGB8_ETC1:
		case gfx_api::pixel_format::FORMAT_RGB8_ETC2:
		case gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC:
		case gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM:
		case gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM:
			// Use EtcPak
#if defined(ETCPAK_ENABLED)
			return compressImageEtcPak(image, desiredFormat);
#else
			// fall-through
#endif
		default:
			debug(LOG_ERROR, "Unsupported compressed image format: %u", (unsigned)desiredFormat);
			return nullptr;
	}

	return nullptr;
}
