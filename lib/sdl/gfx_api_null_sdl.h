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

#ifndef __INCLUDED_GFX_API_NULL_SDL_H__
#define __INCLUDED_GFX_API_NULL_SDL_H__

#include "lib/ivis_opengl/gfx_api_null.h"
#include <SDL_video.h>
#include <chrono>

class sdl_Null_Impl final : public gfx_api::backend_Null_Impl
{
public:
	sdl_Null_Impl();

	virtual void swapWindow() override;

	virtual bool setSwapInterval(gfx_api::context::swap_interval_mode mode) override;
	virtual gfx_api::context::swap_interval_mode getSwapInterval() const override;

private:
	gfx_api::context::swap_interval_mode swapMode = gfx_api::context::swap_interval_mode::vsync;
	std::chrono::steady_clock::time_point lastSwapEndTime;
};

#endif // __INCLUDED_GFX_API_NULL_SDL_H__
