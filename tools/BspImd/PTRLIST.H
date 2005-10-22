/***************************************************************************/

#ifndef _PTRLIST_H
#define _PTRLIST_H

/***************************************************************************/

extern void		list_Create( PSBSPPTRLIST *ppList );
extern void		list_Empty( PSBSPPTRLIST pList );
extern PSNODE	list_Find( PSBSPPTRLIST pList, void * pData );
extern void *	list_GetFirst( PSBSPPTRLIST pList );
extern void *	list_GetLast( PSBSPPTRLIST pList );
extern void *	list_GetNext( PSBSPPTRLIST pList, void *pData );
extern void *	list_GetPrev( PSBSPPTRLIST pList, void *pData );
extern void		list_Add( PSBSPPTRLIST pList, void *pData );
extern void		list_InsertAtCurrentPos( PSBSPPTRLIST pList, void *pData);
extern void		list_InsertInFrontOfNode( PSBSPPTRLIST pList, void *pData, void *pBefore );
extern void		list_Append( PSBSPPTRLIST pList, void *pData );
extern void		list_Remove( PSBSPPTRLIST pList, void *pData );
extern void *	list_RemoveFirst( PSBSPPTRLIST pList );
extern void *	list_RemoveLast( PSBSPPTRLIST pList );
extern int		list_NumNodes( PSBSPPTRLIST pList );
void list_Free( PSBSPPTRLIST pList );


/***************************************************************************/

#endif

/***************************************************************************/
