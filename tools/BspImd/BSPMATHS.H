/***************************************************************************/

#ifndef _BSPMATHS_H_
#define _BSPMATHS_H_

/***************************************************************************/

#include <math.h>

#include "types.h"
//#include "ptrlist.h"

#include "vertex.h"


typedef iIMDPoly * PSTRIANGLE;

/***************************************************************************/

#define		OPPOSITE_SIDE						0
#define		IN_PLANE							1
#define		SAME_SIDE							2
#define		SPLIT_BY_PLANE						3
#define		INTERSECTION_INSIDE_LINE_SEGMENT	4
#define		INTERSECTION_OUTSIDE_LINE_SEGMENT	5


#define SPLITTING_ERROR (-1)


#if(0)

#define		TOLERANCE							1.2E-05

#define		PI_DIV_180							(PI / 180)

/***************************************************************************/

#define IS_EQ(a,b) ((fabs((double)(a)-(b)) >= (double) TOLERANCE) ? 0 : 1)

/***************************************************************************/

#endif
void GetTriangleNormal( PSTRIANGLE psTri, iVectorHD * psN );
void	GetPlane( PSTRIANGLE psTri, HDPLANE *psPlane );

int IsPointOnPlane( HDPLANE *psPlane, iVectorHD * vP, HDVAL * pvDot );
int				IsTriOnPlane( PSTRIANGLE psTri, HDPLANE *psPlane );


int				SplitTriangleWithPlane( HDPLANE *psPlane,
										PSTRIANGLE psInputTri,
										PSBSPPTRLIST psSameSideTriList,
										PSBSPPTRLIST psOppoSideTriList );



iVectorHD *iNormalise(iVectorHD * v);
HDVAL iDotProduct( HDVAL * psD, iVectorHD * psA, iVectorHD * psB );



void dumpIMDvec(int VectorID);

void dumpvector(iVectorHD *Vec);
void dumpplane(HDPLANE *plane);
void dumppoly(iIMDPoly *psTri);





/***************************************************************************/

#endif		/* _BSPMATHS_H_ */

/***************************************************************************/
