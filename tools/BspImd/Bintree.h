/***************************************************************************/

#ifndef _BINTREE_H_
#define _BINTREE_H_

/***************************************************************************/
/***************************************************************************/

PSBINTREE	BinTreeCreate( PSBNODE psBNodeDummy, COMPFUNC cf,
							int dup_ok, size_t node_size );
void		BinTreeDestroy( PSBINTREE psT, DELETEFUNC df );

int			BinTreeWalk( PSBINTREE psT, DOFUNC df,
							enum BINTREEORDER bTreeOrder );
int			BinTreeInsertNode( PSBINTREE psT, PSBNODE psN );
PSBNODE		BinTreeDeleteNode( PSBINTREE psT, PSBNODE psN );
PSBNODE		BinTreeFind( PSBINTREE psT, PSBNODE psN );

PSBNODE		BinTreeInitNode( size_t size );

/***************************************************************************/

#endif		/* _BINTREE_H_ */

/***************************************************************************/
