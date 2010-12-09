/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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


static inline WZ_DECL_CONST float hypotf(float x, float y)
{
	return sqrtf(x * x + y * y);
}
#endif


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

/// Don't know if all compilers support _Complex.
typedef struct { double r, i; } ComplexDouble;
static inline ComplexDouble complexDouble(double r, double i)
{
	ComplexDouble ret = {r, i};
	return ret;
}
static inline ComplexDouble complexDoubleAdd(ComplexDouble a, ComplexDouble b) { return complexDouble(a.r + b.r, a.i + b.i); }
static inline ComplexDouble complexDoubleSub(ComplexDouble a, ComplexDouble b) { return complexDouble(a.r - b.r, a.i - b.i); }
static inline ComplexDouble complexDoubleMul(ComplexDouble a, ComplexDouble b) { return complexDouble(a.r*b.r - a.i*b.i, a.r*b.i + a.i*b.r); }
static inline ComplexDouble complexDoubleDiv(ComplexDouble a, ComplexDouble b) { double s = 1/(b.r*b.r + b.i*b.i); return complexDouble((a.r*b.r + a.i*b.i)*s, (-a.r*b.i + a.i*b.r)*s); }
static inline ComplexDouble complexDoubleExp(ComplexDouble a) { return complexDouble(exp(a.r)*cos(a.i), exp(a.r)*sin(a.i)); }

/// Finds change in y and y' after time dt, according to the differential equation y'' = -ay - fy'
static inline void solveDifferential2ndOrder(float *y_, float *dydt_, float acceleration, float friction, float dt)
{
	// Solution is y = g_1 exp(h_1 t) + g_2 exp(h_2 t), where h_i are the solutions to h^2 + f h + a = 0, which are -f/2 +- sqrt(f^2/4 - a).
	// At t = 0, g_1 = (y' - h_2 y) / (h_1 - h_2) and g_2 = (y' - h_1 y) / (h_2 - h_1).

	double d = friction*friction/4 - acceleration;
	ComplexDouble y, dydt, sqd, h1, h2, e1, e2, g1, g2;

	y = complexDouble(*y_, 0);
	dydt = complexDouble(*dydt_, 0);

	sqd = d > 0? complexDouble(sqrt((double)d), 0) : complexDouble(0, sqrt((double)-d));               // sqd = std::sqrt(d);
	h1 = complexDoubleAdd(complexDouble(-friction/2, 0), sqd);                                         // h1 = -friction/2 + sqd;
	h2 = complexDoubleSub(complexDouble(-friction/2, 0), sqd);                                         // h2 = -friction/2 - sqd;
	e1 = complexDoubleExp(complexDoubleMul(h1, complexDouble(dt, 0)));                                 // e1 = std::exp(h1*dt);
	e2 = complexDoubleExp(complexDoubleMul(h2, complexDouble(dt, 0)));                                 // e2 = std::exp(h2*dt);
	g1 = complexDoubleDiv(complexDoubleSub(dydt, complexDoubleMul(h2, y)), complexDoubleSub(h1, h2));  // g1 = (*dydt - h2 * *y) / (h1 - h2);
	g2 = complexDoubleDiv(complexDoubleSub(dydt, complexDoubleMul(h1, y)), complexDoubleSub(h2, h1));  // g2 = (*dydt - h1 * *y) / (h2 - h1);
	*y_ = complexDoubleAdd(complexDoubleMul(g1, e1), complexDoubleMul(g2, e2)).r;                      // *y = (g1*e1, g2*e2).real();  // .imag() should be 0.
	*dydt_ = complexDoubleAdd(complexDoubleMul(complexDoubleMul(g1, h1), e1), complexDoubleMul(complexDoubleMul(g2, h2), e2)).r;  // *dydt = (g1*h1*e1, g2*h2*e2).real();  // .imag() should be 0.
}

#endif // MATH_EXT_H
