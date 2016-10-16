/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
#include <cmath>
#include <complex>
#include <cstdlib>
#include <math.h>

// Also PERCENT(int,int);	// returns a int value 0->100 of the percentage of the first param over the second
#define PERCENT(a,b) (((a)*100)/(b))
#define PERNUM(range,a,b) (((a)*range)/(b))

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

#if (!defined(WZ_C99) && !defined(__cplusplus) && !defined(WZ_CC_GNU)) || (defined _MSC_VER)
#if (_MSC_VER <= 1600)
# include <float.h>


static inline int roundf(float x)
{
	// Ensure that float truncation results in a proper rounding
	if (x < 0.0f)
	{
		return x - 0.5f;
	}
	else
	{
		return x + 0.5f;
	}
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
static inline double nearbyint(double x)
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
// this is already included for MSVC 2010
#if _MSC_VER < 1600
static inline WZ_DECL_CONST float hypotf(float x, float y)
{
	return sqrtf(x * x + y * y);
}
#endif
#endif
#endif


/*!
 * Clips x to boundaries
 * \param x Value to clip
 * \param min Lower bound
 * \param max Upper bound
 */
static inline WZ_DECL_CONST int clip(int x, int min, int max)
{
	// std::min and std::max not constexpr until C++14.
	return x < min ? min : x > max ? max : x;
}


/*!
 * Clips x to boundaries
 * \param x Value to clip
 * \param min Lower bound
 * \param max Upper bound
 */
static inline WZ_DECL_CONST float clipf(float x, float min, float max)
{
	return x < min ? min : x > max ? max : x;
}

/// Finds change in y and y' after time dt, according to the differential equation y'' = -ay - fy'
static inline void solveDifferential2ndOrder(float *y_, float *dydt_, double acceleration, double friction, double dt)
{
	double y = *y_, dydt = *dydt_;
	// Solution is y = g_1 exp(h_1 t) + g_2 exp(h_2 t), where h_i are the solutions to h^2 + f h + a = 0, which are -f/2 +- sqrt(f^2/4 - a).
	// At t = 0, g_1 = (y' - h_2 y) / (h_1 - h_2) and g_2 = (y' - h_1 y) / (h_2 - h_1).

	std::complex<double> d = friction * friction / 4 - acceleration;

	std::complex<double> sqd    = std::sqrt(d);
	std::complex<double> h1     = -friction / 2 + sqd;
	std::complex<double> h2     = -friction / 2 - sqd;
	std::complex<double> e1     = std::exp(h1 * dt);
	std::complex<double> e2     = std::exp(h2 * dt);
	std::complex<double> g1     = (dydt - h2 * y) / (h1 - h2);
	std::complex<double> g2     = (dydt - h1 * y) / (h2 - h1);
	*y_    = (g1 * e1    + g2 * e2).real(); // .imag() should be 0.
	*dydt_ = (g1 * h1 * e1 + g2 * h2 * e2).real(); // .imag() should be 0.
}

// Windows unfortunately appears to do this, so do this too for compatibility...
using std::abs;
using std::sqrt;
using std::pow;

#endif // MATH_EXT_H
