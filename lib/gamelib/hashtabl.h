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
