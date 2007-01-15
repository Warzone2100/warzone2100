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
/*! \file mem.h
 *  \brief Interface to the malloc/free replacements
 */

#ifndef _mem_h
#define _mem_h

#include <stdlib.h>
#include "types.h"
#include "debug.h"

/* DEBUG_MALLOC == TRUE uses debugging malloc and free
   DEBUG_MALLOC == FALSE uses normal malloc and free */
#ifdef DEBUG

#define DEBUG_MALLOC	TRUE
#define DEBUG_HEAP	TRUE
/* Uncomment below for even more debug spam */
#undef REALLY_DEBUG_HEAP
#undef REALLY_DEBUG_MALLOC

#else

#undef DEBUG_MALLOC
#undef DEBUG_HEAP

#endif

/* Function Prototypes */

/* Initialise the memory system */
BOOL memInitialise(void);
/* Shutdown the memory system */
void memShutDown(void);

/* Set a block heap to use for all memory allocation rather than standard malloc/free */
struct _block_heap;
void memSetBlockHeap(struct _block_heap *psHeap);
/* Get the current block heap */
struct _block_heap *memGetBlockHeap(void);

/* malloc and free replacements */
#ifdef DEBUG_MALLOC
void *memMalloc(const char *pFileName, SDWORD LineNumber, size_t Size);
void memFree(const char *pFileName, SDWORD LineNumber, void *pMemToFree);

#define MALLOC(size)		memMalloc(__FILE__, __LINE__, size)
#define FREE(ptr)		do { memFree(__FILE__, __LINE__, ptr); ptr = NULL; } while(0)

#else // !DEBUG_MALLOC
void *memMallocRelease(size_t Size);
void memFreeRelease(void *pMemToFree);

#define MALLOC(size)		memMallocRelease(size)
#define FREE(ptr)		do { memFreeRelease(ptr); ptr = NULL; } while(0)

#endif // DEBUG_MALLOC

/* Check a pointer refers to a valid block of memory */
BOOL memPointerValid(void *pPtr, size_t Size);

#ifdef REALLY_DEBUG_MALLOC
/* Report on currently allocated memory */
void memMemoryReport(void);

/* Recursive function to print out the list of memory blocks */
extern SDWORD memRecReport(MEM_NODE *psRoot);
#endif
/* Display the memory treap */
void memDisplayTreap(void);

#ifdef DEBUG_MALLOC


 #ifndef NO_PTRVALID
  #define PTRVALID(ptr, size)	memPointerValid(ptr, size) 
 #else 
  #define PTRVALID(ptr, size)	(((ptr)==NULL)?FALSE:TRUE)
 #endif

#else // !DEBUG_MALLOC

#define PTRVALID(ptr, size)	(TRUE)

#endif // DEBUG_MALLOC

#endif // _mem_h_
