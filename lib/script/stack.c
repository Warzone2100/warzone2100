/*
 * Stack.c
 *
 * A stack for the script interpreter.
 */

#include <stdarg.h>
#include <string.h>


#include "lib/framework/frame.h"
#include "interp.h"
#include "stack.h"
#include "codeprint.h"
#include "script.h"

#include "lib/script/script_parser.h"

/* number of values in each stack chunk */
#define INIT_SIZE		15
#define EXT_SIZE		2

char STRSTACK[MAXSTACKLEN][MAXSTRLEN]; //simple string 'stack'
UDWORD CURSTACKSTR = 0;    //Points to the top of the string stack

/* store for a 'chunk' of the stack */
typedef struct _stack_chunk
{
	INTERP_VAL	*aVals;
	UDWORD		size;

	struct _stack_chunk		*psNext, *psPrev;
} STACK_CHUNK;

/* The first chunk of the stack */
static STACK_CHUNK		*psStackBase=NULL;

/* The current stack chunk */
static STACK_CHUNK		*psCurrChunk=NULL;

/* The current free entry on the current stack chunk */
static UDWORD			currEntry=0;

/* The block heap the stack was created in */
static BLOCK_HEAP		*psStackBlock;


/* Check if the stack is empty */
BOOL stackEmpty(void)
{
	return psCurrChunk == psStackBase && currEntry == 0;
}


/* Allocate a new chunk for the stack */
static BOOL stackNewChunk(UDWORD size)
{
	BLOCK_HEAP		*psHeap;
	unsigned int						i;

	/* see if a chunk has already been allocated */
	if (psCurrChunk->psNext != NULL)
	{
		psCurrChunk = psCurrChunk->psNext;
		currEntry = 0;
	}
	else
	{
		psHeap = memGetBlockHeap();
		memSetBlockHeap(psStackBlock);

		/* Allocate a new chunk */
		psCurrChunk->psNext = (STACK_CHUNK *)MALLOC(sizeof(STACK_CHUNK));
		if (!psCurrChunk->psNext)
		{
			return FALSE;
		}
		psCurrChunk->psNext->aVals = (INTERP_VAL*)MALLOC(sizeof(INTERP_VAL) * size);
		if (!psCurrChunk->psNext->aVals)
		{
			FREE(psCurrChunk->psNext);
			return FALSE;
		}

		psCurrChunk->psNext->size = size;
		psCurrChunk->psNext->psPrev = psCurrChunk;
		psCurrChunk->psNext->psNext = NULL;
		psCurrChunk = psCurrChunk->psNext;
		currEntry = 0;

		/* initialize pointers */
		for (i = 0; i<size; i++)
		{
			psCurrChunk->psNext->aVals[i].v.ival = 0;
			psCurrChunk->psNext->aVals[i].type = VAL_INT;		//default type, should be ok
		}

		memSetBlockHeap(psHeap);
	}

	return TRUE;
}

/* Push a value onto the stack */
BOOL stackPush(INTERP_VAL  *psVal)
{
	/* Store the value in the stack - psCurrChunk/currEntry always point to
	valid space */

	/* String support: Copy the string, otherwise the stack will operate directly on the
	original string (like & opcode will actually store the result in the first
	variable which would point to the original string) */
	if(psVal->type == VAL_STRING)		//pushing string
	{
		/* strings should already have memory allocated */
		if(psCurrChunk->aVals[currEntry].type != VAL_STRING)		//needs to have memory allocated for string
			psCurrChunk->aVals[currEntry].v.sval = (char*)MALLOC(MAXSTRLEN);

		strcpy(psCurrChunk->aVals[currEntry].v.sval, psVal->v.sval);		//copy string to stack
		psCurrChunk->aVals[currEntry].type = VAL_STRING;
	}
	else		/* pushing non-string */
	{
		/* free stack var allocated string memory, if stack var used to be of type VAL_STRING */
		if(psCurrChunk->aVals[currEntry].type == VAL_STRING)
			FREE(psCurrChunk->aVals[currEntry].v.sval);			//don't need it anymore

		/* copy type/data as union */
		memcpy(&(psCurrChunk->aVals[currEntry]), psVal, sizeof(INTERP_VAL));
	}

	/* Now update psCurrChunk and currEntry */
	currEntry++;
	if (currEntry == psCurrChunk->size)
	{
		/* At the end of this chunk, need a new one */
		if (!stackNewChunk(EXT_SIZE))
		{
			/* Out of memory */
			debug(LOG_ERROR, "stackPush: Out of memory");
			return FALSE;
		}
	}

	return TRUE;
}



/* Pop a value off the stack */
BOOL stackPop(INTERP_VAL  *psVal)
{
	if ((psCurrChunk->psPrev == NULL) && (currEntry == 0))
	{
		debug(LOG_ERROR, "stackPop: stack empty");
		ASSERT( FALSE, "stackPop: stack empty" );
		return FALSE;
	}

	/* move the stack pointer down one */
	if (currEntry == 0)
	{
		/* have to move onto the previous chunk. */
		psCurrChunk = psCurrChunk->psPrev;
		currEntry = psCurrChunk->size -1;
	}
	else
	{
		currEntry--;
	}

	/* copy the entire value off the stack */
	memcpy(psVal, &(psCurrChunk->aVals[currEntry]), sizeof(INTERP_VAL));

	return TRUE;
}


/* Pop a value off the stack, checking that the type matches what is passed in */
BOOL stackPopType(INTERP_VAL  *psVal)
{
	INTERP_VAL	*psTop;

	if ((psCurrChunk->psPrev == NULL) && (currEntry == 0))
	{
		debug(LOG_ERROR, "stackPopType: stack empty");
		ASSERT( FALSE, "stackPopType: stack empty" );
		return FALSE;
	}

	/* move the stack pointer down one */
	if (currEntry == 0)
	{
		/* have to move onto the previous chunk. */
		psCurrChunk = psCurrChunk->psPrev;
		currEntry = psCurrChunk->size -1;
	}
	else
	{
		currEntry--;
	}

	psTop = psCurrChunk->aVals + currEntry;

	//String support
	if(psVal->type == VAL_STRING)		//If we are about to assign something to a string variable (psVal)
	{
		if(psTop->type != VAL_STRING)		//if assigning a non-string value to a string variable, then convert the value
		{
			/* Check for compatible types */
			switch (psTop->type)
			{
				case VAL_INT:			/* int->string */
					sprintf(psVal->v.sval, "%d", psTop->v.ival);
					break;
				case VAL_BOOL:		/* bool->string */
					sprintf(psVal->v.sval, "%d", psTop->v.bval);
					break;
				case VAL_FLOAT:		/* float->string */
					sprintf(psVal->v.sval, "%f", psTop->v.fval);
					break;
				default:
					debug(LOG_ERROR, "stackPopType: trying to assign an incompatible data type to a string variable (type: %d)", psTop->type);
					return FALSE;
					break;
			}
		}
		else	//assigning string value to a string variable - COPY string
		{
			strcpy(psVal->v.sval, psTop->v.sval);
		}
	}
	else		// we are about to assign something to a non-string variable (psVal)
	{
		if (!interpCheckEquiv(psVal->type,psTop->type))
		{
			debug(LOG_ERROR, "stackPopType: type mismatch");
			ASSERT( FALSE, "stackPopType: type mismatch" );
			return FALSE;
		}

		/* copy the entire union off the stack */
		memcpy(&(psVal->v), &(psTop->v), sizeof(psTop->v));
	}

	return TRUE;
}


/* Pop a number of values off the stack checking their types
 * This is used by instinct functions to get their parameters
 */
BOOL stackPopParams(SDWORD numParams, ...)
{
	va_list		args;
	SDWORD		i;
	INTERP_TYPE	type;
	void		*pData;
	INTERP_VAL	*psVal;
	UDWORD		index, params;
	STACK_CHUNK	*psCurr;

	//debug(LOG_SCRIPT,"stackPopParams");

	va_start(args, numParams);

	// Find the position of the first parameter, and set
	// the stack top to it
	if ((UDWORD)numParams <= currEntry)
	{
		// parameters are all on current chunk
		currEntry = currEntry - numParams;
		psCurr = psCurrChunk;
	}
	else
	{
		// Have to work down the previous chunks to find the first param
		params = numParams - currEntry;

		for(psCurr = psCurrChunk->psPrev; psCurr != NULL; psCurr = psCurr->psPrev)
		{
			if (params <= psCurr->size)
			{
				// found the first param
				currEntry = psCurr->size - params;
				psCurrChunk = psCurr;
				break;
			}
			else
			{
				params -= psCurr->size;
			}
		}
	}

	if (!psCurr)
	{
		debug(LOG_ERROR,"stackPopParams: not enough parameters on stack");
		ASSERT( FALSE, "stackPopParams: not enough parameters on stack" );
		return FALSE;
	}


	//string support
	// Get the values, checking their types
	index = currEntry;
	for (i=0; i< numParams; i++)
	{
		type = (INTERP_TYPE)(va_arg(args, int));
		pData = va_arg(args, void *);

		psVal = psCurr->aVals + index;

		if(type == VAL_FLOAT)		//expecting a float
		{
			/* if (!interpCheckEquiv(type,psVal->type))
			{
				ASSERT( FALSE, "stackPopParams: type mismatch (%d/%d)" , type, psVal->type);
				va_end(args);
				return FALSE;
			} */
			*((float*)pData) = psVal->v.fval;
		}
		else if(type != VAL_STRING)	//anything (including references)
		{
			if (!interpCheckEquiv(type,psVal->type))
			{
				ASSERT( FALSE, "stackPopParams: type mismatch" );
				va_end(args);
				return FALSE;
			}
			*((SDWORD*)pData) = psVal->v.ival;
		}
		else	//TODO: allow only compatible types
		{
			if(psVal->type == VAL_STRING)	//Passing a String
			{
				//*((char **)pData) = psVal->v.sval;
				strcpy(((char *)pData), psVal->v.sval);		//COPY string
			}
			else		//Integer or float
			{
				switch (psVal->type)
				{
				case VAL_INT:
					sprintf(((char *)pData), "%d", psVal->v.ival);
					break;
				case VAL_FLOAT:
					sprintf(((char *)pData), "%f", psVal->v.fval);
					break;
				case VAL_BOOL:
					sprintf(((char *)pData), "%d", psVal->v.bval);
					break;
				default:
					ASSERT(FALSE, "StackPopParam - wrong data type being converted to string (type: %d)", psVal->type);
					break;
				}
				//*((char **)pData) = psVal->v.sval;
			}
		}

		index += 1;
		if (index >= psCurr->size)
		{
			psCurr = psCurr->psNext;
			index = 0;
		}
	}

	va_end(args);
	return TRUE;
}


/* Push a value onto the stack without using a value structure
	NOTE: result->type is _not_ set yet - use 'type' instead
*/
BOOL stackPushResult(INTERP_TYPE type, INTERP_VAL *result)
{
	/* assign type, wasn't done before */
	result->type = type;

	/* String support: Copy the string, otherwise the stack will operate directly on the
	original string (like & opcode will actually store the result in the first
	variable which would point to the original string) */
	if(result->type == VAL_STRING)		//pushing string
	{
		/* strings should already have memory allocated */
		if(psCurrChunk->aVals[currEntry].type != VAL_STRING)		//needs to have memory allocated for string
			psCurrChunk->aVals[currEntry].v.sval = (char*)MALLOC(MAXSTRLEN);

		strcpy(psCurrChunk->aVals[currEntry].v.sval, result->v.sval);		//copy string to stack
		psCurrChunk->aVals[currEntry].type = VAL_STRING;
	}
	else		/* pushing non-string */
	{
		/* free stack var allocated string memory, if stack var used to be of type VAL_STRING */
		if(psCurrChunk->aVals[currEntry].type == VAL_STRING)
			FREE(psCurrChunk->aVals[currEntry].v.sval);			//don't need it anymore

		/* copy type/data as union */
		memcpy(&(psCurrChunk->aVals[currEntry]), result, sizeof(INTERP_VAL));
	}

	// Now update psCurrChunk and currEntry
	currEntry++;
	if (currEntry == psCurrChunk->size)
	{
		// At the end of this chunk, need a new one
		if (!stackNewChunk(EXT_SIZE))
		{
			// Out of memory
			return FALSE;
		}
	}

	return TRUE;
}


/* Look at a value on the stack without removing it.
 * index is how far down the stack to look.
 * Index 0 is the top entry on the stack.
 */
BOOL stackPeek(INTERP_VAL *psVal, UDWORD index)
{
	STACK_CHUNK		*psCurr;

	if (index < currEntry)
	{
		/* Looking at entry on current chunk */
		memcpy(psVal, &(psCurrChunk->aVals[currEntry - index - 1]), sizeof(INTERP_VAL));
		return TRUE;
	}
	else
	{
		/* Have to work down the previous chunks to find the entry */
		index -= currEntry;

		for(psCurr = psCurrChunk->psPrev; psCurr != NULL; psCurr = psCurr->psPrev)
		{
			if (index < psCurr->size)
			{
				/* found the entry */
				memcpy(psVal, &(psCurr->aVals[psCurr->size - 1 - index]), sizeof(INTERP_VAL));
				return TRUE;
			}
			else
			{
				index -= psCurr->size;
			}
		}
	}

	/* If we got here the index is off the bottom of the stack */
	ASSERT( FALSE, "stackPeek: index too large" );
	return FALSE;
}


/* Print the top value on the stack */
void stackPrintTop(void)
{
	INTERP_VAL	sVal;
	if (stackPeek(&sVal, 0))
	{
		cpPrintVal(&sVal);
	}
	else
	{
		debug( LOG_NEVER, "STACK EMPTY" );
	}
}


/* Do binary operations on the top of the stack
 * This effectively pops two values and pushes the result
 */
BOOL stackBinaryOp(OPCODE opcode)
{
	STACK_CHUNK		*psChunk;
	INTERP_VAL		*psV1, *psV2;

	// Get the parameters
	if (psCurrChunk->psPrev == NULL && currEntry < 2)
	{
		debug(LOG_ERROR, "stackBinaryOp: not enough entries on stack");
		ASSERT( FALSE, "stackBinaryOp: not enough entries on stack" );
		return FALSE;
	}

	if (currEntry > 1)
	{
		psV1 = psCurrChunk->aVals + currEntry - 2;
		psV2 = psCurrChunk->aVals + currEntry - 1;
		currEntry -= 1;
	}
	else if (currEntry == 1)
	{
		// One value is on the previous chunk
		psChunk = psCurrChunk->psPrev;
		psV1 = psChunk->aVals + psChunk->size - 1;
		psV2 = psCurrChunk->aVals + currEntry - 1;
		currEntry -= 1;
	}
	else
	{
		// both on the previous chunk, pop to the previous chunk
		psCurrChunk = psCurrChunk->psPrev;
		currEntry = psCurrChunk->size - 1;
		psV1 = psCurrChunk->aVals + psCurrChunk->size - 2;
		psV2 = psCurrChunk->aVals + psCurrChunk->size - 1;
	}

	if(opcode != OP_CONC)		//string - Don't check if OP_CONC, since types can be mixed here
	{
		if (!interpCheckEquiv(psV1->type, psV2->type))
		{
			debug(LOG_ERROR, "stackBinaryOp: type mismatch");
			ASSERT( FALSE, "stackBinaryOp: type mismatch" );
			return FALSE;
		}
	}

	/* find out if the result will be a float. Both or neither arguments are floats - should be taken care of by bison*/
	if(psV1->type == VAL_FLOAT || psV2->type == VAL_FLOAT)
	{
		ASSERT(psV1->type == VAL_FLOAT && psV2->type == VAL_FLOAT, "Can't implicitly convert float->int (type1: %d, type2: %d)", psV1->type , psV2->type );
	}

	// do the operation
	switch (opcode)
	{
	case OP_ADD:
		if(psV1->type == VAL_FLOAT || psV2->type == VAL_FLOAT)
		{
			psV1->type = VAL_FLOAT;
			psV1->v.fval = psV1->v.fval + psV2->v.fval;
		}
		else
		{
			psV1->v.ival = psV1->v.ival + psV2->v.ival;
		}
		break;
	case OP_SUB:
		if(psV1->type == VAL_FLOAT || psV2->type == VAL_FLOAT)
		{
			psV1->type = VAL_FLOAT;
			psV1->v.fval = psV1->v.fval - psV2->v.fval;
		}
		else
		{
			psV1->v.ival = psV1->v.ival - psV2->v.ival;
		}
		break;
	case OP_MUL:
		if(psV1->type == VAL_FLOAT || psV2->type == VAL_FLOAT)
		{
			psV1->type = VAL_FLOAT;
			psV1->v.fval = psV1->v.fval * psV2->v.fval;
		}
		else
		{
			psV1->v.ival = psV1->v.ival * psV2->v.ival;
		}
		break;
	case OP_DIV:
		if(psV1->type == VAL_FLOAT || psV2->type == VAL_FLOAT)
		{
			if(psV2->v.fval != 0.0f)
			{
				psV1->type = VAL_FLOAT;
				psV1->v.fval = psV1->v.fval / psV2->v.fval;
			}
			else
			{
				ASSERT(FALSE, "stackBinaryOp: division by zero (float)");
			}
		}
		else
		{
			if(psV2->v.ival != 0)
			{
				psV1->v.ival = psV1->v.ival / psV2->v.ival;
			}
			else
			{
				ASSERT(FALSE, "stackBinaryOp: division by zero (integer)");
			}
		}
		break;
	case OP_AND:
		psV1->v.bval = psV1->v.bval && psV2->v.bval;
		break;
	case OP_OR:
		psV1->v.bval = psV1->v.bval || psV2->v.bval;
		break;
	case OP_EQUAL:
		psV1->type = VAL_BOOL;
		psV1->v.ival = psV1->v.ival == psV2->v.ival;
		break;
	case OP_NOTEQUAL:
		psV1->type = VAL_BOOL;
		psV1->v.ival = psV1->v.ival != psV2->v.ival;
		break;
	case OP_GREATEREQUAL:
		psV1->type = VAL_BOOL;
		psV1->v.bval = psV1->v.ival >= psV2->v.ival;
		break;
	case OP_LESSEQUAL:
		psV1->type = VAL_BOOL;
		psV1->v.bval = psV1->v.ival <= psV2->v.ival;
		break;
	case OP_GREATER:
		psV1->type = VAL_BOOL;
		psV1->v.bval = psV1->v.ival > psV2->v.ival;
		break;
	case OP_LESS:
		psV1->type = VAL_BOOL;
		psV1->v.bval = psV1->v.ival < psV2->v.ival;
		break;
	case OP_CONC:	//String concatenation
		{
			char tempstr1[MAXSTRLEN];
			char tempstr2[MAXSTRLEN];

			/* Check first value if it's compatible with Strings */
			switch (psV1->type)
			{
			case VAL_INT:		//first value isn't string, but can be converted to string
				sprintf(tempstr1,"%d",psV1->v.ival);	//int->string
				psV1->type = VAL_STRING;			//Mark as string
				psV1->v.sval =  (char*)MALLOC(MAXSTRLEN);				//allocate space for the string, since the result (string) of concatenation will be saved here
				break;

			case VAL_BOOL:
				sprintf(tempstr1,"%d",psV1->v.bval);	//bool->string
				psV1->type = VAL_STRING;			//Mark as string
				psV1->v.sval =  (char*)MALLOC(MAXSTRLEN);				//allocate space for the string, since the result (string) of concatenation will be saved here
				break;

			case VAL_FLOAT:
				sprintf(tempstr1,"%f",psV1->v.fval);	//float->string
				psV1->type = VAL_STRING;			//Mark as string
				psV1->v.sval =  (char*)MALLOC(MAXSTRLEN);				//allocate space for the string, since the result (string) of concatenation will be saved here
				break;

			case VAL_STRING:
				strcpy(tempstr1,psV1->v.sval);
				break;

			default:
				debug(LOG_ERROR, "stackBinaryOp: OP_CONC: first parameter is not compatible with Strings");
				return FALSE;
				break;
			}

			/* Check second value if it's compatible with Strings */
			switch (psV2->type)
			{
			case VAL_INT:
				sprintf(tempstr2,"%d",psV2->v.ival);		//int->string
				break;

			case VAL_BOOL:
				sprintf(tempstr2,"%d",psV2->v.bval);		//bool->string
				break;

			case VAL_FLOAT:
				sprintf(tempstr2,"%f",psV2->v.fval);		//float->string
				break;

			case VAL_STRING:
				strcpy(tempstr2,psV2->v.sval);
				break;

			default:
				debug(LOG_ERROR, "stackBinaryOp: OP_CONC: first parameter is not compatible with Strings");
				return FALSE;
				break;
			}

			strcat(tempstr1,tempstr2);
			strcpy(psV1->v.sval,tempstr1);		//Assign
		}
		break;

	default:
		debug(LOG_ERROR, "stackBinaryOp: unknown opcode");
		ASSERT( FALSE, "stackBinaryOp: unknown opcode" );
		return FALSE;
		break;
	}

	return TRUE;
}


/* Perform a unary operation on the top of the stack
 * This effectively pops a value and pushes the result
 */
BOOL stackUnaryOp(OPCODE opcode)
{
	STACK_CHUNK		*psChunk;
	INTERP_VAL		*psVal;

	// Get the value
	if (psCurrChunk->psPrev == NULL && currEntry == 0)
	{
		ASSERT( FALSE, "stackUnaryOp: not enough entries on stack" );
		return FALSE;
	}

	if (currEntry > 0)
	{
		psVal = psCurrChunk->aVals + currEntry - 1;
	}
	else
	{
		// Value is on the previous chunk
		psChunk = psCurrChunk->psPrev;
		psVal = psChunk->aVals + psChunk->size - 1;
	}

	// Do the operation
	switch (opcode)
	{
	case OP_NEG:
		switch (psVal->type)
		{
		case VAL_INT:
			psVal->v.ival = - psVal->v.ival;
			break;
		default:
			ASSERT( FALSE, "stackUnaryOp: invalid type for negation" );
			break;
		}
		break;
	case OP_NOT:
		switch (psVal->type)
		{
		case VAL_BOOL:
			psVal->v.bval = !psVal->v.bval;
			break;
		default:
			ASSERT( FALSE, "stackUnaryOp: invalid type for NOT" );
			break;
		}
		break;
	default:
		ASSERT( FALSE, "stackUnaryOp: unknown opcode" );
		break;
	}

	return TRUE;
}

BOOL castTop(INTERP_TYPE neededType)
{
	INTERP_VAL	top;

	//debug(LOG_WZ, "casting to %d", neededType);

	ASSERT(neededType == VAL_INT || neededType == VAL_FLOAT, "stackCast: can't cast to %d", neededType);
	
	if (!stackPop(&top))
	{
		return FALSE;
	}

	//debug(LOG_WZ, "castTop: stack type %d", top.type);

	/* see if we can cast this data type */
	switch (top.type)
	{
	case VAL_BOOL:

		switch (neededType)
		{
		case VAL_FLOAT:		/* casting from bool to float */

			return stackPushResult(neededType, &top);

			break;
		default:
			debug(LOG_ERROR, "cast error"); 
			break;
		}

	case VAL_INT:

		switch (neededType)
		{
		case VAL_FLOAT:		/* casting from int to float */

			top.v.fval = (float)top.v.ival;
			return  stackPushResult(neededType, &top);

			break;
		default:
			debug(LOG_ERROR, "cast error"); 
			break;
		}

	case VAL_FLOAT:

		switch (neededType)
		{
		case VAL_INT:		/* casting from float to int */

			top.v.ival = (SDWORD)top.v.fval;
			return  stackPushResult(neededType, &top);

			break;
		default:
			debug(LOG_ERROR, "cast error"); 
			break;
		}

	default:
		debug(LOG_ERROR, "can't cast from %d", top.type); 
		break;
	}

	return FALSE;
}


/* Initialise the stack */
BOOL stackInitialise(void)
{
	int				i;

	psStackBlock = memGetBlockHeap();

	psStackBase = (STACK_CHUNK *)MALLOC(sizeof(STACK_CHUNK));
	if (psStackBase == NULL)
	{
		debug( LOG_ERROR, "Out of memory" );
		abort();
		return FALSE;
	}
	psStackBase->aVals = (INTERP_VAL*)MALLOC(sizeof(INTERP_VAL) * INIT_SIZE);
	if (!psStackBase->aVals)
	{
		debug( LOG_ERROR, "Out of memory" );
		abort();
		return FALSE;
	}

	psStackBase->size = INIT_SIZE;
	psStackBase->psPrev = NULL;
	psStackBase->psNext = NULL;
	psCurrChunk = psStackBase;

	/* initialize pointers */
	for(i = 0; i<INIT_SIZE; i++)
	{
		psStackBase->aVals[i].v.ival = 0;
		psStackBase->aVals[i].type = VAL_INT;		//default type, should be ok
	}

	//string support
	CURSTACKSTR = 0;		//initialize string 'stack'

	return TRUE;
}



/* Shutdown the stack */
void stackShutDown(void)
{
	STACK_CHUNK		*psCurr, *psNext;
	UDWORD				i;

	if ((psCurrChunk != psStackBase) && (currEntry != 0))
	{
		debug( LOG_NEVER, "stackShutDown: stack is not empty on shutdown" );
	}

	for(psCurr = psStackBase; psCurr != NULL; psCurr = psNext)
	{
		psNext = psCurr->psNext;

		/* Free strings */
		for(i=0; i< psCurr->size; i++)		//go through all values on this chunk
		{
			if(psCurr->aVals[i].type == VAL_STRING)
			{
				if(psCurr->aVals[i].v.sval != NULL)					//FIXME: seems to be causing problems sometimes
				{
					debug(LOG_WZ, "freeing '%s' ", psCurr->aVals[i].v.sval);
					FREE(psCurr->aVals[i].v.sval);
				}
				else
				{
					debug(LOG_SCRIPT, "stackShutDown: VAL_STRING with null pointer");
				}
			}
		}

		FREE(psCurr->aVals);
		FREE(psCurr);
	}
}


/* Reset the stack to an empty state */
void stackReset(void)
{
	ASSERT( ((psCurrChunk == psStackBase) && (currEntry == 0)),
		"stackReset: stack is not empty" );

	psCurrChunk = psStackBase;
	currEntry = 0;
}



/* Get the first entry on the stack
 * (i.e. the second value for a binary operator).
 */
/*BOOL stackGetTop(UDWORD *pType, UDWORD *pData)
{
	STACK_CHUNK		*psChunk;
	INTERP_VAL		*psVal;

	if (psCurrChunk->psPrev == NULL && currEntry == 0)
	{
		ASSERT( FALSE, "stackGetTop: not enough entries on stack" );
		return FALSE;
	}

	if (currEntry == 0)
	{
		// Value is on the previous chunk
		psChunk = psCurrChunk->psPrev;
		psVal = psChunk->aVals + psChunk->size - 1;
	}
	else
	{
		psVal = psCurrChunk->aVals + currEntry - 1;
	}

	// The data could be any type but this will copy it over
	*pType = (UDWORD)psVal->type;
	*pData = (UDWORD)psVal->v.ival;

	return TRUE;
}*/


/* Replace the value at the top of the stack */
/*BOOL stackSetTop(UDWORD type, UDWORD data)
{
	STACK_CHUNK		*psChunk;
	INTERP_VAL		*psVal;

	if (psCurrChunk->psPrev == NULL && currEntry == 0)
	{
		ASSERT( FALSE, "stackSetTop: not enough entries on stack" );
		return FALSE;
	}

	if (currEntry == 0)
	{
		// Value is on the previous chunk
		psChunk = psCurrChunk->psPrev;
		psVal = psChunk->aVals + psChunk->size - 1;
	}
	else
	{
		psVal = psCurrChunk->aVals + currEntry - 1;
	}

	// The data could be any type but this will copy it over
	psVal->type = type;
	psVal->v.ival = (SDWORD)data;

	return TRUE;
}*/


/* Get the second entry on the stack
 * (i.e. the first value for a binary operator).
 */
/*BOOL stackGetSecond(UDWORD *pType, UDWORD *pData)
{
	STACK_CHUNK		*psChunk;
	INTERP_VAL		*psVal;

	if (psCurrChunk->psPrev == NULL && currEntry < 2)
	{
		ASSERT( FALSE, "stackGetSecond: not enough entries on stack" );
		return FALSE;
	}

	if (currEntry < 2)
	{
		// Value is on the previous chunk
		psChunk = psCurrChunk->psPrev;
		psVal = psChunk->aVals + psChunk->size + currEntry - 2;
	}
	else
	{
		psVal = psCurrChunk->aVals + currEntry - 2;
	}

	// The data could be any type but this will copy it over
	*pType = (UDWORD)psVal->type;
	*pData = (UDWORD)psVal->v.ival;

	return TRUE;
}*/


/* Get pointers to the two top values */
/*BOOL stackTopTwo(INTERP_VAL **ppsV1, INTERP_VAL **ppsV2)
{
	STACK_CHUNK		*psChunk;

	if (psCurrChunk->psPrev == NULL && currEntry < 2)
	{
		ASSERT( FALSE, "stackGetSecond: not enough entries on stack" );
		return FALSE;
	}

	if (currEntry > 1)
	{
		*ppsV1 = psCurrChunk->aVals + currEntry - 2;
		*ppsV2 = psCurrChunk->aVals + currEntry - 1;
	}
	else if (currEntry == 1)
	{
		// Value is on the previous chunk, but pop doesn't change the chunk
		psChunk = psCurrChunk->psPrev;
		*ppsV1 = psChunk->aVals + psChunk->size - 1;
		*ppsV2 = psCurrChunk->aVals + currEntry - 1;
	}
	else
	{
		// both on the previous chunk
		psChunk = psCurrChunk->psPrev;
		*ppsV1 = psChunk->aVals + psChunk->size - 2;
		*ppsV2 = psChunk->aVals + psChunk->size - 1;
	}

	return TRUE;
}*/


/* Pop the top value from the stack and replace the new top value
 * This is used to return the result of a binary maths operator
 */
/*BOOL stackPopAndSet(UDWORD type, UDWORD data)
{
	STACK_CHUNK		*psChunk;
	INTERP_VAL		*psVal;

	if (psCurrChunk->psPrev == NULL && currEntry < 2)
	{
		ASSERT( FALSE, "stackGetSecond: not enough entries on stack" );
		return FALSE;
	}

	if (currEntry > 1)
	{
		psVal = psCurrChunk->aVals + currEntry - 2;
		currEntry -= 1;
	}
	else if (currEntry == 1)
	{
		// Value is on the previous chunk, but pop doesn't change the chunk
		psChunk = psCurrChunk->psPrev;
		psVal = psChunk->aVals + psChunk->size - 1;
		currEntry -= 1;
	}
	else
	{
		// pop to the previous chunk
		psCurrChunk = psCurrChunk->psPrev;
		psVal = psCurrChunk->aVals + psCurrChunk->size - 2;
		currEntry = psCurrChunk->size - 1;
	}

	// The data could be any type but this will copy it over
	psVal->type = type;
	psVal->v.ival = (SDWORD)data;

	return TRUE;
}*/





