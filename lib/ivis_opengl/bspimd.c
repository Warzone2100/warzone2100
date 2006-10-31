/*

	BSP Draw routines for iVis02

	- 5 Sept 1997  - Tim Cannell - Pumpkin Studios - Eidos



	- Main routines were written by Gareth Jones .... so if you find any bugs I think you better speak to him ...

*/

#include <string.h>

#include "lib/framework/frame.h"	// just for the typedef's
#include "lib/ivis_common/pietypes.h"
#include "piematrix.h"
#include "lib/ivis_common/ivisdef.h"// this can have the #define for BSPIMD in it
#include "lib/ivis_common/imd.h"// this has the #define for BSPPOLYID_TERMINATE
#include "lib/ivis_common/ivi.h"

#ifdef BSPIMD		// covers the whole file

#define BSP_BACKFACECULL	// if defined we perform the backface cull in the BSP (we get this for free (!))

//#define BSP_MAXDEBUG		// define this if you want max debug options (runs very slow)

#include "lib/ivis_common/bspimd.h"



#include "lib/ivis_common/bspfunc.h"


#include <stdio.h>
#include <assert.h>

// from imddraw.c
void DrawTriangleList(BSPPOLYID PolygonNumber);


// Local prototypes
static inline int IsPointOnPlane( PSPLANE psPlane, iVector * vP );
static iVector *IMDvec(int Vertex);
static void TraverseTreeAndRender( PSBSPTREENODE psNode);

iIMDShape *BSPimd=NULL;		// This is a global ... it is used in imddraw.c (for speed)


// Local static variables
static iVector *BSPScrPos=NULL;

static iVector *CurrentVertexList=NULL;
//static int CurrentVertexListCount=0;



extern BOOL NoCullBSP;	// Oh yes... a global externaly referenced variable....

/***************************************************************************/
/*
 * Calculates whether point is on same side, opposite side or in plane;
 *
 * returns OPPOSITE_SIDE if opposite,
 *         IN_PLANE if contained in plane,
 *         SAME_SIDE if same side
 *       Also returns pvDot - the dot product
 * - inputs vP vector to the point
 * - psPlane structure containing the plane equation
 */
/***************************************************************************/
static inline int IsPointOnPlane( PSPLANE psPlane, iVector * vP )
{
	iVectorf	vecP;
	FRACT Dot;

	/* validate input */
#ifdef BSP_MAXDEBUG
	ASSERT( PTRVALID(psPlane,sizeof(PLANE)),
			"IsPointOnPlane: invalid plane\n" );
#endif

	/* subtract point on plane from input point to get position vector */
	vecP.x = MAKEFRACT(vP->x - psPlane->vP.x);
	vecP.y = MAKEFRACT(vP->y - psPlane->vP.y);
	vecP.z = MAKEFRACT(vP->z - psPlane->vP.z);

	/* get dot product of result with plane normal (a,b,c of plane) */
	Dot = (FRACT)(FRACTmul(vecP.x,psPlane->a) + FRACTmul(vecP.y,psPlane->b) + FRACTmul(vecP.z,psPlane->c));

	/* if result is -ve, return -1 */
	if ( (abs((int)Dot)) <  FRACTCONST(1,100) )
	{
		return IN_PLANE;
	}
	else if ( Dot < 0 )
	{
		return OPPOSITE_SIDE;
	}
	return SAME_SIDE;
}



/*
	This is the main BSP Traversal routine. It Zaps through the tree (recursively) - and draws all the polygons
	for the IMD in the correct order ... pretty clever eh ..
*/

static void TraverseTreeAndRender( PSBSPTREENODE psNode)
{
	/* is viewer on same side? */
// On the playstation we need the list in reverse order (front most polygon first)
// so we just do the list the opposite way around - this affects the BACKFACE culling as well
	if ( IsPointOnPlane( &psNode->Plane, BSPScrPos ) == SAME_SIDE )
	{
		/* recurse on opposite side, render this node on same side,
		 * recurse on same side.
		 */

		if (psNode->link[LEFT]!=NULL) TraverseTreeAndRender( psNode->link[LEFT]);
		if (psNode->TriSameDir!=BSPPOLYID_TERMINATE) DrawTriangleList(psNode->TriSameDir);
#ifndef BSP_BACKFACECULL
		if (psNode->TriOppoDir!=BSPPOLYID_TERMINATE) DrawTriangleList(psNode->TriOppoDir);
#warning	LETS_FORCE_AN_ERROR_TO_MAKE_SURE_THAT_THIS_CODE_ISNT_COMPILED
#endif
		if (psNode->link[RIGHT]!=NULL) TraverseTreeAndRender( psNode->link[RIGHT]);
	}
	else
	/* viewer in plane or on opposite side */
	{
		/* recurse on same side, render this node on opposite side
		 * recurse on opposite side.
		 */
		if (psNode->link[RIGHT]!=NULL) TraverseTreeAndRender( psNode->link[RIGHT]);
		if (psNode->TriOppoDir!=BSPPOLYID_TERMINATE) DrawTriangleList(psNode->TriOppoDir);

#ifndef BSP_BACKFACECULL
		if (psNode->TriSameDir!=BSPPOLYID_TERMINATE) DrawTriangleList(psNode->TriSameDir);
#endif
		if (psNode->link[LEFT]!=NULL) 	TraverseTreeAndRender( psNode->link[LEFT]);
	}
}





/*
	These routines are used by the IMD Load_BSP routine
*/
typedef iIMDPoly * PSTRIANGLE;

static iVectorf *iNormalise(iVectorf * v);
static iVectorf * iCrossProduct( iVectorf * psD, iVectorf * psA, iVectorf * psB );
static void GetTriangleNormal( PSTRIANGLE psTri, iVectorf * psN,int pA, int pB, int pC );
PSBSPTREENODE InitNode(PSBSPTREENODE psBSPNode);


static FRACT GetDist( PSTRIANGLE psTri, int pA, int pB );

// little routine for getting an imd vector structure in the IMD from the vertex ID
static inline iVector *IMDvec(int Vertex)
{
#ifdef BSP_MAXDEBUG
	assert((Vertex>=0)&&(Vertex<CurrentVertexListCount));
#endif
	return (CurrentVertexList+Vertex);
}



/*
 Its easy enough to calculate the plane equation if there is only 3 points
 ... but if there is four things get a little tricky ...

In theory you should be able the pick any 3 of the points to calculate the equation, however
in practise mathematically inacurraceys mean that  you need the three points that are the furthest apart

*/
void GetPlane( iIMDShape *s, UDWORD PolygonID, PSPLANE psPlane )
{

	iVectorf Result;
	iIMDPoly *psTri;
	/* validate input */
	ASSERT( PTRVALID(psPlane,sizeof(PLANE)),
			"GetPlane: invalid plane\n" );

	psTri=&(s->polys[PolygonID]);
	CurrentVertexList=s->points;
#ifdef BSP_MAXDEBUG
	CurrentVertexListCount=s->npoints;	// for error detection
#endif

	if (psTri->npnts==4)
	{
		int pA,pB,pC;
		FRACT ShortDist,Dist;

		ShortDist=MAKEFRACT(999);
		pA=0;pB=1;pC=2;

		Dist=GetDist(psTri,0,1);		// Distance between two points in the quad
		if (Dist < ShortDist)	{	ShortDist=Dist;	pA=0;pB=2;pC=3;		}

		Dist=GetDist(psTri,0,2);
		if (Dist < ShortDist)	{	ShortDist=Dist;	pA=0;pB=1;pC=3;		}

		Dist=GetDist(psTri,0,3);
		if (Dist < ShortDist)	{	ShortDist=Dist;	pA=0;pB=1;pC=2;		}

		Dist=GetDist(psTri,1,2);
		if (Dist < ShortDist)	{	ShortDist=Dist;	pA=0;pB=1;pC=3;		}

		Dist=GetDist(psTri,1,3);
		if (Dist < ShortDist)	{	ShortDist=Dist;	pA=0;pB=1;pC=2;		}

		Dist=GetDist(psTri,2,3);
		if (Dist < ShortDist)	{	ShortDist=Dist;	pA=0;pB=1;pC=2;		}

		GetTriangleNormal(psTri,&Result,pA,pB,pC);
	}
	else
	{
		GetTriangleNormal(psTri,&Result,0,1,2);		// for a triangle there is no choice ...
	}

	/* normalise normal */
	iNormalise( &Result );

// This result MUST be cast to a fract and not called using MAKEFRACT
//
//  This is because on a playstation we are casting from FRACT->FRACT (so no conversion is needed)
//    ... and on a PC we are converting from DOUBLE->FLOAT  (so a cast is needed)
	psPlane->a = (FRACT)(Result.x);
	psPlane->b = (FRACT)(Result.y);
	psPlane->c = (FRACT)(Result.z);
	/* since plane eqn is ax + by + cz + d = 0,
	 * d = -(ax + by + cz).
	 */
	// Because we are multiplying a FRACT by a const we can use normal '*'
	psPlane->d = - (	( psPlane->a* IMDvec(psTri->pindex[0])->x ) +
		( psPlane->b* IMDvec(psTri->pindex[0])->y ) +
		( psPlane->c* IMDvec(psTri->pindex[0])->z ) );
	/* store first triangle vertex as point on plane for later point /
	 * plane classification in IsPointOnPlane
	 */
	memcpy( &psPlane->vP, IMDvec(psTri->pindex[0]), sizeof(iVector) );
}


/***************************************************************************/
/*
 * iCrossProduct
 *
 * Calculates cross product of a and b.
 */
/***************************************************************************/

static iVectorf * iCrossProduct( iVectorf * psD, iVectorf * psA, iVectorf * psB )
{
    psD->x = FRACTmul(psA->y , psB->z) - FRACTmul(psA->z ,psB->y);
    psD->y = FRACTmul(psA->z , psB->x) - FRACTmul(psA->x ,psB->z);
    psD->z = FRACTmul(psA->x , psB->y) - FRACTmul(psA->y ,psB->x);

    return psD;
}


static FRACT GetDist( PSTRIANGLE psTri, int pA, int pB )
{
	FRACT vx,vy,vz;
	FRACT sum_square,dist;

	vx = MAKEFRACT( IMDvec(psTri->pindex[pA])->x - IMDvec(psTri->pindex[pB])->x);
	vy = MAKEFRACT( IMDvec(psTri->pindex[pA])->y - IMDvec(psTri->pindex[pB])->y);
	vz = MAKEFRACT( IMDvec(psTri->pindex[pA])->z - IMDvec(psTri->pindex[pB])->z);



	sum_square = (FRACTmul(vx,vx)+FRACTmul(vy,vy)+FRACTmul(vz,vz) );
	dist = fSQRT(sum_square);
	return dist;
}


static void GetTriangleNormal( PSTRIANGLE psTri, iVectorf * psN,int pA, int pB, int pC )
{
	iVectorf vecA, vecB;

	/* validate input */
	ASSERT( PTRVALID(psTri,sizeof(iIMDPoly)),
			"GetTriangleNormal: invalid triangle\n" );

	/* get triangle edge vectors */
	vecA.x = MAKEFRACT( IMDvec(psTri->pindex[pA])->x - IMDvec(psTri->pindex[pB])->x);
	vecA.y = MAKEFRACT( IMDvec(psTri->pindex[pA])->y - IMDvec(psTri->pindex[pB])->y);
	vecA.z = MAKEFRACT( IMDvec(psTri->pindex[pA])->z - IMDvec(psTri->pindex[pB])->z);


	vecB.x = MAKEFRACT( IMDvec(psTri->pindex[pA])->x - IMDvec(psTri->pindex[pC])->x);
	vecB.y = MAKEFRACT( IMDvec(psTri->pindex[pA])->y - IMDvec(psTri->pindex[pC])->y);
	vecB.z = MAKEFRACT( IMDvec(psTri->pindex[pA])->z - IMDvec(psTri->pindex[pC])->z);

	/* normal is cross product */
	iCrossProduct( psN, &vecA, &vecB );
}


/***************************************************************************/
/*
 * Normalises the vector v
 */
/***************************************************************************/

static iVectorf *iNormalise(iVectorf * v)
{
    FRACT vx, vy, vz, mod, sum_square;

	vx = (FRACT)v->x;
	vy = (FRACT)v->y;
	vz = (FRACT)v->z;

 	if ((vx == 0) && (vy == 0) && (vz == 0))
	{
        return v;
	}


	sum_square = (FRACTmul(vx,vx)+FRACTmul(vy,vy)+FRACTmul(vz,vz) );
	mod = fSQRT(sum_square);

	v->x = FRACTdiv( vx , mod);
	v->y = FRACTdiv( vy , mod);
	v->z = FRACTdiv( vz , mod);

	return v;
}




PSBSPTREENODE InitNode(PSBSPTREENODE psBSPNode)
{
	PSBNODE			psBNode;

	/* create node triangle lists */
	psBSPNode->TriSameDir=BSPPOLYID_TERMINATE;
	psBSPNode->TriOppoDir=BSPPOLYID_TERMINATE;

	psBNode = (PSBNODE) psBSPNode;

	psBNode->link[LEFT] = NULL;
	psBNode->link[RIGHT] = NULL;

	return psBSPNode;
}


// PC Specific drawing routines


// Calculate the real camera position based on the coordinates of the camera and the camera
// distance - the result is stores in CameraLoc ,,  up is +ve Y
void GetRealCameraPos(OBJPOS *Camera,SDWORD Distance, iVector *CameraLoc)
{
	int Yinc;

//  as pitch is negative ... we need to subtract the value from y to go up the axis
	CameraLoc->y = Camera->y - 	((SIN(Camera->pitch) * Distance)>>FP12_SHIFT);
//	CameraLoc->y = Camera->y + 	((iV_SIN(Camera->pitch) * Distance)>>FP12_SHIFT);

	Yinc =  ((COS(Camera->pitch) * Distance)>>FP12_SHIFT);

//	DBPRINTF(("camera x=%d y=%d z=%d\n",Camera->x,Camera->y,Camera->z));
//	DBPRINTF(("pitch=%ld yaw=%ld Yinc=%ld Dist=%ld\n",Camera->pitch,Camera->yaw,Yinc,Distance));

	CameraLoc->x = Camera->x + ((SIN(-Camera->yaw) * (-Yinc))>>FP12_SHIFT);
	CameraLoc->z = Camera->z + ((COS(-Camera->yaw) * (Yinc))>>FP12_SHIFT);

//	DBPRINTF(("out) camera x=%d y=%d z=%d\n",CameraLoc->x,CameraLoc->y,CameraLoc->z));
//	DBPRINTF(("t=%d t1=%d t2=%d t3=%d z=%d\n",t,t1,t2,t3,CameraLoc->z));

}

void DrawBSPIMD(iIMDShape *IMDdef, iVector *pPos)
{
	BSPScrPos=pPos;
	BSPimd=IMDdef;

	TraverseTreeAndRender(IMDdef->BSPNode);
}

#endif			// #ifdef BSPIMD
