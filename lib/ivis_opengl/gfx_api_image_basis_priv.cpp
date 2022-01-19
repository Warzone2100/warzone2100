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

#include "gfx_api_image_basis_priv.h"
#include "gfx_api_image_compress_priv.h"
#include "gfx_api.h"
#include "pietypes.h"

#include "lib/framework/physfs_ext.h"

#include <vector>
#include <array>
#include <algorithm>

#if defined(BASIS_ENABLED)

static optional<gfx_api::pixel_format> bestAvailableBasisCompressionFormat_GameTextureRGBA = nullopt;
static optional<gfx_api::pixel_format> bestAvailableBasisCompressionFormat_GameTextureRGB = nullopt;
static optional<gfx_api::pixel_format> bestAvailableBasisCompressionFormat_AlphaMask = nullopt;
static optional<gfx_api::pixel_format> bestAvailableBasisCompressionFormat_NormalMap = nullopt;
static optional<gfx_api::pixel_format> bestAvailableBasisCompressionFormat_SpecularMap = nullopt;

// MARK: - Basis Universal transcoder implementation

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#pragma clang diagnostic ignored "-Warray-bounds"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wpedantic"
# if __GNUC__ >= 8
#  pragma GCC diagnostic ignored "-Wclass-memaccess"
# endif
#endif
#include <basisu_transcoder.h>
#if defined(__clang__)
#pragma GCC diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

static basist::etc1_global_selector_codebook sel_codebook;

static optional<basist::transcoder_texture_format> to_basis_format(gfx_api::pixel_format desiredFormat)
{
	switch (desiredFormat)
	{
		// UNCOMPRESSED FORMATS
		case gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8:
			return basist::transcoder_texture_format::cTFRGBA32;
		case gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8:
		case gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8:
		case gfx_api::pixel_format::FORMAT_RG8_UNORM:
		case gfx_api::pixel_format::FORMAT_R8_UNORM:
			// not supported by basis transcoder - requires separate transforms
			return nullopt;
		// COMPRESSED FORMAT
		case gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM:
			return basist::transcoder_texture_format::cTFBC1_RGB;
		case gfx_api::pixel_format::FORMAT_RGBA_BC2_UNORM:
			return nullopt;
		case gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM:
			return basist::transcoder_texture_format::cTFBC3_RGBA;
		case gfx_api::pixel_format::FORMAT_R_BC4_UNORM:
			return basist::transcoder_texture_format::cTFBC4_R;
		case gfx_api::pixel_format::FORMAT_RG_BC5_UNORM:
			return basist::transcoder_texture_format::cTFBC5_RG;
		case gfx_api::pixel_format::FORMAT_RGBA_BPTC_UNORM:
			return basist::transcoder_texture_format::cTFBC7_RGBA;
		case gfx_api::pixel_format::FORMAT_RGB8_ETC1:
			return basist::transcoder_texture_format::cTFETC1_RGB;
		case gfx_api::pixel_format::FORMAT_RGB8_ETC2:
			return nullopt;
		case gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC:
			return basist::transcoder_texture_format::cTFETC2_RGBA;
		case gfx_api::pixel_format::FORMAT_R11_EAC:
			return basist::transcoder_texture_format::cTFETC2_EAC_R11;
		case gfx_api::pixel_format::FORMAT_RG11_EAC:
			return basist::transcoder_texture_format::cTFETC2_EAC_RG11;
		case gfx_api::pixel_format::FORMAT_ASTC_4x4_UNORM:
			return basist::transcoder_texture_format::cTFASTC_4x4_RGBA;
		default:
			debug(LOG_ERROR, "Unrecognised pixel format: %zu", static_cast<size_t>(desiredFormat));
	}

	return nullopt;
}

static bool basisLibrarySupportsFormat(gfx_api::pixel_format format)
{
	// Check if it's supported by *at least* one of the basis_tex_formats
	auto bFormat = to_basis_format(format);
	if (!bFormat.has_value())
	{
		return false;
	}
	return basist::basis_is_format_supported(bFormat.value(), basist::basis_tex_format::cUASTC4x4) || basist::basis_is_format_supported(bFormat.value(), basist::basis_tex_format::cETC1S);
}

void gfx_api::initBasisTranscoder()
{
	static bool basisInitialized = false;

	if (!basisInitialized)
	{
		basist::basisu_transcoder_init();
		sel_codebook.init(basist::g_global_selector_cb_size, basist::g_global_selector_cb);
		basisInitialized = true;
	}

	bestAvailableBasisCompressionFormat_GameTextureRGBA = nullopt;
	bestAvailableBasisCompressionFormat_GameTextureRGB = nullopt;
	bestAvailableBasisCompressionFormat_AlphaMask = nullopt;
	bestAvailableBasisCompressionFormat_NormalMap = nullopt;
	bestAvailableBasisCompressionFormat_SpecularMap = nullopt;

	// gfx_api::texture_type::game_texture: a RGB / RGBA texture, possibly stored in a compressed format
	// Overall quality ranking:
	//   FORMAT_ASTC_4x4_UNORM > FORMAT_RGBA_BPTC_UNORM > FORMAT_RGBA8_ETC2_EAC (/ FORMAT_RGB8_ETC2) > FORMAT_RGBA_BC3_UNORM (DXT5) / FORMAT_RGB_BC1_UNORM (for RGB - 4bpp) > ETC1 (only RGB) > PVRTC (is generally the lowest quality)
	constexpr std::array<gfx_api::pixel_format, 4> qualityOrderRGBA = {
		gfx_api::pixel_format::FORMAT_ASTC_4x4_UNORM, // NOTE: On OpenGL, only power-of-two ASTC textures are supported.
		gfx_api::pixel_format::FORMAT_RGBA_BPTC_UNORM,
		gfx_api::pixel_format::FORMAT_RGBA8_ETC2_EAC,
		gfx_api::pixel_format::FORMAT_RGBA_BC3_UNORM };
	constexpr std::array<gfx_api::pixel_format, 2> qualityOrderRGB = { gfx_api::pixel_format::FORMAT_RGB_BC1_UNORM, gfx_api::pixel_format::FORMAT_RGB8_ETC1 };

	for ( auto format : qualityOrderRGBA )
	{
		if (!basisLibrarySupportsFormat(format))
		{
			continue;
		}
		if (gfx_api::context::get().texture2DFormatIsSupported(format, gfx_api::pixel_format_usage::sampled_image))
		{
			bestAvailableBasisCompressionFormat_GameTextureRGBA = format;
			break;
		}
	}
	for ( auto format : qualityOrderRGB )
	{
		if (!basisLibrarySupportsFormat(format))
		{
			continue;
		}
		if (gfx_api::context::get().texture2DFormatIsSupported(format, gfx_api::pixel_format_usage::sampled_image))
		{
			bestAvailableBasisCompressionFormat_GameTextureRGB = format;
			break;
		}
	}

	// gfx_api::texture_type::alpha_mask:	// a single-channel texture, containing the alpha values
	// gfx_api::texture_type::specular_map: // a single-channel texture, containing the specular / luma value
		// Is is expected that for either of the above, the single-channel value is stored in the R (and possibly GB) channels
	// Overall quality rankings: FORMAT_R11_EAC (4bpp) > FORMAT_R_BC4_UNORM (4bpp)
	constexpr std::array<gfx_api::pixel_format, 3> qualityOrderR = { gfx_api::pixel_format::FORMAT_ASTC_4x4_UNORM, gfx_api::pixel_format::FORMAT_R11_EAC, gfx_api::pixel_format::FORMAT_R_BC4_UNORM };
	for ( auto format : qualityOrderR )
	{
		if (!basisLibrarySupportsFormat(format))
		{
			continue;
		}
		if (gfx_api::context::get().texture2DFormatIsSupported(format, gfx_api::pixel_format_usage::sampled_image))
		{
			bestAvailableBasisCompressionFormat_AlphaMask = format;
			bestAvailableBasisCompressionFormat_SpecularMap = format;
			break;
		}
	}
}

// Determine the best available live compressed image format for the current system (+ textureType)
static optional<gfx_api::pixel_format> getBestAvailableTranscodeFormatForBasisFile(const basist::ktx2_df_channel_id channel_id, uint32_t dfd_transfer_func, gfx_api::texture_type textureType)
{
	switch (textureType)
	{
		case gfx_api::texture_type::user_interface:
			// FUTURE TODO:
			break;
		case gfx_api::texture_type::game_texture: // a RGB / RGBA texture, generally
			if (bestAvailableBasisCompressionFormat_GameTextureRGBA.has_value())
			{
				// just use the best available RGBA format (if one is available)
				return bestAvailableBasisCompressionFormat_GameTextureRGBA;
			}
			else if (bestAvailableBasisCompressionFormat_GameTextureRGB.has_value())
			{
				// verify the input file is RGB
				switch (channel_id)
				{
					case basist::KTX2_DF_CHANNEL_UASTC_RGB:
						return bestAvailableBasisCompressionFormat_GameTextureRGB;
					default:
						break;
				}
			}
			break;
		case gfx_api::texture_type::alpha_mask:	// a single-channel texture, containing the alpha values
			return bestAvailableBasisCompressionFormat_AlphaMask;
		case gfx_api::texture_type::normal_map:
			return bestAvailableBasisCompressionFormat_NormalMap;
		case gfx_api::texture_type::specular_map: // a single-channel texture, containing the specular / luma value
			return bestAvailableBasisCompressionFormat_SpecularMap;
		default:
			// unsupported
			break;
	}

	return nullopt;
}

// Returns an iV_BaseImage for each mipLevel in a basis file, in the best possible format for the given texture_type (and input file) for the current system
// - game_texture: A compressed RGBA (or RGB) format, or an uncompressed format
// - alpha_mask: A compressed single-channel format, or R8 uncompressed
// - normal_map: TODO
// - specular_map: A compressed single-channel format, or R8 uncompressed
static std::vector<std::unique_ptr<iV_BaseImage>> loadiVImagesFromFile_Basis_Data(const void* pData, uint32_t dataSize, const char *filename, optional<gfx_api::texture_type> textureType /*= nullopt*/, uint32_t maxWidth /*= UINT32_MAX*/, uint32_t maxHeight /*= UINT32_MAX*/, optional<size_t> maxMips = nullopt)
{
	basist::ktx2_transcoder transcoder(&sel_codebook);
	if (!transcoder.init(pData, dataSize))
	{
		debug(LOG_ERROR, "Failed to initialize ktx2_transcoder for file: %s", filename);
		return {};
	}

	if (!transcoder.is_uastc())
	{
		debug(LOG_ERROR, "Unsupported: ktx2 file does not use UASTC supercompression format: %s", filename);
		return {};
	}

	auto format = basist::transcoder_texture_format::cTFRGBA32; // standard full uncompressed format
	auto internalFormat = gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8;
	if (textureType.has_value())
	{
		auto targetInternalFormat = getBestAvailableTranscodeFormatForBasisFile(transcoder.get_dfd_channel_id0(), transcoder.get_dfd_transfer_func(), textureType.value());
		if (targetInternalFormat.has_value())
		{
			auto basisFormat = to_basis_format(targetInternalFormat.value());
			if (basisFormat.has_value())
			{
				format = basisFormat.value();
				internalFormat = targetInternalFormat.value();
			}
			else
			{
				ASSERT(basisFormat.has_value(), "Failed to convert target internal format (%u) to basis format", (unsigned)targetInternalFormat.value());
			}
		}
	}

	if (!basist::basis_is_format_supported(format, transcoder.get_format()))
	{
		const char* formatName = basist::basis_get_format_name(format);
		debug(LOG_ERROR, "Basis transcoder was not compiled with support for format: %s; failed to transcode file: %s", (formatName) ? formatName : "", filename);
		return {};
	}

	std::vector<std::unique_ptr<iV_BaseImage>> outputImages;

	uint32_t mipmapLevels = transcoder.get_levels();
	uint32_t width = transcoder.get_width();
	uint32_t height = transcoder.get_height();
	debug(LOG_WZ, "%s: Width x Height: (%u x %u); Mipmap levels: %u", filename, width, height, mipmapLevels);

	// for now, these are both zero - if we need support for layers or faces in the future, we'll need some nested loops
	uint32_t layerIndex = 0;
	uint32_t faceIndex = 0;

	for (uint32_t levelIndex = 0; levelIndex < transcoder.get_levels(); levelIndex++)
	{
		basist::ktx2_image_level_info level_info;
		if (!transcoder.get_image_level_info(level_info, levelIndex, layerIndex, faceIndex))
		{
			debug(LOG_ERROR, "Failed retrieving image level info (%u %u %u) for: %s", levelIndex, layerIndex, faceIndex, filename);
			return {};
		}

		if (level_info.m_orig_width > maxWidth || level_info.m_orig_height > maxHeight)
		{
			debug(LOG_WZ, "Skipping level [%u] (%u x %u) - exceeds maxWidth or maxHeight: %d x %d", levelIndex, level_info.m_orig_width, level_info.m_orig_height, maxWidth, maxHeight);
			continue;
		}

		std::unique_ptr<iV_Image> uncompressedOutput;
		std::unique_ptr<iV_CompressedImage> compressedOutput;
		uint32_t bytes_per_block_or_pixel = basist::basis_get_bytes_per_block_or_pixel(format);
		uint32_t numBlocksOrPixels = 0;
		if (basist::basis_transcoder_format_is_uncompressed(format))
		{
			ASSERT_OR_RETURN({}, format == basist::transcoder_texture_format::cTFRGBA32, "Unsupported uncompressed format: %u", (unsigned)format);
			numBlocksOrPixels = level_info.m_orig_width * level_info.m_orig_height;

			uncompressedOutput = std::unique_ptr<iV_Image>(new iV_Image());
			if (!uncompressedOutput->allocate(level_info.m_orig_width, level_info.m_orig_height, 4, false)) // hard-coded for RGBA32 for now
			{
				debug(LOG_ERROR, "Failed to allocate memory for uncompressed image buffer");
				return {};
			}
		}
		else
		{
			numBlocksOrPixels = level_info.m_total_blocks;
			uint32_t outputSize = numBlocksOrPixels * bytes_per_block_or_pixel;

			compressedOutput = std::unique_ptr<iV_CompressedImage>(new iV_CompressedImage());
			if (!compressedOutput->allocate(internalFormat, outputSize, level_info.m_orig_width, level_info.m_orig_height, false))
			{
				debug(LOG_ERROR, "Failed to allocate memory for buffer");
				return {};
			}
		}

		uint32_t decode_flags = 0;
		if (!transcoder.transcode_image_level(levelIndex, layerIndex, faceIndex, (uncompressedOutput) ? uncompressedOutput->bmp_w() : compressedOutput->data_w(), numBlocksOrPixels, format, decode_flags))
		{
			debug(LOG_ERROR, "Failed transcoding image level (%u %u %u) for: %s", levelIndex, layerIndex, faceIndex, filename);
			return {};
		}

		if (basist::basis_transcoder_format_is_uncompressed(format))
		{
			if (textureType.has_value())
			{
				// Convert to expected # of channels based on textureType
				switch (textureType.value())
				{
					case gfx_api::texture_type::specular_map:
					case gfx_api::texture_type::alpha_mask:
						// extract single channel (should always be in R)
						uncompressedOutput->convert_to_single_channel(0);
						break;
					default:
						break;
				}
			}
			outputImages.push_back(std::move(uncompressedOutput));
		}
		else
		{
			outputImages.push_back(std::move(compressedOutput));
		}

		if (maxMips.has_value() && outputImages.size() >= maxMips.value())
		{
			break;
		}
	}

	transcoder.clear();

	return outputImages;
}

// Returns an iV_BaseImage for each mipLevel in a basis file, in the desiredFormat (if supported by the basis transcoder)
static std::vector<std::unique_ptr<iV_BaseImage>> loadiVImagesFromFile_Basis_internal(const char *filename, optional<gfx_api::texture_type> textureType /*= nullopt*/, uint32_t maxWidth /*= UINT32_MAX*/, uint32_t maxHeight /*= UINT32_MAX*/, optional<size_t> maxMips)
{
	PHYSFS_file	*fp = PHYSFS_openRead(filename);
	debug(LOG_3D, "Reading...[directory: %s] %s", PHYSFS_getRealDir(filename), filename);
	ASSERT_OR_RETURN({}, fp != nullptr, "Could not open %s", filename);
	PHYSFS_sint64 filesize = PHYSFS_fileLength(fp);
	// TODO: Make sure filesize < uint32_t::max
	std::vector<unsigned char> buffer;
	buffer.resize(filesize);
	PHYSFS_sint64 readSize = WZ_PHYSFS_readBytes(fp, buffer.data(), filesize);
	if (readSize < filesize)
	{
		debug(LOG_FATAL, "WZ_PHYSFS_readBytes(%s) failed with error: %s\n", filename, WZ_PHYSFS_getLastError());
		PHYSFS_close(fp);
		return {};
	}
	PHYSFS_close(fp);
	return loadiVImagesFromFile_Basis_Data(buffer.data(), filesize, filename, textureType, maxWidth, maxHeight, maxMips);
}

// Returns an iV_BaseImage for each mipLevel in a basis file, in the desiredFormat (if supported by the basis transcoder)
std::vector<std::unique_ptr<iV_BaseImage>> gfx_api::loadiVImagesFromFile_Basis(const char *filename, optional<gfx_api::texture_type> textureType /*= nullopt*/, uint32_t maxWidth /*= UINT32_MAX*/, uint32_t maxHeight /*= UINT32_MAX*/)
{
	return loadiVImagesFromFile_Basis_internal(filename, textureType, maxWidth, maxHeight, nullopt);
}

gfx_api::texture* gfx_api::loadImageTextureFromFile_KTX2(const std::string& filename, gfx_api::texture_type textureType, int maxWidth /*= -1*/, int maxHeight /*= -1*/)
{
	uint32_t maxWidth_u32 = (maxWidth > 0) ? static_cast<uint32_t>(maxWidth) : UINT32_MAX;
	uint32_t maxHeight_u32 = (maxHeight > 0) ? static_cast<uint32_t>(maxHeight) : UINT32_MAX;

	auto images = gfx_api::loadiVImagesFromFile_Basis(filename.c_str(), textureType, maxWidth_u32, maxHeight_u32);
	if (images.empty())
	{
		// Failed to load
		return nullptr;
	}

	// Create a new compatible gpu texture object
	size_t mipmap_levels = images.size();
	unsigned int toplevel_width = images[0]->width();
	unsigned int toplevel_height = images[0]->height();
	auto uploadFormat = images[0]->pixel_format();
	std::unique_ptr<gfx_api::texture> pTexture = std::unique_ptr<gfx_api::texture>(gfx_api::context::get().create_texture(mipmap_levels, toplevel_width, toplevel_width, uploadFormat, filename));

	// Upload image levels to texture
	for (size_t i = 0; i < mipmap_levels; i++)
	{
		unsigned int expected_width = std::max<unsigned int>(1, toplevel_width >> i);
		unsigned int expected_height = std::max<unsigned int>(1, toplevel_height >> i);

		ASSERT(images[i]->width() == expected_width && images[i]->height() == expected_height, "Actual mip-level dimensions (%u x %u) do not match expected dimensions (%u x %u) for level[%zu] in image: %s", images[i]->width(), images[i]->height(), expected_width, expected_height, i, filename.c_str());

		bool uploadResult = pTexture->upload(i, *(images[i]));
		ASSERT_OR_RETURN(nullptr, uploadResult, "Failed to upload buffer to image");
	}

	return pTexture.release();
}

#endif
