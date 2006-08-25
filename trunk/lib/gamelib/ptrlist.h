/***************************************************************************/

#ifndef _PTRLIST_H_
#define _PTRLIST_H_

/***************************************************************************/
/* defines
 */

/***************************************************************************/
/* macros
 */

/***************************************************************************/
/* structs
 */

typedef struct LISTNODE
{
	void				*psElement;
	SDWORD				sdwKey;
	struct LISTNODE		*psNext;
}
LISTNODE;

typedef struct sPTRLIST
{
	OBJ_HEAP		*psNodeHeap;
	OBJ_HEAP		*psElementHeap;
	LISTNODE		*psNode;
	LISTNODE		*psCurNode;
	UDWORD			udwTableSize;
	UDWORD			udwElements;
	UDWORD			udwExtElements;
	UDWORD			udwElementSize;
	UDWORD			sdwCurIndex;
	BOOL			bDontGetNext;
}
PTRLIST;

/***************************************************************************/
/* functions
 */

BOOL	ptrList_Create( PTRLIST **pptrList, UDWORD udwInitElements,
						UDWORD udwExtElements, UDWORD udwElementSize );
void	ptrList_Destroy( PTRLIST *ptrList );
void	ptrList_Clear( PTRLIST *ptrList );

void *	ptrList_GetElement( PTRLIST *ptrList );
void	ptrList_FreeElement( PTRLIST *ptrList, void *psElement );
void	ptrList_InsertElement( PTRLIST *ptrList, void *psElement,
								SDWORD sdwKey );
BOOL	ptrList_RemoveElement( PTRLIST *ptrList, void *psElement,
								SDWORD sdwKey );

void *	ptrList_GetFirst( PTRLIST *ptrList );
void *	ptrList_GetNext(  PTRLIST *ptrList );

/***************************************************************************/

#endif	// _PTRLIST_H_

/***************************************************************************/
