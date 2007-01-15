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
/*! \file memint.h
 *  \brief Internal framework definitions for the memory system.
 */
#ifndef _memint_h
#define _memint_h

/* The size of saftey buffer to leave before and after any malloc'ed memory.
   Can use this to check writing over end of buffers */

#define SAFETY_ZONE_SIZE	(32)


/* The character used to initialise malloc'ed memory, or to trash memory before
   freeing it */
#define SAFETY_ZONE_BYTE	(0xac)
#define INITIALISE_BYTE		(0xcd)
#define FREE_BYTE			(0xdd)

// memory block header for the treap code
typedef struct _mem_node
{
	TREAP_NODE_BASE;		// The treap data to store the node
	TREAP_NODE_DEBUG;		// The debug info for the node (file, line).
	UDWORD		size;		// The memory block size
} MEM_NODE;

/* compare two memory blocks */
extern SDWORD	memBlockCmp(void *key1, void *key2);
#endif

