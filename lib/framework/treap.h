/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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

/**
 * Function to create a treap
 *
 * \return On success a non-NULL pointer, which holds the created treap. On
 *         failure NULL is returned.
 */
struct TREAP_NODE **treapCreate();

/* Add an object to a treap
 */
bool treapAdd(struct TREAP_NODE **psTreap, const char *key, const char *string);

/* Find an object in a treap */
const char *treapFind(struct TREAP_NODE **psTreap, const char *key);

/** Find the key associated with the given string number */
const char *treapFindKey(struct TREAP_NODE **psTreap, const char *string);

/* Destroy a treap and release all the memory associated with it */
void treapDestroy(struct TREAP_NODE **psTreap);

#endif
