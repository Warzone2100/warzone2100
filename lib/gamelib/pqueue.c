/***************************************************************************/
/*
 * Priority queue implementation
 *
 *
 */
/***************************************************************************/

#include "lib/framework/frame.h"

#include "priority.h"
#include "pqueue.h"

/***************************************************************************/

#define QUEUE_INSERTION_TEST	1

/***************************************************************************/

BOOL
queue_Init( QUEUE ** ppQueue, int iMaxElements, int iElementSize,
				QUEUE_CLEAR_FUNC pfClearFunc )
{
	int			i;
	QUEUE_NODE	*psNode;

	/* allocate queue */
	(*ppQueue) = (QUEUE *) MALLOC( sizeof(QUEUE) );

	if ( (*ppQueue) == NULL )
	{
		DBERROR( ("queue_Init: couldn't allocate memory for queue") );
		return FALSE;
	}

	/* set up free node list */
	(*ppQueue)->psFreeNodeList = NULL;
	for ( i=0; i<iMaxElements; i++ )
	{
		/* allocate node */
		psNode = (QUEUE_NODE *) MALLOC( sizeof(QUEUE_NODE) );

		if ( (*ppQueue) == NULL )
		{
			DBERROR( ("queue_Init: couldn't allocate memory for queue node") );
			return FALSE;
		}

		/* init node */
		psNode->psElement = NULL;
		psNode->iPriority  = LOW_PRIORITY;

		/* add to head of list */
		psNode->psPrev = NULL;
		psNode->psNext = (*ppQueue)->psFreeNodeList;
		(*ppQueue)->psFreeNodeList = psNode;
	}

	/* init node counts */
	(*ppQueue)->iFreeNodes  = iMaxElements;
	(*ppQueue)->iQueueNodes = 0;

	/* init queue list */
	(*ppQueue)->psNodeQHead = NULL;
	(*ppQueue)->psCurNode   = NULL;

	/* init other queue struct members */
	(*ppQueue)->iElementSize = iElementSize;
	(*ppQueue)->iMaxElements = iMaxElements;
	(*ppQueue)->pfClearFunc  = pfClearFunc;

	return TRUE;
}

/***************************************************************************/

void
queue_Destroy( QUEUE * pQueue )
{
	QUEUE_NODE	*psNode;

	/* free up free node list */
	while ( pQueue->psFreeNodeList != NULL )
	{
		psNode = pQueue->psFreeNodeList;
		pQueue->psFreeNodeList = pQueue->psFreeNodeList->psNext;
		FREE( psNode );
	}

	/* free up node queue */
	while ( pQueue->psNodeQHead != NULL )
	{
		psNode = pQueue->psNodeQHead;
		pQueue->psNodeQHead = pQueue->psNodeQHead->psNext;
		FREE( psNode );
	}
	FREE( pQueue );
}

/***************************************************************************/

void queue_Clear( QUEUE * pQueue )
{
	QUEUE_NODE	*psNode;

	while ( pQueue->psNodeQHead != NULL )
	{
		psNode = pQueue->psNodeQHead;
		pQueue->psNodeQHead = pQueue->psNodeQHead->psNext;

		/* do element callback */
		(pQueue->pfClearFunc)(psNode->psElement);

		/* return node to free node list */
		psNode->psNext = pQueue->psFreeNodeList;
		pQueue->psFreeNodeList = psNode;

		/* update node counts */
		pQueue->iFreeNodes++;
		pQueue->iQueueNodes--;
	}
}

/***************************************************************************/
/*
 * queue_Enqueue
 *
 * Add element to queue in priority order
 */
/***************************************************************************/

void
queue_Enqueue( QUEUE * pQueue, void * psElement, int iPriority )
{
	QUEUE_NODE	*psFreeNode, *psNode, *psNodePrev;

	/* check input */
	ASSERT( (PTRVALID(pQueue,sizeof(QUEUE)),
			"queue_Enqueue: queue pointer invalid\n") );
	ASSERT( (PTRVALID(psElement,pQueue->iElementSize),
			"queue_Enqueue: element pointer invalid\n") );

#if QUEUE_INSERTION_TEST
	psNode = pQueue->psNodeQHead;
	while ( psNode != NULL )
	{
		ASSERT( (psNode->psElement != psElement,
				"duplicate element found\n") );

		psNode = psNode->psNext;
	}
#endif

	/* check list not empty */
	if ( pQueue->psFreeNodeList == NULL )
	{
		DBPRINTF( ("queue_GetFreeElement: all nodes allocated: flushing queue.\n") );
		queue_Clear( pQueue );
	}

	/* get free node from head of list */
	psFreeNode = pQueue->psFreeNodeList;
	pQueue->psFreeNodeList = pQueue->psFreeNodeList->psNext;

	/* update node counts */
	pQueue->iFreeNodes--;
	pQueue->iQueueNodes++;

	/* attach element to node */
	psFreeNode->psElement = psElement;

	/* init pointer to head of queue */
	psNode = pQueue->psNodeQHead;
	psNodePrev = NULL;

	/* init node priority */
	psFreeNode->iPriority = iPriority;

	/* find correct place in queue according to priority */
	while ( psNode != NULL )
	{
		/* break if priority slot found */
		if ( psNode->iPriority < iPriority )
		{
			break;
		}

		/* update pointers */
		psNodePrev = psNode;
		psNode = psNode->psNext;
	}

	/* insert new node */
	psFreeNode->psPrev = psNodePrev;
	psFreeNode->psNext = psNode;

	/* update previous queue node pointer */
	if ( psNodePrev == NULL )
	{
		pQueue->psNodeQHead = psFreeNode;
	}
	else
	{
		psNodePrev->psNext = psFreeNode;
	}

	/* update next queue node pointer */
	if ( psNode != NULL )
	{
		psNode->psPrev = psFreeNode;
	}
}

/***************************************************************************/
/*
 * queue_Dequeue
 *
 * Remove head of queue
 */
/***************************************************************************/

void *
queue_Dequeue( QUEUE * pQueue )
{
	void *			psElement = NULL;
	QUEUE_NODE *	psNode;

	/* check input */
	ASSERT( (PTRVALID(pQueue,sizeof(QUEUE)),
			"queue_Dequeue: queue pointer invalid\n") );

	if ( pQueue->psNodeQHead != NULL )
	{
		/* remove node at head of queue */
		psNode = pQueue->psNodeQHead;
		pQueue->psNodeQHead = pQueue->psNodeQHead->psNext;

		/* copy element from node for return */
		psElement = psNode->psElement;

		/* return node to free node list */
		psNode->psNext = pQueue->psFreeNodeList;
		pQueue->psFreeNodeList = psNode;

		/* update node counts */
		pQueue->iFreeNodes++;
		pQueue->iQueueNodes--;

		ASSERT( (pQueue->iQueueNodes >= 0, "queue_Dequeue: queue nodes < 0") );
	}

	return psElement;
}

/***************************************************************************/
/*
 * queue_FindElement
 *
 * Find specific element in queue and return node pointer
 */
/***************************************************************************/

QUEUE_NODE *
queue_FindElement( QUEUE * pQueue, void * psElement )
{
	QUEUE_NODE	*psNode, *psNodePrev;

	/* check input */
	ASSERT( (PTRVALID(pQueue,sizeof(QUEUE)),
			"queue_FindElement: queue pointer invalid\n") );
	ASSERT( (PTRVALID(psElement,pQueue->iElementSize),
			"queue_FindElement: element pointer invalid\n") );

	/* init pointers to head of queue */
	psNodePrev = psNode = pQueue->psNodeQHead;

	/* find node in queue */
	while ( psNodePrev != NULL && psNode != NULL )
	{
		/* return TRUE if match found */
		if ( psNode->psElement == psElement )
		{
			return psNode;
		}

		/* update pointers */
		psNodePrev = psNode;
		psNode = psNode->psNext;
	}

	return NULL;
}

/***************************************************************************/
/*
 * queue_GetHead
 *
 * Get element at head of queue
 */
/***************************************************************************/

void *
queue_GetHead( QUEUE * pQueue )
{
	/* set current node to head of queue */
	pQueue->psCurNode = pQueue->psNodeQHead;

	if ( pQueue->psCurNode != NULL )
	{
		return pQueue->psCurNode->psElement;
	}
	else
	{
		return NULL;
	}
}

/***************************************************************************/
/*
 * queue_GetNext
 *
 * Get next element in queue
 */
/***************************************************************************/

void *
queue_GetNext( QUEUE * pQueue )
{
	/* move to next node */
	if ( pQueue->psCurNode != NULL )
	{
		pQueue->psCurNode = pQueue->psCurNode->psNext;
	}

	/* return node element */
	if ( pQueue->psCurNode == NULL )
	{
		return NULL;
	}
	else
	{
		return pQueue->psCurNode->psElement;
	}
}

/***************************************************************************/
/*
 * queue_RemoveCurrent
 *
 * Remove current node from queue
 */
/***************************************************************************/

BOOL
queue_RemoveCurrent( QUEUE * pQueue )
{
	return queue_RemoveNode( pQueue, pQueue->psCurNode );
}

/***************************************************************************/
/*
 * queue_RemoveNode
 *
 * Remove node from queue
 */
/***************************************************************************/

BOOL
queue_RemoveNode( QUEUE * pQueue, QUEUE_NODE *psNode )
{
	/* check input */
	ASSERT( (PTRVALID(pQueue,sizeof(QUEUE)),
			"queue_RemoveNode: queue pointer invalid\n") );
	ASSERT( (PTRVALID(psNode,sizeof(QUEUE_NODE)),
			"queue_RemoveNode: node pointer invalid\n") );

	/* if node valid, remove from queue and return to free node list */
	if ( psNode == NULL )
	{
		return FALSE;
	}
	else
	{
		/* remove node from queue */
		if ( psNode->psPrev == NULL && psNode->psNext == NULL )
		{
			pQueue->psCurNode = NULL;
			pQueue->psNodeQHead = NULL;
		}
		else
		{
			/* make previous node point to next */
			if ( psNode->psPrev != NULL )
			{
				psNode->psPrev->psNext = psNode->psNext;
			}

			/* make next node point to previous */
			if ( psNode->psNext != NULL )
			{
				psNode->psNext->psPrev = psNode->psPrev;
			}

			/* set current queue node if necessary */
			if ( pQueue->psCurNode == psNode )
			{
				if ( psNode->psPrev != NULL )
				{
					pQueue->psCurNode = psNode->psPrev;
				}
				else if ( psNode->psNext != NULL )
				{
					pQueue->psCurNode = psNode->psNext;
				}
				else
				{
					pQueue->psCurNode = NULL;
				}
			}

			/* update queue pointer if neccessary */
			if ( pQueue->psNodeQHead == psNode )
			{
				pQueue->psNodeQHead = psNode->psNext;
			}
		}

		/* return node to free node list */
		psNode->psNext = pQueue->psFreeNodeList;
		pQueue->psFreeNodeList = psNode;

		/* update node counts */
		pQueue->iFreeNodes++;
		pQueue->iQueueNodes--;

		ASSERT( (pQueue->iQueueNodes >= 0, "queue_RemoveNode: queue nodes < 0") );

		return TRUE;
	}
}

/***************************************************************************/
/*
 * queue_RemoveElement
 *
 * Remove specific element from queue
 */
/***************************************************************************/

BOOL
queue_RemoveElement( QUEUE * pQueue, void * psElement )
{
	QUEUE_NODE *psNode;

	/* check input */
	ASSERT( (PTRVALID(pQueue,sizeof(QUEUE)),
			"queue_Dequeue: queue pointer invalid\n") );
	ASSERT( (PTRVALID(psElement,pQueue->iElementSize),
			"queue_RemoveElement: element pointer invalid\n") );

	if ( (psNode = queue_FindElement( pQueue, psElement )) != NULL )
	{
		queue_RemoveNode( pQueue, psNode );
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/***************************************************************************/
