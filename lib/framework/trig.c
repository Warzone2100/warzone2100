/*
 * Trig.c
 *
 * Trig lookup tables
 *
 */

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include <assert.h>
#include "types.h"
#include "debug.h"
#include "mem.h"
#include "fractions.h"
#include "trig.h"

#define PI 3.141592654

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
/* Square root table - not needed on PSX cos there is a fast hardware sqrt */
static FRACT	*aSqrt;
static FRACT	*aInvSin;


/* Initialise the Trig tables */
BOOL trigInitialise(void)
{

	FRACT	val, inc;
	UDWORD	count;


	// Allocate the tables
	aSin=(FRACT*)MALLOC(sizeof(FRACT) * TRIG_DEGREES);
	if (!aSin)
	{
		return FALSE;
	}
	aCos=(FRACT*)MALLOC(sizeof(FRACT) * TRIG_DEGREES);
	if (!aCos)
	{
		return FALSE;
	}
	aInvSin=(FRACT*)MALLOC(sizeof(FRACT) * TRIG_ACCURACY);
	if (!aInvSin)
	{
		return FALSE;
	}

	aInvCos=(FRACT*)MALLOC(sizeof(FRACT) * TRIG_ACCURACY);
	if (!aInvCos)
	{
		return FALSE;
	}


//#ifndef PSX
	aSqrt=(FRACT*)MALLOC(sizeof(FRACT) * SQRT_ACCURACY);
	if (!aSqrt)
	{
		return FALSE;
	}
//#endif

	// Initialise the tables
	// inc = 2*PI/TRIG_DEGREES
	inc = FRACTmul(FRACTCONST(2,1), FRACTCONST(PI,TRIG_DEGREES));
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
		aInvSin[count] = FRACTmul( ASINFUNC(val), FRACTCONST(TRIG_DEGREES/2, PI) );
		aInvCos[count] = FRACTmul( ACOSFUNC(val), FRACTCONST(TRIG_DEGREES/2, PI) );
		val += inc;
	}

//#ifndef PSX
	// Build the sqrt table
	for(count=0; count < SQRT_ACCURACY; count++)
	{
		val = (FRACT)count / (FRACT)(SQRT_ACCURACY / 2);
		aSqrt[count]= (FRACT)sqrt(val);
  	}
//#endif



	return TRUE;
}


/* Shutdown the trig tables */
void trigShutDown(void)
{

	FREE(aSin);
	FREE(aCos);
	FREE(aInvSin);
	FREE(aInvCos);
	FREE(aSqrt);

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








