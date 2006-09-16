/*
 * mem.c
 *
 * Replacements for malloc and free to track memory usage.
 *
 * Also allows pointer validity checking.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include "types.h"
#include "debug.h"
#include "mem.h"
#include "heap.h"
#include "treap.h"
#include "treapint.h"
#include "memint.h"
#include "block.h"

#include <assert.h>

/* Whether allocated memory is initialised to a value and whether the memory
 * is trashed before it is freed.
 * This is done automatically by Visual C's memory routines.
 */
#define MEMORY_SET		TRUE

/* Number of bytes after which memory amounts are displayed in Kb */
#define SHOW_KB_LIMIT	(0x400)

/* What functions to use for the real malloc and free */

#define RMALLOC		malloc
#define RFREE		free


/* The root of the memory treap */
static MEM_NODE		*psMemRoot = NULL;

/* The current block heap to use instead of MALLOC */
static BLOCK_HEAP	*psCurrBlockHeap;
void memMemoryDump(MEM_NODE *Node);

/* Initialise the memory system */
BOOL memInitialise(void)
{
	if (psMemRoot != NULL)
	{
		debug(LOG_MEMORY, "memInitialise: *** WARNING *** : memory still allocated??");
	}
	psMemRoot = NULL;

	psCurrBlockHeap = NULL;

	return TRUE;
}


/* Release the memory treap */
static void memTreapDestroy(TREAP_NODE *psRoot)
{
	if (psRoot)
	{
		// Destroy the sub trees
		memTreapDestroy(psRoot->psLeft);
		memTreapDestroy(psRoot->psRight);

		// Free the root
		RFREE(psRoot);
	}
}


/* Shutdown the memory system */
void memShutDown(void)
{
	// Report any memory still allocated
	memMemoryReport(NULL);

	// Free up the allocated memory
	memTreapDestroy((TREAP_NODE *)psMemRoot);
}


/* Set a block heap to use for all memory allocation rather than standard malloc/free */
void memSetBlockHeap(BLOCK_HEAP *psHeap)
{

	psCurrBlockHeap = psHeap;
}


/* Get the current block heap */
BLOCK_HEAP *memGetBlockHeap(void)
{
	return psCurrBlockHeap;
}


/* compare two memory blocks
 * NOTE: key1 is always the block passed into the treap code
 *       and therefore not necessarily to be trusted
 */
SDWORD	memBlockCmp(UDWORD	key1, UDWORD key2)
{
	UDWORD	start1,start2, end1,end2;

	// Calculate the edges of the memory blocks
	start1 = (UDWORD)((UBYTE *)((MEM_NODE *)key1)->pObj + sizeof(MEM_NODE)
						+ SAFETY_ZONE_SIZE);
	start2 = (UDWORD)((UBYTE *)((MEM_NODE *)key2)->pObj + sizeof(MEM_NODE)
						+ SAFETY_ZONE_SIZE);
	end1 = start1 + ((MEM_NODE *)key1)->size;
	end2 = start2 + ((MEM_NODE *)key2)->size;

	// see if one block is inside another
	if ((start1 >= start2 && end1 <= end2))// ||		// block 1 inside block 2
//		(start2 >= start1 && end2 <= end1))			// block 2 inside block 1
	{
		return 0;
	}
	else if (start1 < start2)
	{
		// less than
		return -1;
	}

	// greater than
	return 1;
}

#ifdef DEBUG_MALLOC

/* Replacement for malloc that records where the memory was requested.
 * All allocated memory is initialised to INITIALISE_BYTE
 * A buffer is also allocated at the top and bottom of the memory to check for
 * overwrites.
 */
void *memMalloc(const char *pFileName, SDWORD LineNumber, size_t Size)
{
	void		*pMemBase;
	MEM_NODE	*psNode;

	ASSERT( (pFileName != NULL), "No filename passed to mem_Malloc" );
	ASSERT( Size != 0, "Cannot allocate 0 bytes of memory." );

	if (psCurrBlockHeap != NULL)
	{
		// use a block heap rather than normal malloc
		blkCallPos(pFileName, LineNumber);
		return blkAlloc(psCurrBlockHeap, Size);
	}

	pMemBase = RMALLOC( Size + sizeof(MEM_NODE) + 2 * SAFETY_ZONE_SIZE );
	if (!pMemBase)
	{


		ASSERT( FALSE, "Warning: malloc returning NULL - [%s - %d]",pFileName,LineNumber );
		debug( LOG_NEVER, "[%s - %d] %d bytes\n", pFileName, LineNumber, Size );
		return NULL;
	}

	/* Got the main bit of memory - set up the node entry */
	psNode = (MEM_NODE *)pMemBase;
	psNode->pFile = (char *)RMALLOC( strlen(pFileName)+1 );
	if (!psNode->pFile)
	{
		RFREE(pMemBase);
		debug( LOG_NEVER, "Warning: malloc returning NULL" );
		return NULL;
	}
	strcpy(psNode->pFile, pFileName);
	psNode->line = LineNumber;
	psNode->size = Size;

	/* Store the new entry in the memory treap */
	psNode->priority = (UDWORD)rand();
	psNode->key = (UDWORD)psNode;
	psNode->pObj = psNode;
	psNode->psLeft = NULL;
	psNode->psRight = NULL;
	treapAddNode((TREAP_NODE **)&psMemRoot, (TREAP_NODE *)psNode, memBlockCmp);

	/* Now initialise the memory - try to catch unitialised variables */
	memset((UBYTE *)(pMemBase) + sizeof(MEM_NODE),
			SAFETY_ZONE_BYTE, SAFETY_ZONE_SIZE);
	memset((UBYTE *)(pMemBase) + sizeof(MEM_NODE) + SAFETY_ZONE_SIZE + Size,
			SAFETY_ZONE_BYTE, SAFETY_ZONE_SIZE);
#ifdef MEMORY_SET
	/* The PC initialises malloc'ed memory already, no need to do it again */
	memset((UBYTE *)(pMemBase) + sizeof(MEM_NODE) + SAFETY_ZONE_SIZE,
			INITIALISE_BYTE, Size);
#endif

	return ((UBYTE *)(pMemBase) + sizeof(MEM_NODE) + SAFETY_ZONE_SIZE);
}
#endif

/* Replacement for malloc for release builds. */
void *memMallocRelease(size_t Size)
{
	if (psCurrBlockHeap != NULL)
	{
		// use a block heap rather than normal malloc
		return blkAlloc(psCurrBlockHeap, Size);
	}

	return RMALLOC(Size);
}

#ifdef DEBUG_MALLOC

/* Replacement for free
 *
 * Checks whether the pointer is valid before freeing it.
 * If the pointer is invalid, it asserts.
 * The buffer zones around the allocated memory are also checked for
 * overwrites.
 * All memory is reset to FREE_BYTE before freeing to avoid using
 * freed memory.
 */
void memFree(const char *pFileName, SDWORD LineNumber, void *pMemToFree)
{
	MEM_NODE	sNode, *psDeleted;
	SDWORD		i, InvalidBottom, InvalidTop;
	UBYTE		*pMemBase;
	BLOCK_HEAP	*psBlock;
#ifdef MEMORY_SET
	SDWORD		Size;
#endif

	(void)LineNumber;
	(void)pFileName;


	ASSERT( (pFileName != NULL), "No filename passed to mem_Free" );
	ASSERT( (pMemToFree != NULL), "Attempt to free NULL pointer, called by:\n"
								  "File: %s\nLine: %d\n", pFileName, LineNumber );


	// see if the pointer was allocated in a block
	psBlock = blkFind(pMemToFree);
	if (psBlock != NULL)
	{
		// use a block heap rather than normal free
		blkCallPos(pFileName, LineNumber);
		blkFree(psBlock, pMemToFree);
		return;
	}

	// Create a dummy node for the search
	// This is only looked at by memBlockCmp so only need to set the object and size
	sNode.pObj = ((UBYTE *)pMemToFree) - sizeof(MEM_NODE) - SAFETY_ZONE_SIZE;
	sNode.size = 1;

	/* Get the node for the memory block */
	psDeleted = (MEM_NODE *)treapDelRec((TREAP_NODE **)&psMemRoot,
										(UDWORD)&sNode, memBlockCmp);


	ASSERT( psDeleted != NULL,
			"Invalid pointer passed to mem_Free by:\n"
			"File: %s\nLine: %d\n\n"
			"Attempt to free already freed pointer?",
			pFileName, LineNumber );
	if (psDeleted)
	{
		/* The pointer is valid, check the buffer zones */
		pMemBase = (UBYTE *)(pMemToFree) - SAFETY_ZONE_SIZE;
		InvalidBottom = InvalidTop = 0;
		for(i=0; i<SAFETY_ZONE_SIZE; i++)
		{
			if (pMemBase[i] != SAFETY_ZONE_BYTE)
			{
				InvalidBottom++;
			}
			if (pMemBase[i + psDeleted->size + SAFETY_ZONE_SIZE] != SAFETY_ZONE_BYTE)
			{
				InvalidTop++;
			}
		}

		ASSERT( !InvalidBottom && !InvalidTop,
				"Safety zone on memory overwritten.\n"
				"%d Invalid bytes (of %d) found below memory buffer.\n"
				"%d Invalid bytes (of %d) found above memory buffer.\n\n"
				"Memory allocated by:\nFile: %s\nLine: %d\n"
				"Memory freed by:\nFile: %s\nLine: %d\n",
				InvalidBottom, SAFETY_ZONE_SIZE, InvalidTop, SAFETY_ZONE_SIZE,
				psDeleted->pFile, psDeleted->line,
				pFileName, LineNumber );

		/* Trash the memory before it is freed (The PC already does this) */
#ifdef MEMORY_SET
		Size = psDeleted->size;
		memset(pMemToFree, FREE_BYTE, Size);
#endif

		/* Now free the memory */

		RFREE(psDeleted->pFile);
		RFREE(psDeleted);
	}

}
#endif

/* Replacement for Free for release builds */
void memFreeRelease(void *pMemToFree)
{
	BLOCK_HEAP	*psBlock;

	// see if the pointer was allocated in a block
	psBlock = blkFind(pMemToFree);
	if (psBlock != NULL)
	{
		// use a block heap rather than normal free
		blkFree(psBlock, pMemToFree);
		return;
	}


	RFREE(pMemToFree);
}


/* Checks whether the memory buffer pointed to by pPtr of size Size
 * is contained in any of the memory blocks allocated.
 */
BOOL memPointerValid(void *pPtr, size_t size)
{
	MEM_NODE	sNode;

	ASSERT( size, "memPointerValid: cannot check a pointer with zero size" );

	if (pPtr == NULL)
	{
		return FALSE;
	}

	// Create a dummy node for the search
	// This is only looked at by memBlockCmp so only need to set the object and size
	sNode.pObj = ((UBYTE *)pPtr) - sizeof(MEM_NODE) - SAFETY_ZONE_SIZE;
	sNode.size = size;

	// See if the block is in the treap
	if (treapFindRec((TREAP_NODE *)psMemRoot, (UDWORD)&sNode, memBlockCmp))
	{
		return TRUE;
	}

	// check the block heaps as well (if the code is there)
#ifdef DEBUG_BLOCK
	return blkPointerValidAll(pPtr, size);
#else
	return FALSE;
#endif
}


/* Recursive function to print out the list of memory blocks */
SDWORD memRecReport(MEM_NODE *psRoot)
{
#ifdef REALLY_DEBUG_MALLOC
	if (psRoot)
	{
		if (psRoot->size < SHOW_KB_LIMIT)
		{
			debug(LOG_MEMORY, "memRecReport for %s line %d: \t%d bytes",
			      psRoot->pFile, psRoot->line, psRoot->size);
		}
		else
		{
			debug(LOG_MEMORY, "memRecReport for %s line %d: \t%d kilobytes",
			      psRoot->pFile, psRoot->line, (int) psRoot->size / 1024);
		}

		return memRecReport((MEM_NODE *)psRoot->psLeft) +
			   memRecReport((MEM_NODE *)psRoot->psRight) +
			   psRoot->size;
	}
#endif
	return 0;
}

#ifdef DEBUG_MALLOC
#define MAXMODULES (32)
typedef struct
{
	char pFile[128];
	int Count;
	int Total;
} MEMMOD;
static MEMMOD MemModuleInfo[MAXMODULES];

static UDWORD MemTotalEntries;
static UDWORD MemTotalModules;
static UDWORD MemTotalAllocated;
#endif

/* Recursive function to total up the amount of mem allocated */
void memSummary(MEM_NODE *psRoot)
{
#ifdef DEBUG_MALLOC

						// bsort
	if (psRoot)
	{
	int i;
	BOOL FoundModule;

		MemTotalEntries++;
		MemTotalAllocated+=psRoot->size;

		FoundModule=FALSE;
		for (i=0;i<(SDWORD)MemTotalModules;i++)
		{
			if (strcmp(psRoot->pFile,MemModuleInfo[i].pFile)==0)
			{
				MemModuleInfo[i].Count++;
				MemModuleInfo[i].Total+=psRoot->size;
				FoundModule=TRUE;
			}
		}
		if (FoundModule==FALSE)
		{
			if (MemTotalModules <MAXMODULES)
			{
				strcpy(MemModuleInfo[MemTotalModules].pFile,psRoot->pFile);
				MemModuleInfo[MemTotalModules].Count=1;
				MemModuleInfo[MemTotalModules].Total=psRoot->size;
				MemTotalModules++;
			}

		}



		memSummary((MEM_NODE *)psRoot->psLeft);
		memSummary((MEM_NODE *)psRoot->psRight);
	}
	return ;
#endif
}

void memMemorySummary(void)
{
	memMemoryDump(psMemRoot);
}

void memMemoryDump(MEM_NODE *Node)
{
#ifdef DEBUG_MALLOC

	int i;

	MemTotalEntries=0;
	MemTotalModules=0;
	MemTotalAllocated=0;
	memSummary(Node);

	debug(LOG_MEMORY, "Memory Summary: %d bytes allocated in %d handy size chunks", MemTotalAllocated, MemTotalEntries);
	for (i=0;i<(SDWORD)MemTotalModules;i++)
	{
		debug(LOG_MEMORY, "memMemoryDump: %d) [%s] %d allocations totalling %d bytes",
		      i, MemModuleInfo[i].pFile, MemModuleInfo[i].Count, MemModuleInfo[i].Total);
	}
#endif

}

/* Report on currently allocated memory.
 * If pFileName is not NULL send the report to the specified file.
 * If pFileName is NULL the report goes to DBPRINTF
 */
void memMemoryReport(const char *pFileName)
{
#ifdef DEBUG_MALLOC
	SDWORD		TotMem;

	if (!psMemRoot)
	{
		debug(LOG_MEMORY, "memMemoryReport: No memory allocated");
	}
	else
	{
		TotMem = memRecReport(psMemRoot);

		if (TotMem < SHOW_KB_LIMIT)
		{
			debug(LOG_MEMORY, "memMemoryReport: Total memory allocated is %d bytes", TotMem);
		}
		else
		{
			debug(LOG_MEMORY, "memMemoryReport: Total memory allocated is %d kilobytes", (int) TotMem / 1024);
		}

	}
#endif
}


/* Display the memory treap */
void memDisplayTreap(const char *pFileName)
{
#ifdef DEBUG_MALLOC
	debug(LOG_MEMORY, "Memory Allocation Treap:");
	treapDisplayRec((TREAP_NODE *)psMemRoot, 0);
#endif
}
