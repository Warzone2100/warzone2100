/*
 * Interp.c
 *
 * Execute the compiled version of a script
 *
 */

/* Control the execution trace printf's */
#define DEBUG_GROUP0
#include "lib/framework/frame.h"
#include "interp.h"
#include "stack.h"
#include "codeprint.h"
#include "script.h"
#include "event.h" //needed for eventGetEventID()


// the maximum number of instructions to execute before assuming
// an infinite loop
#define INTERP_MAXINSTRUCTIONS		100000



/* The size of each opcode */
SDWORD aOpSize[] =
{
	2,	// OP_PUSH | type, value
	2,	// OP_PUSHREF | type, value
	1,	// OP_POP

	1,	// OP_PUSHGLOBAL | var_num
	1,	// OP_POPGLOBAL | var_num

	1,	// OP_PUSHARRAYGLOBAL | array_dimensions | array_base
	1,	// OP_POPARRAYGLOBAL | array_dimensions | array_base

	2,	// OP_CALL, func_pointer
	2,  // OP_VARCALL, func_pointer

	1,	// OP_JUMP | offset
	1,  // OP_JUMPTRUE | offset
	1,	// OP_JUMPFALSE | offset

	1,  // OP_BINARYOP | secondary op
	1,  // OP_UNARYOP | secondary op

	1,	// OP_EXIT
	1,  // OP_PAUSE
};

/* The type equivalence table */
static TYPE_EQUIV	*asInterpTypeEquiv;

// whether the interpreter is running
static BOOL		bInterpRunning = FALSE;

/* Whether to output trace information */
BOOL	interpTrace;

/* Print out trace info if tracing is turned on */
#define TRCPRINTF(x) \
	if (interpTrace) \
		debug( LOG_NEVER, x)

#define TRCPRINTVAL(x) \
	if (interpTrace) \
		cpPrintVal(x)

#define TRCPRINTMATHSOP(x) \
	if (interpTrace) \
		cpPrintMathsOp(x)

#define TRCPRINTSTACKTOP() \
	if (interpTrace) \
		stackPrintTop()


#define TRCPRINTFUNC(x) \
	if (interpTrace) \
		cpPrintFunc(x)

#define TRCPRINTVARFUNC(x, data) \
	if (interpTrace) \
		cpPrintVarFunc(x, data)


// TRUE if the interpreter is currently running
BOOL interpProcessorActive(void)
{
	return bInterpRunning;
}

/* Find the value store for a global variable */
__inline INTERP_VAL *interpGetVarData(VAL_CHUNK *psGlobals, UDWORD index)
{
	VAL_CHUNK	*psChunk;

	psChunk = psGlobals;
	while (index >= CONTEXT_VALS)
	{
		index -= CONTEXT_VALS;
		psChunk = psChunk->psNext;
	}

	return psChunk->asVals + index;
}


// get the array data for an array operation
BOOL interpGetArrayVarData(UDWORD **pip, VAL_CHUNK *psGlobals, SCRIPT_CODE *psProg, INTERP_VAL **ppsVal)
{
	SDWORD		i, dimensions, vals[VAR_MAX_DIMENSIONS];
	UBYTE		*elements; //[VAR_MAX_DIMENSIONS]
	SDWORD		size, val;//, elementDWords;
//	UBYTE		*pElem;
	UDWORD		*ip = *pip;
	UDWORD		base, index;

	// get the base index of the array
	base = (*ip) & ARRAY_BASE_MASK;

	// get the number of dimensions
	dimensions = ((*ip) & ARRAY_DIMENSION_MASK) >> ARRAY_DIMENSION_SHIFT;

	if (base >= psProg->numArrays)
	{
		ASSERT( FALSE,
			"interpGetArrayVarData: array base index out of range" );
		return FALSE;
	}
	if (dimensions != psProg->psArrayInfo[base].dimensions)
	{
		ASSERT( FALSE,
			"interpGetArrayVarData: dimensions do not match" );
		return FALSE;
	}

	// get the number of elements for each dimension
	elements = psProg->psArrayInfo[base].elements;
/*	pElem = (UBYTE *) (ip + 1);
	for(i=0; i<dimensions; i+=1)
	{
		elements[i] = *pElem;

		pElem += 1;
	}*/

	// calculate the index of the array element
	size = 1;
	index = 0;
	for(i=dimensions-1; i>=0; i-=1)
	{
		if (!stackPopParams(1, VAL_INT, &val))
		{
			return FALSE;
		}

		if ( (val < 0) || (val >= elements[i]) )
		{
			ASSERT( FALSE, "interpGetArrayVarData: Array index for dimension %d out of range", i );
			return FALSE;
		}

		index += val * size;
		size *= psProg->psArrayInfo[base].elements[i];
		vals[i] = val;
	}

	// print out the debug trace
	if (interpTrace)
	{
		debug( LOG_NEVER, "%d->", base );
		for(i=0; i<dimensions; i+= 1)
		{
			debug( LOG_NEVER, "[%d/%d]", vals[i], elements[i] );
		}
		debug( LOG_NEVER, "(%d) ", index );
	}

	// check the index is valid
	if (index > psProg->arraySize)
	{
		ASSERT( FALSE, "interpGetArrayVarData: Array indexes out of variable space" );
		return FALSE;
	}

	// get the variable data
	*ppsVal = interpGetVarData(psGlobals, psProg->psArrayInfo[base].base + index);

	// calculate the number of DWORDs needed to store the number of elements for each dimension of the array
//	elementDWords = (dimensions - 1)/4 + 1;

	// update the insrtuction pointer
	*pip += 1;// + elementDWords;

	return TRUE;
}


// Initialise the interpreter
BOOL interpInitialise(void)
{
	asInterpTypeEquiv = NULL;

	return TRUE;
}

/* Run a compiled script */
BOOL interpRunScript(SCRIPT_CONTEXT *psContext, INTERP_RUNTYPE runType, UDWORD index, UDWORD offset)
{
	UDWORD		*ip, opcode, data;
	INTERP_VAL	sVal, *psVar;
	VAL_CHUNK	*psGlobals;
	UDWORD		numGlobals;
	UDWORD		*pCodeStart, *pCodeEnd, *pCodeBase;
	SCRIPT_FUNC		scriptFunc;
	SCRIPT_VARFUNC	scriptVarFunc;
	SCRIPT_CODE		*psProg;
//	SDWORD			arrayIndex, dimensions, arrayElements[VAR_MAX_DIMENSIONS];
	SDWORD			instructionCount = 0;

	UDWORD		CurEvent;
	BOOL		bStop,bEvent;
	SDWORD		callDepth;
	const STRING	*pTrigLab, *pEventLab;

	//debug(LOG_SCRIPT, "interpRunScript 1");

	ASSERT( PTRVALID(psContext, sizeof(SCRIPT_CONTEXT)),
		"interpRunScript: invalid context pointer" );
	psProg=psContext->psCode;
	ASSERT( PTRVALID(psProg, sizeof(SCRIPT_CODE)),
		"interpRunScript: invalid script code pointer" );


	if (bInterpRunning)
	{
		debug(LOG_ERROR,"interpRunScript: interpreter already running"
			"                 - callback being called from within a script function?");
		ASSERT( FALSE,
			"interpRunScript: interpreter already running"
			"                 - callback being called from within a script function?" );
		goto exit_with_error;
	}

	// note that the interpreter is running to stop recursive script calls
	bInterpRunning = TRUE;

	// Reset the stack in case another script messed up
	stackReset();

	//reset return stack
	retStackReset();

	// Turn off tracing initially
	interpTrace = FALSE;

	/* Get the global variables */
	numGlobals = psProg->numGlobals;
	psGlobals = psContext->psGlobals;

	bEvent = FALSE;

	// Find the code range
	switch (runType)
	{
	case IRT_TRIGGER:
		if (index > psProg->numTriggers)
		{
			debug(LOG_ERROR,"interpRunScript: trigger index out of range");
			ASSERT( FALSE, "interpRunScript: trigger index out of range" );
			return FALSE;
		}
		pCodeBase = psProg->pCode + psProg->pTriggerTab[index];
		pCodeStart = pCodeBase;
		pCodeEnd  = psProg->pCode + psProg->pTriggerTab[index+1];
		break;
	case IRT_EVENT:
		if (index > psProg->numEvents)
		{
			debug(LOG_ERROR,"interpRunScript: trigger index out of range");
			ASSERT( FALSE, "interpRunScript: trigger index out of range" );
			return FALSE;
		}
		pCodeBase = psProg->pCode + psProg->pEventTab[index];
		pCodeStart = pCodeBase + offset;		//offset only used for pause() script function
		pCodeEnd  = psProg->pCode + psProg->pEventTab[index+1];

		bEvent = TRUE; //remember it's an event
		break;
	default:
		debug(LOG_ERROR,"interpRunScript: unknown run type");
		ASSERT( FALSE, "interpRunScript: unknown run type" );
		return FALSE;
	}



	// Get the first opcode
	ip = pCodeStart;
	opcode = (*ip) >> OPCODE_SHIFT;
	instructionCount = 0;

	CurEvent = index;
	bStop = FALSE;

	//debug(LOG_SCRIPT, "interpRunScript 2");

	while(!bStop)
	{
		// Run the code
		if (ip < pCodeEnd)// && opcode != OP_EXIT)
		{
			if (instructionCount > INTERP_MAXINSTRUCTIONS)
			{
				debug( LOG_ERROR, "interpRunScript: max instruction count exceeded - infinite loop ?" );
				ASSERT( FALSE,
					"interpRunScript: max instruction count exceeded - infinite loop ?" );
				goto exit_with_error;
			}
			instructionCount += 1;

			TRCPRINTF(("%-6d  ", ip - psProg->pCode));
			opcode = (*ip) >> OPCODE_SHIFT;
			data = (*ip) & OPCODE_DATAMASK;
			switch (opcode)
			{
				/* Custom function call */
				case OP_FUNC:
					//debug( LOG_SCRIPT, "-OP_FUNC" );
					//debug( LOG_SCRIPT, "OP_FUNC: remember event %d, ip=%d", CurEvent, (ip + 2) );
					if(!RetStackRemember(CurEvent, (ip + 2)))	//Remember where to jump back later
					{
						debug( LOG_ERROR, "interpRunScript() - RetStackRemember() failed.");
						return FALSE;
					}

					CurEvent = *(ip+1);			//Current event = event to jump to



					if (CurEvent > psProg->numEvents)
					{
						debug( LOG_ERROR, "interpRunScript: trigger index out of range");
						return FALSE;
					}

					//Set new code execution boundries
					//----------------------------------
					pCodeBase = psProg->pCode + psProg->pEventTab[CurEvent];
					pCodeStart = pCodeBase;
					pCodeEnd  = psProg->pCode + psProg->pEventTab[CurEvent+1];

					ip = pCodeStart;				//Start at the beginning of the new event

					//debug( LOG_SCRIPT, "-OP_FUNC: jumped to event %d; ip=%d, numLocalVars: %d", CurEvent, ip, psContext->psCode->numLocalVars[CurEvent] );

					//debug( LOG_SCRIPT, "-END OP_FUNC" );

					break;

				//handle local variables
			case OP_PUSHLOCAL:

				//debug( LOG_SCRIPT, "OP_PUSHLOCAL");
				//debug( LOG_SCRIPT, "OP_PUSHLOCAL, (CurEvent=%d, data =%d) num loc vars: %d; pushing: %d", CurEvent, data, psContext->psCode->numLocalVars[CurEvent], psContext->psCode->ppsLocalVarVal[CurEvent][data].v.ival);

				if (data >= psContext->psCode->numLocalVars[CurEvent])
				{
					debug(LOG_ERROR, "interpRunScript: OP_PUSHLOCAL: variable index out of range");
					ASSERT( FALSE, "interpRunScript: OP_PUSHLOCAL: variable index out of range" );
					goto exit_with_error;
				}

				//debug( LOG_SCRIPT, "OP_PUSHLOCAL 2");
				//debug(LOG_SCRIPT, "OP_PUSHLOCAL type: %d", psContext->psCode->ppsLocalVarVal[CurEvent][data].type);

				if (!stackPush( &(psContext->psCode->ppsLocalVarVal[CurEvent][data]) ))
				{
					debug(LOG_ERROR, "interpRunScript: OP_PUSHLOCAL: push failed");
					goto exit_with_error;
				}

				//debug( LOG_SCRIPT, "OP_PUSHLOCAL 3");

				ip += 1;//aOpSize[opcode];
				break;
			case OP_POPLOCAL:

				//debug( LOG_SCRIPT, "OP_POPLOCAL, event index: '%d', data: '%d'", CurEvent, data);
				//debug( LOG_SCRIPT, "OP_POPLOCAL, numLocalVars: '%d'", psContext->psCode->numLocalVars[CurEvent]);

				if (data >= psContext->psCode->numLocalVars[CurEvent])
				{
					debug(LOG_ERROR, "interpRunScript: OP_POPLOCAL: variable index out of range");
					ASSERT( FALSE, "interpRunScript: variable index out of range" );
					goto exit_with_error;
				}

				//debug( LOG_SCRIPT, "OP_POPLOCAL 2");
				//DbgMsg("OP_POPLOCAL type: %d, CurEvent=%d, data=%d", psContext->psCode->ppsLocalVarVal[CurEvent][data].type, CurEvent, data);

				if (!stackPopType( &(psContext->psCode->ppsLocalVarVal[CurEvent][data]) ))
				{
					debug(LOG_ERROR, "interpRunScript: OP_POPLOCAL: pop failed");
					goto exit_with_error;
				}
				//debug(LOG_SCRIPT, "OP_POPLOCAL: type=%d, val=%d", psContext->psCode->ppsLocalVarVal[CurEvent][data].type, psContext->psCode->ppsLocalVarVal[CurEvent][data].v.ival);

				//debug( LOG_SCRIPT, "OP_POPLOCAL 3");
				ip += 1; //aOpSize[opcode];

				break;

			case OP_PUSHLOCALREF:

				// The type of the variable is stored in with the opcode
				sVal.type = (*ip) & OPCODE_DATAMASK;

				/* get local var index */
				data = *(ip + 1);

				if (data >= psContext->psCode->numLocalVars[CurEvent])
				{
					debug(LOG_ERROR, "interpRunScript: OP_PUSHLOCALREF: variable index out of range");
					ASSERT( FALSE, "interpRunScript: OP_PUSHLOCALREF: variable index out of range" );
					goto exit_with_error;
				}
				/* get local variable */
				psVar = &(psContext->psCode->ppsLocalVarVal[CurEvent][data]);

				sVal.v.oval = &(psVar->v.ival);

				TRCPRINTF(("PUSHREF     "));
				TRCPRINTVAL(&sVal);
				TRCPRINTF(("\n"));

				if (!stackPush(&sVal))
				{
					debug(LOG_ERROR, "interpRunScript: OP_PUSHLOCALREF: push failed");
					goto exit_with_error;
				}
				ip += 2;
				break;

			case OP_PUSH:
				// The type of the value is stored in with the opcode
				sVal.type = (*ip) & OPCODE_DATAMASK;
				// Copy the data as a DWORD
				sVal.v.ival = (SDWORD)(*(ip+1));
				TRCPRINTF(("PUSH        "));
				TRCPRINTVAL(&sVal);
				TRCPRINTF(("\n"));
				if (!stackPush(&sVal))
				{
					// Eeerk, out of memory
					debug( LOG_ERROR, "interpRunScript: out of memory!" );
					ASSERT( FALSE, "interpRunScript: out of memory!" );
					goto exit_with_error;
				}
				ip += aOpSize[opcode];
				break;
			case OP_PUSHREF:
				// The type of the variable is stored in with the opcode
				sVal.type = (*ip) & OPCODE_DATAMASK;
				// store the pointer
				psVar = interpGetVarData(psGlobals, *(ip + 1));
				sVal.v.oval = &(psVar->v.ival);
				TRCPRINTF(("PUSHREF     "));
				TRCPRINTVAL(&sVal);
				TRCPRINTF(("\n"));
				if (!stackPush(&sVal))
				{
					// Eeerk, out of memory
					debug( LOG_ERROR, "interpRunScript: out of memory!" );
					ASSERT( FALSE, "interpRunScript: out of memory!" );
					goto exit_with_error;
				}
				ip += aOpSize[opcode];
				break;
			case OP_POP:
				TRCPRINTF(("POP\n"));
				if (!stackPop(&sVal))
				{
					debug( LOG_ERROR, "interpRunScript: could not do stack pop" );
					ASSERT( FALSE, "interpRunScript: could not do stack pop" );
					goto exit_with_error;
				}
				ip += aOpSize[opcode];
				break;
			case OP_BINARYOP:
				TRCPRINTMATHSOP(data);
				if (!stackBinaryOp((OPCODE)data))
				{
					debug( LOG_ERROR, "interpRunScript: could not do binary op" );
					ASSERT( FALSE, "interpRunScript: could not do binary op" );
					goto exit_with_error;
				}
				TRCPRINTSTACKTOP();
				TRCPRINTF(("\n"));
				ip += aOpSize[opcode];
				break;
			case OP_UNARYOP:
				TRCPRINTMATHSOP(data);
				if (!stackUnaryOp((OPCODE)data))
				{
					debug( LOG_ERROR, "interpRunScript: could not do unary op" );
					ASSERT( FALSE, "interpRunScript: could not do unary op" );
					goto exit_with_error;
				}
				TRCPRINTSTACKTOP();
				TRCPRINTF(("\n"));
				ip += aOpSize[opcode];
				break;
			case OP_PUSHGLOBAL:
				TRCPRINTF(("PUSHGLOBAL  %d\n", data));
				if (data >= numGlobals)
				{
					debug( LOG_ERROR, "interpRunScript: variable index out of range" );
					ASSERT( FALSE, "interpRunScript: variable index out of range" );
					goto exit_with_error;
				}
				if (!stackPush(interpGetVarData(psGlobals, data)))
				{
					debug( LOG_ERROR, "interpRunScript: could not do stack push" );
					ASSERT( FALSE, "interpRunScript: could not do stack push" );
					goto exit_with_error;
				}
				ip += aOpSize[opcode];
				break;
			case OP_POPGLOBAL:
				TRCPRINTF(("POPGLOBAL   %d ", data));
				TRCPRINTSTACKTOP();
				TRCPRINTF(("\n"));
				if (data >= numGlobals)
				{
					debug( LOG_ERROR, "interpRunScript: variable index out of range" );
					ASSERT( FALSE, "interpRunScript: variable index out of range" );
					goto exit_with_error;
				}
				if (!stackPopType(interpGetVarData(psGlobals, data)))
				{
					debug( LOG_ERROR, "interpRunScript: could not do stack pop" );
					ASSERT( FALSE, "interpRunScript: could not do stack pop" );
					goto exit_with_error;
				}
				ip += aOpSize[opcode];
				break;
			case OP_PUSHARRAYGLOBAL:
				// get the number of array elements
	//			arrayElements = (data & ARRAY_ELEMENT_MASK) >> ARRAY_ELEMENT_SHIFT;
	//			data = data & ARRAY_INDEX_MASK;
				// get the array index
	//			if (!stackPopParams(1, VAL_INT, &arrayIndex))
	//			{
	//				goto exit_with_error;
	//			}
	//			TRCPRINTF(("PUSHARRAYGLOBAL  [%d] %d(+%d)\n", arrayIndex, data, arrayElements));
	//			if (data + arrayElements > numGlobals)
	//			{
	//				ASSERT( FALSE, "interpRunScript: variable index out of range" );
	//				goto exit_with_error;
	//			}
	//			if (arrayIndex < 0 || arrayIndex >= arrayElements)
	//			{
	//				ASSERT( FALSE, "interpRunScript: array index out of range" );
	//				goto exit_with_error;
	//			}
				TRCPRINTF(("PUSHARRAYGLOBAL  "));
				if (!interpGetArrayVarData(&ip, psGlobals, psProg, &psVar))
				{
					debug( LOG_ERROR, "interpRunScript: could not get array var data, CurEvent=%d", CurEvent );
					ASSERT( FALSE, "interpRunScript: could not get array var data" );
					goto exit_with_error;
				}
				TRCPRINTF(("\n"));
				if (!stackPush(psVar))
				{
					debug( LOG_ERROR, "interpRunScript: could not do stack push" );
					ASSERT( FALSE, "interpRunScript: could not do stack push" );
					goto exit_with_error;
				}
				break;
			case OP_POPARRAYGLOBAL:
				// get the number of array elements
	//			arrayElements = (data & ARRAY_ELEMENT_MASK) >> ARRAY_ELEMENT_SHIFT;
	//			data = data & ARRAY_INDEX_MASK;
				// get the array index
	/*			if (!stackPopParams(1, VAL_INT, &arrayIndex))
				{
					ASSERT( FALSE, "interpRunScript: could not do pop of params" );
					goto exit_with_error;
				}
				TRCPRINTF(("POPARRAYGLOBAL   [%d] %d(+%d) ", arrayIndex, data, arrayElements));
				TRCPRINTSTACKTOP();
				TRCPRINTF(("\n"));
				if (data + arrayElements > numGlobals)
				{
					ASSERT( FALSE, "interpRunScript: variable index out of range" );
					goto exit_with_error;
				}
				if (arrayIndex < 0 || arrayIndex >= arrayElements)
				{
					ASSERT( FALSE, "interpRunScript: array index out of range" );
					goto exit_with_error;
				}
				if (!stackPopType(interpGetVarData(psGlobals, data + arrayIndex)))
				{
					ASSERT( FALSE, "interpRunScript: could not do pop stack of type" );
					goto exit_with_error;
				}
				ip += aOpSize[opcode];*/
				TRCPRINTF(("POPARRAYGLOBAL   "));
				if (!interpGetArrayVarData(&ip, psGlobals, psProg, &psVar))
				{
					debug( LOG_ERROR, "interpRunScript: could not get array var data" );
					ASSERT( FALSE, "interpRunScript: could not get array var data" );
					goto exit_with_error;
				}
				TRCPRINTSTACKTOP();
				TRCPRINTF(("\n"));
				if (!stackPopType(psVar))
				{
					debug( LOG_ERROR, "interpRunScript: could not do pop stack of type" );
					ASSERT( FALSE, "interpRunScript: could not do pop stack of type" );
					goto exit_with_error;
				}
				break;
			case OP_JUMPFALSE:
				TRCPRINTF(("JUMPFALSE   %d (%d)",
						(SWORD)data, ip - psProg->pCode + (SWORD)data));
				if (!stackPop(&sVal))
				{
					debug( LOG_ERROR, "interpRunScript: could not do pop of stack" );
					ASSERT( FALSE, "interpRunScript: could not do pop of stack" );
					goto exit_with_error;
				}
				if (!sVal.v.bval)
				{
					// Do the jump
					TRCPRINTF((" - done -\n"));
					ip += (SWORD)data;
					if (ip < pCodeStart || ip > pCodeEnd)
					{
						debug( LOG_ERROR, "interpRunScript: jump out of range" );
						ASSERT( FALSE, "interpRunScript: jump out of range" );
						goto exit_with_error;
					}
				}
				else
				{
					TRCPRINTF(("\n"));
					ip += aOpSize[opcode];
				}
				break;
			case OP_JUMP:
				TRCPRINTF(("JUMP        %d (%d)\n",
						(SWORD)data, ip - psProg->pCode + (SWORD)data));
				// Do the jump
				ip += (SWORD)data;
				if (ip < pCodeStart || ip > pCodeEnd)
				{
					debug( LOG_ERROR, "interpRunScript: jump out of range" );
					ASSERT( FALSE, "interpRunScript: jump out of range" );
					goto exit_with_error;
				}
				break;
			case OP_CALL:
				//debug(LOG_SCRIPT, "OP_CALL");
				TRCPRINTFUNC( (SCRIPT_FUNC)(*(ip+1)) );
				TRCPRINTF(("\n"));
				scriptFunc = (SCRIPT_FUNC)*(ip+1);
				//debug(LOG_SCRIPT, "OP_CALL 1");
				if (!scriptFunc())
				{
					debug( LOG_ERROR, "interpRunScript: could not do func" );
					ASSERT( FALSE, "interpRunScript: could not do func" );
					goto exit_with_error;
				}
				//debug(LOG_SCRIPT, "OP_CALL 2");
				ip += aOpSize[opcode];
				//debug(LOG_SCRIPT, "OP_CALL 3");
				break;
			case OP_VARCALL:
				TRCPRINTF(("VARCALL     "));
				TRCPRINTVARFUNC( (SCRIPT_VARFUNC)(*(ip+1)), data );
				TRCPRINTF(("(%d)\n", data));
				scriptVarFunc = (SCRIPT_VARFUNC)*(ip+1);
				if (!scriptVarFunc(data))
				{
					debug( LOG_ERROR, "interpRunScript: could not do var func" );
					ASSERT( FALSE, "interpRunScript: could not do var func" );
					goto exit_with_error;
				}
				ip += aOpSize[opcode];
				break;
			case OP_EXIT:
				// jump out of the code
				ip = pCodeEnd;
				break;
			case OP_PAUSE:
				TRCPRINTF(("PAUSE       %d\n", data));
				ASSERT( stackEmpty(),
					"interpRunScript: OP_PAUSE without empty stack" );
				ip += aOpSize[opcode];
				// tell the event system to reschedule this event
				if (!eventAddPauseTrigger(psContext, index, ip - pCodeBase, data))
				{
					debug( LOG_ERROR, "interpRunScript: could not add pause trigger" );
					ASSERT( FALSE, "interpRunScript: could not add pause trigger" );
					goto exit_with_error;
				}
				// now jump out of the event
				ip = pCodeEnd;
				break;
			default:
				debug(LOG_ERROR, "interpRunScript: unknown opcode: %d", opcode);
				ASSERT( FALSE, "interpRunScript: unknown opcode: %d", opcode );
				goto exit_with_error;
				break;
			}
		}

		else	//End of the event reached, see if we have to jump back to the caller function or just exit
		{
			//debug(LOG_SCRIPT, "End of event reached");

			if(!IsRetStackEmpty())		//There was a caller function before this one
			{
				//debug(LOG_SCRIPT, "GetCallDepth = %d", GetCallDepth());

				//reset local vars (since trigger can't be called, only local vars of an event can be reset here)
				if(!resetLocalVars(psProg, CurEvent))
				{
					debug( LOG_ERROR, "interpRunScript: could not reset local vars for event %d", CurEvent );
					goto exit_with_error;
				}

				if(!PopRetStack((UDWORD *)&ip))		//Pop return address
				{
					debug( LOG_ERROR, "interpRunScript() - PopRetStack(): failed to pop return adress.");
					return FALSE;
				}

				//debug(LOG_SCRIPT, "Return adress = %d", ip);

 				if(!PopRetStack(&CurEvent))	//Pop event index
				{
					debug( LOG_ERROR, "interpRunScript() - PopRetStack(): failed to pop return event index.");
					return FALSE;
				}

				//debug( LOG_SCRIPT, "RETURNED TO CALLER EVENT %d", CurEvent );

				//Set new boundries
				//--------------------------
				if(IsRetStackEmpty())	//if we jumped back to the original caller
				{
					if(!bEvent)		//original caller was a trigger (is it possible at all?)
					{
						pCodeBase = psProg->pCode + psProg->pTriggerTab[CurEvent];
						pCodeStart = pCodeBase;
						pCodeEnd  = psProg->pCode + psProg->pTriggerTab[CurEvent+1];
					}
					else			//original caller was an event
					{
 						pCodeBase = psProg->pCode + psProg->pEventTab[CurEvent];
						pCodeStart = pCodeBase + offset;	//also use the offset passed, since it's an original caller event (offset is used for pause() )
						pCodeEnd  = psProg->pCode + psProg->pEventTab[CurEvent+1];
					}
				}
				else	//we are still jumping thru functions (this can't be a callback, since it can't/should not be called)
				{
 					pCodeBase = psProg->pCode + psProg->pEventTab[CurEvent];
					pCodeStart = pCodeBase;
					pCodeEnd  = psProg->pCode + psProg->pEventTab[CurEvent+1];
				}
			}
			else
			{
				//debug( LOG_SCRIPT, " *** CALL STACK EMPTY ***" );

				//reset local vars only if original caller was an event, not a trigger
				if(bEvent)
				{
					if(!resetLocalVars(psProg, index))
					{
						debug( LOG_ERROR, "interpRunScript: could not reset local vars" );
						goto exit_with_error;
					}
				}

				bStop = TRUE;		//Stop execution of this event here, no more calling functions stored
			}
		}

	}


	//debug(LOG_SCRIPT, "interpRunScript 3");


	TRCPRINTF(("%-6d  EXIT\n", ip - psProg->pCode));

	bInterpRunning = FALSE;
	return TRUE;

exit_with_error:
	// Deal with the script crashing or running out of memory
	debug(LOG_ERROR,"interpRunScript: *** ERROR EXIT *** (CurEvent=%d)", CurEvent);

	if(bEvent)
		debug(LOG_ERROR,"Original event ID: %d (of %d)", index, psProg->numEvents);
	else
		debug(LOG_ERROR,"Original trigger ID: %d (of %d)", index, psProg->numTriggers);

	debug(LOG_ERROR,"Current event ID: %d (of %d)", CurEvent, psProg->numEvents);
	callDepth = GetCallDepth();
	debug(LOG_ERROR,"Call depth : %d", callDepth);

	if(psProg->psDebug != NULL)
	{
		debug(LOG_ERROR,"Displaying debug info:");

		if(bEvent)
		{
			// find the debug info for the original (caller)  event
			pEventLab = eventGetEventID(psProg, index);
			debug(LOG_ERROR,"Original event name: %s", pEventLab);

			pEventLab = eventGetEventID(psProg, CurEvent);
			debug(LOG_ERROR,"Current event name: %s", pEventLab);
		}
		else
		{
			// find the debug info for the trigger
			pTrigLab = eventGetTriggerID(psProg, index);
			debug(LOG_ERROR,"Trigger: %s", pTrigLab);
		}
	}


	TRCPRINTF(("*** ERROR EXIT ***\n"));
	bInterpRunning = FALSE;
	return FALSE;
}


/* Set the type equivalence table */
void scriptSetTypeEquiv(TYPE_EQUIV *psTypeTab)
{
#ifdef DEBUG
	SDWORD		i;

	for(i=0; psTypeTab[i].base != 0; i++)
	{
		ASSERT( psTypeTab[i].base >= VAL_USERTYPESTART,
			"scriptSetTypeEquiv: can only set type equivalence for user types" );
	}
#endif

	asInterpTypeEquiv = psTypeTab;
}


/* Check if two types are equivalent */
BOOL interpCheckEquiv(INTERP_TYPE to, INTERP_TYPE from)
{
	SDWORD	i,j;
	BOOL	toRef = FALSE, fromRef = FALSE;

	// check for the VAL_REF flag
	if (to & VAL_REF)
	{
		toRef = TRUE;
		to = to & ~VAL_REF;
	}
	if (from & VAL_REF)
	{
		fromRef = TRUE;
		from = from & ~VAL_REF;
	}
	if (toRef != fromRef)
	{
		return FALSE;
	}

	if (to == from)
	{
		return TRUE;
	}
	else if (asInterpTypeEquiv)
	{
		for(i=0; asInterpTypeEquiv[i].base != 0; i++)
		{
			if (asInterpTypeEquiv[i].base == to)
			{
				for(j=0; j<asInterpTypeEquiv[i].numEquiv; j++)
				{
					if (asInterpTypeEquiv[i].aEquivTypes[j] == from)
					{
						return TRUE;
					}
				}
			}
		}
	}

	return FALSE;
}


/* Instinct function to turn on tracing */
BOOL interpTraceOn(void)
{
	interpTrace = TRUE;

	return TRUE;
}

/* Instinct function to turn off tracing */
BOOL interpTraceOff(void)
{
	interpTrace = FALSE;

	return TRUE;
}



/* Call stack stuff */

static UDWORD	retStack[200];	//Primitive stack
static SDWORD	retStackPos;	//Current Position, always points to the last valid element

void retStackReset(void)
{
	retStackPos = -1;		//Beginning of the stack
}

//Remember current script execution adress
UDWORD RetStackRemember(UDWORD EvTrigIndex, UDWORD address)
{
	//DbgMsg("To remember: %d, %d, %d", EvTrigIndex, address, retStackPos);

	if (retStackPos >= 200)
	{
		debug( LOG_ERROR, "RetStackRemember() - return address stack is full");
		return FALSE;	//Stack full
	}

	retStackPos = retStackPos + 2;
	retStack[retStackPos - 1] = EvTrigIndex;	//First store event index
	retStack[retStackPos] = address;			//current ip

	//debug( LOG_SCRIPT, "RetStackRemember: ip=%d, event=%d", address, EvTrigIndex);

	return TRUE;
}

BOOL IsRetStackEmpty(void)
{
	if(retStackPos == (-1)) return TRUE;
	return FALSE;
}

BOOL PopRetStack(UDWORD  *psVal)
{
	if(retStackPos < 0)
	{
		debug( LOG_ERROR, "PopRetStack: retStackPos < 0");
		return FALSE;
	}

	*psVal = retStack[retStackPos];

	retStackPos = retStackPos - 1;

	//debug( LOG_SCRIPT, "PopRetStack: val=%d", *psVal);

	return TRUE;
}

SDWORD GetCallDepth(void)
{
	return (retStackPos + 1) / 2;
}
