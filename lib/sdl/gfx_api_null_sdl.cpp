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

#include "lib/framework/frame.h"
#include "gfx_api_null_sdl.h"
#include <thread>
#include <chrono>

#define NULLBACKEND_VSYNC_MAX_FPS 120

const auto minFrameDuration = std::chrono::nanoseconds(1000000000) / NULLBACKEND_VSYNC_MAX_FPS;

sdl_Null_Impl::sdl_Null_Impl()
{
	// no-op
}

void sdl_Null_Impl::swapWindow()
{
	// Handle throttling / sleeping
	switch (swapMode)
	{
		case gfx_api::context::swap_interval_mode::vsync:
			// fall-through
		case gfx_api::context::swap_interval_mode::adaptive_vsync:
		{
			// simulate pseudo-vsync - try to restrict to NULLBACKEND_VSYNC_FPS "frames" per sec
			auto swapEndTime = std::chrono::steady_clock::now();
			auto minSwapEndTime = lastSwapEndTime + minFrameDuration;
			if (swapEndTime < minSwapEndTime)
			{
				std::this_thread::sleep_until(minSwapEndTime);
				swapEndTime = minSwapEndTime;
			}
			lastSwapEndTime = swapEndTime;
			break;
		}
		case gfx_api::context::swap_interval_mode::immediate:
			break;
	}
}

bool sdl_Null_Impl::setSwapInterval(gfx_api::context::swap_interval_mode mode)
{
	swapMode = mode;
	return true;
}

gfx_api::context::swap_interval_mode sdl_Null_Impl::getSwapInterval() const
{
	return swapMode;
}

