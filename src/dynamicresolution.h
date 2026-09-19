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
/** @file dynamicresolution.h
 * Dynamic render resolution controller driven by GPU frame timing.
 */

#pragma once

#include <nonstd/optional.hpp>
using nonstd::optional;

/// Run the dynamic resolution controller for this frame.
/// Call once per rendered 3D frame, before the frame's draw commands are recorded.
void dynamicResolutionUpdate();

/// Pin the scene render fraction to a fixed value for testing, bypassing the GPU timing feedback - or pass no value to restore normal control.
/// A pinned fraction keeps the scene targets at full size and renders a sub-rect of them, which is the state the automatic controller reaches
/// when the GPU misses its frame budget.
void dynamicResolutionSetFractionOverride(optional<float> fraction);
