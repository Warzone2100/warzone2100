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
 * Interface to trig lookup tables.
 */

#ifndef _trig_h
#define _trig_h

#include "frame.h"

/* Initialise the Trig tables */
extern bool trigInitialise(void);

// Deterministic trig functions.
WZ_DECL_PURE int32_t iSin(uint16_t a);               ///< Returns sin(a*2π >> 16) << 16, rounded to nearest integer. Used as the x component in this game.
WZ_DECL_PURE int32_t iCos(uint16_t a);               ///< Returns cos(a*2π >> 16) << 16, rounded to nearest integer. Used as the y component in this game.
WZ_DECL_PURE int32_t iSinR(uint16_t a, int32_t r);   ///< Returns r*sin(a*2π >> 16), with up to 16 bits precision.
WZ_DECL_PURE int32_t iCosR(uint16_t a, int32_t r);   ///< Returns r*cos(a*2π >> 16), with up to 16 bits precision.
WZ_DECL_PURE int32_t iSinSR(int32_t a, int32_t s, int32_t r);  ///< Returns r*sin(a*2π/s), with up to 16 bits precision.
WZ_DECL_PURE int32_t iCosSR(int32_t a, int32_t s, int32_t r);  ///< Returns r*cos(a*2π/s), with up to 16 bits precision.
WZ_DECL_PURE uint16_t iAtan2(int32_t s, int32_t c);  ///< Returns atan2(s, c)/2π << 16, with a small ±1.5 platform-independent error. Used as atan2(x, y) in this game.
WZ_DECL_PURE int32_t iSqrt(uint32_t n);              ///< Returns √(n), rounded down.
WZ_DECL_PURE int32_t i64Sqrt(uint64_t n);            ///< Returns √(n), rounded down.
WZ_DECL_PURE int32_t iHypot(int32_t x, int32_t y);   ///< Returns √(x² + y²), rounded down. In case of overflow, returns correct result cast to (int32_t).
WZ_DECL_PURE int32_t iHypot3(int32_t x, int32_t y, int32_t z);  ///< Returns √(x² + y² + z²), rounded down. In case of overflow, returns correct result cast to (int32_t).

/// Returns the given angle, wrapped to the range [-180°; 180°) = [-32768; 32767].
static inline WZ_DECL_CONST int32_t angleDelta(int32_t a)
{
	return (int16_t)a;  // Cast wrapping intended.
}

#endif
