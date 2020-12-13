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
