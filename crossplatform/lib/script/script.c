/*
 * Script.c
 *
 * A few general functions for the script library
 */

#include "frame.h"
#include "script.h"
#include <assert.h>
							

// Flags for storing function indexes
#define FUNC_SETBIT		0x80000000		// set for a set function, clear otherwise
#define FUNC_LISTMASK	0x70000000
#define FUNC_INDEXMASK	0x0fffffff
#define FUNC_INSTINCT	0x00000000		// instinct function
#define FUNC_CALLBACK	0x40000000		// callback function
#define FUNC_OBJVAR		0x10000000		// object variable
#define FUNC_EXTERNAL	0x20000000		// external variable

// Header on a binary save file
typedef struct _binary_hdr
{
	UBYTE		aType[4];
	UDWORD		version;
	UDWORD		size;
	UWORD		debugInfo;		// number of debug entries in the file
	UWORD		numVars;		// number of global variables
	UWORD		numArrays;		// number of arrays
	UWORD		numTriggers;	// number of triggers
	UWORD		numEvents;		// number of events
	UDWORD		arraySize;		// total number of value entries used by the arrays
} BINARY_HDR;


#ifdef NOSCRIPT
// Stuff ripped out of SCRIPT_Y.c which is needed even when we aren't compiling the scripts

/* The table of instinct function type definitions */
FUNC_SYMBOL		*asScrInstinctTab;

/* The table of external variables and their access functions */
VAR_SYMBOL		*asScrExternalTab;

/* The table of object variables and their access functions. */
VAR_SYMBOL		*asScrObjectVarTab;

/* The table of callback triggers */
CALLBACK_SYMBOL	*asScrCallbackTab;

/* The table of user types */
TYPE_SYMBOL		*asScrTypeTab;


/* Set the function table */
void scriptSetFuncTab(FUNC_SYMBOL *psFuncTab)
{
	asScrInstinctTab = psFuncTab;
}

/* Set the object variable table */
void scriptSetObjectTab(VAR_SYMBOL *psObjTab)
{
	asScrObjectVarTab = psObjTab;
}

/* Set the external variable table */
void scriptSetExternalTab(VAR_SYMBOL *psExtTab)
{
	asScrExternalTab = psExtTab;
}


/* Set the callback table */
void scriptSetCallbackTab(CALLBACK_SYMBOL *psCallTab)
{
	asScrCallbackTab = psCallTab;
}


void scriptSetTypeTab(TYPE_SYMBOL *psTypeTab)
{

	asScrTypeTab = psTypeTab;
}


#endif



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


// Save a binary version of a program
BOOL scriptSaveProg(SCRIPT_CODE *psProg, UDWORD *pSize, UBYTE **ppData)
{
	BOOL		debugInfo;
	UDWORD		i;
	BINARY_HDR	*psHdr;
	UBYTE		*pPos;
	UDWORD		*ip;
	UDWORD		alignVars,TabSize;
	FUNC_SYMBOL	*psFunc;
	CALLBACK_SYMBOL	*psCallback;

	VAR_SYMBOL  *psVar;
	OPCODE		opcode;
	UDWORD		data, saveFunc, fptr;
	UDWORD DbgSize=0;


	// Calculate the size of the saved code
	alignVars = (psProg->numGlobals % 2) == 0 ? psProg->numGlobals :
								psProg->numGlobals + 1;



	// Calculate the size of all the tables
	TabSize=sizeof(UWORD)*(psProg->numTriggers+1);
	TabSize+=((sizeof(UWORD)*(psProg->numEvents+1)) + (sizeof(SWORD)*psProg->numEvents));
	TabSize=(TabSize+3)&(~3);	// align the tables

	*pSize = sizeof(BINARY_HDR) + psProg->size +
			 sizeof(INTERP_TYPE) * psProg->numGlobals +
			 sizeof(ARRAY_DATA) * psProg->numArrays +
			 sizeof(TRIGGER_DATA) * psProg->numTriggers +
			 TabSize ;

	debugInfo = psProg->psDebug != NULL;
	if (debugInfo)
	{
		*pSize = *pSize + sizeof(SCRIPT_DEBUG) * psProg->debugEntries;
		for(i=0; i<psProg->numGlobals; i++)
		{
			*pSize += sizeof(STORAGE_TYPE) +
					  strlen(psProg->psVarDebug[i].pIdent) + 1;
		}
		for(i=0; i<psProg->numArrays; i++)
		{
			*pSize += sizeof(STORAGE_TYPE) +
					  strlen(psProg->psArrayDebug[i].pIdent) + 1;
		}
	}

	// Allocate the save buffer
	*ppData = MALLOC(*pSize);
	if (!*ppData)
	{
		DBERROR(("scriptSaveProg: out of memory"));
		return FALSE;
	}

	// Put in the header data
	psHdr = (BINARY_HDR *)*ppData;
	psHdr->aType[0] = 'c';
	psHdr->aType[1] = 'o';
	psHdr->aType[2] = 'd';
	psHdr->aType[3] = 'e';
	psHdr->version = 0;
	psHdr->size = psProg->size;
	if (debugInfo)
	{
		psHdr->debugInfo = psProg->debugEntries;
	}
	else
	{
		psHdr->debugInfo = 0;
	}
	psHdr->numVars = psProg->numGlobals;
	psHdr->numArrays = psProg->numArrays;
	psHdr->numTriggers = psProg->numTriggers;
	psHdr->numEvents = psProg->numEvents;
	psHdr->arraySize = psProg->arraySize;

	// Save the variable types
	pPos = *ppData + sizeof(BINARY_HDR);


//	DBPRINTF(("Header %d\n",sizeof(BINARY_HDR)));
	DbgSize+=sizeof(BINARY_HDR);


	memcpy(pPos, psProg->pGlobals, sizeof(INTERP_TYPE) * psProg->numGlobals);
	pPos += (sizeof(INTERP_TYPE) * psProg->numGlobals);


//	DBPRINTF(("globs %d\n", (sizeof(INTERP_TYPE) * psProg->numGlobals)));
	DbgSize+=(sizeof(INTERP_TYPE) * psProg->numGlobals);

	// save the array information
	memcpy(pPos, psProg->psArrayInfo, sizeof(ARRAY_DATA) * psProg->numArrays);
	pPos += sizeof(ARRAY_DATA) * psProg->numArrays;

	DbgSize += sizeof(ARRAY_DATA) * psProg->numArrays;

	// Save the trigger data
	memcpy(pPos, psProg->psTriggerData, sizeof(TRIGGER_DATA) * psProg->numTriggers);
	pPos += sizeof(TRIGGER_DATA) * psProg->numTriggers;

//	DBPRINTF(("Trigs %d\n",(sizeof(TRIGGER_DATA) * psProg->numTriggers)));
	DbgSize+= (sizeof(TRIGGER_DATA) * psProg->numTriggers);

	// Save the trigger and event tables
	//
	// tjc - These need to be saved as  numTriggers/Events +1 because we need to offsets for the end of the table 
	//-see interp.c line #144 -> #158
	memcpy(pPos, psProg->pTriggerTab, sizeof(UWORD) * (psProg->numTriggers+1));
	pPos += sizeof(UWORD) * (psProg->numTriggers+1);  // just to make sure pPos is on a DWORD boundary

//	DBPRINTF(("Trig2 %d\n",(sizeof(UWORD) * (psProg->numTriggers+1))));
	DbgSize+= (sizeof(UWORD) * (psProg->numTriggers+1));


	memcpy(pPos, psProg->pEventTab, sizeof(UWORD) * (psProg->numEvents+1));
	pPos += sizeof(UWORD) * (psProg->numEvents+1);

//	DBPRINTF(("Events %d\n",(sizeof(UWORD) * (psProg->numEvents+1))));
	DbgSize+= (sizeof(UWORD) * (psProg->numEvents+1));



	memcpy(pPos, psProg->pEventLinks, sizeof(SWORD) * psProg->numEvents);
	pPos += sizeof(SWORD) * psProg->numEvents;

//	DBPRINTF(("EventsLinks %d\n",(sizeof(SWORD) * (psProg->numEvents))));
	DbgSize+= (sizeof(SWORD) * (psProg->numEvents));


//	DBPRINTF(("Align [%d] ",pPos));
	pPos= (UBYTE *) ((((UDWORD)pPos)+3)&(~3));	// align up me landlord !
//	DBPRINTF((" [%d]\n",pPos));


	// Save the code
	ASSERT(((pPos - *ppData) % 4 == 0,
		"scriptSaveProg: data not DWORD aligned"));
	memcpy(pPos, psProg->pCode, psProg->size);

	// Now search the code for function pointers and replace
	// them with their position in the array
	ip = (UDWORD *)pPos;
	while (((UBYTE *)ip) < pPos + psProg->size)
	{

		BOOL MatchFound;

		opcode = (*ip) >> OPCODE_SHIFT;
		data = (*ip) & OPCODE_DATAMASK;
		switch (opcode)
		{
		case OP_CALL:
			fptr = (*(ip + 1));
			saveFunc = 0;
			MatchFound=FALSE;
			for(psFunc = asScrInstinctTab; psFunc->pFunc != NULL; psFunc++)
			{
				if (((UDWORD)psFunc->pFunc) == fptr)
				{
					// Found the function
					saveFunc = psFunc - asScrInstinctTab;
					saveFunc |= FUNC_INSTINCT;
					MatchFound=TRUE;
					break;
				}
			}
			if (MatchFound==FALSE)
			{
				for(psCallback = asScrCallbackTab; psCallback->type != 0; psCallback++)
				{
					if (((UDWORD)psCallback->pFunc) == fptr)
					{
						// Found the function
						saveFunc = psCallback - asScrCallbackTab;
						saveFunc |= FUNC_CALLBACK;
						MatchFound=TRUE;
						break;
					}
				}
			}
			ASSERT((MatchFound==TRUE,"scriptSaveProg: No Function match"));
			*(ip+1) = saveFunc;
			break;
		case OP_VARCALL:
			fptr = (*(ip + 1));
			saveFunc = 0;
			MatchFound=FALSE;
			for(psVar = asScrObjectVarTab; psVar->pIdent != NULL; psVar++)
			{
				if (((UDWORD)psVar->set) == fptr && psVar->index == data)
				{
					saveFunc = psVar - asScrObjectVarTab;
					saveFunc |= FUNC_SETBIT | FUNC_OBJVAR;
					MatchFound=TRUE;
					break;
				}
				else if (((UDWORD)psVar->get) == fptr && psVar->index == data)
				{
					saveFunc = psVar - asScrObjectVarTab;
					saveFunc |= FUNC_OBJVAR;
					MatchFound=TRUE;
					break;
				}
			}
			for(psVar = asScrExternalTab; psVar->pIdent != NULL; psVar++)
			{
				if (((UDWORD)psVar->set) == fptr && psVar->index == data)
				{
					saveFunc = psVar - asScrExternalTab;
					saveFunc |= FUNC_SETBIT | FUNC_EXTERNAL;
					MatchFound=TRUE;
					break;
				}
				else if (((UDWORD)psVar->get) == fptr && psVar->index == data)
				{
					saveFunc = psVar - asScrExternalTab;
					saveFunc |= FUNC_EXTERNAL;
					MatchFound=TRUE;
					break;
				}
			}
			ASSERT((MatchFound==TRUE,"scriptSaveProg: No var match"));
			*(ip+1) = saveFunc;			
			break;
		default:
			break;
		}
		ip += aOpSize[opcode];
	}
	pPos += psProg->size;
//	DBPRINTF(("Prog %d\n",(psProg->size)));
	DbgSize+= (psProg->size);

	// Now store the debug info
	if (debugInfo)
	{
		UDWORD DbgISize=0;
		
		SCRIPT_DEBUG *SavedDebug,*IngameDebug;


		// Copy each debug entry individually so that we can clear the pLabel pointer
		SavedDebug=(SCRIPT_DEBUG *)pPos;
		IngameDebug=psProg->psDebug;
		for (i=0;i<psProg->debugEntries ;i++)
		{
			memcpy(SavedDebug,IngameDebug,sizeof(SCRIPT_DEBUG));
			SavedDebug->pLabel=NULL;	// remove the ascii label pointers from the saved binary info
			SavedDebug++;
			IngameDebug++;
		}


		pPos += sizeof(SCRIPT_DEBUG) * psProg->debugEntries;
//		DBPRINTF(("DbgInfo %d\n",(sizeof(SCRIPT_DEBUG) * psProg->debugEntries)));
		DbgSize+= (sizeof(SCRIPT_DEBUG) * psProg->debugEntries);


		for(i=0; i<psProg->numGlobals; i++)
		{

			*((STORAGE_TYPE *)pPos) = psProg->psVarDebug[i].storage;
			pPos += sizeof(STORAGE_TYPE);

			DbgISize+=sizeof(STORAGE_TYPE);

			// We need to be able to handle NULL strings because the bastarding script compiler sometime generates them !!!!1
			if (psProg->psVarDebug[i].pIdent==NULL)
			{
				*pPos=0;			// Null string ... just store a zero
				pPos++;				// Move it on by one
				DbgISize++;
//				DBPRINTF(("%d) storage=%d size=2 - NULL STRING\n",i,psProg->psVarDebug[i].storage));
			}
			else
			{
				strcpy((STRING *)pPos, psProg->psVarDebug[i].pIdent);
				pPos += (strlen(psProg->psVarDebug[i].pIdent) + 1);
				DbgISize+= (strlen(psProg->psVarDebug[i].pIdent) + 1);
//				DBPRINTF(("%d) storage=%d size=%d [%s]\n",i,psProg->psVarDebug[i].storage,strlen(psProg->psVarDebug[i].pIdent)+1+sizeof(STORAGE_TYPE),psProg->psVarDebug[i].pIdent));
			}

		}

		for(i=0; i<psProg->numArrays; i++)
		{

			*((STORAGE_TYPE *)pPos) = psProg->psArrayDebug[i].storage;
			pPos += sizeof(STORAGE_TYPE);

			DbgISize+=sizeof(STORAGE_TYPE);

			// We need to be able to handle NULL strings because the bastarding script compiler sometime generates them !!!!1
			if (psProg->psArrayDebug[i].pIdent==NULL)
			{
				*pPos=0;			// Null string ... just store a zero
				pPos++;				// Move it on by one
				DbgISize++;
//				DBPRINTF(("%d) storage=%d size=2 - NULL STRING\n",i,psProg->psVarDebug[i].storage));
			}
			else
			{
				strcpy((STRING *)pPos, psProg->psArrayDebug[i].pIdent);
				pPos += (strlen(psProg->psArrayDebug[i].pIdent) + 1);
				DbgISize+= (strlen(psProg->psArrayDebug[i].pIdent) + 1);
//				DBPRINTF(("%d) storage=%d size=%d [%s]\n",i,psProg->psVarDebug[i].storage,strlen(psProg->psVarDebug[i].pIdent)+1+sizeof(STORAGE_TYPE),psProg->psVarDebug[i].pIdent));
			}

		}

//		DBPRINTF(("DbgISize %d\n",DbgISize));
		DbgSize+=DbgISize;
	}


//	DBPRINTF(("Total script size %d\n",DbgSize));


	return TRUE;
}

#define DBG1() ({DBPRINTF(("DbgSize=%d MemUsed=%d\n",DbgSize,pPos - pData));     })


// Load a binary version of a program
BOOL scriptLoadProg(UDWORD size, UBYTE *pData, SCRIPT_CODE **ppsProg)
{
	SCRIPT_CODE	*psProg;
	BOOL		debugInfo;
	UDWORD		i;
	BINARY_HDR	*psHdr;
	UBYTE		*pPos;
	UDWORD		*ip;
//	UDWORD		alignTriggers;
	OPCODE		opcode;
	UDWORD DbgSize=0;
	UDWORD		data, saveFunc;

	// Allocate the program structure
	psProg = MALLOC(sizeof(SCRIPT_CODE));
	if (!psProg)
	{
		DBERROR(("scriptLoadProg: out of memory"));
		return FALSE;
	}

	// Load the header data
	psHdr = (BINARY_HDR *)pData;
	if (psHdr->aType[0] != 'c' ||
		psHdr->aType[1] != 'o' ||
		psHdr->aType[2] != 'd' ||
		psHdr->aType[3] != 'e' ||
		psHdr->version != 0)
	{
		DBERROR(("scriptLoadProg: invalid file type/version"));
		return FALSE;
	}
	psProg->size = psHdr->size;
	psProg->debugEntries = psHdr->debugInfo;
	debugInfo = psProg->debugEntries > 0;
	psProg->numGlobals = psHdr->numVars;
	psProg->numArrays = psHdr->numArrays;
	psProg->numTriggers = psHdr->numTriggers;
	psProg->numEvents = psHdr->numEvents;
	psProg->arraySize = psHdr->arraySize;

	// need one extra trigger entry here (see save code for explaination -tjc )
//	alignTriggers = ((psProg->numTriggers+1) % 2) == 0 ? psProg->numTriggers+1 :
//								psProg->numTriggers+1+1;

	// Load the variable types
	pPos = pData + sizeof(BINARY_HDR);


//	DBPRINTF(("HEADER = %d\n",sizeof(BINARY_HDR)));
	DbgSize+=sizeof(BINARY_HDR);


//	DBG1();


	if (psProg->numGlobals > 0)
	{
		psProg->pGlobals = MALLOC(sizeof(INTERP_TYPE) * psProg->numGlobals);
		if (!psProg->pGlobals)
		{
			DBERROR(("scriptLoadProg: out of memory"));
			return FALSE;
		}
		memcpy(psProg->pGlobals, pPos, sizeof(INTERP_TYPE) * psProg->numGlobals);
		pPos += sizeof(INTERP_TYPE) * psProg->numGlobals;

//	DBPRINTF(("GLOBALS = %d\n", sizeof(INTERP_TYPE) * psProg->numGlobals));
//	DBG1();
	DbgSize+=( sizeof(INTERP_TYPE) * psProg->numGlobals);
	}
	else
	{
		psProg->pGlobals=NULL;
	}


	if (psProg->numArrays > 0)
	{
		// Load the array data
		psProg->psArrayInfo = MALLOC(sizeof(ARRAY_DATA) * psProg->numArrays);
		if (psProg->psArrayInfo == NULL)
		{
			DBERROR(("scriptLoadProg: out of memory"));
			return FALSE;
		}
		memcpy(psProg->psArrayInfo, pPos, sizeof(ARRAY_DATA) * psProg->numArrays);
		pPos += sizeof(ARRAY_DATA) * psProg->numArrays;

		DbgSize += sizeof(ARRAY_DATA) * psProg->numArrays;
	}

	// Load the trigger data
	psProg->psTriggerData = MALLOC(sizeof(TRIGGER_DATA) * psProg->numTriggers);
	if (!psProg->psTriggerData)
	{
		DBERROR(("scriptLoadProg: out of memory"));
		return FALSE;
	}
	memcpy(psProg->psTriggerData, pPos, sizeof(TRIGGER_DATA) * psProg->numTriggers);
	pPos += sizeof(TRIGGER_DATA) * psProg->numTriggers;


//	DBPRINTF(("TRIGGERS = %d\n",sizeof(TRIGGER_DATA) * psProg->numTriggers));
//	DBG1();
	DbgSize+=sizeof(TRIGGER_DATA) * psProg->numTriggers;


	// Load the trigger and event tables
	psProg->pTriggerTab = MALLOC(sizeof(UWORD) * (psProg->numTriggers+1));
	if (!psProg->pTriggerTab)
	{
		DBERROR(("scriptLoadProg: out of memory"));
		return FALSE;
	}
	memcpy(psProg->pTriggerTab, pPos, sizeof(UWORD) * (psProg->numTriggers+1));
	pPos += sizeof(UWORD) * (psProg->numTriggers+1); 

//	DBPRINTF(("TRIGTAB = %d\n",sizeof(UWORD) * (psProg->numTriggers+1))); 
//	DBG1();
	DbgSize+=(sizeof(UWORD) * (psProg->numTriggers+1)); 



	psProg->pEventTab = MALLOC(sizeof(UWORD) * (psProg->numEvents+1));
	if (!psProg->pEventTab)
	{
		DBERROR(("scriptLoadProg: out of memory"));
		return FALSE;
	}
	memcpy(psProg->pEventTab, pPos, sizeof(UWORD) * (psProg->numEvents+1));
	pPos += sizeof(UWORD) * (psProg->numEvents+1);


//	DBPRINTF(("EVENTTAB = %d\n",sizeof(UWORD) * (psProg->numEvents+1))); 
//	DBG1();
	DbgSize+=(sizeof(UWORD) * (psProg->numEvents+1)); 


	psProg->pEventLinks = MALLOC(sizeof(SWORD) * psProg->numEvents);
	if (!psProg->pEventLinks)
	{
		DBERROR(("scriptLoadProg: out of memory"));
		return FALSE;
	}
	memcpy(psProg->pEventLinks, pPos, sizeof(SWORD) * psProg->numEvents);
	pPos += sizeof(SWORD) * psProg->numEvents;


//	DBPRINTF(("EVENTLINKS = %d\n",sizeof(UWORD) * (psProg->numEvents))); 
//	DBG1();
	DbgSize+=(sizeof(SWORD) * psProg->numEvents); 



//	DBPRINTF(("ALIGNMENT PHASE: old=%p ",pPos));
	pPos= (UBYTE *) ((((UDWORD)pPos)+3)&(~3));	// align up me landlord !
//	DBPRINTF(("new=%p\n",pPos));

	// Load the code
	ASSERT(((pPos - pData) % 4 == 0,
		"scriptLoadProg: data not DWORD aligned"));




	psProg->pCode = MALLOC(psProg->size);
	if (!psProg->pCode)
	{
		DBERROR(("scriptLoadProg: out of memory"));
		return FALSE;
	}
	memcpy(psProg->pCode, pPos, psProg->size);

	// Now search the code for function pointers and replace
	// them with their position in the array
	ip = (UDWORD *)psProg->pCode;
	while (((UBYTE *)ip) < ((UBYTE *)psProg->pCode) + psProg->size)
	{
		opcode = (*ip) >> OPCODE_SHIFT;
		data = (*ip) & OPCODE_DATAMASK;
		switch (opcode)
		{
		case OP_CALL:
		case OP_VARCALL:
			saveFunc = *(ip + 1);
			switch (saveFunc & FUNC_LISTMASK)
			{
			case FUNC_CALLBACK:
				saveFunc = (UDWORD)asScrCallbackTab[saveFunc & FUNC_INDEXMASK].pFunc;
				break;
			case FUNC_INSTINCT:
				saveFunc = (UDWORD)asScrInstinctTab[saveFunc & FUNC_INDEXMASK].pFunc;
				break;
			case FUNC_OBJVAR:
				if (saveFunc & FUNC_SETBIT)
				{
					saveFunc = (UDWORD)asScrObjectVarTab[saveFunc & FUNC_INDEXMASK].set;
				}
				else
				{
					saveFunc = (UDWORD)asScrObjectVarTab[saveFunc & FUNC_INDEXMASK].get;
				}
				break;
			case FUNC_EXTERNAL:
				if (saveFunc & FUNC_SETBIT)
				{
					saveFunc = (UDWORD)asScrExternalTab[saveFunc & FUNC_INDEXMASK].set;
				}
				else
				{
					saveFunc = (UDWORD)asScrExternalTab[saveFunc & FUNC_INDEXMASK].get;
				}
				break;
			}
			*(ip+1) = saveFunc;
			break;
		default:
			break;
		}
		ip += aOpSize[opcode];
	}
	pPos += psProg->size;
																	  

//	DBPRINTF(("PROG = %d\n",(psProg->size))); 
	DbgSize+=((psProg->size)); 
//	DBG1();


	// Now load the debug info
	if (debugInfo)
	{

		UDWORD Tot=0;

		psProg->psDebug = MALLOC(sizeof(SCRIPT_DEBUG) * psProg->debugEntries);
		if (!psProg->psDebug)
		{
			DBERROR(("scriptLoadProg: out of memory"));
			return FALSE;
		}
		memcpy(psProg->psDebug, pPos, sizeof(SCRIPT_DEBUG) * psProg->debugEntries);
		pPos += sizeof(SCRIPT_DEBUG) * psProg->debugEntries;

		if (psProg->numGlobals > 0)
		{
			psProg->psVarDebug = MALLOC(sizeof(VAR_DEBUG) * psProg->numGlobals);
			if (!psProg->psVarDebug)
			{
				DBERROR(("scriptLoadProg: out of memory"));
				return FALSE;
			}
		}
		else
		{
			psProg->psVarDebug=NULL;
			psProg->pGlobals = NULL;
		}
		if (psProg->numArrays > 0)
		{
			psProg->psArrayDebug = MALLOC(sizeof(ARRAY_DEBUG) * psProg->numArrays);
			if (!psProg->psArrayDebug)
			{
				DBERROR(("scriptLoadProg: out of memory"));
				return FALSE;
			}
		}
		else
		{
			psProg->psArrayInfo = NULL;
			psProg->psArrayDebug = NULL;
		}

//		DBPRINTF(("DBGINFO = %d\n",sizeof(SCRIPT_DEBUG) * psProg->debugEntries));
		DbgSize+=(sizeof(SCRIPT_DEBUG) * psProg->debugEntries);
//	DBG1();
	

		for(i=0; i<psProg->numGlobals; i++)
		{

			UDWORD	StringSize;


			psProg->psVarDebug[i].storage = *((STORAGE_TYPE *)pPos);
			pPos += sizeof(STORAGE_TYPE);
			DbgSize+= sizeof(STORAGE_TYPE);
			Tot+= sizeof(STORAGE_TYPE);


			// Special check for null string
			if (*pPos==0)
			{
//				DBPRINTF(("String %d) is NULL\n",i));
				psProg->psVarDebug[i].pIdent = NULL;
				StringSize=1;
			}
			else
			{

				StringSize=strlen((STRING *)pPos)+1;

//				DBPRINTF(("String %d) size=%d [%s]\n",i,StringSize+sizeof(STORAGE_TYPE),(STRING *)pPos));

				psProg->psVarDebug[i].pIdent = MALLOC(StringSize);
				if (!psProg->psVarDebug[i].pIdent)
				{
					DBERROR(("scriptLoadProg: out of memory"));
					return FALSE;
				}
				strcpy(psProg->psVarDebug[i].pIdent, (STRING *)pPos);
			}
			pPos += StringSize;
			DbgSize+= StringSize;
			Tot+= StringSize;
//		DBG1();

		}

		for(i=0; i<psProg->numArrays; i++)
		{

			UDWORD	StringSize;


			psProg->psArrayDebug[i].storage = *((STORAGE_TYPE *)pPos);
			pPos += sizeof(STORAGE_TYPE);
			DbgSize+= sizeof(STORAGE_TYPE);
			Tot+= sizeof(STORAGE_TYPE);


			// Special check for null string
			if (*pPos==0)
			{
//				DBPRINTF(("String %d) is NULL\n",i));
				psProg->psArrayDebug[i].pIdent = NULL;
				StringSize=1;
			}
			else
			{

				StringSize=strlen((STRING *)pPos)+1;

//				DBPRINTF(("String %d) size=%d [%s]\n",i,StringSize+sizeof(STORAGE_TYPE),(STRING *)pPos));

				psProg->psArrayDebug[i].pIdent = MALLOC(StringSize);
				if (!psProg->psArrayDebug[i].pIdent)
				{
					DBERROR(("scriptLoadProg: out of memory"));
					return FALSE;
				}
				strcpy(psProg->psArrayDebug[i].pIdent, (STRING *)pPos);
			}
			pPos += StringSize;
			DbgSize+= StringSize;
			Tot+= StringSize;
//		DBG1();

		}

//		DBPRINTF(("DBGSTRING = %d\n",Tot));
//	DBG1();

	}
	else
	{
		psProg->psDebug = NULL;
		psProg->psVarDebug = NULL;
		psProg->psArrayDebug = NULL;
	}

	if (pPos - pData > (SDWORD)size)
	{
		DBERROR(("ERROR : scriptLoadProg: filesize (%d) does not match data size (%d) - sum=%d",size,pPos-pData,DbgSize));
		return FALSE;
	}

	*ppsProg = psProg;

	return TRUE;
}

/* Free a SCRIPT_CODE structure */
void scriptFreeCode(SCRIPT_CODE *psCode)
{
	UDWORD	i;

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


		



