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
/*! \file math-help.h
 *  \brief Routines to provide simple math helper functions
 */

// Also PERCENT(int,int);	// returns a int value 0->100 of the percentage of the first param over the second

#include "wzglobal.h"

#ifndef __INCLUDED_LIB_FRAMEWORK_MATH_HELP_H__
#define __INCLUDED_LIB_FRAMEWORK_MATH_HELP_H__

#define PERCENT(a,b) (((a)*100)/(b))
#define PERNUM(range,a,b) (((a)*range)/(b))

/* conversion macros */
#define RAD_TO_DEG(x)	(x * 180.0 / M_PI)

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

#if !defined(WZ_C99) && !defined(__cplusplus) && !defined(WZ_CC_GNU)
static inline int roundf(float x)
{
	// Ensure that float truncation results in a proper rounding
	if (x < 0.0f)
		return x - 0.5f;
	else
		return x + 0.5f;
}
#endif


/*!
 * Converts x from degrees to radian
 * \param x Degree value to convert
 * \return Radian value
 */
static inline WZ_DECL_CONST float deg2radf(float x)
{
	return x * (float)M_PI / 180.0f;
}


/*!
 * Converts x from radian to degrees
 * \param x Radian value to convert
 * \return Degree value
 */
static inline WZ_DECL_CONST float rad2degf(float x)
{
	return x / (float)M_PI * 180.0f;
}

/*!
 * Moves x into the range 0 - max
 * \param x Value to clip
 * \param max Upper range
 * \return Value in the range 0 - max
 */
static inline WZ_DECL_CONST int wrap(int x, int max)
{
	while(x < 0) x += max;
	while(x >= max) x -= max;
	return x;
}


/*!
 * Moves x into the range 0.0f - max
 * \param x Value to clip
 * \param max Upper range
 * \return Value in the range 0.0f - max
 */
static inline WZ_DECL_CONST float wrapf(float x, float max)
{
	while(x < 0.0f) x += max;
	while(x >= max) x -= max;
	return x;
}

/*!
 * Clips x to boundaries
 * \param x Value to clip
 * \param min Lower bound
 * \param max Upper bound
 */
static inline WZ_DECL_CONST int clip(int x, int min, int max)
{
	if (x < min) return min;
	if (x > max) return max;
	return x;
}

#endif // __INCLUDED_LIB_FRAMEWORK_MATH_HELP_H__
