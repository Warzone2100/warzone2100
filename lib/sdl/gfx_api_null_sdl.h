// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#ifndef __INCLUDED_GFX_API_NULL_SDL_H__
#define __INCLUDED_GFX_API_NULL_SDL_H__

#include "lib/ivis_opengl/gfx_api_null.h"
#include <SDL3/SDL_video.h>
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
