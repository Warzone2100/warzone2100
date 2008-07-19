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
 * Treap.c
 *
 * Balanced tree implementation
 *
 */

#include <string.h>
#include <stdlib.h>

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include "types.h"
#include "debug.h"
#include "treap.h"

typedef struct TREAP_NODE
{
	const char*                     key;                    //< The key to sort the node on
	unsigned int                    priority;               //< Treap priority
	struct STR_ID*                  pObj;                   //< The object stored in the treap
	struct TREAP_NODE               *psLeft, *psRight;      //< The sub trees

#ifdef DEBUG_TREAP
	const char                      *pFile;                 //< file the node was created in
	int                             line;                   //< line the node was created at
#endif

} TREAP_NODE;

/** Treap data structure */
typedef struct TREAP
{
	TREAP_NODE                      *psRoot;                ///< root of the tree

#ifdef DEBUG_TREAP
	const char                      *pFile;                 ///< file the treap was created in
	int                             line;                   ///< line the treap was created at
#endif
} TREAP;

/* Position of the last call */
static int cLine = -1;
static const char* pCFile = "No file specified";

void treapSetCallPos(const char* fileName, int lineNumber)
{
	cLine = lineNumber;

	/* Assume that fileName originates from a __FILE__ macro, which is
	 * statically allocated. Thus we can rely on this memory remaining
	 * available during the entire program execution time.
	 */
	pCFile = fileName;
}

/* A useful comparison function - keys are char pointers */
static int treapStringCmp(const char *key1, const char *key2)
{
	int result;

	result = strcmp(key1, key2);
	if      (result < 0)
		return -1;
	else if (result > 0)
		return 1;
	else
		return 0;
}


BOOL treapCreate(TREAP **ppsTreap)
{
	*ppsTreap = (TREAP*)malloc(sizeof(TREAP));
	if (!(*ppsTreap))
	{
		debug( LOG_ERROR, "treapCreate: Out of memory" );
		abort();
		return false;
	}

	// Initialise the tree to nothing
	(*ppsTreap)->psRoot = NULL;

#if DEBUG_TREAP
	// Store the call location
	(*ppsTreap)->pFile = pCFile;
	(*ppsTreap)->line = cLine;
#endif
	return true;
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
static void treapAddNode(TREAP_NODE **ppsRoot, TREAP_NODE *psNew)
{
	if (*ppsRoot == NULL)
	{
		// Make the node the root of the tree
		*ppsRoot = psNew;
	}
	else if (treapStringCmp(psNew->key, (*ppsRoot)->key) < 0)
	{
		// Node less than root, insert to the left of the tree
		treapAddNode(&(*ppsRoot)->psLeft, psNew);

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
		treapAddNode(&(*ppsRoot)->psRight, psNew);

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
BOOL treapAdd(TREAP *psTreap, const char *key, struct STR_ID* pObj)
{
	TREAP_NODE* psNew = malloc(sizeof(TREAP_NODE));

	if (psNew == NULL)
	{
		debug(LOG_ERROR, "treapAdd: Out of memory");
		return false;
	}
	psNew->priority = rand();
	psNew->key = key;
	psNew->pObj = pObj;
	psNew->psLeft = NULL;
	psNew->psRight = NULL;
#if DEBUG_TREAP
	// Store the call location
	psNew->pFile = pCFile;
	psNew->line = cLine;
#endif

	treapAddNode(&psTreap->psRoot, psNew);

	return true;
}

/* Recursively find and remove a node from the tree */
static TREAP_NODE *treapDelRec(TREAP_NODE **ppsRoot, const char *key)
{
	TREAP_NODE	*psFound;

	if (*ppsRoot == NULL)
	{
		// not found
		return NULL;
	}

	switch (treapStringCmp(key, (*ppsRoot)->key))
	{
	case -1:
		// less than
		return treapDelRec(&(*ppsRoot)->psLeft, key);
		break;
	case 1:
		// greater than
		return treapDelRec(&(*ppsRoot)->psRight, key);
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
				return treapDelRec(&(*ppsRoot)->psLeft, key);
			}
			else
			{
				// rotate left and recurse
				treapRotRight(ppsRoot);
				return treapDelRec(&(*ppsRoot)->psRight, key);
			}
		}
		break;
	default:
		ASSERT( false, "treapDelRec: invalid return from comparison" );
		break;
	}
	return NULL;
}


/* Remove an object from the treap */
BOOL treapDel(TREAP *psTreap, const char *key)
{
	TREAP_NODE	*psDel;

	// Find the node to remove
	psDel = treapDelRec(&psTreap->psRoot, key);
	if (!psDel)
	{
		return false;
	}

	free(psDel);

	return true;
}


/* Recursively find an object in a treap */
static struct STR_ID* treapFindRec(TREAP_NODE *psRoot, const char *key)
{
	if (psRoot == NULL)
	{
		return NULL;
	}

	switch (treapStringCmp(key, psRoot->key))
	{
	case 0:
		// equal
		return psRoot->pObj;
		break;
	case -1:
		return treapFindRec(psRoot->psLeft, key);
		break;
	case 1:
		return treapFindRec(psRoot->psRight, key);
		break;
	default:
		ASSERT( false, "treapFindRec: invalid return from comparison" );
		break;
	}
	return NULL;
}


/* Find an object in a treap */
struct STR_ID* treapFind(TREAP *psTreap, const char *key)
{
	return treapFindRec(psTreap->psRoot, key);
}


#if DEBUG_TREAP
/* Recursively print out where the nodes were allocated */
static void treapReportRec(TREAP_NODE *psRoot)
{
	if (psRoot)
	{
		debug(LOG_NEVER, "   %s, line %d\n", psRoot->pFile, psRoot->line);
		treapReportRec(psRoot->psLeft);
		treapReportRec(psRoot->psRight);
	}
}
#endif


/* Recursively free a treap */
static void treapDestroyRec(TREAP_NODE *psRoot)
{
	if (psRoot == NULL)
	{
		return;
	}

	// free the sub branches
	treapDestroyRec(psRoot->psLeft);
	treapDestroyRec(psRoot->psRight);

	// free the root
	free(psRoot);
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
#endif

	treapDestroyRec(psTreap->psRoot);
	free(psTreap);
}

static struct STR_ID* treapGetSmallestRec(TREAP_NODE *psRoot)
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
struct STR_ID* treapGetSmallest(TREAP *psTreap)
{
	if (psTreap->psRoot == NULL)
	{
		return NULL;
	}

	return treapGetSmallestRec(psTreap->psRoot);
}
