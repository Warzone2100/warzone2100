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
#include "trig.h"

/* Number of steps between -1 and 1 for the inverse tables */
#define TRIG_ACCURACY	4096
#define TRIG_ACCMASK	0x0fff

/* Number of entries in sqrt table */
#define SQRT_ACCURACY	4096
#define SQRT_ACCBITS	12


static float aSin[TRIG_DEGREES];
static float aCos[TRIG_DEGREES];
static float aInvCos[TRIG_ACCURACY];
static float aInvSin[TRIG_ACCURACY];
static float aSqrt[SQRT_ACCURACY];


/* Initialise the Trig tables */
BOOL trigInitialise(void)
{
	float val = 0.0f, inc = 2.0f * (float)M_PI / TRIG_DEGREES;
	int i;

	// Initialise the tables
	for (i = 0; i < TRIG_DEGREES; i++)
	{
		aSin[i] = sinf(val);
		aCos[i] = cosf(val);
		val += inc;
	}

	inc = 2.0f / (TRIG_ACCURACY-1);
	val = -1;
	for (i = 0; i < TRIG_ACCURACY; i++)
	{
		aInvSin[i] = asinf(val) * (float)TRIG_DEGREES / (2.0f * (float)M_PI);
		aInvCos[i] = acosf(val) * (float)TRIG_DEGREES / (2.0f * (float)M_PI);
		val += inc;
	}

	for (i = 0; i < SQRT_ACCURACY; i++)
	{
		val = (float)i / (SQRT_ACCURACY / 2);
		aSqrt[i]= sqrtf(val);
	}

	return TRUE;
}


/* Shutdown the trig tables */
void trigShutDown(void)
{}


/* Access the trig tables */
float trigSin(int angle)
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


float trigCos(int angle)
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


float trigInvSin(float val)
{
	SDWORD index = (val+1) * (TRIG_ACCURACY-1) / 2;

	return aInvSin[index & TRIG_ACCMASK];
}


float trigInvCos(float val)
{
	SDWORD index = (val+1) * (TRIG_ACCURACY-1) / 2;

	return aInvCos[index & TRIG_ACCMASK];
}


/* Fast lookup sqrt */
float trigIntSqrt(unsigned int val)
{
	UDWORD exp, mask;

	if (val == 0)
	{
		return 0.0f;
	}

	// find the exponent of the number
	mask = 0x80000000; // set the msb in the mask
	for(exp = 32; exp != 0; exp--)
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
	ASSERT( val < SQRT_ACCURACY, "trigIntSqrt: table index out of range" );

	return aSqrt[val] * (1 << (exp/2));
}
