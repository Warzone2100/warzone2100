#ifndef i_BSPIMD
#define i_BSPIMD

#ifdef PIETOOL				// only needed when generating the tree
typedef double HDVAL;
typedef struct {HDVAL x, y, z;} iVectorHD;
#endif


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
	iVector	vP;	// a point on the plane - in normal non-fract format
}
PLANE, *PSPLANE;



#ifdef PIETOOL
enum BINTREEORDER	{ PREORDER, INORDER, POSTORDER };


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



#endif

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
#define		SPLIT_BY_PLANE						3
#define		INTERSECTION_INSIDE_LINE_SEGMENT	4
#define		INTERSECTION_OUTSIDE_LINE_SEGMENT	5


#define SPLITTING_ERROR (-1)


/***************************************************************************/
#endif

