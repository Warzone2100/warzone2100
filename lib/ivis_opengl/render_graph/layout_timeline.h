// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** @file layout_timeline.h
 * Compile-time layout timeline planner (`planLayoutTimeline`) for pre/post-pass barriers.
 */

#pragma once

#include "compile.h"

#include <vector>

namespace gfx_api
{

/// <summary>
/// Plans pre/post-pass layout metadata and barrier ops for all passes.
///
/// Seeds swapchain Present via `context::getPipelineSurface(SwapchainColor)`.
/// Returns false on unrecoverable compile error (e.g. InGraph read / layoutState mismatch).
/// </summary>
bool planLayoutTimeline(std::vector<CompiledPass>& passes);

} // namespace gfx_api
