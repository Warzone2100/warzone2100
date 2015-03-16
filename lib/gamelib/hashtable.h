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

/*! \file hashtabl.h
 * \brief A generic implementation of a hashtable
 *
 * The hashtable can store elements indexed by two keys.
 * Additionally traversing the stored elements similar to an
 * singly-linked list is possible.
 */

#ifndef _HASHTABL_H_
#define _HASHTABL_H_

/***************************************************************************/

#include "lib/framework/frame.h"

/***************************************************************************/
/* macros
 */

/**
 * Hashing function
 *
 * \param	iKey1		first key
 * \param	iKey2		second key
 * \return calculated hash-value for the keys
 * \see hashTable_SetHashFunction
 */
typedef UDWORD(* HASHFUNC)(intptr_t iKey1, intptr_t iKey2);

/**
 * Free Element function
 *
 * \param	psElement	the element to be freed
 * \see hashTable_SetFreeElementFunction
 */
typedef void	(* HASHFREEFUNC)(void *psElement);

/***************************************************************************/
/* structs
 */

struct HASHNODE
{
	intptr_t			iKey1;
	intptr_t			iKey2;
	void			*psElement;
	HASHNODE               *psNext;
};

struct HASHTABLE
{
	HASHNODE		**ppsNode;
	HASHNODE		*psNextNode;
	HASHFUNC		pHashFunc;
	HASHFREEFUNC	pFreeFunc;
	UDWORD			udwTableSize;
	UDWORD			udwElements;
	UDWORD			udwExtElements;
	UDWORD			udwElementSize;
	UDWORD			sdwCurIndex;
};

/***************************************************************************/
/* functions
 */

/**
 * Function to create a new hashtable
 *
 * \param	ppsTable	out-parameter which holds the created hashtable
 * \param	udwTableSize	size of the hashtable in elements
 * \param	udwInitElements	initial number of elements of the heap used to store the nodes
 * \param	udwExtElements	number of elements when extending the heap
 * \param	udwElementSize	size of elements to be stored in the hashtable
 */
bool	hashTable_Create(HASHTABLE **ppsTable, UDWORD udwTableSize,
                         UDWORD udwInitElements, UDWORD udwExtElements,
                         UDWORD udwElementSize);

/**
 * Function to destroy a hashtable
 *
 * \param	psTable		the hashtable to be destroyed
 */
void    hashTable_Destroy(HASHTABLE *psTable);

/**
 * Returns all nodes from hash table to free node list
 *
 * \param	psTable		the hashtable to be cleared
 */
void	hashTable_Clear(HASHTABLE *psTable);

/**
 * Gets free node from heap and returns element pointer
 *
 * \param	psTable		the hashtable
 * \return	pointer to the element
 */
void 	*hashTable_GetElement(HASHTABLE *psTable);

/**
 * Function to insert an element into the hashtable
 *
 * \param	psTable		the hashtable
 * \param	psElement	the element to be inserted
 * \param	iKey1		first key
 * \param	iKey2		second key
 */
void hashTable_InsertElement(HASHTABLE *psTable, void *psElement, intptr_t iKey1, intptr_t iKey2);

/**
 * Function to remove an element from the hashtable
 *
 * \param	psTable		the hashtable
 * \param	psElement	element to be removed
 * \param	iKey1		first key
 * \param	iKey2		second key
 * \return	true, if the element was contained in the hashtable
 */
bool hashTable_RemoveElement(HASHTABLE *psTable, void *psElement, intptr_t iKey1, intptr_t iKey2);

/**
 * Calculates hash index from keys and returns element in hash table
 *
 * \param	psTable		the hashtable
 * \param	iKey1		first key
 * \param	iKey2		second key
 * \return	the element
 */
void *hashTable_FindElement(HASHTABLE *psTable, intptr_t iKey1, intptr_t iKey2);

/**
 * Gets the first allocated element from the hashtable
 *
 * \param	psTable		the hashtable
 * \see		hashTable_GetNext
 * \return	the first element
 */
void 	*hashTable_GetFirst(HASHTABLE *psTable);

/**
 * Gets the next allocated element from the hashtable
 *
 * \param	psTable		the hashtable
 * \see		hashTable_GetFirst
 * \return	next element
 */
void 	*hashTable_GetNext(HASHTABLE *psTable);

/**
 * Function to the the hashing function to be used
 *
 * \param	psTable		the hashtable
 * \param	pHashFunc	the hashing function
 * \see HASHFUNC
 */
void	hashTable_SetHashFunction(HASHTABLE *psTable, HASHFUNC pHashFunc);

/**
 * Sets the function to be called when freeing an element.
 *
 * \param	psTable		the hashtable
 * \param	pFreeFunc	the free function
 * \see HASHFREEFUNC
 */
void	hashTable_SetFreeElementFunction(HASHTABLE *psTable,
        HASHFREEFUNC pFreeFunc);

/***************************************************************************/

#endif	// _HASHTABL_H_

/***************************************************************************/
