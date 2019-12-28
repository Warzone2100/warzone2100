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

bool uses_vulkan = false;
bool uses_gfx_debug = false;

bool gfx_api::context::initialize(const gfx_api::backend_Impl_Factory& impl, int32_t antialiasing, bool useVulkan)
{
	uses_vulkan = useVulkan;
	return gfx_api::context::get()._initialize(impl, antialiasing);
}

gfx_api::context& gfx_api::context::get()
{
	if (uses_vulkan)
	{
#if defined(WZ_VULKAN_ENABLED)
		static VkRoot ctx(uses_gfx_debug);
		return ctx;
#else
		debug(LOG_FATAL, "Warzone was not compiled with the Vulkan backend enabled. Aborting.");
		abort();
#endif
	}
	else
	{
		static gl_context ctx(uses_gfx_debug);
		return ctx;
	}
}
