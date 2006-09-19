/*
 * EvntSave.c
 *
 * Save the state of the event system.
 *
 */

#include "lib/framework/frame.h"
#include "script.h"
#include "evntsave.h"


// the event save file header
typedef struct _event_save_header
{
	STRING		aFileType[4];
	UDWORD		version;
} EVENT_SAVE_HDR;


// save the context information for the script system
static BOOL eventSaveContext(char *pBuffer, UDWORD *pSize)
{
	UDWORD				size, valSize;
	SDWORD				numVars, i, numContext;
	SCRIPT_CONTEXT		*psCCont;
	VAL_CHUNK			*psCVals;
	INTERP_VAL			*psVal;
	SCR_VAL_SAVE		saveFunc;
	char				*pPos;
//not hashed	STRING				*pScriptID;
	UDWORD				hashedName;
	UWORD				*pValSize = NULL;


	size = 0;
	numContext = 0;
	pPos = pBuffer;

	// reserve space to store how many contexts are saved
	if (pBuffer != NULL)
	{
		pPos += sizeof(SWORD);
	}
	size += sizeof(SWORD);

	// go through the context list
	for(psCCont = psContList; psCCont != NULL; psCCont = psCCont->psNext)
	{
		numContext += 1;

		// save the context info
//nothashed if (!resGetIDfromData("SCRIPT", psCCont->psCode, &hashedName))
		if (!resGetHashfromData("SCRIPT", psCCont->psCode, &hashedName))
		{
			debug( LOG_ERROR, "eventSaveContext: couldn't find script resource id" );
			abort();
			return FALSE;
		}
		numVars = psCCont->psCode->numGlobals + psCCont->psCode->arraySize;

		if (pBuffer != NULL)
		{
//not hashed			strcpy(pPos, pScriptID);
//not hashed			pPos += strlen(pScriptID) + 1;
			*((UDWORD*)pPos) = (UDWORD)hashedName;
			pPos += sizeof(UDWORD);

			*((SWORD*)pPos) = (SWORD)numVars;
			pPos += sizeof(SWORD);

			*pPos = (UBYTE)psCCont->release;
			pPos += sizeof(UBYTE);
		}

//not hashed		size += strlen(pScriptID) + 1 + sizeof(SWORD) + sizeof(UBYTE);
		size += sizeof(UDWORD) + sizeof(SWORD) + sizeof(UBYTE);

		// save the context variables
		for(psCVals = psCCont->psGlobals; psCVals != NULL; psCVals = psCVals->psNext)
		{
			for(i=0; i < CONTEXT_VALS; i+= 1)
			{
				psVal = psCVals->asVals + i;

				// store the variable type
				if (pBuffer != NULL)
				{
					ASSERT( psVal->type < SWORD_MAX,
						"eventSaveContext: variable type number too big" );
					*((SWORD*)pPos) = (SWORD)psVal->type;
					pPos += sizeof(SWORD);
				}
				size += sizeof(SWORD);

				// store the variable value
				if (psVal->type < VAL_USERTYPESTART)
				{
					// internal type - just store the DWORD value
					if (pBuffer != NULL)
					{
						*((UDWORD *)pPos) = (UDWORD)psVal->v.ival;	//TODO: make it save strings properly
						pPos += sizeof(UDWORD);
					}

					size += sizeof(UDWORD);
				}
				else
				{
					// user defined type
					saveFunc = asScrTypeTab[psVal->type - VAL_USERTYPESTART].saveFunc;

					ASSERT( saveFunc != NULL,
						"eventSaveContext: no save function for type %d\n", psVal->type );

					// reserve some space to store how many bytes the value uses
					if (pBuffer != NULL)
					{
						pValSize = (UWORD *)pPos;
						pPos += sizeof(UWORD);
					}
					size += sizeof(UWORD);

					if (!saveFunc(psVal->type, (UDWORD)psVal->v.ival, pPos, &valSize))
					{
						debug( LOG_ERROR, "eventSaveContext: couldn't get variable value size" );
						abort();
						return FALSE;
					}

					if (pBuffer != NULL)
					{
						*pValSize = (UWORD)valSize;
						pPos += valSize;
					}
					size += valSize;
				}

				numVars -=1;
				if (numVars <= 0)
				{
					// done all the variables
					ASSERT( psCVals->psNext == NULL,
						"eventSaveContext: number of context variables does not match the script code" );
					break;
				}
			}
		}
		ASSERT( numVars == 0,
			"eventSaveContext: number of context variables does not match the script code" );
	}

	// actually store how many contexts have been saved
	if (pBuffer != NULL)
	{
		*((SWORD *)pBuffer) = (SWORD)numContext;
	}
	*pSize = size;

	return TRUE;
}

// load the context information for the script system
static BOOL eventLoadContext(SDWORD version, char *pBuffer, UDWORD *pSize)
{
	UDWORD				size, valSize, data;
	SDWORD				numVars, i, numContext, context;
	SCRIPT_CONTEXT		*psCCont;
	INTERP_TYPE			type;
	SCR_VAL_LOAD		loadFunc;
	char				*pPos;
	STRING				*pScriptID;
	SCRIPT_CODE			*psCode;
	CONTEXT_RELEASE			release;
	INTERP_VAL			*psVal;

	size = 0;
	pPos = pBuffer;

	// get the number of contexts in the save file
	numContext = *((SWORD *)pPos);
	pPos += sizeof(SWORD);
	size += sizeof(SWORD);

	// go through the contexts
	for(context=0; context < numContext; context += 1)
	{
		// get the script code
		pScriptID = (STRING *)pPos;
		psCode = (SCRIPT_CODE*)resGetData("SCRIPT", pScriptID);
		pPos += strlen(pScriptID) + 1;

		// check the number of variables
		numVars = psCode->numGlobals + psCode->arraySize;
		if (numVars != *((SWORD*)pPos))
		{
			debug( LOG_ERROR, "eventLoadContext: number of context variables does not match the script code" );
			abort();
			return FALSE;
		}
		pPos += sizeof(SWORD);

		release = (CONTEXT_RELEASE)*pPos;
		pPos += sizeof(UBYTE);

		// create the context
		if (!eventNewContext(psCode, release, &psCCont))
		{
			return FALSE;
		}

		// bit of a hack this - note the id of the context to link it to the triggers
		psContList->id = (SWORD)context;

		size += strlen(pScriptID) + 1 + sizeof(SWORD) + sizeof(UBYTE);

		// set the context variables
		for(i=0; i < numVars; i+= 1)
		{
			// get the variable type
			type = (INTERP_TYPE)*pPos;
			pPos += sizeof(SWORD);
			size += sizeof(SWORD);

			// get the variable value
			if (type < VAL_USERTYPESTART)
			{
				// internal type - just get the DWORD value
				data = *((UDWORD *)pPos);
				pPos += sizeof(UDWORD);
				size += sizeof(UDWORD);

				// set the value in the context
				if (!eventSetContextVar(psCCont, (UDWORD)i, type, data))
				{
					debug( LOG_ERROR, "eventLoadContext: couldn't set variable value" );
					abort();
					return FALSE;
				}
			}
			else
			{
				// user defined type
				loadFunc = asScrTypeTab[type - VAL_USERTYPESTART].loadFunc;

				ASSERT( loadFunc != NULL,
					"eventLoadContext: no load function for type %d\n", type );

				valSize = *((UWORD *)pPos);
				pPos += sizeof(UWORD);
				size += sizeof(UWORD);

				// get the value pointer so that the loadFunc can write directly
				// into the variables data space.
				if (!eventGetContextVal(psCCont, (UDWORD)i, &psVal))
				{
					debug( LOG_ERROR, "eventLoadContext: couldn't find variable in context" );
					abort();
					return FALSE;
				}

				if (!loadFunc(version, type, pPos, valSize, (UDWORD *)&(psVal->v.ival)))
				{
					debug( LOG_ERROR, "eventLoadContext: couldn't get variable value" );
					abort();
					return FALSE;
				}

				pPos += valSize;
				size += valSize;
			}
		}
	}

	*pSize = size;

	return TRUE;
}

// load the context information for the script system
static BOOL eventLoadContextHashed(SDWORD version, char *pBuffer, UDWORD *pSize)
{
	UDWORD				size, valSize, data;
	SDWORD				numVars, i, numContext, context;
	SCRIPT_CONTEXT		*psCCont;
	INTERP_TYPE			type;
	SCR_VAL_LOAD		loadFunc;
	char				*pPos;
//not hashed	STRING				*pScriptID;
	UDWORD				hashedName;
	SCRIPT_CODE			*psCode;
	CONTEXT_RELEASE			release;
	INTERP_VAL			*psVal;

	size = 0;
	pPos = pBuffer;

	// get the number of contexts in the save file
	numContext = *((SWORD *)pPos);
	pPos += sizeof(SWORD);
	size += sizeof(SWORD);

	// go through the contexts
	for(context=0; context < numContext; context += 1)
	{
		// get the script code
//notHashed		pScriptID = (STRING *)pPos;
//notHashed		psCode = resGetData("SCRIPT", pScriptID);
//notHashed		pPos += strlen(pScriptID) + 1;
		hashedName = *((UDWORD*)pPos);
		pPos += sizeof(UDWORD);
		psCode = (SCRIPT_CODE*)resGetDataFromHash("SCRIPT", hashedName);


		// check the number of variables
		numVars = psCode->numGlobals + psCode->arraySize;
		if (numVars != *((SWORD*)pPos))
		{
			debug( LOG_ERROR, "eventLoadContext: number of context variables does not match the script code" );
			abort();
			return FALSE;
		}
		pPos += sizeof(SWORD);

		release = (CONTEXT_RELEASE)*pPos;
		pPos += sizeof(UBYTE);

		// create the context
		if (!eventNewContext(psCode, release, &psCCont))
		{
			return FALSE;
		}

		// bit of a hack this - note the id of the context to link it to the triggers
		psContList->id = (SWORD)context;

		size += sizeof(UDWORD) + sizeof(SWORD) + sizeof(UBYTE);

		// set the context variables
		for(i=0; i < numVars; i+= 1)
		{
			// get the variable type
			type = (INTERP_TYPE)*pPos;
			pPos += sizeof(SWORD);
			size += sizeof(SWORD);

			// get the variable value
			if (type < VAL_USERTYPESTART)
			{
				// internal type - just get the DWORD value
				data = *((UDWORD *)pPos);
				pPos += sizeof(UDWORD);
				size += sizeof(UDWORD);

				// set the value in the context
				if (!eventSetContextVar(psCCont, (UDWORD)i, type, data))
				{
					debug( LOG_ERROR, "eventLoadContext: couldn't set variable value" );
					abort();
					return FALSE;
				}
			}
			else
			{
				// user defined type
				loadFunc = asScrTypeTab[type - VAL_USERTYPESTART].loadFunc;

				ASSERT( loadFunc != NULL,
					"eventLoadContext: no load function for type %d\n", type );

				valSize = *((UWORD *)pPos);
				pPos += sizeof(UWORD);
				size += sizeof(UWORD);

				// get the value pointer so that the loadFunc can write directly
				// into the variables data space.
				if (!eventGetContextVal(psCCont, (UDWORD)i, &psVal))
				{
					debug( LOG_ERROR, "eventLoadContext: couldn't find variable in context" );
					abort();
					return FALSE;
				}

				if (!loadFunc(version, type, pPos, valSize, (UDWORD *)&(psVal->v.ival)))
				{
					debug( LOG_ERROR, "eventLoadContext: couldn't get variable value" );
					abort();
					return FALSE;
				}

				pPos += valSize;
				size += valSize;
			}
		}
	}

	*pSize = size;

	return TRUE;
}

// return the index of a context
static BOOL eventGetContextIndex(SCRIPT_CONTEXT *psContext, SDWORD *pIndex)
{
	SCRIPT_CONTEXT	*psCurr;
	SDWORD			index;

	index = 0;
	for(psCurr=psContList; psCurr!= NULL; psCurr=psCurr->psNext)
	{
		if (psCurr == psContext)
		{
			*pIndex = index;
			return TRUE;
		}
		index += 1;
	}

	return FALSE;
}

// find a context from it's id number
static BOOL eventFindContext(SDWORD id, SCRIPT_CONTEXT **ppsContext)
{
	SCRIPT_CONTEXT	*psCurr;

	for(psCurr=psContList; psCurr!= NULL; psCurr=psCurr->psNext)
	{
		if (psCurr->id == id)
		{
			*ppsContext = psCurr;
			return TRUE;
		}
	}

	return FALSE;
}

// save a list of triggers
static BOOL eventSaveTriggerList(ACTIVE_TRIGGER *psList, char *pBuffer, UDWORD *pSize)
{
	ACTIVE_TRIGGER		*psCurr;
	UDWORD				size;
	char				*pPos;
	SDWORD				numTriggers, context;

	size = 0;
	pPos = pBuffer;

	// reserve some space for the number of triggers
	if (pBuffer != NULL)
	{
		pPos += sizeof(SDWORD);
	}
	size += sizeof(SDWORD);

	numTriggers = 0;
	for(psCurr = psList; psCurr != NULL; psCurr = psCurr->psNext)
	{
		numTriggers += 1;

		if (pBuffer != NULL)
		{
			*((UDWORD*)pPos) = psCurr->testTime;
			pPos += sizeof(UDWORD);
			if (!eventGetContextIndex(psCurr->psContext, &context))
			{
				debug( LOG_ERROR, "eventSaveTriggerList: couldn't find context" );
				abort();
				return FALSE;
			}
			*((SWORD*)pPos) = (SWORD)context;
			pPos += sizeof(SWORD);
			*((SWORD*)pPos) = psCurr->type;
			pPos += sizeof(SWORD);
			*((SWORD*)pPos) = psCurr->trigger;
			pPos += sizeof(SWORD);
			*((UWORD*)pPos) = psCurr->event;
			pPos += sizeof(UWORD);
			*((UWORD*)pPos) = psCurr->offset;
			pPos += sizeof(UWORD);
		}
		size += sizeof(UDWORD) + sizeof(SWORD)*3 + sizeof(UWORD)*2;
	}
	if (pBuffer != NULL)
	{
		*((SDWORD*)pBuffer) = numTriggers;
	}

	*pSize = size;

	return TRUE;
}


// load a list of triggers
static BOOL eventLoadTriggerList(SDWORD version, char *pBuffer, UDWORD *pSize)
{
	UDWORD				size, event, offset, time;
	char				*pPos;
	SDWORD				numTriggers, context, type, trigger, i;
	SCRIPT_CONTEXT		*psContext;

	version = version;

	size = 0;
	pPos = pBuffer;

	// get the number of triggers
	numTriggers = *((SDWORD*)pPos);
	pPos += sizeof(SDWORD);
	size += sizeof(SDWORD);

	for(i=0; i<numTriggers; i+= 1)
	{
		time = *((UDWORD*)pPos);
		pPos += sizeof(UDWORD);

		context = *((SWORD*)pPos);
		pPos += sizeof(SWORD);
		if (!eventFindContext(context, &psContext))
		{
			debug( LOG_ERROR, "eventLoadTriggerList: couldn't find context" );
			abort();
			return FALSE;
		}

		type = *((SWORD*)pPos);
		pPos += sizeof(SWORD);

		trigger = *((SWORD*)pPos);
		pPos += sizeof(SWORD);

		event = *((UWORD*)pPos);
		pPos += sizeof(UWORD);

		offset = *((UWORD*)pPos);
		pPos += sizeof(UWORD);

		size += sizeof(UDWORD) + sizeof(SWORD)*3 + sizeof(UWORD)*2;

		if (!eventLoadTrigger(time, psContext, type, trigger, event, offset))
		{
			return FALSE;
		}
	}

	*pSize = size;

	return TRUE;
}


// Save the state of the event system
BOOL eventSaveState(SDWORD version, char **ppBuffer, UDWORD *pFileSize)
{
	UDWORD			size, totalSize;
	char			*pBuffer, *pPos;
	EVENT_SAVE_HDR	*psHdr;

	totalSize = sizeof(EVENT_SAVE_HDR);

	// find the size of the context save
	if (!eventSaveContext(NULL, &size))
	{
		return FALSE;
	}
	totalSize += size;

	// find the size of the trigger save
	if (!eventSaveTriggerList(psTrigList, NULL, &size))
	{
		return FALSE;
	}
	totalSize += size;

	// find the size of the callback trigger save
	if (!eventSaveTriggerList(psCallbackList, NULL, &size))
	{
		return FALSE;
	}
	totalSize += size;



	// Allocate the buffer to save to
	pBuffer = (char*)MALLOC(totalSize);
	if (pBuffer == NULL)
	{
		debug( LOG_ERROR, "eventSaveState: out of memory" );
		abort();
		return FALSE;
	}
	pPos = pBuffer;


	// set the header
	psHdr = (EVENT_SAVE_HDR *)pPos;
	psHdr->aFileType[0] = 'e';
	psHdr->aFileType[1] = 'v';
	psHdr->aFileType[2] = 'n';
	psHdr->aFileType[3] = 't';
	psHdr->version = version;

	pPos += sizeof(EVENT_SAVE_HDR);


	// save the contexts
	if (!eventSaveContext(pPos, &size))
	{
		return FALSE;
	}
	pPos += size;

	// save the triggers
	if (!eventSaveTriggerList(psTrigList, pPos, &size))
	{
		return FALSE;
	}
	pPos += size;

	// save the callback triggers
	if (!eventSaveTriggerList(psCallbackList, pPos, &size))
	{
		return FALSE;
	}
	pPos += size;

	*ppBuffer = pBuffer;
	*pFileSize = totalSize;

	return TRUE;
}


// Load the state of the event system
BOOL eventLoadState(char *pBuffer, UDWORD fileSize, BOOL bHashed)
{
	UDWORD			size, totalSize, version;
	char			*pPos;
	EVENT_SAVE_HDR	*psHdr;


	pPos = pBuffer;
	totalSize = 0;

	// Get the header
	psHdr = (EVENT_SAVE_HDR *)pPos;
	if (strncmp(psHdr->aFileType, "evnt", 4) != 0)
	{
		debug( LOG_ERROR, "eventLoadState: invalid file header" );
		abort();
		return FALSE;
	}
/*	if ((psHdr->version != 1) &&
		(psHdr->version != 2))
	{
		DBERROR(("eventLoadState: invalid file version"));
		return FALSE;
	}*/
	version = psHdr->version;
	pPos += sizeof(EVENT_SAVE_HDR);
	totalSize += sizeof(EVENT_SAVE_HDR);


	// load the event contexts
	if (bHashed)
	{
		if (!eventLoadContextHashed(version, pPos, &size))
		{
			return FALSE;
		}
	}
	else
	{
		if (!eventLoadContext(version, pPos, &size))
		{
			return FALSE;
		}
	}

	pPos += size;
	totalSize += size;

	// load the normal triggers
	if (!eventLoadTriggerList(version, pPos, &size))
	{
		return FALSE;
	}
	pPos += size;
	totalSize += size;

	// load the callback triggers
	if (!eventLoadTriggerList(version, pPos, &size))
	{
		return FALSE;
	}
	pPos += size;
	totalSize += size;

	if (totalSize != fileSize)
	{
		debug( LOG_ERROR, "eventLoadState: corrupt save file" );
		abort();
		return FALSE;
	}

	return TRUE;
}
