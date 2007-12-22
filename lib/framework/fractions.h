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
/*! \file fractions.h
 *  \brief Routines to provide simple maths functions that work on both PSX & PC
 */

// Use the type "FRACT" instead of FLOAT
//  - This is defined as a float on PC and a 20.12 fixed point number on PSX
//
//  Use:-
//		MAKEINT(fract);	to convert the other way
//		FRACTmul(fract,fract); to multiply two fract numbers
//		FRACTdiv(fract,fract); to divide two numbers
//		SQRT(fract);		to get square root of a fract (returns a fract)
//      sqrtf(int);			to get a square root of an integer (returns an UDWORD) (no, it does not! - Per)
//      FRACTCONST(constA,constB);	; Generates a constant of (constA/constB)
//                         e.g. to define 0.5 use FRACTCONST(1,2)
//                              to define 0.114 use FRACTCONT(114,1000)
//
// Also PERCENT(int,int);	// returns a int value 0->100 of the percentage of the first param over the second
//

// To multiply a float by a integer just use the normal operator
//   e.g.   FractValue2=FractValue*Interger;
//
// same is true of divide

#ifndef _FRACTIONS_
#define _FRACTIONS_

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */
#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif

#include "types.h"
#include <math.h>

#define PERCENT(a,b) (((a)*100)/(b))
#define PERNUM(range,a,b) (((a)*range)/(b))

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

typedef float FRACT_D;  /* But isn't this is supposed to be double? - Per */

#define ROUND(x) ((x)>=0 ? (SDWORD)((x) + 0.5) : (SDWORD)((x) - 0.5))

#define FRACTmul(x,y) ((float)(x)*(float)(y))
#define FRACTdiv(x,y) ((float)(x)/(float)(y))

#define FRACTCONST(a,b) (((float)(a)) / ((float)(b)))
#define MAKEFRACT_D(x) ((FRACT_D)(x))
#define FRACTmul_D(x,y) ((double)(x)*(double)(y))
#define FRACTdiv_D(x,y) ((double)(x)/(double)(y))

#define MAKEINT_D(x) ((SDWORD)(x))
#define MAKEINT(x) ((SDWORD)(x))

#endif
