/***************************************************************************/

#include <limits.h>
#include <memory.h>

#include "lib/framework/frame.h"

#include "hashtabl.h"

extern void *	g_ElementToBeRemoved;

/***************************************************************************/

#define	HASHTEST		1

/* next four used in HashPJW */
#define	BITS_IN_int		32
#define	THREE_QUARTERS	((UINT) ((BITS_IN_int * 3) / 4))
#define	ONE_EIGHTH		((UINT) (BITS_IN_int / 8))

//#define	HIGH_BITS		( ~((UINT)(~0) >> ONE_EIGHTH ))       /* DOES NOT WORK ON THE PSX ! use the version below */
#define	HIGH_BITS		((UINT)( ~((0xffffffff) >> ONE_EIGHTH )))

/***************************************************************************/

UINT
HashTest( int iKey1, int iKey2 )
{
	return (UINT) iKey1 + iKey2;
}

/***************************************************************************/
/*
 * HashPJW
 *
 * Adaptation of Peter Weinberger's (PJW) generic hashing algorithm listed
 * in Binstock+Rex, "Practical Algorithms" p 69.
 *
 * Accepts element pointer and returns hashed integer.
 */
/***************************************************************************/

UINT
HashPJW( int iKey1, int iKey2 )
{
	UINT	iHashValue, i;
	CHAR	*c = (CHAR *) iKey1;

	/* don't use second key in this one */
	iKey2 = UNUSED_KEY;

	for ( iHashValue=0; *c; ++c )
	{
		iHashValue = ( iHashValue << ONE_EIGHTH ) + *c;

		if ( (i = iHashValue & HIGH_BITS) != 0 )
		{
			iHashValue = ( iHashValue ^ ( i >> THREE_QUARTERS ) ) &
							~HIGH_BITS;
		}
	}

	return iHashValue;
}

/***************************************************************************/

BOOL
hashTable_Create( HASHTABLE **ppsTable, UDWORD udwTableSize,
	UDWORD udwInitElements, UDWORD udwExtElements, UDWORD udwElementSize )
{
	UDWORD			udwSize;

	/* allocate and init table */


	(*ppsTable) = MALLOC( sizeof(HASHTABLE) );
	udwSize = udwTableSize * sizeof(HASHNODE *);
	(*ppsTable)->ppsNode = MALLOC( udwSize );
	memset( (*ppsTable)->ppsNode, 0, udwSize );

	/* allocate heaps */
	if ( !HEAP_CREATE( &(*ppsTable)->psNodeHeap, sizeof(HASHNODE),
						udwInitElements, udwExtElements) )
	{
		return FALSE;
	}

	if ( !HEAP_CREATE( &(*ppsTable)->psElementHeap, udwElementSize,
						udwInitElements, udwExtElements) )
	{
//		my_error("",0,"","htc FAIL!\n");
		return FALSE;
	}

	/* init members */
	(*ppsTable)->udwTableSize   = udwTableSize;
	(*ppsTable)->udwElements    = udwInitElements;
	(*ppsTable)->udwExtElements = udwExtElements;
	(*ppsTable)->udwElementSize = udwElementSize;
	(*ppsTable)->sdwCurIndex    = 0;
	(*ppsTable)->psNextNode     = (*ppsTable)->ppsNode[0];
	(*ppsTable)->pFreeFunc      = NULL;

	/* set hash function to internal */
#if HASHTEST
	hashTable_SetHashFunction( (*ppsTable), HashTest );
#else
	hashTable_SetHashFunction( (*ppsTable), HashPJW );
#endif

	return TRUE;
}

/***************************************************************************/

void
hashTable_Destroy( HASHTABLE *psTable )
{
	ASSERT( (PTRVALID(psTable, sizeof(HASHTABLE)),
			"hashTable_Destroy: table pointer invalid\n") );

	hashTable_Clear( psTable );

	/* destroy heaps */
	HEAP_DESTROY( psTable->psNodeHeap );
	HEAP_DESTROY( psTable->psElementHeap );

	/* free table */
	FREE( psTable->ppsNode );
	FREE( psTable );
}

/***************************************************************************/
/*
 * hashTable_Clear
 *
 * Returns all nodes from hash table to free node list
 */
/***************************************************************************/

void
hashTable_Clear( HASHTABLE *psTable )
{
	HASHNODE	*psNode, *psNodeTmp;
	UDWORD		i;

	ASSERT( (PTRVALID(psTable, sizeof(HASHTABLE)),
			"hashTable_Destroy: table pointer invalid\n") );

	/* free nodes */
	for ( i=0; i<psTable->udwTableSize; i++ )
	{
		/* free table entry nodelist */
		psNode = psTable->ppsNode[i];
		while ( psNode != NULL )
		{
			/* return node element to heap */
			ASSERT( (PTRVALID(psNode->psElement, psTable->udwElementSize),
				"hashTable_Destroy: element pointer invalid\n") );

			/* do free-element callback if set */
			if ( psTable->pFreeFunc != NULL )
			{
				(psTable->pFreeFunc) (psNode->psElement);
			}

			/* free element */
			HEAP_FREE( psTable->psElementHeap, psNode->psElement );

			/* return node to heap */
			ASSERT( (PTRVALID(psNode, sizeof(HASHNODE)),
				"hashTable_Destroy: node pointer invalid\n") );
			psNodeTmp = psNode->psNext;
			HEAP_FREE( psTable->psNodeHeap, psNode );
			psNode = psNodeTmp;
		}

		/* set table entry to NULL */
		psTable->ppsNode[i] = NULL;
	}
}

/***************************************************************************/

void
hashTable_SetHashFunction( HASHTABLE *psTable, HASHFUNC pHashFunc )
{
	ASSERT( (PTRVALID(psTable, sizeof(HASHTABLE)),
			"hashTable_SetHashFunction: table pointer invalid\n") );

	psTable->pHashFunc = pHashFunc;
}

/***************************************************************************/

void
hashTable_SetFreeElementFunction( HASHTABLE *psTable,
									HASHFREEFUNC pFreeFunc )
{
	ASSERT( (PTRVALID(psTable, sizeof(HASHTABLE)),
			"hashTable_SetFreeElementFunction: table pointer invalid\n") );

	psTable->pFreeFunc = pFreeFunc;
}

/***************************************************************************/
/*
 * hashTable_GetElement
 *
 * Gets free node from heap and returns element pointer
 */
/***************************************************************************/

void *
hashTable_GetElement( HASHTABLE *psTable )
{
	void	*psElement;
	BOOL result;

	ASSERT( (PTRVALID(psTable, sizeof(HASHTABLE)),
			"hashTable_GetElement: table pointer invalid\n") );

	result=HEAP_ALLOC( psTable->psElementHeap, &psElement );

	// if the alloc fails then return NULL
	if (result==FALSE) return NULL;


	return psElement;
}

/***************************************************************************/

UDWORD
hashTable_GetHashKey( HASHTABLE *psTable, int iKey1, int iKey2 )
{
	ASSERT( (PTRVALID(psTable, sizeof(HASHTABLE)),
		"hashTable_GetFirst: hash table pointer invalid\n") );

	/* get hashed index */
	return (psTable->pHashFunc) ( iKey1, iKey2 ) % psTable->udwTableSize;
}

/***************************************************************************/

void
hashTable_InsertElement( HASHTABLE *psTable, void *psElement,
							int iKey1, int iKey2 )
{
	UDWORD		udwHashIndex;
	HASHNODE	*psNode;

	ASSERT( (PTRVALID(psTable, sizeof(HASHTABLE)),
			"hashTable_InsertElement: table pointer invalid\n") );
	ASSERT( (PTRVALID(psElement, psTable->udwElementSize),
			"hashTable_InsertElement: element pointer invalid\n") );

	/* get hashed index */
	udwHashIndex = hashTable_GetHashKey( psTable, iKey1, iKey2 );

	/* get node from heap */
	HEAP_ALLOC( psTable->psNodeHeap, (void*) &psNode );

	/* set node elements */
	psNode->iKey1     = iKey1;
	psNode->iKey2     = iKey2;
	psNode->psElement = psElement;

	/* add new node to head of list */
	psNode->psNext = psTable->ppsNode[udwHashIndex];
	psTable->ppsNode[udwHashIndex] = psNode;
}

/***************************************************************************/
/*
 * hashTable_FindElement
 *
 * Calculates hash index from keys and returns element in hash table
 */
/***************************************************************************/

void *
hashTable_FindElement( HASHTABLE *psTable, int iKey1, int iKey2 )
{
	UDWORD		udwHashIndex;
	HASHNODE	*psNode;

	ASSERT( (PTRVALID(psTable, sizeof(HASHTABLE)),
			"hashTable_FindElement: table pointer invalid\n") );

	/* get hashed index */
	udwHashIndex = hashTable_GetHashKey( psTable, iKey1, iKey2 );

	/* check hash index within bounds */
	ASSERT( (udwHashIndex>=0 && udwHashIndex<psTable->udwTableSize,
		"hashTable_GetElement: hash value %i too large for table size %i\n",
		udwHashIndex, psTable->udwTableSize) );

	psNode = psTable->ppsNode[udwHashIndex];

	/* loop through node list to find element match */
	while (    psNode != NULL &&
			!( psNode->iKey1 == iKey1 && psNode->iKey2 == iKey2) )
	{
		psNode = psNode->psNext;
	}

	/* remove node from hash table and return to heap */
	if ( psNode == NULL )
	{
		return FALSE;
	}
	else
	{
		return psNode->psElement;
	}
}

/***************************************************************************/

static void
hashTable_SetNextNode( HASHTABLE *psTable, BOOL bMoveToNextNode )
{
	if ( (bMoveToNextNode == TRUE) && (psTable->psNextNode != NULL) )
	{
		/* get next node */
		psTable->psNextNode = psTable->psNextNode->psNext;

		/* if next node NULL increment index */
		if ( psTable->psNextNode == NULL )
		{
			psTable->sdwCurIndex++;
		}
	}

	/* search through table for next allocated node */
	while ( psTable->sdwCurIndex < psTable->udwTableSize &&
			psTable->psNextNode == NULL )
	{
		psTable->psNextNode = psTable->ppsNode[psTable->sdwCurIndex];
		if ( psTable->psNextNode == NULL )
		{
			psTable->sdwCurIndex++;
		}
	}

	/* reset pointer if table overrun */
	if ( psTable->sdwCurIndex >= psTable->udwTableSize )
	{
		psTable->psNextNode = NULL;
	}
}

/***************************************************************************/

BOOL
hashTable_RemoveElement( HASHTABLE *psTable, void *psElement,
							int iKey1, int iKey2 )
{
	UDWORD		udwHashIndex;
	HASHNODE	*psNode, *psPrev;

	ASSERT( (PTRVALID(psTable, sizeof(HASHTABLE)),
			"hashTable_RemoveElement: table pointer invalid\n") );

	/* get hashed index */
	udwHashIndex = hashTable_GetHashKey( psTable, iKey1, iKey2 );

	/* init previous node pointer */
	psPrev = NULL;

	/* get first node in table slot */
	psNode = psTable->ppsNode[udwHashIndex];

	/* loop through node list to find element match */
	while ( psNode != NULL && !( psElement == psNode->psElement &&
			psNode->iKey1 == iKey1 && psNode->iKey2 == iKey2) )
	{
		psPrev = psNode;
		psNode = psNode->psNext;
	}

	/* remove node from hash table and return to heap */
	if ( psNode == NULL )
	{
		return FALSE;
	}
	else
	{
		/* remove from hash table */
		if ( psPrev == NULL )
		{
			psTable->ppsNode[udwHashIndex] = psNode->psNext;
		}
		else
		{
			psPrev->psNext = psNode->psNext;
		}

		/* set next node pointer to this one if necessary */
		if ( psTable->psNextNode == psNode )
		{
			psTable->psNextNode = psPrev;
		}

		/* setup next node pointer */
		hashTable_SetNextNode( psTable, TRUE );

		/* return element to heap */
		ASSERT( (PTRVALID(psNode->psElement, psTable->udwElementSize),
				"hashTable_RemoveElement: element pointer invalid\n") );
		HEAP_FREE( psTable->psElementHeap, psNode->psElement );

		/* return node to heap */
		ASSERT( (PTRVALID(psNode, sizeof(HASHNODE)),
				"hashTable_RemoveElement: node pointer invalid\n") );
		HEAP_FREE( psTable->psNodeHeap, psNode );

		return TRUE;
	}
}

/***************************************************************************/

void *
hashTable_GetNext( HASHTABLE *psTable )
{
	void	*psElement;

	ASSERT( (PTRVALID(psTable, sizeof(HASHTABLE)),
		"hashTable_GetNext: hash table pointer invalid\n") );

	if ( psTable->psNextNode == NULL )
	{
		return NULL;
	}
	else
	{
		psElement = psTable->psNextNode->psElement;

		/* setup next node pointer */
		hashTable_SetNextNode( psTable, TRUE );

		return psElement;
	}
}

/***************************************************************************/

void *
hashTable_GetFirst( HASHTABLE *psTable )
{
	ASSERT( (PTRVALID(psTable, sizeof(HASHTABLE)),
		"hashTable_GetFirst: hash table pointer invalid\n") );

	/* init current index and node to start of table */
	psTable->sdwCurIndex = 0;
	psTable->psNextNode  = psTable->ppsNode[0];

	/* search through table for first allocated node */
	hashTable_SetNextNode( psTable, FALSE );

	/* return it */
	return hashTable_GetNext( psTable );
}

/***************************************************************************/
