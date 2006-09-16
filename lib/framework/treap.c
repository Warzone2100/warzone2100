/*
 * Treap.c
 *
 * Balanced tree implementation
 *
 */

#include <string.h>

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include "types.h"
#include "debug.h"
#include "mem.h"
#include "heap.h"
#include "treap.h"

/* Position of the last call */
static SDWORD	cLine;
static char *pCFile;
static char	pCFileNone[] = "None";

void treapSetCallPos(const char *pFileName, SDWORD lineNumber)
{
	cLine = lineNumber;

	pCFile = (char *)MALLOC(strlen(pFileName) + 1);
	if (pCFile)
	{
		strcpy(pCFile, pFileName);
	}
	else
	{
		pCFile = pCFileNone;
	}
}


/* Default comparison function - assumes keys are ints */
static SDWORD defaultCmp(UDWORD key1, UDWORD key2)
{
	if (key1 < key2)
	{
		return -1;
	}
	else if (key1 > key2)
	{
		return 1;
	}

	return 0;
}

/* A useful comparison function - keys are string pointers */
SDWORD treapStringCmp(UDWORD key1, UDWORD key2)
{
	SDWORD result;
	const char *pStr1 = (const char *)key1;
	const char *pStr2 = (const char *)key2;

	result = strcmp(pStr1,pStr2);
	if (result<0) return -1;
	if (result>0) return 1;
	return 0;
}


BOOL treapCreate(TREAP **ppsTreap, TREAP_CMP cmp, UDWORD init, UDWORD ext)
{
	*ppsTreap = (TREAP*)MALLOC(sizeof(TREAP));
	if (!(*ppsTreap))
	{
		debug( LOG_ERROR, "treapCreate: Out of memory" );
		abort();
		return FALSE;
	}

	if (!HEAP_CREATE(&((*ppsTreap)->psNodes), sizeof(TREAP_NODE), init, ext))
	{
		debug( LOG_ERROR, "treapCreate: Out of memory" );
		abort();
		FREE(*ppsTreap);
		return FALSE;
	}

	// Store the comparison function if there is one, use the default otherwise
	if (cmp)
	{
		(*ppsTreap)->cmp = cmp;
	}
	else
	{
		(*ppsTreap)->cmp = defaultCmp;
	}

	// Initialise the tree to nothing
	(*ppsTreap)->psRoot = NULL;

#if DEBUG_TREAP
	// Store the call location
	(*ppsTreap)->pFile = pCFile;
	(*ppsTreap)->line = cLine;
#endif
	return TRUE;
}

/* Rotate a tree to the right
 * (Make left sub tree the root and the root the right sub tree)
 */
static void treapRotRight(TREAP_NODE **ppsRoot)
{
	TREAP_NODE	*psNewRoot;

	psNewRoot = (*ppsRoot)->psLeft;
	(*ppsRoot)->psLeft = psNewRoot->psRight;
	psNewRoot->psRight = *ppsRoot;
	*ppsRoot = psNewRoot;
}

/* Rotate a tree to the left
 * (Make right sub tree the root and the root the left sub tree)
 */
static void treapRotLeft(TREAP_NODE **ppsRoot)
{
	TREAP_NODE	*psNewRoot;

	psNewRoot = (*ppsRoot)->psRight;
	(*ppsRoot)->psRight = psNewRoot->psLeft;
	psNewRoot->psLeft = *ppsRoot;
	*ppsRoot = psNewRoot;
}

/* Recursive function to add an object to a tree */
void treapAddNode(TREAP_NODE **ppsRoot, TREAP_NODE *psNew, TREAP_CMP cmp)
{
	if (*ppsRoot == NULL)
	{
		// Make the node the root of the tree
		*ppsRoot = psNew;
	}
	else if (cmp(psNew->key, (*ppsRoot)->key) < 0)
	{
		// Node less than root, insert to the left of the tree
		treapAddNode(&(*ppsRoot)->psLeft, psNew, cmp);

		// Sort the priority
		if ((*ppsRoot)->priority > (*ppsRoot)->psLeft->priority)
		{
			// rotate tree right
			treapRotRight(ppsRoot);
		}
	}
	else
	{
		// Node greater than root, insert to the right of the tree
		treapAddNode(&(*ppsRoot)->psRight, psNew, cmp);

		// Sort the priority
		if ((*ppsRoot)->priority > (*ppsRoot)->psRight->priority)
		{
			// rotate tree left
			treapRotLeft(ppsRoot);
		}
	}
}


/* Add an object to a treap
 */
BOOL treapAdd(TREAP *psTreap, UDWORD key, void *pObj)
{
	TREAP_NODE	*psNew;

	if (!HEAP_ALLOC(psTreap->psNodes, (void**)&psNew))
	{
		return FALSE;
	}
	psNew->priority = (UDWORD)rand();
	psNew->key = key;
	psNew->pObj = pObj;
	psNew->psLeft = NULL;
	psNew->psRight = NULL;
#if DEBUG_TREAP
	// Store the call location
	psNew->pFile = pCFile;
	psNew->line = cLine;
#endif

	treapAddNode(&psTreap->psRoot, psNew, psTreap->cmp);

	return TRUE;
}


/* Recursively find and remove a node from the tree */
TREAP_NODE *treapDelRec(TREAP_NODE **ppsRoot, UDWORD key,
							   TREAP_CMP cmp)
{
	TREAP_NODE	*psFound;

	if (*ppsRoot == NULL)
	{
		// not found
		return NULL;
	}

	switch (cmp(key, (*ppsRoot)->key))
	{
	case -1:
		// less than
		return treapDelRec(&(*ppsRoot)->psLeft, key, cmp);
		break;
	case 1:
		// greater than
		return treapDelRec(&(*ppsRoot)->psRight, key, cmp);
		break;
	case 0:
		// equal - either remove or push down the tree to balance it
		if ((*ppsRoot)->psLeft == NULL && (*ppsRoot)->psRight == NULL)
		{
			// no sub trees, remove
			psFound = *ppsRoot;
			*ppsRoot = NULL;
			return psFound;
		}
		else if ((*ppsRoot)->psLeft == NULL)
		{
			// one sub tree, replace
			psFound = *ppsRoot;
			*ppsRoot = psFound->psRight;
			return psFound;
		}
		else if ((*ppsRoot)->psRight == NULL)
		{
			// one sub tree, replace
			psFound = *ppsRoot;
			*ppsRoot = psFound->psLeft;
			return psFound;
		}
		else
		{
			// two sub trees, push the node down and recurse
			if ((*ppsRoot)->psLeft->priority > (*ppsRoot)->psRight->priority)
			{
				// rotate right and recurse
				treapRotLeft(ppsRoot);
				return treapDelRec(&(*ppsRoot)->psLeft, key, cmp);
			}
			else
			{
				// rotate left and recurse
				treapRotRight(ppsRoot);
				return treapDelRec(&(*ppsRoot)->psRight, key, cmp);
			}
		}
		break;
	default:
		ASSERT( FALSE, "treapDelRec: invalid return from comparison" );
		break;
	}
	return NULL;
}


/* Remove an object from the treap */
BOOL treapDel(TREAP *psTreap, UDWORD key)
{
	TREAP_NODE	*psDel;

	// Find the node to remove
	psDel = treapDelRec(&psTreap->psRoot, key, psTreap->cmp);
	if (!psDel)
	{
		return FALSE;
	}

	// Release the node
#ifdef DEBUG_TREAP
	FREE(psDel->pFile);
#endif
	HEAP_FREE(psTreap->psNodes, psDel);

	return TRUE;
}


/* Recurisvely find an object in a treap */
void *treapFindRec(TREAP_NODE *psRoot, UDWORD key, TREAP_CMP cmp)
{
	if (psRoot == NULL)
	{
		return NULL;
	}

	switch (cmp(key, psRoot->key))
	{
	case 0:
		// equal
		return psRoot->pObj;
		break;
	case -1:
		return treapFindRec(psRoot->psLeft, key, cmp);
		break;
	case 1:
		return treapFindRec(psRoot->psRight, key, cmp);
		break;
	default:
		ASSERT( FALSE, "treapFindRec: invalid return from comparison" );
		break;
	}
	return NULL;
}


/* Find an object in a treap */
void *treapFind(TREAP *psTreap, UDWORD key)
{
	return treapFindRec(psTreap->psRoot, key, psTreap->cmp);
}


#if DEBUG_TREAP
/* Recursively print out where the nodes were allocated */
static void treapReportRec(TREAP_NODE *psRoot)
{
	if (psRoot)
	{
		debug( LOG_NEVER, (("   %s, line %d\n", psRoot->pFile, psRoot->line );
		treapReportRec(psRoot->psLeft);
		treapReportRec(psRoot->psRight);
	}
}
#endif


/* Recursively free a treap */
static void treapDestroyRec(TREAP_NODE *psRoot, OBJ_HEAP *psHeap)
{
	if (psRoot == NULL)
	{
		return;
	}

	// free the sub branches
	treapDestroyRec(psRoot->psLeft, psHeap);
	treapDestroyRec(psRoot->psRight, psHeap);

	// free the root
	HEAP_FREE(psHeap, psRoot);
}


/* Release all the nodes in the treap */
void treapReset(TREAP *psTreap)
{
	treapDestroyRec(psTreap->psRoot, psTreap->psNodes);
	psTreap->psRoot = NULL;
}


/* Destroy a treap and release all the memory associated with it */
void treapDestroy(TREAP *psTreap)
{
#if DEBUG_TREAP
	if (psTreap->psRoot)
	{
		debug( LOG_NEVER, "treapDestroy: %s, line %d : nodes still in the tree\n", psTreap->pFile, psTreap->line );
		treapReportRec(psTreap->psRoot);
	}
	FREE(psTreap->pFile);
#endif

	treapDestroyRec(psTreap->psRoot, psTreap->psNodes);
	HEAP_DESTROY(psTreap->psNodes);
	FREE(psTreap);
}

/* Recursively display the treap structure */
void treapDisplayRec(TREAP_NODE *psRoot, UDWORD indent)
{
	UDWORD	i;

	// Display the root
#if DEBUG_TREAP
	debug( LOG_NEVER, "%s, line %d : %d,%d\n", psRoot->pFile, psRoot->line, psRoot->key, psRoot->priority );
#else
	debug( LOG_NEVER, "%d,%d\n", psRoot->key, psRoot->priority );
#endif

	// Display the left of the tree
	if (psRoot->psLeft)
	{
		for(i=0; i<indent; i++)
		{
			debug( LOG_NEVER, "" );
		}
		debug( LOG_NEVER, "L" );
		treapDisplayRec(psRoot->psLeft, indent+1);
	}

	// Display the right of the tree
	if (psRoot->psRight)
	{
		for(i=0; i<indent; i++)
		{
			debug( LOG_NEVER, "" );
		}
		debug( LOG_NEVER, "R" );
		treapDisplayRec(psRoot->psRight, indent+1);
	}
}


/* Display the treap structure using DBPRINTF */
void treapDisplay(TREAP *psTreap)
{
	if (psTreap->psRoot)
	{
		treapDisplayRec(psTreap->psRoot, 0);
	}
}


void *treapGetSmallestRec(TREAP_NODE *psRoot)
{
	if (psRoot->psLeft == NULL)
	{
		return psRoot->pObj;
	}

	return treapGetSmallestRec(psRoot->psLeft);
}


/* Return the object with the smallest key in the treap
 * This is useful if the objects in the treap need to be
 * deallocated.  i.e. getSmallest, delete from treap, free memory
 */
void *treapGetSmallest(TREAP *psTreap)
{
	if (psTreap->psRoot == NULL)
	{
		return NULL;
	}

	return treapGetSmallestRec(psTreap->psRoot);
}



