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
/*
 * Trig.c
 *
 * Trig lookup tables
 *
 */

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include <assert.h>
#include <stdlib.h>
#include "types.h"
#include "debug.h"
#include "fractions.h"
#include "trig.h"

/* Number of steps between -1 and 1 for the inverse tables */
#define TRIG_ACCURACY	4096
#define TRIG_ACCMASK	0x0fff

/* Number of entries in sqrt table */
#define SQRT_ACCURACY	4096
#define SQRT_ACCBITS	12

/* The trig functions */
#define SINFUNC		(FRACT)sin
#define COSFUNC		(FRACT)cos
#define ASINFUNC	(FRACT)asin
#define ACOSFUNC	(FRACT)acos



static FRACT	*aSin;
static FRACT	*aCos;
static FRACT	*aInvCos;
static FRACT	*aSqrt;
static FRACT	*aInvSin;


/* Initialise the Trig tables */
BOOL trigInitialise(void)
{

	FRACT	val, inc;
	UDWORD	count;


	// Allocate the tables
	aSin=(FRACT*)malloc(sizeof(FRACT) * TRIG_DEGREES);
	if (!aSin)
	{
		return FALSE;
	}
	aCos=(FRACT*)malloc(sizeof(FRACT) * TRIG_DEGREES);
	if (!aCos)
	{
		return FALSE;
	}
	aInvSin=(FRACT*)malloc(sizeof(FRACT) * TRIG_ACCURACY);
	if (!aInvSin)
	{
		return FALSE;
	}

	aInvCos=(FRACT*)malloc(sizeof(FRACT) * TRIG_ACCURACY);
	if (!aInvCos)
	{
		return FALSE;
	}

	aSqrt=(FRACT*)malloc(sizeof(FRACT) * SQRT_ACCURACY);
	if (!aSqrt)
	{
		return FALSE;
	}

	// Initialise the tables
	// inc = 2 * M_PI / TRIG_DEGREES
	inc = FRACTmul(FRACTCONST(2,1), FRACTCONST(M_PI, TRIG_DEGREES));
	val = FRACTCONST(0,1);
	for(count = 0; count < TRIG_DEGREES; count++)
	{
		aSin[count] = SINFUNC(val);
		aCos[count] = COSFUNC(val);
		val += inc;
	}
	inc = FRACTCONST(2,TRIG_ACCURACY-1);
	val = FRACTCONST(-1,1);
	for(count =0; count < TRIG_ACCURACY; count++)
	{
		aInvSin[count] = FRACTmul( ASINFUNC(val), FRACTCONST(TRIG_DEGREES/2, M_PI) );
		aInvCos[count] = FRACTmul( ACOSFUNC(val), FRACTCONST(TRIG_DEGREES/2, M_PI) );
		val += inc;
	}

	for(count=0; count < SQRT_ACCURACY; count++)
	{
		val = (FRACT)count / (FRACT)(SQRT_ACCURACY / 2);
		aSqrt[count]= (FRACT)sqrt(val);
	}

	return TRUE;
}


/* Shutdown the trig tables */
void trigShutDown(void)
{

	free(aSin);
	free(aCos);
	free(aInvSin);
	free(aInvCos);
	free(aSqrt);

}



/* Access the trig tables */
FRACT trigSin(SDWORD angle)
{
	if (angle < 0)
	{
		angle = (-angle) % TRIG_DEGREES;
		angle = TRIG_DEGREES - angle;
	}
	else
	{
		angle = angle % TRIG_DEGREES;
	}
	return aSin[angle % TRIG_DEGREES];
}

FRACT trigCos(SDWORD angle)
{
	if (angle < 0)
	{
		angle = (-angle) % TRIG_DEGREES;
		angle = TRIG_DEGREES - angle;
	}
	else
	{
		angle = angle % TRIG_DEGREES;
	}
	return aCos[angle % TRIG_DEGREES];
}
FRACT trigInvSin(FRACT val)
{
	SDWORD index;

	index = MAKEINT(FRACTmul(val, MAKEFRACT((TRIG_ACCURACY-1)/2)))
				+ (TRIG_ACCURACY-1)/2;

	return aInvSin[index & TRIG_ACCMASK];
}

FRACT trigInvCos(FRACT val)
{
	SDWORD index;

	index = MAKEINT(FRACTmul(val, MAKEFRACT((TRIG_ACCURACY-1)/2)))
				+ (TRIG_ACCURACY-1)/2;

	return aInvCos[index & TRIG_ACCMASK];
}



/* Fast lookup sqrt */
FRACT trigIntSqrt(UDWORD val)
{

	UDWORD	exp, mask;


	if (val == 0)
	{
		return FRACTCONST(0,1);
	}

	// find the exponent of the number
	mask = 0x80000000;		// set the msb in the mask
	for(exp=32; exp!=0; exp--)
	{
		if (val & mask)
		{
			break;
		}
		mask >>= 1;
	}

	// make all exponents even
	// odd exponents result in a mantissa of [1..2) rather than [0..1)
	if (exp & 1)
	{
		exp -= 1;
	}

	// need to shift the top bit to SQRT_BITS - left or right?
	if (exp >= SQRT_ACCBITS)
	{
		val >>= exp - SQRT_ACCBITS + 1;
	}
	else
	{
		val <<= SQRT_ACCBITS - 1 - exp;
	}

	// now generate the fractional part for the lookup table
	ASSERT( val < SQRT_ACCURACY,
		"trigIntSqrt: aargh - table index out of range" );
	return aSqrt[val] * (FRACT)((UDWORD)1 << ((UDWORD)exp/2));

}


#define DIVCNT (32)

#define ARCGAP (4096/DIVCNT)			// X2-X1

#define ARCMASK (ARCGAP-1)

/* */








