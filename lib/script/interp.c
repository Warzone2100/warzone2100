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
#define INTERP_MAXINSTRUCTIONS		200000

typedef struct
{
	UDWORD CallerIndex;
	INTERP_VAL *ReturnAddress;
} ReturnAddressStack_t;

/**
 * Reset the return address stack
 */
static inline void retStackReset(void);

/**
 * Check whether the return address stack is empty
 *
 * \return True when empty, false otherwise
 */
static inline BOOL retStackIsEmpty(void);

/**
 * Check whether the return address stack is full
 *
 * \return True when full, false otherwise
 */
static inline BOOL retStackIsFull(void);

/**
 * Push a new address/event pair on the return address stack
 *
 * \param CallerIndex Index of the calling function
 * \param ReturnAddress Address to return to
 * \return False on failure (stack full)
 */
static BOOL retStackPush(UDWORD CallerIndex, INTERP_VAL *ReturnAddress);

/**
 * Pop an address/event pair from the return address stack
 *
 * \param CallerIndex Index of the calling function
 * \param ReturnAddress Address to return to
 * \return False on failure (stack empty)
 */
static BOOL retStackPop(UDWORD *CallerIndex, INTERP_VAL **ReturnAddress);

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

	2,	// OP_CALL | func_pointer
	2,  // OP_VARCALL | func_pointer

	1,	// OP_JUMP | offset
	1,  // OP_JUMPTRUE | offset
	1,	// OP_JUMPFALSE | offset

	1,  // OP_BINARYOP | secondary op
	1,  // OP_UNARYOP | secondary op

	1,	// OP_EXIT
	1,  // OP_PAUSE

	-1, // OP_ADD
	-1, // OP_SUB
	-1, // OP_MUL
	-1, // OP_DIV
	-1, // OP_NEG
	-1, // OP_INC
	-1, // OP_DEC

	-1, // OP_AND
	-1, // OP_OR
	-1, // OP_NOT

	-1, // OP_CONC

	-1, // OP_EQUAL
	-1, // OP_NOTEQUAL
	-1, // OP_GREATEREQUAL
	-1, // OP_LESSEQUAL
	-1, // OP_GREATER
	-1, // OP_LESS

	2,  // OP_FUNC | func_pointer
	1,  // OP_POPLOCAL
	1,  // OP_PUSHLOCAL

	2,  // OP_PUSHLOCALREF
	1,	 //OP_TO_FLOAT
	1,  //OP_TO_INT
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
		debug( LOG_NEVER, (#x))

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
static inline INTERP_VAL *interpGetVarData(VAL_CHUNK *psGlobals, UDWORD index)
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
static BOOL interpGetArrayVarData(INTERP_VAL **pip, VAL_CHUNK *psGlobals, SCRIPT_CODE *psProg, INTERP_VAL **ppsVal)
{
	SDWORD		i, dimensions, vals[VAR_MAX_DIMENSIONS];
	UBYTE		*elements; //[VAR_MAX_DIMENSIONS]
	SDWORD		size, val;//, elementDWords;
//	UBYTE		*pElem;
	INTERP_VAL	*InstrPointer = *pip;
	UDWORD		base, index;

	// get the base index of the array
	base = InstrPointer->v.ival & ARRAY_BASE_MASK;

	// get the number of dimensions
	dimensions = (InstrPointer->v.ival & ARRAY_DIMENSION_MASK) >> ARRAY_DIMENSION_SHIFT;

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
			ASSERT( FALSE, "interpGetArrayVarData: Array index for dimension %d out of range (passed index = %d, max index = %d)", i , val, elements[i]);
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

	// update the instruction pointer
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
	SDWORD			data;
	OPCODE			opcode;
	INTERP_VAL	sVal, *psVar,*InstrPointer;
	VAL_CHUNK	*psGlobals;
	UDWORD		numGlobals = 0;
	INTERP_VAL		*pCodeStart, *pCodeEnd, *pCodeBase;
	SCRIPT_FUNC		scriptFunc = 0;
	SCRIPT_VARFUNC	scriptVarFunc = 0;
	SCRIPT_CODE		*psProg;
	SDWORD			instructionCount = 0;

	UDWORD		CurEvent = 0;
	BOOL		bStop = FALSE, bEvent = FALSE;
	SDWORD		callDepth = 0;
	const char	*pTrigLab, *pEventLab;
	BOOL			bTraceOn=FALSE;		//enable to debug function/event calls

	ASSERT( PTRVALID(psContext, sizeof(SCRIPT_CONTEXT)),
		"interpRunScript: invalid context pointer" );

	psProg = psContext->psCode;

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

		// find the debug info for the trigger
		strcpy(last_called_script_event, eventGetTriggerID(psProg, index));

		if(bTraceOn)
			debug(LOG_SCRIPT,"Trigger: %s", last_called_script_event);

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

		// remember last called event/function
		strcpy(last_called_script_event, eventGetEventID(psProg, index));

		if(bTraceOn)
			debug(LOG_SCRIPT,"Original event name: %s", last_called_script_event);

		break;
	default:
		debug(LOG_ERROR,"interpRunScript: unknown run type");
		ASSERT( FALSE, "interpRunScript: unknown run type" );
		return FALSE;
	}

	// Get the first opcode
	InstrPointer = pCodeStart;

	/* Make sure we start with an opcode */
	ASSERT(InstrPointer->type == VAL_PKOPCODE || InstrPointer->type == VAL_OPCODE,
		"Expected an opcode at the beginning of the interpreting process (type=%d)", InstrPointer->type);

	//opcode = InstrPointer->v.ival >> OPCODE_SHIFT;

	instructionCount = 0;

	CurEvent = index;
	bStop = FALSE;

	while(!bStop)
	{
		// Run the code
		if (InstrPointer < pCodeEnd)// && opcode != OP_EXIT)
		{
			if (instructionCount > INTERP_MAXINSTRUCTIONS)
			{
				debug( LOG_ERROR, "interpRunScript: max instruction count exceeded - infinite loop ?" );
				ASSERT( FALSE,
					"interpRunScript: max instruction count exceeded - infinite loop ?" );
				goto exit_with_error;
			}
			instructionCount++;

			TRCPRINTF(("%-6d  ", InstrPointer - psProg->pCode));
			opcode = (OPCODE)(InstrPointer->v.ival >> OPCODE_SHIFT);			//get opcode
			data = (SDWORD)(InstrPointer->v.ival & OPCODE_DATAMASK);		//get data - only used with packed opcodes
			switch (opcode)
			{
				/* Custom function call */
				case OP_FUNC:
					//debug( LOG_SCRIPT, "-OP_FUNC" );
					//debug( LOG_SCRIPT, "OP_FUNC: remember event %d, ip=%d", CurEvent, (ip + 2) );

					if(!retStackPush(CurEvent, (InstrPointer + aOpSize[opcode]))) //Remember where to jump back later
					{
						debug( LOG_ERROR, "interpRunScript() - retStackPush() failed.");
						return FALSE;
					}

					ASSERT(((INTERP_VAL *)(InstrPointer+1))->type == VAL_EVENT, "wrong value type passed for OP_FUNC: %d", ((INTERP_VAL *)(InstrPointer+1))->type);

					CurEvent = ((INTERP_VAL *)(InstrPointer+1))->v.ival; //Current event = event to jump to

					if (CurEvent > psProg->numEvents)
					{
						debug( LOG_ERROR, "interpRunScript: trigger index out of range");
						goto exit_with_error;
					}

					//Set new code execution boundaries
					//----------------------------------
					pCodeBase = psProg->pCode + psProg->pEventTab[CurEvent];
					pCodeStart = pCodeBase;
					pCodeEnd  = psProg->pCode + psProg->pEventTab[CurEvent+1];

					InstrPointer = pCodeStart;				//Start at the beginning of the new event

					//remember last called event/index
					strcpy(last_called_script_event, eventGetEventID(psProg, CurEvent));

					if(bTraceOn)
						debug(LOG_SCRIPT,"Called: '%s'", last_called_script_event);

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

				InstrPointer += aOpSize[opcode];
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
				InstrPointer += aOpSize[opcode];

				break;

			case OP_PUSHLOCALREF:

				// The type of the variable is stored in with the opcode
				sVal.type = (INTERP_TYPE)(InstrPointer->v.ival & OPCODE_DATAMASK);

				ASSERT(  ((INTERP_VAL *)(InstrPointer + 1))->type == VAL_INT,
					"wrong value type passed for OP_PUSHLOCALREF: %d", ((INTERP_VAL *)(InstrPointer + 1))->type);

				/* get local var index */
				data = ((INTERP_VAL *)(InstrPointer + 1))->v.ival;

				if (data >= psContext->psCode->numLocalVars[CurEvent])
				{
					debug(LOG_ERROR, "interpRunScript: OP_PUSHLOCALREF: variable index out of range");
					ASSERT( FALSE, "interpRunScript: OP_PUSHLOCALREF: variable index out of range" );
					goto exit_with_error;
				}
				/* get local variable */
				psVar = &(psContext->psCode->ppsLocalVarVal[CurEvent][data]);

				//TODO: fix
				sVal.v.oval = &(psVar->v.ival);

				TRCPRINTF(("PUSHREF     "));
				TRCPRINTVAL(&sVal);
				TRCPRINTF(("\n"));

				if (!stackPush(&sVal))
				{
					debug(LOG_ERROR, "interpRunScript: OP_PUSHLOCALREF: push failed");
					goto exit_with_error;
				}
				InstrPointer += aOpSize[opcode];
				break;

			case OP_PUSH:
				// The type of the value is stored in with the opcode
				sVal.type = (INTERP_TYPE)(InstrPointer->v.ival & OPCODE_DATAMASK);

				//ASSERT( ((INTERP_VAL *)(InstrPointer + 1))->type  == sVal.type,
				//	"wrong value type passed for OP_PUSH: %d, expected: %d", ((INTERP_VAL *)(InstrPointer + 1))->type, sVal.type );

				ASSERT(interpCheckEquiv(((INTERP_VAL *)(InstrPointer + 1))->type, sVal.type),
					"wrong value type passed for OP_PUSH: %d, expected: %d", ((INTERP_VAL *)(InstrPointer + 1))->type, sVal.type);

				/* copy value */
				memcpy(&sVal, (INTERP_VAL *)(InstrPointer + 1), sizeof(INTERP_VAL));

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
				InstrPointer += aOpSize[opcode];
				break;
			case OP_PUSHREF:
				// The type of the variable is stored in with the opcode
				sVal.type = (INTERP_TYPE)(InstrPointer->v.ival & OPCODE_DATAMASK);

				// store the pointer
				psVar = interpGetVarData(psGlobals, ((INTERP_VAL *)(InstrPointer + 1))->v.ival);
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
				InstrPointer += aOpSize[opcode];
				break;
			case OP_POP:
				ASSERT( InstrPointer->type == VAL_OPCODE,
					"wrong value type passed for OP_POP: %d", InstrPointer->type);

				TRCPRINTF(("POP\n"));
				if (!stackPop(&sVal))
				{
					debug( LOG_ERROR, "interpRunScript: could not do stack pop" );
					ASSERT( FALSE, "interpRunScript: could not do stack pop" );
					goto exit_with_error;
				}
				InstrPointer += aOpSize[opcode];
				break;
			case OP_BINARYOP:
				ASSERT( InstrPointer->type == VAL_PKOPCODE,
					"wrong value type passed for OP_BINARYOP: %d", InstrPointer->type);

				TRCPRINTMATHSOP(data);
				if (!stackBinaryOp((OPCODE)data))
				{
					debug( LOG_ERROR, "interpRunScript: could not do binary op" );
					ASSERT( FALSE, "interpRunScript: could not do binary op" );
					goto exit_with_error;
				}
				TRCPRINTSTACKTOP();
				TRCPRINTF(("\n"));
				InstrPointer += aOpSize[opcode];
				break;
			case OP_UNARYOP:
				ASSERT( InstrPointer->type == VAL_PKOPCODE,
					"wrong value type passed for OP_UNARYOP: %d", InstrPointer->type);

				TRCPRINTMATHSOP(data);
				if (!stackUnaryOp((OPCODE)data))
				{
					debug( LOG_ERROR, "interpRunScript: could not do unary op" );
					ASSERT( FALSE, "interpRunScript: could not do unary op" );
					goto exit_with_error;
				}
				TRCPRINTSTACKTOP();
				TRCPRINTF(("\n"));
				InstrPointer += aOpSize[opcode];
				break;
			case OP_PUSHGLOBAL:

				ASSERT( InstrPointer->type == VAL_PKOPCODE,
					"wrong value type passed for OP_PUSHGLOBAL: %d", InstrPointer->type);

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
				InstrPointer += aOpSize[opcode];
				break;
			case OP_POPGLOBAL:
				ASSERT( InstrPointer->type == VAL_PKOPCODE,
					"wrong value type passed for OP_POPGLOBAL: %d", InstrPointer->type);

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
				InstrPointer += aOpSize[opcode];
				break;
			case OP_PUSHARRAYGLOBAL:
				ASSERT( InstrPointer->type == VAL_PKOPCODE,
					"wrong value type passed for OP_PUSHARRAYGLOBAL: %d", InstrPointer->type);

				TRCPRINTF(("PUSHARRAYGLOBAL  "));
				if (!interpGetArrayVarData(&InstrPointer, psGlobals, psProg, &psVar))
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
				ASSERT( InstrPointer->type == VAL_PKOPCODE,
					"wrong value type passed for OP_POPARRAYGLOBAL: %d", InstrPointer->type);

				TRCPRINTF(("POPARRAYGLOBAL   "));
				if (!interpGetArrayVarData(&InstrPointer, psGlobals, psProg, &psVar))
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
				ASSERT( InstrPointer->type == VAL_PKOPCODE,
					"wrong value type passed for OP_JUMPFALSE: %d", InstrPointer->type);

				TRCPRINTF(("JUMPFALSE   %d (%d)",
						   (SWORD)data, InstrPointer - psProg->pCode + (SWORD)data));

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
					InstrPointer += (SWORD)data;
					if (InstrPointer < pCodeStart || InstrPointer > pCodeEnd)
					{
						debug( LOG_ERROR, "interpRunScript: jump out of range" );
						ASSERT( FALSE, "interpRunScript: jump out of range" );
						goto exit_with_error;
					}
				}
				else
				{
					TRCPRINTF(("\n"));
					InstrPointer += aOpSize[opcode];
				}
				break;
			case OP_JUMP:
				ASSERT( InstrPointer->type == VAL_PKOPCODE,
					"wrong value type passed for OP_JUMP: %d", InstrPointer->type);

				TRCPRINTF(("JUMP        %d (%d)\n",
						   (SWORD)data, InstrPointer - psProg->pCode + (SWORD)data));
				// Do the jump
				InstrPointer += (SWORD)data;
				if (InstrPointer < pCodeStart || InstrPointer > pCodeEnd)
				{
					debug( LOG_ERROR, "interpRunScript: jump out of range" );
					ASSERT( FALSE, "interpRunScript: jump out of range" );
					goto exit_with_error;
				}
				break;
			case OP_CALL:
				//debug(LOG_SCRIPT, "OP_CALL");

				ASSERT( InstrPointer->type == VAL_OPCODE,
					"wrong value type passed for OP_CALL: %d", InstrPointer->type);

				TRCPRINTFUNC(  ((INTERP_VAL *)(InstrPointer+1))->v.pFuncExtern );
				TRCPRINTF(("\n"));
				scriptFunc = ((INTERP_VAL *)(InstrPointer+1))->v.pFuncExtern;
				//debug(LOG_SCRIPT, "OP_CALL 1");
				if (!scriptFunc())
				{
					debug( LOG_ERROR, "interpRunScript: could not do func" );
					ASSERT( FALSE, "interpRunScript: could not do func" );
					goto exit_with_error;
				}
				//debug(LOG_SCRIPT, "OP_CALL 2");
				InstrPointer += aOpSize[opcode];
				//debug(LOG_SCRIPT, "OP_CALL 3");
				break;
			case OP_VARCALL:
				ASSERT( InstrPointer->type == VAL_PKOPCODE,
					"wrong value type passed for OP_VARCALL: %d", InstrPointer->type);

				TRCPRINTF(("VARCALL     "));
				TRCPRINTVARFUNC(  ((INTERP_VAL *)(InstrPointer+1))->v.pObjGetSet, data );
				TRCPRINTF(("(%d)\n", data));

				ASSERT( ((INTERP_VAL *)(InstrPointer+1))->type == VAL_OBJ_GETSET,
					"wrong set/get function pointer type passed for OP_VARCALL: %d", ((INTERP_VAL *)(InstrPointer+1))->type);

				scriptVarFunc =((INTERP_VAL *)(InstrPointer+1))->v.pObjGetSet;
				if (!scriptVarFunc(data))
				{
					debug( LOG_ERROR, "interpRunScript: could not do var func" );
					ASSERT( FALSE, "interpRunScript: could not do var func" );
					goto exit_with_error;
				}
				InstrPointer += aOpSize[opcode];
				break;
			case OP_EXIT:	/* end of function/event, "exit" or "return" statements */
				ASSERT( InstrPointer->type == VAL_OPCODE,
					"wrong value type passed for OP_EXIT: %d", InstrPointer->type);

				// jump out of the code
				InstrPointer = pCodeEnd;
				break;
			case OP_PAUSE:	
				ASSERT( InstrPointer->type == VAL_PKOPCODE,
					"wrong value type passed for OP_PAUSE: %d", InstrPointer->type);

				TRCPRINTF(("PAUSE       %d\n", data));
				ASSERT( stackEmpty(),
					"interpRunScript: OP_PAUSE without empty stack" );

				InstrPointer += aOpSize[opcode];
				// tell the event system to reschedule this event
				if (!eventAddPauseTrigger(psContext, index, InstrPointer - pCodeBase, data))	//only original caller can be paused since we pass index and not CurEvent (not sure if that's what we want)
				{
					debug( LOG_ERROR, "interpRunScript: could not add pause trigger" );
					ASSERT( FALSE, "interpRunScript: could not add pause trigger" );
					goto exit_with_error;
				}
				// now jump out of the event
				InstrPointer = pCodeEnd;
				break;
			case OP_TO_FLOAT:
				ASSERT( InstrPointer->type == VAL_OPCODE,
					"wrong value type passed for OP_TO_FLOAT: %d", InstrPointer->type);

				if(!castTop(VAL_FLOAT))
				{
					debug( LOG_ERROR, "interpRunScript: OP_TO_FLOAT failed" );
					goto exit_with_error;
				}
				InstrPointer += aOpSize[opcode];
				break;
			case OP_TO_INT:
				ASSERT( InstrPointer->type == VAL_OPCODE,
					"wrong value type passed for OP_TO_INT: %d", InstrPointer->type);

				if(!castTop(VAL_INT))
				{
					debug( LOG_ERROR, "interpRunScript: OP_TO_INT failed" );
					goto exit_with_error;
				}
				InstrPointer += aOpSize[opcode];
				break; 
			default:
				debug(LOG_ERROR, "interpRunScript: unknown opcode: %d, type: %d", opcode);
				ASSERT( FALSE, "interpRunScript: unknown opcode: %d, type: %d", opcode, InstrPointer->type );
				goto exit_with_error;
				break;
			}
		}

		else	//End of the event reached, see if we have to jump back to the caller function or just exit
		{
			//debug(LOG_SCRIPT, "End of event reached");

			if(!retStackIsEmpty())		//There was a caller function before this one
			{
				//debug(LOG_SCRIPT, "GetCallDepth = %d", GetCallDepth());

				//reset local vars (since trigger can't be called, only local vars of an event can be reset here)
				if(!resetLocalVars(psProg, CurEvent))
				{
					debug( LOG_ERROR, "interpRunScript: could not reset local vars for event %d", CurEvent );
					goto exit_with_error;
				}

				//pop caller function index and return address
				if (!retStackPop(&CurEvent, &InstrPointer))
				{
					debug( LOG_ERROR, "interpRunScript() - retStackPop() failed.");
					return FALSE;
				}

				//remember last called event/index
				strcpy(last_called_script_event, eventGetEventID(psProg, CurEvent));

				if(bTraceOn)
					debug(LOG_SCRIPT,"Returned to: '%s'", last_called_script_event);

				//debug( LOG_SCRIPT, "RETURNED TO CALLER EVENT %d", CurEvent );

				//Set new boundaries
				//--------------------------
				if(retStackIsEmpty())	//if we jumped back to the original caller
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
			else		//we have returned to the original caller event/function
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


	TRCPRINTF(("%-6d  EXIT\n", InstrPointer - psProg->pCode));

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
	callDepth = retStackCallDepth();
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
			"scriptSetTypeEquiv: can only set type equivalence for user types (%d)", i );
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
		to = (INTERP_TYPE)(to & ~VAL_REF);
	}
	if (from & VAL_REF)
	{
		fromRef = TRUE;
		from = (INTERP_TYPE)(from & ~VAL_REF);
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
#define RETSTACK_SIZE 100

static ReturnAddressStack_t retStack[RETSTACK_SIZE]; // Primitive stack of return addresses
static Sint8 retStackPos = -1; // Current Position, always points to the last valid element


Sint8 retStackCallDepth(void)
{
	return (retStackPos + 1);
}


static inline void retStackReset(void)
{
	retStackPos = -1; // Beginning of the stack
}


static inline BOOL retStackIsEmpty(void)
{
	if(retStackPos < 0) return TRUE;
	return FALSE;
}


static inline BOOL retStackIsFull(void)
{
	if(retStackPos >= RETSTACK_SIZE) return TRUE;
	return FALSE;
}


static BOOL retStackPush(UDWORD CallerIndex, INTERP_VAL *ReturnAddress)
{
	if (retStackIsFull())
	{
		debug( LOG_ERROR, "retStackPush(): return address stack is full");
		return FALSE; // Stack full
	}

	retStackPos++;
	retStack[retStackPos].CallerIndex = CallerIndex;
	retStack[retStackPos].ReturnAddress = ReturnAddress;

	//debug( LOG_SCRIPT, "retStackPush: Event=%i Address=%p, ", CallerIndex, ReturnAddress);

	return TRUE;
}


static BOOL retStackPop(UDWORD *CallerIndex, INTERP_VAL **ReturnAddress)
{
	if (retStackIsEmpty())
	{
		debug( LOG_ERROR, "retStackPop(): return address stack is empty");
		return FALSE;
	}

	*CallerIndex = retStack[retStackPos].CallerIndex;
	*ReturnAddress = retStack[retStackPos].ReturnAddress;
	retStackPos--;

	//debug( LOG_SCRIPT, "retStackPop: Event=%i Address=%p", *EventTrigIndex, *ReturnAddress);

	return TRUE;
}
