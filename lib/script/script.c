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
 * Script.c
 *
 * A few general functions for the script library
 */

#include "lib/framework/frame.h"
#include "script.h"
#include <assert.h>


// Flags for storing function indexes
#define FUNC_SETBIT	0x80000000		// set for a set function, clear otherwise
#define FUNC_LISTMASK	0x70000000
#define FUNC_INDEXMASK	0x0fffffff
#define FUNC_INSTINCT	0x00000000		// instinct function
#define FUNC_CALLBACK	0x40000000		// callback function
#define FUNC_OBJVAR	0x10000000		// object variable
#define FUNC_EXTERNAL	0x20000000		// external variable


// Initialise the script library
BOOL scriptInitialise()
{
	if (!stackInitialise())
	{
		return false;
	}
	if (!interpInitialise())
	{
		return false;
	}
	if (!eventInitialise())
	{
		return false;
	}

	return true;
}

// Shutdown the script library
void scriptShutDown(void)
{
	eventShutDown();
	stackShutDown();
}


/* Free a SCRIPT_CODE structure */
void scriptFreeCode(SCRIPT_CODE *psCode)
{
	UDWORD	i,j;

	debug(LOG_WZ, "Unloading script data");


	/* Free local vars */
	for(i=0; i < psCode->numEvents; i++)
	{
		if(psCode->numLocalVars[i] > 0)		//only free if any defined
		{
			//free strings for event i
			for(j=0; j < psCode->numLocalVars[i]; j++)
			{
				interpCleanValue(&psCode->ppsLocalVars[i][j]);
			}

			free(psCode->ppsLocalVars[i]); // free pointer to event i local vars
		}
	}

	free(psCode->pCode);
	if (psCode->pTriggerTab)
	{
		free(psCode->pTriggerTab);
	}
	if (psCode->psTriggerData)
	{
		free(psCode->psTriggerData);
	}
	free(psCode->pEventTab);
	free(psCode->pEventLinks);
	if (psCode->pGlobals != NULL)
	{
		free(psCode->pGlobals);
	}
	if (psCode->psArrayInfo != NULL)
	{
		free(psCode->psArrayInfo);
	}
	if (psCode->psDebug)
	{
		for(i=0; i<psCode->debugEntries; i++)
		{
			if (psCode->psDebug[i].pLabel)
			{
				free(psCode->psDebug[i].pLabel);
			}
		}
		free(psCode->psDebug);
	}
	if (psCode->psVarDebug)
	{
		for(i=0; i<psCode->numGlobals; i++)
		{
			if (psCode->psVarDebug[i].pIdent)
			{
				free(psCode->psVarDebug[i].pIdent);
			}
		}
		free(psCode->psVarDebug);
	}
	if (psCode->psArrayDebug)
	{
		for(i=0; i<psCode->numArrays; i++)
		{
			if (psCode->psArrayDebug[i].pIdent)
			{
				free(psCode->psArrayDebug[i].pIdent);
			}
		}
		free(psCode->psArrayDebug);
	}



	if(psCode->numParams != NULL)
		free(psCode->numParams);

	if(psCode->numLocalVars != NULL)
		free(psCode->numLocalVars);

	if(psCode->ppsLocalVars != NULL)
		free(psCode->ppsLocalVars);

	psCode->numEvents = 0;

	free(psCode);
}


/* Lookup a script variable */
BOOL scriptGetVarIndex(SCRIPT_CODE *psCode, char *pID, UDWORD *pIndex)
{
	UDWORD	index;

	if (!psCode->psVarDebug)
	{
		return false;
	}

	for(index=0; index<psCode->numGlobals; index++)
	{
		if (strcmp(psCode->psVarDebug[index].pIdent, pID)==0)
		{
			*pIndex = index;
			return true;
		}
	}

	return false;
}

/* returns true if passed INTERP_TYPE is used as a pointer in INTERP_VAL, false otherwise.
   all types are listed explicitly, with asserts/warnings for invalid/unrecognised types, as
   getting this wrong will cause segfaults if sizeof(void*) != sizeof(SDWORD) (eg. amd64). a lot of
   these aren't currently checked for, but it's a lot clearer what's going on if they're all here */
BOOL scriptTypeIsPointer(INTERP_TYPE type)
{
	ASSERT( ((type < ST_MAXTYPE) || (type >= VAL_REF)), "scriptTypeIsPointer: invalid type: %d", type );
	// any value or'ed with VAL_REF is a pointer
	if (type >= VAL_REF) return true;
	switch (type) {
		case VAL_STRING:
		case VAL_OBJ_GETSET:
		case VAL_FUNC_EXTERN:
		case ST_INTMESSAGE:
		case ST_BASEOBJECT:
		case ST_DROID:
		case ST_STRUCTURE:
		case ST_FEATURE:
		case ST_TEMPLATE:
		case ST_TEXTSTRING:
		case ST_LEVEL:
		case ST_RESEARCH:
		case ST_GROUP:
		case ST_POINTER_O:
		case ST_POINTER_T:
		case ST_POINTER_S:
		case ST_POINTER_STRUCTSTAT:
			return true;
		case VAL_BOOL:
		case VAL_INT:
		case VAL_FLOAT:
		case VAL_TRIGGER:
		case VAL_EVENT:
		case VAL_VOID:
		case VAL_OPCODE:
		case VAL_PKOPCODE:
		case ST_BASESTATS:
		case ST_COMPONENT:
		case ST_BODY:
		case ST_PROPULSION:
		case ST_ECM:
		case ST_SENSOR:
		case ST_CONSTRUCT:
		case ST_WEAPON:
		case ST_REPAIR:
		case ST_BRAIN:
		case ST_STRUCTUREID:
		case ST_STRUCTURESTAT:
		case ST_FEATURESTAT:
		case ST_DROIDID:
		case ST_SOUND:
			return false;
		default:
			ASSERT(false, "scriptTypeIsPointer: unhandled type: %d", type );
			return false;
	}
}
