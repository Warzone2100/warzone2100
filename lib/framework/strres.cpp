/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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

/* A String Resource */
struct STR_RES
{
	struct TREAP_NODE     **psIDTreap;              ///< The treap to store string identifiers
};

/* Initialise the string system */
STR_RES *strresCreate()
{
	STR_RES *const psRes = (STR_RES *)malloc(sizeof(*psRes));
	if (!psRes)
	{
		debug(LOG_FATAL, "Out of memory");
		abort();
		return NULL;
	}

	psRes->psIDTreap = treapCreate();
	if (!psRes->psIDTreap)
	{
		debug(LOG_FATAL, "Out of memory");
		abort();
		free(psRes);
		return NULL;
	}

	return psRes;
}

/* Shutdown the string system */
void strresDestroy(STR_RES *psRes)
{
	// Release the treap and free the final memory
	treapDestroy(psRes->psIDTreap);
	free(psRes);
}


/* Store a string */
bool strresStoreString(STR_RES *psRes, const char *pID, const char *pString)
{
	ASSERT(psRes != NULL, "Invalid string res pointer");

	// Make sure that this ID string hasn't been used before
	if (treapFind(psRes->psIDTreap, pID) != NULL)
	{
		debug(LOG_FATAL, "Duplicate string for id: \"%s\"", pID);
		abort();
		return false;
	}

	return treapAdd(psRes->psIDTreap, pID, pString);
}

const char *strresGetString(const STR_RES *psRes, const char *ID)
{
	ASSERT(psRes != NULL, "Invalid string resource pointer");
	return treapFind(psRes->psIDTreap, ID);
}

/* Load a string resource file */
bool strresLoad(STR_RES *psRes, const char *fileName)
{
	bool retval;
	lexerinput_t input;

	input.type = LEXINPUT_PHYSFS;
	input.input.physfsfile = PHYSFS_openRead(fileName);
	debug(LOG_WZ, "Reading...[directory %s] %s", PHYSFS_getRealDir(fileName), fileName);
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
const char *strresGetIDfromString(STR_RES *psRes, const char *pString)
{
	ASSERT(psRes != NULL, "Invalid string res pointer");

	return treapFindKey(psRes->psIDTreap, pString);
}
