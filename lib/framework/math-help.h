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

#ifndef __INCLUDED_LIB_FRAMEWORK_MATH_HELP_H__
#define __INCLUDED_LIB_FRAMEWORK_MATH_HELP_H__

#define PERCENT(a,b) (((a)*100)/(b))
#define PERNUM(range,a,b) (((a)*range)/(b))

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

#if !defined(WZ_C99)
static inline int roundf(float x)
{
	// Ensure that float truncation results in a proper rounding
	if (x < 0.0f)
		return x - 0.5f;
	else
		return x + 0.5f;
}
#endif

#endif // __INCLUDED_LIB_FRAMEWORK_MATH_HELP_H__
