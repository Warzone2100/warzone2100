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
/*! \file treap.h
 *  * \brief A balanced binary tree implementation
 *
 * Overhead for using the treap is -
 *		Overhead for the heap used by the treap :
 *                  24 bytes + 4 bytes per extension
 *      12 bytes for the root
 *      20 bytes per node
 */
#ifndef _treap_h
#define _treap_h

#include "types.h"
#include "debug.h"

/* Turn on and off the treap debugging */
#ifdef DEBUG
// #define DEBUG_TREAP true
#else
#undef DEBUG_TREAP
#endif


/* Function type for the object compare
 * return -1 for less
 *         1 for more
 *         0 for equal
 */
typedef SDWORD (*TREAP_CMP)(const void *key1, const void *key2);

/// Forward declaration to allow pointers to this type
struct TREAP;

/****************************************************************************************/
/*                           Function Protoypes                                         */
/*                                                                                      */
/*      These should not be called directly - use the macros below                      */


/**
 * Store the location in C code at which a call to the heap was made
 *
 * \param	fileName	source filename
 * \param	lineNumber	source file line number
 */
extern void treapSetCallPos(const char* fileName, int lineNumber);

/* Function type for object equality */
//typedef BOOL (*TREAP_EQUAL)(void *pObj1, void *pObj2);

/**
 * Function to create a treap
 *
 * \param	ppsTreap	out-parameter which holds the created treap
 * \param	cmp			comparison function to use
 * \return true, if the treap creation was successfull
 */
extern BOOL treapCreate(struct TREAP **ppsTreap, TREAP_CMP cmp);

/* Add an object to a treap
 */
extern BOOL treapAdd(struct TREAP *psTreap, void *key, void *pObj);

/* Remove an object from the treap */
extern BOOL treapDel(struct TREAP *psTreap, void *key);

/* Find an object in a treap */
extern void *treapFind(struct TREAP *psTreap, const void *key);

/* Destroy a treap and release all the memory associated with it */
extern void treapDestroy(struct TREAP *psTreap);

/* Return the object with the smallest key in the treap
 * This is useful if the objects in the treap need to be
 * deallocated.  i.e. getSmallest, delete from treap, free memory
 */
extern void *treapGetSmallest(struct TREAP *psTreap);

/****************************************************************************************/
/*                            Comparison Functions                                      */

/* A useful comparison function - keys are char pointers */
extern SDWORD treapStringCmp(const void *key1, const void *key2);

/****************************************************************************************/
/*                            Macro definitions                                         */

#ifdef DEBUG_TREAP

// debugging versions of the TREAP calls
#define TREAP_CREATE(ppsTreap, cmp) \
	(treapSetCallPos(__FILE__, __LINE__), \
	 treapCreate(ppsTreap, cmp))

#define TREAP_ADD(psTreap, key, pObject) \
	(treapSetCallPos(__FILE__, __LINE__), \
	 treapAdd(psTreap, key, pObject))

#else

// release versions of the TREAP calls
#define TREAP_CREATE(ppsTreap, cmp) \
	 treapCreate(ppsTreap, cmp)

#define TREAP_ADD(psTreap, key, pObject) \
	 treapAdd(psTreap, key, pObject)

#endif


#endif
