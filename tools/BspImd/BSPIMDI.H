#error This file is not used anymore
#ifndef i_BSPIMDi
#define i_BSPIMDi



#include "vertex.h"


/*
typedef struct PTRLIST
{
	int		iNumNodes;

	PSNODE	pHead;
	PSNODE	pTail;
	PSNODE	pCurPosition;
}
PTRLIST, *PSPTRLIST;
*/




typedef struct BNODE
{
	BINTREE_STUFF(BNODE);
}
BNODE, *PSBNODE;

typedef int	(*COMPFUNC) ( void *node1, void *node2 );
typedef int	(*DOFUNC)   ( void *node,  int level   );
typedef int	(*DELETEFUNC) ( void *node );

typedef struct BINTREE
{
	PSBNODE		psBNodeDummyHead;
	COMPFUNC	Compare;
	int			DuplicatesOK;
	int			NodeSize;
}
BINTREE, *PSBINTREE;
/***************************************************************************/



typedef struct HDPLANE
{
	// These 1st three entries can NOT NOW be cast into a iVectorf *   (iVectorf on PC are doubles)
	HDVAL		a;	// these values form the plane equation ax+by+cz=d
	HDVAL		b;
	HDVAL		c;
	HDVAL		d;
	iVectorHD	vP;	// a point on the plane - in normal non-fract format
}
HDPLANE;


typedef struct BSPTREENODE
{
	BINTREE_STUFF( BSPTREENODE );

	HDPLANE		*psPlane;
	PSPTRLIST	psTriSameDir;
	PSPTRLIST	psTriOppoDir;
}
BSPTREENODE, *PSBSPTREENODE;




#endif

