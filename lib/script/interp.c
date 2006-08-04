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
		DBPRINTF(x)

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
		ASSERT((FALSE,
			"interpGetArrayVarData: array base index out of range"));
		return FALSE;
	}
	if (dimensions != psProg->psArrayInfo[base].dimensions)
	{
		ASSERT((FALSE,
			"interpGetArrayVarData: dimensions do not match"));
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
			ASSERT((FALSE, "interpGetArrayVarData: Array index for dimension %d out of range", i));
			return FALSE;
		}

		index += val * size;
		size *= psProg->psArrayInfo[base].elements[i];
		vals[i] = val;
	}

	// print out the debug trace
	if (interpTrace)
	{
		DBPRINTF(("%d->", base));
		for(i=0; i<dimensions; i+= 1)
		{
			DBPRINTF(("[%d/%d]", vals[i], elements[i]));
		}
		DBPRINTF(("(%d) ", index));
	}

	// check the index is valid
	if (index > psProg->arraySize)
	{
		ASSERT((FALSE, "interpGetArrayVarData: Array indexes out of variable space"));
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

	ASSERT((PTRVALID(psContext, sizeof(SCRIPT_CONTEXT)),
		"interpRunScript: invalid context pointer"));
	psProg=psContext->psCode;
	ASSERT((PTRVALID(psProg, sizeof(SCRIPT_CODE)),
		"interpRunScript: invalid script code pointer"));

	if (bInterpRunning)
	{
		ASSERT((FALSE,
			"interpRunScript: interpreter already running"
			"                 - callback being called from within a script function?"));
		goto exit_with_error;
	}

	// note that the interpreter is running to stop recursive script calls
	bInterpRunning = TRUE;

	// Reset the stack in case another script messed up
	stackReset();

	// Turn off tracing initially
	interpTrace = FALSE;

	/* Get the global variables */
	numGlobals = psProg->numGlobals;
	psGlobals = psContext->psGlobals;

	// Find the code range
	switch (runType)
	{
	case IRT_TRIGGER:
		if (index > psProg->numTriggers)
		{
			ASSERT((FALSE, "interpRunScript: trigger index out of range"));
			return FALSE;
		}
		pCodeBase = psProg->pCode + psProg->pTriggerTab[index];
		pCodeStart = pCodeBase;
		pCodeEnd  = psProg->pCode + psProg->pTriggerTab[index+1];
		break;
	case IRT_EVENT:
		if (index > psProg->numEvents)
		{
			ASSERT((FALSE, "interpRunScript: trigger index out of range"));
			return FALSE;
		}
		pCodeBase = psProg->pCode + psProg->pEventTab[index];
		pCodeStart = pCodeBase + offset;
		pCodeEnd  = psProg->pCode + psProg->pEventTab[index+1];
		break;
	default:
		ASSERT((FALSE, "interpRunScript: unknown run type"));
		return FALSE;
	}

	// Get the first opcode
	ip = pCodeStart;
	opcode = (*ip) >> OPCODE_SHIFT;
	instructionCount = 0;

	// Run the code
	while (ip < pCodeEnd)// && opcode != OP_EXIT)
	{
		if (instructionCount > INTERP_MAXINSTRUCTIONS)
		{
			ASSERT((FALSE,
				"interpRunScript: max instruction count exceeded - infinite loop ?"));
			goto exit_with_error;
		}
		instructionCount += 1;

		TRCPRINTF(("%-6d  ", ip - psProg->pCode));
		opcode = (*ip) >> OPCODE_SHIFT;
		data = (*ip) & OPCODE_DATAMASK;
		switch (opcode)
		{
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
				ASSERT((FALSE, "interpRunScript: out of memory!"));
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
				ASSERT((FALSE, "interpRunScript: out of memory!"));
				goto exit_with_error;
			}
			ip += aOpSize[opcode];
			break;
		case OP_POP:
			TRCPRINTF(("POP\n"));
			if (!stackPop(&sVal))
			{
				ASSERT((FALSE, "interpRunScript: could not do stack pop"));
				goto exit_with_error;
			}
			ip += aOpSize[opcode];
			break;
		case OP_BINARYOP:
			TRCPRINTMATHSOP(data);
			if (!stackBinaryOp((OPCODE)data))
			{
				ASSERT((FALSE, "interpRunScript: could not do binary op"));
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
				ASSERT((FALSE, "interpRunScript: could not do unary op"));
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
				ASSERT((FALSE, "interpRunScript: variable index out of range"));
				goto exit_with_error;
			}
			if (!stackPush(interpGetVarData(psGlobals, data)))
			{
				ASSERT((FALSE, "interpRunScript: could not do stack push"));
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
				ASSERT((FALSE, "interpRunScript: variable index out of range"));
				goto exit_with_error;
			}
			if (!stackPopType(interpGetVarData(psGlobals, data)))
			{
				ASSERT((FALSE, "interpRunScript: could not do stack pop"));
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
//				ASSERT((FALSE, "interpRunScript: variable index out of range"));
//				goto exit_with_error;
//			}
//			if (arrayIndex < 0 || arrayIndex >= arrayElements)
//			{
//				ASSERT((FALSE, "interpRunScript: array index out of range"));
//				goto exit_with_error;
//			}
			TRCPRINTF(("PUSHARRAYGLOBAL  "));
			if (!interpGetArrayVarData(&ip, psGlobals, psProg, &psVar))
			{
				ASSERT((FALSE, "interpRunScript: could not get array var data"));
				goto exit_with_error;
			}
			TRCPRINTF(("\n"));
			if (!stackPush(psVar))
			{
				ASSERT((FALSE, "interpRunScript: could not do stack push"));
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
				ASSERT((FALSE, "interpRunScript: could not do pop of params"));
				goto exit_with_error;
			}
			TRCPRINTF(("POPARRAYGLOBAL   [%d] %d(+%d) ", arrayIndex, data, arrayElements));
			TRCPRINTSTACKTOP();
			TRCPRINTF(("\n"));
			if (data + arrayElements > numGlobals)
			{
				ASSERT((FALSE, "interpRunScript: variable index out of range"));
				goto exit_with_error;
			}
			if (arrayIndex < 0 || arrayIndex >= arrayElements)
			{
				ASSERT((FALSE, "interpRunScript: array index out of range"));
				goto exit_with_error;
			}
			if (!stackPopType(interpGetVarData(psGlobals, data + arrayIndex)))
			{
				ASSERT((FALSE, "interpRunScript: could not do pop stack of type"));
				goto exit_with_error;
			}
			ip += aOpSize[opcode];*/
			TRCPRINTF(("POPARRAYGLOBAL   "));
			if (!interpGetArrayVarData(&ip, psGlobals, psProg, &psVar))
			{
				ASSERT((FALSE, "interpRunScript: could not get array var data"));
				goto exit_with_error;
			}
			TRCPRINTSTACKTOP();
			TRCPRINTF(("\n"));
			if (!stackPopType(psVar))
			{
				ASSERT((FALSE, "interpRunScript: could not do pop stack of type"));
				goto exit_with_error;
			}
			break;
		case OP_JUMPFALSE:
			TRCPRINTF(("JUMPFALSE   %d (%d)",
					(SWORD)data, ip - psProg->pCode + (SWORD)data));
			if (!stackPop(&sVal))
			{
				ASSERT((FALSE, "interpRunScript: could not do pop of stack"));
				goto exit_with_error;
			}
			if (!sVal.v.bval)
			{
				// Do the jump
				TRCPRINTF((" - done -\n"));
				ip += (SWORD)data;
				if (ip < pCodeStart || ip > pCodeEnd)
				{
					ASSERT((FALSE, "interpRunScript: jump out of range"));
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
				ASSERT((FALSE, "interpRunScript: jump out of range"));
				goto exit_with_error;
			}
			break;
		case OP_CALL:
			TRCPRINTF(("CALL        "));
			TRCPRINTFUNC( (SCRIPT_FUNC)(*(ip+1)) );
			TRCPRINTF(("\n"));
			scriptFunc = (SCRIPT_FUNC)*(ip+1);
			if (!scriptFunc())
			{
				ASSERT((FALSE, "interpRunScript: could not do func"));
				goto exit_with_error;
			}
			ip += aOpSize[opcode];
			break;
		case OP_VARCALL:
			TRCPRINTF(("VARCALL     "));
			TRCPRINTVARFUNC( (SCRIPT_VARFUNC)(*(ip+1)), data );
			TRCPRINTF(("(%d)\n", data));
			scriptVarFunc = (SCRIPT_VARFUNC)*(ip+1);
			if (!scriptVarFunc(data))
			{
				ASSERT((FALSE, "interpRunScript: could not do var func"));
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
			ASSERT((stackEmpty(),
				"interpRunScript: OP_PAUSE without empty stack"));
			ip += aOpSize[opcode];
			// tell the event system to reschedule this event
			if (!eventAddPauseTrigger(psContext, index, ip - pCodeBase, data))
			{
				ASSERT((FALSE, "interpRunScript: could not add pause trigger"));
				goto exit_with_error;
			}
			// now jump out of the event
			ip = pCodeEnd;
			break;
		default:
			ASSERT((FALSE, "interpRunScript: unknown opcode"));
			goto exit_with_error;
			break;
		}
	}
	TRCPRINTF(("%-6d  EXIT\n", ip - psProg->pCode));

	bInterpRunning = FALSE;
	return TRUE;

exit_with_error:
	// Deal with the script crashing or running out of memory
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
		ASSERT((psTypeTab[i].base >= VAL_USERTYPESTART,
			"scriptSetTypeEquiv: can only set type equivalence for user types"));
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

