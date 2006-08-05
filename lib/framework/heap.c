/*
 * Heap.c
 *
 * Memory routines for managing groups of the same sized objects.
 *
 */

#include <string.h>
#include <stdio.h>
#include <assert.h>

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include "types.h"
#include "debug.h"
#include "mem.h"
#include "heap.h"
#include "treap.h"
#include "treapint.h"
#include "block.h"

// Control whether a heap usage report is printed out when a heap is destroyed
#define HEAP_USAGE_REPORT	FALSE

// Control whether a copy for the filename string is taken in DEBUG mode
#define COPY_FILE_STRING	FALSE

/* Store the call position */
static STRING	*pCFile;
static SDWORD	cPos;


#if DEBUG_HEAP
#define MAXDEBUGHEAPS (32)
static OBJ_HEAP * HeapDebugList[MAXDEBUGHEAPS];
#endif

/* Store the location in C code at which a call to the heap was made */
void heapSetCallPos(STRING *pFileName, SDWORD lineNumber)
{
	cPos = lineNumber;

#if COPY_FILE_STRING
	pCFile = (STRING *)MALLOC(strlen(pFileName) + 1);
	if (!pCFile)
	{
		return;
	}
	strcpy(pCFile, pFileName);
#else
	pCFile = pFileName;
#endif
}


/* Create the free list for a heap */
static void heapCreateFreeList(OBJ_HEAP *psHeap)
{
	UDWORD		size = psHeap->objSize;
	UDWORD		ext = psHeap->extAlloc;
	UDWORD		i, extSize;
	FREE_OBJECT	*psCurr = NULL;
	UBYTE		*pBase;
	HEAP_EXTENSION	*psExt;


	// Set up the main memory block
#if DEBUG_HEAP
	UDWORD          init = psHeap->initAlloc;

	// Initialise the memory to a fixed value to check for memory overwrites
	memset(psHeap->pMemory, FREE_BYTE, size * init);
#endif
	psHeap->psFree = (FREE_OBJECT *)(psHeap->pMemory);
	pBase = psHeap->pMemory;
	for(i=psHeap->initAlloc; i>0; i--)
	{
		psCurr = (FREE_OBJECT *)(pBase);
		psCurr->psNext = (FREE_OBJECT *)(pBase + size);
		pBase += size;
	}

	// Set up the extension blocks if any
	extSize = size * ext;
	for(psExt = psHeap->psExt; psExt; psExt = psExt->psNext)
	{
#if DEBUG_HEAP
		// Initialise the memory to check for overwrites
		memset(psExt->pMemory, FREE_BYTE, extSize);
#endif

		// Link this list to the end of the free list
		psCurr->psNext = (FREE_OBJECT *)(psExt->pMemory);

		// Now create the free object list
		pBase = psExt->pMemory;
		for(i=psHeap->extAlloc; i>0; i--)
		{
			psCurr = (FREE_OBJECT *)(pBase);
			psCurr->psNext = (FREE_OBJECT *)(pBase + size);
			pBase += size;
		}
	}

	// Terminate the free list
	psCurr->psNext = NULL;
}


/* Function to create a heap
 * Takes the size of the objects to be managed by the heap,
 * the initial number of objects to allocate and the number of
 * objects to allocate when the heap is extended.
 * Returns an initialised OBJ_HEAP structure.
 */
BOOL heapCreate(OBJ_HEAP **ppsHeap, UDWORD size, UDWORD init, UDWORD ext)
{
/*	UDWORD		i;
	FREE_OBJECT	*psCurr;
	UBYTE		*pBase;*/

#if DEBUG_HEAP
	int			Heap;
#endif
	ASSERT((size >= sizeof(FREE_OBJECT),
		"heapCreate: object is too small to be stored in free list"));

#if DEBUG_HEAP
	/* Increase the object size to store the call position */
	size += sizeof(HEAP_OBJHDR);

#endif

	/* Allocate the heap object and its memory */
	*ppsHeap = (OBJ_HEAP *)MALLOC(sizeof(OBJ_HEAP));
	if (*ppsHeap == NULL)
	{
		DBERROR(("heapCreate: Out of memory"));
		return FALSE;
	}
//	memset(*ppsHeap,0,sizeof(OBJ_HEAP));			//setting everything to 0 first (debug test)-Q
	(*ppsHeap)->pMemory = (UBYTE *)MALLOC(size * init);

/*
	if (PTRVALID((*ppsHeap)->pMemory,size*init)==FALSE)
	{
		DBPRINTF(("Allocated heap memory is not valid!\n"));
	}
	else
	{
		DBPRINTF(("valid\n"));
	}
*/

	if ((*ppsHeap)->pMemory == NULL)
	{
		DBERROR(("heapCreate: Out of memory"));
		return FALSE;
	}

	/* Initialise the object */
	(*ppsHeap)->objSize = size;
	(*ppsHeap)->initAlloc = init;
	(*ppsHeap)->extAlloc = ext;
	(*ppsHeap)->psExt = NULL;
	(*ppsHeap)->psBlkHeap = memGetBlockHeap();
#if DEBUG_HEAP
	(*ppsHeap)->maxUsage = 0;
	(*ppsHeap)->currUsage = 0;
	(*ppsHeap)->pFile = pCFile;
	(*ppsHeap)->line = cPos;
	(*ppsHeap)->psInUse = NULL;

	// Keep a list of all the heaps allocated
	for (Heap=0;Heap<MAXDEBUGHEAPS;Heap++)
	{
		if (HeapDebugList[Heap] == NULL)	// found an empty one
		{
			HeapDebugList[Heap]=*ppsHeap;
			break;
		}
	}
	if (HeapDebugList[Heap]!=*ppsHeap)
	{
		DBERROR(("heapCreate: MAXDEBUGHEAPS too small"));
	}

#endif

	// Now create the free object list
	heapCreateFreeList(*ppsHeap);
/*
	if (PTRVALID((*ppsHeap)->pMemory,10)==FALSE)
	{
		DBPRINTF(("Allocated heap memory is not valid!\n"));
	}
	else
	{
		DBPRINTF(("valid\n"));
	}
*/
	/*
#if DEBUG_MALLOC
	// Initialise the memory to a fixed value to check for memory overwrites
	memset((*ppsHeap)->pMemory, FREE_BYTE, size * init);
#endif

	// Now create the free object list
	(*ppsHeap)->psFree = (FREE_OBJECT *)((*ppsHeap)->pMemory);
	pBase = (*ppsHeap)->pMemory;
	for(i=0; i<init; i++)
	{
		psCurr = (FREE_OBJECT *)(pBase + i*size);
		psCurr->psNext = (FREE_OBJECT *)((UBYTE *)psCurr + size);
	}
	psCurr->psNext = NULL;
	*/

	return TRUE;
}

/* Allocate an object from a heap
 * Returns a pointer to the object if successful
 */
BOOL heapAlloc(OBJ_HEAP *psHeap, void **ppObject)
{
	HEAP_EXTENSION		*psNew;
	UDWORD		i;
	FREE_OBJECT	*psCurr = NULL;
	UBYTE		*pBase;
	BLOCK_HEAP	*psCurrBlk;
#if DEBUG_HEAP
	HEAP_OBJHDR	*psHdr;
#endif

	ASSERT((PTRVALID(psHeap, sizeof(OBJ_HEAP)),
		"heapAlloc: Invalid heap pointer"));

	if (psHeap->psFree == NULL)
	{
		if (psHeap->extAlloc == 0)
		{
			// heap doesn't expand
#if DEBUG_HEAP && COPY_FILE_STRING
			FREE(pCFile);
#endif
			return FALSE;
		}

		/* No objects left - need to add a heap extension */
		psCurrBlk = memGetBlockHeap();
		memSetBlockHeap(psHeap->psBlkHeap);

#ifdef REALLY_DEBUG_HEAP
		debug(LOG_MEMORY, "heapAlloc: Heap %s, line %d extended. Max use: %d\n", psHeap->pFile, psHeap->line, psHeap->maxUsage);
#endif

		psNew = (HEAP_EXTENSION *)MALLOC(sizeof(HEAP_EXTENSION));
		if (psNew == NULL)
		{

			/* Out of memory */
			return FALSE;
		}
		psNew->pMemory = (UBYTE *)MALLOC(psHeap->objSize * psHeap->extAlloc);
		if (psNew->pMemory == NULL)
		{

			/* Out of memory */
			FREE(psNew);
			return FALSE;
		}
		memSetBlockHeap(psCurrBlk);

#if DEBUG_HEAP
		/* Initialise the memory to check for overwrites */
		memset(psNew->pMemory, FREE_BYTE, psHeap->objSize * psHeap->extAlloc);
#endif

		/* Add the extension to the list */
		psNew->psNext = psHeap->psExt;
		psHeap->psExt = psNew;

		/* Now create the free object list */
		psHeap->psFree = (FREE_OBJECT *)(psNew->pMemory);
		pBase = psNew->pMemory;
		for(i=0; i<psHeap->extAlloc; i++)
		{
			psCurr = (FREE_OBJECT *)(pBase + i*psHeap->objSize);
			psCurr->psNext = (FREE_OBJECT *)((UBYTE *)psCurr + psHeap->objSize);
		}
		psCurr->psNext = NULL;
	}

	/* Return the object and update the free list */
	*ppObject = psHeap->psFree;
	psHeap->psFree = psHeap->psFree->psNext;

#if DEBUG_HEAP
	/* Update the usage counts */
	psHeap->currUsage += 1;
	if (psHeap->currUsage > psHeap->maxUsage)
	{
		psHeap->maxUsage = psHeap->currUsage;
	}

	/* Check the object we're giving out hasn't been scribbled over by anything */
	if (psHeap->objSize > sizeof(FREE_OBJECT))
	{
		for(pBase = (UBYTE *)*ppObject + sizeof(FREE_OBJECT);
			pBase < (UBYTE *)*ppObject + psHeap->objSize;
			pBase++)
		{
			if (*pBase != FREE_BYTE)
			{
				break;
			}
		}
		ASSERT((pBase == (UBYTE *)*ppObject + psHeap->objSize,
			"heapAlloc: unallocated object memory has been overwritten"));
	}

	/* Store the call position */
	psHdr = (HEAP_OBJHDR *)*ppObject;
	psHdr->pFile = pCFile;
	psHdr->line = cPos;
	*ppObject = ((UBYTE *)*ppObject) + sizeof(HEAP_OBJHDR);

	/* Add the object to the in use list */
	psHdr->psNext = psHeap->psInUse;
	psHeap->psInUse = psHdr;
#endif

	return TRUE;
}

/* Return an object to the heap */
BOOL heapFree(OBJ_HEAP *psHeap, void *pObject)
{
#if DEBUG_HEAP
	BOOL			found;
	HEAP_OBJHDR		*psHdr, *psCurrHdr, *psPrevHdr;
#endif
	FREE_OBJECT		*psFree;

	ASSERT((PTRVALID(psHeap, sizeof(OBJ_HEAP)),
		"heapFree: Invalid heap pointer"));

#if DEBUG_HEAP
	/* Adjust the pointer to include the call position */
	pObject = ((UBYTE *)pObject) - sizeof(HEAP_OBJHDR);
	/* Free the string memory for the file name */
	psHdr = (HEAP_OBJHDR *)pObject;
#if COPY_FILE_STRING
	if (psHdr->pFile)
	{
		FREE(psHdr->pFile);
	}
#endif

	/* Remove the object from the in use list */
	found = FALSE;
	if (psHeap->psInUse == psHdr)
	{
		psHeap->psInUse = psHdr->psNext;
		found = TRUE;
	}
	else
	{
		for(psCurrHdr=psHeap->psInUse; psCurrHdr; psCurrHdr = psCurrHdr->psNext)
		{
			if (psCurrHdr == psHdr)
			{
				psPrevHdr->psNext = psCurrHdr->psNext;
				found = TRUE;
				break;
			}
			psPrevHdr = psCurrHdr;
		}
	}
	ASSERT((found, "heapFree: object not allocated on this heap"));

	/* Check the object hasn't been freed already */
	found = FALSE;
	for(psFree = psHeap->psFree; psFree != NULL; psFree = psFree->psNext)
	{
		if (pObject == (void *)psFree)
		{
			found = TRUE;
		}
	}
	ASSERT((!found, "heapFree: Object has already been freed"));

	/* On the debug build we'll trash the memory as well - just to be sure */
	memset(pObject, FREE_BYTE, psHeap->objSize);

	/* Update the usage counter */
	psHeap->currUsage -= 1;
#endif

	psFree = (FREE_OBJECT *)pObject;
	psFree->psNext = psHeap->psFree;
	psHeap->psFree = psFree;

	return TRUE;
}


/* Reset the heap, i.e. free all the objects in the heap */
void heapReset(OBJ_HEAP *psHeap)
{
	/* Initialise the object */
#if DEBUG_HEAP
	psHeap->currUsage = 0;
	psHeap->psInUse = NULL;
#endif

	// Now create the free object list
	heapCreateFreeList(psHeap);
}


/* Destroy a heap and release all the memory associated with it */
void heapDestroy(OBJ_HEAP *psHeap)
{
	HEAP_EXTENSION	*psExt, *psNext;
#if DEBUG_HEAP
	FREE_OBJECT		*psFree;
	UBYTE			*pStart, *pEnd;
	BOOL			clean;
	HEAP_OBJHDR		*psCurrHdr;
	int 			Heap;
#endif

	ASSERT((PTRVALID(psHeap, sizeof(OBJ_HEAP)),
		"heapDestroy: invalid heap pointer"));

#if DEBUG_HEAP
	/* Warn about any unfreed objects */
	if (psHeap->currUsage > 0)
	{
		DBPRINTF(("heapDestroy: %s, line %d : %d objects in use\n",
				psHeap->pFile, psHeap->line, psHeap->currUsage));
	}

	/* Print out where the unfreed objects were allocated */
	for(psCurrHdr = psHeap->psInUse; psCurrHdr; psCurrHdr=psCurrHdr->psNext)
	{
		debug(LOG_MEMORY, "heapDestroy unfreed: %s, line %d",
		      psCurrHdr->pFile, psCurrHdr->line);
#if COPY_FILE_STRING
		if (psCurrHdr->pFile)
		{
			FREE(psCurrHdr->pFile);
		}
#endif
	}

	/* Check for any memory overwrites on the free objects */
	clean = TRUE;
	for(psFree = psHeap->psFree; psFree != NULL; psFree = psFree->psNext)
	{
		pEnd = (UBYTE *)psFree + psHeap->objSize;
		for(pStart = (UBYTE *)psFree + sizeof(FREE_OBJECT); pStart<pEnd; pStart++)
		{
			if (*pStart != FREE_BYTE)
			{
				clean = FALSE;
			}
		}
	}
	ASSERT((clean, "heapDestroy: unallocated memory has been overwritten"));

#if HEAP_USAGE_REPORT
	DBPRINTF(("heapDestory: %s, line %d : Max usage %d (Init %d Ext %d)\n",
			psHeap->pFile, psHeap->line, psHeap->maxUsage,
			psHeap->initAlloc, psHeap->extAlloc));
#endif

#if COPY_FILE_STRING
	if (psHeap->pFile)
	{
		FREE(psHeap->pFile);
	}
#endif

	// Find the heap
	for (Heap=0;Heap<MAXDEBUGHEAPS;Heap++)
	{
		if (HeapDebugList[Heap] == psHeap)	// found an match
		{
			HeapDebugList[Heap]=NULL;
			break;
		}
	}

#endif

	/* Deallocate all the heap's memory */
	for(psExt = psHeap->psExt; psExt != NULL; psExt = psNext)
	{
		psNext = psExt->psNext;
		FREE(psExt->pMemory);
		FREE(psExt);
	}
	FREE(psHeap->pMemory);
	FREE(psHeap);
}


BOOL heapIntegrityCheck(OBJ_HEAP *psHeap)
{
#if DEBUG_HEAP
	FREE_OBJECT		*psCurr;
	UBYTE			*pBase;

	if (psHeap->objSize < sizeof(FREE_OBJECT))
	{
		// too small to check
		return TRUE;
	}

	for(psCurr = psHeap->psFree; psCurr; psCurr=psCurr->psNext)
	{
		// Check each object hasn't been scribbled over by anything
		for(pBase = (UBYTE *)psCurr + sizeof(FREE_OBJECT);
			pBase < (UBYTE *)psCurr + psHeap->objSize;
			pBase++)
		{
			if (*pBase != FREE_BYTE)
			{
				break;
			}
		}
		ASSERT((pBase == (UBYTE *)psCurr + psHeap->objSize,
			"heapIntegrityCheck: unallocated object memory has been overwritten"));
	}
#else
	psHeap = psHeap;
#endif
	return TRUE;
}


void heapReport(void)
{
#if DEBUG_HEAP

	int Heap;

	OBJ_HEAP *psHeap;

	DBPRINTF(("\nheapReport\n==========\n"));

	for (Heap=0;Heap<MAXDEBUGHEAPS;Heap++)
	{
		if (HeapDebugList[Heap] != NULL)
		{
			psHeap=HeapDebugList[Heap];
			DBPRINTF(("Heap: %s, line %d size=%d mem taken=%d\n",
				psHeap->pFile, psHeap->line, psHeap->objSize, psHeap->objSize*psHeap->initAlloc));
			
			DBPRINTF((" Current Usage=%d ",psHeap->currUsage));
			
			DBPRINTF((" Max usage %d (Init %d Ext %d)\n",
				psHeap->maxUsage,
				psHeap->initAlloc, psHeap->extAlloc));

		}
	}

#else
#endif

}
