/*
	This file is part of Warzone 2100.
	Copyright (C) 2017-2019  Warzone 2100 Project

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

#include "gfx_api_vk.h"
#include "gfx_api_gl.h"
#include "gfx_api_null.h"
#include "gfx_api_image_compress_priv.h"
#include "gfx_api_image_basis_priv.h"
#include "lib/framework/physfs_ext.h"
#include <unordered_map>
#include <algorithm>

static gfx_api::backend_type backend = gfx_api::backend_type::opengl_backend;
bool uses_gfx_debug = false;
static gfx_api::context* current_backend_context = nullptr;

bool gfx_api::context::initialize(const gfx_api::backend_Impl_Factory& impl, int32_t antialiasing, swap_interval_mode swapMode, optional<float> mipLodBias, gfx_api::backend_type backendType)
{
	if (current_backend_context != nullptr && backend == backendType)
	{
		// ignore re-init for same backendType (for now)
		debug(LOG_ERROR, "Attempt to re-initialize gfx_api::context for the same backend type - ignoring (for now)");
		return true;
	}
	backend = backendType;
	if (current_backend_context)
	{
		debug(LOG_FATAL, "Attempt to reinitialize gfx_api::context for a new backend type - currently unsupported");
		return false;
	}
	switch (backend)
	{
		case gfx_api::backend_type::null_backend:
			current_backend_context = new null_context(uses_gfx_debug);
			break;
		case gfx_api::backend_type::opengl_backend:
			current_backend_context = new gl_context(uses_gfx_debug);
			break;
		case gfx_api::backend_type::vulkan_backend:
#if defined(WZ_VULKAN_ENABLED)
			current_backend_context = new VkRoot(uses_gfx_debug);
#else
			debug(LOG_FATAL, "Warzone was not compiled with the Vulkan backend enabled. Aborting.");
			abort();
#endif
			break;
	}
	ASSERT(current_backend_context != nullptr, "Failed to initialize gfx backend context");
	bool result = gfx_api::context::get()._initialize(impl, antialiasing, swapMode, mipLodBias);
	if (!result)
	{
		return false;
	}

	// Calculate the best available run-time compression formats
	gfx_api::initBestRealTimeCompressionFormats();

#if defined(BASIS_ENABLED)
	// Init basis transcoder
	gfx_api::initBasisTranscoder();
#endif

	return result;
}

gfx_api::context& gfx_api::context::get()
{
	return *current_backend_context;
}

// MARK: - Per-texture compression overrides

#include "lib/framework/file.h"
static std::unordered_map<std::string, gfx_api::max_texture_compression_level> textureCompressionOverrides;
static optional<std::string> textureCompressionOverrideFileRealDir;

static inline void processTexCompressOverrideLine(const std::string& line)
{
	if (line.empty())
	{
		return;
	}
	if (line.front() == '#')
	{
		return; // ignore comment lines
	}
	auto delimeterPos = line.rfind('=');
	if (delimeterPos == std::string::npos)
	{
		// line is missing delimeter
		debug(LOG_WZ, "TexCompressOverride line is incorrectly formatted: %s", line.c_str());
		return;
	}
	if (delimeterPos == 0)
	{
		// line is missing filename
		debug(LOG_WZ, "TexCompressOverride line is incorrectly formatted: %s", line.c_str());
		return;
	}
	if (delimeterPos == line.size() - 1)
	{
		// line is missing parameter after delimeter
		debug(LOG_WZ, "TexCompressOverride line is incorrectly formatted: %s", line.c_str());
		return;
	}

	auto maxLevelStr = line.substr(delimeterPos + 1);
	gfx_api::max_texture_compression_level maxLevel;
	if (maxLevelStr.compare("0") == 0)
	{
		maxLevel = gfx_api::max_texture_compression_level::same_as_source;
	}
	else if (maxLevelStr.compare("1") == 0)
	{
		maxLevel = gfx_api::max_texture_compression_level::highest_quality;
	}
	else
	{
		debug(LOG_WZ, "TexCompressOverride line is incorrectly formatted (unknown value): %s", line.c_str());
		return;
	}

	textureCompressionOverrides[line.substr(0, delimeterPos)] = maxLevel;
}

bool gfx_api::loadTextureCompressionOverrides()
{
	const std::string fileName = "texpages/compression_overrides.txt";

	optional<std::string> newRealDirPath;
	if (PHYSFS_exists(fileName.c_str()))
	{
		newRealDirPath = WZ_PHYSFS_getRealDir_String(fileName.c_str());
		if (textureCompressionOverrideFileRealDir.has_value() && newRealDirPath.value() == textureCompressionOverrideFileRealDir.value())
		{
			// no need to reload
			return true;
		}
	}

	// load the latest file
	textureCompressionOverrideFileRealDir = newRealDirPath;
	textureCompressionOverrides.clear();

	// [FILE FORMAT]
	// Comments: Line begins with #
	// Each line is:
	// <filename>=<max texture compression quality level>
	// Where <filename> is the path inside the data directory (example: texpages/page-6.png), *case-sensitive*
	// Where <max texture compression quality level> corresponds to the following table:
	//   0: same as source (for loading a PNG: no gpu texture compression, for loading a KTX2: ASTC4x4 or uncompressed)
	//   1: highest quality compressed texture formats only (ASTC4x4, BPTC - for PNGs, this will still currently lead to uncompressed textures in memory)
	// To get the default behavior, which is to use the best texture compression format available on a particular system, omit listing the texture filename at all.
	// (This file is intended only for overriding the default behavior.)
	// NOTE: This is only intended for compatibility with some old manually-created pixel-art texture atlases that are tightly packed, low resolution, and may have "bleed" issues as a result of texture compression.
	// In general, modern texture files with appropriate format, resolution, and padding (if needed between elements) should not need to be listed here, or should be reformatted to avoid it.

	std::vector<char> loadedTexConfigData;
	debug(LOG_WZ, "Loading texture compression config overrides: %s", fileName.c_str());
	if (!loadFileToBufferVector(fileName.c_str(), loadedTexConfigData, false, false))
	{
		debug(LOG_WZ, "No texture compression config override file: %s", fileName.c_str());
		return false;
	}

	// Remove all \r from the string
	loadedTexConfigData.erase(std::remove(loadedTexConfigData.begin(), loadedTexConfigData.end(), '\r'), loadedTexConfigData.end());

	auto it_previous = loadedTexConfigData.begin();
	auto it = std::find(loadedTexConfigData.begin(), loadedTexConfigData.end(), '\n');
	auto it_end = loadedTexConfigData.end();
	std::string tempLine;
	for (; it != it_end; it = std::find(it, loadedTexConfigData.end(), '\n'))
	{
		tempLine.assign(it_previous, it);
		processTexCompressOverrideLine(tempLine);
		it_previous = ++it;
	}
	if (it != loadedTexConfigData.begin())
	{
		tempLine.assign(it_previous, it);
		processTexCompressOverrideLine(tempLine);
	}

	debug(LOG_WZ, "Loaded %zu texture compression config overrides", textureCompressionOverrides.size());
	return true;
}

optional<gfx_api::max_texture_compression_level> gfx_api::getMaxTextureCompressionLevelOverride(const std::string& filename)
{
	auto it = textureCompressionOverrides.find(filename);
	if (it == textureCompressionOverrides.end())
	{
		return nullopt;
	}
	return it->second;
}

// MARK: - Image load helpers

#include "png_util.h"

static gfx_api::texture* loadImageTextureFromFile_PNG(const std::string& filename, gfx_api::texture_type textureType, int maxWidth /*= -1*/, int maxHeight /*= -1*/)
{
	iV_Image loadedUncompressedImage;

	// 1.) Load the PNG into an iV_Image
	bool forceRGB = (textureType == gfx_api::texture_type::game_texture) || (textureType == gfx_api::texture_type::user_interface);
	if (!iV_loadImage_PNG2(filename.c_str(), loadedUncompressedImage, forceRGB))
	{
		// Failed to load the image
		return nullptr;
	}

	return gfx_api::context::get().loadTextureFromUncompressedImage(std::move(loadedUncompressedImage), textureType, filename, maxWidth, maxHeight);
}

#if defined(BASIS_ENABLED)
const WzString wz_png_extension = WzString(".png");
#endif

static std::string imageLoadFilenameFromInputFilename(const WzString& filename)
{
#if defined(BASIS_ENABLED)
	// For backwards-compatibility support, filenames that end in ".png" are used as "keys" and we first check for a .ktx2 file with the same filename
	if (filename.endsWith(wz_png_extension))
	{
		// First, check if the .png file exists
		// (.png files take precedence, so that existing mods work - and only have to include png files)
		if (PHYSFS_exists(filename))
		{
			return filename.toUtf8();
		}

		// Check for presence of .ktx2 file
		WzString ktx2Filename = filename.substr(0, filename.size() - wz_png_extension.length());
		ktx2Filename.append(".ktx2");
		if (PHYSFS_exists(ktx2Filename))
		{
			return ktx2Filename.toUtf8();
		}
		// Fall-back to the .png file (which presumably doesn't exist because of the first check above, but this will cause it to be named as such in the later error message)
	}
#endif
	return filename.toUtf8();
}

// MARK: - High-level texture loading

// Load a texture from a file
// (which loads straight to a texture based on the appropriate texture_type, handling mip_maps, compression, etc)
gfx_api::texture* gfx_api::context::loadTextureFromFile(const char *filename, gfx_api::texture_type textureType, int maxWidth /*= -1*/, int maxHeight /*= -1*/)
{
	std::string imageLoadFilename = imageLoadFilenameFromInputFilename(filename);

#if defined(BASIS_ENABLED)
	if (strEndsWith(imageLoadFilename, ".ktx2"))
	{
		return gfx_api::loadImageTextureFromFile_KTX2(imageLoadFilename, textureType, maxWidth, maxHeight);
	}
	else
#endif
	if (strEndsWith(imageLoadFilename, ".png"))
	{
		return loadImageTextureFromFile_PNG(imageLoadFilename, textureType, maxWidth, maxHeight);
	}
	else
	{
		debug(LOG_ERROR, "Unable to load image file: %s", filename);
		return nullptr;
	}
}

static inline size_t calcMipmapLevelsForUncompressedImage(const iV_Image& image, gfx_api::texture_type textureType)
{
	// 3.) Determine mipmap levels (if needed / desired)
	bool generateMipMaps = (textureType != gfx_api::texture_type::user_interface);
	size_t mipmap_levels = 1;
	if (generateMipMaps)
	{
		// Calculate how many mip-map levels (with a target minimum level dimension of 4)
		mipmap_levels = 1 + static_cast<size_t>(floor(log2(std::max(image.width(), image.height()))));
	}
	return mipmap_levels;
}

static inline bool uncompressedPNGImageConvertChannels(iV_Image& image, gfx_api::texture_type textureType, const std::string& filename)
{
	// 1.) Convert to expected # of channels based on textureType
	switch (textureType)
	{
		case gfx_api::texture_type::specular_map:
		{
			bool result = image.convert_to_luma();
			ASSERT_OR_RETURN(false, result, "(%s): Failed to convert specular map", filename.c_str());
			break;
		}
		case gfx_api::texture_type::alpha_mask:
		{
			if (image.channels() > 1)
			{
				ASSERT_OR_RETURN(false, image.channels() == 4, "(%s): Alpha mask does not have 1 or 4 channels, as expected", filename.c_str());
				image.convert_to_single_channel(3); // extract alpha channel
			}
			break;
		}
		case gfx_api::texture_type::height_map:
		{
			if (image.channels() > 1)
			{
				image.convert_to_single_channel(0); // extract first channel
			}
			break;
		}
		default:
			break;
	}

	return true;
}

static bool checkFormatVersusMaxCompressionLevel_FromUncompressed(optional<gfx_api::max_texture_compression_level> maxLevel, gfx_api::pixel_format desiredFormat)
{
	if (maxLevel.has_value())
	{
		switch (maxLevel.value())
		{
			case gfx_api::max_texture_compression_level::same_as_source:
				return gfx_api::is_uncompressed_format(desiredFormat);
			case gfx_api::max_texture_compression_level::highest_quality:
				return gfx_api::is_uncompressed_format(desiredFormat) || desiredFormat == gfx_api::pixel_format::FORMAT_ASTC_4x4_UNORM || desiredFormat == gfx_api::pixel_format::FORMAT_RGBA_BPTC_UNORM;
		}
	}

	return true;
}

// Takes an iv_Image and texture_type and loads a texture as appropriate / possible
gfx_api::texture* gfx_api::context::loadTextureFromUncompressedImage(iV_Image&& image, gfx_api::texture_type textureType, const std::string& filename, int maxWidth /*= -1*/, int maxHeight /*= -1*/)
{
	// 1.) Convert to expected # of channels based on textureType
	if (!uncompressedPNGImageConvertChannels(image, textureType, filename))
	{
		return nullptr;
	}

	// 2.) If maxWidth / maxHeight exceed current image dimensions, resize()
	image.scale_image_max_size(maxWidth, maxHeight);

	// 3.) Determine mipmap levels (if needed / desired, based on textureType)
	size_t mipmap_levels = calcMipmapLevelsForUncompressedImage(image, textureType);

	// 4.) Extend channels, if needed, to a supported uncompressed format
	auto channels = image.channels();
	// Verify that the gfx backend supports this format
	auto closestSupportedChannels = getClosestSupportedUncompressedImageFormatChannels(gfx_api::pixel_format_target::texture_2d, channels);
	ASSERT_OR_RETURN(nullptr, closestSupportedChannels.has_value(), "Exhausted all possible uncompressed formats??");
	for (auto i = image.channels(); i < closestSupportedChannels; ++i)
	{
		image.expand_channels_towards_rgba();
	}

	auto uploadFormat = image.pixel_format();
	auto bestAvailableCompressedFormat = gfx_api::bestRealTimeCompressionFormatForImage(gfx_api::pixel_format_target::texture_2d, image, textureType);
	if (bestAvailableCompressedFormat.has_value() && bestAvailableCompressedFormat.value() != gfx_api::pixel_format::invalid)
	{
		if (checkFormatVersusMaxCompressionLevel_FromUncompressed(gfx_api::getMaxTextureCompressionLevelOverride(filename), bestAvailableCompressedFormat.value()))
		{
			uploadFormat = bestAvailableCompressedFormat.value();
		}
		else
		{
			debug(LOG_WZ, "Texture compression override prevented compressing to %s for file: %s", format_to_str(bestAvailableCompressedFormat.value()), filename.c_str());
		}
	}

	// 5.) Create a new compatible gpu texture object
	std::unique_ptr<gfx_api::texture> pTexture = std::unique_ptr<gfx_api::texture>(gfx_api::context::get().create_texture(mipmap_levels, image.width(), image.height(), uploadFormat, filename));

	// 6.) Upload initial (full) level
	if (uploadFormat == image.pixel_format())
	{
		bool uploadResult = pTexture->upload(0, image);
		ASSERT_OR_RETURN(nullptr, uploadResult, "Failed to upload buffer to image");
	}
	else
	{
		// Run-time compression
		auto compressedImage = gfx_api::compressImage(image, uploadFormat);
		ASSERT_OR_RETURN(nullptr, compressedImage != nullptr, "Failed to compress image to format: %zu", static_cast<size_t>(uploadFormat));
		bool uploadResult = pTexture->upload(0, *compressedImage);
		ASSERT_OR_RETURN(nullptr, uploadResult, "Failed to upload buffer to image");
	}

	// 7.) Generate and upload mipmaps (if needed)
	for (size_t i = 1; i < mipmap_levels; i++)
	{
		optional<int> alphaChannelOverride;
		if (textureType == gfx_api::texture_type::alpha_mask)
		{
			alphaChannelOverride = 0;
		}

		unsigned int output_w = std::max<unsigned int>(1, image.width() >> 1);
		unsigned int output_h = std::max<unsigned int>(1, image.height() >> 1);

		image.resize(output_w, output_h, alphaChannelOverride);

		if (uploadFormat == image.pixel_format())
		{
			bool uploadResult = pTexture->upload(i, image);
			ASSERT_OR_RETURN(nullptr, uploadResult, "Failed to upload buffer to image");
		}
		else
		{
			// Run-time compression
			auto compressedImage = gfx_api::compressImage(image, uploadFormat);
			ASSERT_OR_RETURN(nullptr, compressedImage != nullptr, "Failed to compress image to format: %zu", static_cast<size_t>(uploadFormat));
			bool uploadResult = pTexture->upload(i, *compressedImage);
			ASSERT_OR_RETURN(nullptr, uploadResult, "Failed to upload buffer to image");
		}
	}

	return pTexture.release();
}

std::unique_ptr<iV_Image> gfx_api::loadUncompressedImageFromFile(const char *filename, gfx_api::texture_type textureType, int maxWidth /*= -1*/, int maxHeight /*= -1*/, bool forceRGBA8 /*= false*/)
{
	std::string imageLoadFilename = imageLoadFilenameFromInputFilename(filename);
	std::unique_ptr<iV_Image> loadedUncompressedImage;

#if defined(BASIS_ENABLED)
	if (strEndsWith(imageLoadFilename, ".ktx2"))
	{
		loadedUncompressedImage = gfx_api::loadUncompressedImageFromFile_KTX2(imageLoadFilename, textureType, maxWidth, maxHeight);
		if (!loadedUncompressedImage)
		{
			// Failed to load the image
			return nullptr;
		}
		if (forceRGBA8)
		{
			loadedUncompressedImage->convert_to_rgba();
		}
	}
	else
#endif
	if (strEndsWith(imageLoadFilename, ".png"))
	{
		// Load the PNG into an iV_Image
		loadedUncompressedImage = std::unique_ptr<iV_Image>(new iV_Image());
		if (!iV_loadImage_PNG2(imageLoadFilename.c_str(), *loadedUncompressedImage, forceRGBA8))
		{
			// Failed to load the image
			return nullptr;
		}
		// Convert to expected # of channels based on textureType (if forceRGBA8 is not set)
		if (!forceRGBA8 && !uncompressedPNGImageConvertChannels(*loadedUncompressedImage, textureType, imageLoadFilename))
		{
			return nullptr;
		}
		// If maxWidth / maxHeight exceed current image dimensions, resize()
		loadedUncompressedImage->scale_image_max_size(maxWidth, maxHeight);
	}
	else
	{
		debug(LOG_ERROR, "Unable to load image file: %s", filename);
		return nullptr;
	}

	return loadedUncompressedImage;
}

// MARK: - texture

optional<unsigned int> gfx_api::context::getClosestSupportedUncompressedImageFormatChannels(pixel_format_target target, unsigned int channels)
{
	auto format = iV_Image::pixel_format_for_channels(channels);

	// Verify that the gfx backend supports this format
	while (!gfx_api::context::get().textureFormatIsSupported(target, format, gfx_api::pixel_format_usage::flags::sampled_image))
	{
		ASSERT_OR_RETURN(nullopt, channels < 4, "Exhausted all possible uncompressed formats??");
		channels += 1;
		format = iV_Image::pixel_format_for_channels(channels);
	}

	return channels;
}

gfx_api::texture* gfx_api::context::createTextureForCompatibleImageUploads(const size_t& mipmap_count, const iV_Image& bitmap, const std::string& filename)
{
	// Get the channels of this iV_Image
	auto channels = bitmap.channels();
	// Verify that the gfx backend supports this format
	auto closestSupportedChannels = getClosestSupportedUncompressedImageFormatChannels(gfx_api::pixel_format_target::texture_2d, channels);
	ASSERT_OR_RETURN(nullptr, closestSupportedChannels.has_value(), "Exhausted all possible uncompressed formats??");

	auto target_pixel_format = iV_Image::pixel_format_for_channels(closestSupportedChannels.value());

	gfx_api::texture* pTexture = gfx_api::context::get().create_texture(1, bitmap.width(), bitmap.height(), target_pixel_format, filename);
	return pTexture;
}

const char* gfx_api::format_to_str(gfx_api::pixel_format format)
{
	switch (format)
	{
		// UNCOMPRESSED FORMATS
		case gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8: return "RGBA8_UNORM";
		case gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8: return "BGRA8_UNORM";
		case gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8: return "RGB8_UNORM";
		case gfx_api::pixel_format::FORMAT_RG8_UNORM: return "RG8_UNORM";
		case gfx_api::pixel_format::FORMAT_R8_UNORM: return "R8_UNORM";
		// COMPRESSED FORMAT
		case gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM: return "RGB_BC1_UNORM";
		case gfx_api::pixel_format::FORMAT_RGBA_BC2_UNORM: return "RGBA_BC2_UNORM";
		case gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM: return "RGBA_BC3_UNORM";
		case gfx_api::pixel_format::FORMAT_R_BC4_UNORM: return "R_BC4_UNORM";
		case gfx_api::pixel_format::FORMAT_RG_BC5_UNORM: return "RG_BC5_UNORM";
		case gfx_api::pixel_format::FORMAT_RGBA_BPTC_UNORM: return "RGBA_BPTC_UNORM";
		case gfx_api::pixel_format::FORMAT_RGB8_ETC1: return "RGB8_ETC1";
		case gfx_api::pixel_format::FORMAT_RGB8_ETC2: return "RGB8_ETC2";
		case gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC: return "RGBA8_ETC2_EAC";
		case gfx_api::pixel_format::FORMAT_R11_EAC: return "R11_EAC";
		case gfx_api::pixel_format::FORMAT_RG11_EAC: return "RG11_EAC";
		case gfx_api::pixel_format::FORMAT_ASTC_4x4_UNORM: return "ASTC_4x4_UNORM";
		default:
			debug(LOG_ERROR, "Unrecognised pixel format: %zu", static_cast<size_t>(format));
			return "";
	}
	return ""; // silence warning
}

unsigned int gfx_api::format_channels(gfx_api::pixel_format format)
{
	switch (format)
	{
		case gfx_api::pixel_format::invalid:
			return 1; // just return 1 for now...
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
		case gfx_api::pixel_format::FORMAT_ASTC_4x4_UNORM:
			return 4; // just return 4 for now...
	}

	return 1; // silence warning
}

template <size_t blockWidth, size_t blockHeight>
static inline size_t calculate_astc_size(size_t width, size_t height)
{
	return ((width + blockWidth - 1) / blockWidth) * ((height + blockHeight - 1) / blockHeight) * 16;
}

size_t gfx_api::format_texel_block_width(gfx_api::pixel_format format)
{
	switch (format)
	{
		case gfx_api::pixel_format::invalid:
			return 1; // just return 1 for now...
		// UNCOMPRESSED FORMATS
		case gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8:
		case gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8:
		case gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8:
		case gfx_api::pixel_format::FORMAT_RG8_UNORM:
		case gfx_api::pixel_format::FORMAT_R8_UNORM:
			return 1;
		// COMPRESSED FORMAT
		case gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM:
		case gfx_api::pixel_format::FORMAT_RGBA_BC2_UNORM:
		case gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM:
		case gfx_api::pixel_format::FORMAT_R_BC4_UNORM:
		case gfx_api::pixel_format::FORMAT_RG_BC5_UNORM:
			return 4;
		case gfx_api::pixel_format::FORMAT_RGBA_BPTC_UNORM:
			return 4;
		case gfx_api::pixel_format::FORMAT_RGB8_ETC1:
		case gfx_api::pixel_format::FORMAT_RGB8_ETC2:
		case gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC:
		case gfx_api::pixel_format::FORMAT_R11_EAC:
		case gfx_api::pixel_format::FORMAT_RG11_EAC:
			return 4;
		case gfx_api::pixel_format::FORMAT_ASTC_4x4_UNORM:
			return 4;
	}

	return 1; // silence warning
}

size_t gfx_api::format_memory_size(gfx_api::pixel_format format, size_t width, size_t height)
{
	switch (format)
	{
		case gfx_api::pixel_format::invalid:
			return 0;
		// [UNCOMPRESSED FORMATS]
		case gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8:
		case gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8:
			return width * height * 4;
		case gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8:
			return width * height * 3;
		case gfx_api::pixel_format::FORMAT_RG8_UNORM:
			return width * height * 2;
		case gfx_api::pixel_format::FORMAT_R8_UNORM:
			return width * height;
		// [COMPRESSED FORMATS]
		// BC / DXT formats
		// 4x4 blocks, each block having a certain number of bytes
		case gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM:
			return ((width + 3) / 4) * ((height + 3) / 4) * 8;
		case gfx_api::pixel_format::FORMAT_RGBA_BC2_UNORM:
		case gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM:
			return ((width + 3) / 4) * ((height + 3) / 4) * 16;
		case gfx_api::pixel_format::FORMAT_R_BC4_UNORM:
			return ((width + 3) / 4) * ((height + 3) / 4) * 8;
		case gfx_api::pixel_format::FORMAT_RG_BC5_UNORM:
		case gfx_api::pixel_format::FORMAT_RGBA_BPTC_UNORM:
			return ((width + 3) / 4) * ((height + 3) / 4) * 16;
		// ETC
		// Size calculations from: https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glCompressedTexImage2D.xhtml
		case gfx_api::pixel_format::FORMAT_RGB8_ETC1:
		case gfx_api::pixel_format::FORMAT_RGB8_ETC2:
			return ((width + 3) / 4) * ((height + 3) / 4) * 8;
		case gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC:
			return ((width + 3) / 4) * ((height + 3) / 4) * 16;
		case gfx_api::pixel_format::FORMAT_R11_EAC:
			return ((width + 3) / 4) * ((height + 3) / 4) * 8;
		case gfx_api::pixel_format::FORMAT_RG11_EAC:
			return ((width + 3) / 4) * ((height + 3) / 4) * 16;
		// ASTC
		case gfx_api::pixel_format::FORMAT_ASTC_4x4_UNORM:
			return calculate_astc_size<4, 4>(width, height);
		// no default case to ensure compiler error if more formats are added
	}
	return 0; // silence warning
}
