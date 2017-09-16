/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
 * @file
 *
 * Handles fixed point operations for degrees and matrix math.
 */

#ifndef FIXEDPOINT_H
#define FIXEDPOINT_H

#include "wzglobal.h"

//! PSX-style float emulation: 12 digit semi-floats stored in an int
// FIXME!
#define FP12_SHIFT 12
#define FP12_MULTIPLIER (1 << FP12_SHIFT)


/*
 *	Global Definitions (CONSTANTS)
 */
static const int DEG_360 = 65536;
static const float DEG_1 = (float)65536 / 360.f;

static inline WZ_DECL_CONST float UNDEG(uint16_t angle)
{
	return angle * (360.f / 65536.0f) * (3.141592f / 180.f);
}

// Should be a macro (or two separate functions), since we can't do function overloading for float and int, and we don't want to use the float version for anything game-state related.
// 65536 / 360 = 8192 / 45, with a bit less overflow risk.
#define DEG(degrees) ((degrees) * 8192 / 45)

#endif // FIXEDPOINT_H
