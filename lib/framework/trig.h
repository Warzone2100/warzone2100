/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
#include "math-help.h"

/* The number of units around a full circle */
#define TRIG_DEGREES	360

/* conversion macros */
#define DEG_TO_RAD(x)	(x * M_PI / 180.0)
#define RAD_TO_DEG(x)	(x * 180.0 / M_PI)

/*!
 * Moves x into the range 0 - y
 * \param x Value to clip
 * \param y Upper range
 * \return Value in the range 0 - y
 */
static inline WZ_DECL_CONST WZ_DECL_WARN_UNUSED_RESULT int wrap(int x, int y)
{
	while(x < 0) x += y;
	while(x >= y) x -= y;
	return x;
}

/*!
 * Moves x into the range 0.0f - y
 * \param x Value to clip
 * \param y Upper range
 * \return Value in the range 0.0f - y
 */
static inline WZ_DECL_CONST WZ_DECL_WARN_UNUSED_RESULT float wrapf(float x, float y)
{
	while(x < 0.0f) x += y;
	while(x >= y) x -= y;
	return x;
}

/* Initialise the Trig tables */
extern BOOL trigInitialise(void);

/* Shutdown the trig tables */
extern void trigShutDown(void);

/* Lookup trig functions */
// These can be assumed const, since they only rely on memory setup on startup
extern float trigSin(int angle) WZ_DECL_CONST;
extern float trigCos(int angle) WZ_DECL_CONST;
extern float trigInvSin(float val) WZ_DECL_CONST;
extern float trigInvCos(float val) WZ_DECL_CONST;

/* Supposedly fast lookup sqrt - unfortunately it's probably slower than the FPU sqrt :-( */
extern float trigIntSqrt(unsigned int val);

#endif

