/*
 * mem.h
 *
 * Interface to the malloc/free replacements
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

/* malloc replacements */
void *memMalloc(STRING *pFileName, SDWORD LineNumber, size_t Size);
void *memMallocRelease(size_t Size);

/* free replacements */
void memFree(STRING *pFileName, SDWORD LineNumber, void *pMemToFree);
void memFreeRelease(void *pMemToFree);

/* Check a pointer refers to a valid block of memory */
BOOL memPointerValid(void *pPtr, size_t Size);

/* Report on currently allocated memory */
void memMemoryReport(STRING *pFileName);

/* Display the memory treap */
void memDisplayTreap(STRING *pFileName);

#ifdef DEBUG_MALLOC

#define MALLOC(size)		memMalloc(__FILE__, __LINE__, size)
#define FREE(ptr)		{ void** p = &(ptr); memFree(__FILE__, __LINE__, *p); *p = NULL; }
#ifndef NO_PTRVALID
#define PTRVALID(ptr, size)	memPointerValid(ptr, size) 
#else
#define PTRVALID(ptr, size)	(((ptr)==NULL)?FALSE:TRUE)
#endif
#define MEMORYREPORT(file)	memMemoryReport(file)
#define MEMORYTREAP(file)	memDisplayTreap(file)

#else // !DEBUG_MALLOC

#define MALLOC(size)		memMallocRelease(size)
#define FREE(ptr)		{ void** p = (void**)&(ptr); memFreeRelease(*p); *p = NULL; }
#define PTRVALID(ptr, size)	(TRUE)
#define MEMORYREPORT(file)
#define MEMORYTREAP(file)	memDisplayTreap(file)

#endif // DEBUG_MALLOC


#endif
