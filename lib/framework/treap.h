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


/* The basic elements in the treap node.
 * These are done as macros so that the memory system
 * can use parts of the treap system.
 */
#define TREAP_NODE_BASE \
	void				*key;				/* The key to sort the node on */ \
	UDWORD				priority;			/* Treap priority */ \
	void				*pObj;				/* The object stored in the treap */ \
	struct _treap_node	*psLeft, *psRight	/* The sub trees */

/* The debug info */
#define TREAP_NODE_DEBUG \
	const char			*pFile;	/* file the node was created in */ \
	SDWORD				line	/* line the node was created at */

typedef struct _treap_node
{
	TREAP_NODE_BASE;

#ifdef DEBUG_TREAP
	TREAP_NODE_DEBUG;
#endif

} TREAP_NODE;

/* Treap data structure */
typedef struct TREAP
{
	TREAP_CMP		cmp;		// comparison function
	TREAP_NODE		*psRoot;	// root of the tree

#ifdef DEBUG_TREAP
	const char		*pFile;		// file the treap was created in
	SDWORD			line;		// line the treap was created at
#endif
} TREAP;

/****************************************************************************************/
/*                           Function Protoypes                                         */
/*                                                                                      */
/*      These should not be called directly - use the macros below                      */


/**
 * Store the location in C code at which a call to the heap was made
 *
 * \param	pFileName	source filename
 * \param	lineNumber	source file line number
 */
extern void treapSetCallPos(const char *pFileName, SDWORD lineNumber);

/* Function type for object equality */
//typedef BOOL (*TREAP_EQUAL)(void *pObj1, void *pObj2);

/**
 * Function to create a treap
 *
 * \param	ppsTreap	out-parameter which holds the created treap
 * \param	cmp			comparison function to use
 * \return true, if the treap creation was successfull
 */
extern BOOL treapCreate(TREAP **ppsTreap, TREAP_CMP cmp);

/* Add an object to a treap
 */
extern BOOL treapAdd(TREAP *psTreap, void *key, void *pObj);

/* Remove an object from the treap */
extern BOOL treapDel(TREAP *psTreap, void *key);

/* Find an object in a treap */
extern void *treapFind(TREAP *psTreap, const void *key);

/* Release all the nodes in the treap */
extern void treapReset(TREAP *psTreap);

/* Destroy a treap and release all the memory associated with it */
extern void treapDestroy(TREAP *psTreap);

/* Display the treap structure using DBPRINTF */
extern void treapDisplay(TREAP *psTreap);

/* Return the object with the smallest key in the treap
 * This is useful if the objects in the treap need to be
 * deallocated.  i.e. getSmallest, delete from treap, free memory
 */
extern void *treapGetSmallest(TREAP *psTreap);

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

#define TREAP_DEL(psTreap, key) \
	treapDel(psTreap, key)

#define TREAP_FIND(psTreap, key) \
	treapFind(psTreap, key)

#define TREAP_RESET(psTreap) \
	treapReset(psTreap)

#define TREAP_DESTROY(psTreap) \
	treapDestroy(psTreap)

#define TREAP_DISPLAY(psTreap) \
	treapDisplay(psTreap)

#define TREAP_GETSMALLEST(psTreap) \
	treapGetSmallest(psTreap)

#else

// release versions of the TREAP calls
#define TREAP_CREATE(ppsTreap, cmp) \
	 treapCreate(ppsTreap, cmp)

#define TREAP_ADD(psTreap, key, pObject) \
	 treapAdd(psTreap, key, pObject)

#define TREAP_DEL(psTreap, key) \
	treapDel(psTreap, key)

#define TREAP_FIND(psTreap, key) \
	treapFind(psTreap, key)

#define TREAP_RESET(psTreap) \
	treapReset(psTreap)

#define TREAP_DESTROY(psTreap) \
	treapDestroy(psTreap)

#define TREAP_DISPLAY(psTreap)

#define TREAP_GETSMALLEST(psTreap) \
	treapGetSmallest(psTreap)

#endif


#endif
