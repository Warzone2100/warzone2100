/***************************************************************************/

#ifndef _BSPTREE_H_
#define _BSPTREE_H_

/***************************************************************************/

//#include "ptrlist.h"
#include "bspnode.h"

/***************************************************************************/


#define DBGPRINT   (0)

											 // isupper
// 0 - none
// 1 - some
// 2 - lots
// 3 - all


//typedef int	(*DISPLAYFUNC) ( PSPTRLIST psTriList );

/***************************************************************************/

extern void				GetBestPartitionPlane( PSBSPPTRLIST psTriList,
											   HDPLANE *psPlane );
extern void				PartitionTriListWithPlane( PSBSPTREENODE psNode,
												   PSBSPPTRLIST psTriList );
extern PSBSPTREENODE	InitNode();
//extern void				TraverseTreeAndRender( PSBSPTREENODE psNode,
//								D3DVECTOR * pPos, DISPLAYFUNC df );

extern int DeleteNodeFunc( void *n1 );
extern int CompareFunc( void *n1, void *n2 );
extern int	ShowFunc( void *m, int level );
//extern int	DisplayFunc( PSPTRLIST psTriList );

/***************************************************************************/

#endif		/* _BSPTREE_H_ */

/***************************************************************************/
