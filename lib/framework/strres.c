/*
 * StrRes.c
 *
 * String storage an manipulation functions
 *
 */

#include <string.h>

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

// Report unused strings
//#define DEBUG_GROUP0
#include "types.h"
#include "debug.h"
#include "mem.h"
#include "heap.h"
#include "treap.h"
#include "strres.h"
#include "strresly.h"

/* The string resource currently being loaded */
STR_RES	*psCurrRes;

/* The id number of ID strings allocated by the system is ORed with this */
#define ID_ALLOC	0x80000000

/* Allocate a string block */
static BOOL strresAllocBlock(STR_BLOCK **ppsBlock, UDWORD size)
{
	*ppsBlock = (STR_BLOCK*)MALLOC(sizeof(STR_BLOCK));
	if (!*ppsBlock)
	{
		DBERROR(("strresAllocBlock: Out of memory - 1"));
		return FALSE;
	}

	(*ppsBlock)->apStrings = (STRING**)MALLOC(sizeof(STRING *) * size);
	if (!(*ppsBlock)->apStrings)
	{
		DBERROR(("strresAllocBlock: Out of memory - 2"));
		FREE(*ppsBlock);
		return FALSE;
	}
	memset((*ppsBlock)->apStrings, 0, sizeof(STRING *) * size);

#ifdef DEBUG
	(*ppsBlock)->aUsage = MALLOC(sizeof(UDWORD) * size);
	memset((*ppsBlock)->aUsage, 0, sizeof(UDWORD) * size);
#endif

	return TRUE;
}


/* Initialise the string system */
BOOL strresCreate(STR_RES **ppsRes, UDWORD init, UDWORD ext)
{
	STR_RES		*psRes;

	psRes = (STR_RES*)MALLOC(sizeof(STR_RES));
	if (!psRes)
	{
		DBERROR(("strresCreate: Out of memory"));
		return FALSE;
	}
	psRes->init = init;
	psRes->ext = ext;
	psRes->nextID = 0;

	if (!TREAP_CREATE(&psRes->psIDTreap, treapStringCmp, init, ext))
	{
		DBERROR(("strresCreate: Out of memory"));
		FREE(psRes);
		return FALSE;
	}

	if (!strresAllocBlock(&psRes->psStrings, init))
	{
		TREAP_DESTROY(psRes->psIDTreap);
		FREE(psRes);
		return FALSE;
	}
	psRes->psStrings->psNext = NULL;
	psRes->psStrings->idStart = 0;
	psRes->psStrings->idEnd = init-1;

	*ppsRes = psRes;

	return TRUE;
}


/* Release the id strings, but not the strings themselves,
 * (they can be accessed only by id number).
 */
void strresReleaseIDStrings(STR_RES *psRes)
{
	STR_ID		*psID;

	ASSERT((PTRVALID(psRes, sizeof(STR_RES)),
		"strresLoadFixedID: Invalid string res pointer"));

	for(psID = (STR_ID*)TREAP_GETSMALLEST(psRes->psIDTreap); psID;
		psID = (STR_ID*)TREAP_GETSMALLEST(psRes->psIDTreap))
	{
		TREAP_DEL(psRes->psIDTreap, (UDWORD)psID->pIDStr);
		if (psID->id & ID_ALLOC)
		{
			FREE(psID->pIDStr);
			FREE(psID);
		}
	}
}


/* Shutdown the string system */
void strresDestroy(STR_RES *psRes)
{
	STR_BLOCK	*psBlock, *psNext = NULL;
	UDWORD		i;

	ASSERT((PTRVALID(psRes, sizeof(STR_RES)),
		"strresLoadFixedID: Invalid string res pointer"));

	// Free the string id's
	strresReleaseIDStrings(psRes);

	// Free the strings themselves
	for(psBlock = psRes->psStrings; psBlock; psBlock=psNext)
	{
		for(i=psBlock->idStart; i<=psBlock->idEnd; i++)
		{
#ifdef DEBUG_GROUP0
			if (psBlock->aUsage[i - psBlock->idStart] == 0
				&& i != 0 && i < psRes->nextID)
			{
				DBP0(("strresDestroy: String id %d not used:\n"
						  "               \"%s\"\n",
					i, psBlock->apStrings[i - psBlock->idStart]));
			}
#endif
			if (psBlock->apStrings[i - psBlock->idStart])
			{
				FREE(psBlock->apStrings[i - psBlock->idStart]);
			}
#ifdef DEBUG
			else if (i < psRes->nextID)
			{
				DBPRINTF(("strresDestroy: No string loaded for id %d\n", i));
			}
#endif
		}
		psNext = psBlock->psNext;
		FREE(psBlock->apStrings);
#ifdef DEBUG
		FREE(psBlock->aUsage);
#endif
		FREE(psBlock);
	}

	// Release the treap and free the final memory
	TREAP_DESTROY(psRes->psIDTreap);
	FREE(psRes);
}


/* Load a list of string ID's from a memory buffer */
BOOL strresLoadFixedID(STR_RES *psRes, STR_ID *psID, UDWORD numID)
{
	UDWORD	i;

	ASSERT((PTRVALID(psRes, sizeof(STR_RES)),
		"strresLoadFixedID: Invalid string res pointer"));

	for (i=0; i<numID; i++)
	{
		ASSERT((psID->id == psRes->nextID,
			"strresLoadFixedID: id out of sequence"));

		// Store the ID string
		if (!TREAP_ADD(psRes->psIDTreap, (UDWORD)psID->pIDStr, psID))
		{
			DBERROR(("strresLoadFixedID: Out of memory"));
			return FALSE;
		}

		psID += 1;
		psRes->nextID += 1;
	}

	return TRUE;
}


/* Return the ID number for an ID string */
BOOL strresGetIDNum(STR_RES *psRes, STRING *pIDStr, UDWORD *pIDNum)
{
	STR_ID	*psID;

	ASSERT((PTRVALID(psRes, sizeof(STR_RES)),
		"strresLoadFixedID: Invalid string res pointer"));

	psID = (STR_ID*)TREAP_FIND(psRes->psIDTreap, (UDWORD)pIDStr);
	if (!psID)
	{
		*pIDNum = 0;
		return FALSE;
	}

	if (psID->id & ID_ALLOC)
	{
		*pIDNum = psID->id & ~ID_ALLOC;
	}
	else
	{
		*pIDNum = psID->id;
	}
	return TRUE;
}


/* Return the ID stored ID string that matches the string passed in */
BOOL strresGetIDString(STR_RES *psRes, STRING *pIDStr, STRING **ppStoredID)
{
	STR_ID	*psID;

	ASSERT((PTRVALID(psRes, sizeof(STR_RES)),
		"strresLoadFixedID: Invalid string res pointer"));

	psID = (STR_ID*)TREAP_FIND(psRes->psIDTreap, (UDWORD)pIDStr);
	if (!psID)
	{
		*ppStoredID = NULL;
		return FALSE;
	}

	*ppStoredID = psID->pIDStr;

	return TRUE;
}


/* Store a string */
BOOL strresStoreString(STR_RES *psRes, STRING *pID, STRING *pString)
{
	STR_ID		*psID;
	STRING		*pNew;
	STR_BLOCK	*psBlock;
	UDWORD		id;

	ASSERT((PTRVALID(psRes, sizeof(STR_RES)),
		"strresLoadFixedID: Invalid string res pointer"));

	// Find the id for the string
	psID = (STR_ID*)TREAP_FIND(psRes->psIDTreap, (UDWORD)pID);
	if (!psID)
	{
		// No ID yet so generate a new one
		psID = (STR_ID*)MALLOC(sizeof(STR_ID));
		if (!psID)
		{
			DBERROR(("strresStoreString: Out of memory"));
			return FALSE;
		}
		psID->pIDStr = (STRING*)MALLOC(sizeof(STRING) * (stringLen(pID) + 1));
		if (!psID->pIDStr)
		{
			DBERROR(("strresStoreString: Out of memory"));
			FREE(psID);
			return FALSE;
		}
		stringCpy(psID->pIDStr, pID);
		psID->id = psRes->nextID | ID_ALLOC;
		psRes->nextID += 1;
		TREAP_ADD(psRes->psIDTreap, (UDWORD)psID->pIDStr, psID);
	}
	id = psID->id & ~ID_ALLOC;

	// Find the block to store the string in
	for(psBlock = psRes->psStrings; psBlock->idEnd < id;
		psBlock = psBlock->psNext)
	{
		if (!psBlock->psNext)
		{
			// Need to allocate a new string block
			if (!strresAllocBlock(&psBlock->psNext, psRes->ext))
			{
				return FALSE;
			}
			psBlock->psNext->idStart = psBlock->idEnd+1;
			psBlock->psNext->idEnd = psBlock->idEnd + psRes->ext;
			psBlock->psNext->psNext = NULL;
		}
	}

	// Put the new string in the string block
	if (psBlock->apStrings[psID->id - psBlock->idStart] != NULL)
	{
		DBERROR(("strresStoreString: Duplicate string for id: %s", psID->pIDStr));
		return FALSE;
	}

	// Allocate a copy of the string
	pNew = (STRING*)MALLOC(sizeof(STRING) * (stringLen(pString) + 1));
	if (!pNew)
	{
		DBERROR(("strresStoreString: Out of memory"));
		return FALSE;
	}
	stringCpy(pNew, pString);
	psBlock->apStrings[psID->id - psBlock->idStart] = pNew;

	return TRUE;
}


/* Get the string from an ID number */
STRING *strresGetString(STR_RES *psRes, UDWORD id)
{
	STR_BLOCK	*psBlock;

	ASSERT((PTRVALID(psRes, sizeof(STR_RES)),
		"strresLoadFixedID: Invalid string res pointer"));

	// find the block the string is in
	for(psBlock = psRes->psStrings; psBlock && psBlock->idEnd < id;
		psBlock = psBlock->psNext)
		;

	if (!psBlock)
	{
		ASSERT((FALSE, "strresGetString: String not found"));
		// Return the default string
		return psRes->psStrings->apStrings[0];
	}

	ASSERT((psBlock->apStrings[id - psBlock->idStart] != NULL,
		"strresGetString: String not found"));

#ifdef DEBUG
	psBlock->aUsage[id - psBlock->idStart] += 1;
#endif

	if (psBlock->apStrings[id - psBlock->idStart] == NULL)
	{
		// Return the default string
		return psRes->psStrings->apStrings[0];
	}

	return psBlock->apStrings[id - psBlock->idStart];
}


/* Load a string resource file */
BOOL strresLoad(STR_RES *psRes, UBYTE *pData, UDWORD size)
{
	psCurrRes = psRes;
	strresSetInputBuffer(pData, size);
	if (strres_parse() != 0)
	{
		return FALSE;
	}

	return TRUE;
}


/* Return the the length of a STRING */
UDWORD stringLen(STRING *pStr)
{
	UDWORD	count=0;

	while (*pStr++)
	{
		count += 1;
	}

	return count;
}

/* Copy a STRING */
void stringCpy(STRING *pDest, STRING *pSrc)
{
	do
	{
		*pDest++ = *pSrc;
	} while (*pSrc++);
}


/* Get the ID number for a string*/
UDWORD strresGetIDfromString(STR_RES *psRes, STRING *pString)
{
	STR_BLOCK	*psBlock, *psNext = NULL;
	UDWORD		i;

	ASSERT((PTRVALID(psRes, sizeof(STR_RES)),
		"strresGetID: Invalid string res pointer"));

	// Search through all the blocks to find the string
	for(psBlock = psRes->psStrings; psBlock; psBlock=psNext)
	{
		for(i=psBlock->idStart; i<=psBlock->idEnd; i++)
		{
			if (psBlock->apStrings[i - psBlock->idStart])
			{
				if (!strcmp(psBlock->apStrings[i - psBlock->idStart], pString))
				{
					return i;
				}
			}
		}
		psNext = psBlock->psNext;
	}
	return 0;
}

