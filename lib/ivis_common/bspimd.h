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
#ifndef i_BSPIMD
#define i_BSPIMD

#ifdef BSPIMD

typedef UDWORD WORLDCOORD;
typedef SWORD ANGLE;

typedef struct
{
	WORLDCOORD x,y,z;
	ANGLE pitch,yaw,roll;
} OBJPOS;




typedef struct NODE
{
	struct NODE	*pPrev;
	struct NODE	*pNext;
	void		*pData;
}
NODE;

typedef NODE*	PSNODE;

typedef struct BSPPTRLIST
{
	int		iNumNodes;

	PSNODE	pHead;
	PSNODE	pTail;
	PSNODE	pCurPosition;
}
BSPPTRLIST, *PSBSPPTRLIST;





#define	TREE_OK		(0)
#define	TREE_FAIL	(-1)
#define	LEFT		1
#define	RIGHT		0

typedef struct BNODE
{
	struct BNODE *link[2];
}
BNODE, *PSBNODE;
/***************************************************************************/


#define TOLERANCE (10)

typedef struct PLANE
{
	// These 1st three entries can NOT NOW be cast into a iVectorf *   (iVectorf on PC are doubles)
	FRACT		a;	// these values form the plane equation ax+by+cz=d
	FRACT		b;
	FRACT		c;
	FRACT		d;
	Vector3i	vP;	// a point on the plane - in normal non-fract format
}
PLANE, *PSPLANE;

typedef struct BSPTREENODE
{
	struct BSPTREENODE *link[2];

	PLANE 			Plane;
	// points to first polygon in the BSP tree entry ... BSP_NextPoly in the iIMDPoly structure will point to the next entry
	BSPPOLYID		TriSameDir;	// id of the first polygon in the list ... or BSPPOLYID_TERMINATE for none
	BSPPOLYID		TriOppoDir;	// id of the first polygon in the list ... or BSPPOLYID_TERMINATE for none
#ifdef PIETOOL				// only needed when generating the tree
	HDPLANE		*psPlane;		// High def version of the plane equation
	PSBSPPTRLIST	psTriSameDir;
	PSBSPPTRLIST	psTriOppoDir;
#endif
}
BSPTREENODE, *PSBSPTREENODE;

/***************************************************************************/


#define		OPPOSITE_SIDE						0
#define		IN_PLANE							1
#define		SAME_SIDE							2



/***************************************************************************/

#endif

#endif

