/*
 * Trig.c
 *
 * Trig lookup tables
 *
 */

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include <assert.h>
#include "Types.h"
#include "Debug.h"
#include "Mem.h"
#include "Fractions.h"
#include "Trig.h"

#define PI 3.141592654

/* Number of steps between -1 and 1 for the inverse tables */
#define TRIG_ACCURACY	4096
#define TRIG_ACCMASK	0x0fff

/* Number of entries in sqrt table */
#define SQRT_ACCURACY	4096
#define SQRT_ACCBITS	12

/* The trig functions */
#ifdef WIN32
#define SINFUNC		(FRACT)sin
#define COSFUNC		(FRACT)cos
#define ASINFUNC	(FRACT)asin
#define ACOSFUNC	(FRACT)acos
#else
// for PSX
#define SINFUNC		_mathstub
#define COSFUNC		_mathstub
#define ASINFUNC	_mathstub
#define ACOSFUNC	_mathstub

FRACT ArcCos(FRACT Input);

#endif

#ifdef WIN32
static FRACT	*aSin;
static FRACT	*aCos;
static FRACT	*aInvCos;
/* Square root table - not needed on PSX cos there is a fast hardware sqrt */
static FRACT	*aSqrt;
static FRACT	*aInvSin;
#endif

/* Initialise the Trig tables */
BOOL trigInitialise(void)
{
#ifdef WIN32
	FRACT	val, inc;
	UDWORD	count;


	// Allocate the tables
	aSin=MALLOC(sizeof(FRACT) * TRIG_DEGREES);
	if (!aSin)
	{
		return FALSE;
	}
	aCos=MALLOC(sizeof(FRACT) * TRIG_DEGREES);
	if (!aCos)
	{
		return FALSE;
	}
	aInvSin=MALLOC(sizeof(FRACT) * TRIG_ACCURACY);
	if (!aInvSin)
	{
		return FALSE;
	}

	aInvCos=MALLOC(sizeof(FRACT) * TRIG_ACCURACY);
	if (!aInvCos)
	{
		return FALSE;
	}


//#ifdef WIN32
	aSqrt=MALLOC(sizeof(FRACT) * SQRT_ACCURACY);
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

//#ifdef WIN32
	// Build the sqrt table
	for(count=0; count < SQRT_ACCURACY; count++)
	{
		val = (FRACT)count / (FRACT)(SQRT_ACCURACY / 2);
		aSqrt[count]= (FRACT)sqrt(val);
  	}
//#endif


#else



#endif
	return TRUE;
}


/* Shutdown the trig tables */
void trigShutDown(void)
{
#ifdef WIN32
	FREE(aSin);
	FREE(aCos);
	FREE(aInvSin);
	FREE(aInvCos);
	FREE(aSqrt);
#endif
}

#ifdef PSX

#define angle_WORLD2PSX(ang) ((((ang)%360)*4096)/360)
#define angle_PSX2WORLD(ang) ((((ang)%4096)*360)/4096)


FRACT trigSin(SDWORD angle)
{
	return(rsin(angle_WORLD2PSX(angle)));
}

FRACT trigCos(SDWORD angle)
{
	return(rcos(angle_WORLD2PSX(angle)));
}


FRACT trigInvSin(FRACT val)
{
	assert(FALSE);
	return(0);
}


FRACT trigInvCos(FRACT val)
{

	if (val>=0) return ArcCos(val);

	return (MAKEFRACT(180) - ArcCos(-val));
	

/*
	SDWORD index;

	index = MAKEINT(FRACTmul(val, MAKEFRACT((TRIG_ACCURACY-1)/2)))
				+ (TRIG_ACCURACY-1)/2;

	return aInvCos[index & TRIG_ACCMASK];
*/
}




// Okay, Many thanks to our resedant maths wizard Jeremey.
//
//
//  invcos(val)  =   invtan( ( SQRT(1-(VAL*VAL))  ) / (VAL) )
/*
FRACT CALCtrigInvCos(FRACT val)
{

	FRACT ValSq,a,b,c;

	if(val==0) return(MAKEFRACT(90));

	ValSq=FRACTmul(val,val);
	a=MAKEFRACT(1) - ValSq;
	b=fSQRT(a);
	c=FRACTdiv(b,val);
	if (c==0)
	{
		return(MAKEFRACT(90));
		  
	}
	else
	{
		return(angle_PSX2WORLD(catan(c)));  
	}
}
*/
#else

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


#endif
/* Fast lookup sqrt */
FRACT trigIntSqrt(UDWORD val)
{
#ifdef WIN32
	UDWORD	exp, mask;
#endif

	if (val == 0)
	{
		return FRACTCONST(0,1);
	}

#ifdef WIN32
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
	ASSERT((val < SQRT_ACCURACY,
		"trigIntSqrt: aargh - table index out of range"));
	return aSqrt[val] * (FRACT)((UDWORD)1 << ((UDWORD)exp/2));
#else

// ffs - jeremy if you touch psx only code again, I will break your legs
//	return(fSQRT(val));		// is this ok ? or does iSQRT really return a UDWORD ?

	return(MAKEFRACT(iSQRT(val)));		// is this ok ? or does iSQRT really return a UDWORD ?

#endif
}


#define DIVCNT (32)

#define ARCGAP (4096/DIVCNT)			// X2-X1

#define ARCMASK (ARCGAP-1)

/* */

#ifdef PSX

UDWORD ArcCosTab[DIVCNT+1]=
{
	0x5a000,0x58358,0x566aa,0x549ee,0x52d1b,0x5102b,0x4f316,0x4d5d4,
	0x4b85c,0x49aa4,0x47ca4,0x45e4f,0x43f9c,0x4207d,0x400e3,0x3e0bf,
	0x3c000,0x39e8f,0x37c56,0x35939,0x33515,0x30fc4,0x2e914,0x2c0c7,
	0x2968d,0x269ff,0x23a8b,0x20763,0x1cf47,0x19020,0x145d3,0xe5c8,0
};



FRACT ArcCos(FRACT Input)
{
	FRACT Ratio;
	FRACT X1;
	FRACT	Theta1 ,Theta2;
	FRACT Theta,value;
	FRACT *pTab;

	assert(Input>=0);
	assert(Input<=4096);


	X1 = Input /ARCGAP;			 // first table entry
	Ratio = (((Input & ARCMASK)*4096)/ARCGAP);

	pTab=(ArcCosTab+X1);

	Theta1=*pTab++;
	Theta2=*pTab;

	if (Ratio==0) return Theta1;

	value=FRACTmul(Ratio , (Theta1-Theta2));

	Theta= Theta1 - value;

	return Theta;

}

#endif




