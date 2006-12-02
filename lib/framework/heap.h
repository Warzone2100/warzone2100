/*! \file heap.h
 * \brief Interface to the heap memory routines.
 *
 * Overhead of using the heap is :
 *			24 bytes for the initial block
 *           4 bytes for the extension blocks
 *
 */
#ifndef _heap_h
#define _heap_h

#include "types.h"
#include "debug.h"
 /* Include Mem.h to get the DEBUG_MALLOC #define - this controls whether
  * normal or debugging memory management is used.
  */
#include "mem.h"

/* structure used to store the list of free heap objects */
typedef struct _free_object
{
	struct _free_object		*psNext;
} FREE_OBJECT;

/* Header for each object in DEBUG_MALLOC mode */
typedef struct _heap_objhdr
{
	const char	*pFile;
	SDWORD	line;
	struct _heap_objhdr		*psNext;
} HEAP_OBJHDR;

/* structure used to store the extra space allocated for the heap */
typedef struct _heap_extension
{
	UBYTE		*pMemory;

	struct _heap_extension	*psNext;
} HEAP_EXTENSION;

typedef struct _obj_heap
{
	UDWORD		objSize;		// The size of the objects being stored on the heap
	UDWORD		initAlloc;		// The initial number of objects allocated
	UDWORD		extAlloc;		// The number of objects to allocate after the initial
								// allocation is used up
	struct _block_heap	*psBlkHeap;		// which block heap (if any) this object heap was allocated from

	FREE_OBJECT		*psFree;	// The currently free objects

	UBYTE		*pMemory;		// The main memory heap
	HEAP_EXTENSION	*psExt;		// Extension memory for the heap

#if DEBUG_HEAP
	UDWORD		maxUsage;		// The maximum number of objects used so far
	UDWORD		currUsage;		// The number of objects being used at the moment
	HEAP_OBJHDR	*psInUse;		// The list of headers of objects currently allocated

	const char	*pFile;			// The name of the file the heap was created in
	SDWORD		line;			// The line of the file the heap was created on
#endif
} OBJ_HEAP;

/****************************************************************************************/
/*                           Function Protoypes                                         */
/*                                                                                      */
/*      These should not be called directly - use the macros below                      */


/* Store the location in C code at which a call to the heap was made */
extern void heapSetCallPos(const char *pFileName, SDWORD lineNumber);

/* Function to create a heap
 * Takes the size of the objects to be managed by the heap,
 * the initial number of objects to allocate and the number of
 * objects to allocate when the heap is extended.
 * Returns an initialised OBJ_HEAP structure.
 */
extern BOOL heapCreate(OBJ_HEAP	**ppsHeap, UDWORD size, UDWORD init, UDWORD ext);

/* Allocate an object from a heap
 * Returns a pointer to the object if successful
 */
extern BOOL heapAlloc(OBJ_HEAP *psHeap, void **ppObject);

/* Return an object to the heap */
extern BOOL heapFree(OBJ_HEAP *psHeap, void *pObject);

/* Reset the heap, i.e. free all the objects in the heap */
extern void heapReset(OBJ_HEAP *psHeap);

/* Destroy a heap and release all the memory associated with it */
extern void heapDestroy(OBJ_HEAP *psHeap);

/* Produce a summary report on the heaps ... DEBUG_MALLOC only */
void heapReport(void);

void heapIntegrityCheck(OBJ_HEAP *psHeap);

/****************************************************************************************/
/*                            Macro definitions                                         */

#if DEBUG_HEAP

#define HEAP_CREATE(ppsHeap, size, init, ext) \
	(heapSetCallPos(__FILE__, __LINE__), \
	 heapCreate(ppsHeap, size, init, ext))

#define HEAP_ALLOC(psHeap, ppObject) \
	(heapSetCallPos(__FILE__, __LINE__), \
	 heapAlloc(psHeap, ppObject))

#else

#define HEAP_CREATE(ppsHeap, size, init, ext) \
	heapCreate(ppsHeap, size, init, ext)

#define HEAP_ALLOC(psHeap, ppObject) \
	heapAlloc(psHeap, ppObject)

#endif


#define HEAP_FREE(psHeap, pObject) \
	heapFree(psHeap, pObject)

#define HEAP_RESET(psHeap) \
	heapReset(psHeap)

#define HEAP_DESTROY(psHeap)  \
	heapDestroy(psHeap)

#endif

