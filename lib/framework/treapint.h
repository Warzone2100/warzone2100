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
/*! \file treapint.h
 *  * \brief Internal function calls for the treap system to allow it
 * to be used by the memory system.
 *
 */
#ifndef _treapint_h
#define _treapint_h

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */
#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif

/* Recursive function to add an object to a tree */
extern void treapAddNode(TREAP_NODE **ppsRoot, TREAP_NODE *psNew, TREAP_CMP cmp);

/* Recursively find and remove a node from the tree */
extern TREAP_NODE *treapDelRec(TREAP_NODE **ppsRoot, void *key, TREAP_CMP cmp);

/* Recursively display the treap structure */
extern void treapDisplayRec(TREAP_NODE *psRoot, UDWORD indent);

#endif

