
#define BSPIMD

#include <assert.h>

#include <stdio.h>

#include "frame.h"
#include "ivi.h"	// for the typedefs
#include "imd.h"
#include "bspimd.h"

#include "bsptree.h"

#include "vertex.h"

#include "ivisheap.h"

iVectorHD *HDvertices;

static int CurrentVertexListCount=0;


extern BOOL Verbose;

iVectorHD *GetIMDvec(int Vertex)
{
	assert((Vertex>=0)&&(Vertex<CurrentVertexListCount));
	return (HDvertices+Vertex);
}


	// Create a high-precesion vertex list
void	CreateHiPreVectices(iIMDShape *imd)
{
	int Vertex;

	HDvertices=MALLOC(imd->npoints * sizeof(iVectorHD));

	CurrentVertexListCount=imd->npoints;

	assert(HDvertices);

	for (Vertex=0;Vertex<imd->npoints;Vertex++)
	{
		  HDvertices[Vertex].x=(HDVAL) imd->points[Vertex].x;
		  HDvertices[Vertex].y=(HDVAL) imd->points[Vertex].y;
		  HDvertices[Vertex].z=(HDVAL) imd->points[Vertex].z;

		if (Verbose==TRUE)
		{
			printf("Vertex %d) (%d,%d,%d,)\n",Vertex,
				imd->points[Vertex].x,imd->points[Vertex].y,imd->points[Vertex].z);
		}
	
	}


	iV_HeapFree(imd->points,imd->npoints * sizeof(iVector));

	//FREE(imd->points);
	imd->points=NULL;
	imd->npoints=0;
}


void RemapPolygonsAndRecurse(PSBSPTREENODE psNode, int *VertexTable);

// Create a lo-precesion vertex list - remove duplicate points ...
void	CreateLoPreVectices(iIMDShape *imd)	
{
	int VertNum,Vertex;

	BOOL VertexRemoved;
	int32 Vx,Vy,Vz;


	int *VertexTable;
	
	assert(HDvertices);

	VertexTable=MALLOC(sizeof(int)*CurrentVertexListCount);	// which vertex match which	   - used in polygon remapping

	//imd->points=MALLOC(sizeof(iVector)*CurrentVertexListCount);	// new list of vertices

	imd->points=(iVector *)iV_HeapAlloc(sizeof(iVector)*CurrentVertexListCount);	// new list of vertices


	for (Vertex=0;Vertex<CurrentVertexListCount;Vertex++)
	{

		Vx = (int32) HDvertices[Vertex].x;
		Vy = (int32) HDvertices[Vertex].y;
		Vz = (int32) HDvertices[Vertex].z;

		VertexRemoved=FALSE;
		
		// Check 


		for (VertNum=0;VertNum<imd->npoints;VertNum++)
		{
			if ((imd->points[VertNum].x == Vx)&&
				(imd->points[VertNum].y == Vy)&&
				(imd->points[VertNum].z == Vz))
			{
				VertexTable[Vertex]=VertNum;

				VertexRemoved=TRUE;
				break;
			}

		}

		if (VertexRemoved==FALSE)
		{

			VertexTable[Vertex]=imd->npoints;

			imd->points[imd->npoints].x = Vx;
			imd->points[imd->npoints].y = Vy;
			imd->points[imd->npoints].z = Vz;

			imd->npoints++;

		}

	}

	// Now remap the polygon vertices
	RemapPolygonsAndRecurse(imd->BSPNode,VertexTable);


	FREE(HDvertices);
}



void RemapPolygonList(PSBSPPTRLIST TriList, int *VertexTable)
{
	iIMDPoly *CurPoly;
	int Vertex;

	if (TriList==NULL) return;
	
	CurPoly=list_GetFirst(TriList);	

	while (CurPoly!=NULL)
	{
		for (Vertex=0;Vertex<CurPoly->npnts;Vertex++)
		{
			int VertID;

			VertID=CurPoly->pindex[Vertex];
			CurPoly->pindex[Vertex]=VertexTable[VertID];
		}
		CurPoly=list_GetNext(TriList,CurPoly);
	}
}

void RemapPolygonsAndRecurse(PSBSPTREENODE psNode, int *VertexTable)
{
	if ( psNode == NULL )
	{
		return;
	}

	RemapPolygonsAndRecurse( psNode->link[LEFT],VertexTable);
	RemapPolygonList( psNode->psTriSameDir ,VertexTable);
	RemapPolygonList( psNode->psTriOppoDir ,VertexTable);
	RemapPolygonsAndRecurse( psNode->link[RIGHT],VertexTable);
}







/*
	Adds a new point into the imd
	- returns an id indicating which point has been added
	- scans through the list on vertices to avoid duplicates
	- returns -1 if memory allocation is a problem

	This has to copy the list across, and so all points to the point list will be invalid!!!!!

*/
int AddNewIMDVertex(iVectorHD *vNewPoint)
{
	int Point;
	iVectorHD *Vec;
	iVectorHD *NewVecs;	   
	int PointCount;

	PointCount=CurrentVertexListCount;

	// Scan for the points
	Vec=HDvertices;
	for (Point=0;Point<PointCount;Point++)
	{
		if ((Vec->x==vNewPoint->x) &&(Vec->y==vNewPoint->y) &&(Vec->z==vNewPoint->z) )
		{
			return Point;
		}
		Vec++;
	}

	NewVecs=MALLOC(sizeof(iVectorHD) * (PointCount+1));

	if (NewVecs==NULL)
	{
		printf("No mem for vector!\n");
		return -1;
	}
	
	memcpy(NewVecs,HDvertices,sizeof(iVectorHD)*PointCount);
	

	NewVecs[PointCount].x=vNewPoint->x;
	NewVecs[PointCount].y=vNewPoint->y;
	NewVecs[PointCount].z=vNewPoint->z;

	FREE(HDvertices);	// free up the old point area - this is going to fragment mem!!!!

	HDvertices = NewVecs;

	CurrentVertexListCount++;


	return (PointCount);
}
