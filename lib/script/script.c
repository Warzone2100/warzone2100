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
BOOL scriptInitialise(EVENT_INIT *psInit)
{
	if (!stackInitialise())
	{
		return FALSE;
	}
	if (!interpInitialise())
	{
		return FALSE;
	}
	if (!eventInitialise(psInit))
	{
		return FALSE;
	}

	return TRUE;
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

	FREE(psCode->pCode);
	if (psCode->pTriggerTab)
	{
		FREE(psCode->pTriggerTab);
	}
	if (psCode->psTriggerData)
	{
		FREE(psCode->psTriggerData);
	}
	FREE(psCode->pEventTab);
	FREE(psCode->pEventLinks);
	if (psCode->pGlobals != NULL)
	{
		FREE(psCode->pGlobals);
	}
	if (psCode->psArrayInfo != NULL)
	{
		FREE(psCode->psArrayInfo);
	}
	if (psCode->psDebug)
	{
		for(i=0; i<psCode->debugEntries; i++)
		{
			if (psCode->psDebug[i].pLabel)
			{
				FREE(psCode->psDebug[i].pLabel);
			}
		}
		FREE(psCode->psDebug);
	}
	if (psCode->psVarDebug)
	{
		for(i=0; i<psCode->numGlobals; i++)
		{
			if (psCode->psVarDebug[i].pIdent)
			{
				FREE(psCode->psVarDebug[i].pIdent);
			}
		}
		FREE(psCode->psVarDebug);
	}
	if (psCode->psArrayDebug)
	{
		for(i=0; i<psCode->numArrays; i++)
		{
			if (psCode->psArrayDebug[i].pIdent)
			{
				FREE(psCode->psArrayDebug[i].pIdent);
			}
		}
		FREE(psCode->psArrayDebug);
	}

	/* Free local vars */
	for(i=0; i < psCode->numEvents; i++)
	{
		if(psCode->numLocalVars[i] > 0)		//only free if any defined
		{
			//free strings for event i
			for(j=0; j < psCode->numLocalVars[i]; j++)
			{
				if(psCode->ppsLocalVarVal[i][j].type == VAL_STRING);	//if a string
				{
					if(psCode->ppsLocalVarVal[i][j].v.sval != NULL)		//doublecheck..
						FREE(psCode->ppsLocalVarVal[i][j].v.sval);		//free string
				}
			}

			FREE(psCode->ppsLocalVars[i]);
			FREE(psCode->ppsLocalVarVal[i]);	//free pointer to event i local vars
		}
	}

	FREE(psCode->numParams);
	FREE(psCode->numLocalVars);

	FREE(psCode->ppsLocalVars);
	FREE(psCode->ppsLocalVarVal);

	psCode->numEvents = 0;

	FREE(psCode);
}


/* Lookup a script variable */
BOOL scriptGetVarIndex(SCRIPT_CODE *psCode, STRING *pID, UDWORD *pIndex)
{
	UDWORD	index;

	if (!psCode->psVarDebug)
	{
		return FALSE;
	}

	for(index=0; index<psCode->numGlobals; index++)
	{
		if (strcmp(psCode->psVarDebug[index].pIdent, pID)==0)
		{
			*pIndex = index;
			return TRUE;
		}
	}

	return FALSE;
}
