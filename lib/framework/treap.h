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


/// Forward declarations to allow pointers to these types
struct TREAP;
struct STR_ID;

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

/**
 * Function to create a treap
 *
 * \param	ppsTreap	out-parameter which holds the created treap
 * \return true, if the treap creation was successfull
 */
extern BOOL treapCreate(struct TREAP **ppsTreap);

/* Add an object to a treap
 */
extern BOOL treapAdd(struct TREAP *psTreap, const char *key, struct STR_ID* pObj);

/* Remove an object from the treap */
extern BOOL treapDel(struct TREAP *psTreap, const char *key);

/* Find an object in a treap */
extern struct STR_ID* treapFind(struct TREAP *psTreap, const char *key);

/* Destroy a treap and release all the memory associated with it */
extern void treapDestroy(struct TREAP *psTreap);

/* Return the object with the smallest key in the treap
 * This is useful if the objects in the treap need to be
 * deallocated.  i.e. getSmallest, delete from treap, free memory
 */
extern struct STR_ID* treapGetSmallest(struct TREAP *psTreap);

/****************************************************************************************/
/*                            Macro definitions                                         */

#ifdef DEBUG_TREAP

// debugging versions of the TREAP calls
#define TREAP_CREATE(ppsTreap) \
	(treapSetCallPos(__FILE__, __LINE__), \
	 treapCreate(ppsTreap))

#define TREAP_ADD(psTreap, key, pObject) \
	(treapSetCallPos(__FILE__, __LINE__), \
	 treapAdd(psTreap, key, pObject))

#else

// release versions of the TREAP calls
#define TREAP_CREATE(ppsTreap) \
	 treapCreate(ppsTreap)

#define TREAP_ADD(psTreap, key, pObject) \
	 treapAdd(psTreap, key, pObject)

#endif


#endif
