// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
