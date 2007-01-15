/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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

#include "lib/framework/frame.h"

#include "ptrlist.h"

/***************************************************************************/

extern void					*g_ElementToBeRemoved;

/***************************************************************************/

static void	ptrList_Init( PTRLIST *ptrList );

/***************************************************************************/

BOOL
ptrList_Create( PTRLIST **ppsList, UDWORD udwInitElements,
				UDWORD udwExtElements, UDWORD udwElementSize )
{
	/* create ptr list struct */
	(*ppsList) = (PTRLIST*)MALLOC( sizeof(PTRLIST) );

	/* allocate heaps */
	if ( !HEAP_CREATE( &(*ppsList)->psNodeHeap, sizeof(LISTNODE),
						udwInitElements, udwExtElements) )
	{
		return FALSE;
	}

	if ( !HEAP_CREATE( &(*ppsList)->psElementHeap, udwElementSize,
						udwInitElements, udwExtElements) )
	{
		return FALSE;
	}

	/* init members */
	(*ppsList)->udwElements    = udwInitElements;
	(*ppsList)->udwExtElements = udwExtElements;
	(*ppsList)->udwElementSize = udwElementSize;

	ptrList_Init( *ppsList );

	return TRUE;
}

/***************************************************************************/

void
ptrList_Destroy( PTRLIST *ptrList )
{
	ASSERT( PTRVALID(ptrList, sizeof(PTRLIST)),
			"ptrList_Destroy: list pointer invalid\n" );

	ptrList_Clear( ptrList );

	/* destroy heaps */
	HEAP_DESTROY( ptrList->psNodeHeap );
	HEAP_DESTROY( ptrList->psElementHeap );

	/* free struct */
	FREE( ptrList );
}

/***************************************************************************/

static void
ptrList_Init( PTRLIST *ptrList )
{
	ptrList->sdwCurIndex   = 0;
	ptrList->psCurNode     = NULL;
	ptrList->psNode        = NULL;
	ptrList->bDontGetNext  = FALSE;
}

/***************************************************************************/
/*
 * ptrList_Clear
 *
 * Returns all nodes from hash table to free node list
 */
/***************************************************************************/

void
ptrList_Clear( PTRLIST *ptrList )
{
	LISTNODE	*psNode, *psNodeTmp;

	ASSERT( PTRVALID(ptrList, sizeof(PTRLIST)),
			"ptrList_Destroy: table pointer invalid\n" );

	/* free nodes */
	psNode = ptrList->psNode;

	while ( psNode != NULL )
	{
		/* return node element to heap */
		ASSERT( PTRVALID(psNode->psElement, ptrList->udwElementSize),
			"ptrList_Destroy: element pointer invalid\n" );
		HEAP_FREE( ptrList->psElementHeap, psNode->psElement );

		/* return node to heap */
		ASSERT( PTRVALID(psNode, sizeof(LISTNODE)),
				"ptrList_Destroy: node pointer invalid\n" );
		psNodeTmp = psNode->psNext;
		HEAP_FREE( ptrList->psNodeHeap, psNode );
		psNode = psNodeTmp;
	}

	ptrList_Init( ptrList );
}

/***************************************************************************/
/*
 * ptrList_GetElement
 *
 * Gets free node from heap and returns element pointer
 */
/***************************************************************************/

void *
ptrList_GetElement( PTRLIST *ptrList )
{
	void	*psElement;

	ASSERT( PTRVALID(ptrList, sizeof(PTRLIST)),
			"ptrList_GetElement: table pointer invalid\n" );

	HEAP_ALLOC( ptrList->psElementHeap, &psElement );

	return psElement;
}

/***************************************************************************/
/*
 * ptrList_FreeElement
 *
 * Free element that was allocated using ptrList_GetElement without
 * inserting in list: will fail if element not allocated from ptrList
 */
/***************************************************************************/

void
ptrList_FreeElement( PTRLIST *ptrList, void *psElement )
{
	ASSERT( PTRVALID(ptrList, sizeof(PTRLIST)),
			"ptrList_FreeElement: table pointer invalid\n" );

	if ( HEAP_FREE( ptrList->psElementHeap, psElement ) == FALSE )
	{
		debug( LOG_NEVER, "ptrList_FreeElement: couldn't free element\n" );
	}
}

/***************************************************************************/

void
ptrList_InsertElement( PTRLIST *ptrList, void *psElement, SDWORD sdwKey )
{
	LISTNODE	*psNode, *psCurNode, *psPrevNode;

	ASSERT( PTRVALID(ptrList, sizeof(PTRLIST)),
			"ptrList_InsertElement: table pointer invalid\n" );
	ASSERT( PTRVALID(psElement, ptrList->udwElementSize),
			"ptrList_InsertElement: element pointer invalid\n" );

	/* get node from heap */
	HEAP_ALLOC( ptrList->psNodeHeap, (void**) &psNode );

	/* set node elements */
	psNode->sdwKey    = sdwKey;
	psNode->psElement = psElement;
	psNode->psNext    = NULL;

	psPrevNode = NULL;
	psCurNode = ptrList->psNode;

	/* find correct position to insert node */
	while ( psCurNode != NULL )
	{
		if ( psCurNode->sdwKey < sdwKey )
		{
			break;
		}

		psPrevNode = psCurNode;
		psCurNode = psCurNode->psNext;
	}

	/* insert node */
	if ( psPrevNode == NULL )
	{
		ptrList->psNode = psNode;

		if ( psCurNode != NULL )
		{
			psNode->psNext = psCurNode;
		}
	}
	else
	{
		psPrevNode->psNext = psNode;
		psNode->psNext = psCurNode;
	}
}

/***************************************************************************/

BOOL
ptrList_RemoveElement( PTRLIST *ptrList, void *psElement, SDWORD sdwKey )
{
	LISTNODE	*psCurNode, *psPrevNode;
	BOOL		bOK;

	ASSERT( PTRVALID(ptrList, sizeof(PTRLIST)),
			"ptrList_RemoveElement: table pointer invalid\n" );

	psPrevNode = NULL;
	psCurNode = ptrList->psNode;

	/* find correct position to insert node */
	while ( psCurNode != NULL &&
			!(psCurNode->sdwKey == sdwKey &&
			  psCurNode->psElement == psElement) )
	{
		psPrevNode = psCurNode;
		psCurNode = psCurNode->psNext;
	}

	/* remove node from hash table and return to heap */
	if ( psCurNode == NULL )
	{
		bOK = FALSE;
	}
	else
	{

ASSERT( psCurNode->psElement == psElement,
"ptrList_RemoveElement: removing wrong element!\n" );

		/* remove from list */
		if ( psPrevNode == NULL )
		{
			ptrList->psNode = psCurNode->psNext;
		}
		else
		{
			psPrevNode->psNext = psCurNode->psNext;
		}

		/* check whether table current node pointer is this node */
		if ( ptrList->psCurNode == psCurNode )
		{
			/* point it to the previous node if valid */
			if ( psPrevNode == NULL )
			{
				/* set next node and set flag */
				ptrList->psCurNode = psCurNode->psNext;
				ptrList->bDontGetNext = TRUE;
			}
			else
			{
				ptrList->psCurNode = psPrevNode;
			}
		}

		/* return element to heap */
		ASSERT( PTRVALID(psCurNode->psElement, ptrList->udwElementSize),
				"ptrList_RemoveElement: element pointer invalid\n" );
		ASSERT( psCurNode->psElement == psElement,
				"ptrList_RemoveElement: removing wrong element!\n" );
		HEAP_FREE( ptrList->psElementHeap, psCurNode->psElement );

		/* return node to heap */
		ASSERT( PTRVALID(psCurNode, sizeof(LISTNODE)),
				"ptrList_RemoveElement: node pointer invalid\n" );
		HEAP_FREE( ptrList->psNodeHeap, psCurNode );

		bOK = TRUE;
	}

	return bOK;
}

/***************************************************************************/

void *
ptrList_GetNext( PTRLIST *ptrList )
{
	void	*pElement = NULL;

	if ( ptrList == NULL )
	{
		pElement = NULL;
	}

	if ( ptrList->psCurNode == NULL )
	{
		pElement = NULL;
	}
	else
	{
		if ( ptrList->bDontGetNext == TRUE )
		{
			ptrList->bDontGetNext = FALSE;
		}
		else
		{
			ptrList->psCurNode = ptrList->psCurNode->psNext;
		}

		if ( ptrList->psCurNode == NULL )
		{
			pElement = NULL;
		}
		else
		{
			pElement = ptrList->psCurNode->psElement;
		}
	}

	return pElement;
}

/***************************************************************************/

void *
ptrList_GetFirst( PTRLIST *ptrList )
{
	void	*pElement = NULL;

	ptrList->bDontGetNext = FALSE;
	ptrList->psCurNode = ptrList->psNode;

	if ( ptrList->psCurNode == NULL )
	{
		pElement = NULL;
	}
	else
	{
		pElement = ptrList->psCurNode->psElement;
	}

	return pElement;
}

/***************************************************************************/
