/*
	This file is part of Warzone 2100.
	Copyright (C) 2017-2021  Warzone 2100 Project

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

#include <cstdint>

namespace gfx_api
{
	enum class pixel_format
	{
		invalid,

		// [UNCOMPRESSED FORMATS]
		FORMAT_RGBA8_UNORM_PACK8,
		FORMAT_BGRA8_UNORM_PACK8,
		FORMAT_RGB8_UNORM_PACK8,
		FORMAT_RG8_UNORM,			// not guaranteed support
		FORMAT_R8_UNORM,
		FORMAT_R16_UNORM,			// desktop only (not supported on GLES)
		FORMAT_RG16_UNORM,			// desktop only (not supported on GLES)
		FORMAT_D24_UNORM_S8,		// depth/stencil attachment

		// [COMPRESSED FORMATS]

		// Desktop platforms only (generally), but widely available
		FORMAT_RGB_BC1_UNORM,   // BC1 / DXT1
		FORMAT_RGBA_BC2_UNORM,  // BC2 / DXT3
		FORMAT_RGBA_BC3_UNORM,  // BC3 / DXT5
		FORMAT_R_BC4_UNORM,		// BC4 / RED_RGTC1 (Interpolated greyscale)
		FORMAT_RG_BC5_UNORM,	// BC5 / RG_RGTC2 (Interpolated two-channel)
		FORMAT_RGBA_BPTC_UNORM, // BC7 - higher quality, not quite as widely available (not available at all on macOS)

		// OpenGL ES 2.0+ (with appropriate extension) only:
		FORMAT_RGB8_ETC1,		// Available on most OpenGL ES 2.0+ - RGB only and not great quality

		// OpenGL ES 3.0 (or 2.0 with appropriate extension):
		FORMAT_RGB8_ETC2,		// Compresses RGB888 data (successor to ETC1)
		FORMAT_RGBA8_ETC2_EAC,	// Compresses RGBA8888 data with full alpha support

		FORMAT_R11_EAC,			// one channel unsigned data
		FORMAT_RG11_EAC,		// two channel unsigned data

		// ASTC (LDR) - requires an extension
		FORMAT_ASTC_4x4_UNORM,

		// Additional uncompressed / depth formats (appended to preserve prior enumerant values)
		FORMAT_A2B10G10R10_UNORM_PACK32, // 10-bit RGB + 2-bit A (e.g. VK scene color)
		FORMAT_D32_SFLOAT_S8_UINT,       // depth/stencil attachment
		FORMAT_D32_SFLOAT,               // depth-only (e.g. shadow maps)

		// Floating point color, render targets only (nothing loads a texture in these)
		FORMAT_R16G16B16A16_SFLOAT,      // half float RGBA
		FORMAT_B10G11R11_UFLOAT_PACK32,  // packed unsigned float, same size as RGBA8 but no alpha

		// sRGB counterparts (for color formats)
		FORMAT_RGBA8_SRGB_PACK8,
		FORMAT_RGB8_SRGB_PACK8,
		FORMAT_RGB_BC1_SRGB,
		FORMAT_RGBA_BC2_SRGB,
		FORMAT_RGBA_BC3_SRGB,
		FORMAT_RGBA_BPTC_SRGB,
		FORMAT_RGB8_ETC2_SRGB,
		FORMAT_RGBA8_ETC2_EAC_SRGB,
		FORMAT_ASTC_4x4_SRGB,
	};
	constexpr pixel_format MAX_PIXEL_FORMAT = pixel_format::FORMAT_ASTC_4x4_SRGB;

	/// The sRGB counterpart of a color format, or `invalid` where the format has none
	static inline pixel_format srgb_format_for(const pixel_format& format)
	{
		switch (format)
		{
			case pixel_format::FORMAT_RGBA8_UNORM_PACK8: return pixel_format::FORMAT_RGBA8_SRGB_PACK8;
			case pixel_format::FORMAT_RGB8_UNORM_PACK8:  return pixel_format::FORMAT_RGB8_SRGB_PACK8;
			case pixel_format::FORMAT_RGB_BC1_UNORM:     return pixel_format::FORMAT_RGB_BC1_SRGB;
			case pixel_format::FORMAT_RGBA_BC2_UNORM:    return pixel_format::FORMAT_RGBA_BC2_SRGB;
			case pixel_format::FORMAT_RGBA_BC3_UNORM:    return pixel_format::FORMAT_RGBA_BC3_SRGB;
			case pixel_format::FORMAT_RGBA_BPTC_UNORM:   return pixel_format::FORMAT_RGBA_BPTC_SRGB;
			case pixel_format::FORMAT_RGB8_ETC2:         return pixel_format::FORMAT_RGB8_ETC2_SRGB;
			case pixel_format::FORMAT_RGBA8_ETC2_EAC:    return pixel_format::FORMAT_RGBA8_ETC2_EAC_SRGB;
			case pixel_format::FORMAT_ASTC_4x4_UNORM:    return pixel_format::FORMAT_ASTC_4x4_SRGB;
			default: return pixel_format::invalid;
		}
	}

	/// The UNORM counterpart of an sRGB format, or the format itself where it is not sRGB.
	/// The bytes are identical either way, so encoders and transcoders work in these terms.
	static inline pixel_format unorm_format_for(const pixel_format& format)
	{
		switch (format)
		{
			case pixel_format::FORMAT_RGBA8_SRGB_PACK8:      return pixel_format::FORMAT_RGBA8_UNORM_PACK8;
			case pixel_format::FORMAT_RGB8_SRGB_PACK8:       return pixel_format::FORMAT_RGB8_UNORM_PACK8;
			case pixel_format::FORMAT_RGB_BC1_SRGB:          return pixel_format::FORMAT_RGB_BC1_UNORM;
			case pixel_format::FORMAT_RGBA_BC2_SRGB:         return pixel_format::FORMAT_RGBA_BC2_UNORM;
			case pixel_format::FORMAT_RGBA_BC3_SRGB:         return pixel_format::FORMAT_RGBA_BC3_UNORM;
			case pixel_format::FORMAT_RGBA_BPTC_SRGB:        return pixel_format::FORMAT_RGBA_BPTC_UNORM;
			case pixel_format::FORMAT_RGB8_ETC2_SRGB:        return pixel_format::FORMAT_RGB8_ETC2;
			case pixel_format::FORMAT_RGBA8_ETC2_EAC_SRGB:   return pixel_format::FORMAT_RGBA8_ETC2_EAC;
			case pixel_format::FORMAT_ASTC_4x4_SRGB:         return pixel_format::FORMAT_ASTC_4x4_UNORM;
			default: return format;
		}
	}

	/// True for the sRGB color formats (the sampler converts them to linear)
	static inline bool is_srgb_format(const pixel_format& format)
	{
		return unorm_format_for(format) != format;
	}

	/// True where an image's bytes can be uploaded to a texture created with `textureFormat`
	/// (an sRGB format and its UNORM counterpart hold identical bytes)
	static inline bool format_bytes_compatible(const pixel_format& imageFormat, const pixel_format& textureFormat)
	{
		return unorm_format_for(imageFormat) == unorm_format_for(textureFormat);
	}

	/// True for the floating point color formats
	static inline bool is_float_format(const pixel_format& format)
	{
		switch (format)
		{
			case pixel_format::FORMAT_R16G16B16A16_SFLOAT:
			case pixel_format::FORMAT_B10G11R11_UFLOAT_PACK32:
				return true;
			default:
				return false;
		}
	}

	static inline bool is_uncompressed_format(const pixel_format& format)
	{
		switch (format)
		{
			// UNCOMPRESSED FORMATS
			case gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8:
			case gfx_api::pixel_format::FORMAT_BGRA8_UNORM_PACK8:
			case gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8:
			case gfx_api::pixel_format::FORMAT_RG8_UNORM:
			case gfx_api::pixel_format::FORMAT_R8_UNORM:
			case gfx_api::pixel_format::FORMAT_A2B10G10R10_UNORM_PACK32:
			case gfx_api::pixel_format::FORMAT_R16_UNORM:
			case gfx_api::pixel_format::FORMAT_RG16_UNORM:
			case gfx_api::pixel_format::FORMAT_R16G16B16A16_SFLOAT:
			case gfx_api::pixel_format::FORMAT_B10G11R11_UFLOAT_PACK32:
			case gfx_api::pixel_format::FORMAT_RGBA8_SRGB_PACK8:
			case gfx_api::pixel_format::FORMAT_RGB8_SRGB_PACK8:
				return true;
			default:
				return false;
		}
		return false;
	}

	const char* format_to_str(gfx_api::pixel_format format);
	unsigned int format_channels(gfx_api::pixel_format format);
	size_t format_memory_size(gfx_api::pixel_format format, size_t width, size_t height);

	namespace pixel_format_usage
	{
		enum flags: uint32_t
		{
			none						= 0,		// 0 (if the format is unsupported)
			sampled_image				= 1 << 0,	// 00001 == 1
			storage_image				= 1 << 1,	// 00010 == 2
			depth_stencil_attachment	= 1 << 2,	// 00100 == 4
		};
		constexpr flags flag_max = flags::depth_stencil_attachment; // must be updated if the above enum is updated

		inline flags& operator|=(flags& a, flags b) {
			return a = static_cast<flags> (a | b);
		}
		inline flags operator|(flags a, flags b) {
			return static_cast<flags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
		}
		inline flags operator&(flags a, flags b) {
			return static_cast<flags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
		}
	}

	enum class pixel_format_target : uint8_t
	{
		texture_2d = 0,
		texture_2d_array = 1,
		depth_map = 2,
	};
	constexpr pixel_format_target MAX_PIXEL_FORMAT_TARGET = pixel_format_target::texture_2d_array;
	constexpr size_t PIXEL_FORMAT_TARGET_COUNT = static_cast<size_t>(MAX_PIXEL_FORMAT_TARGET) + 1;

	enum class texture_type
	{
		user_interface,
		game_texture, // a RGB / RGBA texture, possibly stored in a compressed format
		alpha_mask,	// a single-channel texture, containing the alpha values
		normal_map,
		specular_map, // a single-channel texture, containing the specular / luma value
		height_map, // a single-channel texture, containing the height values
	};
	constexpr texture_type MAX_TEXTURE_TYPE = texture_type::height_map;
	constexpr size_t TEXTURE_TYPE_COUNT = static_cast<size_t>(MAX_TEXTURE_TYPE) + 1;

	enum class max_texture_compression_level: uint8_t {
		same_as_source = 0,
		highest_quality = 1
	};
}
