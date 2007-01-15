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
/*! \file pqueue.h
 * \brief Priority queue code
 *
 */
/***************************************************************************/

#ifndef _PQUEUE_H_
#define _PQUEUE_H_

/***************************************************************************/

typedef void (* QUEUE_CLEAR_FUNC) ( void *psElement );

typedef struct QUEUE_NODE
{
	void *				psElement;
	int					iPriority;
	struct QUEUE_NODE *	psPrev;
	struct QUEUE_NODE *	psNext;
}
QUEUE_NODE;

typedef struct QUEUE
{
	QUEUE_NODE *	psFreeNodeList;
	QUEUE_NODE *	psNodeQHead;
	QUEUE_NODE *	psCurNode;
	int				iElementSize;
	int				iMaxElements;
	int				iFreeNodes;
	int				iQueueNodes;

	QUEUE_CLEAR_FUNC	pfClearFunc;
}
QUEUE;

/***************************************************************************/

extern BOOL			queue_Init( QUEUE ** ppQueue, int iMaxElements,
								int iElementSize,
								QUEUE_CLEAR_FUNC pfClearFunc );
extern void			queue_Destroy( QUEUE * pQueue );
extern void			queue_Clear( QUEUE * pQueue );

extern void			queue_Enqueue( QUEUE * pQueue, void * psElement,
									int iPriority );
extern void *		queue_Dequeue( QUEUE * pQueue );
extern QUEUE_NODE *	queue_FindElement( QUEUE * pQueue, void * psElement );
extern BOOL			queue_RemoveElement( QUEUE * pQueue, void * psElement );
extern void	*		queue_GetHead( QUEUE * pQueue );
extern void	*		queue_GetNext( QUEUE * pQueue );
extern BOOL			queue_RemoveCurrent( QUEUE * pQueue );
extern BOOL			queue_RemoveNode( QUEUE * pQueue, QUEUE_NODE *psNode );
extern SDWORD		queue_GetFreeElements( QUEUE * pQueue );

/***************************************************************************/

#endif	// _PQUEUE_H_

/***************************************************************************/
