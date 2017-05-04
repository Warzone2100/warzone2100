/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
 * Script.h
 *
 * Interface to the script library
 */
#ifndef _script_h
#define _script_h

#include "interpreter.h"
#include "stack.h"
#include "codeprint.h"
#include "parse.h"
#include "event.h"
#include "eventsave.h"

/* Whether to include debug info when compiling */
enum SCR_DEBUGTYPE
{
	SCR_DEBUGINFO,		// Generate debug info
	SCR_NODEBUG,		// Do not generate debug info
};

// If this is defined we save out the compiled scripts
#define SCRIPTTYPE SCR_DEBUGINFO

// Initialise the script library
extern bool scriptInitialise();

// Shutdown the script library
extern void scriptShutDown();

/***********************************************************************************
 *
 * Compiler setup functions
 */

/* Set the type table */
extern void scriptSetTypeTab(TYPE_SYMBOL *psTypeTab);

/* Set the function table */
extern void scriptSetFuncTab(FUNC_SYMBOL *psFuncTab);

/* Set the external variable table */
extern void scriptSetExternalTab(VAR_SYMBOL *psExtTab);

/* Set the object variable table */
extern void scriptSetObjectTab(VAR_SYMBOL *psObjTab);

/* Set the constant table */
extern void scriptSetConstTab(CONST_SYMBOL *psConstTab);

/* Set the callback table */
extern void scriptSetCallbackTab(CALLBACK_SYMBOL *psCallTab);

/* Set the type equivalence table */
extern void scriptSetTypeEquiv(TYPE_EQUIV *psTypeTab);


/***********************************************************************************
 *
 * Return stack stuff
 */

/**
 * Current call depth
 *
 * \return Number of calls on the return address stack
 */
extern UDWORD retStackCallDepth();


/***********************************************************************************
 *
 * Compiler functions
 */

/** Compile a script program
 *  \param fileHandle a PhysicsFS file handle to the script-file to compile
 *  \param debugType whether debugging information should be generated or not.
 *                   Select \c SCR_DEBUGINFO to generate debugging information,
 *                   choose any other parameter if you don't want debugging
 *                   information attached.
 *  \return a valid, non-null pointer on success,
 *          otherwise a null pointer on failure.
 */
extern SCRIPT_CODE *scriptCompile(PHYSFS_file *fileHandle, SCR_DEBUGTYPE debugType);

/* Free a SCRIPT_CODE structure */
extern void scriptFreeCode(SCRIPT_CODE *psCode);

/* Lookup a script variable */
extern bool scriptGetVarIndex(SCRIPT_CODE *psCode, char *pID, UDWORD *pIndex);

/* returns true if passed INTERP_TYPE is used as a pointer in INTERP_VAL, false otherwise */
extern bool scriptTypeIsPointer(INTERP_TYPE type);

extern const char *scriptTypeToString(INTERP_TYPE type) WZ_DECL_PURE;
extern const char *scriptOpcodeToString(OPCODE opcode) WZ_DECL_PURE;
extern const char *scriptFunctionToString(SCRIPT_FUNC function) WZ_DECL_PURE;

/* Run a compiled script */
extern bool interpRunScript(SCRIPT_CONTEXT *psContext, INTERP_RUNTYPE runType,
                            UDWORD index, UDWORD offset);


/***********************************************************************************
 *
 * Event system functions
 */

// reset the event system
extern void eventReset();

// Initialise the create/release function array - specify the maximum value type
extern bool eventInitValueFuncs(SDWORD maxType);

// a create function for data stored in an INTERP_VAL
typedef bool (*VAL_CREATE_FUNC)(INTERP_VAL *psVal);

// a release function for data stored in an INTERP_VAL
typedef void (*VAL_RELEASE_FUNC)(INTERP_VAL *psVal);

// Add a new value create function
extern bool eventAddValueCreate(INTERP_TYPE type, VAL_CREATE_FUNC create);

// Add a new value release function
extern bool eventAddValueRelease(INTERP_TYPE type, VAL_RELEASE_FUNC release);

// Create a new context for a script
extern bool eventNewContext(SCRIPT_CODE *psCode, CONTEXT_RELEASE release, SCRIPT_CONTEXT **ppsContext);

// Add a new object to the trigger system
// Time is the application time at which all the triggers are to be started
extern bool eventRunContext(SCRIPT_CONTEXT *psContext, UDWORD time);

// Remove a context from the event system
extern void eventRemoveContext(SCRIPT_CONTEXT *psContext);

// Set a global variable value for a context
extern bool eventSetContextVar(SCRIPT_CONTEXT *psContext, UDWORD index, INTERP_VAL *data);

// Get the value pointer for a variable index
extern bool eventGetContextVal(SCRIPT_CONTEXT *psContext, UDWORD index, INTERP_VAL **ppsVal);

// Process all the currently active triggers
// Time is the application time at which all the triggers are to be processed
extern void eventProcessTriggers(UDWORD currTime);

// Activate a callback trigger
extern void eventFireCallbackTrigger(TRIGGER_TYPE callback);

/***********************************************************************************
 *
 * Support functions for writing instinct functions
 */

/* Pop a number of values off the stack checking their types
 * This is used by instinct functions to get their parameters
 * The varargs part is a set of INTERP_TYPE, UDWORD * pairs.
 * The value of the parameter is stored in the DWORD pointed to by the UDWORD *
 */
extern bool stackPopParams(unsigned int numParams, ...);

/* Push a value onto the stack without using a value structure */
extern bool stackPushResult(INTERP_TYPE type, INTERP_VAL *result);

/***********************************************************************************
 *
 * Script library instinct functions
 *
 * These would be declared in the function symbol array:
 *
 *	{ "traceOn",				interpTraceOn,			VAL_VOID,
 *		0, { VAL_VOID } },
 *	{ "traceOff",				interpTraceOff,			VAL_VOID,
 *		0, { VAL_VOID } },
 *	{ "setEventTrigger",		eventSetTrigger,		VAL_VOID,
 *		2, { VAL_EVENT, VAL_TRIGGER } },
 *	{ "eventTraceLevel",		eventSetTraceLevel,		VAL_VOID,
 *		1, { VAL_INT } },
 *
 */

/* Instinct function to turn on tracing */
extern bool interpTraceOn();

/* Instinct function to turn off tracing */
extern bool interpTraceOff();

// Change the trigger assigned to an event
// This is an instinct function that takes a VAL_EVENT and VAL_TRIGGER as parameters
extern bool eventSetTrigger();

// set the event tracing level
//   0 - no tracing
//   1 - only fired triggers
//   2 - added and fired triggers
//   3 - as 2 but show tested but not fired triggers as well
extern bool eventSetTraceLevel();

#endif
