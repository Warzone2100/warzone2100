/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
#include "lib/framework/stdio_ext.h"
#include "lib/framework/file.h"
#include "script.h"
#include <assert.h>

// Lua
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "lib/lua/warzone.h"

#include "src/scriptfuncs.h"
#include "src/scriptai.h"
extern void registerScriptfuncs(lua_State *L);

#include "src/scripttabs.h"

// Flags for storing function indexes
#define FUNC_SETBIT	0x80000000		// set for a set function, clear otherwise
#define FUNC_LISTMASK	0x70000000
#define FUNC_INDEXMASK	0x0fffffff
#define FUNC_INSTINCT	0x00000000		// instinct function
#define FUNC_CALLBACK	0x40000000		// callback function
#define FUNC_OBJVAR	0x10000000		// object variable
#define FUNC_EXTERNAL	0x20000000		// external variable

// FIXME: should be static?
lua_State *lua_states[100];

void registerScript(lua_State *L)
{
	int i;
	for(i=0;i<100;i++)
	{
		if(!lua_states[i])
		{
			lua_states[i] = L;
			return;
		}
	}
}

void freeScript(lua_State *L)
{
	int i;
	for(i=0;i<100;i++)
	{
		if(lua_states[i] == L)
		{
			lua_states[i] = NULL;
			lua_close(L);
			return;
		}
	}
	debug(LOG_ERROR, "script was not registered");
	lua_close(L);
}

static lua_State *scrLoadState(const char *filename)
{
	lua_State *L = luaL_newstate();
	
	debug(LOG_SCRIPT, "loading script %s", filename);
	
	luaL_openlibs(L);
	luaWZ_openlibs(L);
	
	registerScriptfuncs(L);
	registerScriptAIfuncs(L);
	scrRegisterConstants(L);
	scrRegisterCVariables(L);
	
	lua_pushstring(L, filename);
	lua_setglobal(L, "_filename");
	
	luaWZ_dofile(L, "script/save.lua");
	luaWZ_dofile(L, "script/event.lua");
	luaWZ_dofile(L, "script/group.lua");
	luaWZ_dofile(L, "script/glue.lua");
	luaWZ_dofile(L, "script/functions.lua");
	luaWZ_dofile(L, "script/version.lua");
	
	if (luaWZ_dofile(L, filename))
	{
		return L;
	}
	debug(LOG_WARNING, "failed loading script %s", filename);
	return NULL;
}

lua_State *scrNewState(const char *filename)
{
	lua_State *L = scrLoadState(filename);
	
	if (L)
	{
		registerScript(L);
	}
	else
	{
		debug(LOG_WARNING, "failed loading script %s", filename);
	}
	return L;
}

static char *scrSaveSingleState(lua_State *L)
{
	const char *result, *scriptfile;
	char *save;
	lua_getglobal(L, "saveall");
	luaWZ_pcall_backtrace(L, 0, 1);
	result = lua_tostring(L, -1);
	lua_pop(L,1);
	lua_getglobal(L, "_filename");
	scriptfile = lua_tostring(L, -1);
	lua_pop(L,1);
	asprintf(&save, "--[[ name of corresponding script:\n%s\n]]--\n%s", scriptfile, result);
	return save;
}

void scrLoadStates(const char *directory)
{
	int i,j;
	PHYSFS_file* fileHandle;
	char *filename, *p, *start, *stop;
	char text[1024]; // FIXME
	debug(LOG_SCRIPT, "loading script states from %s", directory);
	
	//for(i=0;i<100;i++)
	//{
	//	lua_states[i] = NULL;
	//}
	for(i=0;i<100;i++)
	{
		asprintf(&filename, "%s/state%i.lua", directory, i);
		fileHandle = PHYSFS_openRead(filename);
		if (!fileHandle)
		{
			free(filename);
			continue;
		}
		if (PHYSFS_read(fileHandle, text, 1, 1023) < 1)
		{
			debug(LOG_ERROR, "scrLoadStates: error reading %s", filename);
			free(filename);
			continue;
		}
		text[1023] = 0;
		start = strchr(text, '\n');
		p = strchr(text, '\r');
		if (p && p>start)
		{
			start = p;
		}
		ASSERT(start, "expected a newline within the first 1023 chars");
		start++;
		stop = strchr(start, '\n');
		p = strchr(start, '\r');
		if (p && p<stop)
		{
			stop = p;
		}
		ASSERT(stop, "expected a second newline within the first 1023 chars");
		*stop = 0;
		// now start contains the name of the script this data belongs to
		// let's search for it
		for (j=0;i<100;j++)
		{
			lua_getglobal(lua_states[i], "_filename");
			if (strcmp(start, lua_tostring(lua_states[i], -1)) == 0)
			{
				// found!
				lua_pop(lua_states[i], 1);
				debug(LOG_SCRIPT, "loading state for %s from %s", start, filename);
				luaWZ_dofile(lua_states[i], filename);
				// execute the onload handler which will fix things up
				lua_getglobal(lua_states[i], "onload");
				luaWZ_pcall_backtrace(lua_states[i], 0, 1);
				break;
			}
			lua_pop(lua_states[i], 1);
		}
		if (j == 100)
		{
			debug(LOG_ERROR, "while loading a state from %s: %s not loaded", filename, start);
		}
	}
}

void scrSaveState(const char *directory)
{
	char *buffer;
	char *filename;
	int i;
	
	debug(LOG_SCRIPT, "saving script states to %s", directory);
	
	for(i=0;i<100;i++)
	{
		if(!lua_states[i])
		{
			continue;
		}
		buffer = scrSaveSingleState(lua_states[i]);
		sasprintf(&filename, "%s/state%i.lua", directory, i);
		saveFile(filename, buffer, strlen(buffer));
		free(buffer);
	}
}

// Initialise the script library
BOOL scriptInitialise()
{
	int i;
	
	debug(LOG_SCRIPT, "initialising script system");
	
	for(i=0;i<100;i++)
	{
		lua_states[i] = NULL;
	}
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
#if 0
	UDWORD	i,j;

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
#endif
}


/* Lookup a script variable */
BOOL scriptGetVarIndex(SCRIPT_CODE *psCode, char *pID, UDWORD *pIndex)
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
BOOL scriptTypeIsPointer(INTERP_TYPE type)
{
	ASSERT( ((type < ST_MAXTYPE) || (type >= VAL_REF)), "scriptTypeIsPointer: invalid type: %d", type );
	// any value or'ed with VAL_REF is a pointer
	if (type >= VAL_REF) return true;
	switch (type) {
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

