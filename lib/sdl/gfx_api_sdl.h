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

#pragma once

#ifndef __INCLUDED_GFX_API_SDL_H__
#define __INCLUDED_GFX_API_SDL_H__

#include "lib/ivis_opengl/gfx_api.h"
#include <SDL_video.h>

class SDL_gfx_api_Impl_Factory final : public gfx_api::backend_Impl_Factory
{
public:
	SDL_gfx_api_Impl_Factory(SDL_Window* window, bool useOpenGLES, bool useOpenGLESLibrary);

	virtual std::unique_ptr<gfx_api::backend_OpenGL_Impl> createOpenGLBackendImpl() const override;
#if defined(WZ_VULKAN_ENABLED)
	virtual std::unique_ptr<gfx_api::backend_Vulkan_Impl> createVulkanBackendImpl() const override;
#endif

private:
	SDL_Window* window;
	bool useOpenGLES;
	bool useOpenGLESLibrary;
};

#endif // __INCLUDED_GFX_API_SDL_H__
