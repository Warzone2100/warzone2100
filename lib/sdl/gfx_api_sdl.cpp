/*
	This file is part of Warzone 2100.
	Copyright (C) 2019  Warzone 2100 Project

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

#include "gfx_api_sdl.h"
#include "gfx_api_gl_sdl.h"
#include "gfx_api_vk_sdl.h"
#include <SDL_version.h>
#include <SDL_messagebox.h>

SDL_gfx_api_Impl_Factory::SDL_gfx_api_Impl_Factory(SDL_Window* _window)
{
	ASSERT(_window != nullptr, "Invalid SDL_Window*");
	window = _window;
}

std::unique_ptr<gfx_api::backend_OpenGL_Impl> SDL_gfx_api_Impl_Factory::createOpenGLBackendImpl() const
{
	return std::unique_ptr<gfx_api::backend_OpenGL_Impl>(new sdl_OpenGL_Impl(window, false));
}

#if defined(WZ_VULKAN_ENABLED)
std::unique_ptr<gfx_api::backend_Vulkan_Impl> SDL_gfx_api_Impl_Factory::createVulkanBackendImpl() const
{
#if defined(HAVE_SDL_VULKAN_H)
	return std::unique_ptr<gfx_api::backend_Vulkan_Impl>(new sdl_Vulkan_Impl(window));
#else // !defined(HAVE_SDL_VULKAN_H)
	SDL_version compiled_version;
	SDL_VERSION(&compiled_version);
	debug(LOG_ERROR, "The version of SDL used for compilation (%u.%u.%u) did not have the \"SDL_vulkan.h\" header", (unsigned int)compiled_version.major, (unsigned int)compiled_version.minor, (unsigned int)compiled_version.patch);

	char errorMessage[512];
	ssprintf(errorMessage, "Unable to initialize SDL Vulkan implementation.\nThe version of SDL used for compilation (%u.%u.%u) did not have the \"SDL_vulkan.h\" header", (unsigned int)compiled_version.major, (unsigned int)compiled_version.minor, (unsigned int)compiled_version.patch);

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
							 "Error: SDL Vulkan init",
							 errorMessage,
							 window);

	return std::unique_ptr<gfx_api::backend_Vulkan_Impl>();
#endif
}
#endif
