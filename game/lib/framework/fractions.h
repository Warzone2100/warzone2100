
// Routines to provide simple maths functions that work on both PSX & PC

// Use the type "FRACT" instead of FLOAT
//  - This is defined as a float on PC and a 20.12 fixed point number on PSX
//
//  Use:-
//		MAKEFRACT(int);  to convert from a SDWORD to a FRACT
//		MAKEINT(fract);	to convert the other way
//		FRACTmul(fract,fract); to multiply two fract numbers
//		FRACTdiv(fract,fract); to divide two numbers
//		SQRT(fract);		to get square root of a fract (returns a fract)
//      iSQRT(int);			to get a square root of an integer (returns an UDWORD)
//      FRACTCONST(constA,constB);	; Generates a constant of (constA/constB)
//                         e.g. to define 0.5 use FRACTCONST(1,2)
//                              to define 0.114 use FRACTCONT(114,1000)
//
// Also PERCENT(int,int);	// returns a int value 0->100 of the percentage of the first param over the second
//


// This file used to be in the deliverance src directory. But Jeremy quite correctly
// pointed out to me that it should be library based not deliverance based, and hence
// has now been moved to the lib\framework directory
//
// If you are reading this file from the deliverance source directory, please delete it now




// To multiply a FRACT by a integer just use the normal operator 
//   e.g.   FractValue2=FractValue*Interger;
//
// save is true of divide

#ifndef _FRACTIONS_
#define _FRACTIONS_

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */
#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif

#include "types.h"

#ifdef DEBUG

SDWORD PercentFunc(char *File,UDWORD Line,SDWORD a,SDWORD b);
SDWORD PerNumFunc(char *File,UDWORD Line,SDWORD range,SDWORD a,SDWORD b);

#define PERCENT(a,b) PercentFunc(__FILE__,__LINE__,a,b)
#define PERNUM(range,a,b) PerNumFunc(__FILE__,__LINE__,range,a,b)

#else

#define PERCENT(a,b) (((a)*100)/(b))
#define PERNUM(range,a,b) (((a)*range)/(b))

#endif

#ifdef PSX

#include <sys\types.h>	// for PlayStation library types
#include "libgte.h"

#define PSX360 (4096)

#define FIXEDBITS (12)
#define FIXEDVAL (1<<FIXEDBITS)


typedef SDWORD FRACT;
typedef signed long long FRACT_D;

#define DIVACC (256)
#define MULTACC (256)


// largest value that the top part of the divide can be
#define FRACTDIVMAX ((0x7fffffff) / DIVACC)

// This allows the functions to compile without warnings on both debug & optimize build
// - the routines are defined once in the framework library, elsewhere they are inline
// - in non optimize build, they all call the framework library version
// - in an optimize build they should all be inline.

// #define FRACT_LIB_CODE needs to be defined in one and one file only
#ifdef DEFINE_INLINE
#define FUNCINLINE 
#else
#define FUNCINLINE __inline extern
#endif


FUNCINLINE FRACT MAKEFRACT(SDWORD input)
{
	return(input*FIXEDVAL);
}

FUNCINLINE SDWORD MAKEINT(FRACT input)
{
	return(input/FIXEDVAL);	// needed for sign
}


FUNCINLINE SDWORD ROUND(FRACT value)
{
	if (value >=0)
	{
		value+=(FIXEDVAL/2);
	}
	else
	{
		value-=(FIXEDVAL/2);
	}
	return(value/FIXEDVAL);	// needed for sign ... but should optimze into a shift
}



FUNCINLINE FRACT FRACTmul(FRACT inputA,FRACT inputB)
{
	return(((inputA/MULTACC)*(inputB/MULTACC))*(FIXEDVAL/MULTACC));
}

FUNCINLINE FRACT FRACTdiv(FRACT inputA,FRACT inputB)
{
	return(((inputA*DIVACC)/inputB)*(FIXEDVAL/DIVACC));
}

FUNCINLINE FRACT fSQRT(FRACT input)
{
	return(SquareRoot12(input));
}

FUNCINLINE FRACT iSQRT(UDWORD input)
{
	return(SquareRoot0(input));
}


#define FRACTCONST(a,b) ( ((a)*FIXEDVAL)/(b) )

#define FRACTabs(a) ( (a)<0 ? -(a) : (a) )


FUNCINLINE FRACT_D MAKEFRACT_D(SDWORD input)
{
	return(((FRACT_D)input)*FIXEDVAL);
}


FUNCINLINE SDWORD MAKEINT_D(FRACT_D input)
{
	return(input/FIXEDVAL);	// needed for sign
}

FUNCINLINE FRACT_D FRACTmul_D(FRACT_D inputA,FRACT_D inputB)
{
	return(((inputA/MULTACC)*(inputB/MULTACC))*(FIXEDVAL/MULTACC));
}

FUNCINLINE FRACT_D FRACTdiv_D(FRACT_D inputA,FRACT_D inputB)
{
	return(((inputA*DIVACC)/inputB)*(FIXEDVAL/DIVACC));
}

FUNCINLINE FRACT_D fSQRT_D(FRACT_D input)
{
	return((FRACT_D)SquareRoot12((FRACT)input));
}

FUNCINLINE FRACT_D iSQRT_D(UDWORD input)
{
	return((FRACT_D)SquareRoot0(input));
}

// these are useful if inputA is big and inputB is a fraction (i.e < 1.0 )
FUNCINLINE FRACT FRACTmul_1(FRACT inputA,FRACT inputB)
{
	return	(FRACT)((((FRACT_D)inputA)*inputB)/4096);
}

FUNCINLINE FRACT FRACTdiv_1(FRACT inputA,FRACT inputB)
{
	return (FRACT)((((FRACT_D)inputA)*4096)/inputB);
}



#else

#include <math.h>

typedef float FRACT;
typedef float FRACT_D;

#define ROUND(x) ((x)>=0 ? (SDWORD)((x) + 0.5) : (SDWORD)((x) - 0.5))

#define FAST_FRACTS

#ifdef FAST_FRACTS

#define MAKEFRACT(x) ((FRACT)(x))
#define FRACTmul(x,y) ((x)*(y))
#define FRACTdiv(x,y) ((x)/(y))
#define FRACTmul_1(x,y) ((x)*(y))
#define FRACTdiv_1(x,y) ((x)/(y))
#define fSQRT(x) ((FRACT)sqrt(x))

// Jeremy ... the usual leg breaking rule aplies if you remove this again
#define iSQRT(x) ((FRACT)sqrt(x))


#define FRACTCONST(a,b) (((float)(a)) / ((float)(b)))
#define FRACTabs(a) ((float)fabs(a))

#define MAKEFRACT_D(x) ((FRACT_D)(x))
#define FRACTmul_D(x,y) ((x)*(y))
#define FRACTdiv_D(x,y) ((x)/(y))
#define fSQRT_D(x) ((FRACT)sqrt(x))

//#define MAKEINT_D(x) ((SDWORD)(x))
__inline SDWORD MAKEINT_D (float f)
{
	SDWORD i;
	__asm fld f;
	__asm fistp i;
	return i;
}

//changed definitions
//#define MAKEINT(x) ((SDWORD)(x))
//#define MAKEINT(x) (ftol(x))
__inline SDWORD MAKEINT (float f)
{
	SDWORD i;
	__asm fld f;
	__asm fistp i;
	return i;
}


//#define fastRoot(x,y) (sqrt(x * x + y * y))
#define fastRoot(x,y) ((abs(x) > abs(y)) ? (abs(x) + abs(y)/2) : (abs(y) + abs(x)/2))

//unused definitions
//#define iSQRT_D(x) ((UDWORD)sqrt((double)x))
//#define iSQRT(x) ((UDWORD)sqrt((double)x))

#else

#define MAKEFRACT(x) ((FRACT)(x))
#define FRACTmul(x,y) ((x)*(y))
#define FRACTdiv(x,y) ((x)/(y))
#define FRACTmul_1(x,y) ((x)*(y))
#define FRACTdiv_1(x,y) ((x)/(y))
#define fSQRT(x) ((FRACT)sqrt(x))

#define FRACTCONST(a,b) (((float)(a)) / ((float)(b)))
#define FRACTabs(a) ((float)fabs(a))

#define MAKEFRACT_D(x) ((FRACT_D)(x))
#define FRACTmul_D(x,y) ((x)*(y))
#define FRACTdiv_D(x,y) ((x)/(y))
#define fSQRT_D(x) ((FRACT)sqrt(x))
// Jeremy ... the usual leg breaking rule aplies if you remove this again
#define iSQRT(x) ((FRACT)sqrt(x))

#define MAKEINT(x) ((SDWORD)(x))
#define MAKEINT_D(x) ((SDWORD)(x))

#define fastRoot(x,y) (sqrt(x * x + y * y))

//#define iSQRT_D(x) ((UDWORD)sqrt((double)x))
//#define iSQRT(x) ((UDWORD)sqrt((double)x))

#endif

#endif


#endif
