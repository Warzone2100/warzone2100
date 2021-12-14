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
	};
	constexpr pixel_format MAX_PIXEL_FORMAT = pixel_format::FORMAT_R8_UNORM;

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

	enum class texture_type
	{
		user_interface,
		game_texture, // a RGB / RGBA texture, possibly stored in a compressed format
		alpha_mask,	// a single-channel texture, containing the alpha values
		normal_map,
		specular_map // a single-channel texture, containing the specular / luma value
	};
	constexpr texture_type MAX_TEXTURE_TYPE = texture_type::specular_map;
	constexpr size_t TEXTURE_TYPE_COUNT = static_cast<size_t>(MAX_TEXTURE_TYPE) + 1;
}
