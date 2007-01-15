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
/*! \file ptrlist.h
 * \brief Generic singly-linked list of elements
 *
 */
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
