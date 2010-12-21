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
 * Parse.h
 *
 * Definitions for the script parser
 */
#ifndef _parse_h
#define _parse_h

#include <physfs.h>

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
typedef struct _scr_define
{
	char scr_define_macro[MAX_SCR_MACRO_LEN];
	char scr_define_body[MAXSTRLEN];
}SCR_MACRO;

/* Definition for the chunks of code that are used within the compiler */
typedef struct _code_block
{
	UDWORD				size;				// size of the code block
	INTERP_VAL			*pCode;		// pointer to the code data
	UDWORD				debugEntries;
	SCRIPT_DEBUG		*psDebug;	// Debugging info for the script.
	INTERP_TYPE			type;			// The type of the code block
} CODE_BLOCK;

/* The chunk of code returned from parsing a parameter list. */
typedef struct _param_block
{
	UDWORD		numParams;
	INTERP_TYPE	*aParams;	// List of parameter types
	UDWORD		size;
	INTERP_VAL	*pCode;		// The code that puts the parameters onto the stack
}PARAM_BLOCK;

/* The types of a functions parameters, returned from parsing a parameter declaration */
typedef struct _param_decl
{
	UDWORD		numParams;
	INTERP_TYPE	*aParams;
} PARAM_DECL;

/* The chunk of code used while parsing a conditional statement */
typedef struct _cond_block
{
	UDWORD		numOffsets;
	UDWORD		*aOffsets;	// Positions in the code that have to be
							// replaced with the offset to the end of the
							// conditional statement (for the jumps). - //TODO: change to INTERP_VAL - probbaly not necessary
	UDWORD		size;
	INTERP_VAL		*pCode;
	UDWORD			debugEntries;	// Number of debugging entries in psDebug.
	SCRIPT_DEBUG	*psDebug;		// Debugging info for the script.
} COND_BLOCK;

/* The possible access types for a type */
typedef enum _access_type
{
	AT_SIMPLE,			// The type represents a simple data value
	AT_OBJECT,			// The type represents an object
} ACCESS_TYPE;


// function pointer for script variable saving
// if pBuffer is NULL the script system is just asking how much space the saved variable will require
// otherwise pBuffer points to an array to store the value in
typedef BOOL (*SCR_VAL_SAVE)(INTERP_VAL *psVal, char *pBuffer, UDWORD *pSize);
// function pointer for script variable loading
typedef BOOL (*SCR_VAL_LOAD)(SDWORD version, INTERP_VAL *psVal, char *pBuffer, UDWORD size);

/* Type for a user type symbol */
typedef struct _type_symbol
{
	const char		*pIdent;	// Type identifier
	INTERP_TYPE		typeID;		// The type id to use in the type field of values
	SWORD			accessType;	// Whether the type is an object or a simple value
	SCR_VAL_SAVE	saveFunc;	// load and save functions
	SCR_VAL_LOAD	loadFunc;	//
} TYPE_SYMBOL;

/* Type for a variable identifier declaration */
typedef struct _var_ident_decl
{
	char			*pIdent;						// variable identifier
	SDWORD			dimensions;						// number of dimensions of an array - 0 for normal var
	SDWORD			elements[VAR_MAX_DIMENSIONS];	// number of elements in an array
} VAR_IDENT_DECL;

/* Type for a variable symbol */
typedef struct _var_symbol
{
	const char	*pIdent;	// variable's identifier
	INTERP_TYPE		type;		// variable type
	STORAGE_TYPE	storage;	// Where the variable is stored
	uint32_t /*INTERP_TYPE*/ objType;  // The object type if this is an object variable
	UDWORD			index;		// Index of the variable in its data space
	SCRIPT_VARFUNC	get, set;	// Access functions if the variable is stored in an object/in-game
	UDWORD			dimensions;						// number of dimensions of an array - 0 for normal var
	SDWORD			elements[VAR_MAX_DIMENSIONS];	// number of elements in an array

	struct _var_symbol *psNext;
} VAR_SYMBOL;


/* Type for an array access block */
typedef struct _array_block
{
	VAR_SYMBOL		*psArrayVar;
	UDWORD			dimensions;

	UDWORD			size;
	INTERP_VAL			*pCode;
	UDWORD			debugEntries;	// Number of debugging entries in psDebug.
	SCRIPT_DEBUG	*psDebug;		// Debugging info for the script.
} ARRAY_BLOCK;

/* Type for a constant symbol */
typedef struct _const_symbol
{
	const char		*pIdent;	// variable's identifier
	INTERP_TYPE		type;		// variable type

	/* The actual value of the constant.
	 * Only one of these will be valid depending on type.
	 * A union is not used as a union cannot be statically initialised
	 */
	BOOL			bval;
	SDWORD			ival;
	void			*oval;
	char			*sval;	//String values
	float			fval;
} CONST_SYMBOL;

/* The chunk of code used to reference an object variable */
typedef struct _objvar_block
{
	VAR_SYMBOL		*psObjVar;	// The object variables symbol

	UDWORD			size;
	INTERP_VAL		*pCode;		// The code to get the object value on the stack
} OBJVAR_BLOCK;

/* The maximum number of parameters for an instinct function */
#define INST_MAXPARAMS		20

/* Type for a function symbol */
typedef struct _func_symbol
{
	const char	*pIdent;	// function's identifier
	SCRIPT_FUNC	pFunc;		// Pointer to the instinct function
	INTERP_TYPE	type;		// function type
	UDWORD		numParams;	// Number of parameters to the function
	uint32_t/*INTERP_TYPE*/ aParams[INST_MAXPARAMS];
							// List of parameter types
	BOOL		script;		// Whether the function is defined in the script
							// or a C instinct function
	UDWORD		size;		// The size of script code
	INTERP_VAL	*pCode;		// The code for a function if it is defined in the script
	UDWORD		location;	// The position of the function in the final code block
	UDWORD			debugEntries;	// Number of debugging entries in psDebug.
	SCRIPT_DEBUG	*psDebug;		// Debugging info for the script.

	struct _func_symbol *psNext;
} FUNC_SYMBOL;

/* The type for a variable declaration */
typedef struct _var_decl
{
	INTERP_TYPE		type;
	STORAGE_TYPE	storage;
} VAR_DECL;

/* The type for a trigger sub declaration */
typedef struct _trigger_decl
{
	TRIGGER_TYPE	type;
	UDWORD			size;
	INTERP_VAL		*pCode;
	UDWORD			time;
} TRIGGER_DECL;

/* Type for a trigger symbol */
typedef struct _trigger_symbol
{
	char			*pIdent;	// Trigger's identifier
	UDWORD			index;		// The triggers index number
	TRIGGER_TYPE	type;		// Trigger type
	UDWORD			size;		// Code size for the trigger
	INTERP_VAL		*pCode;		// The trigger code
	UDWORD			time;		// How often to check the trigger

	UDWORD			debugEntries;
	SCRIPT_DEBUG	*psDebug;

	struct _trigger_symbol *psNext;
} TRIGGER_SYMBOL;

/* The type for a callback trigger symbol */
typedef struct _callback_symbol
{
	const char		*pIdent;	// Callback identifier
	TRIGGER_TYPE	type;		// user defined callback id >= TR_CALLBACKSTART
	SCRIPT_FUNC		pFunc;		// Pointer to the instinct function
	UDWORD			numParams;	// Number of parameters to the function
	uint32_t/*INTERP_TYPE*/ aParams[INST_MAXPARAMS];
								// List of parameter types
} CALLBACK_SYMBOL;


/* Type for an event symbol */
typedef struct _event_symbol
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
	BOOL		bFunction;		//if this event is defined as a function
	BOOL		bDeclared;		//if function was declared before
	INTERP_TYPE	retType;		//return type if a function

	INTERP_TYPE	aParams[INST_MAXPARAMS];

	struct _event_symbol *psNext;
} EVENT_SYMBOL;

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
extern void scriptSetInputFile(PHYSFS_file* fileHandle);

/* Initialise the parser ready for a new script */
extern BOOL scriptInitParser(void);

/* Set off the scenario file parser */
extern int scr_parse(void);

/* Give an error message */
void scr_error(const char *pMessage, ...) WZ_DECL_FORMAT(printf, 1, 2);

extern void scriptGetErrorData(int *pLine, char **ppText);

/* Look up a type symbol */
extern BOOL scriptLookUpType(const char *pIdent, INTERP_TYPE *pType);

/* Add a new variable symbol */
extern BOOL scriptAddVariable(VAR_DECL *psStorage, VAR_IDENT_DECL *psVarIdent);

/* Add a new trigger symbol */
extern BOOL scriptAddTrigger(const char *pIdent, TRIGGER_DECL *psDecl, UDWORD line);

/* Add a new event symbol */
extern BOOL scriptDeclareEvent(const char *pIdent, EVENT_SYMBOL **ppsEvent, SDWORD numArgs);

// Add the code to a defined event
extern BOOL scriptDefineEvent(EVENT_SYMBOL *psEvent, CODE_BLOCK *psCode, SDWORD trigger);

/* Look up a variable symbol */
extern BOOL scriptLookUpVariable(const char *pIdent, VAR_SYMBOL **ppsSym);

/* Look up a constant variable symbol */
extern BOOL scriptLookUpConstant(const char *pIdent, CONST_SYMBOL **ppsSym);

/* Lookup a trigger symbol */
extern BOOL scriptLookUpTrigger(const char *pIdent, TRIGGER_SYMBOL **ppsTrigger);

/* Lookup a callback trigger symbol */
extern BOOL scriptLookUpCallback(const char *pIdent, CALLBACK_SYMBOL **ppsCallback);

/* Lookup an event symbol */
extern BOOL scriptLookUpEvent(const char *pIdent, EVENT_SYMBOL **ppsEvent);

/* Add a new function symbol */
extern BOOL scriptStartFunctionDef(const char *pIdent,	// Functions name
						  INTERP_TYPE		type);		// return type

/* Store the parameter types for the current script function definition  */
extern BOOL scriptSetParameters(UDWORD		numParams,	// number of parameters
							  INTERP_TYPE		*pParams);	// parameter types

/* Store the code for a script function definition.
 * Clean up the local variable list for this function definition.
 */
extern BOOL scriptSetCode(CODE_BLOCK  *psBlock);	// The code block

/* Look up a function symbol */
extern BOOL scriptLookUpFunction(const char *pIdent, FUNC_SYMBOL **ppsSym);

/* Look up an in-script custom function symbol */
extern BOOL scriptLookUpCustomFunction(const char *pIdent, EVENT_SYMBOL **ppsSym);

extern BOOL popArguments(INTERP_VAL **ip_temp, SDWORD numParams);

extern void widgCopyString(char *pDest, const char *pSrc); // FIXME Duplicate declaration of internal widget function

extern void script_debug(const char *pFormat, ...);

#endif
