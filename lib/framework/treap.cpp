/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

struct TREAP_NODE
{
	const char                     *key;                    //< The key to sort the node on
	unsigned int                    priority;               //< Treap priority
	const char                     *string;                 //< The string stored in the treap
	TREAP_NODE                      *psLeft, *psRight;      //< The sub trees
};

/* A useful comparison function - keys are char pointers */
static int treapStringCmp(const char *key1, const char *key2)
{
	int result;

	result = strcmp(key1, key2);
	if (result < 0)
	{
		return -1;
	}
	else if (result > 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


TREAP_NODE **treapCreate()
{
	TREAP_NODE **const psTreap = (TREAP_NODE **)malloc(sizeof(*psTreap));
	if (!psTreap)
	{
		debug(LOG_FATAL, "Out of memory");
		abort();
		return NULL;
	}

	// Initialise the tree to nothing
	*psTreap = NULL;

	return psTreap;
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
bool treapAdd(TREAP_NODE **psTreap, const char *key, const char *string)
{
	/* Over-allocate so that we can put the key and the string in the same
	 * chunck of memory as the TREAP_NODE struct. Which means a single
	 * free() call on the entire STR structure will suffice.
	 */
	const size_t key_size    = strlen(key)    + 1;
	const size_t string_size = strlen(string) + 1;
	TREAP_NODE *const psNew = (TREAP_NODE *)malloc(sizeof(*psNew) + key_size + string_size);

	if (psNew == NULL)
	{
		debug(LOG_FATAL, "treapAdd: Out of memory");
		abort();
		return false;
	}

	psNew->key    = strcpy((char *)(psNew + 1),            key);
	psNew->string = strcpy((char *)(psNew + 1) + key_size, string);

	psNew->priority = rand();
	psNew->psLeft = NULL;
	psNew->psRight = NULL;

	treapAddNode(psTreap, psNew);

	return true;
}


/* Recursively find an object in a treap */
static const char *treapFindRec(TREAP_NODE *psRoot, const char *key)
{
	if (psRoot == NULL)
	{
		return NULL;
	}

	switch (treapStringCmp(key, psRoot->key))
	{
	case 0:
		// equal
		return psRoot->string;
		break;
	case -1:
		return treapFindRec(psRoot->psLeft, key);
		break;
	case 1:
		return treapFindRec(psRoot->psRight, key);
		break;
	default:
		ASSERT(false, "treapFindRec: invalid return from comparison");
		break;
	}
	return NULL;
}


/* Find an object in a treap */
const char *treapFind(TREAP_NODE **psTreap, const char *key)
{
	ASSERT(psTreap != NULL, "Invalid treap pointer!");

	return treapFindRec(*psTreap, key);
}

static const char *treapFindKeyRec(TREAP_NODE const *const psNode, const char *const string)
{
	const char *key;

	if (psNode == NULL)
	{
		return NULL;
	}

	if (strcmp(psNode->string, string) == 0)
	{
		return psNode->key;
	}

	key = treapFindKeyRec(psNode->psLeft, string);
	if (key)
	{
		return key;
	}

	return treapFindKeyRec(psNode->psRight, string);
}

const char *treapFindKey(TREAP_NODE **psTreap, const char *string)
{
	ASSERT(psTreap != NULL, "Invalid treap pointer!");

	return treapFindKeyRec(*psTreap, string);
}

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
void treapDestroy(TREAP_NODE **psTreap)
{
	ASSERT(psTreap != NULL, "Invalid treap pointer!");

	treapDestroyRec(*psTreap);
	free(psTreap);
}
