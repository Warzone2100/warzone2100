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

#if defined(WZ_VULKAN_ENABLED) && defined(HAVE_SDL_VULKAN_H)

#include "lib/framework/frame.h"
#include "gfx_api_vk_sdl.h"
#include <SDL_vulkan.h>
#include <SDL_version.h>

sdl_Vulkan_Impl::sdl_Vulkan_Impl(SDL_Window* _window)
{
	ASSERT(_window != nullptr, "Invalid SDL_Window*");
	window = _window;
}

PFN_vkGetInstanceProcAddr sdl_Vulkan_Impl::getVkGetInstanceProcAddr()
{
	PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
	if(!_vkGetInstanceProcAddr)
	{
		debug(LOG_ERROR, "SDL_Vulkan_GetVkGetInstanceProcAddr() failed: %s\n", SDL_GetError());
		return nullptr;
	}
	return _vkGetInstanceProcAddr;
}

bool sdl_Vulkan_Impl::getRequiredInstanceExtensions(std::vector<const char*> &output)
{
	// Get the required extension count
	unsigned int requiredExtensionCount;
	if (!SDL_Vulkan_GetInstanceExtensions(window, &requiredExtensionCount, nullptr))
	{
		debug(LOG_ERROR, "SDL_Vulkan_GetInstanceExtensions failed: %s\n", SDL_GetError());
		return false;
	}

	std::vector<const char*> extensions;
	extensions.resize(requiredExtensionCount);

	if (!SDL_Vulkan_GetInstanceExtensions(window, &requiredExtensionCount, extensions.data()))
	{
		debug(LOG_ERROR, "SDL_Vulkan_GetInstanceExtensions[2] failed: %s\n", SDL_GetError());
		return false;
	}

	output = extensions;
	return true;
}

bool sdl_Vulkan_Impl::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
{
	if (!SDL_Vulkan_CreateSurface(window, instance, surface))
	{
		debug(LOG_ERROR, "SDL_Vulkan_CreateSurface() failed: %s\n", SDL_GetError());
		return false;
	}
	return true;
}

void sdl_Vulkan_Impl::getDrawableSize(int* w, int* h)
{
	SDL_Vulkan_GetDrawableSize(window, w, h);
}

#endif // defined(WZ_VULKAN_ENABLED) && defined(HAVE_SDL_VULKAN_H)
