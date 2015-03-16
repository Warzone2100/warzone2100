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
 * CodePrint.c
 *
 * Routines for displaying compiled scripts
 */

#include "lib/framework/frame.h"
#include "interpreter.h"
#include "parse.h"
#include "codeprint.h"
#include "script.h"


/* Display a value  */
void cpPrintVal(INTERP_VAL value)
{
	if (value.type & VAL_REF)
	{
		value.type = (INTERP_TYPE)(value.type & ~VAL_REF);
		debug(LOG_NEVER, "type: %5s, value: %i (ref)",
		      scriptTypeToString(value.type), value.v.ival);
		return;
	}

	switch (value.type)
	{
	case VAL_BOOL:
		debug(LOG_NEVER, "type: %5s, value: %s",
		      scriptTypeToString(value.type),
		      bool2string(value.v.bval));
		break;
	case VAL_INT:
		debug(LOG_NEVER, "type: %5s, value: %d",
		      scriptTypeToString(value.type),
		      value.v.ival);
		break;
	case VAL_FLOAT:
		debug(LOG_NEVER, "type: %5s, value: %f",
		      scriptTypeToString(value.type),
		      value.v.fval);
		break;
	case VAL_STRING:
		debug(LOG_NEVER, "type: %5s, value: %s",
		      scriptTypeToString(value.type),
		      value.v.sval);
		break;
	case VAL_TRIGGER:
	case VAL_EVENT:
	default:
		debug(LOG_NEVER, "type: %5s, value: %d",
		      scriptTypeToString(value.type),
		      value.v.ival);
		break;
	}
}


/* Display a value from a program that has been packed with an opcode */
void cpPrintPackedVal(INTERP_VAL *ip)
{
	INTERP_VAL data = {(INTERP_TYPE)(ip->v.ival & OPCODE_DATAMASK), {0}};
	data.v = (ip + 1)->v;

	cpPrintVal(data);
}


/* Print a variable access function name */
void cpPrintVarFunc(SCRIPT_VARFUNC pFunc, UDWORD index)
{
	// Search the external variable functions
	if (asScrExternalTab)
	{
		unsigned int i;
		for (i = 0; asScrExternalTab[i].pIdent != NULL; i++)
		{
			if (asScrExternalTab[i].index == index)
			{
				if (asScrExternalTab[i].set == pFunc)
				{
					debug(LOG_NEVER, "%s (set)", asScrExternalTab[i].pIdent);
					return;
				}
				else if (asScrExternalTab[i].get == pFunc)
				{
					debug(LOG_NEVER, "%s (get)", asScrExternalTab[i].pIdent);
					return;
				}
			}
		}
	}

	// Search the object functions
	if (asScrObjectVarTab)
	{
		unsigned int i;
		for (i = 0; asScrObjectVarTab[i].pIdent != NULL; i++)
		{
			if (asScrObjectVarTab[i].index == index)
			{
				if (asScrObjectVarTab[i].set == pFunc)
				{
					debug(LOG_NEVER, "%s (set)", asScrObjectVarTab[i].pIdent);
					return;
				}
				else if (asScrObjectVarTab[i].get == pFunc)
				{
					debug(LOG_NEVER, "%s (get)", asScrObjectVarTab[i].pIdent);
					return;
				}
			}
		}
	}
}


/* Print the array information */
static void cpPrintArrayInfo(INTERP_VAL *ip, SCRIPT_CODE *psProg)
{
	// get the base index of the array
	unsigned int base = (ip->v.ival & ARRAY_BASE_MASK);
	unsigned int i;

	debug(LOG_NEVER, "-> %d", base);
	for (i = 0; i < psProg->psArrayInfo[base].dimensions; i++)
	{
		debug(LOG_NEVER, "   [%d]", psProg->psArrayInfo[base].elements[i]);
	}
}


/* Display the contents of a program in readable form */
void cpPrintProgram(SCRIPT_CODE *psProg)
{
	INTERP_VAL		*ip, *end;
	OPCODE			opcode;
	UDWORD			data, i, dim;
	SCRIPT_DEBUG	*psCurrDebug = NULL;
	int32_t			debugInfo, triggerCode;
	UDWORD			jumpOffset;
	VAR_DEBUG		*psCurrVar;
	ARRAY_DATA		*psCurrArray;
	ARRAY_DEBUG		*psCurrArrayDebug;

	ASSERT(psProg != NULL,
	       "cpPrintProgram: Invalid program pointer");

	debugInfo = (psProg->psDebug != NULL);

	if (debugInfo)
	{
		// Print out the global variables
		if (psProg->numGlobals > 0)
		{
			debug(LOG_NEVER, "index  storage  type variable name\n");
			psCurrVar = psProg->psVarDebug;

			for (i = 0; i < psProg->numGlobals; i++)
			{
				debug(LOG_NEVER, "%-6d %s  %-4d %s\n", (int)(psCurrVar - psProg->psVarDebug), psCurrVar->storage == ST_PUBLIC ? "Public " : "Private", psProg->pGlobals[i], psCurrVar->pIdent);
				psCurrVar++;
			}
		}

		if (psProg->numArrays > 0)
		{
			debug(LOG_NEVER, "\narrays\n");
			psCurrArray = psProg->psArrayInfo;
			psCurrArrayDebug = psProg->psArrayDebug;

			for (i = 0; i < psProg->numArrays; i++)
			{
				debug(LOG_NEVER, "%-6d %s  %-4d %s", i, psCurrArrayDebug->storage == ST_PUBLIC ? "Public " : "Private", psCurrArray->type, psCurrArrayDebug->pIdent);
				for (dim = 0; dim < psCurrArray->dimensions; dim += 1)
				{
					debug(LOG_NEVER, "[%d]", psCurrArray->elements[dim]);
				}
				debug(LOG_NEVER, "\n");
				psCurrArray++;
				psCurrArrayDebug++;
			}
		}

		debug(LOG_NEVER, "\nindex line offset\n");
		psCurrDebug = psProg->psDebug;
	}

	// Find the first trigger with code
	for (jumpOffset = 0; jumpOffset < psProg->numTriggers; jumpOffset++)
	{
		if (psProg->psTriggerData[jumpOffset].type == TR_CODE)
		{
			break;
		}
	}

	ip = psProg->pCode;
	end = (INTERP_VAL *)((UBYTE *)ip + psProg->size);
	triggerCode = (psProg->numTriggers > 0);

	opcode = (OPCODE)(ip->v.ival >> OPCODE_SHIFT);
	data = (ip->v.ival & OPCODE_DATAMASK);

	while (ip < end)
	{
		if (debugInfo)
		{
			// display a label if there is one
			if ((UDWORD)(ip - psProg->pCode) == psCurrDebug->offset &&
			    psCurrDebug->pLabel != NULL)
			{
				debug(LOG_NEVER, "label: %s\n", psCurrDebug->pLabel);
			}
			// Display the line number
			if ((UDWORD)(ip - psProg->pCode) == psCurrDebug->offset)
			{
				debug(LOG_NEVER, "line: %-6d", psCurrDebug->line);
				psCurrDebug++;
			}
		}

		// Display the trigger/event index
		if (triggerCode)
		{
			if (ip - psProg->pCode == psProg->pTriggerTab[jumpOffset])
			{
				debug(LOG_NEVER, "jumpoffset: %-6d", jumpOffset);
				jumpOffset++;

				// Find the next trigger with code
				while (jumpOffset < psProg->numTriggers)
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
				debug(LOG_NEVER, "jumpoffset: %-6d", jumpOffset);
				jumpOffset++;
			}
		}

		// Display the code offset
		debug(LOG_NEVER, "offset: %-6d", (int)(ip - psProg->pCode));
		debug(LOG_NEVER, "opcode: %s", scriptOpcodeToString(opcode));

		switch (opcode)
		{
		case OP_POP:
// 				cpPrintVal(*stackPeek(0));
			break;
		case OP_PUSH:
		case OP_PUSHREF:
			cpPrintPackedVal(ip);
			break;
		case OP_POPGLOBAL:
		case OP_PUSHGLOBAL:
		case OP_POPLOCAL:
		case OP_PUSHLOCAL:
		case OP_PUSHLOCALREF:
			debug(LOG_NEVER, "-> %d", data);
			break;
		case OP_PUSHARRAYGLOBAL:
		case OP_POPARRAYGLOBAL:
			cpPrintArrayInfo(ip, psProg);
			break;
		case OP_FUNC:
			debug(LOG_NEVER, "-> %d", (ip + 1)->v.ival);
			break;
		case OP_CALL:
			debug(LOG_NEVER, "-> %s", scriptFunctionToString((ip + 1)->v.pFuncExtern));
			break;
		case OP_VARCALL:
			cpPrintVarFunc(((INTERP_VAL *)(ip + 1))->v.pObjGetSet, data);
			debug(LOG_NEVER, "-> (%d)", data);
			break;
		case OP_JUMP:
		case OP_JUMPTRUE:
		case OP_JUMPFALSE:
			debug(LOG_NEVER, "-> %d (%d)", (SWORD)data, (int)(ip - psProg->pCode + (SWORD)data));
			break;
		case OP_BINARYOP:
		case OP_UNARYOP:
			debug(LOG_NEVER, "-> %s", scriptOpcodeToString((OPCODE)data));
			break;
		case OP_EXIT:
			break;
		case OP_PAUSE:
			debug(LOG_NEVER, "-> %d", data);
			break;
		case OP_TO_FLOAT:
		case OP_TO_INT:
			break;
		default:
			ASSERT(false, "cpPrintProgram: Unknown opcode: %x", ip->type);
			break;
		}
		debug(LOG_NEVER, "\n");

		ip += aOpSize[opcode];
		ASSERT((ip <= end) || ip != NULL,
		       "cpPrintProgram: instruction pointer no longer valid");

		opcode = (OPCODE)(ip->v.ival >> OPCODE_SHIFT);
		data = ip->v.ival & OPCODE_DATAMASK;
	}
}
