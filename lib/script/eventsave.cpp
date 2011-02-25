/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
 * EvntSave.c
 *
 * Save the state of the event system.
 *
 */
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/endian_hack.h"
#include "lib/framework/string_ext.h"
#include "script.h"
#include "eventsave.h"


// the event save file header
struct EVENT_SAVE_HDR  // : public GAME_SAVEHEADER
{
	char		aFileType[4];
	UDWORD		version;
};


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
		if (!resGetHashfromData("SCRIPT", psCCont->psCode, &hashedName))
		{
			debug( LOG_FATAL, "eventSaveContext: couldn't find script resource id" );
			abort();
			return false;
		}
		numVars = psCCont->psCode->numGlobals + psCCont->psCode->arraySize;

		if (pBuffer != NULL)
		{
			*((UDWORD*)pPos) = (UDWORD)hashedName;
			endian_udword((UDWORD*)pPos);
			pPos += sizeof(UDWORD);

			*((SWORD*)pPos) = (SWORD)numVars;
			endian_sword((SWORD*)pPos);
			pPos += sizeof(SWORD);

			*pPos = (UBYTE)psCCont->release;
			pPos += sizeof(UBYTE);
		}

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
					endian_sword((SWORD*)pPos);

					pPos += sizeof(SWORD);
				}
				size += sizeof(SWORD);

				// store the variable value
				if (psVal->type == VAL_STRING)
				{
					UDWORD stringLen = 0;

					if(psVal->v.sval != NULL && strlen(psVal->v.sval) > 0)
					{
						stringLen = strlen(psVal->v.sval) + 1;
					}

					if (pBuffer != NULL)
					{
						*((UDWORD *)pPos) = stringLen;
						endian_udword((UDWORD *)pPos);
						pPos += sizeof(UDWORD);

						if(stringLen > 0)
						{
							strcpy((char *)pPos, psVal->v.sval);
						}
						pPos += stringLen;
					}

					size += sizeof(UDWORD) + stringLen;
				}
				else if (psVal->type < VAL_USERTYPESTART)
				{
					// internal type
					if (pBuffer != NULL)
					{
/* FIXME: this does not work for VAL_OBJ_GETSET, VAL_FUNC_EXTERN */
						*((UDWORD *)pPos) = (UDWORD)psVal->v.ival;
						endian_udword((UDWORD*)pPos);
						
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

					if (!saveFunc(psVal, pPos, &valSize))
					{
						debug( LOG_FATAL, "eventSaveContext: couldn't get variable value size" );
						abort();
						return false;
					}

					if (pBuffer != NULL)
					{
						*pValSize = (UWORD)valSize;
						endian_uword((UWORD*)pValSize);

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
		endian_sword((SWORD*)pBuffer);
	}
	*pSize = size;

	return true;
}

// load the context information for the script system
static BOOL eventLoadContext(const SDWORD version, char *pBuffer, UDWORD *pSize)
{
	UDWORD				size, valSize,stringLen;
	SDWORD				numVars, i, numContext, context;
	SCRIPT_CONTEXT		*psCCont;
	INTERP_TYPE			type;
	SCR_VAL_LOAD		loadFunc;
	char				*pPos;
	UDWORD				hashedName;
	SCRIPT_CODE			*psCode;
	CONTEXT_RELEASE			release;
	INTERP_VAL			*psVal, data;

	size = 0;
	pPos = pBuffer;

	// get the number of contexts in the save file
	endian_sword((SWORD*)pPos);
	numContext = *((SWORD *)pPos);
	pPos += sizeof(SWORD);
	size += sizeof(SWORD);

	// go through the contexts
	for(context=0; context < numContext; context += 1)
	{
    		endian_udword((UDWORD*)pPos);
    		hashedName = *((UDWORD*)pPos);
    		psCode = (SCRIPT_CODE*)resGetDataFromHash("SCRIPT", hashedName);
    		pPos += sizeof(UDWORD);

		// check the number of variables
		endian_sword((SWORD*)pPos);
   		numVars = psCode->numGlobals + psCode->arraySize;
        
		if (numVars != *((SWORD*)pPos))
		{
			ASSERT(false, "Context %d of %d: Number of context variables (%d) does not match the script code (%d)", 
			       context, numContext, numVars, *((SWORD*)pPos));
			return false;
		}
		pPos += sizeof(SWORD);

		release = (CONTEXT_RELEASE)*pPos;
		pPos += sizeof(UBYTE);

		// create the context
		if (!eventNewContext(psCode, release, &psCCont))
		{
			return false;
		}

		// bit of a hack this - note the id of the context to link it to the triggers
		psContList->id = (SWORD)context;

		size += sizeof(UDWORD) + sizeof(SWORD) + sizeof(UBYTE);

		// set the context variables
		for(i=0; i < numVars; i+= 1)
		{
			// get the variable type
			endian_sword((SWORD*)pPos);
			type = (INTERP_TYPE) *((SWORD*)pPos);
			pPos += sizeof(SWORD);
			size += sizeof(SWORD);

			// get the variable value
			if (type < VAL_USERTYPESTART)
			{
				data.type = type;

				endian_udword((UDWORD*)pPos);

				switch (type) {
				case VAL_BOOL:
					data.v.bval = *((BOOL*)pPos);
					pPos += sizeof(BOOL);
					size += sizeof(BOOL);
					break;
				case VAL_FLOAT:
					data.v.fval = *((float*)pPos);
					pPos += sizeof(float);
					size += sizeof(float);
					break;
				case VAL_INT:
				case VAL_TRIGGER:
				case VAL_EVENT:
				case VAL_VOID:
				case VAL_OPCODE:
				case VAL_PKOPCODE:
					data.v.ival = *((UDWORD *)pPos);
					pPos += sizeof(UDWORD);
					size += sizeof(UDWORD);
					break;
				case VAL_STRING:
					data.v.sval = (char*)malloc(MAXSTRLEN);
					strcpy(data.v.sval, "\0");

					stringLen = *((UDWORD *)pPos);	//read string length
					
					pPos += sizeof(UDWORD);
					size += sizeof(UDWORD);

					//load string
					if(stringLen > 0)
					{
						strlcpy(data.v.sval, (char *)pPos, MIN(stringLen + 1, MAXSTRLEN));
						pPos += stringLen;
						size += stringLen;
					}
					break;
				case VAL_OBJ_GETSET:
/* FIXME: saving pointer on disk! */
					data.v.pObjGetSet = *((SCRIPT_VARFUNC*)pPos);
					pPos += sizeof(SCRIPT_VARFUNC);
					size += sizeof(SCRIPT_VARFUNC);
					break;
				case VAL_FUNC_EXTERN:
/* FIXME: saving pointer on disk! */
					data.v.pFuncExtern = *((SCRIPT_FUNC*)pPos);
					pPos += sizeof(SCRIPT_FUNC);
					size += sizeof(SCRIPT_FUNC);
					break;
				default:
					ASSERT( false, "eventLoadContext: invalid internal type" );
				}

				// set the value in the context
				if (!eventSetContextVar(psCCont, (UDWORD)i, &data))
				{
					debug( LOG_FATAL, "eventLoadContext: couldn't set variable value" );
					abort();
					return false;
				}
			}
			else
			{
				// user defined type
				loadFunc = asScrTypeTab[type - VAL_USERTYPESTART].loadFunc;

				ASSERT( loadFunc != NULL,
					"eventLoadContext: no load function for type %d\n", type );

				endian_uword((UWORD*)pPos);
				valSize = *((UWORD *)pPos);

				pPos += sizeof(UWORD);
				size += sizeof(UWORD);

				// get the value pointer so that the loadFunc can write directly
				// into the variables data space.
				if (!eventGetContextVal(psCCont, (UDWORD)i, &psVal))
				{
					debug( LOG_FATAL, "eventLoadContext: couldn't find variable in context" );
					abort();
					return false;
				}

				if (!loadFunc(version, psVal, pPos, valSize))
				{
					debug( LOG_FATAL, "eventLoadContext: couldn't get variable value" );
					abort();
					return false;
				}

				pPos += valSize;
				size += valSize;
			}
		}
	}

	*pSize = size;

	return true;
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
			return true;
		}
		index += 1;
	}

	return false;
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
			return true;
		}
	}

	return false;
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
			endian_udword((UDWORD*)pPos);

			pPos += sizeof(UDWORD);
			if (!eventGetContextIndex(psCurr->psContext, &context))
			{
				debug( LOG_FATAL, "eventSaveTriggerList: couldn't find context" );
				abort();
				return false;
			}
			*((SWORD*)pPos) = (SWORD)context;
			endian_sword((SWORD*)pPos);
			pPos += sizeof(SWORD);
			*((SWORD*)pPos) = psCurr->type;
			endian_sword((SWORD*)pPos);
			pPos += sizeof(SWORD);
			*((SWORD*)pPos) = psCurr->trigger;
			endian_sword((SWORD*)pPos);
			pPos += sizeof(SWORD);
			*((UWORD*)pPos) = psCurr->event;
			endian_uword((UWORD*)pPos);
			pPos += sizeof(UWORD);
			*((UWORD*)pPos) = psCurr->offset;
			endian_uword((UWORD*)pPos);
			pPos += sizeof(UWORD);
		}
		size += sizeof(UDWORD) + sizeof(SWORD)*3 + sizeof(UWORD)*2;
	}
	if (pBuffer != NULL)
	{
		*((SDWORD*)pBuffer) = numTriggers;
		endian_sdword((SDWORD*)pBuffer);
	}

	*pSize = size;

	return true;
}


// load a list of triggers
static BOOL eventLoadTriggerList(WZ_DECL_UNUSED const SDWORD version, char *pBuffer, UDWORD *pSize)
{
	UDWORD				size, event, offset, time;
	char				*pPos;
	SDWORD				numTriggers, context, type, trigger, i;
	SCRIPT_CONTEXT		*psContext;

	size = 0;
	pPos = pBuffer;

	// get the number of triggers
	endian_sdword((SDWORD*)pPos);
	numTriggers = *((SDWORD*)pPos);
	pPos += sizeof(SDWORD);
	size += sizeof(SDWORD);

	for(i=0; i<numTriggers; i+= 1)
	{
		endian_udword((UDWORD*)pPos);
		time = *((UDWORD*)pPos);
		pPos += sizeof(UDWORD);

		endian_sword((SWORD*)pPos);
		context = *((SWORD*)pPos);
		pPos += sizeof(SWORD);
		if (!eventFindContext(context, &psContext))
		{
			debug( LOG_FATAL, "eventLoadTriggerList: couldn't find context" );
			abort();
			return false;
		}

		endian_sword((SWORD*)pPos);
		type = *((SWORD*)pPos);
		pPos += sizeof(SWORD);

		endian_sword((SWORD*)pPos);
		trigger = *((SWORD*)pPos);
		pPos += sizeof(SWORD);

		endian_uword((UWORD*)pPos);
		event = *((UWORD*)pPos);
		pPos += sizeof(UWORD);

		endian_uword((UWORD*)pPos);
		offset = *((UWORD*)pPos);
		pPos += sizeof(UWORD);

		size += sizeof(UDWORD) + sizeof(SWORD)*3 + sizeof(UWORD)*2;

		if (!eventLoadTrigger(time, psContext, type, trigger, event, offset))
		{
			return false;
		}
	}

	*pSize = size;

	return true;
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
		return false;
	}
	totalSize += size;

	// find the size of the trigger save
	if (!eventSaveTriggerList(psTrigList, NULL, &size))
	{
		return false;
	}
	totalSize += size;

	// find the size of the callback trigger save
	if (!eventSaveTriggerList(psCallbackList, NULL, &size))
	{
		return false;
	}
	totalSize += size;

	// Allocate the buffer to save to
	pBuffer = (char*)malloc(totalSize);
	if (pBuffer == NULL)
	{
		debug( LOG_FATAL, "eventSaveState: out of memory" );
		abort();
		return false;
	}
	pPos = pBuffer;

	// set the header
	psHdr = (EVENT_SAVE_HDR *)pPos;
	psHdr->aFileType[0] = 'e';
	psHdr->aFileType[1] = 'v';
	psHdr->aFileType[2] = 'n';
	psHdr->aFileType[3] = 't';
	psHdr->version = version;
	endian_udword(&psHdr->version);

	pPos += sizeof(EVENT_SAVE_HDR);

	// save the contexts
	if (!eventSaveContext(pPos, &size))
	{
		return false;
	}
	pPos += size;

	// save the triggers
	if (!eventSaveTriggerList(psTrigList, pPos, &size))
	{
		return false;
	}
	pPos += size;

	// save the callback triggers
	if (!eventSaveTriggerList(psCallbackList, pPos, &size))
	{
		return false;
	}
	pPos += size;

	*ppBuffer = pBuffer;
	*pFileSize = totalSize;

	return true;
}


// Load the state of the event system
BOOL eventLoadState(char *pBuffer, UDWORD fileSize)
{
	UDWORD			size, totalSize, version;
	char			*pPos;
	EVENT_SAVE_HDR	*psHdr;

	pPos = pBuffer;
	totalSize = 0;

	// Get the header
	psHdr = (EVENT_SAVE_HDR *)pPos;
	endian_udword(&psHdr->version);
	ASSERT_OR_RETURN(false, strncmp(psHdr->aFileType, "evnt", 4) == 0, "Invalid file header");
	version = psHdr->version;
	pPos += sizeof(EVENT_SAVE_HDR);
	totalSize += sizeof(EVENT_SAVE_HDR);

	// load the event contexts
	if (!eventLoadContext(version, pPos, &size))
	{
		return false;
	}

	pPos += size;
	totalSize += size;

	// load the normal triggers
	if (!eventLoadTriggerList(version, pPos, &size))
	{
		return false;
	}
	pPos += size;
	totalSize += size;

	// load the callback triggers
	if (!eventLoadTriggerList(version, pPos, &size))
	{
		return false;
	}
	pPos += size;
	totalSize += size;

	if (totalSize != fileSize)
	{
		debug( LOG_FATAL, "eventLoadState: corrupt save file" );
		abort();
		return false;
	}

	return true;
}
