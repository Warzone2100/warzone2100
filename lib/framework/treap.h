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

/// Forward declarations to allow pointers to these types
struct TREAP_NODE;
struct STR_ID;

/**
 * Function to create a treap
 *
 * \return On success a non-NULL pointer, which holds the created treap. On
 *         failure NULL is returned.
 */
extern struct TREAP_NODE** treapCreate(void);

/* Add an object to a treap
 */
extern BOOL treapAdd(struct TREAP_NODE** psTreap, const char *key, struct STR_ID* pObj);

/* Remove an object from the treap */
extern BOOL treapDel(struct TREAP_NODE** psTreap, const char *key);

/* Find an object in a treap */
extern struct STR_ID* treapFind(struct TREAP_NODE** psTreap, const char *key);

/* Destroy a treap and release all the memory associated with it */
extern void treapDestroy(struct TREAP_NODE** psTreap);

/* Return the object with the smallest key in the treap
 * This is useful if the objects in the treap need to be
 * deallocated.  i.e. getSmallest, delete from treap, free memory
 */
extern struct STR_ID* treapGetSmallest(struct TREAP_NODE** psTreap);

#endif
