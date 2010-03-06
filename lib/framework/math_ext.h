/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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
/*!
 * \file
 * \brief Routines to provide simple math helper functions
 */

#ifndef MATH_EXT_H
#define MATH_EXT_H

#include "wzglobal.h"
#include <math.h>
#include <stdlib.h>

// Also PERCENT(int,int);	// returns a int value 0->100 of the percentage of the first param over the second
#define PERCENT(a,b) (((a)*100)/(b))
#define PERNUM(range,a,b) (((a)*range)/(b))

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

#if !defined(WZ_C99) && !defined(__cplusplus) && !defined(WZ_CC_GNU)
# include <float.h>

static inline float fmaxf(float x, float y)
{
	/* Any comparison will return false if either of the arguments are NAN */
	return (x > y || isnan(y) ? x : y);
}


static inline float fminf(float x, float y)
{
	/* Any comparison will return false if either of the arguments are NAN */
	return (x < y || isnan(y) ? x : y);
}


static inline int roundf(float x)
{
	// Ensure that float truncation results in a proper rounding
	if (x < 0.0f)
		return x - 0.5f;
	else
		return x + 0.5f;
}


/**
 * nearbyint(3) implementation because that function is only available on C99
 * compatible C libraries.
 *
 * This function rounds its argument to an integer value in floating point
 * format, using the current rounding direction and without raising an
 * @c inexact exception.
 *
 * @return The rounded integer value. If @c x is integral or infinite, @c x
 *         itself is returned.
 */
static double nearbyint(double x)
{
	if (ceil(x + 0.5) == floor(x + 0.5))
	{
		if ((int)ceil(x) % 2 == 0)
		{
			return ceil(x);
		}
		else
		{
			return floor(x);
		}
	}
	else
	{
		return floor(x + 0.5);
	}
}


static inline WZ_DECL_CONST double hypot(double x, double y)
{
	return sqrt(x * x + y * y);
}


static inline WZ_DECL_CONST float hypotf(float x, float y)
{
	return sqrtf(x * x + y * y);
}
#endif



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


/*!
 * Clips x to boundaries
 * \param x Value to clip
 * \param min Lower bound
 * \param max Upper bound
 */
static inline WZ_DECL_CONST float clipf(float x, float min, float max)
{
	if (x < min) return min;
	if (x > max) return max;
	return x;
}

#endif // MATH_EXT_H
