/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
 * CodePrint.c
 *
 * Routines for displaying compiled scripts
 */

#include "lib/framework/frame.h"
#include "interp.h"
#include "parse.h"
#include "codeprint.h"
#include "script.h"

/* Display a value type */
void cpPrintType(INTERP_TYPE type)
{
	UDWORD	i;
	BOOL	ref = false;

	if (type & VAL_REF)
	{
		ref = true;
		type = (INTERP_TYPE)(type & ~VAL_REF);
	}

	switch(type)
	{
	case VAL_BOOL:
	case VAL_INT:
	case VAL_FLOAT:
	case VAL_STRING:
	case VAL_TRIGGER:
	case VAL_EVENT:
	case VAL_VOID:
		debug( LOG_NEVER, interpTypeToString(type) );
		break;
	default:
		// See if it is a user defined type
		if (asScrTypeTab)
		{
			for(i=0; asScrTypeTab[i].typeID != 0; i++)
			{
				if (asScrTypeTab[i].typeID == type)
				{
					debug( LOG_NEVER, "%s", asScrTypeTab[i].pIdent );
					return;
				}
			}
		}
		ASSERT( false, "cpPrintType: Unknown type" );
		break;
	}

	if (ref)
	{
		debug( LOG_NEVER, " REF" );
	}
}


/* Display a value  */
void cpPrintVal(INTERP_VAL *psVal)
{
	UDWORD	i;

	if (psVal->type & VAL_REF)
	{
		debug( LOG_NEVER, "type: " );
		cpPrintType(psVal->type);
		debug( LOG_NEVER, " value: %x", psVal->v.ival );
		return;
	}

	switch(psVal->type)
	{
	case VAL_BOOL:
		debug( LOG_NEVER, "type: BOOL    value: %s", psVal->v.bval ? "true" : "false" );
		break;
	case VAL_INT:
		debug( LOG_NEVER, "type: INT     value: %d", psVal->v.ival );
		break;
	case VAL_FLOAT:
		debug( LOG_NEVER, "type: FLOAT   value: %f", psVal->v.fval );
		break;
	case VAL_STRING:
		debug( LOG_NEVER, "type: char    value: %s", psVal->v.sval );
		break;
	case VAL_TRIGGER:
		debug( LOG_NEVER, "type: TRIGGER value: %d", psVal->v.ival );
		break;
	case VAL_EVENT:
		debug( LOG_NEVER, "type: EVENT   value: %d", psVal->v.ival );
		break;
	default:
		// See if it is a user defined type
		if (asScrTypeTab)
		{
			for(i=0; asScrTypeTab[i].typeID != 0; i++)
			{
				if (asScrTypeTab[i].typeID == psVal->type)
				{
					debug( LOG_NEVER, "type: %s value: %x", asScrTypeTab[i].pIdent, psVal->v.ival );
					return;
				}
			}
		}
		ASSERT( false, "cpPrintVal: Unknown value type" );
		break;
	}
}


/* Display a value from a program that has been packed with an opcode */
void cpPrintPackedVal(INTERP_VAL *ip)
{
	INTERP_TYPE	type = (INTERP_TYPE)(ip->v.ival & OPCODE_DATAMASK);
	UDWORD		i;
	INTERP_VAL data;

	/* copy value */
	memcpy(&data, (INTERP_VAL *)(ip + 1), sizeof(INTERP_VAL));

	if (type & VAL_REF)
	{
		debug( LOG_NEVER, "type: " );
		cpPrintType(type);
		debug( LOG_NEVER, " value: %x", data.type );
		return;
	}

	switch(type)
	{
	case VAL_BOOL:
		debug( LOG_NEVER, "BOOL    : %s", data.v.bval ? "true" : "false" );
		break;
	case VAL_INT:
		debug( LOG_NEVER, "INT     : %d", data.v.ival);
		break;
	case VAL_FLOAT:
		debug( LOG_NEVER, "FLOAT   : %f", data.v.fval );
		break;
	case VAL_STRING:
		debug( LOG_NEVER, "char  : %s", data.v.sval );
		break;
	case VAL_TRIGGER:
		debug( LOG_NEVER, "TRIGGER : %d", data.v.ival );
		break;
	case VAL_EVENT:
		debug( LOG_NEVER, "EVENT   : %d", data.v.ival );
		break;
	default:
		// See if it is a user defined type
		if (asScrTypeTab)
		{
			for(i=0; asScrTypeTab[i].typeID != 0; i++)
			{
				if (asScrTypeTab[i].typeID == type)
				{
					debug( LOG_NEVER, "type: %s value: %x", asScrTypeTab[i].pIdent, data.v.ival );
					return;
				}
			}
		}
		ASSERT( false, "cpPrintVal: Unknown value type" );
		break;
	}
}


/* Display a maths operator */
void cpPrintMathsOp(OPCODE opcode)
{
	switch (opcode)
	{
	case OP_ADD:
		debug( LOG_NEVER, "ADD" );
		break;
	case OP_SUB:
		debug( LOG_NEVER, "SUB" );
		break;
	case OP_MUL:
		debug( LOG_NEVER, "MUL" );
		break;
	case OP_DIV:
		debug( LOG_NEVER, "DIV" );
		break;
	case OP_NEG:
		debug( LOG_NEVER, "NEG" );
		break;
	case OP_DEC:
		debug( LOG_NEVER, "DED" );
		break;
	case OP_INC:
		debug( LOG_NEVER, "INC" );
		break;
	case OP_AND:
		debug( LOG_NEVER, "AND" );
		break;
	case OP_OR:
		debug( LOG_NEVER, "OR" );
		break;
	case OP_NOT:
		debug( LOG_NEVER, "NOT" );
		break;
	case OP_EQUAL:
		debug( LOG_NEVER, "EQUAL" );
		break;
	case OP_NOTEQUAL:
		debug( LOG_NEVER, "NOT_EQUAL" );
		break;
	case OP_GREATEREQUAL:
		debug( LOG_NEVER, "GRT_EQUAL" );
		break;
	case OP_LESSEQUAL:
		debug( LOG_NEVER, "LESS_EQUAL" );
		break;
	case OP_GREATER:
		debug( LOG_NEVER, "GREATER" );
		break;
	case OP_LESS:
		debug( LOG_NEVER, "LESS" );
		break;
	case OP_CONC:
		debug( LOG_NEVER, "CONCAT" );
		break;
	default:
		ASSERT( false, "cpPrintMathsOp: unknown operator" );
		break;
	}
}

/* Print a function name */
void cpPrintFunc(SCRIPT_FUNC pFunc)
{
	SDWORD		i;

	// Search the instinct functions
	if (asScrInstinctTab)
	{
		for(i = 0; asScrInstinctTab[i].pFunc != NULL; i++)
		{
			if (asScrInstinctTab[i].pFunc == pFunc)
			{
				debug( LOG_NEVER, "%s", asScrInstinctTab[i].pIdent );
				return;
			}
		}
	}

	// Search the callback functions
	if (asScrCallbackTab)
	{
		for(i = 0; asScrCallbackTab[i].type != 0; i++)
		{
			if (asScrCallbackTab[i].pFunc == pFunc)
			{
				debug( LOG_NEVER, "%s", asScrCallbackTab[i].pIdent );
				return;
			}
		}
	}
}


/* Print a variable access function name */
void cpPrintVarFunc(SCRIPT_VARFUNC pFunc, UDWORD index)
{
	SDWORD		i;

	// Search the external variable functions
	if (asScrExternalTab)
	{
		for(i = 0; asScrExternalTab[i].pIdent != NULL; i++)
		{
			if (asScrExternalTab[i].set == pFunc &&
				asScrExternalTab[i].index == index)
			{
				debug( LOG_NEVER, "%s (set)", asScrExternalTab[i].pIdent );
				return;
			}
			else if (asScrExternalTab[i].get == pFunc &&
					 asScrExternalTab[i].index == index)
			{
				debug( LOG_NEVER, "%s (get)", asScrExternalTab[i].pIdent );
				return;
			}
		}
	}

	// Search the object functions
	if (asScrObjectVarTab)
	{
		for(i = 0; asScrObjectVarTab[i].pIdent != NULL; i++)
		{
			if (asScrObjectVarTab[i].set == pFunc &&
				asScrObjectVarTab[i].index == index)
			{
				debug( LOG_NEVER, "%s (set)", asScrObjectVarTab[i].pIdent );
				return;
			}
			else if (asScrObjectVarTab[i].get == pFunc &&
					 asScrObjectVarTab[i].index == index)
			{
				debug( LOG_NEVER, "%s (get)", asScrObjectVarTab[i].pIdent );
				return;
			}
		}
	}
}


/* Print the array information */
static void cpPrintArrayInfo(INTERP_VAL **pip, SCRIPT_CODE *psProg)
{
	SDWORD		i, dimensions;//, elements[VAR_MAX_DIMENSIONS];
//	SDWORD		elementDWords;
//	UBYTE		*pElem;
	INTERP_VAL		*ip = *pip;
	UDWORD		base;

	// get the base index of the array
	base = ip->v.ival & ARRAY_BASE_MASK;

	// get the number of dimensions
	dimensions = (ip->v.ival & ARRAY_DIMENSION_MASK) >> ARRAY_DIMENSION_SHIFT;

	// get the number of elements for each dimension
/*	pElem = (UBYTE *) (ip + 1);
	for(i=0; i<dimensions; i+=1)
	{
		elements[i] = *pElem;

		pElem += 1;
	}*/

	debug( LOG_NEVER, "%d->", base );
	for(i=0; i<psProg->psArrayInfo[base].dimensions; i+= 1)
	{
		debug( LOG_NEVER, "[%d]", psProg->psArrayInfo[base].elements[i] );
	}
	// calculate the number of DWORDs needed to store the number of elements for each dimension of the array
//	elementDWords = (dimensions - 1)/4 + 1;

	// update the instruction pointer
	*pip += 1;// + elementDWords;
}


/* Display the contents of a program in readable form */
void cpPrintProgram(SCRIPT_CODE *psProg)
{
	INTERP_VAL		*ip, *end;
	OPCODE			opcode;
	UDWORD			data, i, dim;
	SCRIPT_DEBUG	*psCurrDebug=NULL;
	BOOL			debugInfo, triggerCode;
	UDWORD			jumpOffset;
	VAR_DEBUG		*psCurrVar;
	ARRAY_DATA		*psCurrArray;
	ARRAY_DEBUG		*psCurrArrayDebug;

	ASSERT( psProg != NULL,
		"cpPrintProgram: Invalid program pointer" );

	debugInfo = psProg->psDebug != NULL;

	if (debugInfo)
	{
		// Print out the global variables
		if (psProg->numGlobals > 0)
		{
			debug( LOG_NEVER, "index  storage  type variable name\n" );
			psCurrVar = psProg->psVarDebug;
			for(i=0; i<psProg->numGlobals; i++)
			{
				debug( LOG_NEVER, "%-6d %s  %-4d %s\n", (int)(psCurrVar - psProg->psVarDebug), psCurrVar->storage == ST_PUBLIC ? "Public " : "Private", psProg->pGlobals[i], psCurrVar->pIdent );
				psCurrVar++;
			}
		}

		if (psProg->numArrays > 0)
		{
			debug( LOG_NEVER, "\narrays\n" );
			psCurrArray = psProg->psArrayInfo;
			psCurrArrayDebug = psProg->psArrayDebug;
			for(i=0; i<psProg->numArrays; i++)
			{
				debug( LOG_NEVER, "%-6d %s  %-4d %s", i, psCurrArrayDebug->storage == ST_PUBLIC ? "Public " : "Private", psCurrArray->type, psCurrArrayDebug->pIdent );
				for(dim=0; dim < psCurrArray->dimensions; dim += 1)
				{
					debug( LOG_NEVER, "[%d]", psCurrArray->elements[dim] );
				}
				debug( LOG_NEVER, "\n" );
				psCurrArray++;
				psCurrArrayDebug++;
			}
		}

		debug( LOG_NEVER, "\nindex line offset\n" );
		psCurrDebug = psProg->psDebug;
	}
	else
	{
		debug( LOG_NEVER, "index offset\n" );
	}

	// Find the first trigger with code
	for(jumpOffset = 0; jumpOffset < psProg->numTriggers; jumpOffset++)
	{
		if (psProg->psTriggerData[jumpOffset].type == TR_CODE)
		{
			break;
		}
	}

	ip = psProg->pCode;
	triggerCode = psProg->numTriggers > 0 ? true : false;
	end = (INTERP_VAL *)(((UBYTE *)ip) + psProg->size);
	opcode = (OPCODE)(ip->v.ival >> OPCODE_SHIFT);
	data = (ip->v.ival & OPCODE_DATAMASK);
	while (ip < end)
	{
		// display a label if there is one
		if (debugInfo)
		{
			if ((UDWORD)(ip - psProg->pCode) == psCurrDebug->offset &&
				psCurrDebug->pLabel != NULL)
			{
				debug( LOG_NEVER, "%s\n", psCurrDebug->pLabel );
			}
		}

		// Display the trigger/event index
		if (triggerCode)
		{
			if (ip - psProg->pCode == psProg->pTriggerTab[jumpOffset])
			{
				debug( LOG_NEVER, "%-6d", jumpOffset );
				jumpOffset+= 1;
				// Find the next trigger with code
				while(jumpOffset < psProg->numTriggers)
				{
					if (psProg->psTriggerData[jumpOffset].type == TR_CODE)
					{
						break;
					}
					jumpOffset++;
				}
				if (jumpOffset >= psProg->numTriggers)
				{
					// Got to the end of the triggers
					triggerCode = false;
					jumpOffset = 0;
				}
			}
		}
		else
		{
			if (ip - psProg->pCode == psProg->pEventTab[jumpOffset])
			{
				debug( LOG_NEVER, "%-6d", jumpOffset );
				jumpOffset+= 1;
			}
		}

		// Display the line number
		if (debugInfo)
		{
			if ((UDWORD)(ip - psProg->pCode) == psCurrDebug->offset)
			{
				debug( LOG_NEVER, "%-6d", psCurrDebug->line );
				psCurrDebug++;
			}
		}

		// Display the code offset
		debug( LOG_NEVER, "%-6d", (int)(ip - psProg->pCode) );
		switch (opcode)
		{
		case OP_PUSH:
			debug( LOG_NEVER, "PUSH" );
			cpPrintPackedVal(ip);
			debug( LOG_NEVER, "\n" );
			ip += aOpSize[opcode];
			break;
		case OP_PUSHREF:
			debug( LOG_NEVER, "PUSHREF" );
			cpPrintPackedVal(ip);
			debug( LOG_NEVER, "\n" );
			ip += aOpSize[opcode];
			break;
		case OP_POP:
			debug( LOG_NEVER, "POP\n" );
			ip += aOpSize[opcode];
			break;
		case OP_PUSHGLOBAL:
			debug( LOG_NEVER, "PUSHGLOBAL %d\n", data );
			ip += aOpSize[opcode];
			break;
		case OP_POPGLOBAL:
			debug( LOG_NEVER, "POPGLOBAL  %d\n", data );
			ip += aOpSize[opcode];
			break;
		case OP_PUSHARRAYGLOBAL:
			debug( LOG_NEVER, "PUSHARRAYGLOBAL" );
			cpPrintArrayInfo(&ip, psProg);
			debug( LOG_NEVER, "\n" );
			break;
		case OP_POPARRAYGLOBAL:
			debug( LOG_NEVER, "POPARRAYGLOBAL" );
			cpPrintArrayInfo(&ip, psProg);
			debug( LOG_NEVER, "\n" );
			break;
		case OP_CALL:
			debug( LOG_NEVER, "CALL" );
			cpPrintFunc( ((INTERP_VAL *)(ip+1))->v.pFuncExtern );
			debug( LOG_NEVER, "\n" );
			ip += aOpSize[opcode];
			break;
		case OP_VARCALL:
			debug( LOG_NEVER, "VARCALL" );
			cpPrintVarFunc( ((INTERP_VAL *)(ip+1))->v.pObjGetSet, data);
			debug( LOG_NEVER, "(%d)\n", data );
			ip += aOpSize[opcode];
			break;
		case OP_JUMP:
			debug( LOG_NEVER, "JUMP       %d (%d)\n", (SWORD)data, (int)(ip - psProg->pCode + (SWORD)data) );
			ip += aOpSize[opcode];
			break;
		case OP_JUMPTRUE:
			debug( LOG_NEVER, "JUMPTRUE   %d (%d)\n", (SWORD)data, (int)(ip - psProg->pCode + (SWORD)data) );
			ip += aOpSize[opcode];
			break;
		case OP_JUMPFALSE:
			debug( LOG_NEVER, "JUMPFALSE  %d (%d)\n", (SWORD)data, (int)(ip - psProg->pCode + (SWORD)data) );
			ip += aOpSize[opcode];
			break;
		case OP_BINARYOP:
		case OP_UNARYOP:
			cpPrintMathsOp(data);
			debug( LOG_NEVER, "\n" );
			ip += aOpSize[opcode];
			break;
		case OP_EXIT:
			debug( LOG_NEVER, "EXIT\n" );
			ip += aOpSize[opcode];
			break;
		case OP_PAUSE:
			debug( LOG_NEVER, "PAUSE %d\n", data );
			ip += aOpSize[opcode];
			break;
		default:
			ASSERT( false,"cpPrintProgram: Unknown opcode: %x", ip->type );
			break;
		}

		ASSERT( (ip <= end) || ip != NULL,
			"cpPrintProgram: instruction pointer no longer valid" );

		opcode = (OPCODE)(ip->v.ival >> OPCODE_SHIFT);
		data = ip->v.ival & OPCODE_DATAMASK;
	}
}
