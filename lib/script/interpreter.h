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
 * Interp.h
 *
 * Script interpreter definitions
 */
#ifndef _interp_h
#define _interp_h

/* The type of function called by an OP_CALL */
typedef bool (*SCRIPT_FUNC)(void);

/* The type of function called to access an object or in-game variable */
typedef bool (*SCRIPT_VARFUNC)(UDWORD index);

/* The possible value types for scripts */
enum INTERP_TYPE
{
	// Basic types
	VAL_BOOL,
	VAL_INT,
	VAL_FLOAT,
	VAL_STRING,

	// events and triggers
	VAL_TRIGGER, //!< trigger index
	VAL_EVENT, //!< event (or in-script function)

	/* Type used by the compiler for functions that do not return a value */
	VAL_VOID,

	VAL_OPCODE, //!< Script opcode
	VAL_PKOPCODE, //!< Packed script opcode

	VAL_OBJ_GETSET, //!< Pointer to the object .set or .get function
	VAL_FUNC_EXTERN, //!< External function pointer

	VAL_USERTYPESTART, //!< user defined types should start with this id
	VAL_REF = 0x00100000 //!< flag to specify a variable reference rather than simple value
};


/* A value consists of its type and value */
struct INTERP_VAL
{
	INTERP_TYPE		type;					//Value type for interpreter; opcode or value type for compiler
	union
	{
		char					*sval;		//String value - VAL_STRING
		SCRIPT_VARFUNC		pObjGetSet;		//Set/Get method for objects - VAL_OBJ_GETSET
		SCRIPT_FUNC			pFuncExtern;	//External (C) function - VAL_FUNC_EXTERN
		void					*oval;		//Object value - any in-game object
		float					fval;		//Float value - VAL_FLOAT
		int						ival;		// Integer value - VAL_INT
		int32_t					bval;		//Boolean value - VAL_BOOL [NOTE: Even though this is a bool, we must make it this size to prevent issues]
	} v;
};


// maximum number of equivalent types for a type
#define INTERP_MAXEQUIV		10

// type equivalences
struct TYPE_EQUIV
{
	INTERP_TYPE		base;		// the type that the others are equivalent to
	unsigned int		numEquiv;	// number of equivalent types
	INTERP_TYPE		aEquivTypes[INTERP_MAXEQUIV]; // the equivalent types
};

/* Opcodes for the script interpreter */
enum OPCODE
{
	OP_PUSH,		// Push value onto stack
	OP_PUSHREF,		// Push a pointer to a variable onto the stack
	OP_POP,			// Pop value from stack

	OP_PUSHGLOBAL,	// Push the value of a global variable onto the stack
	OP_POPGLOBAL,	// Pop a value from the stack into a global variable

	OP_PUSHARRAYGLOBAL,	// Push the value of a global array variable onto the stack
	OP_POPARRAYGLOBAL,	// Pop a value from the stack into a global array variable

	OP_CALL,		// Call the 'C' function pointed to by the next value
	OP_VARCALL,		// Call the variable access 'C' function pointed to by the next value

	OP_JUMP,		// Jump to a different location in the script
	OP_JUMPTRUE,	// Jump if the top stack value is true
	OP_JUMPFALSE,	// Jump if the top stack value is false

	OP_BINARYOP,	// Call a binary maths/boolean operator
	OP_UNARYOP,		// Call a unary maths/boolean operator

	OP_EXIT,			// End the program
	OP_PAUSE,			// temporarily pause the current event

	// The following operations are secondary data to OP_BINARYOP and OP_UNARYOP

	// Maths operators
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_NEG,
	OP_INC,
	OP_DEC,

	// Boolean operators
	OP_AND,
	OP_OR,
	OP_NOT,

	//String concatenation
	OP_CONC,

	// Comparison operators
	OP_EQUAL,
	OP_NOTEQUAL,
	OP_GREATEREQUAL,
	OP_LESSEQUAL,
	OP_GREATER,
	OP_LESS,

	OP_FUNC,		//custom (in-script) function call
	OP_POPLOCAL,	//local var
	OP_PUSHLOCAL,

	OP_PUSHLOCALREF,	//variable of object type (pointer)
	OP_TO_FLOAT,			//float cast
	OP_TO_INT,				//int cast
};

/* How far the opcode is shifted up a UDWORD to allow other data to be
 * stored in the same UDWORD
 */
#define OPCODE_SHIFT		24
#define OPCODE_DATAMASK		0x00ffffff

// maximum sizes for arrays
#define VAR_MAX_DIMENSIONS		4
#define VAR_MAX_ELEMENTS		UBYTE_MAX

/* The mask for the number of array elements stored in the data part of an opcode */
#define ARRAY_BASE_MASK			0x000fffff
#define ARRAY_DIMENSION_SHIFT	20
#define ARRAY_DIMENSION_MASK	0x00f00000

/* The possible storage types for a variable */
enum enum_STORAGE_TYPE
{
	ST_PUBLIC,		// Public variable
	ST_PRIVATE,		// Private variable
	ST_OBJECT,		// A value stored in an objects data space.
	ST_EXTERN,		// An external value accessed by function call
	ST_LOCAL,		// A local variable
};

typedef UBYTE STORAGE_TYPE;

/* Variable debugging info for a script */
struct VAR_DEBUG
{
	char			*pIdent;
	STORAGE_TYPE	storage;
};

/* Array info for a script */
struct ARRAY_DATA
{
	UDWORD			base;			// the base index of the array values
	INTERP_TYPE		type;			// the array data type
	UBYTE			dimensions;
	UBYTE			elements[VAR_MAX_DIMENSIONS];
};

/* Array debug info for a script */
struct ARRAY_DEBUG
{
	char			*pIdent;
	UBYTE			storage;
};

/* Line debugging information for a script */
struct SCRIPT_DEBUG
{
	UDWORD	offset;		// Offset in the compiled script that corresponds to
	UDWORD	line;		// this line in the original script.
	char	*pLabel;	// the trigger/event that starts at this line
};

/* Different types of triggers */
enum TRIGGER_TYPE
{
	TR_INIT,		// Trigger fires when the script is first run
	TR_CODE,		// Trigger uses script code
	TR_WAIT,		// Trigger after a time pause
	TR_EVERY,		// Trigger at repeated intervals
	TR_PAUSE,		// Event has paused for an interval and will restart in the middle of it's code

	TR_CALLBACKSTART,	// The user defined callback triggers should start with this id
	CALL_GAMEINIT = TR_CALLBACKSTART,
	CALL_DELIVPOINTMOVED,
	CALL_DROIDDESIGNED,
	CALL_DROIDBUILT,
	CALL_POWERGEN_BUILT,
	CALL_RESEX_BUILT,
	CALL_RESEARCH_BUILT,
	CALL_FACTORY_BUILT,
	CALL_MISSION_START,
	CALL_MISSION_END,
	CALL_VIDEO_QUIT,
	CALL_LAUNCH_TRANSPORTER,
	CALL_START_NEXT_LEVEL,
	CALL_TRANSPORTER_REINFORCE,
	CALL_MISSION_TIME,
	CALL_ELECTRONIC_TAKEOVER,
	CALL_BUILDLIST,
	CALL_BUILDGRID,
	CALL_RESEARCHLIST,
	CALL_MANURUN,
	CALL_MANULIST,
	CALL_BUTTON_PRESSED,
	CALL_DROID_SELECTED,
	CALL_DESIGN_QUIT,
	CALL_DESIGN_WEAPON,
	CALL_DESIGN_SYSTEM,
	CALL_DESIGN_COMMAND,
	CALL_DESIGN_BODY,
	CALL_DESIGN_PROPULSION,
	CALL_RESEARCHCOMPLETED,
	CALL_NEWDROID,
	CALL_STRUCT_ATTACKED,
	CALL_DROID_ATTACKED,
	CALL_ATTACKED,
	CALL_STRUCT_SEEN,
	CALL_DROID_SEEN,
	CALL_FEATURE_SEEN,
	CALL_OBJ_SEEN,
	CALL_OBJ_DESTROYED,
	CALL_STRUCT_DESTROYED,
	CALL_DROID_DESTROYED,
	CALL_FEATURE_DESTROYED,
	CALL_OBJECTOPEN,
	CALL_OBJECTCLOSE,
	CALL_TRANSPORTER_OFFMAP,
	CALL_TRANSPORTER_LANDED,
	CALL_ALL_ONSCREEN_DROIDS_SELECTED,
	CALL_NO_REINFORCEMENTS_LEFT,
	CALL_CLUSTER_EMPTY,
	CALL_VTOL_OFF_MAP,
	CALL_UNITTAKEOVER,
	CALL_PLAYERLEFT,
	CALL_ALLIANCEOFFER,
	CALL_CONSOLE,			// Gets fired when user types something in the console and presses enter
	CALL_AI_MSG,			// Player received msg from another player
	CALL_BEACON,			// Beacon help (blip) msg received
	CALL_STRUCTBUILT,		// Gets fired when a structure is built for a certain player, returns structure
	CALL_TRANSPORTER_LANDED_B,
	CALL_DORDER_STOP,		// Fired when droid is forced to stop via user interface
	CALL_DROID_REACH_LOCATION,	// Fired when droid reached the destination and stopped on its own
	CALL_VTOL_RETARGET,		// VTOL is out of targets
};

/* Description of a trigger for the SCRIPT_CODE */
struct TRIGGER_DATA
{
	TRIGGER_TYPE		type;		// Type of trigger
	UWORD			code;		// bool - is there code with this trigger
	UDWORD			time;		// How often to check the trigger
};

/* A compiled script and its associated data */
struct SCRIPT_CODE
{
	UDWORD			size;			// The size (in bytes) of the compiled code
	INTERP_VAL		*pCode;			// Pointer to the compiled code

	UWORD			numTriggers;	// The number of triggers
	UWORD			numEvents;		// The number of events
	UWORD			*pTriggerTab;	// The table of trigger offsets
	TRIGGER_DATA	*psTriggerData;	// The extra info for each trigger
	UWORD			*pEventTab;		// The table of event offsets
	SWORD			*pEventLinks;	// The original trigger/event linkage
	// -1 for no link

	UWORD			numGlobals;		// The number of global variables
	UWORD			numArrays;		// the number of arrays in the program
	UDWORD			arraySize;		// the number of elements in all the defined arrays
	INTERP_TYPE		*pGlobals;		// Types of the global variables


	INTERP_TYPE		**ppsLocalVars;		//storage for local vars (type)
	UDWORD			*numLocalVars;		//number of local vars each event has
	INTERP_VAL		**ppsLocalVarVal;	//Values of the local vars used during interpreting process
	UDWORD			*numParams;			//number of arguments this event has


	VAR_DEBUG		*psVarDebug;	// The names and storage types of variables
	ARRAY_DATA		*psArrayInfo;	// The sizes of the program arrays
	ARRAY_DEBUG		*psArrayDebug;	// Debug info for the arrays

	UWORD			debugEntries;	// Number of entries in psDebug
	SCRIPT_DEBUG	*psDebug;		// Debugging info for the script
};


/* What type of code should be run by the interpreter */
enum INTERP_RUNTYPE
{
	IRT_TRIGGER,					// Run trigger code
	IRT_EVENT,						// Run event code
};


/* The size of each opcode */
extern SDWORD aOpSize[];

/* Check if two types are equivalent */
extern bool interpCheckEquiv(INTERP_TYPE to, INTERP_TYPE from);

// Initialise the interpreter
extern bool interpInitialise(void);

// true if the interpreter is currently running
extern bool interpProcessorActive(void);

/* Output script call stack trace */
extern void scrOutputCallTrace(code_part part);

#endif
