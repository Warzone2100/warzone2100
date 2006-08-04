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
	BOOL	ref = FALSE;

	if (type & VAL_REF)
	{
		ref = TRUE;
		type = type & ~VAL_REF;
	}

	switch(type)
	{
	case VAL_BOOL:
		DBPRINTF(("BOOL"));
		break;
	case VAL_INT:
		DBPRINTF(("INT"));
		break;
/*	case VAL_FLOAT:
		DBPRINTF(("FLOAT"));
		break;*/
	case VAL_STRING:
		DBPRINTF(("STRING"));
		break;
	case VAL_TRIGGER:
		DBPRINTF(("TRIGGER"));
		break;
	case VAL_EVENT:
		DBPRINTF(("EVENT"));
		break;
	case VAL_VOID:
		DBPRINTF(("VOID"));
		break;
	default:
		// See if it is a user defined type
		if (asScrTypeTab)
		{
			for(i=0; asScrTypeTab[i].typeID != 0; i++)
			{
				if (asScrTypeTab[i].typeID == type)
				{
					DBPRINTF(("%s", asScrTypeTab[i].pIdent ));
					return;
				}
			}
		}
		ASSERT((FALSE, "cpPrintType: Unknown type"));
		break;
	}

	if (ref)
	{
		DBPRINTF((" REF"));
	}
}


/* Display a value  */
void cpPrintVal(INTERP_VAL *psVal)
{
	UDWORD	i;

	if (psVal->type & VAL_REF)
	{
		DBPRINTF(("type: "));
		cpPrintType(psVal->type);
		DBPRINTF((" value: %x", psVal->v.ival));
		return;
	}

	switch(psVal->type)
	{
	case VAL_BOOL:
		DBPRINTF(("type: BOOL    value: %s",
					psVal->v.bval ? "true" : "false"));
		break;
	case VAL_INT:
		DBPRINTF(("type: INT     value: %d", psVal->v.ival ));
		break;
/*	case VAL_FLOAT:
		DBPRINTF(("type: FLOAT   value: %f", psVal->v.fval ));
		break;*/
	case VAL_STRING:
		DBPRINTF(("type: STRING  value: %s", psVal->v.sval ));
		break;
	case VAL_TRIGGER:
		DBPRINTF(("type: TRIGGER value: %d", psVal->v.ival ));
		break;
	case VAL_EVENT:
		DBPRINTF(("type: EVENT   value: %d", psVal->v.ival ));
		break;
	default:
		// See if it is a user defined type
		if (asScrTypeTab)
		{
			for(i=0; asScrTypeTab[i].typeID != 0; i++)
			{
				if (asScrTypeTab[i].typeID == psVal->type)
				{
					DBPRINTF(("type: %s value: %x",
							asScrTypeTab[i].pIdent, psVal->v.ival ));
					return;
				}
			}
		}
		ASSERT((FALSE, "cpPrintVal: Unknown value type"));
		break;
	}
}


/* Display a value from a program that has been packed with an opcode */
void cpPrintPackedVal(UDWORD *ip)
{
	INTERP_TYPE	type = (*ip) & OPCODE_DATAMASK;
	UDWORD		i;
	UDWORD data = *(ip + 1);

	if (type & VAL_REF)
	{
		DBPRINTF(("type: "));
		cpPrintType(type);
		DBPRINTF((" value: %x", data));
		return;
	}

	switch(type)
	{
	case VAL_BOOL:
		DBPRINTF(("BOOL   : %s",
					((BOOL)data) ? "true" : "false"));
		break;
	case VAL_INT:
		DBPRINTF(("INT     : %d", (SDWORD)data ));
		break;
/*	case VAL_FLOAT:
		DBPRINTF(("FLOAT   : %f", (float)data ));
		break;*/
	case VAL_STRING:
		DBPRINTF(("STRING  : %s", (STRING *)data ));
		break;
	case VAL_TRIGGER:
		DBPRINTF(("TRIGGER : %d", (SDWORD)data ));
		break;
	case VAL_EVENT:
		DBPRINTF(("EVENT   : %d", (SDWORD)data ));
		break;
	default:
		// See if it is a user defined type
		if (asScrTypeTab)
		{
			for(i=0; asScrTypeTab[i].typeID != 0; i++)
			{
				if (asScrTypeTab[i].typeID == type)
				{
					DBPRINTF(("type: %s value: %x",
							asScrTypeTab[i].pIdent, data ));
					return;
				}
			}
		}
		ASSERT((FALSE, "cpPrintVal: Unknown value type"));
		break;
	}
}


/* Display a maths operator */
void cpPrintMathsOp(UDWORD opcode)
{
	switch (opcode)
	{
	case OP_ADD:
		DBPRINTF(("ADD         "));
		break;
	case OP_SUB:
		DBPRINTF(("SUB         "));
		break;
	case OP_MUL:
		DBPRINTF(("MUL         "));
		break;
	case OP_DIV:
		DBPRINTF(("DIV         "));
		break;
	case OP_NEG:
		DBPRINTF(("NEG         "));
		break;
	case OP_AND:
		DBPRINTF(("AND         "));
		break;
	case OP_OR:
		DBPRINTF(("OR          "));
		break;
	case OP_NOT:
		DBPRINTF(("NOT         "));
		break;
	case OP_EQUAL:
		DBPRINTF(("EQUAL       "));
		break;
	case OP_NOTEQUAL:
		DBPRINTF(("NOT_EQUAL   "));
		break;
	case OP_GREATEREQUAL:
		DBPRINTF(("GRT_EQUAL   "));
		break;
	case OP_LESSEQUAL:
		DBPRINTF(("LESS_EQUAL  "));
		break;
	case OP_GREATER:
		DBPRINTF(("GREATER     "));
		break;
	case OP_LESS:
		DBPRINTF(("LESS        "));
		break;
	default:
		ASSERT((FALSE, "cpPrintMathsOp: unknown operator"));
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
				DBPRINTF(("%s", asScrInstinctTab[i].pIdent));
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
				DBPRINTF(("%s", asScrCallbackTab[i].pIdent));
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
				DBPRINTF(("%s (set)", asScrExternalTab[i].pIdent));
				return;
			}
			else if (asScrExternalTab[i].get == pFunc &&
					 asScrExternalTab[i].index == index)
			{
				DBPRINTF(("%s (get)", asScrExternalTab[i].pIdent));
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
				DBPRINTF(("%s (set)", asScrObjectVarTab[i].pIdent));
				return;
			}
			else if (asScrObjectVarTab[i].get == pFunc &&
					 asScrObjectVarTab[i].index == index)
			{
				DBPRINTF(("%s (get)", asScrObjectVarTab[i].pIdent));
				return;
			}
		}
	}
}


/* Print the array information */
void cpPrintArrayInfo(UDWORD **pip, SCRIPT_CODE *psProg)
{
	SDWORD		i, dimensions;//, elements[VAR_MAX_DIMENSIONS];
//	SDWORD		elementDWords;
//	UBYTE		*pElem;
	UDWORD		*ip = *pip;
	UDWORD		base;

	// get the base index of the array
	base = (*ip) & ARRAY_BASE_MASK;

	// get the number of dimensions
	dimensions = ((*ip) & ARRAY_DIMENSION_MASK) >> ARRAY_DIMENSION_SHIFT;

	// get the number of elements for each dimension
/*	pElem = (UBYTE *) (ip + 1);
	for(i=0; i<dimensions; i+=1)
	{
		elements[i] = *pElem;

		pElem += 1;
	}*/

	DBPRINTF(("%d->", base));
	for(i=0; i<psProg->psArrayInfo[base].dimensions; i+= 1)
	{
		DBPRINTF(("[%d]", psProg->psArrayInfo[base].elements[i]));
	}
	// calculate the number of DWORDs needed to store the number of elements for each dimension of the array
//	elementDWords = (dimensions - 1)/4 + 1;

	// update the insrtuction pointer
	*pip += 1;// + elementDWords;
}


/* Display the contents of a program in readable form */
void cpPrintProgram(SCRIPT_CODE *psProg)
{
	UDWORD			*ip, *end;
	OPCODE			opcode;
	UDWORD			data, i, dim;
	SCRIPT_DEBUG	*psCurrDebug=NULL;
	BOOL			debugInfo, triggerCode;
	UDWORD			jumpOffset;
	VAR_DEBUG		*psCurrVar;
	ARRAY_DATA		*psCurrArray;
	ARRAY_DEBUG		*psCurrArrayDebug;

	ASSERT((PTRVALID(psProg, sizeof(SCRIPT_CODE)),
		"cpPrintProgram: Invalid program pointer"));

	debugInfo = psProg->psDebug != NULL;

	if (debugInfo)
	{
		// Print out the global variables
		if (psProg->numGlobals > 0)
		{
			DBPRINTF(("index  storage  type variable name\n"));
			psCurrVar = psProg->psVarDebug;
			for(i=0; i<psProg->numGlobals; i++)
			{
				DBPRINTF(("%-6d %s  %-4d %s\n", psCurrVar - psProg->psVarDebug,
					psCurrVar->storage == ST_PUBLIC ? "Public " : "Private",
					psProg->pGlobals[i], psCurrVar->pIdent));
				psCurrVar++;
			}
		}

		if (psProg->numArrays > 0)
		{
			DBPRINTF(("\narrays\n"));
			psCurrArray = psProg->psArrayInfo;
			psCurrArrayDebug = psProg->psArrayDebug;
			for(i=0; i<psProg->numArrays; i++)
			{
				DBPRINTF(("%-6d %s  %-4d %s", i,
					psCurrArrayDebug->storage == ST_PUBLIC ? "Public " : "Private",
					psCurrArray->type, psCurrArrayDebug->pIdent));
				for(dim=0; dim < psCurrArray->dimensions; dim += 1)
				{
					DBPRINTF(("[%d]", psCurrArray->elements[dim]));
				}
				DBPRINTF(("\n"));
				psCurrArray++;
				psCurrArrayDebug++;
			}
		}

		DBPRINTF(("\nindex  line   offset\n"));
		psCurrDebug = psProg->psDebug;
	}
	else
	{
		DBPRINTF(("index         offset\n"));
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
	triggerCode = psProg->numTriggers > 0 ? TRUE : FALSE;
	end = (UDWORD *)(((UBYTE *)ip) + psProg->size);
	opcode = (*ip) >> OPCODE_SHIFT;
	data = (*ip) & OPCODE_DATAMASK;
	while (ip < end)
	{
		// display a label if there is one
		if (debugInfo)
		{
			if ((UDWORD)(ip - psProg->pCode) == psCurrDebug->offset &&
				psCurrDebug->pLabel != NULL)
			{
				DBPRINTF(("%s\n", psCurrDebug->pLabel));
			}
		}

		// Display the trigger/event index
		if (triggerCode)
		{
			if (ip - psProg->pCode == psProg->pTriggerTab[jumpOffset])
			{
				DBPRINTF(("%-6d ", jumpOffset));
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
					triggerCode = FALSE;
					jumpOffset = 0;
				}
			}
			else
			{
				DBPRINTF(("       "));
			}
		}
		else
		{
			if (ip - psProg->pCode == psProg->pEventTab[jumpOffset])
			{
				DBPRINTF(("%-6d ", jumpOffset));
				jumpOffset+= 1;
			}
			else
			{
				DBPRINTF(("       "));
			}
		}

		// Display the line number
		if (debugInfo)
		{
			if ((UDWORD)(ip - psProg->pCode) == psCurrDebug->offset)
			{
				DBPRINTF(("%-6d ", psCurrDebug->line));
				psCurrDebug++;
			}
			else
			{
				DBPRINTF(("       "));
			}
		}

		// Display the code offset
		DBPRINTF(("%-6d  ", ip - psProg->pCode));
		switch (opcode)
		{
		case OP_PUSH:
			DBPRINTF(("PUSH        "));
			cpPrintPackedVal(ip);
			DBPRINTF(("\n"));
			ip += aOpSize[opcode];
			break;
		case OP_PUSHREF:
			DBPRINTF(("PUSHREF     "));
			cpPrintPackedVal(ip);
			DBPRINTF(("\n"));
			ip += aOpSize[opcode];
			break;
		case OP_POP:
			DBPRINTF(("POP\n"));
			ip += aOpSize[opcode];
			break;
		case OP_PUSHGLOBAL:
			DBPRINTF(("PUSHGLOBAL  %d\n", data));
			ip += aOpSize[opcode];
			break;
		case OP_POPGLOBAL:
			DBPRINTF(("POPGLOBAL   %d\n", data));
			ip += aOpSize[opcode];
			break;
		case OP_PUSHARRAYGLOBAL:
			DBPRINTF(("PUSHARRAYGLOBAL  "));
			cpPrintArrayInfo(&ip, psProg);
			DBPRINTF(("\n"));
			break;
		case OP_POPARRAYGLOBAL:
			DBPRINTF(("POPARRAYGLOBAL   "));
			cpPrintArrayInfo(&ip, psProg);
			DBPRINTF(("\n"));
			break;
		case OP_CALL:
			DBPRINTF(("CALL        "));
			cpPrintFunc( (SCRIPT_FUNC)(*(ip+1)) );
			DBPRINTF(("\n"));
			ip += aOpSize[opcode];
			break;
		case OP_VARCALL:
			DBPRINTF(("VARCALL     "));
			cpPrintVarFunc( (SCRIPT_VARFUNC)(*(ip+1)), data);
			DBPRINTF(("(%d)\n", data));
			ip += aOpSize[opcode];
			break;
		case OP_JUMP:
			DBPRINTF(("JUMP        %d (%d)\n",
					(SWORD)data, ip - psProg->pCode + (SWORD)data));
			ip += aOpSize[opcode];
			break;
		case OP_JUMPTRUE:
			DBPRINTF(("JUMPTRUE    %d (%d)\n",
					(SWORD)data, ip - psProg->pCode + (SWORD)data));
			ip += aOpSize[opcode];
			break;
		case OP_JUMPFALSE:
			DBPRINTF(("JUMPFALSE   %d (%d)\n",
					(SWORD)data, ip - psProg->pCode + (SWORD)data));
			ip += aOpSize[opcode];
			break;
		case OP_BINARYOP:
		case OP_UNARYOP:
			cpPrintMathsOp(data);
			DBPRINTF(("\n"));
			ip += aOpSize[opcode];
			break;
		case OP_EXIT:
			DBPRINTF(("EXIT\n"));
			ip += aOpSize[opcode];
			break;
		case OP_PAUSE:
			DBPRINTF(("PAUSE       %d\n", data));
			ip += aOpSize[opcode];
			break;
		default:
			ASSERT((FALSE,"cpPrintProgram: Unknown opcode: %x", *ip));
			break;
		}

		ASSERT(((ip <= end) || PTRVALID(ip, sizeof(UDWORD)),
			"cpPrintProgram: instruction pointer no longer valid"));

		opcode = (*ip) >> OPCODE_SHIFT;
		data = (*ip) & OPCODE_DATAMASK;
	}
}
