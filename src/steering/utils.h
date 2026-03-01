// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project

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
/**
 * @file utils.h
 * Utility functions for integer-based steering calculations.
 */

#pragma once

#include "lib/framework/vector.h"
#include <stdint.h>

namespace steering
{

// Fixed-point precision constant (65536 = 1.0)
// All steering calculations use this as the base unit for fractional values.
constexpr int32_t PRECISION = 65536;

// Calculate orthogonal vector (90 degrees rotation).
// Returns the vector rotated 90 degrees clockwise.
inline WZ_DECL_PURE Vector2i orthogonalCW(const Vector2i& v)
{
	return Vector2i(v.y, -v.x);
}

} // namespace steering
