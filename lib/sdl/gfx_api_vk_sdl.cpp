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

#include "gfx_api_vk_sdl.h" // Must be prior to frame.h include
#include "lib/framework/frame.h"
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_version.h>

sdl_Vulkan_Impl::sdl_Vulkan_Impl(SDL_Window* _window, bool _allowImplicitLayers)
{
	ASSERT(_window != nullptr, "Invalid SDL_Window*");
	window = _window;
	m_allowImplicitLayers = _allowImplicitLayers;
}

PFN_vkGetInstanceProcAddr sdl_Vulkan_Impl::getVkGetInstanceProcAddr()
{
#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4191 ) // warning C4191: 'reinterpret_cast': unsafe conversion from 'SDL_FunctionPointer' to 'PFN_vkGetInstanceProcAddr'
#endif
	PFN_vkGetInstanceProcAddr _vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
#if defined( _MSC_VER )
#pragma warning( pop )
#endif
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
	uint32_t requiredExtensionCount;
	const char * const *instance_extensions = SDL_Vulkan_GetInstanceExtensions(&requiredExtensionCount);
	if (!instance_extensions)
	{
		debug(LOG_ERROR, "SDL_Vulkan_GetInstanceExtensions failed: %s\n", SDL_GetError());
		return false;
	}

	std::vector<const char*> extensions;
	extensions.reserve(requiredExtensionCount);
	for (uint32_t i = 0; i < requiredExtensionCount; i++)
	{
		if (!instance_extensions[i]) { continue; }
		extensions.push_back(instance_extensions[i]);
	}

	output = extensions;
	return true;
}

bool sdl_Vulkan_Impl::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
{
	if (!SDL_Vulkan_CreateSurface(window, instance, NULL, surface))
	{
		debug(LOG_ERROR, "SDL_Vulkan_CreateSurface() failed: %s\n", SDL_GetError());
		return false;
	}
	return true;
}

void sdl_Vulkan_Impl::getDrawableSize(int* w, int* h)
{
	SDL_GetWindowSizeInPixels(window, w, h);
}

bool sdl_Vulkan_Impl::allowImplicitLayers() const
{
	return m_allowImplicitLayers;
}

#endif // defined(WZ_VULKAN_ENABLED) && defined(HAVE_SDL_VULKAN_H)
