/***************************************************************************/
#include <stdio.h>

#define BSPIMD
#define PIETOOL





#include "frame.h"	// for the fractional maths code
#include "ivi.h"	// for the typedefs
#include "imd.h"	// for the ivis vector structures & imd stuff
#include "bspimd.h"

#include "vertex.h"

//#include "drawimd_psx.h"	// for the scrvertex structure

#include "bspnode.h"
#include "bspmaths.h"
#include "bsptree.h"

extern BOOL Verbose;

// Prototypes from bsptree.c

int IsQuadOnPlane( PSTRIANGLE psTri, HDPLANE *psPlane );
int	SplitPolygonWithPlane( HDPLANE *psPlane, PSTRIANGLE psInputPoly,	PSBSPPTRLIST psSameSideTriList, PSBSPPTRLIST psOppoSideTriList );

/*


  This version superceeds (is that spelt wright?), any other version of the bsp code in other directories

   - tjc 3-sept-97

*/

/***************************************************************************/

#define	MAX_PLANES	99999

/***************************************************************************/

int DeleteNodeFunc( void *n1 )
{
	PSBSPTREENODE	psNode = (PSBSPTREENODE) n1;

	FREE( psNode->psPlane );

	list_Empty( psNode->psTriSameDir );
	FREE( psNode->psTriSameDir );

	list_Empty( psNode->psTriOppoDir );
	FREE( psNode->psTriOppoDir );
	
	FREE( psNode );

	return TREE_OK;
}

/***************************************************************************/

int CompareFunc( void *n1, void *n2 )
{
	PSBSPTREENODE	psNode1 = (PSBSPTREENODE) n1,
					psNode2 = (PSBSPTREENODE) n2;

	return 0;
}

/***************************************************************************/

PSBSPTREENODE InitNode()
{
	PSBNODE			psBNode;
	PSBSPTREENODE	psBSPNode;

	/* allocate structure */
	psBSPNode = MALLOC( sizeof(BSPTREENODE) );

	/* allocate node plane storage */
	psBSPNode->psPlane = (HDPLANE *) MALLOC( sizeof(HDPLANE) );

	/* create node triangle lists */
	list_Create( &(psBSPNode->psTriSameDir) );
	list_Create( &(psBSPNode->psTriOppoDir) );

	psBNode = (PSBNODE) psBSPNode;

	psBNode->link[LEFT] = NULL;
	psBNode->link[RIGHT] = NULL;

	return psBSPNode;
}

/***************************************************************************/

#define SMALLNUMBER (0.0001f)
BOOL IsZero(double V)
{
	if (V<SMALLNUMBER && V > (-SMALLNUMBER) ) return TRUE; 
	return FALSE;
}


void GetBestPartitionPlane( PSBSPPTRLIST psTriList, HDPLANE *psPlane )
{
	iIMDPoly *psTri, *psPlaneTri;
	int			iSplitCount, iPlanesTried, iMinSplits = 9999;
	HDPLANE	*psCurPlane;

	/* allocate storage for plane */
	psCurPlane = MALLOC( sizeof(HDPLANE) );

	/* init plane to first tri in list */
	psPlaneTri = (iIMDPoly *) list_GetFirst( psTriList );

	/* init tri count */
	iPlanesTried = 0;

	/* loop while not all planes tried */
	while ( psPlaneTri != NULL && iPlanesTried < MAX_PLANES )
	{
		/* get plane of triangle */

  		GetPlane( psPlaneTri, psCurPlane );
//		dumppoly(psPlaneTri);
//		dumpplane(psCurPlane);

		if (IsZero(psCurPlane->a)==TRUE && IsZero(psCurPlane->b)==TRUE && IsZero(psCurPlane->c)==TRUE)
		{
			printf("Zero plane equation !!!!!!! %f,%f,%f\n",psCurPlane->a,psCurPlane->b,psCurPlane->c);
			
			dumppoly(psPlaneTri);

			printf("Removing the polygon ...\n");

			list_Remove(psTriList,psPlaneTri);	// get rid of the polygon 
	
			iSplitCount=9999;
		
		}
		else
		{
			/* get first tri in list */
			psTri = (iIMDPoly *) list_GetFirst( psTriList );

			/* init split count for this plane */
			iSplitCount = 0;

			/* loop over list */
			while ( psTri != NULL )
			{
				int TriRes;

				if (psTri->npnts==3)
					TriRes = IsTriOnPlane( psTri, psCurPlane );
				else
					TriRes = IsQuadOnPlane( psTri, psCurPlane );

	/*
				if (TriRes==SPLIT_BY_PLANE)
					printf("TRI:SPLIT\n");
				if (TriRes==IN_PLANE)
					printf("TRI:IN_PLANE\n");
				if (TriRes==SAME_SIDE)
					printf("TRI:SAME_SIDE\n");
				if (TriRes==OPPOSITE_SIDE)
					printf("TRI:OPPOSITE_SIDE\n");
	*/


				/* test for split */
				if ( TriRes == SPLIT_BY_PLANE )
				{
					iSplitCount++;
				}

				/* next tri */
				psTri = list_GetNext( psTriList, psTri );

			}		
		}

//		printf("%d) Split count = %d\n",iPlanesTried,iSplitCount);

		/* update best plane if cause fewer splits */
		if ( iMinSplits > iSplitCount )
		{
			/* copy plane */
			memcpy( psPlane, psCurPlane, sizeof(HDPLANE) );

			/* update split count */
			iMinSplits = iSplitCount; 
		}
		
		/* next plane tri */
		psPlaneTri = list_GetNext( psTriList, psPlaneTri );

		/* update number of planes tried */
		iPlanesTried++;
	}

	/* get rid of plane storage */
	FREE( psCurPlane );
}

/***************************************************************************/
// Recursive routine to generate the BSP tree

void PartitionPolyListWithPlane( PSBSPTREENODE psNode, PSBSPPTRLIST psTriList )
{
	iIMDPoly	*psTri;
	int			iSide;
	PSBSPPTRLIST	psSameSideTriList;
	PSBSPPTRLIST	psOppoSideTriList;
	iVectorHD	vecN;
	HDVAL		vD;

	int	TriCount;

	ASSERT( (PTRVALID(psNode,sizeof(BSPTREENODE)),
			"PartitionTriListWithPlane: invalid node\n") );

	ASSERT( (PTRVALID(psTriList,sizeof(BSPPTRLIST)),
			"PartitionTriListWithPlane: invalid list\n") );
		  





if (Verbose==TRUE) printf("partition started - %d triangles in list %p\n",psTriList->iNumNodes,psTriList);

	/* get first tri in list */
	psTri = (iIMDPoly *) list_GetFirst( psTriList );

	/* return if list empty */
	if ( psTri == NULL )
	{
//		printf("list empty\n");
		return;
	}

/*
	if (psTriList->iNumNodes == 69)
	{
		PSPTRLIST	pList;
		int tri;
		iIMDPoly	*pstTri;

		printf("Nodes=69!\n");

		pList=psTriList;
		pstTri = (iIMDPoly *) list_GetFirst( pList );

		for(tri=0;tri<4;tri++)
		{
//			printf("%d) %p\n",tri,pstTri);
			dumppoly(pstTri);
			pstTri = (iIMDPoly *) list_GetNext( pList , pstTri);
		}
	}


	if (psTriList->iNumNodes == 95)
	{
		printf("Nodes=95!\n");
	}
*/



if (Verbose==TRUE) 	printf("Getting best partition\n");

	GetBestPartitionPlane( psTriList, psNode->psPlane );

if (Verbose==TRUE) 	printf("finished getting best partition\n");

	/* create same and opposite side lists */
	list_Create( &psSameSideTriList );
	list_Create( &psOppoSideTriList );

if (Verbose==TRUE) 
{
		printf("\n\nPartition Plane selected :\n");
		dumpplane(psNode->psPlane);
}

	TriCount=0;
	/* split input list by plane */
	while ( psTri != NULL )
	{
//		if (Verbose==TRUE)	dumppoly(psTri);
		if (Verbose==TRUE) 	printf("Tri %d) ",TriCount++);
//		dumppoly(psTri);
//		dumpplane(psNode->psPlane);
		/* assign triangle to same or opposite list or split */

		if (psTri->npnts==3)
			iSide = IsTriOnPlane( psTri, psNode->psPlane );
		else
			iSide = IsQuadOnPlane( psTri, psNode->psPlane );


		/*

				results are :-

				
			#define		OPPOSITE_SIDE						0
			#define		IN_PLANE							1
			#define		SAME_SIDE							2
			#define		SPLIT_BY_PLANE						3
			#define		INTERSECTION_INSIDE_LINE_SEGMENT	4
			#define		INTERSECTION_OUTSIDE_LINE_SEGMENT	5



		*/

		if (Verbose==TRUE)
		{
			switch (iSide)
			{
				case OPPOSITE_SIDE:
					printf("poly %p is on opposite side to plane. ",psTri);
					break;
				case SAME_SIDE:
					printf("poly %p is on same side to plane. ",psTri);
					break;
				case SPLIT_BY_PLANE:
					printf("poly %p is split by plane. ",psTri);
					break;
				case IN_PLANE:
					printf("poly %p in coplanar with plane. ",psTri);
					break;


				default:
					printf("UNXPECTED VALUE - %d\n",iSide);
			}
		}

		/* check whether to split triangle */
		if ( iSide == SPLIT_BY_PLANE )
		{
			/* split triangle; test may decide triangle in plane after all
			 * (numerical error)
			 */
			if (Verbose==TRUE)  printf("Splitting triangle\n");


			iSide = SplitPolygonWithPlane( psNode->psPlane, psTri,
											psSameSideTriList,
											psOppoSideTriList );
			if (Verbose==TRUE)  printf("Splitting triangle result=%d\n",iSide);
		}

		if ( iSide == OPPOSITE_SIDE )
		{
			/* add to opposite side list */
			if (Verbose==TRUE) 
			{
				printf("adding to opposite side list %p\n",psOppoSideTriList);
//				dumppoly(psTri);
			}
			list_Add( psOppoSideTriList, (void *) psTri );

/*
												{
														PSPTRLIST	pList;
													int tri;
													iIMDPoly	*pstTri;

													pList=psOppoSideTriList;
													pstTri = (iIMDPoly *) list_GetFirst( pList );

													for(tri=0;tri<4;tri++)
													{
														printf("%d) %p\n",tri,pstTri);
											//			dumppoly(pstTri);
														pstTri = (iIMDPoly *) list_GetNext( pList , pstTri);
													}

												}
*/
		}		
		if ( iSide == SAME_SIDE )
		{
			/* add to same side list */
			if (Verbose==TRUE) 	printf("adding to same side list %p\n",psSameSideTriList);
			list_Add( psSameSideTriList, (void *) psTri );
		}

		/* if triangle in plane, add to node in correct direction list */
		if ( iSide == IN_PLANE )
		{
			iVectorHD PlaneNormal;

			/* get triangle normal */

			GetTriangleNormal( psTri, &vecN );
		
			iNormalise( &vecN );	// The normal MUST be normalised - else the result of the dot product will not be correct

//			dumpvector(&vecN);
//			dumpvector((iVectorf *) psNode->psPlane);


			/* add to node list on correct side by testing normal products */

			PlaneNormal.x=psNode->psPlane->a;
			PlaneNormal.y=psNode->psPlane->b;
			PlaneNormal.z=psNode->psPlane->c;

//			printf("PlaneNorm %f,%f,%f\n",PlaneNormal.x,PlaneNormal.y,PlaneNormal.z);
//			printf("VecNorm %f,%f,%f\n",vecN.x,vecN.y,vecN.z);

			iDotProduct( &vD, &PlaneNormal, &vecN );
			
			
			if (Verbose==TRUE) 	printf("Co-planar dotproduct %f, ",vD);

			if ( vD > MAKEFRACT(0) )
			{
				// This list will contain the actual polygon that the plane was selected from, and hence will always contain at least 1 polygon
				if (Verbose==TRUE) 	printf("adding to co-planar list - same direction as plane\n");
//				dumppoly(psTri);
				list_Add( psNode->psTriSameDir, (void *) psTri );
			}
			else
			{
				if (Verbose==TRUE) printf("adding to co-planar list - opposite direction to plane\n");
//				dumppoly(psTri);
				list_Add( psNode->psTriOppoDir, (void *) psTri );
			}
		}

		/* remove from list */

		list_Remove( psTriList, (void *) psTri );

		/* reset to head of list */
		psTri = (iIMDPoly *) list_GetFirst( psTriList );
	}

	/* free up input list - now split into two */
	if (Verbose==TRUE) 	printf("freeing input list\n");
	FREE( psTriList );

	/* create left node and partition opposite side list with it */
	if ( (psTri = (iIMDPoly *) list_GetFirst( psOppoSideTriList )) != NULL )
	{
		if (Verbose==TRUE) 	printf("processing left list\n");
		/* create new node link left */
		psNode->link[LEFT] = InitNode();

		/* split opposite side list with plane */
		PartitionPolyListWithPlane( psNode->link[LEFT], psOppoSideTriList );
	}
	else
	{
		if (Verbose==TRUE) 		printf("empty left list\n");
		/* no need for list if not recursing */
		FREE( psOppoSideTriList );
	}
				  
	/* create right node and partition same side list with it */
	if ( (psTri = (iIMDPoly *) list_GetFirst( psSameSideTriList )) != NULL )
	{
		if (Verbose==TRUE) 	printf("processing right list\n");
		/* create new node link left */
		psNode->link[RIGHT] = InitNode();

		/* split opposite side list with plane */
		PartitionPolyListWithPlane( psNode->link[RIGHT], psSameSideTriList );
	}
	else
	{
		if (Verbose==TRUE) 	printf("empty right list\n");
		/* no need for list if not recursing */
		FREE( psSameSideTriList );
	}

	if (Verbose==TRUE) 	printf("partition complete\n");



}

/***************************************************************************/






/***************************************************************************/

