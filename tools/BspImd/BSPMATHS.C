
/***************************************************************************/



#define BSPIMD
#define PIETOOL

#include <memory.h>
#include <assert.h>
#include <stdio.h>

#include "ivi.h"	// for the typedefs
#include "frame.h"
#include "imd.h"
#include "bspimd.h"

#include "vertex.h"


#include "bspnode.h"
#include "bspmaths.h"

#include "debug.h"
#include "mem.h"

#include "bsptree.h"

extern BOOL Verbose;
extern BOOL Warnings;


#define MAXNEWVERTEX (6)

#define		PTOLERANCE							1.2E-05


extern PSBSPPTRLIST LoadedPolyList;	// linked list of polygons in shape - for BSP generation 


// This is the structure containing infomation about a new intersected vertex
typedef struct
{
	iVectorHD Vertex;		// Position of the vertex
	HDVAL u,v;			// Texture coord
} INTERVERTEX;

typedef struct
{
	int VertexID;	// Number of vertex in vertex list
	int u,v,tpage;		// Texture coordinates
	int PlanePos;	// Where on the split plane is this vertex - 	 0=OPPOSITE_SIDE, 1=IN_PLANE, 2=SAME_SIDE.
} VERTEXINFO;


// 
static int LinePlaneIntersection( HDPLANE *psPlane, INTERVERTEX *psP1, INTERVERTEX *psP2, INTERVERTEX *psPIntersection );


/***************************************************************************/

//#define		INTERSECT_THRESHOLD			(5E-5)

/***************************************************************************/


/***************************************************************************/
/*
 * Normalises the vector v
 */
/***************************************************************************/

iVectorHD *iNormalise(iVectorHD * v)
{
    HDVAL vx, vy, vz, mod, sum_square;

	vx = v->x;
	vy = v->y;
	vz = v->z;
	if ((vx == 0) && (vy == 0) && (vz == 0))
	{
        return v;
	}


	sum_square = (vx*vx)+(vy*vy)+(vz*vz);


	mod = sqrt(sum_square);

//	printf("%x = %x\n",sum_square,mod);

	v->x = vx/mod;
	v->y = vy/mod;
	v->z = vz/mod;

	return v;
}

/***************************************************************************/
/*
 * iCrossProduct
 *
 * Calculates cross product of a and b.
 */
/***************************************************************************/

iVectorHD * iCrossProduct( iVectorHD * psD, iVectorHD * psA, iVectorHD * psB )
{
    psD->x = (psA->y * psB->z) - (psA->z *psB->y);
    psD->y = (psA->z * psB->x) - (psA->x *psB->z);
    psD->z = (psA->x * psB->y) - (psA->y *psB->x);

    return psD;
}

/***************************************************************************/
/*
 * iDotProduct
 *
 * Calculates dot product of a and b.
 */
/***************************************************************************/

HDVAL iDotProduct( HDVAL * psD, iVectorHD * psA, iVectorHD * psB )
{
	*psD = (psA->x*psB->x) + (psA->y*psB->y) + (psA->z*psB->z);

	return (*psD);
}

/***************************************************************************/
/*
 * GetTriangleNormal
 */
/***************************************************************************/

//static iVector *CurrentVertexList=NULL;
//static int CurrentVertexListCount=0;

static iIMDShape *BSPimd=NULL;


// This needs to be called when info in the imd chages (like the point or polygon array)
//void GetCurrentIMDInfo(void)
//{
	//CurrentVertexList=BSPimd->points;
	//CurrentVertexListCount=BSPimd->npoints;	// for error detection
//}


void SetCurrentIMD(iIMDShape *imd)
{
	assert(imd);
	BSPimd=imd;
//	GetCurrentIMDInfo();

}


/*
static iVector *IMDvec(int Vertex)
{
	assert((Vertex>=0)&&(Vertex<CurrentVertexListCount));
	return (CurrentVertexList+Vertex);
}
*/

void GetTriangleNormal( PSTRIANGLE psTri, iVectorHD * psN )
{
	iVectorHD vecA, vecB;

	/* validate input */
	ASSERT( (PTRVALID(psTri,sizeof(iIMDPoly)),
			"GetTriangleNormal: invalid triangle\n") );

	/* get triangle edge vectors */
	vecA.x = ( GetIMDvec(psTri->pindex[0])->x - GetIMDvec(psTri->pindex[1])->x);
	vecA.y = ( GetIMDvec(psTri->pindex[0])->y - GetIMDvec(psTri->pindex[1])->y);
	vecA.z = ( GetIMDvec(psTri->pindex[0])->z - GetIMDvec(psTri->pindex[1])->z);

	vecB.x = ( GetIMDvec(psTri->pindex[0])->x - GetIMDvec(psTri->pindex[2])->x);
	vecB.y = ( GetIMDvec(psTri->pindex[0])->y - GetIMDvec(psTri->pindex[2])->y);
	vecB.z = ( GetIMDvec(psTri->pindex[0])->z - GetIMDvec(psTri->pindex[2])->z);

	/* normal is cross product */
	iCrossProduct( psN, &vecA, &vecB );
}

/***************************************************************************/
/*
 * LinePlaneIntersection
 *
 * Finds point of intersection of line and plane;
 *
 * line eqn is P = P1 + ud and plane eqn is n.P = s, where
 *
 * s  is plane scalar constant,
 * P  is point of intersection,
 * n  is plane normal,
 * P1 is point on line,
 * u  is line scalar constant,
   d  is line vector
 *
 * so n.(P1 + ud) = s,
 *
 * therefore u = s - n.P1
 *               --------
 *                  n.d
 */
/***************************************************************************/

static int LinePlaneIntersection( HDPLANE *psPlane, INTERVERTEX *psP1, INTERVERTEX *psP2, INTERVERTEX *psPIntersection )
{
	iVectorHD	d;
	iVectorHD NormalisedVector;
	iVectorHD PlaneNormal;

	HDVAL	u, nDotP1, nDotd;
	HDVAL du,dv,Fu,Fv;

	/* validate input */
	ASSERT( (PTRVALID(psPlane,sizeof(HDPLANE)),
			"LinePlaneIntersection: invalid plane\n") );

	if (Verbose==TRUE)
	{
		printf("LPI:\nP1 x=%f y=%f z=%f\n",psP1->Vertex.x,psP1->Vertex.y,psP1->Vertex.z);
		printf("P2 x=%f y=%f z=%f\n",psP2->Vertex.x,psP2->Vertex.y,psP2->Vertex.z);
		printf("Plane a=%f b=%f c=%f d=%f\n",psPlane->a,psPlane->b,psPlane->c,psPlane->d);
	}

	/* get line vector d */
	d.x = psP2->Vertex.x - psP1->Vertex.x;
	d.y = psP2->Vertex.y - psP1->Vertex.y;
	d.z = psP2->Vertex.z - psP1->Vertex.z;

	du= psP2->u - psP1->u;		// texture coords
	dv= psP2->v - psP1->v;



	Fu = (HDVAL)(psP1->u);
	Fv = (HDVAL)(psP1->v);


	/* get dot product of plane normal and line vector    n.d  */


	memcpy(&NormalisedVector,&d,sizeof(iVectorHD));


	if (Verbose==TRUE)
	{
		printf("Vector between the two points on the line : x=%f y=%f z=%f\n",NormalisedVector.x,NormalisedVector.y,NormalisedVector.z);
	}


	PlaneNormal.x=psPlane->a;
	PlaneNormal.y=psPlane->b;
	PlaneNormal.z=psPlane->c;

	iDotProduct( &nDotd, &PlaneNormal,  &NormalisedVector );


	memcpy(&NormalisedVector,&psP1->Vertex,sizeof(iVectorHD));
//	iNormalise(&NormalisedVector);
	/* get dot product of plane normal and point    n.P1  */

	if (Verbose)
	{
		printf("PlaneNormal: x=%f y=%f z=%f\n",PlaneNormal.x,PlaneNormal.y,PlaneNormal.z);
		printf("P1 Vector  : x=%f y=%f z=%f\n",NormalisedVector.x,NormalisedVector.y,NormalisedVector.z);

	}

	
	
	iDotProduct( &nDotP1,&PlaneNormal, &NormalisedVector );


	/* check for divide by zero; implies line and plane coincident,
	 * so return IN_PLANE (numerical error)
	 */
	if ( ((nDotd)<PTOLERANCE)&&((nDotd)> -PTOLERANCE)	)
	{
		if (Verbose)
			printf("Dot Product between normal of plane and vector between two points is %f\nHence this line is parralel to the plane\n",nDotd);

		return IN_PLANE;
	}

	if (Verbose==TRUE)
	{
		printf("nDotP1=%f nDotd=%f	\n",nDotP1,nDotd);
	}
	u = (((-psPlane->d) - nDotP1) / nDotd);


	/* substitute u into line equation to get intersection */


	// u is the fraction from P1 to P2 that the intersection has occured
	// ... hence if u=0.00 then the intersection is at P1 
	// ... if u=1.00 then the intersection is at P2


	psPIntersection->Vertex.x = (psP1->Vertex.x + (u*d.x));
	psPIntersection->Vertex.y = (psP1->Vertex.y + (u*d.y));
	psPIntersection->Vertex.z = (psP1->Vertex.z + (u*d.z));

	psPIntersection->u =   (Fu + (u*du));
	psPIntersection->v =   (Fv + (u*dv));


	if (Verbose==TRUE)
	{

#ifdef WIN32
		printf("fraction from start of line to end =%f\n",u);
#else
		printf("fraction from start of line to end =%d\n",u);
#endif
	}

	/* check for intersection */

	// if u is between 0 & 1 then the point is on the line

	if ((psPIntersection->Vertex.x == psP1->Vertex.x) &&
		(psPIntersection->Vertex.y == psP1->Vertex.y) &&
		(psPIntersection->Vertex.z == psP1->Vertex.z) )
	{
//		printf("intersection on point 1\n");
		return INTERSECTION_OUTSIDE_LINE_SEGMENT;

	}

	if ((psPIntersection->Vertex.x == psP2->Vertex.x) &&
		(psPIntersection->Vertex.y == psP2->Vertex.y) &&
		(psPIntersection->Vertex.z == psP2->Vertex.z) )
	{
//		printf("intersection on point 2\n");
		return INTERSECTION_OUTSIDE_LINE_SEGMENT;
	}

#define		LTOLERANCE							1.2E-05

	if ( (u > LTOLERANCE) && (u < (1.0 - LTOLERANCE) ) )
	{
		return INTERSECTION_INSIDE_LINE_SEGMENT;
	}
	else
	{
		return INTERSECTION_OUTSIDE_LINE_SEGMENT;
	}
}






// Removes a polygon from the list of polygons held in the IMD
// - uses the linked list of polygons.

// We will need to re-create the list of polygons from the linked list after we have finshed BSPing
BOOL RemovePolygon(PSTRIANGLE DeletedTri)
{
	assert(DeletedTri);
	// We delete allocated memory from within the triangle
	// - we don't however delete the triangle itself
	//   - this is because it is part of the large areas of polygons, after BSP is complete we will regenerate that array
	FREE(DeletedTri->pindex); // index to the vertex point numbers
	FREE(DeletedTri->vrt);	// index to texture coord info

	list_Remove(LoadedPolyList,DeletedTri);	// remove from linked list

	
//	list_Remove(BSPimd->PolyList,DeletedTri);	// remove from linked list
	return TRUE;
}




// Generates a new polygon, and adds it to the list in the same side as PointA
// Some of the info is copied from oldpoly - normal, polygon flags, that sort of thing
//
// - oldpoly must be on the same plane as the one we are creating  - not a problem for polygon splitting
//
// will generate a triangle if PointD==NULL
//
void GenerateQuad(VERTEXINFO *PointA,VERTEXINFO *PointB,VERTEXINFO *PointC,  VERTEXINFO *PointD, iIMDPoly *OldPoly,
				PSBSPPTRLIST psSameSideTriList, PSBSPPTRLIST psOppoSideTriList )
{
	PSTRIANGLE NewTri;

	int NumPoints=4;

		
	if (PointD==NULL) NumPoints=3;

	NewTri=MALLOC(sizeof(iIMDPoly));
	if (NewTri==NULL)
	{
		printf("no mem for split polygon\n");
		return;
	}
	
if (Verbose==TRUE)
{
	if (PointD!=NULL)
				printf("NewQuad @%p Vertices(%d,%d,%d,%d)\n",NewTri,PointA->VertexID,PointB->VertexID,PointC->VertexID,PointD->VertexID);
		else	printf("NewTri  @%p Vertices(%d,%d,%d)\n",NewTri,PointA->VertexID,PointB->VertexID,PointC->VertexID);


	if (PointD!=NULL)
				printf("NewQuad TexCoord(%d,%d) (%d,%d) (%d,%d) (%d,%d)\n",PointA->u,PointA->v,PointB->u,PointB->v,PointC->u,PointC->v,PointD->u,PointD->v);
		else	printf("NewTri TexCoord(%d,%d) (%d,%d) (%d,%d)\n",PointA->u,PointA->v,PointB->u,PointB->v,PointC->u,PointC->v);

}


	/*

	  - check for duplicate points
	*/
	if (PointD==NULL)
	{

		if ((PointA->VertexID==PointB->VertexID)||
			(PointA->VertexID==PointC->VertexID)||
			(PointB->VertexID==PointC->VertexID))
		{
			printf("NewTri: - duplicate points - not generated!\n");
			return;
		}
	}

	else
	{

		if (
			(PointA->VertexID==PointB->VertexID)||
			(PointA->VertexID==PointC->VertexID)||
			(PointA->VertexID==PointD->VertexID)||
			(PointB->VertexID==PointC->VertexID)||
			(PointB->VertexID==PointD->VertexID)||
			(PointC->VertexID==PointD->VertexID)
			)
		{
			printf("NewQuad: - duplicate points - not generated!\n");
			return;
		}


	}




	NewTri->pindex=MALLOC(sizeof(int)* NumPoints);
	NewTri->vrt=MALLOC(sizeof(iVertex)* NumPoints);

	if ((NewTri->pindex==NULL)||(NewTri->vrt==NULL))
	{
		printf("no mem for spliy polygon bits\n");
		return;
	}



	NewTri->flags=OldPoly->flags;	// we need some indication that this polygon is a new one - so that when we regenerate the list after completing the BSP, we can delete it individually rather than from the initial array


	NewTri->flags |= iV_IMD_BSPFRESH;	// added by the bsp -

#ifndef PIEPSX
	NewTri->pTexAnim = OldPoly->pTexAnim;		
	NewTri->zcentre=OldPoly->zcentre;	// this cant be right!
#endif

	NewTri->npnts=NumPoints;
	NewTri->pindex[0]=PointA->VertexID;
	NewTri->pindex[1]=PointB->VertexID;
	NewTri->pindex[2]=PointC->VertexID;
//	NewTri->anim_delay=OldPoly->anim_delay;	// this isn't used is it ?
//	NewTri->anim_frame=OldPoly->anim_frame;
	NewTri->vrt[0].u=PointA->u;
	NewTri->vrt[0].v=PointA->v;
//	NewTri->vrt[0].tpage=PointA->tpage;

	NewTri->vrt[1].u=PointB->u;
	NewTri->vrt[1].v=PointB->v;
//	NewTri->vrt[1].tpage=PointB->tpage;

	NewTri->vrt[2].u=PointC->u;
	NewTri->vrt[2].v=PointC->v;
//	NewTri->vrt[2].tpage=PointC->tpage;

	if (PointD != NULL)
	{
		NewTri->pindex[3]=PointD->VertexID;
		NewTri->vrt[3].u=PointD->u;
		NewTri->vrt[3].v=PointD->v;
//		NewTri->vrt[3].tpage=PointD->tpage;
	}

	memcpy(&(NewTri->normal),&(OldPoly->normal),sizeof(iVector));	// copy the plane normal...


	if (PointA->PlanePos == SAME_SIDE)
	{
//		printf("adding new poly to same side list:%p\n",psSameSideTriList);
		list_Add(psSameSideTriList,NewTri);	// add to the linked list
	}
	else
	{
//		printf("adding new poly to oppo side list:%p\n",psOppoSideTriList);
		list_Add(psOppoSideTriList,NewTri);	// add to the linked list
	}


}



/***************************************************************************/
/*
 *
 */
/***************************************************************************/




int	SplitPolygonWithPlane( HDPLANE *psPlane, PSTRIANGLE psInputPoly,
				PSBSPPTRLIST psSameSideTriList, PSBSPPTRLIST psOppoSideTriList )
{
	iVectorHD		vIntersect;

	int VertexID,i,VertexCount;

	VERTEXINFO	NewVertex[MAXNEWVERTEX];

//	PSTRIANGLE	psTri;

	int			iCurVertex = 0, iV, iNextVert,
				iIntersectVert = 0, iRet,
				iSplitCount = 0, iNewVert = 0;
	int			iPos[3];
	BOOL 		InPlane[MAXNEWVERTEX];	// has this edge been split ?
	HDVAL		vDot;

	int t;

   INTERVERTEX iVert1,iVert2,iVertResult;

	/* validate input */
//	ASSERT( (PTRVALID(psInputPoly,sizeof(TRIANGLE)),
//			"SplitTriangleWithPlane: invalid triangle\n") );
	ASSERT( (PTRVALID(psPlane,sizeof(HDPLANE)),
			"SplitTriangleWithPlane: invalid plane\n") );

	/* copy triangle to vertex array */
	/* init count array */


	iPos[0] = iPos[1] = iPos[2] = 0;

	InPlane[0]=InPlane[1]=InPlane[2]=InPlane[3]=InPlane[4]=FALSE;

	VertexCount=psInputPoly->npnts;

	if (Verbose==TRUE)
	{
		printf("Vertex count=%d\n",VertexCount);
	}
	assert((VertexCount==3) || (VertexCount==4));	// only works on tris or quads

	/* find intersections */
	for ( iV=0; iV<VertexCount; iV++ )
	{
		assert(iNewVert < MAXNEWVERTEX);


		/* find side of plane vertex lies on */
		iRet = IsPointOnPlane( psPlane,  GetIMDvec(psInputPoly->pindex[iV]), &vDot );

		/* increment count of sides -
		 * index 0=OPPOSITE_SIDE, 1=IN_PLANE, 2=SAME_SIDE.
		 */


		if (Verbose==TRUE)
		{
			UDWORD VertID=psInputPoly->pindex[iV];
			printf("point %d [%d] (%f,%f,%f)- relative to plane:",iV,VertID,GetIMDvec(VertID)->x,GetIMDvec(VertID)->y,GetIMDvec(VertID)->z);
		}
		if (iRet==OPPOSITE_SIDE)
		{
			if (Verbose==TRUE)	printf("OPPOSITE_SIDE\n");
		}
		else if (iRet==IN_PLANE)
		{
			InPlane[iNewVert]=TRUE;
			if (Verbose==TRUE)	printf("IN_PLANE\n");
		}
		else if (iRet==SAME_SIDE)
		{
			if (Verbose==TRUE)	printf("SAME_SIDE\n");
		}
		else
		{
			if (Verbose==TRUE)	printf(" umm - somthing else!\n");
		}


		iPos[iRet]++;



		/* save side of point */
		NewVertex[iNewVert].PlanePos= iRet;
		
		/* get poly side intersection with plane */
		iNextVert = (iV+1) % (VertexCount);

								   


		memcpy(&iVert1.Vertex,GetIMDvec(psInputPoly->pindex[iV]),sizeof(iVectorHD));
		memcpy(&iVert2.Vertex,GetIMDvec(psInputPoly->pindex[iNextVert]),sizeof(iVectorHD));



		iVert1.u=psInputPoly->vrt[iV].u;
		iVert1.v=psInputPoly->vrt[iV].v;
		iVert2.u=psInputPoly->vrt[iNextVert].u;
		iVert2.v=psInputPoly->vrt[iNextVert].v;


		if (iVert1.v > 512 || iVert2.v > 512)
		{
			printf("BAD BAD BAD Texture coord - %d %d\n",iVert1.v,iVert2.v);
		}


/*

#define		OPPOSITE_SIDE						0
#define		IN_PLANE							1
#define		SAME_SIDE							2
#define		SPLIT_BY_PLANE						3
#define		INTERSECTION_INSIDE_LINE_SEGMENT	4
#define		INTERSECTION_OUTSIDE_LINE_SEGMENT	5

*/

		iRet = LinePlaneIntersection( psPlane, &iVert1, &iVert2, &iVertResult);
	if (Verbose==TRUE)	
	{
		printf("LinePlaneInter Between vert %d & %d is ",iV,iNextVert);
		switch (iRet)
		{
		case IN_PLANE:
				printf("IN_PLANE !!!! - (i.e. line is paralel to plane)\n");
				break;
		case INTERSECTION_INSIDE_LINE_SEGMENT:
				printf("INTERSECTION_INSIDE_LINE_SEGMENT\n");
				break;
		case INTERSECTION_OUTSIDE_LINE_SEGMENT:
				printf("INTERSECTION_OUTSIDE_LINE_SEGMENT\n");
				break;

		
		case OPPOSITE_SIDE:
				printf("UNEXPECTED OPPOSITE_SIDE\n");
				break;
		case SAME_SIDE:
				printf("UNEXPECTED SAME_SIDE\n");
				break;
		case SPLIT_BY_PLANE:
				printf("UNEXPECTED SPLIT_BY_PLANE\n");
				break;
		default:
			printf("unexpected value - %d\n",iRet);
		}
	}


		memcpy(&vIntersect,&iVertResult.Vertex,sizeof(iVectorHD));

		NewVertex[iNewVert].VertexID=psInputPoly->pindex[iV];
		NewVertex[iNewVert].u = psInputPoly->vrt[iV].u;
		NewVertex[iNewVert].v = psInputPoly->vrt[iV].v;
//		NewVertex[iNewVert].tpage = psInputPoly->vrt[iV].tpage;

		iNewVert++;
		
		/* update counts if intersection found */
		if ( iRet == INTERSECTION_INSIDE_LINE_SEGMENT )
		{
			/* store split edge index */
			iSplitCount++;

			/* increment intersection counter */
			iIntersectVert++;
			/* add intersection vertex to new array */

			if (iVertResult.v > 512)
			{
				printf("BAD TEXTURE COORD - %d\n",iVertResult.v);
			}

			VertexID=AddNewIMDVertex(&vIntersect);	// Add a new vertex to the IMD file - check to see if it's not already there

			if (Verbose==TRUE)
			{
				printf("New Vertex added - %d   (%f,%f,%f)\n",VertexID,vIntersect.x,vIntersect.y,vIntersect.z);
				dumpIMDvec(VertexID);
			}

			if (VertexID==-1)
			{
				printf("unable to add a new vertex\n");
			}


			// Just a little sanity check to make sure that the new point is in the plane
			if (IsPointOnPlane(psPlane, GetIMDvec(VertexID),&vDot)!=IN_PLANE)
			{
				printf("New vertex is not in the plane!!!\n");
				dumpplane(psPlane);
				dumpIMDvec(VertexID);
				printf("Dot prod = %f\n",vDot);
			}
			
			NewVertex[iNewVert].u = (int)iVertResult.u;
			NewVertex[iNewVert].v = (int)iVertResult.v;
//			NewVertex[iNewVert].tpage = psInputPoly->vrt[iV].tpage;	// tpage remains constant thoughout - it doesn't even need to be in this structure
			NewVertex[iNewVert].VertexID=VertexID;
			NewVertex[iNewVert].PlanePos=IN_PLANE;

			InPlane[iNewVert]=TRUE;

			iNewVert++;
		}
	}

	assert(iNewVert <= MAXNEWVERTEX);

	/* split input polygon into new tri/quads and add to correct sides */

if (Verbose==TRUE)	
{
	for (t=0;t<iNewVert;t++)
	{
		int VertID;

		VertID=NewVertex[t].VertexID;
		printf("vert %d id=%d Inplane=%d  x=%f y=%f z=%f u=%f v=%f\n",t,VertID,InPlane[t],GetIMDvec(VertID)->x,GetIMDvec(VertID)->y,GetIMDvec(VertID)->z,NewVertex[t].u,NewVertex[t].v);
	}
}




	// End result is a hex - split into either 1tri/1quad&1tri or 2quads
	if (iNewVert==6)
	{
		// first find the first point that is on the plane
		for (i=0;i<6;i++)
			if (InPlane[i]==TRUE) break;

		if (InPlane[(i+1)%6]==TRUE) 
			{
				int t,VertLeft;
				printf("two splits consect I\n"); 
				
				for (t=0;t<iNewVert;t++)
				{
					int VertID;

					VertID=NewVertex[t].VertexID;
					printf("vert %d id=%d Inplane=%d  x=%f y=%f z=%f\n",t,VertID,InPlane[t],GetIMDvec(VertID)->x,GetIMDvec(VertID)->y,GetIMDvec(VertID)->z);
				}

				printf("Removing point %d\n",i+1);

				VertLeft=4-i;
				if (VertLeft>0)
				{
					memmove(&NewVertex[i+1],&NewVertex[i+2],sizeof(VERTEXINFO)*VertLeft);
					memmove(&InPlane[i+1],&InPlane[i+2],sizeof(BOOL)*VertLeft);
				}

				iNewVert--;

//				for (t=0;t<iNewVert;t++)
//				{
//					int VertID;
//					VertID=NewVertex[t].VertexID;
//					printf("vert %d id=%d Inplane=%d  x=%d y=%d z=%d\n",t,VertID,InPlane[t],IMDvec(VertID)->x,IMDvec(VertID)->y,IMDvec(VertID)->z);
//				}

				 // Oh my god  .... its a goto
				 goto Check5Vert;	// Skip the rest of the 6-polygon checking and goto the 5-polygon checking

			}

		if (InPlane[(i+2)%6]==TRUE) 
		{
			if (i>=4) {printf("not enough splits\n"); return SPLIT_BY_PLANE; }

#if (DBGPRINT>2)
			printf("Split Pos A\n");
#endif
			GenerateQuad(&NewVertex[(i+1)%6],&NewVertex[(i+2)%6],&NewVertex[i],NULL,psInputPoly,psSameSideTriList,psOppoSideTriList);

			GenerateQuad(&NewVertex[(i+5)%6],&NewVertex[i],&NewVertex[(i+2)%6],&NewVertex[(i+3)%6],psInputPoly,psSameSideTriList,psOppoSideTriList); 
			GenerateQuad(&NewVertex[(i+5)%6],&NewVertex[(i+3)%6],&NewVertex[(i+4)%6],NULL,psInputPoly,psSameSideTriList,psOppoSideTriList);	
			return SPLIT_BY_PLANE;
		}

		if (InPlane[(i+3)%6]==TRUE)
		{
			if (i>=3) {printf("not enough splits\n"); return SPLIT_BY_PLANE; }

#if (DBGPRINT>2)
			printf("Split Pos B\n");
#endif
			GenerateQuad(&NewVertex[(i+1)%6],&NewVertex[(i+2)%6],&NewVertex[(i+3)%6],&NewVertex[(i+0)%6],psInputPoly,psSameSideTriList,psOppoSideTriList); 
			GenerateQuad(&NewVertex[(i+4)%6],&NewVertex[(i+5)%6],&NewVertex[(i+0)%6],&NewVertex[(i+3)%6],psInputPoly,psSameSideTriList,psOppoSideTriList); 
			return SPLIT_BY_PLANE;
		}

		if (InPlane[(i+4)%6]==TRUE)
		{
		if (i>=2) {printf("not enough splits\n"); return SPLIT_BY_PLANE; }
#if (DBGPRINT>2)
			printf("Split Pos A1\n");
#endif
			GenerateQuad(&NewVertex[(i+1)%6],&NewVertex[(i+2)%6],&NewVertex[i],NULL,psInputPoly,psSameSideTriList,psOppoSideTriList);

			GenerateQuad(&NewVertex[(i+2)%6],&NewVertex[(i+3)%6],&NewVertex[(i+4)%6],&NewVertex[i],psInputPoly,psSameSideTriList,psOppoSideTriList); 
			GenerateQuad(&NewVertex[(i+5)%6],&NewVertex[i],&NewVertex[(i+4)%6],NULL,psInputPoly,psSameSideTriList,psOppoSideTriList);	
			return SPLIT_BY_PLANE;
		}
		printf("Generic splitting error A\n");
		return SPLITTING_ERROR;
	}


Check5Vert:
	if (iNewVert==5)
	{
		// first find the first point that is on the plane
		for (i=0;i<5;i++)
			if (InPlane[i]==TRUE) break;

		if (InPlane[(i+1)%5]==TRUE)
		{
			int t,VertLeft;


			printf("two splits consect II\n"); 
				for (t=0;t<iNewVert;t++)
				{
					int VertID;

					VertID=NewVertex[t].VertexID;
					printf("vert %d id=%d Inplane=%d  x=%d y=%d z=%d\n",t,VertID,InPlane[t],GetIMDvec(VertID)->x,GetIMDvec(VertID)->y,GetIMDvec(VertID)->z);
				}

				printf("Removing point %d\n",i+1);

				VertLeft=3-i;
				if (VertLeft>0)
				{
					memmove(&NewVertex[i+1],&NewVertex[i+2],sizeof(VERTEXINFO)*VertLeft);
					memmove(&InPlane[i+1],&InPlane[i+2],sizeof(BOOL)*VertLeft);
				}

				iNewVert--;

//				for (t=0;t<iNewVert;t++)
//				{
//					int VertID;
//					VertID=NewVertex[t].VertexID;
//					printf("vert %d id=%d Inplane=%d  x=%d y=%d z=%d\n",t,VertID,InPlane[t],IMDvec(VertID)->x,IMDvec(VertID)->y,IMDvec(VertID)->z);
//				}

				 // Oh my god  .... its a goto
				 goto Check4Vert;	// Skip the rest of the 5-polygon checking and goto the 4-polygon checking




		}

		if (InPlane[(i+2)%5]==TRUE) 
		{
			if (i>=3) {printf("not enough splits\n"); return SPLIT_BY_PLANE; }

#if (DBGPRINT>2)
			printf("Split Pos C\n");
#endif
			GenerateQuad(&NewVertex[(i+1)%5],&NewVertex[(i+2)%5],&NewVertex[i],NULL,psInputPoly,psSameSideTriList,psOppoSideTriList);
			GenerateQuad(&NewVertex[(i+3)%5],&NewVertex[(i+4)%5],&NewVertex[i],&NewVertex[(i+2)%5],psInputPoly,psSameSideTriList,psOppoSideTriList); 
			return SPLIT_BY_PLANE;
		}

		if (InPlane[(i+3)%5]==TRUE)
		{
			if (i>=2) {printf("not enough splits\n"); return SPLIT_BY_PLANE; }

#if (DBGPRINT>2)
			printf("Split Pos D\n");
#endif
			GenerateQuad(&NewVertex[(i+4)%5],&NewVertex[(i+0)%5],&NewVertex[(i+3)%5],NULL,psInputPoly,psSameSideTriList,psOppoSideTriList);
			GenerateQuad(&NewVertex[(i+1)%5],&NewVertex[(i+2)%5],&NewVertex[(i+3)%5],&NewVertex[(i+0)%5],psInputPoly,psSameSideTriList,psOppoSideTriList); 
			return SPLIT_BY_PLANE;
		}
		printf("Generic splitting error B\n");
		return SPLITTING_ERROR;
	}


Check4Vert:		/// YAK

	if (iNewVert==4)
	{
		int j;	// debug
		// first find the first point that is on the plane
		for (i=0;i<4;i++)
			if (InPlane[i]==TRUE) break;

		if (InPlane[(i+1)%4]==TRUE) {printf("two splits consect III\n"); return SPLIT_BY_PLANE; }

		if (InPlane[(i+2)%4]==TRUE) 
		{
			if (i>=2) {printf("not enough splits\n"); return SPLIT_BY_PLANE; }
#if (DBGPRINT>2)
			printf("Split Pos E\n");
#endif

			GenerateQuad(&NewVertex[(i+1)%4],&NewVertex[(i+2)%4],&NewVertex[i],NULL,psInputPoly,psSameSideTriList,psOppoSideTriList);
			GenerateQuad(&NewVertex[(i+3)%4],&NewVertex[(i+0)%4],&NewVertex[(i+2)%4],NULL,psInputPoly,psSameSideTriList,psOppoSideTriList); 
			return SPLIT_BY_PLANE;
		}


		printf("Generic splitting error C - Bad number of points on plane(A Quad polygon was created by a split)\n");
		for (j=0;j<4;j++)
		{
			if (InPlane[j]==TRUE)
				printf("    Point %j is on the splitting plane\n",j);
		}





		return SPLITTING_ERROR;
	}

	printf("No splits found !\n");
	/* assign poly to side with most points in it */
	if ( iPos[SAME_SIDE] > iPos[OPPOSITE_SIDE] )
	{
		return SAME_SIDE;
	}
	else
	{
		return OPPOSITE_SIDE;
	}


 }


HDVAL PlaneToPointDistance(HDPLANE *psPlane, iVectorHD *vp)
{
	HDVAL Dist;

	Dist = psPlane->a * (vp->x);
	Dist+= psPlane->b * (vp->y);
	Dist+= psPlane->c * (vp->z);
	Dist+= psPlane->d;

	return Dist;
}

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

int IsPointOnPlane( HDPLANE *psPlane, iVectorHD * vP, HDVAL * pvDot )
{
	iVectorHD	vecP;
	iVectorHD PlaneNormal;

	/* validate input */
	ASSERT( (PTRVALID(psPlane,sizeof(HDPLANE)),
			"IsPointOnPlane: invalid plane\n") );

//	if (getdebug()==1)
//	{
//		printf("pos=(%d,%d,%d) planep=(%d,%d,%d)\n",
			//vP->x,vP->y,vP->z,
			//psPlane->vP.x,psPlane->vP.y,psPlane->vP.z);
	//}

	/* subtract point on plane from input point to get position vector */
	vecP.x = (vP->x - psPlane->vP.x);
	vecP.y = (vP->y - psPlane->vP.y);
	vecP.z = (vP->z - psPlane->vP.z);


	/* get dot product of result with plane normal (a,b,c of plane) */
	PlaneNormal.x= psPlane->a;
	PlaneNormal.y= psPlane->b;
	PlaneNormal.z= psPlane->c;

	iDotProduct( pvDot, &vecP, &PlaneNormal );

#ifdef WIN32
//	printf("vP=(%f,%f,%f) plane=(%f,%f,%f) dot=%f\n",
			//vecP.x,vecP.y,vecP.z,
			//psPlane->a,psPlane->b,psPlane->c, *pvDot);
#else
	//printf("vP=(%d,%d,%d) plane=(%d,%d,%d) dot=%d\n",
	//		vecP.x,vecP.y,vecP.z,
	//		psPlane->a,psPlane->b,psPlane->c, *pvDot);
#endif



	/* if result is -ve, return -1 */
	if ( ((*pvDot)<PTOLERANCE)&&((*pvDot)> -PTOLERANCE)	)
//	if ( abs(*pvDot) <  TOLEREANCE )
//	if ( abs(MAKEINT(*pvDot)) < TOLERANCE )
	{
//		printf("point:inplane\n");
		return IN_PLANE;
	}
	else if ( *pvDot < 0 )
	{
//		printf("point:opposite\n");
		return OPPOSITE_SIDE;
	}
	else
	{
//		printf("point:same\n");
		return SAME_SIDE;
	}

	return 0;
}

/***************************************************************************/
/*
 * Checks three vertices of tri to see if on same side, opposite side
 * or in plane of triangle.
 *
 * returns OPPOSITE_SIDE  if opposite,
 *         IN_PLANE       if contained in plane,
 *         SAME_SIDE      if same side,
 *         SPLIT_BY_PLANE if split by plane
 */
/***************************************************************************/

int IsTriOnPlane( PSTRIANGLE psTri, HDPLANE *psPlane )
{
	int			iPosV1, iPosV2, iPosV3;
	HDVAL	vDot1, vDot2, vDot3;

	/* validate input */
//	ASSERT( (PTRVALID(psTri,sizeof(TRIANGLE)),
//			"IsTriOnPlane: invalid triangle\n") );
	ASSERT( (PTRVALID(psPlane,sizeof(HDPLANE)),
			"IsTriOnPlane: invalid plane\n") );

	iPosV1 = IsPointOnPlane( psPlane, GetIMDvec(psTri->pindex[0]), &vDot1 );
	iPosV2 = IsPointOnPlane( psPlane, GetIMDvec(psTri->pindex[1]), &vDot2 );
	iPosV3 = IsPointOnPlane( psPlane, GetIMDvec(psTri->pindex[2]), &vDot3 );

//	printf("IsTriOnPlane - %d %d %d\n",iPosV1,iPosV2,iPosV3);


	if ( iPosV1 == iPosV2 && iPosV1 == iPosV3 )
	{
		return iPosV1;
	}
	else
	{

// if any of the points are on the plane, then we need to look at the remaning points to see if they are
// behind or infront of the plane, indicating a split or all points on one side... there must be a neater way of handling this ... with bits perhaps

		if (iPosV1==IN_PLANE)
		{
			if (iPosV2==IN_PLANE)
			{
				return iPosV3;	// if points 1&2 are on the plane then use point 3 as the direction
			}
			else
			{
				if (iPosV3==IN_PLANE)
				{
					return iPosV2;	// if points 1&3 are on the plane then use point 2
				}
				else
				{
					if (iPosV2==iPosV3)	// if 1 is on the plane, and two & three are the same then use either 2 or 3
					{
						return iPosV2;
					}
					else
					{
						return SPLIT_BY_PLANE;	// if 1 is on the plane, and two & three are different then we need a split
					}
				}
			}
		}

		// we know than point 1 is not on the plane
		if (iPosV2==IN_PLANE)
		{

			if (iPosV3==IN_PLANE)	// if 2 & 3 are on the plane then return 1
			{
				return iPosV1;
			}
			else
			{
				if (iPosV1==iPosV3)	// if 2 is on the plane and 1&3 are not; are 1 & 3 the same
				{
					return iPosV1;
				}
				else
				{
					return SPLIT_BY_PLANE;
				}
			}
			
		}

		// we know now that 1 & 2 are not on the plane
		if (iPosV3==IN_PLANE)
		{
			if (iPosV1==iPosV2)	// if 3 is on the plane and 1 & 3 (which are not) are equal then return either
			{
				return iPosV1;
			}
			else
			{
				return SPLIT_BY_PLANE;
			}
		}

		// Now we know that ALL the points are not on the plane...
		// we also know that they are not all equal (i.e. on the same side)...
		//	... hence

		return SPLIT_BY_PLANE;

	}
}



int IsQuadOnPlane( PSTRIANGLE psTri, HDPLANE *psPlane )
{
	int			iPosV1, iPosV2, iPosV3, iPosV4;
	HDVAL	vDot1, vDot2, vDot3, vDot4;


	int SplitCount[3];

	/* validate input */
//	ASSERT( (PTRVALID(psTri,sizeof(TRIANGLE)),
//			"IsTriOnPlane: invalid triangle\n") );
	ASSERT( (PTRVALID(psPlane,sizeof(HDPLANE)),
			"IsTriOnPlane: invalid plane\n") );



//	dumppoly(psTri);

	iPosV1 = IsPointOnPlane( psPlane, GetIMDvec(psTri->pindex[0]), &vDot1 );
	iPosV2 = IsPointOnPlane( psPlane, GetIMDvec(psTri->pindex[1]), &vDot2 );
	iPosV3 = IsPointOnPlane( psPlane, GetIMDvec(psTri->pindex[2]), &vDot3 );
	iPosV4 = IsPointOnPlane( psPlane, GetIMDvec(psTri->pindex[3]), &vDot4 );


//	printf("IsQuadOnPlane - %d %d %d %d\n",iPosV1,iPosV2,iPosV3,iPosV4);

	if ( (iPosV1 == iPosV2) && (iPosV1 == iPosV3) && (iPosV1 == iPosV4)) return iPosV1;

// if any of the points are on the plane, then we need to look at the remaning points to see if they are
// behind or infront of the plane, indicating a split or all points on one side... there must be a neater way of handling this ... with bits perhaps

	SplitCount[IN_PLANE]=0;
	SplitCount[SAME_SIDE]=0;
	SplitCount[OPPOSITE_SIDE]=0;

	
	SplitCount[iPosV1]++;
	SplitCount[iPosV2]++;
	SplitCount[iPosV3]++;
	SplitCount[iPosV4]++;

	// if 3 or 4 vertices are in the plane, then the whole poly must be
	if (SplitCount[IN_PLANE] > 2) return IN_PLANE;

	// if no points are on the plane - we must have some on one side & some on the other
	
	if (SplitCount[IN_PLANE]==0 ) return SPLIT_BY_PLANE;
		
	if (SplitCount[IN_PLANE]==2)
	{
		if (SplitCount[SAME_SIDE]==2) return SAME_SIDE;
		if (SplitCount[OPPOSITE_SIDE]==2) return OPPOSITE_SIDE;
		return SPLIT_BY_PLANE;
	}


	// just 1 point on the plane
	if (SplitCount[SAME_SIDE]==3) return SAME_SIDE;

	if (SplitCount[OPPOSITE_SIDE]==3) return OPPOSITE_SIDE;

	return SPLIT_BY_PLANE;

}



/***************************************************************************/
/*
 * Calculates plane of triangle returned as ax + by + cz + d = 0;
 * (a,b,c) is then normalised normal to plane.
 */
/***************************************************************************/

void GetPlane( PSTRIANGLE psTri, HDPLANE *psPlane )
{
//	iVectorHD	v1, v2;

	iVectorHD Result;
	/* validate input */
//	ASSERT( (PTRVALID(psTri,sizeof(TRIANGLE)),
//			"GetPlane: invalid triangle\n") );
	ASSERT( (PTRVALID(psPlane,sizeof(HDPLANE)),
			"GetPlane: invalid plane\n") );


	GetTriangleNormal(psTri,&Result);

	/* normalise normal */				
	iNormalise( &Result );


	psPlane->a = Result.x;
	psPlane->b = Result.y;
	psPlane->c = Result.z;
	/* since plane eqn is ax + by + cz + d = 0,
	 * d = -(ax + by + cz).
	 */

	psPlane->d = - (	( psPlane->a* GetIMDvec(psTri->pindex[0])->x ) +
		( psPlane->b* GetIMDvec(psTri->pindex[0])->y ) +
		( psPlane->c* GetIMDvec(psTri->pindex[0])->z ) );


	/* store first triangle vertex as point on plane for later point /
	 * plane classification in IsPointOnPlane
	 */
	memcpy( &psPlane->vP, GetIMDvec(psTri->pindex[0]), sizeof(iVectorHD) );

	if (Verbose)
	{
		printf("plane created :\n");
		dumppoly(psTri);
		dumpplane(psPlane);
	}
//	printf("\n");

	// If we are getting the plane of a quad ... we need to check that it is co-planar
	//
	// This should not be needed ... but is a useful check for valid data
	if (psTri->npnts == 4)
	{
		BOOL iRet;
		HDVAL vDot;

		iRet = IsPointOnPlane( psPlane,  GetIMDvec(psTri->pindex[3]), &vDot );

		if (iRet != IN_PLANE)
		{
			UDWORD i;

			if (Verbose==TRUE || Warnings==TRUE)
			{
			
				printf("WARNING- NON-COPLANAR QUAD!:");

				for (i=0;i<4;i++)
				{
					HDVAL Dist;

					Dist=PlaneToPointDistance(psPlane, GetIMDvec(psTri->pindex[i]));

					if (Dist != 0.0f)
						printf("Dist to point %d = %f ",i,Dist);

				}


				dumppoly(psTri);
			}
			return;
		}

	}


}

/***************************************************************************/


// Debug functions

void dumppoly(iIMDPoly *psTri)
{
	if (psTri->npnts==3)
	{

		
		printf("TRI(%p): a(%f,%f,%f) b(%f,%f,%f) c(%f,%f,%f)\n",psTri,
			 GetIMDvec(psTri->pindex[0])->x,
			 GetIMDvec(psTri->pindex[0])->y,
			 GetIMDvec(psTri->pindex[0])->z,

			 GetIMDvec(psTri->pindex[1])->x,
			 GetIMDvec(psTri->pindex[1])->y,
			 GetIMDvec(psTri->pindex[1])->z,

			 GetIMDvec(psTri->pindex[2])->x,
			GetIMDvec(psTri->pindex[2])->y,
			GetIMDvec(psTri->pindex[2])->z );

			printf("  %p  a(%d) b(%d) c(%d)\n",&psTri->pindex[0],
				 psTri->pindex[0],
				 psTri->pindex[1],
				 psTri->pindex[2] );
	
	}

	else if (psTri->npnts==4)
	{
			printf("QUAD(%p): a(%f,%f,%f) b(%f,%f,%f) c(%f,%f,%f) d(%f,%f,%f)\n",psTri,
		 GetIMDvec(psTri->pindex[0])->x,
		 GetIMDvec(psTri->pindex[0])->y,
		 GetIMDvec(psTri->pindex[0])->z,

		 GetIMDvec(psTri->pindex[1])->x,
		 GetIMDvec(psTri->pindex[1])->y,
		 GetIMDvec(psTri->pindex[1])->z,

		 GetIMDvec(psTri->pindex[2])->x,
		GetIMDvec(psTri->pindex[2])->y,
		GetIMDvec(psTri->pindex[2])->z, 
		
		 GetIMDvec(psTri->pindex[3])->x,
		GetIMDvec(psTri->pindex[3])->y,
		GetIMDvec(psTri->pindex[3])->z 
		);

		printf("  %p  a(%d) b(%d) c(%d) d(%d)\n",&psTri->pindex[0],
			 psTri->pindex[0],
			 psTri->pindex[1],
			 psTri->pindex[2],
			 psTri->pindex[3]
			 );

	}
	else
	{
		printf("dumppoly: Bad number of points\n");
	}
}



void dumpplane(HDPLANE *plane)
{
#ifdef WIN32
	printf("PLANE: %fx + %fy + %fz + %f =0\n",
		plane->a,plane->b,plane->c,plane->d);
#else
	printf("PLANE: %dx + %dy + %dz = %d\n",
		plane->a,plane->b,plane->c,plane->d);
#endif

}


void dumpIMDvec(int VectorID)
{

	iVectorHD *Vec;


	Vec=GetIMDvec(VectorID);
	printf("IMDvec:%d x=%f y=%f z=%f\n",VectorID,Vec->x,Vec->y,Vec->z);
}

void dumpvector(iVectorHD *Vec)
{
    HDVAL vx, vy, vz, mod, sum_square;

	vx = Vec->x;
	vy = Vec->y;
	vz = Vec->z;


	sum_square = ((vx*vx)+(vy*vy)+(vz*vz) );
	mod = sqrt(sum_square);

#ifdef WIN32
	printf("vec: (%f,%f,%f) len=%f\n",Vec->x,Vec->y,Vec->z,mod);
#else
	printf("vec: (%x,%x,%x) len=%x\n",Vec->x,Vec->y,Vec->z,mod);
#endif
}




























