/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
/***************************************************************************/

#include <limits.h>
#include <memory.h>
#include <string.h>

#include "lib/framework/frame.h"

#include "hashtable.h"

/***************************************************************************/

static UDWORD HashTest(intptr_t iKey1, intptr_t iKey2)
{
	return (UDWORD) iKey1 + iKey2;
}

/***************************************************************************/

bool
hashTable_Create( HASHTABLE **ppsTable, UDWORD udwTableSize,
	UDWORD udwInitElements, UDWORD udwExtElements, UDWORD udwElementSize )
{
	UDWORD			udwSize;

	/* allocate and init table */

	(*ppsTable) = (HASHTABLE*)malloc( sizeof(HASHTABLE) );
	udwSize = udwTableSize * sizeof(HASHNODE *);
	(*ppsTable)->ppsNode = (HASHNODE**)malloc( udwSize );
	memset( (*ppsTable)->ppsNode, 0, udwSize );

	/* init members */
	(*ppsTable)->udwTableSize   = udwTableSize;
	(*ppsTable)->udwElements    = udwInitElements;
	(*ppsTable)->udwExtElements = udwExtElements;
	(*ppsTable)->udwElementSize = udwElementSize;
	(*ppsTable)->sdwCurIndex    = 0;
	(*ppsTable)->psNextNode     = (*ppsTable)->ppsNode[0];
	(*ppsTable)->pFreeFunc      = NULL;

	/* set hash function to internal */
	hashTable_SetHashFunction( (*ppsTable), HashTest );

	return true;
}

/***************************************************************************/

void
hashTable_Destroy( HASHTABLE *psTable )
{
	ASSERT( psTable != NULL,
			"hashTable_Destroy: table pointer invalid\n" );

	hashTable_Clear( psTable );

	/* free table */
	free( psTable->ppsNode );
	free( psTable );
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

	ASSERT( psTable != NULL,
			"hashTable_Destroy: table pointer invalid\n" );

	/* free nodes */
	for ( i=0; i<psTable->udwTableSize; i++ )
	{
		/* free table entry nodelist */
		psNode = psTable->ppsNode[i];
		while ( psNode != NULL )
		{
			/* return node element to heap */
			ASSERT( psNode->psElement != NULL,
				"hashTable_Destroy: element pointer invalid\n" );

			/* do free-element callback if set */
			if ( psTable->pFreeFunc != NULL )
			{
				(psTable->pFreeFunc) (psNode->psElement);
			}

			/* free element */
			free(psNode->psElement);

			/* return node to heap */
			ASSERT( psNode != NULL,
				"hashTable_Destroy: node pointer invalid\n" );
			psNodeTmp = psNode->psNext;
			free(psNode);
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
	ASSERT( psTable != NULL,
			"hashTable_SetHashFunction: table pointer invalid\n" );

	psTable->pHashFunc = pHashFunc;
}

/***************************************************************************/

void
hashTable_SetFreeElementFunction( HASHTABLE *psTable,
									HASHFREEFUNC pFreeFunc )
{
	ASSERT( psTable != NULL,
			"hashTable_SetFreeElementFunction: table pointer invalid\n" );

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

	ASSERT( psTable != NULL,
			"hashTable_GetElement: table pointer invalid\n" );

	psElement = malloc(psTable->udwElementSize);
	if (psElement == NULL)  // if the alloc fails then return NULL
	{
		debug(LOG_ERROR, "hashTable_GetElement: Out of memory");
		return NULL;
	}

	return psElement;
}

/***************************************************************************/

static UDWORD hashTable_GetHashKey(HASHTABLE *psTable, intptr_t iKey1, intptr_t iKey2)
{
	ASSERT( psTable != NULL,
		"hashTable_GetFirst: hash table pointer invalid\n" );

	/* get hashed index */
	return (psTable->pHashFunc) ( iKey1, iKey2 ) % psTable->udwTableSize;
}

/***************************************************************************/

void hashTable_InsertElement(HASHTABLE *psTable, void *psElement, intptr_t iKey1, intptr_t iKey2)
{
	UDWORD		udwHashIndex;
	HASHNODE	*psNode;

	ASSERT( psTable != NULL,
			"hashTable_InsertElement: table pointer invalid\n" );
	ASSERT( psElement != NULL,
			"hashTable_InsertElement: element pointer invalid\n" );

	/* get hashed index */
	udwHashIndex = hashTable_GetHashKey( psTable, iKey1, iKey2 );

	/* get node from heap */
	psNode = (HASHNODE *)malloc(sizeof(HASHNODE));

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

void *hashTable_FindElement(HASHTABLE *psTable, intptr_t iKey1, intptr_t iKey2)
{
	UDWORD		udwHashIndex;
	HASHNODE	*psNode;

	ASSERT( psTable != NULL,
			"hashTable_FindElement: table pointer invalid\n" );

	/* get hashed index */
	udwHashIndex = hashTable_GetHashKey( psTable, iKey1, iKey2 );

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
		return NULL;
	}
	else
	{
		return psNode->psElement;
	}
}

/***************************************************************************/

static void
hashTable_SetNextNode( HASHTABLE *psTable, bool bMoveToNextNode )
{
	if ( (bMoveToNextNode == true) && (psTable->psNextNode != NULL) )
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

bool
hashTable_RemoveElement(HASHTABLE *psTable, void *psElement, intptr_t iKey1, intptr_t iKey2)
{
	UDWORD		udwHashIndex;
	HASHNODE	*psNode, *psPrev;

	ASSERT( psTable != NULL,
			"hashTable_RemoveElement: table pointer invalid\n" );

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
		return false;
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
		hashTable_SetNextNode( psTable, true );

		/* return element to heap */
		ASSERT( psNode->psElement != NULL,
				"hashTable_RemoveElement: element pointer invalid\n" );
		free(psNode->psElement);

		/* return node to heap */
		ASSERT( psNode != NULL,
				"hashTable_RemoveElement: node pointer invalid\n" );
		free(psNode);

		return true;
	}
}

/***************************************************************************/

void *
hashTable_GetNext( HASHTABLE *psTable )
{
	void	*psElement;

	ASSERT( psTable != NULL,
		"hashTable_GetNext: hash table pointer invalid\n" );

	if ( psTable->psNextNode == NULL )
	{
		return NULL;
	}
	else
	{
		psElement = psTable->psNextNode->psElement;

		/* setup next node pointer */
		hashTable_SetNextNode( psTable, true );

		return psElement;
	}
}

/***************************************************************************/

void *
hashTable_GetFirst( HASHTABLE *psTable )
{
	ASSERT( psTable != NULL,
		"hashTable_GetFirst: hash table pointer invalid\n" );

	/* init current index and node to start of table */
	psTable->sdwCurIndex = 0;
	psTable->psNextNode  = psTable->ppsNode[0];

	/* search through table for first allocated node */
	hashTable_SetNextNode( psTable, false );

	/* return it */
	return hashTable_GetNext( psTable );
}

/***************************************************************************/
