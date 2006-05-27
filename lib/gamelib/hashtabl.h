/***************************************************************************/

#ifndef _HASHTABL_H_
#define _HASHTABL_H_

/***************************************************************************/

#include "lib/framework/frame.h"

/***************************************************************************/
/* defines
 */

/* flags key not used in hash function */
#define	UNUSED_KEY	-747

/***************************************************************************/
/* macros
 */

typedef UDWORD	(* HASHFUNC)		( int iKey1, int iKey2 );
typedef void	(* HASHFREEFUNC)	( void *psElement );

/***************************************************************************/
/* structs
 */

typedef struct HASHNODE
{
	int					iKey1;
	int					iKey2;
	void				*psElement;
	struct HASHNODE		*psNext;
}
HASHNODE;

typedef struct HASHTABLE
{
	OBJ_HEAP		*psNodeHeap;
	OBJ_HEAP		*psElementHeap;
	HASHNODE		**ppsNode;
	HASHNODE		*psNextNode;
	HASHFUNC		pHashFunc;
	HASHFREEFUNC	pFreeFunc;
	UDWORD			udwTableSize;
	UDWORD			udwElements;
	UDWORD			udwExtElements;
	UDWORD			udwElementSize;
	UDWORD			sdwCurIndex;
}
HASHTABLE;

/***************************************************************************/
/* functions
 */

BOOL	hashTable_Create( HASHTABLE **ppsTable, UDWORD udwTableSize,
							UDWORD udwInitElements, UDWORD udwExtElements,
							UDWORD udwElementSize );
void	hashTable_Destroy( HASHTABLE *psTable );
void	hashTable_Clear( HASHTABLE *psTable );

void *	hashTable_GetElement( HASHTABLE *psTable );
void	hashTable_InsertElement( HASHTABLE *psTable, void *psElement,
										int iKey1, int iKey2 );
BOOL	hashTable_RemoveElement( HASHTABLE *psTable, void *psElement,
										int iKey1, int iKey2 );
void *	hashTable_FindElement( HASHTABLE *psTable,
										int iKey1, int iKey2 );

void *	hashTable_GetFirst( HASHTABLE *psTable );
void *	hashTable_GetNext(  HASHTABLE *psTable );

void	hashTable_SetHashFunction( HASHTABLE *psTable, HASHFUNC pHashFunc );
void	hashTable_SetFreeElementFunction( HASHTABLE *psTable,
											HASHFREEFUNC pFreeFunc );

/***************************************************************************/

#endif	// _HASHTABL_H_

/***************************************************************************/
