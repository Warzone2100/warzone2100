/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
 * Script.c
 *
 * A few general functions for the script library
 */

#include "lib/framework/frame.h"
#include "script.h"
#include <assert.h>


// Flags for storing function indexes
#define FUNC_SETBIT	0x80000000		// set for a set function, clear otherwise
#define FUNC_LISTMASK	0x70000000
#define FUNC_INDEXMASK	0x0fffffff
#define FUNC_INSTINCT	0x00000000		// instinct function
#define FUNC_CALLBACK	0x40000000		// callback function
#define FUNC_OBJVAR	0x10000000		// object variable
#define FUNC_EXTERNAL	0x20000000		// external variable


// Initialise the script library
bool scriptInitialise()
{
	if (!stackInitialise())
	{
		return false;
	}
	if (!interpInitialise())
	{
		return false;
	}
	if (!eventInitialise())
	{
		return false;
	}

	return true;
}

// Shutdown the script library
void scriptShutDown(void)
{
	eventShutDown();
	stackShutDown();
}


/* Free a SCRIPT_CODE structure */
void scriptFreeCode(SCRIPT_CODE *psCode)
{
	unsigned int i, j;

	debug(LOG_WZ, "Unloading script data");

	/* Free local vars */
	for (i = 0; i < psCode->numEvents; i++)
	{
		if (psCode->numLocalVars[i] > 0) // only free if any defined
		{
			//free strings for event i
			for (j = 0; j < psCode->numLocalVars[i]; j++)
			{
				// When a script fails, it don't allocate storage.
				if (psCode->ppsLocalVarVal)
				{
					if (psCode->ppsLocalVarVal[i][j].type == VAL_STRING)
					{
						free(psCode->ppsLocalVarVal[i][j].v.sval);
					}
				}
			}

			free(psCode->ppsLocalVars[i]);
			if (psCode->ppsLocalVarVal)
			{
				free(psCode->ppsLocalVarVal[i]); // free pointer to event i local vars
			}
		}
	}

	free(psCode->numLocalVars);
	free(psCode->ppsLocalVars);
	free(psCode->ppsLocalVarVal);

	free(psCode->pCode);

	free(psCode->pTriggerTab);
	free(psCode->psTriggerData);

	free(psCode->pEventTab);
	free(psCode->pEventLinks);

	free(psCode->pGlobals);

	free(psCode->psArrayInfo);

	if (psCode->psDebug)
	{
		for (i = 0; i < psCode->debugEntries; i++)
		{
			free(psCode->psDebug[i].pLabel);
		}
		free(psCode->psDebug);
	}

	if (psCode->psVarDebug)
	{
		for (i = 0; i < psCode->numGlobals; i++)
		{
			free(psCode->psVarDebug[i].pIdent);
		}
		free(psCode->psVarDebug);
	}

	if (psCode->psArrayDebug)
	{
		for (i = 0; i < psCode->numArrays; i++)
		{
			free(psCode->psArrayDebug[i].pIdent);
		}
		free(psCode->psArrayDebug);
	}

	free(psCode->numParams);

	free(psCode);
}


/* Lookup a script variable */
bool scriptGetVarIndex(SCRIPT_CODE *psCode, char *pID, UDWORD *pIndex)
{
	UDWORD	index;

	if (!psCode->psVarDebug)
	{
		return false;
	}

	for(index=0; index<psCode->numGlobals; index++)
	{
		if (strcmp(psCode->psVarDebug[index].pIdent, pID)==0)
		{
			*pIndex = index;
			return true;
		}
	}

	return false;
}

/* returns true if passed INTERP_TYPE is used as a pointer in INTERP_VAL, false otherwise.
   all types are listed explicitly, with asserts/warnings for invalid/unrecognised types, as
   getting this wrong will cause segfaults if sizeof(void*) != sizeof(SDWORD) (eg. amd64). a lot of
   these aren't currently checked for, but it's a lot clearer what's going on if they're all here */
bool scriptTypeIsPointer(INTERP_TYPE type)
{
	ASSERT((SCR_USER_TYPES)type < ST_MAXTYPE || type >= VAL_REF, "Invalid type: %d", type);
	// any value or'ed with VAL_REF is a pointer
	if (type >= VAL_REF) return true;
	switch ((unsigned)type)  // Unsigned cast to suppress compiler warnings due to enum abuse.
	{
		case VAL_STRING:
		case VAL_OBJ_GETSET:
		case VAL_FUNC_EXTERN:
		case ST_INTMESSAGE:
		case ST_BASEOBJECT:
		case ST_DROID:
		case ST_STRUCTURE:
		case ST_FEATURE:
		case ST_TEMPLATE:
		case ST_TEXTSTRING:
		case ST_LEVEL:
		case ST_RESEARCH:
		case ST_GROUP:
		case ST_POINTER_O:
		case ST_POINTER_T:
		case ST_POINTER_S:
		case ST_POINTER_STRUCTSTAT:
			return true;
		case VAL_BOOL:
		case VAL_INT:
		case VAL_FLOAT:
		case VAL_TRIGGER:
		case VAL_EVENT:
		case VAL_VOID:
		case VAL_OPCODE:
		case VAL_PKOPCODE:
		case ST_BASESTATS:
		case ST_COMPONENT:
		case ST_BODY:
		case ST_PROPULSION:
		case ST_ECM:
		case ST_SENSOR:
		case ST_CONSTRUCT:
		case ST_WEAPON:
		case ST_REPAIR:
		case ST_BRAIN:
		case ST_STRUCTUREID:
		case ST_STRUCTURESTAT:
		case ST_FEATURESTAT:
		case ST_DROIDID:
		case ST_SOUND:
			return false;
		default:
			ASSERT(false, "scriptTypeIsPointer: unhandled type: %d", type );
			return false;
	}
}


static const struct {
	INTERP_TYPE type;
	const char *name;
} typeToStringMap[] = {
	// Basic types
	{ VAL_BOOL, "bool" },
	{ VAL_INT, "int" },
	{ VAL_FLOAT, "float" },
	{ VAL_STRING, "string" },

	// events and triggers
	{ VAL_TRIGGER, "trigger" },
	{ VAL_EVENT, "event" },

	{ VAL_VOID, "void" },

	{ VAL_OPCODE, "opcode" },
	{ VAL_PKOPCODE, "pkopcode" },

	{ VAL_OBJ_GETSET, "objgs" },
	{ VAL_FUNC_EXTERN, "func" },

	{ VAL_USERTYPESTART, "usertype" },
	{ VAL_REF, "ref" },
};


const char *scriptTypeToString(INTERP_TYPE type)
{
	int i; // Loop goes down -> signed

	// Look whether it is a defaul type:
	for (i = ARRAY_SIZE(typeToStringMap)-1;
		i >= 0 && type <= typeToStringMap[i].type;
		i--)
	{
		if (type >= typeToStringMap[i].type)
			return typeToStringMap[i].name;
	}

	// Look whether it is a user type:
	if (asScrTypeTab)
	{
		unsigned int i;
		for(i = 0; asScrTypeTab[i].typeID != 0; i++)
		{
			if (asScrTypeTab[i].typeID == type)
			{
				return asScrTypeTab[i].pIdent;
			}
		}
	}

	return "unknown";
}


static const struct {
	OPCODE opcode;
	const char *name;
} opcodeToStringMap[] = {
	{ OP_PUSH, "push" },
	{ OP_PUSHREF, "push(ref)" },
	{ OP_POP, "pop" },

	{ OP_PUSHGLOBAL, "push(global)" },
	{ OP_POPGLOBAL, "pop(global)" },

	{ OP_PUSHARRAYGLOBAL, "push(global[])" },
	{ OP_POPARRAYGLOBAL, "push(global[])" },

	{ OP_CALL, "call" },
	{ OP_VARCALL, "vcall" },

	{ OP_JUMP, "jump" },
	{ OP_JUMPTRUE, "jump(true)" },
	{ OP_JUMPFALSE, "jump(false)" },

	{ OP_BINARYOP, "binary" },
	{ OP_UNARYOP, "unary" },

	{ OP_EXIT, "exit" },
	{ OP_PAUSE, "pause" },

	// The following operations are secondary data to OP_BINARYOP and OP_UNARYOP

	// Maths operators
	{ OP_ADD, "+" },
	{ OP_SUB, "-" },
	{ OP_MUL, "*" },
	{ OP_DIV, "/" },
	{ OP_NEG, "(-)" },
	{ OP_INC, "--" },
	{ OP_DEC, "++" },

	// Boolean operators
	{ OP_AND, "&&" },
	{ OP_OR, "||" },
	{ OP_NOT, "!" },

	//String concatenation
	{ OP_CONC, "&" },

	// Comparison operators
	{ OP_EQUAL, "=" },
	{ OP_NOTEQUAL, "!=" },
	{ OP_GREATEREQUAL, ">=" },
	{ OP_LESSEQUAL, "<=" },
	{ OP_GREATER, ">" },
	{ OP_LESS, "<" },

	{ OP_FUNC, "func" },
	{ OP_POPLOCAL, "pop(local)" },
	{ OP_PUSHLOCAL, "push(local)" },

	{ OP_PUSHLOCALREF, "push(localref)" },
	{ OP_TO_FLOAT, "(float)" },
	{ OP_TO_INT, "(int)" },
};


const char *scriptOpcodeToString(OPCODE opcode)
{
	int i; // Loop goes down -> signed

	// Look whether it is a defaul type:
	for (i = ARRAY_SIZE(opcodeToStringMap)-1;
		i >= 0 && opcode <= opcodeToStringMap[i].opcode;
		i--)
	{
		if (opcode >= opcodeToStringMap[i].opcode)
			return opcodeToStringMap[i].name;
	}

	return "unknown";
}


const char *scriptFunctionToString(SCRIPT_FUNC function)
{
	// Search the instinct functions
	if (asScrInstinctTab)
	{
		unsigned int i;
		for(i = 0; asScrInstinctTab[i].pFunc != NULL; i++)
		{
			if (asScrInstinctTab[i].pFunc == function)
			{
				return asScrInstinctTab[i].pIdent;
			}
		}
	}

	// Search the callback functions
	if (asScrCallbackTab)
	{
		unsigned int i;
		for(i = 0; asScrCallbackTab[i].type != 0; i++)
		{
			if (asScrCallbackTab[i].pFunc == function)
			{
				return asScrCallbackTab[i].pIdent;
			}
		}
	}

	return "unknown";
}

