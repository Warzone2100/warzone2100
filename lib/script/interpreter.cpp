/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
 * Interp.c
 *
 * Execute the compiled version of a script
 *
 */

/* Control the execution trace printf's */
#include "lib/framework/frame.h"
#include "interpreter.h"
#include "stack.h"
#include "codeprint.h"
#include "script.h"
#include "event.h" //needed for eventGetEventID()


// the maximum number of instructions to execute before assuming
// an infinite loop
#define INTERP_MAXINSTRUCTIONS		300000
#define MAX_FUNC_CALLS 300

static INTERP_VAL	*varEnvironment[MAX_FUNC_CALLS];		//environments for local variables of events/functions

struct ReturnAddressStack_t
{
	UDWORD CallerIndex;
	INTERP_VAL *ReturnAddress;
};

/**
 * Reset the return address stack
 */
static inline void retStackReset(void);

/**
 * Check whether the return address stack is empty
 *
 * \return True when empty, false otherwise
 */
static inline bool retStackIsEmpty(void);

/**
 * Check whether the return address stack is full
 *
 * \return True when full, false otherwise
 */
static inline bool retStackIsFull(void);

/**
 * Push a new address/event pair on the return address stack
 *
 * \param CallerIndex Index of the calling function
 * \param ReturnAddress Address to return to
 * \return False on failure (stack full)
 */
static bool retStackPush(UDWORD CallerIndex, INTERP_VAL *ReturnAddress);

/**
 * Pop an address/event pair from the return address stack
 *
 * \param CallerIndex Index of the calling function
 * \param ReturnAddress Address to return to
 * \return False on failure (stack empty)
 */
static bool retStackPop(UDWORD *CallerIndex, INTERP_VAL **ReturnAddress);

/* Creates a new local var environment for a new function call */
static inline void createVarEnvironment(SCRIPT_CONTEXT *psContext, UDWORD eventIndex);

/* Destroy all created variable environments */
static void cleanupVarEnvironments(void);

static inline void destroyVarEnvironment(SCRIPT_CONTEXT *psContext, UDWORD envIndex, UDWORD eventIndex);

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
static TYPE_EQUIV *asInterpTypeEquiv = NULL;

// whether the interpreter is running
static bool		bInterpRunning = false;

/* Whether to output trace information */
static bool	interpTrace = false;

static SCRIPT_CODE *psCurProg = NULL;
static bool bCurCallerIsEvent = false;

/* Print out trace info if tracing is turned on */
#define TRCPRINTF(...) do { if (interpTrace) { fprintf( stderr, __VA_ARGS__ ); } } while (false)

#define TRCPRINTVAL(x) \
	if (interpTrace) \
		cpPrintVal(x)

#define TRCPRINTOPCODE(x) \
	if (interpTrace) \
		debug( LOG_NEVER, "%s", scriptOpcodeToString((OPCODE)x) )

#define TRCPRINTSTACKTOP() \
	if (interpTrace) \
		stackPrintTop()

#define TRCPRINTFUNC(x) \
	if (interpTrace) \
		debug( LOG_NEVER, "%s", scriptFunctionToString(x) )

#define TRCPRINTVARFUNC(x, data) \
	if (interpTrace) \
		cpPrintVarFunc(x, data)


// true if the interpreter is currently running
bool interpProcessorActive(void)
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
static bool interpGetArrayVarData(INTERP_VAL **pip, VAL_CHUNK *psGlobals, SCRIPT_CODE *psProg, INTERP_VAL **ppsVal)
{
	SDWORD		i, dimensions, vals[VAR_MAX_DIMENSIONS];
	UBYTE		*elements;
	SDWORD		size, val;
	INTERP_VAL	*InstrPointer = *pip;
	UDWORD		base, index;

	// get the base index of the array
	base = InstrPointer->v.ival & ARRAY_BASE_MASK;

	// get the number of dimensions
	dimensions = (InstrPointer->v.ival & ARRAY_DIMENSION_MASK) >> ARRAY_DIMENSION_SHIFT;

	ASSERT_OR_RETURN(false, base < psProg->numArrays, "Arrray base index out of range (%d should be less than %d)", base, psProg->numArrays);
	ASSERT_OR_RETURN(false, dimensions == psProg->psArrayInfo[base].dimensions, "Array dimensions do not match (%d vs %d)", dimensions, psProg->psArrayInfo[base].dimensions);

	// get the number of elements for each dimension
	elements = psProg->psArrayInfo[base].elements;

	// calculate the index of the array element
	size = 1;
	index = 0;
	for (i = dimensions - 1; i >= 0; i -= 1)
	{
		if (!stackPopParams(1, VAL_INT, &val))
		{
			return false;
		}

		if ((val < 0) || (val >= elements[i]))
		{
			debug(LOG_ERROR, "interpGetArrayVarData: Array index for dimension %d out of range (passed index = %d, max index = %d)", i , val, elements[i]);
			return false;
		}

		index += val * size;
		size *= psProg->psArrayInfo[base].elements[i];
		vals[i] = val;
	}

	// print out the debug trace
	if (interpTrace)
	{
		debug(LOG_NEVER, "%d->", base);
		for (i = 0; i < dimensions; i += 1)
		{
			debug(LOG_NEVER, "[%d/%d]", vals[i], elements[i]);
		}
		debug(LOG_NEVER, "(%d) ", index);
	}

	// check the index is valid
	ASSERT_OR_RETURN(false, index <= psProg->arraySize, "Array indexes out of variable space (%d)", index);

	// get the variable data
	*ppsVal = interpGetVarData(psGlobals, psProg->psArrayInfo[base].base + index);

	// update the instruction pointer
	*pip += 1;

	return true;
}


// Initialise the interpreter
bool interpInitialise(void)
{
	asInterpTypeEquiv = NULL;

	return true;
}

/* Run a compiled script */
bool interpRunScript(SCRIPT_CONTEXT *psContext, INTERP_RUNTYPE runType, UDWORD index, UDWORD offset)
{
	UDWORD			data;
	OPCODE			opcode;
	INTERP_VAL		sVal, *psVar, *InstrPointer;
	VAL_CHUNK		*psGlobals;
	UDWORD			numGlobals = 0;
	INTERP_VAL		*pCodeStart, *pCodeEnd, *pCodeBase;
	SCRIPT_FUNC		scriptFunc = 0;
	SCRIPT_VARFUNC	scriptVarFunc = 0;
	SCRIPT_CODE		*psProg;
	SDWORD			instructionCount = 0;

	UDWORD			CurEvent = 0;
	bool			bStop = false, bEvent = false;
	UDWORD			callDepth = 0;
	bool			bTraceOn = false;		//enable to debug function/event calls

	ASSERT(psContext != NULL, "Invalid context pointer");

	psProg = psContext->psCode;
	psCurProg = psProg;		//remember for future use

	ASSERT(psProg != NULL, "Invalid script code pointer");

	if (bInterpRunning)
	{
		debug(LOG_ERROR, "Interpreter already running"
		      "                 - callback being called from within a script function?");
		goto exit_with_error;
	}

	// note that the interpreter is running to stop recursive script calls
	bInterpRunning = true;

	// Reset the stack in case another script messed up
	stackReset();

	//reset return stack
	retStackReset();

	// Turn off tracing initially
	interpTrace = false;

	/* Get the global variables */
	numGlobals = psProg->numGlobals;
	psGlobals = psContext->psGlobals;

	bEvent = false;

	// Find the code range
	switch (runType)
	{
	case IRT_TRIGGER:
		if (index > psProg->numTriggers)
		{
			ASSERT(false, "Trigger index out of range");
			return false;
		}
		pCodeBase = psProg->pCode + psProg->pTriggerTab[index];
		pCodeStart = pCodeBase;
		pCodeEnd  = psProg->pCode + psProg->pTriggerTab[index + 1];

		bCurCallerIsEvent = false;

		// find the debug info for the trigger
		strcpy(last_called_script_event, eventGetTriggerID(psProg, index));

		if (bTraceOn)
		{
			debug(LOG_SCRIPT, "Trigger: %s", last_called_script_event);
		}

		break;
	case IRT_EVENT:
		if (index > psProg->numEvents)
		{
			ASSERT(false, "Trigger index out of range");
			return false;
		}
		pCodeBase = psProg->pCode + psProg->pEventTab[index];
		pCodeStart = pCodeBase + offset;		//offset only used for pause() script function
		pCodeEnd  = psProg->pCode + psProg->pEventTab[index + 1];

		bEvent = true; //remember it's an event
		bCurCallerIsEvent = true;

		// remember last called event/function
		strcpy(last_called_script_event, eventGetEventID(psProg, index));

		if (bTraceOn)
		{
			debug(LOG_SCRIPT, "Original event name: %s", last_called_script_event);
		}

		break;
	default:
		ASSERT(false, "Unknown run type");
		return false;
	}

	// Get the first opcode
	InstrPointer = pCodeStart;

	/* Make sure we start with an opcode */
	ASSERT(InstrPointer->type == VAL_PKOPCODE || InstrPointer->type == VAL_OPCODE,
	       "Expected an opcode at the beginning of the interpreting process (type=%d)", InstrPointer->type);

	//opcode = InstrPointer->v.ival >> OPCODE_SHIFT;

	instructionCount = 0;

	CurEvent = index;
	bStop = false;

	// create new variable environment for this call
	if (bEvent)
	{
		createVarEnvironment(psContext, CurEvent);
	}

	while (!bStop)
	{
		// Run the code
		if (InstrPointer < pCodeEnd)// && opcode != OP_EXIT)
		{
			if (instructionCount > INTERP_MAXINSTRUCTIONS)
			{
				debug(LOG_ERROR, "interpRunScript: max instruction count exceeded - infinite loop ?");
				goto exit_with_error;
			}
			instructionCount++;

			TRCPRINTF("%-6d  ", (int)(InstrPointer - psProg->pCode));
			opcode = (OPCODE)(InstrPointer->v.ival >> OPCODE_SHIFT);			//get opcode
			data = (SDWORD)(InstrPointer->v.ival & OPCODE_DATAMASK);		//get data - only used with packed opcodes
			switch (opcode)
			{
			/* Custom function call */
			case OP_FUNC:
				//debug( LOG_SCRIPT, "-OP_FUNC" );
				//debug( LOG_SCRIPT, "OP_FUNC: remember event %d, ip=%d", CurEvent, (ip + 2) );

				if (!retStackPush(CurEvent, (InstrPointer + aOpSize[opcode]))) //Remember where to jump back later
				{
					debug(LOG_ERROR, "interpRunScript() - retStackPush() failed.");
					return false;
				}

				ASSERT(((INTERP_VAL *)(InstrPointer + 1))->type == VAL_EVENT, "wrong value type passed for OP_FUNC: %d", ((INTERP_VAL *)(InstrPointer + 1))->type);

				// get index of the new event
				CurEvent = ((INTERP_VAL *)(InstrPointer + 1))->v.ival; //Current event = event to jump to

				if (CurEvent > psProg->numEvents)
				{
					debug(LOG_ERROR, "interpRunScript: trigger index out of range");
					goto exit_with_error;
				}

				// create new variable environment for this call
				createVarEnvironment(psContext, CurEvent);

				//Set new code execution boundaries
				//----------------------------------
				pCodeBase = psProg->pCode + psProg->pEventTab[CurEvent];
				pCodeStart = pCodeBase;
				pCodeEnd  = psProg->pCode + psProg->pEventTab[CurEvent + 1];

				InstrPointer = pCodeStart;				//Start at the beginning of the new event

				//remember last called event/index
				strcpy(last_called_script_event, eventGetEventID(psProg, CurEvent));

				if (bTraceOn)
				{
					debug(LOG_SCRIPT, "Called: '%s'", last_called_script_event);
				}

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
					goto exit_with_error;
				}

				//debug(LOG_SCRIPT, "OP_PUSHLOCAL type: %d", psContext->psCode->ppsLocalVarVal[CurEvent][data].type);

				if (!stackPush(&(varEnvironment[retStackCallDepth()][data])))
				{
					debug(LOG_ERROR, "interpRunScript: OP_PUSHLOCAL: push failed");
					goto exit_with_error;
				}

				InstrPointer += aOpSize[opcode];
				break;
			case OP_POPLOCAL:

				//debug( LOG_SCRIPT, "OP_POPLOCAL, event index: '%d', data: '%d'", CurEvent, data);
				//debug( LOG_SCRIPT, "OP_POPLOCAL, numLocalVars: '%d'", psContext->psCode->numLocalVars[CurEvent]);

				if (data >= psContext->psCode->numLocalVars[CurEvent])
				{
					debug(LOG_ERROR, "interpRunScript: OP_POPLOCAL: variable index out of range");
					goto exit_with_error;
				}

				//DbgMsg("OP_POPLOCAL type: %d, CurEvent=%d, data=%d", psContext->psCode->ppsLocalVarVal[CurEvent][data].type, CurEvent, data);

				if (!stackPopType(&(varEnvironment[retStackCallDepth()][data])))
				{
					debug(LOG_ERROR, "interpRunScript: OP_POPLOCAL: pop failed");
					goto exit_with_error;
				}

				//debug(LOG_SCRIPT, "OP_POPLOCAL: type=%d, val=%d", psContext->psCode->ppsLocalVarVal[CurEvent][data].type, psContext->psCode->ppsLocalVarVal[CurEvent][data].v.ival);

				InstrPointer += aOpSize[opcode];

				break;

			case OP_PUSHLOCALREF:

				// The type of the variable is stored in with the opcode
				sVal.type = (INTERP_TYPE)(InstrPointer->v.ival & OPCODE_DATAMASK);

				ASSERT(((INTERP_VAL *)(InstrPointer + 1))->type == VAL_INT,
				       "wrong value type passed for OP_PUSHLOCALREF: %d", ((INTERP_VAL *)(InstrPointer + 1))->type);

				/* get local var index */
				data = ((INTERP_VAL *)(InstrPointer + 1))->v.ival;

				if (data >= psContext->psCode->numLocalVars[CurEvent])
				{
					debug(LOG_ERROR, "interpRunScript: OP_PUSHLOCALREF: variable index out of range");
					goto exit_with_error;
				}

				/* get local variable */
				sVal.v.oval = &(varEnvironment[retStackCallDepth()][data]);

				TRCPRINTOPCODE(opcode);
				TRCPRINTVAL(sVal);
				TRCPRINTF("\n");

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

				TRCPRINTOPCODE(opcode);
				TRCPRINTVAL(sVal);
				TRCPRINTF("\n");
				if (!stackPush(&sVal))
				{
					// Eeerk, out of memory
					debug(LOG_ERROR, "interpRunScript: out of memory!");
					goto exit_with_error;
				}
				InstrPointer += aOpSize[opcode];
				break;
			case OP_PUSHREF:
				// The type of the variable is stored in with the opcode
				sVal.type = (INTERP_TYPE)(InstrPointer->v.ival & OPCODE_DATAMASK);

				// store pointer to INTERP_VAL
				sVal.v.oval = interpGetVarData(psGlobals, ((INTERP_VAL *)(InstrPointer + 1))->v.ival);

				TRCPRINTOPCODE(opcode);
				TRCPRINTVAL(sVal);
				TRCPRINTF("\n");
				if (!stackPush(&sVal))
				{
					// Eeerk, out of memory
					debug(LOG_ERROR, "interpRunScript: out of memory!");
					goto exit_with_error;
				}
				InstrPointer += aOpSize[opcode];
				break;
			case OP_POP:
				ASSERT(InstrPointer->type == VAL_OPCODE,
				       "wrong value type passed for OP_POP: %d", InstrPointer->type);

				TRCPRINTOPCODE(opcode);
				if (!stackPop(&sVal))
				{
					debug(LOG_ERROR, "interpRunScript: could not do stack pop");
					goto exit_with_error;
				}
				InstrPointer += aOpSize[opcode];
				break;
			case OP_BINARYOP:
				ASSERT(InstrPointer->type == VAL_PKOPCODE,
				       "wrong value type passed for OP_BINARYOP: %d", InstrPointer->type);

				TRCPRINTOPCODE(data);
				if (!stackBinaryOp((OPCODE)data))
				{
					debug(LOG_ERROR, "interpRunScript: could not do binary op");
					goto exit_with_error;
				}
				TRCPRINTSTACKTOP();
				TRCPRINTF("\n");
				InstrPointer += aOpSize[opcode];
				break;
			case OP_UNARYOP:
				ASSERT(InstrPointer->type == VAL_PKOPCODE,
				       "wrong value type passed for OP_UNARYOP: %d", InstrPointer->type);

				TRCPRINTOPCODE(data);
				if (!stackUnaryOp((OPCODE)data))
				{
					debug(LOG_ERROR, "interpRunScript: could not do unary op");
					goto exit_with_error;
				}
				TRCPRINTSTACKTOP();
				TRCPRINTF("\n");
				InstrPointer += aOpSize[opcode];
				break;
			case OP_PUSHGLOBAL:

				ASSERT(InstrPointer->type == VAL_PKOPCODE,
				       "wrong value type passed for OP_PUSHGLOBAL: %d", InstrPointer->type);

				TRCPRINTF("PUSHGLOBAL  %d\n", data);
				if (data >= numGlobals)
				{
					debug(LOG_ERROR, "interpRunScript: variable index out of range");
					goto exit_with_error;
				}
				if (!stackPush(interpGetVarData(psGlobals, data)))
				{
					debug(LOG_ERROR, "interpRunScript: could not do stack push");
					goto exit_with_error;
				}
				InstrPointer += aOpSize[opcode];
				break;
			case OP_POPGLOBAL:
				ASSERT(InstrPointer->type == VAL_PKOPCODE,
				       "wrong value type passed for OP_POPGLOBAL: %d", InstrPointer->type);

				TRCPRINTF("POPGLOBAL   %d ", data);
				TRCPRINTSTACKTOP();
				TRCPRINTF("\n");
				if (data >= numGlobals)
				{
					debug(LOG_ERROR, "interpRunScript: variable index out of range");
					goto exit_with_error;
				}
				if (!stackPopType(interpGetVarData(psGlobals, data)))
				{
					debug(LOG_ERROR, "interpRunScript: could not do stack pop");
					goto exit_with_error;
				}
				InstrPointer += aOpSize[opcode];
				break;
			case OP_PUSHARRAYGLOBAL:
				ASSERT(InstrPointer->type == VAL_PKOPCODE,
				       "wrong value type passed for OP_PUSHARRAYGLOBAL: %d", InstrPointer->type);

				TRCPRINTOPCODE(opcode);
				if (!interpGetArrayVarData(&InstrPointer, psGlobals, psProg, &psVar))
				{
					debug(LOG_ERROR, "interpRunScript: could not get array var data, CurEvent=%d", CurEvent);
					goto exit_with_error;
				}
				TRCPRINTF("\n");
				if (!stackPush(psVar))
				{
					debug(LOG_ERROR, "interpRunScript: could not do stack push");
					goto exit_with_error;
				}
				break;
			case OP_POPARRAYGLOBAL:
				ASSERT(InstrPointer->type == VAL_PKOPCODE,
				       "wrong value type passed for OP_POPARRAYGLOBAL: %d", InstrPointer->type);

				TRCPRINTOPCODE(opcode);
				if (!interpGetArrayVarData(&InstrPointer, psGlobals, psProg, &psVar))
				{
					debug(LOG_ERROR, "interpRunScript: could not get array var data");
					goto exit_with_error;
				}
				TRCPRINTSTACKTOP();
				TRCPRINTF("\n");
				if (!stackPopType(psVar))
				{
					debug(LOG_ERROR, "interpRunScript: could not do pop stack of type");
					goto exit_with_error;
				}
				break;

			case OP_JUMPFALSE:
				ASSERT(InstrPointer->type == VAL_PKOPCODE,
				       "wrong value type passed for OP_JUMPFALSE: %d", InstrPointer->type);

				TRCPRINTF("JUMPFALSE   %d (%d)",
				          (SWORD)data, (int)(InstrPointer - psProg->pCode + (SWORD)data));

				if (!stackPop(&sVal))
				{
					debug(LOG_ERROR, "interpRunScript: could not do pop of stack");
					goto exit_with_error;
				}
				if (!sVal.v.bval)
				{
					// Do the jump
					TRCPRINTF(" - done -\n");
					InstrPointer += (SWORD)data;
					if (InstrPointer < pCodeStart || InstrPointer > pCodeEnd)
					{
						debug(LOG_ERROR, "interpRunScript: jump out of range");
						goto exit_with_error;
					}
				}
				else
				{
					TRCPRINTF("\n");
					InstrPointer += aOpSize[opcode];
				}
				break;
			case OP_JUMP:
				ASSERT(InstrPointer->type == VAL_PKOPCODE,
				       "wrong value type passed for OP_JUMP: %d", InstrPointer->type);

				TRCPRINTF("JUMP        %d (%d)\n",
				          (SWORD)data, (int)(InstrPointer - psProg->pCode + (SWORD)data));
				// Do the jump
				InstrPointer += (SWORD)data;
				if (InstrPointer < pCodeStart || InstrPointer > pCodeEnd)
				{
					debug(LOG_ERROR, "interpRunScript: jump out of range");
					goto exit_with_error;
				}
				break;
			case OP_CALL:
				//debug(LOG_SCRIPT, "OP_CALL");

				ASSERT(InstrPointer->type == VAL_OPCODE,
				       "wrong value type passed for OP_CALL: %d", InstrPointer->type);

				scriptFunc = ((INTERP_VAL *)(InstrPointer + 1))->v.pFuncExtern;
				TRCPRINTFUNC(scriptFunc);
				TRCPRINTF("\n");
				//debug(LOG_SCRIPT, "OP_CALL 1");
				if (!scriptFunc())
				{
					debug(LOG_ERROR, "interpRunScript: could not do func");
					goto exit_with_error;
				}
				//debug(LOG_SCRIPT, "OP_CALL 2");
				InstrPointer += aOpSize[opcode];
				//debug(LOG_SCRIPT, "OP_CALL 3");
				break;
			case OP_VARCALL:
				ASSERT(InstrPointer->type == VAL_PKOPCODE,
				       "wrong value type passed for OP_VARCALL: %d", InstrPointer->type);

				TRCPRINTOPCODE(opcode);
				TRCPRINTVARFUNC(((INTERP_VAL *)(InstrPointer + 1))->v.pObjGetSet, data);
				TRCPRINTF("(%d)\n", data);

				ASSERT(((INTERP_VAL *)(InstrPointer + 1))->type == VAL_OBJ_GETSET,
				       "wrong set/get function pointer type passed for OP_VARCALL: %d", ((INTERP_VAL *)(InstrPointer + 1))->type);

				scriptVarFunc = ((INTERP_VAL *)(InstrPointer + 1))->v.pObjGetSet;
				if (!scriptVarFunc(data))
				{
					debug(LOG_ERROR, "interpRunScript: could not do var func");
					goto exit_with_error;
				}
				InstrPointer += aOpSize[opcode];
				break;
			case OP_EXIT:	/* end of function/event, "exit" or "return" statements */
				ASSERT(InstrPointer->type == VAL_OPCODE,
				       "wrong value type passed for OP_EXIT: %d", InstrPointer->type);

				// jump out of the code
				InstrPointer = pCodeEnd;
				break;
			case OP_PAUSE:
				ASSERT(InstrPointer->type == VAL_PKOPCODE,
				       "wrong value type passed for OP_PAUSE: %d", InstrPointer->type);

				TRCPRINTF("PAUSE       %d\n", data);
				ASSERT(stackEmpty(),
				       "interpRunScript: OP_PAUSE without empty stack");

				InstrPointer += aOpSize[opcode];
				// tell the event system to reschedule this event
				if (!eventAddPauseTrigger(psContext, index, (UDWORD)(InstrPointer - pCodeBase), data))	//only original caller can be paused since we pass index and not CurEvent (not sure if that's what we want)
				{
					debug(LOG_ERROR, "interpRunScript: could not add pause trigger");
					goto exit_with_error;
				}
				// now jump out of the event
				InstrPointer = pCodeEnd;
				break;
			case OP_TO_FLOAT:
				ASSERT(InstrPointer->type == VAL_OPCODE,
				       "wrong value type passed for OP_TO_FLOAT: %d", InstrPointer->type);

				if (!stackCastTop(VAL_FLOAT))
				{
					debug(LOG_ERROR, "interpRunScript: OP_TO_FLOAT failed");
					goto exit_with_error;
				}
				InstrPointer += aOpSize[opcode];
				break;
			case OP_TO_INT:
				ASSERT(InstrPointer->type == VAL_OPCODE,
				       "wrong value type passed for OP_TO_INT: %d", InstrPointer->type);

				if (!stackCastTop(VAL_INT))
				{
					debug(LOG_ERROR, "interpRunScript: OP_TO_INT failed");
					goto exit_with_error;
				}
				InstrPointer += aOpSize[opcode];
				break;
			default:
				debug(LOG_ERROR, "interpRunScript: unknown opcode: %d, type: %d", opcode, InstrPointer->type);
				goto exit_with_error;
				break;
			}
		}
		else	//End of the event reached, see if we have to jump back to the caller function or just exit
		{
			//debug(LOG_SCRIPT, "End of event reached");

			if (!retStackIsEmpty())		//There was a caller function before this one
			{
				// destroy current variable environment
				destroyVarEnvironment(psContext, retStackCallDepth(), CurEvent);

				//pop caller function index and return address
				if (!retStackPop(&CurEvent, &InstrPointer))
				{
					debug(LOG_ERROR, "interpRunScript() - retStackPop() failed.");
					return false;
				}

				//remember last called event/index
				strcpy(last_called_script_event, eventGetEventID(psProg, CurEvent));

				if (bTraceOn)
				{
					debug(LOG_SCRIPT, "Returned to: '%s'", last_called_script_event);
				}

				//debug( LOG_SCRIPT, "RETURNED TO CALLER EVENT %d", CurEvent );

				//Set new boundaries
				//--------------------------
				if (retStackIsEmpty())	//if we jumped back to the original caller
				{
					if (!bEvent)		//original caller was a trigger (is it possible at all?)
					{
						pCodeBase = psProg->pCode + psProg->pTriggerTab[CurEvent];
						pCodeStart = pCodeBase;
						pCodeEnd  = psProg->pCode + psProg->pTriggerTab[CurEvent + 1];
					}
					else			//original caller was an event
					{
						pCodeBase = psProg->pCode + psProg->pEventTab[CurEvent];
						pCodeStart = pCodeBase + offset;	//also use the offset passed, since it's an original caller event (offset is used for pause() )
						pCodeEnd  = psProg->pCode + psProg->pEventTab[CurEvent + 1];
					}
				}
				else	//we are still jumping thru functions (this can't be a callback, since it can't/should not be called)
				{
					pCodeBase = psProg->pCode + psProg->pEventTab[CurEvent];
					pCodeStart = pCodeBase;
					pCodeEnd  = psProg->pCode + psProg->pEventTab[CurEvent + 1];
				}
			}
			else		//we have returned to the original caller event/function
			{
				//debug( LOG_SCRIPT, " *** CALL STACK EMPTY ***" );

				//reset local vars only if original caller was an event, not a trigger
				if (bEvent)
				{
					// destroy current variable environment
					destroyVarEnvironment(psContext, retStackCallDepth(), CurEvent);
				}

				bStop = true;		//Stop execution of this event here, no more calling functions stored
			}
		}

	}

	psCurProg = NULL;
	TRCPRINTF("%-6d  EXIT\n", (int)(InstrPointer - psProg->pCode));

	bInterpRunning = false;
	return true;

exit_with_error:
	// Deal with the script crashing or running out of memory
	debug(LOG_ERROR, "interpRunScript: *** ERROR EXIT *** (CurEvent=%d)", CurEvent);

	/* Free all memory allocated for variable environments */
	cleanupVarEnvironments();

	if (bEvent)
	{
		debug(LOG_ERROR, "Original event ID: %d (of %d)", index, psProg->numEvents);
	}
	else
	{
		debug(LOG_ERROR, "Original trigger ID: %d (of %d)", index, psProg->numTriggers);
	}

	debug(LOG_ERROR, "Current event ID: %d (of %d)", CurEvent, psProg->numEvents);
	callDepth = retStackCallDepth();
	debug(LOG_ERROR, "Call depth : %d", callDepth);

	/* Output script call trace */
	scrOutputCallTrace(LOG_ERROR);
	psCurProg = NULL;

	TRCPRINTF("*** ERROR EXIT ***\n");

	ASSERT(!"error while executing a script", "interpRunScript: error while executing a script");

	bInterpRunning = false;
	return false;
}


/* Set the type equivalence table */
void scriptSetTypeEquiv(TYPE_EQUIV *psTypeTab)
{
#ifdef DEBUG
	SDWORD		i;

	for (i = 0; psTypeTab[i].base != 0; i++)
	{
		ASSERT(psTypeTab[i].base >= VAL_USERTYPESTART,
		       "scriptSetTypeEquiv: can only set type equivalence for user types (%d)", i);
	}
#endif

	asInterpTypeEquiv = psTypeTab;
}


/* Check if two types are equivalent
 * Means: Their data can be copied without conversion.
 * I.e. strings are NOT equivalent to anything but strings, even though they can be converted
 */
bool interpCheckEquiv(INTERP_TYPE to, INTERP_TYPE from)
{
	bool toRef = false, fromRef = false;

	// check for the VAL_REF flag
	if (to & VAL_REF)
	{
		toRef = true;
		to = (INTERP_TYPE)(to & ~VAL_REF);
	}
	if (from & VAL_REF)
	{
		fromRef = true;
		from = (INTERP_TYPE)(from & ~VAL_REF);
	}
	if (toRef != fromRef)
	{
		return false;
	}

	/* Void pointer is compatible with any other type */
	if (toRef == true && fromRef == true &&
	    (to == VAL_VOID || from == VAL_VOID))
	{
		return true;
	}

	if (to == from)
	{
		return true;
	}
	else if (asInterpTypeEquiv)
	{
		unsigned int i;
		for (i = 0; asInterpTypeEquiv[i].base != 0; i++)
		{
			if (asInterpTypeEquiv[i].base == to)
			{
				unsigned int j;
				for (j = 0; j < asInterpTypeEquiv[i].numEquiv; j++)
				{
					if (asInterpTypeEquiv[i].aEquivTypes[j] == from)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}


/* Instinct function to turn on tracing */
bool interpTraceOn(void)
{
	interpTrace = true;

	return true;
}

/* Instinct function to turn off tracing */
bool interpTraceOff(void)
{
	interpTrace = false;

	return true;
}


/* Call stack stuff */
static ReturnAddressStack_t retStack[MAX_FUNC_CALLS]; // Primitive stack of return addresses
static SDWORD retStackPos = -1; // Current Position, always points to the last valid element


UDWORD retStackCallDepth(void)
{
	ASSERT(retStackPos + 1 >= 0 && retStackPos + 1 < MAX_FUNC_CALLS,
	       "retStackCallDepth: wrong call depth: %d", retStackPos + 1);

	return (retStackPos + 1);
}


static inline void retStackReset(void)
{
	retStackPos = -1; // Beginning of the stack
}


static inline bool retStackIsEmpty(void)
{
	if (retStackPos < 0)
	{
		return true;
	}
	return false;
}


static inline bool retStackIsFull(void)
{
	if (retStackPos >= MAX_FUNC_CALLS)
	{
		return true;
	}
	return false;
}


static bool retStackPush(UDWORD CallerIndex, INTERP_VAL *ReturnAddress)
{
	if (retStackIsFull())
	{
		debug(LOG_ERROR, "retStackPush(): return address stack is full");
		return false; // Stack full
	}

	retStackPos++;
	retStack[retStackPos].CallerIndex = CallerIndex;
	retStack[retStackPos].ReturnAddress = ReturnAddress;

	//debug( LOG_SCRIPT, "retStackPush: Event=%i Address=%p, ", CallerIndex, ReturnAddress);

	return true;
}


static bool retStackPop(UDWORD *CallerIndex, INTERP_VAL **ReturnAddress)
{
	if (retStackIsEmpty())
	{
		debug(LOG_ERROR, "retStackPop(): return address stack is empty");
		return false;
	}

	*CallerIndex = retStack[retStackPos].CallerIndex;
	*ReturnAddress = retStack[retStackPos].ReturnAddress;
	retStackPos--;

	//debug( LOG_SCRIPT, "retStackPop: Event=%i Address=%p", *EventTrigIndex, *ReturnAddress);

	return true;
}


/* Output script call stack trace */
void scrOutputCallTrace(code_part part)
{
	SDWORD i;
	const char *pEvent;

	debug(part, " *** Script call trace: ***");

	if (!bInterpRunning)
	{
		debug(part, "<Interpreter is inactive>");
		return;
	}

	if (psCurProg == NULL)
	{
		return;
	}

	debug(part, "%d: %s (current event)", retStackPos + 1, &(last_called_script_event[0]));

	if (psCurProg->psDebug != NULL)
	{
		for (i = retStackPos; i >= 0; i--)
		{
			if (i == 0 && !bCurCallerIsEvent) 	//if original caller is a trigger
			{
				pEvent = eventGetTriggerID(psCurProg, retStack[i].CallerIndex);
			}
			else
			{
				pEvent = eventGetEventID(psCurProg, retStack[i].CallerIndex);
			}

			debug(part, "%d: %s (return address: %p)", i, pEvent, retStack[i].ReturnAddress);
		}
	}
	else
	{
		debug(part, "<No debug information available>");
	}
}

/* create a new local var environment for a new function call */
static inline void createVarEnvironment(SCRIPT_CONTEXT *psContext, UDWORD eventIndex)
{
	UDWORD i, callDepth = retStackCallDepth();
	UDWORD numEventVars = psContext->psCode->numLocalVars[eventIndex];

	if (numEventVars > 0)
	{
		// alloc memory
		varEnvironment[callDepth] = (INTERP_VAL *)malloc(sizeof(INTERP_VAL) * numEventVars);

		// create environment
		memcpy(varEnvironment[callDepth], psContext->psCode->ppsLocalVarVal[eventIndex], sizeof(INTERP_VAL) * numEventVars);

		// allocate new space for strings to preserve original ones
		for (i = 0; i < numEventVars; i++)
		{
			if (varEnvironment[callDepth][i].type == VAL_STRING)
			{
				varEnvironment[callDepth][i].v.sval = (char *)malloc(MAXSTRLEN);
				strcpy(varEnvironment[callDepth][i].v.sval, "");	//initialize
			}
		}
	}
	else
	{
		varEnvironment[callDepth] = NULL;
	}
}

static inline void destroyVarEnvironment(SCRIPT_CONTEXT *psContext, UDWORD envIndex, UDWORD eventIndex)
{
	UDWORD i;
	UDWORD numEventVars = 0;

	if (psContext != NULL)
	{
		numEventVars = psContext->psCode->numLocalVars[eventIndex];
	}

	if (varEnvironment[envIndex] != NULL)
	{
		// deallocate string space
		for (i = 0; i < numEventVars; i++)
		{
			if (varEnvironment[envIndex][i].type == VAL_STRING)
			{
				free(varEnvironment[envIndex][i].v.sval);
				varEnvironment[envIndex][i].v.sval = NULL;
			}
		}

		free(varEnvironment[envIndex]);
		varEnvironment[envIndex] = NULL;
	}
}

/* Destroy all created variable environments */
static void cleanupVarEnvironments(void)
{
	UDWORD i;

	for (i = 0; i < retStackCallDepth(); i++)
	{
		destroyVarEnvironment(NULL, i, 0);
	}
}
