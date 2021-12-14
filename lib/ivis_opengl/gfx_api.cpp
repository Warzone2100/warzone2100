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

static gfx_api::backend_type backend = gfx_api::backend_type::opengl_backend;
bool uses_gfx_debug = false;
static gfx_api::context* current_backend_context = nullptr;

bool gfx_api::context::initialize(const gfx_api::backend_Impl_Factory& impl, int32_t antialiasing, swap_interval_mode swapMode, gfx_api::backend_type backendType)
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
	return gfx_api::context::get()._initialize(impl, antialiasing, swapMode);
}

gfx_api::context& gfx_api::context::get()
{
	return *current_backend_context;
}

// MARK: - texture

static inline nonstd::optional<size_t> getClosestSupportedUncompressedImageFormatChannels(size_t channels)
{
	auto format = iV_Image::pixel_format_for_channels(channels);

	// Verify that the gfx backend supports this format
	while (!gfx_api::context::get().texture2DFormatIsSupported(format, gfx_api::pixel_format_usage::flags::sampled_image))
	{
		ASSERT_OR_RETURN(nonstd::nullopt, channels < 4, "Exhausted all possible uncompressed formats??");
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
	auto closestSupportedChannels = getClosestSupportedUncompressedImageFormatChannels(channels);
	ASSERT_OR_RETURN(nullptr, closestSupportedChannels.has_value(), "Exhausted all possible uncompressed formats??");

	auto target_pixel_format = iV_Image::pixel_format_for_channels(closestSupportedChannels.value());

	gfx_api::texture* pTexture = gfx_api::context::get().create_texture(1, bitmap.width(), bitmap.height(), target_pixel_format, filename);
	return pTexture;
}

void gfx_api::texture::upload(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const iV_Image& bitmap)
{
	ASSERT_OR_RETURN(, bitmap.size_in_bytes() > 0, "Empty bitmap - ignoring");

	// Get the channels of this iV_Image
	auto channels = bitmap.channels();
	// Verify that the gfx backend supports this format
	auto closestSupportedChannels = getClosestSupportedUncompressedImageFormatChannels(channels);
	ASSERT_OR_RETURN(, closestSupportedChannels.has_value(), "Exhausted all possible uncompressed formats??");

	// found the nearest supported format
	if (closestSupportedChannels.value() == channels)
	{
		// Upload the texture (no conversion needed)
		upload(mip_level, offset_x, offset_y, bitmap.width(), bitmap.height(), bitmap.pixel_format(), bitmap.bmp());
	}
	else
	{
		// Make a copy of the image and convert to the desired output format
		iV_Image bitmapCopy;
		bitmapCopy.duplicate(bitmap);
		for (auto i = bitmapCopy.channels(); i < closestSupportedChannels; i++)
		{
			bitmapCopy.expand_channels_towards_rgba();
		}
		// Upload the texture
		upload(mip_level, offset_x, offset_y, bitmapCopy.width(), bitmapCopy.height(), bitmapCopy.pixel_format(), bitmapCopy.bmp());
	}

}
