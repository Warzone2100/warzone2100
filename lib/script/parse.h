/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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
 * Parse.h
 *
 * Definitions for the script parser
 */
#ifndef _parse_h
#define _parse_h

#include "lib/framework/frame.h"
#include <physfs.h>

class WzConfig;

#include "interpreter.h"

#ifndef MAXSTRLEN
#define MAXSTRLEN 255
#endif

/* Maximum number of TEXT items in any one Yacc rule */
#define TEXT_BUFFERS 10

#ifdef DEBUG_SCRIPT
#define RULE(...) script_debug(__VA_ARGS__);
#else
#define RULE(...)
#endif

/* Script includes */
#define MAX_SCR_INCLUDE_DEPTH	10

/* Script defines */
#define MAX_SCR_MACRO_DEPTH		10
#define MAX_SCR_MACROS			150		/* Max defines in a file */
#define MAX_SCR_MACRO_LEN		32

/* Structure to hold script define directive information */
struct SCR_MACRO
{
	char scr_define_macro[MAX_SCR_MACRO_LEN];
	char scr_define_body[MAXSTRLEN];
};

/* Definition for the chunks of code that are used within the compiler */
struct CODE_BLOCK
{
	UDWORD				size;				// size of the code block
	INTERP_VAL			*pCode;		// pointer to the code data
	UDWORD				debugEntries;
	SCRIPT_DEBUG		*psDebug;	// Debugging info for the script.
	INTERP_TYPE			type;			// The type of the code block
};

/* The chunk of code returned from parsing a parameter list. */
struct PARAM_BLOCK
{
	UDWORD		numParams;
	INTERP_TYPE	*aParams;	// List of parameter types
	UDWORD		size;
	INTERP_VAL	*pCode;		// The code that puts the parameters onto the stack
};

/* The types of a functions parameters, returned from parsing a parameter declaration */
struct PARAM_DECL
{
	UDWORD		numParams;
	INTERP_TYPE	*aParams;
};

/* The chunk of code used while parsing a conditional statement */
struct COND_BLOCK
{
	UDWORD		numOffsets;
	UDWORD		*aOffsets;	// Positions in the code that have to be
	// replaced with the offset to the end of the
	// conditional statement (for the jumps). - //TODO: change to INTERP_VAL - probably not necessary
	UDWORD		size;
	INTERP_VAL		*pCode;
	UDWORD			debugEntries;	// Number of debugging entries in psDebug.
	SCRIPT_DEBUG	*psDebug;		// Debugging info for the script.
};

/* The possible access types for a type */
enum ACCESS_TYPE
{
	AT_SIMPLE,			// The type represents a simple data value
	AT_OBJECT,			// The type represents an object
};


// function pointer for script variable saving
// if pBuffer is NULL the script system is just asking how much space the saved variable will require
// otherwise pBuffer points to an array to store the value in
typedef bool (*SCR_VAL_SAVE)(INTERP_VAL *psVal, WzConfig &ini);
// function pointer for script variable loading
typedef bool (*SCR_VAL_LOAD)(INTERP_VAL *psVal, WzConfig &ini);

/* Type for a user type symbol */
struct TYPE_SYMBOL
{
	const char		*pIdent;	// Type identifier
	INTERP_TYPE		typeID;		// The type id to use in the type field of values
	SWORD			accessType;	// Whether the type is an object or a simple value
	SCR_VAL_SAVE	saveFunc;	// load and save functions
	SCR_VAL_LOAD	loadFunc;	//
};

/* Type for a variable identifier declaration */
struct VAR_IDENT_DECL
{
	char			*pIdent;						// variable identifier
	SDWORD			dimensions;						// number of dimensions of an array - 0 for normal var
	SDWORD			elements[VAR_MAX_DIMENSIONS];	// number of elements in an array
};

/* Type for a variable symbol */
struct VAR_SYMBOL
{
	const char	*pIdent;	// variable's identifier
	INTERP_TYPE		type;		// variable type
	STORAGE_TYPE	storage;	// Where the variable is stored
	uint32_t /*INTERP_TYPE*/ objType;  // The object type if this is an object variable
	UDWORD			index;		// Index of the variable in its data space
	SCRIPT_VARFUNC	get, set;	// Access functions if the variable is stored in an object/in-game
	UDWORD			dimensions;						// number of dimensions of an array - 0 for normal var
	SDWORD			elements[VAR_MAX_DIMENSIONS];	// number of elements in an array

	VAR_SYMBOL     *psNext;
};


/* Type for an array access block */
struct ARRAY_BLOCK
{
	VAR_SYMBOL		*psArrayVar;
	UDWORD			dimensions;

	UDWORD			size;
	INTERP_VAL			*pCode;
	UDWORD			debugEntries;	// Number of debugging entries in psDebug.
	SCRIPT_DEBUG	*psDebug;		// Debugging info for the script.
};

/* Type for a constant symbol */
struct CONST_SYMBOL
{
	const char		*pIdent;	// variable's identifier
	INTERP_TYPE		type;		// variable type

	/* The actual value of the constant.
	 * Only one of these will be valid depending on type.
	 * A union is not used as a union cannot be statically initialised
	 */
	int32_t			bval;
	SDWORD			ival;
	void			*oval;
	char			*sval;	//String values
	float			fval;
};

/* The chunk of code used to reference an object variable */
struct OBJVAR_BLOCK
{
	VAR_SYMBOL		*psObjVar;	// The object variables symbol

	UDWORD			size;
	INTERP_VAL		*pCode;		// The code to get the object value on the stack
};

/* The maximum number of parameters for an instinct function */
#define INST_MAXPARAMS		20

/* Type for a function symbol */
struct FUNC_SYMBOL
{
	const char	*pIdent;	// function's identifier
	SCRIPT_FUNC	pFunc;		// Pointer to the instinct function
	INTERP_TYPE	type;		// function type
	UDWORD		numParams;	// Number of parameters to the function
	uint32_t/*INTERP_TYPE*/ aParams[INST_MAXPARAMS];
	// List of parameter types
	int32_t		script;		// Whether the function is defined in the script
	// or a C instinct function
	UDWORD		size;		// The size of script code
	INTERP_VAL	*pCode;		// The code for a function if it is defined in the script
	UDWORD		location;	// The position of the function in the final code block
	UDWORD			debugEntries;	// Number of debugging entries in psDebug.
	SCRIPT_DEBUG	*psDebug;		// Debugging info for the script.

	FUNC_SYMBOL    *psNext;
};

/* The type for a variable declaration */
struct VAR_DECL
{
	INTERP_TYPE		type;
	STORAGE_TYPE	storage;
};

/* The type for a trigger sub declaration */
struct TRIGGER_DECL
{
	TRIGGER_TYPE	type;
	UDWORD			size;
	INTERP_VAL		*pCode;
	UDWORD			time;
};

/* Type for a trigger symbol */
struct TRIGGER_SYMBOL
{
	char			*pIdent;	// Trigger's identifier
	UDWORD			index;		// The triggers index number
	TRIGGER_TYPE	type;		// Trigger type
	UDWORD			size;		// Code size for the trigger
	INTERP_VAL		*pCode;		// The trigger code
	UDWORD			time;		// How often to check the trigger

	UDWORD			debugEntries;
	SCRIPT_DEBUG	*psDebug;

	TRIGGER_SYMBOL *psNext;
};

/* The type for a callback trigger symbol */
struct CALLBACK_SYMBOL
{
	const char		*pIdent;	// Callback identifier
	TRIGGER_TYPE	type;		// user defined callback id >= TR_CALLBACKSTART
	SCRIPT_FUNC		pFunc;		// Pointer to the instinct function
	UDWORD			numParams;	// Number of parameters to the function
	uint32_t/*INTERP_TYPE*/ aParams[INST_MAXPARAMS];
	// List of parameter types
};


/* Type for an event symbol */
struct EVENT_SYMBOL
{
	char		*pIdent;	// Event's identifier
	UDWORD		index;		// the events index number
	UDWORD		size;		// Code size for the event
	INTERP_VAL		*pCode;		// Event code
	SDWORD		trigger;	// Index of the event's trigger

	UDWORD			debugEntries;
	SCRIPT_DEBUG	*psDebug;

	//functions stuff
	UDWORD		numParams;		//Number of parameters to the function
	UDWORD		numLocalVars;	//local variables
	int32_t		bFunction;		//if this event is defined as a function
	int32_t		bDeclared;		//if function was declared before
	INTERP_TYPE	retType;		//return type if a function

	INTERP_TYPE	aParams[INST_MAXPARAMS];

	EVENT_SYMBOL   *psNext;
};

/* The table of user types */
extern TYPE_SYMBOL		*asScrTypeTab;

/* The table of external variables and their access functions */
extern VAR_SYMBOL		*asScrExternalTab;

/* The table of object variable access routines */
extern VAR_SYMBOL		*asScrObjectVarTab;

/* The table of instinct function type definitions */
extern FUNC_SYMBOL		*asScrInstinctTab;

/* The table of constant variables */
extern CONST_SYMBOL		*asScrConstantTab;

/* The table of callback triggers */
extern CALLBACK_SYMBOL	*asScrCallbackTab;

/* Set the current input file for the lexer */
extern void scriptSetInputFile(PHYSFS_file *fileHandle);

/* Initialise the parser ready for a new script */
extern bool scriptInitParser();

/* Set off the scenario file parser */
extern int scr_parse();

/* Give an error message */
#ifndef DEBUG
[[ noreturn ]]
#endif
void scr_error(const char *pMessage, ...);

extern void scriptGetErrorData(int *pLine, char **ppText);

/* Look up a type symbol */
extern bool scriptLookUpType(const char *pIdent, INTERP_TYPE *pType);

/* Add a new variable symbol */
extern bool scriptAddVariable(VAR_DECL *psStorage, VAR_IDENT_DECL *psVarIdent);

/* Add a new trigger symbol */
extern bool scriptAddTrigger(const char *pIdent, TRIGGER_DECL *psDecl, UDWORD line);

/* Add a new event symbol */
extern bool scriptDeclareEvent(const char *pIdent, EVENT_SYMBOL **ppsEvent, SDWORD numArgs);

// Add the code to a defined event
extern bool scriptDefineEvent(EVENT_SYMBOL *psEvent, CODE_BLOCK *psCode, SDWORD trigger);

/* Look up a variable symbol */
extern bool scriptLookUpVariable(const char *pIdent, VAR_SYMBOL **ppsSym);

/* Look up a constant variable symbol */
extern bool scriptLookUpConstant(const char *pIdent, CONST_SYMBOL **ppsSym);

/* Lookup a trigger symbol */
extern bool scriptLookUpTrigger(const char *pIdent, TRIGGER_SYMBOL **ppsTrigger);

/* Lookup a callback trigger symbol */
extern bool scriptLookUpCallback(const char *pIdent, CALLBACK_SYMBOL **ppsCallback);

/* Lookup an event symbol */
extern bool scriptLookUpEvent(const char *pIdent, EVENT_SYMBOL **ppsEvent);

/* Add a new function symbol */
extern bool scriptStartFunctionDef(const char *pIdent,	// Functions name
                                   INTERP_TYPE		type);		// return type

/* Store the parameter types for the current script function definition  */
extern bool scriptSetParameters(UDWORD		numParams,	// number of parameters
                                INTERP_TYPE		*pParams);	// parameter types

/* Store the code for a script function definition.
 * Clean up the local variable list for this function definition.
 */
extern bool scriptSetCode(CODE_BLOCK  *psBlock);	// The code block

/* Look up a function symbol */
extern bool scriptLookUpFunction(const char *pIdent, FUNC_SYMBOL **ppsSym);

/* Look up an in-script custom function symbol */
extern bool scriptLookUpCustomFunction(const char *pIdent, EVENT_SYMBOL **ppsSym);

extern bool popArguments(INTERP_VAL **ip_temp, SDWORD numParams);

extern void widgCopyString(char *pDest, const char *pSrc); // FIXME Duplicate declaration of internal widget function

extern void script_debug(const char *pFormat, ...);

#endif
