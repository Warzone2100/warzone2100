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

#include "lib/ivis_opengl/gfx_api_vk.h"
#include <SDL_video.h>

#if defined(WZ_VULKAN_ENABLED) && defined(HAVE_SDL_VULKAN_H)

class sdl_Vulkan_Impl final : public gfx_api::backend_Vulkan_Impl
{
public:
	sdl_Vulkan_Impl(SDL_Window* window);

	virtual PFN_vkGetInstanceProcAddr getVkGetInstanceProcAddr() override;
	virtual bool getRequiredInstanceExtensions(std::vector<const char*> &output) override;
	virtual bool createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) override;

	// Use this function to get the size of the window's underlying drawable dimensions in pixels. This is used for setting viewport sizes, scissor rectangles, and other places where a VkExtent might show up in relation to the window.
	virtual void getDrawableSize(int* w, int* h) override;

private:
	SDL_Window* window;
};

#endif // defined(WZ_VULKAN_ENABLED) && defined(HAVE_SDL_VULKAN_H)
