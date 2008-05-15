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

#include <math.h>

#include "types.h"
#include "debug.h"
#include "trig.h"

/* Number of steps between -1 and 1 for the inverse tables */
#define TRIG_ACCURACY	4096
#define TRIG_ACCMASK	0x0fff


static float aSin[2*TRIG_DEGREES];
static float aCos[2*TRIG_DEGREES];
static float aInvCos[TRIG_ACCURACY];
static float aInvSin[TRIG_ACCURACY];


/* Initialise the Trig tables */
BOOL trigInitialise(void)
{
	float val = 0.0f, inc = 2.0f * M_PI / TRIG_DEGREES;
	int i;

	// Initialise the tables
	for (i = 0; i < TRIG_DEGREES; i++)
	{
		aSin[i] = sinf(val);
		aCos[i] = cosf(val);
		aSin[TRIG_DEGREES+i] = aSin[i];
		aCos[TRIG_DEGREES+i] = aCos[i];
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
	return true;
}


/* Shutdown the trig tables */
void trigShutDown(void)
{}


/* Access the trig tables */
float trigSin(int angle)
{
	return aSin[angle % TRIG_DEGREES + TRIG_DEGREES];
}


float trigCos(int angle)
{
	return aCos[angle % TRIG_DEGREES + TRIG_DEGREES];
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
	return sqrtf(val);
}
