#if(1)
/***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <string.h>


#include "frame.h"
#include "imd.h"
#include "bspimd.h"


#include "bintree.h"
#include "mem.h"
#include "debug.h"



/***************************************************************************/

PSBNODE
BinTreeInitNode( size_t size )
{
	PSBNODE		psN;

	psN = MALLOC( size );

	psN->link[LEFT] = psN->link[RIGHT] = NULL;

//	RBONLY( psN->red = 0;);

	return psN;
}

/***************************************************************************/

PSBINTREE
BinTreeCreate( PSBNODE psBNodeDummy, COMPFUNC cf,
				int dup_ok, size_t node_size )
{
	PSBINTREE	psT;
	
	psT = (PSBINTREE) MALLOC(sizeof(BINTREE));

	psT->psBNodeDummyHead = psBNodeDummy;
	psT->Compare = cf;
	psT->DuplicatesOK = dup_ok;
	psT->NodeSize = node_size;

	return psT;
}

/***************************************************************************/

int
DelNode( PSBNODE psN, DELETEFUNC df )
{
	if ( psN != NULL )
	{
		/* recursively delete left */
		if ( psN->link[LEFT] != NULL )
		{
			DelNode( psN->link[LEFT], df );
		}

		/* recursively delete right */
		if ( psN->link[RIGHT] != NULL )
		{
			DelNode( psN->link[RIGHT], df );
		}

		/* delete node */
		(df) ( psN );
	}

	return TREE_OK;
}

/***************************************************************************/

void
BinTreeDestroy( PSBINTREE psT, DELETEFUNC df )
{
	int		iOK;

	ASSERT( (psT->psBNodeDummyHead->link[RIGHT] != NULL,
			"BinTreeDestroy: Empty Tree\n") );

	iOK = DelNode( psT->psBNodeDummyHead, df );
	ASSERT( ( iOK == TREE_OK, "BinTreeDestroy: tree not walked ok\n") );

	FREE( psT );
}

/***************************************************************************/

PSBNODE
BinTreeFind( PSBINTREE psT, PSBNODE psN )
{
	PSBNODE		psNCur;
	int			iDir;

	psNCur = psT->psBNodeDummyHead->link[RIGHT];

	while ( psNCur != NULL )
	{
		iDir = (psT->Compare) ( psN, psNCur );

		if ( iDir == 0  && psNCur->link[RIGHT] == NULL )
//		if ( iDir == 0 RBONLY( && psNCur->link[RIGHT] == NULL) )
		{
			return psNCur;
		}
		
		/* next dir */
		iDir = iDir < 0;

		/* next node */
		psNCur = psNCur->link[iDir];
	}

	return NULL;
}

/***************************************************************************/

int
BinTreeInsertNode( PSBINTREE psT, PSBNODE psN )
{
	int			p_dir;
	PSBNODE		p, s;

	/* search until empty arm found  */
	p = psT->psBNodeDummyHead;
	p_dir = RIGHT;
	s = p->link[RIGHT];

	while ( s != NULL )
	{
		p = s;
		p_dir = (psT->Compare) (psN,s);

		if ( p_dir == 0 && psT->DuplicatesOK == 0 )
		{
			return TREE_FAIL;
		}

		p_dir = p_dir < 0;
		s = s->link[p_dir];
	}

	/* add new node */
	p->link[p_dir] = psN;

	return TREE_OK;
}

/***************************************************************************/

PSBNODE
BinTreeDeleteNode( PSBINTREE psT, PSBNODE psN )
{
	PSBNODE		p, s, save;
	int			iDir, iDirOld;

	p = psT->psBNodeDummyHead;
	s = p->link[RIGHT];
	iDir = iDirOld = RIGHT;

	/* look for match */
	while ( s != NULL && (iDir = (psT->Compare) (psN, s)) != 0 )
	{
		p = s;
		iDir = iDir < 0;
		iDirOld = iDir;
		s = p->link[iDir];
	}

	if ( s == NULL )
	{
		return NULL;
	}

	save = s;

	/* First case; if s has no right child,
	 * then replace s with s's left child
	 */
	if ( s->link[RIGHT] == NULL )
	{
		s = s->link[LEFT];
	}
	/* Second case; if s has a right child that lacks a left child,
	 * then replace s with s's right child and copy s's left child into
	 * the right child's left child
	 */
	else if ( s->link[RIGHT]->link[LEFT] == NULL )
	{
		s = s->link[RIGHT];
		s->link[LEFT] = save->link[LEFT];
	}
	/* Final case; find leftmost (smallest) node in s's right subtree.
	 * By def, this node has empty left link, Free it by copying its right
	 * link to its parent's left link and then give it both of s's links
	 * (thus replacing s).
	 */
	else
	{
		// small changed to smalls - cause the compiler complained (!)
		PSBNODE		smalls;

		smalls = s->link[RIGHT];

		while ( smalls->link[LEFT]->link[LEFT] )
		{
			smalls = smalls->link[LEFT];
		}

		s = smalls->link[LEFT];
		smalls->link[LEFT] = smalls->link[RIGHT];
		s->link[LEFT] = save->link[LEFT];
		s->link[RIGHT] = save->link[RIGHT];
	}

	p->link[iDirOld] = s;

	return save;
}

/***************************************************************************/

void
PreOrderWalk( PSBNODE psN, int level, DOFUNC df )
{
	if ( psN != NULL )
	{
		df( psN, level );
		PreOrderWalk( psN->link[LEFT], level+1, df );
		PreOrderWalk( psN->link[RIGHT], level+1, df );
	}
}

/***************************************************************************/

void
InOrderWalk( PSBNODE psN, int level, DOFUNC df )
{
	if ( psN != NULL )
	{
		InOrderWalk( psN->link[LEFT], level+1, df );
		df( psN, level );
		InOrderWalk( psN->link[RIGHT], level+1, df );
	}
}

/***************************************************************************/

void
PostOrderWalk( PSBNODE psN, int level, DOFUNC df )
{
	if ( psN != NULL )
	{
		PostOrderWalk( psN->link[LEFT], level+1, df );
		PostOrderWalk( psN->link[RIGHT], level+1, df );
		df( psN, level );
	}
}

/***************************************************************************/

int
BinTreeWalk( PSBINTREE psT, DOFUNC df, enum BINTREEORDER bTreeOrder )
{
	ASSERT( (psT->psBNodeDummyHead->link[RIGHT] != NULL, "Empty Tree\n") );

	switch ( bTreeOrder )
	{
		case PREORDER:
			PreOrderWalk( psT->psBNodeDummyHead->link[RIGHT], 0 , df );
			break;

		case INORDER:
			InOrderWalk( psT->psBNodeDummyHead->link[RIGHT], 0 , df );
			break;

		case POSTORDER:
			PostOrderWalk( psT->psBNodeDummyHead->link[RIGHT], 0 , df );
			break;
	}

	return TREE_OK;
}

/***************************************************************************/
#endif