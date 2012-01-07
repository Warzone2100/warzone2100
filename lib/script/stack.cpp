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
 * Stack.c
 *
 * A stack for the script interpreter.
 */

#include <stdarg.h>
#include <string.h>


#include "lib/framework/frame.h"
#include "lib/framework/string_ext.h"
#include "interpreter.h"
#include "stack.h"
#include "codeprint.h"
#include "script.h"

/* number of values in each stack chunk */
#define INIT_SIZE		30	//15
#define EXT_SIZE		10	//2

char STRSTACK[MAXSTACKLEN][MAXSTRLEN]; //simple string 'stack'
UDWORD CURSTACKSTR = 0;    //Points to the top of the string stack

/* store for a 'chunk' of the stack */
struct STACK_CHUNK
{
	INTERP_VAL	*aVals;
	UDWORD		size;

	STACK_CHUNK *   psNext, *psPrev;
};

/* The first chunk of the stack */
static STACK_CHUNK		*psStackBase=NULL;

/* The current stack chunk */
static STACK_CHUNK		*psCurrChunk=NULL;

/* The current free entry on the current stack chunk */
static UDWORD			currEntry=0;

/* Get rid of the top value without returning it */
static inline bool stackRemoveTop(void);


/* Check if the stack is empty */
bool stackEmpty(void)
{
	return (psCurrChunk->psPrev == NULL && currEntry == 0);
}


/* Allocate a new chunk for the stack */
static bool stackNewChunk(UDWORD size)
{
	/* see if a chunk has already been allocated */
	if (psCurrChunk->psNext != NULL)
	{
		psCurrChunk = psCurrChunk->psNext;
		currEntry = 0;
	}
	else
	{
		/* Allocate a new chunk */
		psCurrChunk->psNext = (STACK_CHUNK *)calloc(1, sizeof(*psCurrChunk->psNext));
		if (!psCurrChunk->psNext)
		{
			return false;
		}
		psCurrChunk->psNext->aVals = (INTERP_VAL *)calloc(size, sizeof(*psCurrChunk->psNext->aVals));
		if (!psCurrChunk->psNext->aVals)
		{
			free(psCurrChunk->psNext);
			psCurrChunk->psNext = NULL;
			return false;
		}

		psCurrChunk->psNext->size = size;
		psCurrChunk->psNext->psPrev = psCurrChunk;
		psCurrChunk->psNext->psNext = NULL;
		psCurrChunk = psCurrChunk->psNext;
		currEntry = 0;
	}

	return true;
}


/* Push a value onto the stack */
bool stackPush(INTERP_VAL  *psVal)
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
			psCurrChunk->aVals[currEntry].v.sval = (char*)malloc(MAXSTRLEN);

		strcpy(psCurrChunk->aVals[currEntry].v.sval, psVal->v.sval);		//copy string to stack
		psCurrChunk->aVals[currEntry].type = VAL_STRING;
	}
	else		/* pushing non-string */
	{
		/* free stack var allocated string memory, if stack var used to be of type VAL_STRING */
		if(psCurrChunk->aVals[currEntry].type == VAL_STRING)
		{
			free(psCurrChunk->aVals[currEntry].v.sval);			//don't need it anymore
			psCurrChunk->aVals[currEntry].v.sval = NULL;
		}

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
			return false;
		}
	}

	return true;
}



/* Pop a value off the stack */
bool stackPop(INTERP_VAL  *psVal)
{
	if (stackEmpty())
	{
		debug(LOG_ERROR, "stackPop: stack empty");
		ASSERT( false, "stackPop: stack empty" );
		return false;
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

	return true;
}

/* Return pointer to the top value without poping it */
bool stackPeekTop(INTERP_VAL  **ppsVal)
{
	if ((psCurrChunk->psPrev == NULL) && (currEntry == 0))
	{
		debug(LOG_ERROR, "stackPeekTop: stack empty");
		ASSERT( false, "stackPeekTop: stack empty" );
		return false;
	}

	if (currEntry == 0)
	{
		STACK_CHUNK	*pPrevChunck;

		/* have to move onto the previous chunk */
		pPrevChunck = psCurrChunk->psPrev;

		/* Return top value */
		*ppsVal = &(pPrevChunck->aVals[pPrevChunck->size - 1]);
	}
	else
	{
		/* Return top value of the current chunk */
		*ppsVal = &(psCurrChunk->aVals[currEntry - 1]);
	}

	return true;
}


/* Pop a value off the stack, checking that the type matches what is passed in */
bool stackPopType(INTERP_VAL  *psVal)
{
	INTERP_VAL	*psTop;

	if ((psCurrChunk->psPrev == NULL) && (currEntry == 0))
	{
		debug(LOG_ERROR, "stackPopType: stack empty");
		ASSERT( false, "stackPopType: stack empty" );
		return false;
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
					return false;
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
			debug(LOG_ERROR, "stackPopType: type mismatch: %d/%d", psVal->type, psTop->type);
			ASSERT( !"type mismatch", "stackPopType: type mismatch: %d/%d", psVal->type, psTop->type);
			return false;
		}

		/* copy the entire union off the stack */
		memcpy(&(psVal->v), &(psTop->v), sizeof(psTop->v));
	}

	return true;
}


/* Pop a number of values off the stack checking their types
 * This is used by instinct functions to get their parameters
 */
bool stackPopParams(unsigned int numParams, ...)
{
	va_list args;
	unsigned int i, index;
	STACK_CHUNK *psCurr;

	// Find the position of the first parameter, and set
	// the stack top to it
	if (numParams <= currEntry)
	{
		// parameters are all on current chunk
		currEntry = currEntry - numParams;
		psCurr = psCurrChunk;
	}
	else
	{
		// Have to work down the previous chunks to find the first param
		unsigned int params = numParams - currEntry;

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
		debug( LOG_ERROR, "stackPopParams: not enough parameters on stack" );
		ASSERT( false, "stackPopParams: not enough parameters on stack" );
		return false;
	}

	va_start(args, numParams);

	// Get the values, checking their types
	index = currEntry;
	for (i = 0; i < numParams; i++)
	{
		INTERP_VAL *psVal = psCurr->aVals + index;
		INTERP_TYPE type = (INTERP_TYPE)va_arg(args, int);
		void * pData = va_arg(args, void *);

		switch (type)
		{
			case VAL_BOOL:
				*(int32_t*)pData = psVal->v.bval;
				break;
			case VAL_INT:
				*(int*)pData = psVal->v.ival;
				break;
			case VAL_FLOAT:
				*(float*)pData = psVal->v.fval;
				break;
			case VAL_STRING:
				switch (psVal->type)
				{
					case VAL_BOOL:
						sprintf((char*)pData, "%d", psVal->v.bval);
						break;
					case VAL_INT:
						sprintf((char*)pData, "%d", psVal->v.ival);
						break;
					case VAL_FLOAT:
						sprintf((char*)pData, "%f", psVal->v.fval);
						break;
					case VAL_STRING:
						strcpy((char*)pData, psVal->v.sval);
						break;
					default:
						ASSERT(false,
							"StackPopParam - wrong data type being converted to string (type: %s)",
							scriptTypeToString(psVal->type));
						break;
				}
				break;
			default: // anything (including references)
				if (scriptTypeIsPointer(psVal->type))
				{
					if (psVal->type >= VAL_REF) // if it's a reference
					{
						INTERP_VAL *refVal = (INTERP_VAL *)psVal->v.oval; // oval is a pointer to INTERP_VAL in this case

						/* doublecheck types */
						ASSERT(interpCheckEquiv((INTERP_TYPE)(type & ~VAL_REF), refVal->type),
							"stackPopParams: type mismatch for a reference (expected %s, got %s)",
							scriptTypeToString((INTERP_TYPE)(type & ~VAL_REF)), scriptTypeToString(refVal->type));
						// type of provided container and type of the INTERP_VAL pointed by psVal->v.oval

						*(void**)pData = &(refVal->v); /* get pointer to the union */
					}
					else // some pointer type
					{
						*(void**)pData = psVal->v.oval;
					}
				}
				else
				{
					*(int*)pData = psVal->v.ival;
				}
		}

		index++;
		if (index >= psCurr->size)
		{
			psCurr = psCurr->psNext;
			index = 0;
		}
	}

	va_end(args);
	return true;
}


/* Push a value onto the stack without using a value structure
	NOTE: result->type is _not_ set yet - use 'type' instead
*/
bool stackPushResult(INTERP_TYPE type, INTERP_VAL *result)
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
			psCurrChunk->aVals[currEntry].v.sval = (char*)malloc(MAXSTRLEN);

		strcpy(psCurrChunk->aVals[currEntry].v.sval, result->v.sval);		//copy string to stack
		psCurrChunk->aVals[currEntry].type = VAL_STRING;
	}
	else		/* pushing non-string */
	{
		/* free stack var allocated string memory, if stack var used to be of type VAL_STRING */
		if(psCurrChunk->aVals[currEntry].type == VAL_STRING)
		{
			free(psCurrChunk->aVals[currEntry].v.sval);			//don't need it anymore
			psCurrChunk->aVals[currEntry].v.sval = NULL;
		}

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
			return false;
		}
	}

	return true;
}


/* Look at a value on the stack without removing it.
 * index is how far down the stack to look.
 * Index 0 is the top entry on the stack.
 */
bool stackPeek(INTERP_VAL *psVal, UDWORD index)
{
	STACK_CHUNK		*psCurr;

	if (index < currEntry)
	{
		/* Looking at entry on current chunk */
		memcpy(psVal, &(psCurrChunk->aVals[currEntry - index - 1]), sizeof(INTERP_VAL));
		return true;
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
				return true;
			}
			else
			{
				index -= psCurr->size;
			}
		}
	}

	/* If we got here the index is off the bottom of the stack */
	ASSERT( false, "stackPeek: index too large" );
	return false;
}


/* Print the top value on the stack */
void stackPrintTop(void)
{
	INTERP_VAL sVal;
	if (stackPeek(&sVal, 0))
	{
		cpPrintVal(sVal);
	}
	else
	{
		debug( LOG_NEVER, "STACK EMPTY" );
	}
}


/* Do binary operations on the top of the stack
 * This effectively pops two values and pushes the result
 */
bool stackBinaryOp(OPCODE opcode)
{
	STACK_CHUNK		*psChunk;
	INTERP_VAL		*psV1, *psV2;

	// Get the parameters
	if (psCurrChunk->psPrev == NULL && currEntry < 2)
	{
		debug(LOG_ERROR, "stackBinaryOp: not enough entries on stack");
		ASSERT( false, "stackBinaryOp: not enough entries on stack" );
		return false;
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
			ASSERT( false, "stackBinaryOp: type mismatch" );
			return false;
		}

		/* find out if the result will be a float. Both or neither arguments are floats - should be taken care of by bison*/
		if(psV1->type == VAL_FLOAT || psV2->type == VAL_FLOAT)
		{
			ASSERT(psV1->type == VAL_FLOAT && psV2->type == VAL_FLOAT, "Can't implicitly convert float->int (type1: %d, type2: %d)", psV1->type , psV2->type );
		}
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
				debug(LOG_ERROR, "stackBinaryOp: division by zero (float)");
				return false;
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
				debug(LOG_ERROR, "stackBinaryOp: division by zero (integer)");
				return false;
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
		if(psV1->type == VAL_FLOAT && psV2->type == VAL_FLOAT){
			psV1->v.bval = psV1->v.fval == psV2->v.fval;
		}else if(psV1->type == VAL_STRING && psV2->type == VAL_STRING){
			psV1->v.bval = (strcasecmp(psV1->v.sval,psV2->v.sval) == 0);	/* case-insensitive */
		}
		else if (scriptTypeIsPointer(psV1->type))
		{
			psV1->v.bval = scriptOperatorEquals(*psV1, *psV2);
		}
		else
		{
			psV1->v.bval = psV1->v.ival == psV2->v.ival;
		}
		psV1->type = VAL_BOOL;
		break;
	case OP_NOTEQUAL:
		if(psV1->type == VAL_FLOAT && psV2->type == VAL_FLOAT){
			psV1->v.bval = psV1->v.fval != psV2->v.fval;
		}else if(psV1->type == VAL_STRING && psV2->type == VAL_STRING){
			psV1->v.bval = (strcasecmp(psV1->v.sval,psV2->v.sval) != 0);	/* case-insensitive */
		}
		else if (scriptTypeIsPointer(psV1->type))
		{
			psV1->v.bval = !scriptOperatorEquals(*psV1, *psV2);
		}
		else
		{
			psV1->v.bval = psV1->v.ival != psV2->v.ival;
		}

		psV1->type = VAL_BOOL;
		break;
	case OP_GREATEREQUAL:
		if(psV1->type == VAL_FLOAT || psV2->type == VAL_FLOAT){
			psV1->v.bval = psV1->v.fval >= psV2->v.fval;
		}else{
			psV1->v.bval = psV1->v.ival >= psV2->v.ival;
		}
		psV1->type = VAL_BOOL;
		break;
	case OP_LESSEQUAL:
		if(psV1->type == VAL_FLOAT || psV2->type == VAL_FLOAT){
			psV1->v.bval = psV1->v.fval <= psV2->v.fval;
		}else{
			psV1->v.bval = psV1->v.ival <= psV2->v.ival;
		}
		psV1->type = VAL_BOOL;
		break;
	case OP_GREATER:
		if(psV1->type == VAL_FLOAT || psV2->type == VAL_FLOAT){
			psV1->v.bval = psV1->v.fval > psV2->v.fval;
		}else{
			psV1->v.bval = psV1->v.ival > psV2->v.ival;
		}
		psV1->type = VAL_BOOL;
		break;
	case OP_LESS:
		if(psV1->type == VAL_FLOAT || psV2->type == VAL_FLOAT){
			psV1->v.bval = psV1->v.fval < psV2->v.fval;
		}else{
			psV1->v.bval = psV1->v.ival < psV2->v.ival;
		}
		psV1->type = VAL_BOOL;
		break;
	case OP_CONC:	//String concatenation
		{
			char tempstr1[MAXSTRLEN];
			char tempstr2[MAXSTRLEN];

			/* Check first value if it's compatible with Strings */
			switch (psV1->type)
			{
			case VAL_INT:		//first value isn't string, but can be converted to string
				snprintf(tempstr1, sizeof(tempstr1), "%d", psV1->v.ival);	//int->string
				psV1->type = VAL_STRING;			//Mark as string
				psV1->v.sval =  (char*)malloc(MAXSTRLEN);				//allocate space for the string, since the result (string) of concatenation will be saved here
				break;

			case VAL_BOOL:
				snprintf(tempstr1, sizeof(tempstr1), "%d",psV1->v.bval);	//bool->string
				psV1->type = VAL_STRING;			//Mark as string
				psV1->v.sval =  (char*)malloc(MAXSTRLEN);				//allocate space for the string, since the result (string) of concatenation will be saved here
				break;

			case VAL_FLOAT:
				snprintf(tempstr1, sizeof(tempstr1), "%f", psV1->v.fval);	//float->string
				psV1->type = VAL_STRING;			//Mark as string
				psV1->v.sval =  (char*)malloc(MAXSTRLEN);				//allocate space for the string, since the result (string) of concatenation will be saved here
				break;

			case VAL_STRING:
				sstrcpy(tempstr1, psV1->v.sval);
				break;

			default:
				debug(LOG_ERROR, "stackBinaryOp: OP_CONC: first parameter is not compatible with Strings");
				return false;
				break;
			}

			/* Check second value if it's compatible with Strings */
			switch (psV2->type)
			{
			case VAL_INT:
				snprintf(tempstr2, sizeof(tempstr2), "%d", psV2->v.ival);		//int->string
				break;

			case VAL_BOOL:
				snprintf(tempstr2, sizeof(tempstr2), "%d", psV2->v.bval);		//bool->string
				break;

			case VAL_FLOAT:
				snprintf(tempstr2, sizeof(tempstr2), "%f", psV2->v.fval);		//float->string
				break;

			case VAL_STRING:
				sstrcpy(tempstr2, psV2->v.sval);
				break;

			default:
				debug(LOG_ERROR, "stackBinaryOp: OP_CONC: first parameter is not compatible with Strings");
				return false;
				break;
			}

			sstrcat(tempstr1, tempstr2);

			strcpy(psV1->v.sval,tempstr1);		//Assign
		}
		break;

	default:
		debug(LOG_ERROR, "stackBinaryOp: unknown opcode");
		ASSERT( false, "stackBinaryOp: unknown opcode" );
		return false;
		break;
	}

	return true;
}


/* Perform a unary operation on the top of the stack
 * This effectively pops a value and pushes the result
 */
bool stackUnaryOp(OPCODE opcode)
{
	STACK_CHUNK		*psChunk;
	INTERP_VAL		*psVal;

	// Get the value
	if (psCurrChunk->psPrev == NULL && currEntry == 0)
	{
		ASSERT( false, "stackUnaryOp: not enough entries on stack" );
		return false;
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
	case OP_INC:
		switch ((unsigned)psVal->type)  // Unsigned cast to suppress compiler warnings due to enum abuse.
		{
		case (VAL_REF | VAL_INT):

			psVal = (INTERP_VAL*)psVal->v.oval;		//get variable

			ASSERT(psVal->type == VAL_INT, "Invalid type for increment opcode: %d", psVal->type);

			psVal->v.ival++;

			/* Just get rid of the variable pointer, since already increased it */
			if (!stackRemoveTop())
			{
				debug( LOG_ERROR, "stackUnaryOpcode: OP_INC: could not pop" );
				return false;
			}

			break;
		default:
			ASSERT( false, "stackUnaryOp: invalid type for OP_INC (type: %d)", psVal->type );
			return false;
			break;
		}
		break;
	case OP_DEC:
		switch ((unsigned)psVal->type)  // Unsigned cast to suppress compiler warnings due to enum abuse.
		{
		case (VAL_REF | VAL_INT):

			psVal = (INTERP_VAL*)psVal->v.oval;		//get variable

			ASSERT(psVal->type == VAL_INT, "Invalid type for decrement opcode: %d", psVal->type);

			psVal->v.ival--;

			/* Just get rid of the variable pointer, since already decreased it */
			if (!stackRemoveTop())
			{
				debug( LOG_ERROR, "stackUnaryOpcode: OP_DEC: could not pop" );
				return false;
			}
			break;
		default:
			ASSERT( false, "stackUnaryOp: invalid type for OP_DEC (type: %d)", psVal->type );
			return false;
			break;
		}
		break;

	case OP_NEG:
		switch (psVal->type)
		{
		case VAL_INT:
			psVal->v.ival = - psVal->v.ival;
			break;
		case VAL_FLOAT:
			psVal->v.fval = - psVal->v.fval;
			break;
		default:
			ASSERT( false, "stackUnaryOp: invalid type for negation (type: %d)", psVal->type );
			return false;
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
			ASSERT( false, "stackUnaryOp: invalid type for NOT (type: %d)", psVal->type );
			return false;
			break;
		}
		break;
	default:
		ASSERT( false, "stackUnaryOp: unknown opcode (opcode: %d)", opcode );
		return false;
		break;
	}

	return true;
}

bool stackCastTop(INTERP_TYPE neededType)
{
	INTERP_VAL *pTop;

	ASSERT(neededType == VAL_INT || neededType == VAL_FLOAT,
		"stackCast: can't cast to %d", neededType);

	if (!stackPeekTop(&pTop) || pTop==NULL)
	{
		ASSERT(false, "castTop: failed to peek stack");
		return false;
	}

	/* see if we can cast this data type */
	switch (pTop->type)
	{
		case VAL_BOOL:
			switch (neededType)
			{
				case VAL_FLOAT:		/* casting from bool to float */

					pTop->v.fval = (float)pTop->v.bval;
					pTop->type = VAL_FLOAT;
					return true;
				default:
					break;
			}
			break;
		case VAL_INT:
			switch (neededType)
			{
				case VAL_FLOAT:		/* casting from int to float */
					pTop->v.fval = (float)pTop->v.ival;
					pTop->type = VAL_FLOAT;
					return true;
				default:
					break;
			}
			break;
		case VAL_FLOAT:
			switch (neededType)
			{
				case VAL_INT:		/* casting from float to int */
					pTop->v.ival = (int)pTop->v.fval;
					pTop->type = VAL_INT;
					return true;
				default:
					break;
			}
			break;
		default:
			break;
	}

	debug(LOG_ERROR, "can't cast from %s to %s",
		scriptTypeToString(pTop->type), scriptTypeToString(neededType));

	return false;
}


/* Initialise the stack */
bool stackInitialise(void)
{
	psStackBase = (STACK_CHUNK *)calloc(1, sizeof(*psStackBase));
	if (psStackBase == NULL)
	{
		debug( LOG_FATAL, "Out of memory" );
		abort();
		return false;
	}
	psStackBase->aVals = (INTERP_VAL *)calloc(INIT_SIZE, sizeof(*psStackBase->aVals));
	if (!psStackBase->aVals)
	{
		debug( LOG_FATAL, "Out of memory" );
		abort();
		return false;
	}

	psStackBase->size = INIT_SIZE;
	psStackBase->psPrev = NULL;
	psStackBase->psNext = NULL;
	psCurrChunk = psStackBase;

	//string support
	CURSTACKSTR = 0;		//initialize string 'stack'

	return true;
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
					free(psCurr->aVals[i].v.sval);
					psCurr->aVals[i].v.sval = NULL;
				}
				else
				{
					debug(LOG_SCRIPT, "stackShutDown: VAL_STRING with null pointer");
				}
			}
		}

		free(psCurr->aVals);
		free(psCurr);
	}
	psStackBase = NULL;
}

/* Get rid of the top value without returning it */
static inline bool stackRemoveTop(void)
{
	if ((psCurrChunk->psPrev == NULL) && (currEntry == 0))
	{
		debug(LOG_ERROR, "stackRemoveTop: stack empty");
		ASSERT( false, "stackRemoveTop: stack empty" );
		return false;
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

	return true;
}


/* Reset the stack to an empty state */
void stackReset(void)
{
	ASSERT( ((psCurrChunk == psStackBase) && (currEntry == 0)),
		"stackReset: stack is not empty" );

	psCurrChunk = psStackBase;
	currEntry = 0;
}
