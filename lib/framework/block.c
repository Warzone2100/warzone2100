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
/*
 * Block.c
 *
 * Routines to allocate memory from one large block.
 * Any memory allocated is only available to reallocated after
 * the whole block has been reset.
 *
 */

#include <string.h>

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

// memory usage printf's
#define DEBUG_GROUP0

#include "types.h"
#include "debug.h"
#include "mem.h"
#include "heap.h"
#include "treap.h"
#include "treapint.h"
#include "memint.h"
#include "listmacs.h"

#include "block.h"

/* What functions to use for the real malloc and free */

#define RMALLOC		malloc
#define RFREE		free


/* Whether allocated memory is initialised to a value and whether the memory
 * is trashed before it is freed.
 * This is done automatically by Visual C's memory routines.
 */
#define MEMORY_SET		TRUE

// the filename and line number of the last call to the block functions
static const char *pCallFileName;
static SDWORD	callLine;

// the list of allocated blocks
static BLOCK_HEAP	*psBlockList=NULL;

// initialise the block system
BOOL blkInitialise(void)
{
	ASSERT( psBlockList==NULL, "blkInitialise: Blocks already initialised" );	// blkShutDown() needs to be called

	LIST_INIT(psBlockList);

	return TRUE;
}

// shutdown the block system
void blkShutDown(void)
{
	BLOCK_HEAP	*psNext;

	if (psBlockList)
	{
		debug( LOG_NEVER, "blkShutDown: blocks still allocated:\n" );
		while (psBlockList)
		{
			psNext = psBlockList->psNext;
			blkDestroy(psBlockList);
			psBlockList = psNext;
		}
	}
}

// Note the call position for a blkAlloc or blkFree
void blkCallPos(const char *pFileName, SDWORD line)
{
	pCallFileName = pFileName;
	callLine = line;
}

// Create a new block heap
BOOL blkCreate(BLOCK_HEAP **ppsHeap, SDWORD init, SDWORD ext)
{

	debug( LOG_NEVER, "BLKCREATE CALLED !!!!!!!!!!!!!!!!!!!!!!\n" );
	*ppsHeap = (BLOCK_HEAP*)RMALLOC(sizeof(BLOCK_HEAP));
	if (!*ppsHeap)
	{
		debug( LOG_ERROR, "blkCreate: Out of memory" );
		abort();
		return FALSE;
	}
	(*ppsHeap)->psBlocks = (BLOCK_HEAP_MEM*)RMALLOC(sizeof(BLOCK_HEAP_MEM));
	if (!(*ppsHeap)->psBlocks)
	{
		debug( LOG_ERROR, "blkCreate: Out of memory" );
		abort();
		return FALSE;
	}
	(*ppsHeap)->psBlocks->pMem = (UBYTE*)RMALLOC(init);
	if (!(*ppsHeap)->psBlocks->pMem)
	{

		debug( LOG_ERROR, "blkCreate: Out of memory" );
		abort();
		return FALSE;
	}

	(*ppsHeap)->init = init;
	(*ppsHeap)->ext = ext;
	(*ppsHeap)->psBlocks->size = init;
	(*ppsHeap)->psBlocks->pFree = (*ppsHeap)->psBlocks->pMem;
	(*ppsHeap)->psBlocks->psNext = NULL;

#ifdef DEBUG_BLOCK
	(*ppsHeap)->pFileName = pCallFileName;
	(*ppsHeap)->line = callLine;
	(*ppsHeap)->psMemTreap = NULL;
	(*ppsHeap)->free = FALSE;
	(*ppsHeap)->TotalAllocated=0;
#endif

	LIST_ADD(psBlockList, *ppsHeap);

	return TRUE;
}

// Release a block heap
void blkDestroy(BLOCK_HEAP *psHeap)
{
	BLOCK_HEAP_MEM	*psCurr, *psNext;

#ifdef DEBUG_BLOCK
	if (psHeap->psMemTreap != NULL)
	{
		debug( LOG_NEVER, "blkDestroy: %s at %d: memory allocated :\n", psHeap->pFileName, psHeap->line );
#ifdef REALLY_DEBUG_MALLOC
		memRecReport(psHeap->psMemTreap);
#endif
	}
#endif

	LIST_REMOVE(psBlockList, psHeap, BLOCK_HEAP);

	for(psCurr=psHeap->psBlocks; psCurr; psCurr=psNext)
	{
		RFREE(psCurr->pMem);
		psNext = psCurr->psNext;
		RFREE(psCurr);
	}
	RFREE(psHeap);
}

void memMemoryDump(MEM_NODE *Node);


void blkPrintDetails(BLOCK_HEAP *psHeap)
{
	if (psHeap!=NULL)
	{

#ifdef DEBUG_BLOCK
		UDWORD Left = (UDWORD)((psHeap->psBlocks->pMem)+(psHeap->psBlocks->size)-(psHeap->psBlocks->pFree));
		debug( LOG_NEVER, "ptr=%p init=%d ext=%d used=%d (Start=$%p Free=$%p Left=%d)\n", psHeap,psHeap->init, psHeap->ext,psHeap->TotalAllocated, psHeap->psBlocks->pMem, psHeap->psBlocks->pFree, Left );
		memMemoryDump(psHeap->psMemTreap);
#else
		debug( LOG_NEVER, "ptr=%p init=%d ext=%d\n", psHeap, psHeap->init, psHeap->ext );
#endif

	}
	else
	{
		debug( LOG_NEVER, "NULL POINTER IN BLOCK LIST\n" );
	}

}

// report on the blocks
void blkReport(void)
{



#ifdef DEBUG
	UDWORD BlockNumber=0;
	BLOCK_HEAP *psCurHeap;

	debug( LOG_NEVER, "\n\nBlock Report. Current Block=%p:\n", memGetBlockHeap() );

	psCurHeap=psBlockList;

	while (psCurHeap)
	{
		debug( LOG_NEVER, "Block %d)",BlockNumber++ );
		blkPrintDetails(psCurHeap);

		psCurHeap = psCurHeap->psNext;
	}
	debug( LOG_NEVER, "\n\n" );
#endif
}

#if(0)	 // no longer used - uploaded in small chunks
// This is a special free that checks to see if we can free up the memory
// We can only do this if it is the last allocated memory in the block
//
// This can be used for scratch memory ... Critical on the Playstation where memory is so tight
//
//  - e.g.   The sound data must be stored as one 400k file. This must be loaded into scratch memory
//             we clearly do not have 400k of spare memory around.
//
//
//  Returns true or false
//
BOOL blkSpecialFree(BLOCK_HEAP *psHeap, void *Ptr)
{
	BLOCK_HEAP_MEM	*psCurr;

	UDWORD RequestedFreeMem=(UDWORD)Ptr;

	for(psCurr = psHeap->psBlocks; psCurr; psCurr = psCurr->psNext)
	{

		if ((UDWORD)psCurr->pLastAllocated == RequestedFreeMem)
		{
#ifdef DEBUG_BLOCK

			UDWORD BlockSize=((UDWORD)psCurr->pFree)-RequestedFreeMem;
			debug( LOG_NEVER, "FREED %d block bytes\n", BlockSize ); // del me now !
			psHeap->TotalAllocated-=BlockSize;
#endif

			psCurr->pFree = psCurr->pLastAllocated;
			psCurr->pLastAllocated=0;	// remove pointer so that it cant be allocated again


			return(TRUE);				// able to return mem
		}
	}
	return(FALSE);			// unable to free mem

}
#endif

// Allocate some memory from a block heap
void *blkAlloc(BLOCK_HEAP *psHeap, SDWORD size)
{
	void			*pAlloc;
	BLOCK_HEAP_MEM	*psCurr, *psNew;

#ifdef DEBUG_BLOCK
	SDWORD			allocSize;
	MEM_NODE		*psNode;
#endif



	// Round up to nearest 4 bytes ( 32 bit align ).. Neaded for Playstation.. PD.
	size = (size + 3) & 0xfffffffc;


	// can't allocate 0 bytes
	if (size <= 0)
	{
		ASSERT( FALSE, "blkAlloc: cannot allocate 0 bytes" );
		return NULL;
	}

#ifdef DEBUG_BLOCK
	// see if free has been called for this block
	if (psHeap->free)
	{
		debug( LOG_NEVER, "Block Heap: %s at %d: Alloc after free:\n  free %s at %d\n  alloc %s at %d\n", psHeap->pFileName, psHeap->line, psHeap->pFreeFile, psHeap->freeLine, pCallFileName, callLine );
		psHeap->free = FALSE;
	}

	// increase the size of the block to allow for the treap entry and overwrite blocks
	allocSize = size;
	size += sizeof(MEM_NODE) + 2 * SAFETY_ZONE_SIZE;
	psHeap->TotalAllocated+=allocSize;
#endif

	// find a block with a large enough segment free
	pAlloc = NULL;
	for(psCurr = psHeap->psBlocks; psCurr; psCurr = psCurr->psNext)
	{
		if (psCurr->pFree + size <= psCurr->pMem + psCurr->size)
		{
			pAlloc = psCurr->pFree;

			psCurr->pFree += size;
			break;
		}
	}

	// if there wasn't a block try to allocate a new one
	if ((psCurr == NULL) && (psHeap->ext != 0))
	{
		psNew = (BLOCK_HEAP_MEM*)RMALLOC(sizeof(BLOCK_HEAP_MEM));
		if (!psNew)
		{
			ASSERT( FALSE, "blkAlloc: warning out of memory" );

			// Out of memory
			return NULL;

		}
		if (size < psHeap->ext)
		{
			psNew->pMem = (UBYTE*)RMALLOC(psHeap->ext);
			psNew->size = psHeap->ext;
		}
		else
		{
			psNew->pMem = (UBYTE*)RMALLOC(size);
			psNew->size = size;
		}
		if (!psNew->pMem)
		{
			// Out of memory
			RFREE(psNew);
			ASSERT( FALSE, "blkAlloc: warning out of memory" );
			return NULL;
		}
		psNew->psNext = NULL;

		psNew->pFree = psNew->pMem + size;
		pAlloc = psNew->pMem;

		// Add the block to the end of the list
		for(psCurr=psHeap->psBlocks; psCurr->psNext != NULL; psCurr = psCurr->psNext)
			;
		psCurr->psNext = psNew;
	}

#ifdef DEBUG_BLOCK
	if (!pAlloc)
	{
		// failed to allocate the memory

		ASSERT( FALSE, "Warning: malloc returning NULL - [%s - %d] %d bytes",pCallFileName,callLine, size );
		return NULL;
	}

	// add the allocated memory into the treap
	psNode = (MEM_NODE *)pAlloc;
	psNode->pFile = pCallFileName;
	psNode->line = callLine;
	psNode->size = allocSize;

	/* Store the new entry in the memory treap */
	psNode->priority = (UDWORD)rand();
	psNode->key = (void*)psNode;
	psNode->pObj = psNode;
	psNode->psLeft = NULL;
	psNode->psRight = NULL;
	treapAddNode((TREAP_NODE **)&psHeap->psMemTreap, (TREAP_NODE *)psNode, memBlockCmp);

 	/* Now initialise the memory - try to catch unitialised variables */
	memset((UBYTE *)(psNode) + sizeof(MEM_NODE),
			SAFETY_ZONE_BYTE, SAFETY_ZONE_SIZE);
	memset((UBYTE *)(psNode) + sizeof(MEM_NODE) + SAFETY_ZONE_SIZE + allocSize,
			SAFETY_ZONE_BYTE, SAFETY_ZONE_SIZE);
#if MEMORY_SET
	/* The PC initialises malloc'ed memory already, no need to do it again */
	memset((UBYTE *)(psNode) + sizeof(MEM_NODE) + SAFETY_ZONE_SIZE,
			INITIALISE_BYTE, allocSize);
#endif

	// offset the return value by the header size
	pAlloc = ((UBYTE *)(pAlloc) + sizeof(MEM_NODE) + SAFETY_ZONE_SIZE);
#endif

	psCurr->pLastAllocated=(UBYTE*)pAlloc;
///* - error trapping an out-of-mem allocation !!!

//NoMemChk:
	return pAlloc;
}
//*/

// return a chunk of memory to the block
// this only does anything whith DEBUG_BLOCK defined
void blkFree(BLOCK_HEAP *psHeap, void *pMemToFree)
{
#ifdef DEBUG_BLOCK
	MEM_NODE	sNode, *psDeleted;
	SDWORD		i, InvalidBottom, InvalidTop;
	UBYTE		*pMemBase;
#if MEMORY_SET
	SDWORD		Size;
#endif


	// note the call to free
	psHeap->free = TRUE;
	psHeap->pFreeFile = pCallFileName;
	psHeap->freeLine = callLine;

	// Create a dummy node for the search
	// This is only looked at by memBlockCmp so only need to set the object and size
	sNode.pObj = ((UBYTE *)pMemToFree) - sizeof(MEM_NODE) - SAFETY_ZONE_SIZE;
	sNode.size = 1;

	/* Get the node for the memory block */
	psDeleted = (MEM_NODE *)treapDelRec((TREAP_NODE **)&psHeap->psMemTreap,
										(void*)&sNode, memBlockCmp);
	ASSERT( psDeleted != NULL,
			"Invalid pointer passed to mem_Free by:\n"
			"File: %s\nLine: %d\n\n"
			"Attempt to free already freed pointer?",
			pCallFileName, callLine );
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
// this breaks debug...  ***** look into why...
//		ASSERT( !InvalidBottom && !InvalidTop,
//				"Safety zone on memory overwritten.\n"
//				"%d Invalid bytes (of %d) found below memory buffer.\n"
//				"%d Invalid bytes (of %d) found above memory buffer.\n\n"
//				"Memory allocated by:\nFile: %s\nLine: %d\n"
//				"Memory freed by:\nFile: %s\nLine: %d\n",
//				InvalidBottom, SAFETY_ZONE_SIZE, InvalidTop, SAFETY_ZONE_SIZE,
//				psDeleted->pFile, psDeleted->line,
//				pCallFileName, callLine );

		/* Trash the memory before it is freed */
#if MEMORY_SET
		Size = psDeleted->size;
		memset(pMemToFree, FREE_BYTE, Size);
#endif
	}

#else
// non debug free !
//	psHeap = psHeap;
//	pMemToFree = pMemToFree;
#endif

	{
//		BOOL bRes;

#if(1)

		//DBPRINTF(("UNABLE TO FREE MEMORY\n"));


#else
		bRes=blkSpecialFree(psHeap, pMemToFree);	// Free it up if we can ! - we can only free the last entry
#ifdef DEBUG
		if (bRes==TRUE)
		{
			debug( LOG_NEVER, "blkFree called - memory successfully released\n" );
		}
		else
		{
//			debug( LOG_NEVER, "blkFree called - memory NOT released\n" );
		}
#endif
#endif


	}

}




// Reset a block heap
void blkReset(BLOCK_HEAP *psHeap)
{
	BLOCK_HEAP_MEM	*psCurr;
#ifdef DEBUG_GROUP0
	SDWORD		block=0, alloc=0;
#endif

#ifdef DEBUG_BLOCK
	if (psHeap->psMemTreap != NULL)
	{
		debug( LOG_NEVER, "blkReset: %s at %d: memory allocated :\n", psHeap->pFileName, psHeap->line );
#ifdef REALLY_DEBUG_MALLOC
		memRecReport(psHeap->psMemTreap);
#endif
	}
	psHeap->psMemTreap = NULL;
	psHeap->free = FALSE;

	psHeap->TotalAllocated=0;

	debug( LOG_NEVER, "blkReset: %s at %d: memory usage:\n", psHeap->pFileName, psHeap->line );
#else
	debug( LOG_NEVER, "blkReset: memory usage:\n" );
#endif

	psCurr = psHeap->psBlocks;
	while(psCurr)
	{
#ifdef DEBUG_GROUP0
		alloc += psCurr->pFree - psCurr->pMem;
		block += psCurr->size;
#endif
#if defined(DEBUG_BLOCK) && MEMORY_SET
		memset(psCurr->pMem, FREE_BYTE, psCurr->size);
#endif
		psCurr->pFree = psCurr->pMem;
		psCurr = psCurr->psNext;
	}

	debug( LOG_NEVER, "    Blocks allocated %dk, Memory allocated %dk\n", block/1024, alloc/1024 );
}


// Find which block a pointer is from if any
BLOCK_HEAP *blkFind(void *pPtr)
{
	BLOCK_HEAP		*psHeap;
	BLOCK_HEAP_MEM	*psMem;

	for(psHeap=psBlockList; psHeap; psHeap=psHeap->psNext)
	{
		for(psMem=psHeap->psBlocks; psMem; psMem=psMem->psNext)
		{
			if ((UBYTE *)pPtr >= psMem->pMem &&
				(UBYTE *)pPtr < psMem->pMem + psMem->size)
			{
				return psHeap;
			}
		}
	}

	return NULL;
}


// check if a pointer is valid in a block
BOOL blkPointerValid(BLOCK_HEAP *psHeap, void *pData, SDWORD size)
{
#ifdef DEBUG_BLOCK
	MEM_NODE	sNode;
	void *Tmp;

	ASSERT( size, "blkPointerValid: cannot check a pointer with zero size" );

	if (pData == NULL)
	{
		return FALSE;
	}

	// Create a dummy node for the search
	// This is only looked at by memBlockCmp so only need to set the object and size
	sNode.pObj = ((UBYTE *)pData) - sizeof(MEM_NODE) - SAFETY_ZONE_SIZE;
	sNode.size = size;

	// See if the block is in the treap
	Tmp = treapFindRec((TREAP_NODE *)psHeap->psMemTreap, (void*)&sNode, memBlockCmp);
	if (Tmp != NULL)
//	if (treapFindRec((TREAP_NODE *)psHeap->psMemTreap, (void*)&sNode, memBlockCmp))
	{
		return TRUE;
	}

	return FALSE;
#else
	psHeap = psHeap;
	pData = pData;
	size = size;

	return TRUE;
#endif
}

// check if a pointer is valid in any currently allocated block
BOOL blkPointerValidAll(void *pData, SDWORD size)
{
#ifdef DEBUG_BLOCK
	BLOCK_HEAP	*psCurr;

	for(psCurr=psBlockList; psCurr; psCurr=psCurr->psNext)
	{
		if (blkPointerValid(psCurr, pData, size))
		{
			return TRUE;
		}
	}

	return FALSE;
#else
	pData = pData;
	size = size;

	return TRUE;
#endif
}




BLOCK_HEAP	*psSuspendedHeap=NULL;

// suspend the current block ... all allocations pass though to system memory allocation
// if a block is already suspended then an assertion will occur.
void  blockSuspendUsage(void)
{
	ASSERT( psSuspendedHeap==NULL, "a memory block is already suspended" );

	psSuspendedHeap = memGetBlockHeap();
	memSetBlockHeap(NULL);

}

// restore the current block  - if there is one
void blockUnsuspendUsage(void)
{
	memSetBlockHeap(psSuspendedHeap);
	psSuspendedHeap=NULL;

}
