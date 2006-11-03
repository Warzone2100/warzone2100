/*
 * ScriptVals.c
 *
 * Script data value functions
 *
 */


#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/script/script.h"
#include "objects.h"
#include "base.h"
#include "scripttabs.h"
#include "scriptvals.h"
#include "lib/gamelib/gtime.h"
#include "text.h"
#include "group.h"

// Keep all the loaded script contexts
typedef struct _scrv_store
{
	SCRV_TYPE		type;
	char			*pIDString;
	SCRIPT_CONTEXT	*psContext;

	struct _scrv_store *psNext;
} SCRV_STORE;

// The list of script contexts
static SCRV_STORE	*psContextStore=NULL;

// keep a note of all base object pointers
#define MAX_BASEPOINTER		700		//200 - local variables require more of these ("run" error)
static INTERP_VAL	*asBasePointers[MAX_BASEPOINTER];

// Initialise the script value module
BOOL scrvInitialise(void)
{
	psContextStore = NULL;
	memset(asBasePointers, 0, sizeof(asBasePointers));

	return TRUE;
}

// Shut down the script value module
void scrvShutDown(void)
{
	SCRV_STORE	*psCurr;
	while (psContextStore)
	{
		psCurr = psContextStore;
		psContextStore = psContextStore->psNext;

		FREE(psCurr->pIDString);
		FREE(psCurr);
	}
}


// reset the script value module
void scrvReset(void)
{
	SCRV_STORE	*psCurr;
	while (psContextStore)
	{
		psCurr = psContextStore;
		psContextStore = psContextStore->psNext;

		FREE(psCurr->pIDString);
		FREE(psCurr);
	}

	psContextStore = NULL;
	memset(asBasePointers, 0, sizeof(asBasePointers));
}


// Add a new context to the list
BOOL scrvAddContext(char *pID, SCRIPT_CONTEXT *psContext, SCRV_TYPE type)
{
	SCRV_STORE		*psNew;

	psNew = MALLOC(sizeof(SCRV_STORE));
	if (!psNew)
	{
		debug( LOG_ERROR, "scrvAddContext: Out of memory" );
		abort();
		return FALSE;
	}
	psNew->pIDString = MALLOC(strlen(pID) + 1);
	if (!psNew->pIDString)
	{
		debug( LOG_ERROR, "scrvAddContext: Out of memory" );
		abort();
		return FALSE;
	}
	strcpy(psNew->pIDString, pID);
	psNew->type = type;
	psNew->psContext = psContext;

	psNew->psNext = psContextStore;
	psContextStore = psNew;

	return TRUE;
}


// Add a new base pointer variable
BOOL scrvAddBasePointer(INTERP_VAL *psVal)
{
	SDWORD	i;

	for(i=0; i<MAX_BASEPOINTER; i++)
	{
		if (asBasePointers[i] == NULL)
		{
			asBasePointers[i] = psVal;
			return TRUE;
		}
	}

	return FALSE;
}


// remove a base pointer from the list
void scrvReleaseBasePointer(INTERP_VAL *psVal)
{
	SDWORD	i;

	for(i=0; i<MAX_BASEPOINTER; i++)
	{
		if (asBasePointers[i] == psVal)
		{
			asBasePointers[i] = NULL;
			return;
		}
	}
}


// Check all the base pointers to see if they have died
void scrvUpdateBasePointers(void)
{
	SDWORD		i;
	INTERP_VAL	*psVal;
	BASE_OBJECT	*psObj;

	for(i=0; i<MAX_BASEPOINTER; i++)
	{
		if (asBasePointers[i] != NULL)
		{
			psVal = asBasePointers[i];
			psObj = psVal->v.oval;

			if (psObj && psObj->died && psObj->died != NOT_CURRENT_LIST)
			{
				psVal->v.oval = NULL;
			}
		}
	}
}


// create a group structure for a ST_GROUP variable
BOOL scrvNewGroup(INTERP_VAL *psVal)
{
	DROID_GROUP		*psGroup;

	if (!grpCreate(&psGroup))
	{
		return FALSE;
	}

	// increment the refcount so the group doesn't get automatically freed when empty
	grpJoin(psGroup, NULL);

	psVal->v.oval = psGroup;

	return TRUE;
}


// release a ST_GROUP variable
void scrvReleaseGroup(INTERP_VAL *psVal)
{
	DROID_GROUP		*psGroup;

	psGroup = psVal->v.oval;
	grpReset(psGroup);

	ASSERT( psGroup->refCount == 1,
		"scrvReleaseGroup: ref count is wrong" );

	// do a final grpLeave to free the group
	grpLeave(psGroup, NULL);
}


// Get a context from the list
BOOL scrvGetContext(char *pID, SCRIPT_CONTEXT **ppsContext)
{
	SCRV_STORE	*psCurr;

	for(psCurr=psContextStore; psCurr; psCurr=psCurr->psNext)
	{
		if (strcmp(psCurr->pIDString, pID) == 0)
		{
			*ppsContext = psCurr->psContext;
			return TRUE;
		}
	}

	debug( LOG_ERROR, "scrvGetContext: couldn't find context for id: %s", pID );
	abort();
	return FALSE;
}


// Find a base object from it's id
BOOL scrvGetBaseObj(UDWORD id, BASE_OBJECT **ppsObj)
{
	//UDWORD			i;
	//UDWORD			player;
	BASE_OBJECT		*psObj;


	psObj = getBaseObjFromId(id);
	*ppsObj = psObj;

	if (psObj == NULL)
	{
		return FALSE;
	}
	return TRUE;
}

// Find a string from it's (string)id
BOOL scrvGetString(char *pStringID, char **ppString)
{
	UDWORD			id;

	//get the ID for the string
	if (!strresGetIDNum(psStringRes, pStringID, &id))
	{
		debug( LOG_ERROR, "Cannot find the string id %s ", pStringID );
		abort();
		return FALSE;
	}
	//get the string from the id
	*ppString = strresGetString(psStringRes, id);

	return TRUE;
}

// Link any object types to the actual pointer values
/*BOOL scrvLinkValues(void)
{
	SCRV_STORE	*psCurr;
	UDWORD		index;
	SCRIPT_CODE		*psCode;
	SCRIPT_CONTEXT	*psContext;
	INTERP_VAL		*psVal;
	BASE_OBJECT		*psObj;
	SDWORD			compIndex;
	char			*pStr;

	for(psCurr = psContextStore; psCurr; psCurr=psCurr->psNext)
	{
		psContext = psCurr->psContext;
		psCode = psContext->psCode;
		for(index=0; index<psCode->numGlobals; index++)
		{
			switch (psCode->pGlobals[index])
			{
			case ST_DROID:
				if (!eventGetContextVal(psContext, index, &psVal))
				{
					DBERROR(("scrvLinkValues: Droid variable not found"));
					return FALSE;
				}
				if (!scrvGetBaseObj((UDWORD)psVal->v.ival, &psObj))
				{
					DBERROR(("scrvLinkValues: Droid id %d not found", psVal->v.ival));
					return FALSE;
				}
				if (psObj->type != OBJ_DROID)
				{
					DBERROR(("scrvLinkValues: Object id %d is not a droid"));
					return FALSE;
				}
				else
				{
					psVal->v.oval = psObj;
				}
				break;
			case ST_STRUCTURE:
				if (!eventGetContextVal(psContext, index, &psVal))
				{
					DBERROR(("scrvLinkValues: Structure variable not found"));
					return FALSE;
				}
				if (!scrvGetBaseObj((UDWORD)psVal->v.ival, &psObj))
				{
					DBERROR(("scrvLinkValues: Structure id %d not found", psVal->v.ival));
					return FALSE;
				}
				if (psObj->type != OBJ_STRUCTURE)
				{
					DBERROR(("scrvLinkValues: Object id %d is not a structure"));
					return FALSE;
				}
				else
				{
					psVal->v.oval = psObj;
				}
				break;
			case ST_FEATURE:
				if (!eventGetContextVal(psContext, index, &psVal))
				{
					DBERROR(("scrvLinkValues: Feature variable not found"));
					return FALSE;
				}
				if (!scrvGetBaseObj((UDWORD)psVal->v.ival, &psObj))
				{
					DBERROR(("scrvLinkValues: Feature id %d not found", psVal->v.ival));
					return FALSE;
				}
				if (psObj->type != OBJ_FEATURE)
				{
					DBERROR(("scrvLinkValues: Object id %d is not a Feature"));
					return FALSE;
				}
				else
				{
					psVal->v.oval = psObj;
				}
				break;
			case ST_BODY:
				if (!eventGetContextVal(psContext, index, &psVal))
				{
					DBERROR(("scrvLinkValues: body variable not found"));
					return FALSE;
				}
				compIndex = getCompFromName(COMP_BODY, psVal->v.sval);
				if (compIndex == -1)
				{
					DBERROR(("scrvLinkValues: body component %s not found",
						psVal->v.sval));
					return FALSE;
				}
				FREE(psVal->v.sval);
				psVal->v.ival = compIndex;
				break;
			case ST_PROPULSION:
				if (!eventGetContextVal(psContext, index, &psVal))
				{
					DBERROR(("scrvLinkValues: propulsion variable not found"));
					return FALSE;
				}
				compIndex = getCompFromName(COMP_PROPULSION, psVal->v.sval);
				if (compIndex == -1)
				{
					DBERROR(("scrvLinkValues: propulsion component %s not found",
						psVal->v.sval));
					return FALSE;
				}
				FREE(psVal->v.sval);
				psVal->v.ival = compIndex;
				break;
			case ST_ECM:
				if (!eventGetContextVal(psContext, index, &psVal))
				{
					DBERROR(("scrvLinkValues: ECM variable not found"));
					return FALSE;
				}
				compIndex = getCompFromName(COMP_ECM, psVal->v.sval);
				if (compIndex == -1)
				{
					DBERROR(("scrvLinkValues: ECM component %s not found",
						psVal->v.sval));
					return FALSE;
				}
				FREE(psVal->v.sval);
				psVal->v.ival = compIndex;
				break;
			case ST_SENSOR:
				if (!eventGetContextVal(psContext, index, &psVal))
				{
					DBERROR(("scrvLinkValues: sensor variable not found"));
					return FALSE;
				}
				compIndex = getCompFromName(COMP_SENSOR, psVal->v.sval);
				if (compIndex == -1)
				{
					DBERROR(("scrvLinkValues: sensor component %s not found",
						psVal->v.sval));
					return FALSE;
				}
				FREE(psVal->v.sval);
				psVal->v.ival = compIndex;
				break;
			case ST_CONSTRUCT:
				if (!eventGetContextVal(psContext, index, &psVal))
				{
					DBERROR(("scrvLinkValues: construct variable not found"));
					return FALSE;
				}
				compIndex = getCompFromName(COMP_CONSTRUCT, psVal->v.sval);
				if (compIndex == -1)
				{
					DBERROR(("scrvLinkValues: construct component %s not found",
						psVal->v.sval));
					return FALSE;
				}
				FREE(psVal->v.sval);
				psVal->v.ival = compIndex;
				break;
			case ST_WEAPON:
				if (!eventGetContextVal(psContext, index, &psVal))
				{
					DBERROR(("scrvLinkValues: weapon variable not found"));
					return FALSE;
				}
				compIndex = getCompFromName(COMP_WEAPON, psVal->v.sval);
				if (compIndex == -1)
				{
					DBERROR(("scrvLinkValues: weapon component %s not found",
						psVal->v.sval));
					return FALSE;
				}
				FREE(psVal->v.sval);
				psVal->v.ival = compIndex;
				break;
			default:
				// These values do not need to be linked
				break;
			}
		}

		// See if we have to run the script
		if (psCurr->type == SCRV_EXEC)
		{
			eventRunContext(psCurr->psContext, gameTime/SCR_TICKRATE);
		}
	}

	return TRUE;
}*/




