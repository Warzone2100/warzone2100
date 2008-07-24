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
 * StrRes.c
 *
 * String storage an manipulation functions
 *
 */

#include <string.h>
#include <stdlib.h>

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include "types.h"
#include "debug.h"
#include "treap.h"
#include "strres.h"
#include "strresly.h"

/* A string */
typedef struct STR
{
	const char*     str;
	unsigned int    id;

#ifdef DEBUG_CHECK_FOR_UNUSED_STRINGS
	unsigned int    usage;
#endif

	struct STR* psNext;
} STR;

/* An ID entry */
typedef struct STR_ID
{
	unsigned int    id;
	const char*     pIDStr;
} STR_ID;

/* A String Resource */
typedef struct STR_RES
{
	struct TREAP_NODE**     psIDTreap;              ///< The treap to store string identifiers
	STR*                    psStrings;              ///< The store for the strings themselves
	STR**                   psLastStr;              ///< A pointer to the next pointer of the last string in the list
	UDWORD                  nextID;                 ///< The next free ID
} STR_RES;

/* Static forward declarations */
static void strresReleaseIDStrings(STR_RES *psRes);

static STR* strresAllocString(const char* str, unsigned int id)
{
	/* Over-allocate so that we can put the string in the same chunck of
	 * memory. Which means a single free() call on the entire STR structure
	 * will suffice.
	 */
	STR* const psStr = (STR*)malloc(sizeof(*psStr) + strlen(str) + 1);
	if (!psStr)
	{
		debug(LOG_ERROR, "Out of memory - 1");
		abort();
		return NULL;
	}

	// Copy the string into its memory chunk and set the pointer to it
	psStr->str = strcpy((char*)(psStr + 1), str);

	psStr->id = id;
	psStr->psNext = NULL;

#ifdef DEBUG_CHECK_FOR_UNUSED_STRINGS
	psStr->usage = 0;
#endif

	return psStr;
}

static STR_ID* strresAllocIDStr(unsigned int const id, const char * const str)
{
	/* Over-allocate so that we can put the string in the same chunck of
	 * memory. Which means a single free() call on the entire STR_ID
	 * structure will suffice.
	 */
	STR_ID* const psID = (STR_ID*)malloc(sizeof(*psID) + strlen(str) + 1);
	if (!psID)
	{
		debug(LOG_ERROR, "Out of memory");
		abort();
		return NULL;
	}

	// Copy the string into its memory chunk and set the pointer to it
	psID->pIDStr = strcpy((char*)(psID + 1), str);

	psID->id = id;

	return psID;
}

/* Initialise the string system */
STR_RES* strresCreate()
{
	STR_RES* const psRes = (STR_RES*)malloc(sizeof(*psRes));
	if (!psRes)
	{
		debug(LOG_ERROR, "Out of memory");
		abort();
		return NULL;
	}
	psRes->nextID = 0;

	psRes->psIDTreap = treapCreate();
	if (!psRes->psIDTreap)
	{
		debug(LOG_ERROR, "Out of memory");
		abort();
		free(psRes);
		return NULL;
	}

	psRes->psStrings = NULL;
	psRes->psLastStr = &psRes->psStrings;

	return psRes;
}


/* Release the id strings, but not the strings themselves,
 * (they can be accessed only by id number).
 */
static void strresReleaseIDStrings(STR_RES *psRes)
{
	STR_ID		*psID;

	ASSERT( psRes != NULL,
		"strresReleaseIDStrings: Invalid string res pointer" );

	for(psID = treapGetSmallest(psRes->psIDTreap); psID;
		psID = treapGetSmallest(psRes->psIDTreap))
	{
		treapDel(psRes->psIDTreap, psID->pIDStr);
		free(psID);
	}
}


/* Shutdown the string system */
void strresDestroy(STR_RES *psRes)
{
	STR* psString;

	ASSERT( psRes != NULL,
		"strresDestroy: Invalid string res pointer" );

	// Free the string id's
	strresReleaseIDStrings(psRes);

	// Free the strings themselves
	psString = psRes->psStrings;
	while (psString)
	{
		STR * const toDelete = psString;
		psString = psString->psNext;

#ifdef DEBUG_CHECK_FOR_UNUSED_STRINGS
		if (toDelete->usage == 0)
		{
			debug(LOG_NEVER, "Strind id %u not used: \"%s\"", toDelete->id, toDelete->str);
		}
#endif

		free(toDelete);
	}

	// Release the treap and free the final memory
	treapDestroy(psRes->psIDTreap);
	free(psRes);
}


/* Return the ID stored ID string that matches the string passed in */
const char* strresGetIDString(STR_RES *psRes, const char *pIDStr)
{
	STR_ID *psID;

	ASSERT(psRes != NULL, "Invalid string res pointer");

	psID = treapFind(psRes->psIDTreap, pIDStr);
	if (!psID)
	{
		return NULL;
	}

	return psID->pIDStr;
}


/* Store a string */
BOOL strresStoreString(STR_RES *psRes, const char* pID, const char* pString)
{
	STR_ID* psID;
	STR* psString;

	ASSERT( psRes != NULL,
		"strresStoreString: Invalid string res pointer" );

	// Find the id for the string
	psID = treapFind(psRes->psIDTreap, pID);
	if (!psID)
	{
		// No ID yet so generate a new one
		psID = strresAllocIDStr(psRes->nextID++, pID);
		if (!psID)
		{
			return false;
		}

		treapAdd(psRes->psIDTreap, psID->pIDStr, psID);
	}

	// Make sure that this ID number hasn't been used before
	for (psString = psRes->psStrings; psString; psString = psString->psNext)
	{
		if (psString->id == psID->id)
		{
			debug(LOG_ERROR, "Duplicate string for id: \"%s\"", psID->pIDStr);
			abort();
			return false;
		}
	}

	ASSERT(psRes->psLastStr != NULL, "Invalid last string pointer");

	// Allocate the string into a string block
	psString = strresAllocString(pString, psID->id);
	if (!psString)
	{
		return false;
	}

	// Place it into the string list
	*psRes->psLastStr = psString;
	psRes->psLastStr = &psString->psNext;
	psString->psNext = NULL;

	return true;
}


/* Get the string from an ID number */
const char* strresGetString(const STR_RES * psRes, UDWORD id)
{
	STR* psString;

	ASSERT(psRes != NULL, "Invalid string res pointer" );

	// Find the string in the string list
	for (psString = psRes->psStrings; psString; psString = psString->psNext)
	{
		if (psString->id == id)
		{
#ifdef DEBUG_CHECK_FOR_UNUSED_STRINGS
			psString->usage += 1;
#endif

			ASSERT(psString->str != NULL, "There's no string in string block %u", psString->id);

			return psString->str;
		}
	}

	ASSERT(!"String not found", "Couldn't find a string with ID number %u", (unsigned int)id);

	return NULL;
}

const char* strresGetStringByID(const STR_RES* psRes, const char* ID)
{
	STR_ID* psID;

	ASSERT(psRes != NULL, "Invalid string resource pointer");

	// First find the ID number of the given ID string
	psID = treapFind(psRes->psIDTreap, ID);
	if (!psID)
	{
		debug(LOG_ERROR, "Couldn't find string id number for string id \"%s\"", ID);
		return NULL;
	}

	// Now find the string associated with the ID number
	return strresGetString(psRes, psID->id);
}

/* Load a string resource file */
BOOL strresLoad(STR_RES* psRes, const char* fileName)
{
	bool retval;
	lexerinput_t input;

	input.type = LEXINPUT_PHYSFS;
	input.input.physfsfile = PHYSFS_openRead(fileName);
	if (!input.input.physfsfile)
	{
		debug(LOG_ERROR, "strresLoadFile: PHYSFS_openRead(\"%s\") failed with error: %s\n", fileName, PHYSFS_getLastError());
		return false;
	}

	strres_set_extra(&input);
	retval = (strres_parse(psRes) == 0);

	strres_lex_destroy();
	PHYSFS_close(input.input.physfsfile);

	return retval;
}

/* Get the ID number for a string*/
UDWORD strresGetIDfromString(STR_RES *psRes, const char *pString)
{
	STR* psString;

	ASSERT( psRes != NULL,
		"strresGetID: Invalid string res pointer" );

	// Find the string in the string list
	for (psString = psRes->psStrings; psString; psString = psString->psNext)
	{
		ASSERT(psString->str != NULL, "There's no string in string block %u", psString->id);

		if (strcmp(psString->str, pString) == 0)
		{
			return psString->id;
		}
	}

	return 0;
}
