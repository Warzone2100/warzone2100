/*
	This file is part of Warzone 2100.
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

#if defined( _MSC_VER )
	#pragma warning( disable : 4702 ) // warning C4702: unreachable code
	#pragma warning( disable : 4065 ) // warning C4065: switch statement contains 'default' but no 'case' labels
#endif

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         scr_parse
#define yylex           scr_lex
#define yyerror         scr_error
#define yylval          scr_lval
#define yychar          scr_char
#define yydebug         scr_debug
#define yynerrs         scr_nerrs


/* Copy the first part of user declarations.  */

/* Line 268 of yacc.c  */
#line 21 "script_parser.ypp"

/*
 * script.y
 *
 * The yacc grammar for the scipt files.
 */

#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/string_ext.h"
#include "lib/script/interpreter.h"
#include "lib/script/parse.h"
#include "lib/script/script.h"

#include <physfs.h>

/* this will give us a more detailed error output */
#define YYERROR_VERBOSE true

/* Script defines stack */

extern int scr_lex(void);
extern int scr_lex_destroy(void);

/* Error return codes for code generation functions */
enum CODE_ERROR
{
	CE_OK,				// No error
	CE_MEMORY,			// Out of memory
	CE_PARSE			// A parse error occurred
};

/* Turn off a couple of warnings that the yacc generated code gives */

/* Pointer to the compiled code */
static SCRIPT_CODE	*psFinalProg=NULL;

/* Pointer to current block of compiled code */
static CODE_BLOCK	*psCurrBlock=NULL;

/* Pointer to current block of conditional code */
static COND_BLOCK	*psCondBlock=NULL;

/* Pointer to current block of object variable code */
static OBJVAR_BLOCK	*psObjVarBlock=NULL;

/* Pointer to current block of compiled parameter code */
static PARAM_BLOCK	*psCurrPBlock=NULL;

/* Any errors occurred? */
static bool bError=false;

//String support
//-----------------------------
char 				msg[MAXSTRLEN];
extern char			STRSTACK[MAXSTACKLEN][MAXSTRLEN];	// just a simple string "stack"
extern UDWORD		CURSTACKSTR;				//Current string index

/* Pointer into the current code block */
static INTERP_VAL		*ip;

/* Pointer to current parameter declaration block */
//static PARAM_DECL	*psCurrParamDecl=NULL;

/* Pointer to current trigger subdeclaration */
static TRIGGER_DECL	*psCurrTDecl=NULL;

/* Pointer to current variable subdeclaration */
static VAR_DECL		*psCurrVDecl=NULL;

/* Pointer to the current variable identifier declaration */
static VAR_IDENT_DECL	*psCurrVIdentDecl=NULL;

/* Pointer to the current array access block */
static ARRAY_BLOCK		*psCurrArrayBlock=NULL;

/* Return code from code generation functions */
static CODE_ERROR	codeRet;

/* The list of global variables */
static VAR_SYMBOL	*psGlobalVars=NULL;

/* The list of global arrays */
static VAR_SYMBOL	*psGlobalArrays=NULL;

#define			maxEventsLocalVars		1200
static VAR_SYMBOL	*psLocalVarsB[maxEventsLocalVars];	/* local var storage */
static UDWORD		numEventLocalVars[maxEventsLocalVars];	/* number of declard local vars for each event */
EVENT_SYMBOL		*psCurEvent = NULL;		/* stores current event: for local var declaration */

/* The current object variable context */
static INTERP_TYPE	objVarContext = (INTERP_TYPE)0;

/* Control whether debug info is generated */
static bool			genDebugInfo = true;

/* Currently defined triggers */
static TRIGGER_SYMBOL	*psTriggers;
static UDWORD			numTriggers;

/* Currently defined events */
static EVENT_SYMBOL		*psEvents;
static UDWORD			numEvents;

/* This is true when local variables are being defined.
 * (So local variables can have the same name as global ones)
 */
static bool localVariableDef=false;

/* The identifier for the current script function being defined */
//static char *pCurrFuncIdent=NULL;

/* A temporary store for a line number - used when
 * generating debugging info for functions, conditionals and loops.
 */
static UDWORD		debugLine;

/* The table of user types */
TYPE_SYMBOL		*asScrTypeTab;

/* The table of instinct function type definitions */
FUNC_SYMBOL		*asScrInstinctTab;

/* The table of external variables and their access functions */
VAR_SYMBOL		*asScrExternalTab;

/* The table of object variables and their access functions. */
VAR_SYMBOL		*asScrObjectVarTab;

/* The table of constant variables */
CONST_SYMBOL	*asScrConstantTab;

/* The table of callback triggers */
CALLBACK_SYMBOL	*asScrCallbackTab;

/* Used for additional debug output */
void script_debug(const char *pFormat, ...);

/****************************************************************************************
 *
 * Code Block Macros
 *
 * These macros are used to allocate and free the different types of code
 * block used within the compiler
 */


/* What the macro should do if it has an allocation error.
 * This is different depending on whether the macro is used
 * in a function, or in a rule body.
 *
 * This definition is used within the code generation functions
 * and is then changed for use within a rule body.
 */
#define ALLOC_ERROR_ACTION return CE_MEMORY

/* Macro to allocate a program structure, size is in _bytes_ */
#define ALLOC_PROG(psProg, codeSize, pAICode, numGlobs, numArys, numTrigs, numEvnts) \
	(psProg) = (SCRIPT_CODE *)malloc(sizeof(SCRIPT_CODE)); \
	if ((psProg) == NULL) \
	{ \
		debug(LOG_ERROR, "Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psProg)->pCode = (INTERP_VAL *)malloc((codeSize) * sizeof(INTERP_VAL)); \
	if ((psProg)->pCode == NULL) \
	{ \
		debug(LOG_ERROR, "Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	if (numGlobs > 0) \
	{ \
		(psProg)->pGlobals = (INTERP_TYPE *)malloc(sizeof(INTERP_TYPE) * (numGlobs)); \
		if ((psProg)->pGlobals == NULL) \
		{ \
			debug(LOG_ERROR, "Out of memory"); \
			ALLOC_ERROR_ACTION; \
		} \
	} \
	else \
	{ \
		(psProg)->pGlobals = NULL; \
	} \
	if (numArys > 0) \
	{ \
		(psProg)->psArrayInfo = (ARRAY_DATA *)malloc(sizeof(ARRAY_DATA) * (numArys)); \
		if ((psProg)->psArrayInfo == NULL) \
		{ \
			debug(LOG_ERROR, "Out of memory"); \
			ALLOC_ERROR_ACTION; \
		} \
	} \
	else \
	{ \
		(psProg)->psArrayInfo = NULL; \
	} \
	(psProg)->numArrays = (UWORD)(numArys); \
	if ((numTrigs) > 0) \
	{ \
		(psProg)->pTriggerTab = (uint16_t *)malloc(sizeof(uint16_t) * ((numTrigs) + 1)); \
		if ((psProg)->pTriggerTab == NULL) \
		{ \
			debug(LOG_ERROR, "Out of memory"); \
			ALLOC_ERROR_ACTION; \
		} \
		(psProg)->psTriggerData = (TRIGGER_DATA *)malloc(sizeof(TRIGGER_DATA) * (numTrigs)); \
		if ((psProg)->psTriggerData == NULL) \
		{ \
			debug(LOG_ERROR, "Out of memory"); \
			ALLOC_ERROR_ACTION; \
		} \
	} \
	else \
	{ \
		(psProg)->pTriggerTab = NULL; \
		(psProg)->psTriggerData = NULL; \
	} \
	(psProg)->pEventTab = (uint16_t *)malloc(sizeof(uint16_t) * ((numEvnts) + 1)); \
	if ((psProg)->pEventTab == NULL) \
	{ \
		debug(LOG_ERROR, "Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psProg)->pEventLinks = (int16_t *)malloc(sizeof(int16_t) * (numEvnts)); \
	if ((psProg)->pEventLinks == NULL) \
	{ \
		debug(LOG_ERROR, "Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psProg)->numGlobals = (UWORD)(numGlobs); \
	(psProg)->numTriggers = (UWORD)(numTriggers); \
	(psProg)->numEvents = (UWORD)(numEvnts); \
	(psProg)->size = (codeSize) * sizeof(INTERP_VAL);

/* Macro to allocate a code block, blockSize - number of INTERP_VALs we need*/
#define ALLOC_BLOCK(psBlock, num) \
	(psBlock) = (CODE_BLOCK *)malloc(sizeof(CODE_BLOCK)); \
	if ((psBlock) == NULL) \
	{ \
		debug(LOG_ERROR, "Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psBlock)->pCode = (INTERP_VAL *)malloc((num) * sizeof(INTERP_VAL)); \
	if ((psBlock)->pCode == NULL) \
	{ \
		debug(LOG_ERROR, "Out of memory"); \
		free((psBlock)); \
		ALLOC_ERROR_ACTION; \
	} \
	(psBlock)->size = (num); \
	(psBlock)->debugEntries = 0

/* Macro to free a code block */
#define FREE_BLOCK(psBlock) \
	free((psBlock)->pCode); \
	free((psBlock))

/* Macro to allocate a parameter block */
#define ALLOC_PBLOCK(psBlock, num, paramSize) \
	(psBlock) = (PARAM_BLOCK *)malloc(sizeof(PARAM_BLOCK)); \
	if ((psBlock) == NULL) \
	{ \
		debug(LOG_ERROR, "Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psBlock)->pCode = (INTERP_VAL *)malloc((num) * sizeof(INTERP_VAL)); \
	if ((psBlock)->pCode == NULL) \
	{ \
		debug(LOG_ERROR, "Out of memory"); \
		free((psBlock)); \
		ALLOC_ERROR_ACTION; \
	} \
	(psBlock)->aParams = (INTERP_TYPE *)malloc(sizeof(INTERP_TYPE) * (paramSize)); \
	if ((psBlock)->aParams == NULL) \
	{ \
		debug(LOG_ERROR, "Out of memory"); \
		free((psBlock)->pCode); \
		free((psBlock)); \
		ALLOC_ERROR_ACTION; \
	} \
	(psBlock)->size = (num); \
	(psBlock)->numParams = (paramSize)

/* Macro to free a parameter block */
#define FREE_PBLOCK(psBlock) \
	free((psBlock)->pCode); \
	free((psBlock)->aParams); \
	free((psBlock))

/* Macro to allocate a parameter declaration block */
#define ALLOC_PARAMDECL(psPDecl, num) \
	(psPDecl) = (PARAM_DECL *)malloc(sizeof(PARAM_DECL)); \
	if ((psPDecl) == NULL) \
	{ \
		debug(LOG_ERROR, "Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psPDecl)->aParams = (INTERP_TYPE *)malloc(sizeof(INTERP_TYPE) * (num)); \
	if ((psPDecl)->aParams == NULL) \
	{ \
		debug(LOG_ERROR, "Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psPDecl)->numParams = (num)

/* Macro to free a parameter declaration block */
#define FREE_PARAMDECL(psPDecl) \
	free((psPDecl)->aParams); \
	free((psPDecl))

/* Macro to allocate a conditional block */
#define ALLOC_CONDBLOCK(psCB, num, numBlocks) \
	(psCB) = (COND_BLOCK *)malloc(sizeof(COND_BLOCK)); \
	if ((psCB) == NULL) \
	{ \
		debug(LOG_ERROR, "Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psCB)->aOffsets = (UDWORD *)malloc(sizeof(SDWORD) * (num)); \
	if ((psCB)->aOffsets == NULL) \
	{ \
		debug(LOG_ERROR, "Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psCB)->pCode = (INTERP_VAL *)malloc((numBlocks) * sizeof(INTERP_VAL)); \
	if ((psCB)->pCode == NULL) \
	{ \
		debug(LOG_ERROR, "Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psCB)->size = (numBlocks); \
	(psCB)->numOffsets = (num)

/* Macro to free a conditional block */
#define FREE_CONDBLOCK(psCB) \
	free((psCB)->aOffsets); \
	free((psCB)->pCode); \
	free(psCB)

/* Macro to free a code block */
#define FREE_USERBLOCK(psBlock) \
	free((psBlock)->pCode); \
	free((psBlock))

/* Macro to allocate an object variable block */
#define ALLOC_OBJVARBLOCK(psOV, blockSize, psVar) \
	(psOV) = (OBJVAR_BLOCK *)malloc(sizeof(OBJVAR_BLOCK)); \
	if ((psOV) == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psOV)->pCode = (INTERP_VAL *)malloc((blockSize) * sizeof(INTERP_VAL)); \
	if ((psOV)->pCode == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psOV)->size = (blockSize); \
	(psOV)->psObjVar = (psVar)

/* Macro to free an object variable block */
#define FREE_OBJVARBLOCK(psOV) \
	free((psOV)->pCode); \
	free(psOV)

/* Macro to allocate an array variable block */
#define ALLOC_ARRAYBLOCK(psAV, blockSize, psVar) \
	(psAV) = (ARRAY_BLOCK *)malloc(sizeof(ARRAY_BLOCK)); \
	if ((psAV) == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psAV)->pCode = (INTERP_VAL *)malloc((blockSize) * sizeof(INTERP_VAL)); \
	if ((psAV)->pCode == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psAV)->size = (blockSize); \
	(psAV)->dimensions = 1; \
	(psAV)->psArrayVar = (psVar)

/* Macro to free an object variable block */
#define FREE_ARRAYBLOCK(psAV) \
	free((psAV)->pCode); \
	free((psAV))

/* Allocate a trigger subdecl */
#define ALLOC_TSUBDECL(psTSub, blockType, blockSize, blockTime) \
	(psTSub) = (TRIGGER_DECL *)malloc(sizeof(TRIGGER_DECL)); \
	if ((psTSub) == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psTSub)->type = (blockType); \
	(psTSub)->time = (blockTime); \
	if ((blockSize) > 0) \
	{ \
		(psTSub)->pCode = (INTERP_VAL *)malloc((blockSize) * sizeof(INTERP_VAL)); \
		if ((psTSub)->pCode == NULL) \
		{ \
			scr_error("Out of memory"); \
			ALLOC_ERROR_ACTION; \
		} \
		(psTSub)->size = (blockSize); \
	} \
	else \
	{ \
		(psTSub)->pCode = NULL; \
		(psTSub)->size = 0; \
	}

/* Free a trigger subdecl */
#define FREE_TSUBDECL(psTSub) \
	if ((psTSub)->pCode) \
	{ \
		free((psTSub)->pCode); \
	} \
	free(psTSub)

/* Allocate a variable declaration block */
#define ALLOC_VARDECL(psDcl) \
	(psDcl) = (VAR_DECL *)malloc(sizeof(VAR_DECL)); \
	if ((psDcl) == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	}

/* Free a variable declaration block */
#define FREE_VARDECL(psDcl) \
	free(psDcl)

/* Allocate a variable declaration block */
static inline CODE_ERROR do_ALLOC_VARIDENTDECL(VAR_IDENT_DECL** psDcl, const char* ident, unsigned int dim)
{
	// Allocate memory
	*psDcl = (VAR_IDENT_DECL *)malloc(sizeof(VAR_IDENT_DECL));
	if (*psDcl == NULL)
	{
		scr_error("Out of memory");
		ALLOC_ERROR_ACTION;
	}

	// Copy over the "ident" string (if it's there)
	if (ident != NULL)
	{
		(*psDcl)->pIdent = strdup(ident);
		if ((*psDcl)->pIdent == NULL)
		{
			scr_error("Out of memory");
			ALLOC_ERROR_ACTION;
		}
	}
	else
	{
		(*psDcl)->pIdent = NULL;
	}

	(*psDcl)->dimensions = dim;

	return CE_OK;
}

#define ALLOC_VARIDENTDECL(psDcl, ident, dim) \
{ \
	CODE_ERROR err = do_ALLOC_VARIDENTDECL(&psDcl, ident, dim); \
	if (err != CE_OK) \
		return err; \
}

/* Free a variable declaration block */
static void freeVARIDENTDECL(VAR_IDENT_DECL* psDcl)
{
	free(psDcl->pIdent);
	free(psDcl);
}

/****************************************************************************************
 *
 * Code block manipulation macros.
 *
 * These are used to copy chunks of code from one block to another
 * or to insert opcodes or other values into a code block.
 * All the macros use the ip parameter.  This is a pointer into the code
 * block that is incremented by the macro. This ensures that it always points
 * to the next free space in the code block.
 */

/* Macro to store an opcode in a code block, opcode type is '-1' */
#define PUT_OPCODE(ip, opcode) \
	(ip)->type = VAL_OPCODE; \
	(ip)->v.ival = (opcode) << OPCODE_SHIFT; \
	(ip)++

/* Macro to put a packed opcode in a code block */
#define PUT_PKOPCODE(ip, opcode, data) \
	(ip)->type = VAL_PKOPCODE; \
	(ip)->v.ival = ((SDWORD)(data)) & OPCODE_DATAMASK; \
	(ip)->v.ival = (((SDWORD)(opcode)) << OPCODE_SHIFT) | ((ip)->v.ival); \
	(ip)++

/* Special macro for floats, could be a bit tricky */
#define PUT_DATA_FLOAT(ip, data) \
	ip->type = VAL_FLOAT; \
	ip->v.fval = (float)(data); \
	ip++

/* Macros to store a value in a code block */
#define PUT_DATA_BOOL(ip, value) \
	(ip)->type = VAL_BOOL; \
	(ip)->v.bval = (int32_t)(value); \
	(ip)++

#define PUT_DATA_INT(ip, value) \
	(ip)->type = VAL_INT; \
	(ip)->v.ival = (SDWORD)(value); \
	(ip)++

#define PUT_DATA_STRING(ip, value) \
	(ip)->type = VAL_STRING; \
	(ip)->v.sval = (char *)(value); \
	(ip)++

#define PUT_OBJECT_CONST(ip, value, constType) \
	(ip)->type = (constType); \
	(ip)->v.oval = (value); \
	(ip)++

#define PUT_STRING(ip, value) \
	((INTERP_VAL *)(ip))->type = VAL_STRING; \
	((INTERP_VAL *)(ip))->v.sval = (value); \
	((INTERP_VAL *)(ip)) += 1

/* Macro to store an external function index in a code block */
#define PUT_FUNC_EXTERN(ip, func) \
	(ip)->type = VAL_FUNC_EXTERN; \
	(ip)->v.pFuncExtern = (SCRIPT_FUNC)func; \
	(ip)++

/* Macro to store an internal (in-script, actually an event) function index in a code block */
#define PUT_EVENT(ip, func) \
	(ip)->type = VAL_EVENT; \
	(ip)->v.ival = (SDWORD)func; \
	(ip)++

/* Macro to store trigger */
#define PUT_TRIGGER(ip, func) \
	(ip)->type = VAL_TRIGGER; \
	(ip)->v.ival = (SDWORD)func; \
	(ip)++

/* Macro to store a function pointer in a code block */
#define PUT_VARFUNC(ip, func) \
	(ip)->type = VAL_OBJ_GETSET; \
	(ip)->v.pObjGetSet = (SCRIPT_VARFUNC)func; \
	(ip)++

/* Macro to store a variable index number in a code block - NOT USED */
#define PUT_INDEX(ip, index) \
	(ip)->type = -1; \
	(ip)->v.ival = (SDWORD)(index); \
	(ip)++

/* Macro to copy a code block into another code block */
#define PUT_BLOCK(ip, psOldBlock) \
	memcpy(ip, (psOldBlock)->pCode, (psOldBlock)->size * sizeof(INTERP_VAL)); \
	(ip) = (INTERP_VAL *)( (ip) + (psOldBlock)->size)

/***********************************************************************************
 *
 * Debugging information macros
 *
 * These macros are only used to generate debugging information for scripts.
 */

/* Macro to allocate debugging info for a CODE_BLOCK or a COND_BLOCK */
#define ALLOC_DEBUG(psBlock, num) \
	if (genDebugInfo) \
	{ \
		(psBlock)->psDebug = (SCRIPT_DEBUG *)malloc(sizeof(SCRIPT_DEBUG) * (num)); \
		if ((psBlock)->psDebug == NULL) \
		{ \
			scr_error("Out of memory"); \
			ALLOC_ERROR_ACTION; \
		} \
		memset((psBlock)->psDebug, 0, sizeof(SCRIPT_DEBUG) * (num));\
		(psBlock)->debugEntries = (UWORD)(num); \
	} \
	else \
	{ \
		(psBlock)->psDebug = NULL; \
		(psBlock)->debugEntries = 0; \
	}

/* Macro to free debugging info */
#define FREE_DEBUG(psBlock) \
	if (genDebugInfo) \
		free((psBlock)->psDebug)


/* Macro to copy the debugging information from one block to another */
#define PUT_DEBUG(psFinal, psBlock) \
	if (genDebugInfo) \
	{ \
		memcpy((psFinal)->psDebug, (psBlock)->psDebug, \
					sizeof(SCRIPT_DEBUG) * (psBlock)->debugEntries); \
		(psFinal)->debugEntries = (psBlock)->debugEntries; \
	}

/* Macro to combine the debugging information in two blocks into a third block */
static UDWORD		_dbEntry;
static SCRIPT_DEBUG	*_psCurr;
#define COMBINE_DEBUG(psFinal, psBlock1, psBlock2) \
	if (genDebugInfo) \
	{ \
		memcpy((psFinal)->psDebug, (psBlock1)->psDebug, \
				 sizeof(SCRIPT_DEBUG) * (psBlock1)->debugEntries); \
		_baseOffset = (psBlock1)->size / sizeof(UDWORD); \
		for(_dbEntry = 0; _dbEntry < (psBlock2)->debugEntries; _dbEntry++) \
		{ \
			_psCurr = (psFinal)->psDebug + (psBlock1)->debugEntries + _dbEntry; \
			_psCurr->line = (psBlock2)->psDebug[_dbEntry].line; \
			_psCurr->offset = (psBlock2)->psDebug[_dbEntry].offset + _baseOffset; \
		} \
		(psFinal)->debugEntries = (psBlock1)->debugEntries + (psBlock2)->debugEntries; \
	}

/* Macro to append some debugging information onto a block, given the instruction
   offset of the debugging information already in the destination block */
#define APPEND_DEBUG(psFinal, baseOffset, psBlock) \
	if (genDebugInfo) \
	{ \
		for(_dbEntry = 0; _dbEntry < (psBlock)->debugEntries; _dbEntry++) \
		{ \
			_psCurr = (psFinal)->psDebug + (psFinal)->debugEntries + _dbEntry; \
			_psCurr->line = (psBlock)->psDebug[_dbEntry].line; \
			_psCurr->offset = (psBlock)->psDebug[_dbEntry].offset + (baseOffset); \
		} \
		(psFinal)->debugEntries = (UWORD)((psFinal)->debugEntries + (psBlock)->debugEntries); \
	}


/* Macro to store a label in the debug info */
#define DEBUG_LABEL(psBlock, offset, pString) \
	if (genDebugInfo) \
	{ \
		(psBlock)->psDebug[offset].pLabel = (char *)malloc(strlen(pString)+1); \
		if (!(psBlock)->psDebug[offset].pLabel) \
		{ \
			scr_error("Out of memory"); \
			ALLOC_ERROR_ACTION; \
		} \
		strcpy((psBlock)->psDebug[offset].pLabel, pString); \
	}


/***************************************************************************************
 *
 * Code generation functions
 *
 * These functions are used within rule bodies to generate code.
 */

/* Macro to deal with the errors returned by code generation functions.
 * Used within the rule body.
 */
#define CHECK_CODE_ERROR(error) \
	if ((error) == CE_MEMORY) \
	{ \
		YYABORT; \
	} \
	else if ((error) == CE_PARSE) \
	{ \
		YYERROR; \
	}

void script_debug(const char *pFormat, ...)
{
	char    buffer[500];
	va_list pArgs;

	va_start(pArgs, pFormat);
	vsnprintf(buffer, sizeof(buffer), pFormat, pArgs);
	va_end(pArgs);

	debug(LOG_SCRIPT, "%s", buffer);
}

/* Generate the code for a function call, checking the parameter
 * types match.
 */
static CODE_ERROR scriptCodeFunction(FUNC_SYMBOL	*psFSymbol,		// The function being called
							PARAM_BLOCK		*psPBlock,		// The functions parameters
							int32_t			expContext,		// Whether the function is being
															// called in an expression context
							CODE_BLOCK		**ppsCBlock)	// The generated code block
{
	UDWORD		size, i;
	INTERP_VAL	*ip;
	bool		typeError = false;
	char		aErrorString[255];
	INTERP_TYPE	type1,type2;

	ASSERT( psFSymbol != NULL, "ais_CodeFunction: Invalid function symbol pointer" );
	ASSERT( psPBlock != NULL,
		"scriptCodeFunction: Invalid param block pointer" );
	ASSERT( (psPBlock->size == 0) || psPBlock->pCode != NULL,
		"scriptCodeFunction: Invalid parameter code pointer" );
	ASSERT( ppsCBlock != NULL,
		 "scriptCodeFunction: Invalid generated code block pointer" );

	/* Check the parameter types match what the function needs */
	for(i=0; (i<psFSymbol->numParams) && (i<psPBlock->numParams); i++)
	{
/*		if (psFSymbol->aParams[i] != VAL_VOID &&
			psFSymbol->aParams[i] != psPBlock->aParams[i])*/
		//TODO: string support
		if(psFSymbol->aParams[i] != VAL_STRING)	// string - allow mixed types if string is parameter type	//TODO: tweak this
		{
			type1 = (INTERP_TYPE)psFSymbol->aParams[i];
			type2 = (INTERP_TYPE)psPBlock->aParams[i];
			if (!interpCheckEquiv(type1, type2))
			{
				debug(LOG_ERROR, "scriptCodeFunction: Type mismatch for parameter %d (%d/%d)", i, psFSymbol->aParams[i], psPBlock->aParams[i]);
				snprintf(aErrorString, sizeof(aErrorString), "Type mismatch for parameter %d", i);
				scr_error("%s", aErrorString);
				typeError = true;
			}
		}
		//else
		//{
		//	debug(LOG_SCRIPT, "scriptCodeFunction: %s takes string as parameter %d (provided: %d)", psFSymbol->pIdent, i, psPBlock->aParams[i]);
		//}
	}

	/* Check the number of parameters matches that expected */
	if (psFSymbol->numParams != psPBlock->numParams)
	{
		snprintf(aErrorString, sizeof(aErrorString), "Expected %d parameters", psFSymbol->numParams);
		scr_error("%s", aErrorString);
		*ppsCBlock = NULL;
		return CE_PARSE;
	}

	if (typeError)
	{
		/* Report the error here so all the */
		/* type mismatches are reported */
		*ppsCBlock = NULL;
		return CE_PARSE;
	}

	//size = psPBlock->size + sizeof(OPCODE) + sizeof(SCRIPT_FUNC);
	size = psPBlock->size + 1 + 1;	//size + opcode + sizeof(SCRIPT_FUNC)

	if (!expContext && (psFSymbol->type != VAL_VOID))
	{
		//size += sizeof(OPCODE);
		size += 1;	//+ 1 additional opcode to pop value if needed
	}

	ALLOC_BLOCK(*ppsCBlock, size);
	ip = (*ppsCBlock)->pCode;
	(*ppsCBlock)->type = psFSymbol->type;

	/* Copy in the code for the parameters */
	PUT_BLOCK(ip, psPBlock);
	FREE_PBLOCK(psPBlock);

	/* Make the function call */
	if (psFSymbol->script)
	{
		/* function defined in this script */
//		PUT_OPCODE(ip, OP_FUNC);
//		PUT_SCRIPTFUNC(ip, psFSymbol);
		ASSERT(false, "wrong function type call");
	}
	else
	{
		/* call an instinct function */
		PUT_OPCODE(ip, OP_CALL);
		PUT_FUNC_EXTERN(ip, psFSymbol->pFunc);
	}

	if (!expContext && (psFSymbol->type != VAL_VOID))
	{
		/* Clear the return value from the stack */
		PUT_OPCODE(ip, OP_POP);
	}

	return CE_OK;
}


/* Function call: Check the parameter types match, assumes param count matched */
static UDWORD checkFuncParamTypes(EVENT_SYMBOL		*psFSymbol,		// The function being called
							PARAM_BLOCK		*psPBlock)	// The generated code block
{
	UDWORD		i;

	//debug(LOG_SCRIPT,"checkFuncParamTypes");

	/* Check the parameter types match what the function needs */
	for(i=0; (i<psFSymbol->numParams) && (i<psPBlock->numParams); i++)
	{
		//TODO: string support
		//if(psFSymbol->aParams[i] != VAL_STRING)	// string - allow mixed types if string is parameter type
		//{
			if (!interpCheckEquiv(psFSymbol->aParams[i], psPBlock->aParams[i]))
			{
				debug(LOG_ERROR, "checkFuncParamTypes: Type mismatch for parameter %d ('1' based) in Function '%s' (provided type: %d, expected: %d)", (i+1), psFSymbol->pIdent, psPBlock->aParams[i], psFSymbol->aParams[i]);
				scr_error("Parameter type mismatch");
				return i+1;
			}
		//}
	}

	//debug(LOG_SCRIPT,"END checkFuncParamTypes");

	return 0;	//all ok
}

/* Generate the code for a parameter callback, checking the parameter
 * types match.
 */
static CODE_ERROR scriptCodeCallbackParams(
							CALLBACK_SYMBOL	*psCBSymbol,	// The callback being called
							PARAM_BLOCK		*psPBlock,		// The callbacks parameters
							TRIGGER_DECL	**ppsTDecl)		// The generated code block
{
	UDWORD		size, i;
	INTERP_VAL	*ip;
	bool		typeError = false;
	char		aErrorString[255];

	ASSERT( psPBlock != NULL,
		"scriptCodeCallbackParams: Invalid param block pointer" );
	ASSERT( (psPBlock->size == 0) || psPBlock->pCode != NULL,
		"scriptCodeCallbackParams: Invalid parameter code pointer" );
	ASSERT( ppsTDecl != NULL,
		 "scriptCodeCallbackParams: Invalid generated code block pointer" );
	ASSERT( psCBSymbol->pFunc != NULL,
		 "scriptCodeCallbackParams: Expected function pointer for callback symbol" );

	/* Check the parameter types match what the function needs */
	for(i=0; (i<psCBSymbol->numParams) && (i<psPBlock->numParams); i++)
	{
		if (!interpCheckEquiv((INTERP_TYPE)psCBSymbol->aParams[i], psPBlock->aParams[i]))
		{
			snprintf(aErrorString, sizeof(aErrorString), "Type mismatch for parameter %d", i);
			scr_error("%s", aErrorString);
			typeError = true;
		}
	}

	/* Check the number of parameters matches that expected */
	if (psPBlock->numParams == 0)
	{
		scr_error("Expected parameters to callback");
		*ppsTDecl = NULL;
		return CE_PARSE;
	}
	else if (psCBSymbol->numParams != psPBlock->numParams)
	{
		snprintf(aErrorString, sizeof(aErrorString), "Expected %d parameters", psCBSymbol->numParams);
		scr_error("%s", aErrorString);
		*ppsTDecl = NULL;
		return CE_PARSE;
	}
	if (typeError)
	{
		/* Return the error here so all the */
		/* type mismatches are reported */
		*ppsTDecl = NULL;
		return CE_PARSE;
	}

	//size = psPBlock->size + sizeof(OPCODE) + sizeof(SCRIPT_FUNC);
	size = psPBlock->size + 1 + 1;		//size + opcode + SCRIPT_FUNC

	ALLOC_TSUBDECL(*ppsTDecl, psCBSymbol->type, size, 0);
	ip = (*ppsTDecl)->pCode;

	/* Copy in the code for the parameters */
	PUT_BLOCK(ip, psPBlock);
	FREE_PBLOCK(psPBlock);

	/* call the instinct function */
	PUT_OPCODE(ip, OP_CALL);
	PUT_FUNC_EXTERN(ip, psCBSymbol->pFunc);

	return CE_OK;
}


/* Generate code for assigning a value to a variable */
static CODE_ERROR scriptCodeAssignment(VAR_SYMBOL	*psVariable,	// The variable to assign to
							  CODE_BLOCK	*psValue,		// The code for the value to
															// assign
							  CODE_BLOCK	**ppsBlock)		// Generated code
{
	SDWORD		size;

	ASSERT( psVariable != NULL,
		"scriptCodeAssignment: Invalid variable symbol pointer" );
	ASSERT( psValue != NULL,
		"scriptCodeAssignment: Invalid value code block pointer" );
	ASSERT( psValue->pCode != NULL,
		"scriptCodeAssignment: Invalid value code pointer" );
	ASSERT( ppsBlock != NULL,
		"scriptCodeAssignment: Invalid generated code block pointer" );

	//size = psValue->size + sizeof(OPCODE);
	size = psValue->size + 1;		//1 - for assignment opcode

	if (psVariable->storage == ST_EXTERN)
	{
		// Check there is a set function
		if (psVariable->set == NULL)
		{
			scr_error("No set function for external variable");
			return CE_PARSE;
		}

		//size += sizeof(SCRIPT_VARFUNC);
		size += 1;		//1 - for set func pointer
	}
	ALLOC_BLOCK(*ppsBlock, size);
	ip = (*ppsBlock)->pCode;

	/* Copy in the code for the expression */
	PUT_BLOCK(ip, psValue);
	FREE_BLOCK(psValue);

	/* Code to get the value from the stack into the variable */
	switch (psVariable->storage)
	{
	case ST_PUBLIC:
	case ST_PRIVATE:
		PUT_PKOPCODE(ip, OP_POPGLOBAL, psVariable->index);
		break;
	case ST_LOCAL:
		PUT_PKOPCODE(ip, OP_POPLOCAL, psVariable->index);
		break;
	case ST_EXTERN:
		PUT_PKOPCODE(ip, OP_VARCALL, psVariable->index);
		PUT_VARFUNC(ip, psVariable->set);
		break;
	case ST_OBJECT:
		scr_error("Cannot use member variables in this context");
		return CE_PARSE;
		break;
	default:
		scr_error("Unknown storage type");
		return CE_PARSE;
		break;
	}

	return CE_OK;
}


/* Generate code for assigning a value to an object variable */
static CODE_ERROR scriptCodeObjAssignment(OBJVAR_BLOCK	*psVariable,// The variable to assign to
								 CODE_BLOCK		*psValue,	// The code for the value to
															// assign
								 CODE_BLOCK		**ppsBlock)	// Generated code
{
	ASSERT( psVariable != NULL,
		"scriptCodeObjAssignment: Invalid object variable block pointer" );
	ASSERT( psVariable->pCode != NULL,
		"scriptCodeObjAssignment: Invalid object variable code pointer" );
	ASSERT( psVariable->psObjVar != NULL,
		"scriptCodeObjAssignment: Invalid object variable symbol pointer" );
	ASSERT( psValue != NULL,
		"scriptCodeObjAssignment: Invalid value code block pointer" );
	ASSERT( psValue->pCode != NULL,
		"scriptCodeObjAssignment: Invalid value code pointer" );
	ASSERT( ppsBlock != NULL,
		"scriptCodeObjAssignment: Invalid generated code block pointer" );

	// Check there is an access function for the variable
	if (psVariable->psObjVar->set == NULL)
	{
		scr_error("No set function for object variable");
		return CE_PARSE;
	}

	//ALLOC_BLOCK(*ppsBlock, psVariable->size + psValue->size + sizeof(OPCODE) + sizeof(SCRIPT_VARFUNC));
	ALLOC_BLOCK(*ppsBlock, psVariable->size + psValue->size + 1 + 1);	//size + size + opcode + 'sizeof(SCRIPT_VARFUNC)'

	ip = (*ppsBlock)->pCode;

	/* Copy in the code for the value */
	PUT_BLOCK(ip, psValue);
	FREE_BLOCK(psValue);

	/* Copy in the code for the object */
	PUT_BLOCK(ip, psVariable);

	/* Code to get the value from the stack into the variable */
	PUT_PKOPCODE(ip, OP_VARCALL, psVariable->psObjVar->index);
	PUT_VARFUNC(ip, (psVariable->psObjVar->set));

	/* Free the variable block */
	FREE_OBJVARBLOCK(psVariable);

	return CE_OK;
}


/* Generate code for getting a value from an object variable */
static CODE_ERROR scriptCodeObjGet(OBJVAR_BLOCK	*psVariable,// The variable to get from
						  CODE_BLOCK	**ppsBlock)	// Generated code
{
	ASSERT( psVariable != NULL,
		"scriptCodeObjAssignment: Invalid object variable block pointer" );
	ASSERT( psVariable->pCode != NULL,
		"scriptCodeObjAssignment: Invalid object variable code pointer" );
	ASSERT( psVariable->psObjVar != NULL,
		"scriptCodeObjAssignment: Invalid object variable symbol pointer" );
	ASSERT( ppsBlock != NULL,
		"scriptCodeObjAssignment: Invalid generated code block pointer" );

	// Check there is an access function for the variable
	if (psVariable->psObjVar->get == NULL)
	{
		scr_error("No get function for object variable");
		return CE_PARSE;
	}

	//ALLOC_BLOCK(*ppsBlock, psVariable->size + sizeof(OPCODE) + sizeof(SCRIPT_VARFUNC));
	ALLOC_BLOCK(*ppsBlock, psVariable->size + 1 + 1);	//size + opcode + SCRIPT_VARFUNC

	ip = (*ppsBlock)->pCode;

	/* Copy in the code for the object */
	PUT_BLOCK(ip, psVariable);
	(*ppsBlock)->type = psVariable->psObjVar->type;

	/* Code to get the value from the object onto the stack */
	PUT_PKOPCODE(ip, OP_VARCALL, psVariable->psObjVar->index);
	PUT_VARFUNC(ip, psVariable->psObjVar->get);

	/* Free the variable block */
	FREE_OBJVARBLOCK(psVariable);

	return CE_OK;
}


/* Generate code for assigning a value to an array variable */
static CODE_ERROR scriptCodeArrayAssignment(ARRAY_BLOCK	*psVariable,// The variable to assign to
								 CODE_BLOCK		*psValue,	// The code for the value to
															// assign
								 CODE_BLOCK		**ppsBlock)	// Generated code
{
	ASSERT( psVariable != NULL,
		"scriptCodeObjAssignment: Invalid object variable block pointer" );
	ASSERT( psVariable->pCode != NULL,
		"scriptCodeObjAssignment: Invalid object variable code pointer" );
	ASSERT( psVariable->psArrayVar != NULL,
		"scriptCodeObjAssignment: Invalid object variable symbol pointer" );
	ASSERT( psValue != NULL,
		"scriptCodeObjAssignment: Invalid value code block pointer" );
	ASSERT( psValue->pCode != NULL,
		"scriptCodeObjAssignment: Invalid value code pointer" );
	ASSERT( ppsBlock != NULL,
		"scriptCodeObjAssignment: Invalid generated code block pointer" );

	// Check this is an array
	if (psVariable->psArrayVar->dimensions == 0)
	{
		scr_error("Not an array variable");
		return CE_PARSE;
	}

	ALLOC_BLOCK(*ppsBlock, psVariable->size + psValue->size + 1);	//size + size + opcode

	ip = (*ppsBlock)->pCode;

	/* Copy in the code for the value */
	PUT_BLOCK(ip, psValue);
	FREE_BLOCK(psValue);

	/* Copy in the code for the array index */
	PUT_BLOCK(ip, psVariable);

	/* Code to get the value from the stack into the variable */
	PUT_PKOPCODE(ip, OP_POPARRAYGLOBAL,
		((psVariable->psArrayVar->dimensions << ARRAY_DIMENSION_SHIFT) & ARRAY_DIMENSION_MASK) |
		(psVariable->psArrayVar->index & ARRAY_BASE_MASK) );

	/* Free the variable block */
	FREE_ARRAYBLOCK(psVariable);

	return CE_OK;
}


/* Generate code for getting a value from an array variable */
static CODE_ERROR scriptCodeArrayGet(ARRAY_BLOCK	*psVariable,// The variable to get from
						  CODE_BLOCK	**ppsBlock)	// Generated code
{
	ASSERT( psVariable != NULL,
		"scriptCodeObjAssignment: Invalid object variable block pointer" );
	ASSERT( psVariable->pCode != NULL,
		"scriptCodeObjAssignment: Invalid object variable code pointer" );
	ASSERT( psVariable->psArrayVar != NULL,
		"scriptCodeObjAssignment: Invalid object variable symbol pointer" );
	ASSERT( ppsBlock != NULL,
		"scriptCodeObjAssignment: Invalid generated code block pointer" );

	// Check this is an array
	if (psVariable->psArrayVar->dimensions == 0)
	{
		scr_error("Not an array variable");
		return CE_PARSE;
	}

	ALLOC_BLOCK(*ppsBlock, psVariable->size + 1);	//size + opcode

	ip = (*ppsBlock)->pCode;

	/* Copy in the code for the array index */
	PUT_BLOCK(ip, psVariable);
	(*ppsBlock)->type = psVariable->psArrayVar->type;

	/* Code to get the value from the array onto the stack */
	PUT_PKOPCODE(ip, OP_PUSHARRAYGLOBAL,
		((psVariable->psArrayVar->dimensions << ARRAY_DIMENSION_SHIFT) & ARRAY_DIMENSION_MASK) |
		(psVariable->psArrayVar->index & ARRAY_BASE_MASK) );

	/* Free the variable block */
	FREE_ARRAYBLOCK(psVariable);

	return CE_OK;
}


/* Generate the final code block for conditional statements */
static CODE_ERROR scriptCodeConditional(
					COND_BLOCK *psCondBlock,	// The intermediate conditional code
					CODE_BLOCK **ppsBlock)		// The final conditional code
{
	UDWORD		i;

	ASSERT( psCondBlock != NULL,
		"scriptCodeConditional: Invalid conditional code block pointer" );
	ASSERT( psCondBlock->pCode != NULL,
		"scriptCodeConditional: Invalid conditional code pointer" );
	ASSERT( ppsBlock != NULL,
		"scriptCodeConditional: Invalid generated code block pointer" );

	/* Allocate the final block */
	ALLOC_BLOCK(*ppsBlock, psCondBlock->size);
	ALLOC_DEBUG(*ppsBlock, psCondBlock->debugEntries);
	ip = (*ppsBlock)->pCode;

	/* Copy the code over */
	PUT_BLOCK(ip, psCondBlock);

	/* Copy the debugging information */
	PUT_DEBUG(*ppsBlock, psCondBlock);

	/* Now set the offsets of jumps in the conditional to the correct value */
	for(i = 0; i < psCondBlock->numOffsets; i++)
	{
		ip = (*ppsBlock)->pCode + psCondBlock->aOffsets[i];
		// *ip = ((*ppsBlock)->size / sizeof(UDWORD)) - (ip - (*ppsBlock)->pCode);

		ip->type = VAL_PKOPCODE;
		ip->v.ival = (*ppsBlock)->size - (ip - (*ppsBlock)->pCode);
		ip->v.ival = (OP_JUMP << OPCODE_SHIFT) | ( (ip->v.ival) & OPCODE_DATAMASK );
	}

	/* Free the original code */
	FREE_DEBUG(psCondBlock);
	FREE_CONDBLOCK(psCondBlock);

	return CE_OK;
}

/* Generate code for function parameters */
static CODE_ERROR scriptCodeParameter(CODE_BLOCK		*psParam,		// Code for the parameter
							 INTERP_TYPE		type,			// Parameter type
							 PARAM_BLOCK	**ppsBlock)		// Generated code
{
	ASSERT( psParam != NULL,
		"scriptCodeParameter: Invalid parameter code block pointer" );
	ASSERT( psParam->pCode != NULL,
		"scriptCodeParameter: Invalid parameter code pointer" );
	ASSERT( ppsBlock != NULL,
		"scriptCodeParameter: Invalid generated code block pointer" );

	RULE("	scriptCodeParameter(): type: %d", type);

	ALLOC_PBLOCK(*ppsBlock, psParam->size, 1);
	ip = (*ppsBlock)->pCode;

	/* Copy in the code for the parameter */
	PUT_BLOCK(ip, psParam);
	FREE_BLOCK(psParam);

	(*ppsBlock)->aParams[0] = type;

	return CE_OK;
}


/* Generate code for binary operators (e.g. 2 + 2) */
static CODE_ERROR scriptCodeBinaryOperator(CODE_BLOCK	*psFirst,	// Code for first parameter
								  CODE_BLOCK	*psSecond,	// Code for second parameter
								  OPCODE		opcode,		// Operator function
								  CODE_BLOCK	**ppsBlock) // Generated code
{
	ALLOC_BLOCK(*ppsBlock, psFirst->size + psSecond->size + 1);		//size + size + binary opcode
	ip = (*ppsBlock)->pCode;

	/* Copy the already generated bits of code into the code block */
	PUT_BLOCK(ip, psFirst);
	PUT_BLOCK(ip, psSecond);

	/* Now put an add operator into the code */
	PUT_PKOPCODE(ip, OP_BINARYOP, opcode);

	/* Free the two code blocks that have been copied */
	FREE_BLOCK(psFirst);
	FREE_BLOCK(psSecond);

	return CE_OK;
}

/* check if the arguments in the function definition body match the argument types
and names from function declaration (if there was any) */
static bool checkFuncParamType(UDWORD argIndex, UDWORD argType)
{
	VAR_SYMBOL		*psCurr;
	SDWORD			i,j;

	if(psCurEvent == NULL)
	{
		debug(LOG_ERROR, "checkFuncParamType() - psCurEvent == NULL");
		return false;
	}

	if(argIndex < psCurEvent->numParams)
	{
		/* find the argument by the index */
		i=psCurEvent->index;
		j=0;
		for(psCurr = psLocalVarsB[i]; psCurr != NULL; psCurr = psCurr->psNext)
		{
			if((psCurEvent->numParams - j - 1)==argIndex)	/* got to the right argument */
			{
				if(argType != psCurr->type)
				{
					debug(LOG_ERROR, "Argument type with index %d in event '%s' doesn't match function declaration (%d/%d)",argIndex,psCurEvent->pIdent,argType,psCurr->type);
					return false;
				}
				else
				{
					//debug(LOG_SCRIPT, "arg matched ");
					return true;
				}
			}
			j++;
		}
	}
	else
	{
		debug(LOG_ERROR, "checkFuncParamType() - argument %d has wrong argument index, event: '%s'", argIndex, psCurEvent->pIdent);
		return false;
	}

	return false;
}


/* Generate code for accessing an object variable.  The variable symbol is
 * stored with the code for the object value so that this block can be used for
 * both setting and retrieving an object value.
 */
static CODE_ERROR scriptCodeObjectVariable(CODE_BLOCK	*psObjCode,	// Code for the object value
								  VAR_SYMBOL	*psVar,		// The object variable symbol
								  OBJVAR_BLOCK	**ppsBlock) // Generated code
{
	ASSERT( psObjCode != NULL,
		"scriptCodeObjectVariable: Invalid object code block pointer" );
	ASSERT( psObjCode->pCode != NULL,
		"scriptCodeObjectVariable: Invalid object code pointer" );
	ASSERT( psVar != NULL,
		"scriptCodeObjectVariable: Invalid variable symbol pointer" );
	ASSERT( ppsBlock != NULL,
		"scriptCodeObjectVariable: Invalid generated code block pointer" );

	ALLOC_OBJVARBLOCK(*ppsBlock, psObjCode->size, psVar);
	ip = (*ppsBlock)->pCode;

	/* Copy the already generated bit of code into the code block */
	PUT_BLOCK(ip, psObjCode);
	FREE_BLOCK(psObjCode);

	/* Check the variable is the correct type */
	if (psVar->storage != ST_OBJECT)
	{
		scr_error("Only object variables are valid in this context");
		return CE_PARSE;
	}

	return CE_OK;
}


/* Generate code for accessing an array variable.  The variable symbol is
 * stored with the code for the object value so that this block can be used for
 * both setting and retrieving an array value.
 */
static CODE_ERROR scriptCodeArrayVariable(ARRAY_BLOCK	*psArrayCode,	// Code for the array index
								  VAR_SYMBOL	*psVar,			// The array variable symbol
								  ARRAY_BLOCK	**ppsBlock)		// Generated code
{
	ASSERT( psArrayCode != NULL,
		"scriptCodeObjectVariable: Invalid object code block pointer" );
	ASSERT( psArrayCode->pCode != NULL,
		"scriptCodeObjectVariable: Invalid object code pointer" );
	ASSERT( psVar != NULL,
		"scriptCodeObjectVariable: Invalid variable symbol pointer" );
	ASSERT( ppsBlock != NULL,
		"scriptCodeObjectVariable: Invalid generated code block pointer" );

	// Check the variable is the correct type
	if (psVar->dimensions != psArrayCode->dimensions)
	{
		scr_error("Invalid number of array dimensions for this variable");
		return CE_PARSE;
	}

	psArrayCode->psArrayVar = psVar;
	*ppsBlock = psArrayCode;

	return CE_OK;
}


/* Generate code for a constant */
static CODE_ERROR scriptCodeConstant(CONST_SYMBOL	*psConst,	// The object variable symbol
							CODE_BLOCK		**ppsBlock)	// Generated code
{
	ASSERT( psConst != NULL,
		"scriptCodeConstant: Invalid constant symbol pointer" );
	ASSERT( ppsBlock != NULL,
		"scriptCodeConstant: Invalid generated code block pointer" );

	ALLOC_BLOCK(*ppsBlock, 1 + 1);		//OP_PUSH opcode + variable value

	ip = (*ppsBlock)->pCode;
	(*ppsBlock)->type = psConst->type;

	/* Put the value onto the stack */
	switch (psConst->type)
	{
	case VAL_FLOAT:
		RULE("	scriptCodeConstant: pushing float");
		PUT_PKOPCODE(ip, OP_PUSH, VAL_FLOAT);
		PUT_DATA_FLOAT(ip, psConst->fval);
		break;
	case VAL_BOOL:
		RULE("	scriptCodeConstant: pushing bool");
		PUT_PKOPCODE(ip, OP_PUSH, VAL_BOOL);
		PUT_DATA_BOOL(ip, psConst->bval);
		break;
	case VAL_INT:
		RULE("	scriptCodeConstant: pushing int");
		PUT_PKOPCODE(ip, OP_PUSH, VAL_INT);
		PUT_DATA_INT(ip, psConst->ival);
		break;
	case VAL_STRING:
		RULE("	scriptCodeConstant: pushing string");
		PUT_PKOPCODE(ip, OP_PUSH, VAL_STRING);
		PUT_DATA_STRING(ip, psConst->sval);
		break;
	default:	/* Object/user constants */
		RULE("	scriptCodeConstant: pushing object/ user var (type: %d)", psConst->type);
		PUT_PKOPCODE(ip, OP_PUSH, psConst->type);
		PUT_OBJECT_CONST(ip, psConst->oval, psConst->type);		//TODO: differentiate types
		break;
	}

	return CE_OK;
}


/* Generate code for getting a variables value */
static CODE_ERROR scriptCodeVarGet(VAR_SYMBOL		*psVariable,	// The object variable symbol
							CODE_BLOCK		**ppsBlock)		// Generated code
{
	SDWORD size = 1; //1 - for opcode

	if (psVariable->storage == ST_EXTERN)
	{
		// Check there is a set function
		if (psVariable->get == NULL)
		{
			scr_error("No get function for external variable");
			return CE_PARSE;
		}

		size++;
	}

	ALLOC_BLOCK(*ppsBlock, size);
	ip = (*ppsBlock)->pCode;
	(*ppsBlock)->type = psVariable->type;

	/* Code to get the value onto the stack */
	switch (psVariable->storage)
	{
	case ST_PUBLIC:
	case ST_PRIVATE:
		PUT_PKOPCODE(ip, OP_PUSHGLOBAL, psVariable->index);
		break;
	case ST_LOCAL:
		PUT_PKOPCODE(ip, OP_PUSHLOCAL, psVariable->index);	//opcode + event index
		break;
	case ST_EXTERN:
		PUT_PKOPCODE(ip, OP_VARCALL, psVariable->index);
		PUT_VARFUNC(ip, psVariable->get);
		break;
	case ST_OBJECT:
		scr_error("Cannot use member variables in this context");
		return CE_PARSE;
		break;
	default:
		scr_error("Unknown storage type");
		return CE_PARSE;
		break;
	}

	return CE_OK;
}

/* Code Increment/Decrement operators */
static CODE_ERROR scriptCodeIncDec(VAR_SYMBOL		*psVariable,	// The object variable symbol
							CODE_BLOCK		**ppsBlock, OPCODE op_dec_inc)
{

	ALLOC_BLOCK(*ppsBlock, 1 + 1 + 1);	//OP_PUSHREF opcode + variable index + inc opcode
	ip = (*ppsBlock)->pCode;
	(*ppsBlock)->type = psVariable->type;

	/* Put variable index */
	switch (psVariable->storage)
	{
	case ST_PRIVATE:
		PUT_PKOPCODE(ip, OP_PUSHREF, psVariable->type | VAL_REF);
		PUT_DATA_INT(ip, psVariable->index);
		break;
	case ST_LOCAL:
		PUT_PKOPCODE(ip, OP_PUSHLOCALREF, psVariable->type | VAL_REF);
		PUT_DATA_INT(ip, psVariable->index);
		break;
	default:
		scr_error("Wrong variable storage type for increment/decrement operator");
		break;
	}

	/* Put inc/dec opcode */
	PUT_PKOPCODE(ip, OP_UNARYOP, op_dec_inc);

	return CE_OK;
}


/* Generate code for getting a variables value */
static CODE_ERROR scriptCodeVarRef(VAR_SYMBOL		*psVariable,	// The object variable symbol
							PARAM_BLOCK		**ppsBlock)		// Generated code
{
	SDWORD	size;

	//size = sizeof(OPCODE) + sizeof(SDWORD);
	size = 1 + 1;	//OP_PUSHREF opcode + variable index

	ALLOC_PBLOCK(*ppsBlock, size, 1);
	ip = (*ppsBlock)->pCode;

	(*ppsBlock)->aParams[0] = (INTERP_TYPE)(psVariable->type | VAL_REF);

	/* Code to get the value onto the stack */
	switch (psVariable->storage)
	{
	case ST_PUBLIC:
	case ST_PRIVATE:

		PUT_PKOPCODE(ip, OP_PUSHREF, (*ppsBlock)->aParams[0]);
		PUT_DATA_INT(ip, psVariable->index);	//TODO: add new macro
		break;

	case ST_LOCAL:
		PUT_PKOPCODE(ip, OP_PUSHLOCALREF, (*ppsBlock)->aParams[0]);
		PUT_DATA_INT(ip, psVariable->index);
		break;
	case ST_EXTERN:
		scr_error("Cannot use external variables in this context");
		return CE_PARSE;
		break;
	case ST_OBJECT:
		scr_error("Cannot use member variables in this context");
		return CE_PARSE;
		break;
	default:
		scr_error("Unknown storage type: %d", psVariable->storage);
		return CE_PARSE;
		break;
	}

	return CE_OK;
}

/* Change the error action for the ALLOC macro's to what it
 * should be inside a rule body.
 *
 * NOTE: DO NOT USE THE ALLOC MACRO'S INSIDE ANY FUNCTIONS
 *       ONCE ALLOC_ERROR_ACTION IS SET TO THIS VALUE.
 *       ALL FUNCTIONS THAT USE THESE MACROS MUST BE PLACED
 *       BEFORE THIS #define.
 */
#undef ALLOC_ERROR_ACTION
#define ALLOC_ERROR_ACTION YYABORT



/* Line 268 of yacc.c  */
#line 1616 "script_parser.cpp"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     lexFUNCTION = 258,
     TRIGGER = 259,
     EVENT = 260,
     WAIT = 261,
     EVERY = 262,
     INACTIVE = 263,
     INITIALISE = 264,
     LINK = 265,
     REF = 266,
     RET = 267,
     _VOID = 268,
     WHILE = 269,
     IF = 270,
     ELSE = 271,
     EXIT = 272,
     PAUSE = 273,
     BOOLEQUAL = 274,
     NOTEQUAL = 275,
     GREATEQUAL = 276,
     LESSEQUAL = 277,
     GREATER = 278,
     LESS = 279,
     _AND = 280,
     _OR = 281,
     _NOT = 282,
     _INC = 283,
     _DEC = 284,
     TO_INT_CAST = 285,
     TO_FLOAT_CAST = 286,
     UMINUS = 287,
     BOOLEAN_T = 288,
     FLOAT_T = 289,
     INTEGER = 290,
     QTEXT = 291,
     TYPE = 292,
     STORAGE = 293,
     IDENT = 294,
     VAR = 295,
     BOOL_VAR = 296,
     NUM_VAR = 297,
     FLOAT_VAR = 298,
     OBJ_VAR = 299,
     STRING_VAR = 300,
     VAR_ARRAY = 301,
     BOOL_ARRAY = 302,
     NUM_ARRAY = 303,
     FLOAT_ARRAY = 304,
     OBJ_ARRAY = 305,
     BOOL_OBJVAR = 306,
     NUM_OBJVAR = 307,
     USER_OBJVAR = 308,
     OBJ_OBJVAR = 309,
     BOOL_CONSTANT = 310,
     NUM_CONSTANT = 311,
     USER_CONSTANT = 312,
     OBJ_CONSTANT = 313,
     STRING_CONSTANT = 314,
     FUNC = 315,
     BOOL_FUNC = 316,
     NUM_FUNC = 317,
     FLOAT_FUNC = 318,
     USER_FUNC = 319,
     OBJ_FUNC = 320,
     STRING_FUNC = 321,
     VOID_FUNC_CUST = 322,
     BOOL_FUNC_CUST = 323,
     NUM_FUNC_CUST = 324,
     FLOAT_FUNC_CUST = 325,
     USER_FUNC_CUST = 326,
     OBJ_FUNC_CUST = 327,
     STRING_FUNC_CUST = 328,
     TRIG_SYM = 329,
     EVENT_SYM = 330,
     CALLBACK_SYM = 331
   };
#endif
/* Tokens.  */
#define lexFUNCTION 258
#define TRIGGER 259
#define EVENT 260
#define WAIT 261
#define EVERY 262
#define INACTIVE 263
#define INITIALISE 264
#define LINK 265
#define REF 266
#define RET 267
#define _VOID 268
#define WHILE 269
#define IF 270
#define ELSE 271
#define EXIT 272
#define PAUSE 273
#define BOOLEQUAL 274
#define NOTEQUAL 275
#define GREATEQUAL 276
#define LESSEQUAL 277
#define GREATER 278
#define LESS 279
#define _AND 280
#define _OR 281
#define _NOT 282
#define _INC 283
#define _DEC 284
#define TO_INT_CAST 285
#define TO_FLOAT_CAST 286
#define UMINUS 287
#define BOOLEAN_T 288
#define FLOAT_T 289
#define INTEGER 290
#define QTEXT 291
#define TYPE 292
#define STORAGE 293
#define IDENT 294
#define VAR 295
#define BOOL_VAR 296
#define NUM_VAR 297
#define FLOAT_VAR 298
#define OBJ_VAR 299
#define STRING_VAR 300
#define VAR_ARRAY 301
#define BOOL_ARRAY 302
#define NUM_ARRAY 303
#define FLOAT_ARRAY 304
#define OBJ_ARRAY 305
#define BOOL_OBJVAR 306
#define NUM_OBJVAR 307
#define USER_OBJVAR 308
#define OBJ_OBJVAR 309
#define BOOL_CONSTANT 310
#define NUM_CONSTANT 311
#define USER_CONSTANT 312
#define OBJ_CONSTANT 313
#define STRING_CONSTANT 314
#define FUNC 315
#define BOOL_FUNC 316
#define NUM_FUNC 317
#define FLOAT_FUNC 318
#define USER_FUNC 319
#define OBJ_FUNC 320
#define STRING_FUNC 321
#define VOID_FUNC_CUST 322
#define BOOL_FUNC_CUST 323
#define NUM_FUNC_CUST 324
#define FLOAT_FUNC_CUST 325
#define USER_FUNC_CUST 326
#define OBJ_FUNC_CUST 327
#define STRING_FUNC_CUST 328
#define TRIG_SYM 329
#define EVENT_SYM 330
#define CALLBACK_SYM 331




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 293 of yacc.c  */
#line 1559 "script_parser.ypp"

	/* Types returned by the lexer */
	int32_t			bval;
	float			fval;
	SDWORD			ival;
	char			*sval;
	INTERP_TYPE		tval;
	STORAGE_TYPE	stype;
	VAR_SYMBOL		*vSymbol;
	CONST_SYMBOL	*cSymbol;
	FUNC_SYMBOL		*fSymbol;
	TRIGGER_SYMBOL	*tSymbol;
	EVENT_SYMBOL	*eSymbol;
	CALLBACK_SYMBOL	*cbSymbol;

	/* Types only returned by rules */
	CODE_BLOCK		*cblock;
	COND_BLOCK		*condBlock;
	OBJVAR_BLOCK	*objVarBlock;
	ARRAY_BLOCK		*arrayBlock;
	PARAM_BLOCK		*pblock;
	PARAM_DECL		*pdecl;
	TRIGGER_DECL	*tdecl;
	UDWORD			integer_val;
	VAR_DECL		*vdecl;
	VAR_IDENT_DECL	*videcl;



/* Line 293 of yacc.c  */
#line 1834 "script_parser.cpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 343 of yacc.c  */
#line 1846 "script_parser.cpp"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  11
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2212

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  92
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  66
/* YYNRULES -- Number of rules.  */
#define YYNRULES  249
/* YYNRULES -- Number of states.  */
#define YYNSTATES  514

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   331

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    32,     2,
      86,    87,    33,    31,    85,    30,    91,    34,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    82,
       2,    90,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    83,     2,    84,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    88,     2,    89,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     8,    11,    14,    17,    18,    20,
      23,    26,    29,    32,    35,    39,    41,    46,    48,    51,
      54,    58,    59,    61,    64,    68,    72,    76,    78,    80,
      84,    91,    93,    96,    99,   102,   106,   108,   110,   112,
     114,   116,   118,   120,   124,   128,   132,   135,   138,   141,
     144,   147,   150,   153,   156,   158,   162,   167,   171,   176,
     180,   183,   188,   191,   195,   200,   204,   209,   213,   216,
     218,   222,   223,   225,   228,   231,   235,   239,   248,   249,
     259,   268,   274,   280,   287,   294,   300,   306,   309,   312,
     315,   321,   327,   329,   331,   334,   336,   342,   344,   346,
     348,   350,   352,   354,   358,   362,   366,   370,   374,   378,
     382,   386,   390,   394,   398,   402,   406,   410,   415,   420,
     425,   430,   435,   440,   445,   446,   448,   452,   454,   456,
     458,   460,   462,   464,   466,   469,   472,   475,   478,   481,
     484,   486,   490,   492,   496,   500,   501,   510,   511,   518,
     519,   528,   531,   534,   538,   542,   546,   550,   553,   557,
     560,   565,   570,   572,   574,   576,   578,   580,   584,   588,
     592,   596,   599,   602,   606,   611,   616,   618,   620,   624,
     628,   632,   636,   640,   644,   648,   652,   657,   662,   664,
     666,   668,   672,   676,   680,   684,   687,   691,   696,   701,
     703,   705,   707,   709,   711,   715,   719,   723,   727,   731,
     735,   739,   743,   747,   751,   755,   759,   763,   767,   771,
     775,   779,   783,   785,   787,   789,   791,   796,   801,   803,
     805,   807,   809,   811,   816,   821,   823,   825,   828,   831,
     834,   837,   840,   843,   847,   849,   854,   857,   860,   863
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      93,     0,    -1,    94,    -1,    95,   102,    -1,    94,   105,
      -1,    94,    95,    -1,    94,   102,    -1,    -1,    96,    -1,
      95,    96,    -1,   101,    82,    -1,    43,    42,    -1,    43,
       4,    -1,    43,     5,    -1,    83,    40,    84,    -1,    98,
      -1,    99,    83,    40,    84,    -1,    44,    -1,    44,    99,
      -1,    97,   100,    -1,   101,    85,   100,    -1,    -1,   104,
      -1,   102,   104,    -1,   144,    85,    40,    -1,     6,    85,
      40,    -1,     7,    85,    40,    -1,     9,    -1,    81,    -1,
      81,    85,   129,    -1,     4,    44,    86,   103,    87,    82,
      -1,   123,    -1,   105,   123,    -1,     5,    44,    -1,     5,
      80,    -1,     3,    42,   108,    -1,    72,    -1,    73,    -1,
      74,    -1,    76,    -1,    77,    -1,    78,    -1,    75,    -1,
       3,    42,    44,    -1,     3,    13,    44,    -1,     3,    13,
     108,    -1,    42,    47,    -1,    42,    48,    -1,    42,    46,
      -1,    42,    50,    -1,    42,    49,    -1,    42,    59,    -1,
      42,    55,    -1,    42,    45,    -1,   112,    -1,   113,    85,
     112,    -1,   107,    86,   113,    87,    -1,   107,    86,    87,
      -1,   111,    86,   113,    87,    -1,   111,    86,    87,    -1,
      42,   100,    -1,   116,    85,    42,   100,    -1,    86,    87,
      -1,    86,   116,    87,    -1,   109,    86,   116,    87,    -1,
     109,    86,    87,    -1,   110,    86,   116,    87,    -1,   110,
      86,    87,    -1,    12,    82,    -1,   120,    -1,    12,   126,
      82,    -1,    -1,   125,    -1,   122,   125,    -1,   106,    82,
      -1,   109,   117,    82,    -1,   110,   117,    82,    -1,   106,
      86,    79,    87,    88,    95,   122,    89,    -1,    -1,   106,
      86,   103,    87,   124,    88,    95,   122,    89,    -1,   106,
      86,     8,    87,    88,    95,   122,    89,    -1,   119,    88,
      95,   122,    89,    -1,   115,    88,    95,   122,    89,    -1,
     118,    88,    95,   122,   121,    89,    -1,   114,    88,    95,
     122,   121,    89,    -1,   114,    88,    95,   121,    89,    -1,
     118,    88,    95,   121,    89,    -1,   127,    82,    -1,   140,
      82,    -1,   128,    82,    -1,    72,    86,   129,    87,    82,
      -1,    80,    86,   129,    87,    82,    -1,   132,    -1,   138,
      -1,    17,    82,    -1,   121,    -1,    18,    86,    40,    87,
      82,    -1,   141,    -1,   142,    -1,   143,    -1,   144,    -1,
     146,    -1,   145,    -1,    47,    90,   141,    -1,    46,    90,
     144,    -1,    48,    90,   142,    -1,    50,    90,   143,    -1,
      49,    90,   146,    -1,    45,    90,   145,    -1,   148,    90,
     141,    -1,   149,    90,   144,    -1,   150,    90,   145,    -1,
     151,    90,   146,    -1,   154,    90,   141,    -1,   155,    90,
     144,    -1,   157,    90,   145,    -1,   156,    90,   146,    -1,
      67,    86,   129,    87,    -1,    66,    86,   129,    87,    -1,
      69,    86,   129,    87,    -1,    70,    86,   129,    87,    -1,
      65,    86,   129,    87,    -1,    71,    86,   129,    87,    -1,
      68,    86,   129,    87,    -1,    -1,   130,    -1,   129,    85,
     130,    -1,   141,    -1,   144,    -1,   142,    -1,   143,    -1,
     145,    -1,   146,    -1,   131,    -1,    11,    47,    -1,    11,
      48,    -1,    11,    46,    -1,    11,    50,    -1,    11,    45,
      -1,    11,    49,    -1,   133,    -1,   133,    16,   134,    -1,
     135,    -1,   133,    16,   135,    -1,    88,   122,    89,    -1,
      -1,    15,    86,   144,    87,   136,    88,   122,    89,    -1,
      -1,    15,    86,   144,    87,   137,   125,    -1,    -1,    14,
      86,   144,    87,   139,    88,   122,    89,    -1,    47,    28,
      -1,    47,    29,    -1,   141,    31,   141,    -1,   141,    30,
     141,    -1,   141,    33,   141,    -1,   141,    34,   141,    -1,
      30,   141,    -1,    86,   141,    87,    -1,    35,   142,    -1,
      67,    86,   129,    87,    -1,    74,    86,   129,    87,    -1,
      47,    -1,    61,    -1,   148,    -1,   154,    -1,    40,    -1,
     142,    31,   142,    -1,   142,    30,   142,    -1,   142,    33,
     142,    -1,   142,    34,   142,    -1,    36,   141,    -1,    30,
     142,    -1,    86,   142,    87,    -1,    68,    86,   129,    87,
      -1,    75,    86,   129,    87,    -1,    48,    -1,    39,    -1,
     143,    32,   143,    -1,   143,    32,   141,    -1,   141,    32,
     143,    -1,   143,    32,   144,    -1,   144,    32,   143,    -1,
     143,    32,   142,    -1,   142,    32,   143,    -1,    86,   143,
      87,    -1,    71,    86,   129,    87,    -1,    78,    86,   129,
      87,    -1,    50,    -1,    41,    -1,   141,    -1,   144,    25,
     144,    -1,   144,    26,   144,    -1,   144,    19,   144,    -1,
     144,    20,   144,    -1,    27,   144,    -1,    86,   144,    87,
      -1,    66,    86,   129,    87,    -1,    73,    86,   129,    87,
      -1,    46,    -1,    60,    -1,   149,    -1,   155,    -1,    38,
      -1,   141,    19,   141,    -1,   142,    19,   142,    -1,   143,
      19,   143,    -1,   145,    19,   145,    -1,   146,    19,   146,
      -1,   141,    20,   141,    -1,   142,    20,   142,    -1,   143,
      20,   143,    -1,   145,    20,   145,    -1,   146,    20,   146,
      -1,   141,    22,   141,    -1,   142,    22,   142,    -1,   141,
      21,   141,    -1,   142,    21,   142,    -1,   141,    23,   141,
      -1,   142,    23,   142,    -1,   141,    24,   141,    -1,   142,
      24,   142,    -1,    45,    -1,    62,    -1,   150,    -1,   157,
      -1,    69,    86,   129,    87,    -1,    76,    86,   129,    87,
      -1,    79,    -1,     8,    -1,    80,    -1,    49,    -1,    63,
      -1,    70,    86,   129,    87,    -1,    77,    86,   129,    87,
      -1,   151,    -1,   156,    -1,   146,    91,    -1,   145,    91,
      -1,   147,    57,    -1,   147,    56,    -1,   147,    58,    -1,
     147,    59,    -1,    83,   141,    84,    -1,   152,    -1,   153,
      83,   141,    84,    -1,    53,   153,    -1,    52,   153,    -1,
      55,   153,    -1,    51,   153,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1754,  1754,  1982,  1986,  1990,  1994,  2001,  2004,  2009,
    2020,  2030,  2048,  2057,  2067,  2082,  2087,  2107,  2114,  2127,
    2140,  2160,  2161,  2162,  2165,  2174,  2180,  2186,  2192,  2204,
    2214,  2233,  2234,  2237,  2254,  2267,  2286,  2287,  2288,  2289,
    2290,  2291,  2292,  2296,  2323,  2348,  2371,  2375,  2379,  2383,
    2387,  2391,  2395,  2399,  2406,  2417,  2431,  2461,  2496,  2526,
    2561,  2595,  2627,  2628,  2636,  2652,  2673,  2687,  2708,  2711,
    2743,  2792,  2801,  2806,  2836,  2842,  2848,  2856,  2887,  2886,
    2930,  2957,  2994,  3031,  3074,  3115,  3142,  3175,  3193,  3211,
    3229,  3284,  3332,  3336,  3340,  3363,  3408,  3441,  3449,  3457,
    3465,  3473,  3481,  3495,  3505,  3515,  3525,  3535,  3550,  3565,
    3575,  3585,  3600,  3615,  3625,  3635,  3650,  3673,  3684,  3695,
    3706,  3717,  3729,  3740,  3763,  3772,  3777,  3803,  3814,  3825,
    3836,  3847,  3858,  3869,  3876,  3884,  3892,  3900,  3908,  3916,
    3931,  3940,  3976,  3982,  4018,  4040,  4039,  4096,  4095,  4163,
    4162,  4210,  4219,  4235,  4243,  4251,  4259,  4268,  4287,  4292,
    4310,  4323,  4379,  4389,  4399,  4410,  4418,  4436,  4444,  4452,
    4460,  4468,  4485,  4503,  4508,  4519,  4584,  4594,  4618,  4630,
    4640,  4650,  4660,  4670,  4680,  4690,  4696,  4707,  4753,  4763,
    4791,  4806,  4814,  4822,  4830,  4838,  4857,  4862,  4873,  4928,
    4938,  4948,  4956,  4964,  4979,  4987,  4995,  5003,  5016,  5029,
    5037,  5045,  5053,  5066,  5079,  5087,  5095,  5103,  5111,  5119,
    5127,  5135,  5150,  5158,  5168,  5175,  5185,  5194,  5251,  5267,
    5283,  5306,  5314,  5324,  5333,  5389,  5397,  5411,  5419,  5432,
    5448,  5461,  5473,  5492,  5505,  5510,  5527,  5537,  5547,  5557
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "lexFUNCTION", "TRIGGER", "EVENT",
  "WAIT", "EVERY", "INACTIVE", "INITIALISE", "LINK", "REF", "RET", "_VOID",
  "WHILE", "IF", "ELSE", "EXIT", "PAUSE", "BOOLEQUAL", "NOTEQUAL",
  "GREATEQUAL", "LESSEQUAL", "GREATER", "LESS", "_AND", "_OR", "_NOT",
  "_INC", "_DEC", "'-'", "'+'", "'&'", "'*'", "'/'", "TO_INT_CAST",
  "TO_FLOAT_CAST", "UMINUS", "BOOLEAN_T", "FLOAT_T", "INTEGER", "QTEXT",
  "TYPE", "STORAGE", "IDENT", "VAR", "BOOL_VAR", "NUM_VAR", "FLOAT_VAR",
  "OBJ_VAR", "STRING_VAR", "VAR_ARRAY", "BOOL_ARRAY", "NUM_ARRAY",
  "FLOAT_ARRAY", "OBJ_ARRAY", "BOOL_OBJVAR", "NUM_OBJVAR", "USER_OBJVAR",
  "OBJ_OBJVAR", "BOOL_CONSTANT", "NUM_CONSTANT", "USER_CONSTANT",
  "OBJ_CONSTANT", "STRING_CONSTANT", "FUNC", "BOOL_FUNC", "NUM_FUNC",
  "FLOAT_FUNC", "USER_FUNC", "OBJ_FUNC", "STRING_FUNC", "VOID_FUNC_CUST",
  "BOOL_FUNC_CUST", "NUM_FUNC_CUST", "FLOAT_FUNC_CUST", "USER_FUNC_CUST",
  "OBJ_FUNC_CUST", "STRING_FUNC_CUST", "TRIG_SYM", "EVENT_SYM",
  "CALLBACK_SYM", "';'", "'['", "']'", "','", "'('", "')'", "'{'", "'}'",
  "'='", "'.'", "$accept", "accept_script", "script", "var_list",
  "var_line", "variable_decl_head", "array_sub_decl",
  "array_sub_decl_list", "variable_ident", "variable_decl", "trigger_list",
  "trigger_subdecl", "trigger_decl", "event_list", "event_subdecl",
  "function_def", "function_type", "func_subdecl", "void_func_subdecl",
  "void_function_def", "funcvar_decl_types", "funcbody_var_def_body",
  "funcbody_var_def", "void_funcbody_var_def", "argument_decl_head",
  "argument_decl", "function_declaration", "void_function_declaration",
  "return_statement_void", "return_statement", "statement_list",
  "event_decl", "$@1", "statement", "return_exp", "assignment",
  "func_call", "param_list", "parameter", "var_ref", "conditional",
  "cond_clause_list", "terminal_cond", "cond_clause", "$@2", "$@3", "loop",
  "$@4", "inc_dec_exp", "expression", "floatexp", "stringexp", "boolexp",
  "userexp", "objexp", "objexp_dot", "num_objvar", "bool_objvar",
  "user_objvar", "obj_objvar", "array_index", "array_index_list",
  "num_array_var", "bool_array_var", "obj_array_var", "user_array_var", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
      45,    43,    38,    42,    47,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   313,   314,   315,   316,   317,   318,   319,
     320,   321,   322,   323,   324,   325,   326,   327,   328,   329,
     330,   331,    59,    91,    93,    44,    40,    41,   123,   125,
      61,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    92,    93,    94,    94,    94,    94,    95,    95,    95,
      96,    97,    97,    97,    98,    99,    99,   100,   100,   101,
     101,   102,   102,   102,   103,   103,   103,   103,   103,   103,
     104,   105,   105,   106,   106,   107,   108,   108,   108,   108,
     108,   108,   108,   109,   110,   111,   112,   112,   112,   112,
     112,   112,   112,   112,   113,   113,   114,   114,   115,   115,
     116,   116,   117,   117,   118,   118,   119,   119,   120,   121,
     121,   122,   122,   122,   123,   123,   123,   123,   124,   123,
     123,   123,   123,   123,   123,   123,   123,   125,   125,   125,
     125,   125,   125,   125,   125,   125,   125,   126,   126,   126,
     126,   126,   126,   127,   127,   127,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   128,   128,   128,
     128,   128,   128,   128,   129,   129,   129,   130,   130,   130,
     130,   130,   130,   130,   131,   131,   131,   131,   131,   131,
     132,   132,   133,   133,   134,   136,   135,   137,   135,   139,
     138,   140,   140,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   143,   143,
     143,   143,   143,   143,   143,   143,   143,   143,   143,   143,
     143,   144,   144,   144,   144,   144,   144,   144,   144,   144,
     144,   144,   144,   144,   144,   144,   144,   144,   144,   144,
     144,   144,   144,   144,   144,   144,   144,   144,   144,   144,
     144,   144,   145,   145,   145,   145,   145,   145,   145,   145,
     145,   146,   146,   146,   146,   146,   146,   147,   147,   148,
     149,   150,   151,   152,   153,   153,   154,   155,   156,   157
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     2,     2,     2,     0,     1,     2,
       2,     2,     2,     2,     3,     1,     4,     1,     2,     2,
       3,     0,     1,     2,     3,     3,     3,     1,     1,     3,
       6,     1,     2,     2,     2,     3,     1,     1,     1,     1,
       1,     1,     1,     3,     3,     3,     2,     2,     2,     2,
       2,     2,     2,     2,     1,     3,     4,     3,     4,     3,
       2,     4,     2,     3,     4,     3,     4,     3,     2,     1,
       3,     0,     1,     2,     2,     3,     3,     8,     0,     9,
       8,     5,     5,     6,     6,     5,     5,     2,     2,     2,
       5,     5,     1,     1,     2,     1,     5,     1,     1,     1,
       1,     1,     1,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     4,     4,     4,
       4,     4,     4,     4,     0,     1,     3,     1,     1,     1,
       1,     1,     1,     1,     2,     2,     2,     2,     2,     2,
       1,     3,     1,     3,     3,     0,     8,     0,     6,     0,
       8,     2,     2,     3,     3,     3,     3,     2,     3,     2,
       4,     4,     1,     1,     1,     1,     1,     3,     3,     3,
       3,     2,     2,     3,     4,     4,     1,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     4,     4,     1,     1,
       1,     3,     3,     3,     3,     2,     3,     4,     4,     1,
       1,     1,     1,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     1,     1,     1,     1,     4,     4,     1,     1,
       1,     1,     1,     4,     4,     1,     1,     2,     2,     2,
       2,     2,     2,     3,     1,     4,     2,     2,     2,     2
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       7,     0,     0,     2,    21,     8,     0,     0,    12,    13,
      11,     1,     0,     0,     0,     5,     6,    22,     4,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    31,     9,
       3,    17,    19,    10,     0,     0,     0,     0,    33,    34,
      23,    32,    74,     0,     0,     0,     0,     0,     0,     0,
       7,     7,     7,     7,     0,    15,    18,    20,    44,    36,
      37,    38,    42,    39,    40,    41,    45,    43,    35,     0,
       0,     0,   229,    27,     0,     0,     0,     0,   203,   177,
     166,   189,   222,   199,   162,   176,   231,   188,     0,     0,
       0,     0,   200,   163,   223,   232,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   228,   230,
      28,     0,     0,   190,     0,     0,     0,     0,     0,     0,
     164,   201,   224,   235,   165,   202,   236,   225,     0,    57,
      54,     0,     0,    62,     0,    75,    62,     0,    76,    59,
       0,     0,    71,     0,    71,     0,     0,   229,   228,     0,
       0,     0,     0,   195,     0,   157,   172,     0,     0,     0,
       0,     0,   159,     0,     0,   171,     0,   244,   249,   247,
     246,   248,   124,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   124,     0,   124,     0,     0,     0,     0,
      78,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   238,     0,     0,   237,   240,   239,
     241,   242,    53,    48,    46,    47,    50,    49,    52,    51,
       0,    56,    60,     0,    63,    63,    58,     0,     0,     0,
       0,     0,   222,     0,     0,     0,   231,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   230,    69,    95,     0,
      72,     0,     0,    92,   140,   142,    93,     0,     0,     0,
     224,   235,     0,     0,   236,   225,    95,     0,    95,     0,
       0,    14,     0,     0,    25,    26,     7,     0,     0,     0,
       0,     0,     0,   125,   133,   127,   129,   130,   128,   131,
     132,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     7,    29,   158,   173,   185,   196,     0,   204,
     209,   216,   214,   218,   220,   154,   153,   180,     0,   155,
     156,   205,   210,   217,   215,   219,   221,   168,   167,   184,
     169,   170,   206,   211,   179,   183,   178,   181,   193,   194,
     191,   192,   182,    24,   207,     0,   212,   208,   213,    55,
       0,    68,     0,    97,    98,    99,   100,   102,   101,     0,
       0,    94,     0,     0,     0,   151,   152,     0,     0,     0,
       0,   124,   124,   124,   124,   124,   124,   124,   124,   124,
      85,    95,    73,    87,    89,     0,    88,     0,     0,     0,
       0,     0,     0,     0,     0,    82,    86,    95,    81,    16,
      30,    71,   243,     0,   138,   136,   134,   135,   139,   137,
       0,   197,   160,   174,   226,   233,   186,   198,   161,   175,
     227,   234,   187,    71,     7,    61,    70,     0,     0,     0,
     108,   104,   103,   105,   107,   106,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    84,    71,   141,   143,   109,
     110,   111,   112,   113,   114,   116,   115,    83,     0,   245,
     126,     0,    71,   149,   147,     0,   121,   118,   117,   123,
     119,   120,   122,     0,     0,     0,    80,    77,     0,     0,
       0,     0,    96,    90,    91,   144,    79,    71,    71,   148,
       0,     0,   150,   146
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     3,     4,     5,     6,    55,    56,    32,     7,
      16,   112,    17,    18,    19,    20,    66,    21,    22,    23,
     130,   131,    24,    25,   134,    46,    26,    27,   267,   286,
     269,    28,   328,   270,   372,   271,   272,   302,   303,   304,
     273,   274,   467,   275,   500,   501,   276,   499,   277,   305,
     306,   307,   308,   157,   158,   119,   120,   121,   122,   123,
     167,   168,   124,   125,   126,   127
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -236
static const yytype_int16 yypact[] =
{
     -25,    19,    31,   131,    -1,  -236,    -8,   -45,  -236,  -236,
    -236,  -236,    -7,    16,   -14,   -25,    78,  -236,   172,   -69,
      17,    24,    35,    39,    67,    72,    90,    93,  -236,  -236,
      78,    12,  -236,  -236,    -8,   269,   601,   113,  -236,  -236,
    -236,  -236,  -236,    38,   -34,   -23,   132,   -20,   173,   -17,
     -25,   -25,   -25,   -25,   214,  -236,   179,  -236,  -236,  -236,
    -236,  -236,  -236,  -236,  -236,  -236,  -236,  -236,  -236,  1727,
     180,   192,   194,  -236,  1915,  1972,   509,  2020,  -236,  -236,
    -236,  -236,  -236,  -236,  -236,  -236,  -236,  -236,   228,   228,
     228,   228,  -236,  -236,  -236,  -236,   206,   230,   236,   239,
     249,   250,   253,   264,   265,   270,   306,   307,   319,  -236,
     330,  1915,   329,   719,   767,   181,    37,   -10,     1,   161,
    -236,  -236,  -236,  -236,  -236,  -236,  -236,  -236,   341,  -236,
    -236,   153,    -8,   347,   221,  -236,   352,   227,  -236,  -236,
     234,  2062,  2062,  2062,  2062,   348,   416,  -236,  -236,   373,
     421,   425,   385,   444,  1972,  -236,  -236,   391,   396,   114,
     509,   509,  -236,  2020,  2020,  -236,  2020,  -236,   408,   408,
     408,   408,  1801,  1801,  1801,  1801,  1801,  1801,  1801,  1801,
    1801,  1801,  1801,  1801,   411,  1801,   174,   568,     9,     7,
    -236,  2020,  2020,  2020,  2020,  2020,  2020,  2020,  2020,  1915,
    2020,  2020,   509,   509,   509,   509,   509,   509,   509,   509,
    1915,   509,   509,  1915,  1915,  1915,  1915,  1915,  1915,  1915,
    1915,   455,   657,   657,  -236,   657,   657,  -236,  -236,  -236,
    -236,  -236,  -236,  -236,  -236,  -236,  -236,  -236,  -236,  -236,
     458,  -236,  -236,   461,   420,   436,  -236,  1858,   423,   434,
     447,   446,   443,   478,    30,   481,   482,   483,   451,   465,
     484,   488,   494,   496,   497,   499,   500,  -236,   505,  2132,
    -236,   521,   522,  -236,   531,  -236,  -236,   523,   517,   518,
     519,   524,   525,   527,   528,   529,  -236,  1113,   532,  2132,
    1232,  -236,   536,   530,  -236,  -236,   -25,   136,   191,    96,
    2020,   202,   286,  -236,  -236,   719,   767,   181,   271,   -10,
       1,   287,   316,   326,   327,   344,   370,   383,   384,   398,
     403,   407,   -25,   479,  -236,  -236,  -236,  -236,   539,   252,
     252,   252,   252,   252,   252,    98,    98,  -236,   271,  -236,
    -236,   364,   364,   364,   364,   364,   364,   254,   254,  -236,
    -236,  -236,   592,   592,   792,   885,  -236,  -236,   444,   444,
     183,   183,  -236,  -236,   391,    13,   391,   396,   396,  -236,
      -8,  -236,   547,   719,   767,   181,   271,   -10,     1,  1915,
    1915,  -236,   590,   657,  1915,  -236,  -236,  2020,   509,   657,
    1915,  1801,  1801,  1801,  1801,  1801,  1801,  1801,  1801,  1801,
    -236,   542,  -236,  -236,  -236,   -13,  -236,  2020,  1915,   657,
     657,  2020,  1915,   657,   657,  -236,  -236,   545,  -236,  -236,
    -236,  2062,  -236,   211,  -236,  -236,  -236,  -236,  -236,  -236,
    1801,  -236,  -236,  -236,  -236,  -236,  -236,  -236,  -236,  -236,
    -236,  -236,  -236,  2062,   -25,  -236,  -236,   298,   350,   548,
     391,   271,   252,   364,   396,   181,   427,   432,   453,   456,
     457,   467,   474,   475,   480,  -236,  2132,  -236,  -236,   252,
     271,   391,   396,   252,   271,   396,   391,  -236,  1302,  -236,
    -236,  1372,  2062,  -236,   558,   565,  -236,  -236,  -236,  -236,
     557,   563,  -236,   567,   575,  1442,  -236,  -236,  1512,   570,
     571,  2132,  -236,  -236,  -236,  -236,  -236,  2132,  2132,  -236,
    1582,  1652,  -236,  -236
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -236,  -236,  -236,    -2,    -4,  -236,  -236,  -236,   -30,  -236,
     656,   593,   160,  -236,  -236,  -236,   625,  -236,  -236,  -236,
     424,   620,  -236,  -236,   623,   659,  -236,  -236,  -236,  -136,
    -128,   664,  -236,  -235,  -236,  -236,  -236,   245,   259,  -236,
    -236,  -236,  -236,   279,  -236,  -236,  -236,  -236,  -236,   685,
     736,   761,   819,   -31,    54,   856,    15,    20,    68,   115,
    -236,   209,   185,   190,   266,   363
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -234
static const yytype_int16 yytable[] =
{
      29,    15,   249,    13,    57,   268,    35,   288,   128,   222,
     223,    29,   117,    42,   287,   289,   290,    43,     1,   132,
     225,   226,   132,     8,     9,   128,   216,   217,   213,   214,
      38,    11,   218,   219,   402,    36,    31,    33,   117,   220,
      34,   215,     1,   117,    70,    71,    72,    73,   141,   142,
     143,   144,   402,   129,   402,   402,   216,   217,   385,   386,
      37,    10,   218,   219,   133,    74,    39,   136,    75,   220,
     139,   230,   231,    76,    77,   466,    78,    79,    80,    81,
     117,   224,    13,    82,    83,    84,    85,    86,    87,    88,
      89,    90,   227,    91,   327,    54,   326,   118,    92,    93,
      94,    95,   242,    44,    96,    97,    98,    99,   100,   101,
      45,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     387,    47,   221,   118,   111,    49,   197,   198,   118,   200,
     201,   200,   201,   401,    12,    13,    14,    29,    29,    29,
      29,   309,   309,   309,   309,   309,   309,   309,   309,   309,
     309,   309,   309,   417,   309,    50,   278,   278,   278,   278,
      51,   279,   279,   279,   279,   118,   197,   198,   117,   200,
     201,   229,   230,   231,     1,    12,    40,    14,    52,   117,
     422,    53,   117,   117,   117,   117,   117,   117,   117,   117,
      40,   364,   366,   191,   192,   193,   194,   195,   196,    69,
     213,   214,   216,   217,   197,   198,   199,   200,   201,   280,
     280,   280,   280,   215,   135,   220,   377,   228,   229,   230,
     231,   208,   209,   324,   211,   212,   310,   310,   310,   310,
     310,   310,   310,   310,   310,   310,   310,   310,   240,   310,
     241,   197,   198,   402,   200,   201,   402,   424,   425,   426,
     427,   428,   429,   118,   145,   138,   281,   281,   281,   281,
     402,   324,   146,   402,   118,   150,   509,   118,   118,   118,
     118,   118,   118,   118,   118,   402,   402,   151,   325,   367,
     368,   152,   197,   198,   278,   200,   201,   211,   212,   279,
     216,   217,   172,   478,   421,   479,   218,   219,   169,   170,
     171,   378,   278,   220,   278,   278,   243,   279,   244,   279,
     279,   166,   243,    58,   245,   481,   173,   216,   217,   240,
     443,   246,   174,   218,   219,   175,   282,   282,   282,   282,
     220,   283,   283,   283,   283,   176,   177,   280,   495,   178,
     445,    59,    60,    61,    62,    63,    64,    65,   117,   117,
     179,   180,   450,   117,   498,   280,   181,   280,   280,   117,
     309,   309,   309,   309,   309,   309,   309,   309,   309,   216,
     217,   430,   430,   431,   432,   218,   219,   117,   471,   510,
     511,   117,   220,   476,   281,   483,   232,   233,   234,   235,
     236,   237,   182,   183,   208,   209,   238,   211,   212,   309,
     239,   430,   281,   433,   281,   281,   184,   284,   284,   284,
     284,   430,   430,   434,   435,   185,   190,    29,   311,   312,
     313,   314,   315,   316,   317,   318,   319,   320,   321,   430,
     323,   436,   291,   118,   118,   -65,   278,   484,   118,    29,
     -67,   279,   482,   454,   118,   310,   310,   310,   310,   310,
     310,   310,   310,   310,   282,   430,   292,   437,   278,   283,
     293,   294,   118,   279,   472,   295,   118,   475,   430,   430,
     438,   439,   282,   296,   282,   282,   220,   283,    29,   283,
     283,   278,   224,   430,   310,   440,   279,   227,   430,   280,
     441,   300,   430,   278,   442,   363,   278,   278,   279,   322,
     128,   279,   279,   370,   285,   285,   285,   285,   -64,   379,
     278,   280,   430,   278,   486,   279,   278,   430,   279,   487,
     380,   279,   278,   278,   -66,   278,   278,   279,   279,   381,
     279,   279,   382,   383,   280,   284,   281,   391,   430,   160,
     488,   430,   430,   489,   490,    77,   280,   405,    79,   280,
     280,   392,   430,   284,   491,   284,   284,    85,   281,   430,
     430,   492,   493,   280,   430,   430,   280,   494,   384,   280,
     393,   388,   389,   390,   394,   280,   280,    98,   280,   280,
     395,   281,   396,   397,   104,   398,   399,   202,   203,   204,
     205,   206,   207,   281,   400,   161,   281,   281,   208,   209,
     210,   211,   212,   403,   404,   406,   282,   407,   408,   409,
     281,   283,   420,   281,   410,   411,   281,   412,   413,   414,
     419,   416,   281,   281,   215,   281,   281,   444,   282,   446,
     449,   465,   285,   283,   477,   485,   456,   457,   458,   459,
     460,   461,   462,   463,   464,    67,  -145,   502,  -226,   503,
     285,   282,   285,   285,  -233,   325,   283,   504,   507,   508,
      30,    68,   149,   282,   369,   147,   282,   282,   283,   140,
     137,   283,   283,    59,    60,    61,    62,    63,    64,    65,
     282,    48,    41,   282,   468,   283,   282,   284,   283,   480,
       0,   283,   282,   282,     0,   282,   282,   283,   283,     0,
     283,   283,    82,     0,     0,     0,    86,     0,    88,   284,
       0,     0,    91,     0,     0,     0,     0,     0,     0,    94,
      95,     0,     0,     0,     0,     0,    99,   100,   113,     0,
       0,     0,   284,   105,   106,     0,   148,   109,   191,   192,
     193,   194,   195,   196,   284,     0,     0,   284,   284,   197,
     198,   199,   200,   201,   113,     0,     0,     0,     0,   113,
     155,   284,   165,     0,   284,     0,     0,   284,     0,     0,
       0,     0,     0,   284,   284,     0,   284,   284,     0,   114,
       0,     0,     0,     0,   285,     0,   202,   203,   204,   205,
     206,   207,     0,     0,     0,     0,   186,   208,   209,   210,
     211,   212,     0,     0,   115,   114,   285,     0,     0,     0,
     114,   156,   162,   193,   194,   195,   196,     0,     0,     0,
       0,     0,   197,   198,     0,   200,   201,     0,     0,   285,
     115,     0,     0,     0,     0,   115,     0,     0,     0,   297,
       0,   285,     0,     0,   285,   285,     0,   187,   155,   297,
       0,   299,     0,     0,     0,     0,     0,     0,   285,     0,
       0,   285,   116,     0,   285,     0,     0,     0,     0,     0,
     285,   285,   188,   285,   285,     0,   329,   330,   331,   332,
     333,   334,   335,   336,   113,   339,   340,     0,   116,     0,
     298,     0,     0,   153,     0,   113,   156,   298,   113,   113,
     354,   113,   113,   113,   113,   113,   204,   205,   206,   207,
       0,     0,     0,     0,     0,   208,   209,     0,   211,   212,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     189,   159,   373,   159,     0,   114,     0,     0,   341,   342,
     343,   344,   345,   346,   347,   348,   114,   350,   351,   114,
     114,   355,   114,   114,   114,   114,   114,     0,     0,     0,
     337,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   349,     0,     0,   352,   353,   356,   115,   115,   115,
     115,   362,     0,   374,     0,   423,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   375,     0,
     159,     0,     0,     0,     0,     0,     0,     0,   338,   159,
     159,     0,   159,     0,     0,     0,     0,     0,     0,   338,
       0,     0,   338,   338,   357,   358,   359,   360,   361,   338,
       0,     0,     0,     0,     0,     0,     0,   159,   159,   159,
     159,   159,   159,   159,   159,     0,   159,   159,     0,     0,
       0,     0,     0,     0,   113,   113,   376,     0,     0,   113,
       0,     0,   452,     0,     0,   113,     0,     0,   365,   365,
       0,   365,   365,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   469,   113,     0,     0,   473,   113,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   114,   114,     0,     0,     0,
     114,   147,     0,     0,   453,   247,   114,   248,   249,     0,
     250,   251,     0,     0,     0,     0,     0,     0,     0,     0,
     115,   115,     0,     0,   114,   115,     0,     0,   114,     0,
       0,   455,     0,     0,     0,     0,   159,     0,   252,   253,
     254,   255,   256,   257,    88,    89,    90,     0,    91,   115,
       0,     0,     0,   115,     0,    94,    95,     0,   258,   259,
     260,   261,   262,   263,   264,   265,     0,     0,     0,   105,
     106,     0,   148,   266,     0,     0,     0,     0,   447,   448,
       0,     0,   415,   451,     0,     0,     0,     0,     0,   338,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   470,     0,     0,
       0,   474,     0,     0,     0,     0,     0,     0,     0,   365,
     147,     0,     0,   159,   247,   365,   248,   249,     0,   250,
     251,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   159,     0,   365,   365,   159,     0,   365,
     365,     0,     0,     0,     0,     0,     0,   252,   253,   254,
     255,   256,   257,    88,    89,    90,     0,    91,     0,     0,
       0,     0,     0,     0,    94,    95,     0,   258,   259,   260,
     261,   262,   263,   264,   265,     0,     0,     0,   105,   106,
     147,   148,   266,     0,   247,     0,   248,   249,     0,   250,
     251,   418,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   252,   253,   254,
     255,   256,   257,    88,    89,    90,     0,    91,     0,     0,
       0,     0,     0,     0,    94,    95,     0,   258,   259,   260,
     261,   262,   263,   264,   265,     0,     0,     0,   105,   106,
     147,   148,   266,     0,   247,     0,   248,   249,     0,   250,
     251,   496,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   252,   253,   254,
     255,   256,   257,    88,    89,    90,     0,    91,     0,     0,
       0,     0,     0,     0,    94,    95,     0,   258,   259,   260,
     261,   262,   263,   264,   265,     0,     0,     0,   105,   106,
     147,   148,   266,     0,   247,     0,   248,   249,     0,   250,
     251,   497,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   252,   253,   254,
     255,   256,   257,    88,    89,    90,     0,    91,     0,     0,
       0,     0,     0,     0,    94,    95,     0,   258,   259,   260,
     261,   262,   263,   264,   265,     0,     0,     0,   105,   106,
     147,   148,   266,     0,   247,     0,   248,   249,     0,   250,
     251,   505,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   252,   253,   254,
     255,   256,   257,    88,    89,    90,     0,    91,     0,     0,
       0,     0,     0,     0,    94,    95,     0,   258,   259,   260,
     261,   262,   263,   264,   265,     0,     0,     0,   105,   106,
     147,   148,   266,     0,   247,     0,   248,   249,     0,   250,
     251,   506,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   252,   253,   254,
     255,   256,   257,    88,    89,    90,     0,    91,     0,     0,
       0,     0,     0,     0,    94,    95,     0,   258,   259,   260,
     261,   262,   263,   264,   265,     0,     0,     0,   105,   106,
     147,   148,   266,     0,   247,     0,   248,   249,     0,   250,
     251,   512,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   252,   253,   254,
     255,   256,   257,    88,    89,    90,     0,    91,     0,     0,
       0,     0,     0,     0,    94,    95,     0,   258,   259,   260,
     261,   262,   263,   264,   265,     0,     0,     0,   105,   106,
       0,   148,   266,    70,    71,   147,    73,     0,     0,     0,
       0,   513,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    74,     0,     0,    75,     0,     0,
       0,     0,    76,    77,     0,    78,    79,    80,    81,     0,
       0,     0,    82,    83,    84,    85,    86,    87,    88,    89,
      90,     0,    91,     0,     0,     0,     0,    92,    93,    94,
      95,     0,     0,    96,    97,    98,    99,   100,   101,     0,
     102,   103,   104,   105,   106,   107,   148,   109,   110,   147,
       0,     0,   301,   111,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    74,     0,
       0,    75,     0,     0,     0,     0,    76,    77,     0,    78,
      79,    80,    81,     0,     0,     0,    82,    83,    84,    85,
      86,    87,    88,    89,    90,     0,    91,     0,     0,     0,
       0,    92,    93,    94,    95,     0,   147,    96,    97,    98,
      99,   100,   101,     0,   102,   103,   104,   105,   106,   107,
     148,   109,     0,     0,     0,    74,     0,   111,    75,     0,
       0,     0,     0,    76,    77,     0,    78,    79,    80,    81,
       0,     0,     0,    82,    83,    84,    85,    86,    87,    88,
      89,    90,     0,    91,     0,     0,     0,     0,    92,    93,
      94,    95,     0,   147,    96,    97,    98,    99,   100,   101,
       0,   102,   103,   104,   105,   106,   107,   148,   109,     0,
     371,     0,    74,     0,   111,    75,     0,     0,     0,     0,
      76,    77,     0,    78,    79,    80,    81,     0,     0,     0,
      82,    83,    84,    85,    86,    87,    88,    89,    90,     0,
      91,     0,     0,     0,     0,    92,    93,    94,    95,     0,
     147,    96,    97,    98,    99,   100,   101,     0,   102,   103,
     104,   105,   106,   107,   148,   109,     0,     0,     0,     0,
       0,   111,    75,     0,     0,     0,     0,    76,    77,     0,
       0,    79,    80,     0,     0,     0,     0,    82,     0,    84,
      85,    86,     0,    88,     0,    90,     0,    91,   147,     0,
       0,     0,     0,    93,    94,    95,     0,     0,     0,    97,
      98,    99,   100,     0,     0,     0,   103,   104,   105,   106,
     163,   148,   109,     0,     0,    76,     0,     0,   154,     0,
      80,     0,     0,     0,     0,    82,     0,    84,     0,    86,
     147,    88,     0,    90,   247,    91,   248,   249,     0,   250,
     251,    93,    94,    95,     0,     0,     0,    97,     0,    99,
     100,     0,     0,     0,   103,     0,   105,   106,     0,   148,
     109,     0,     0,     0,     0,     1,   164,   252,   253,   254,
     255,   256,   257,    88,    89,    90,     0,    91,     0,     0,
       0,     0,     0,     0,    94,    95,     0,   258,   259,   260,
     261,   262,   263,   264,   265,     0,     0,     0,   105,   106,
     147,   148,   266,     0,   247,     0,   248,   249,     0,   250,
     251,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   252,   253,   254,
     255,   256,   257,    88,    89,    90,     0,    91,     0,     0,
       0,     0,     0,     0,    94,    95,     0,   258,   259,   260,
     261,   262,   263,   264,   265,     0,     0,     0,   105,   106,
       0,   148,   266
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-236))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
       4,     3,    15,     4,    34,   141,    13,   143,    42,    19,
      20,    15,    43,    82,   142,   143,   144,    86,    43,    42,
      19,    20,    42,     4,     5,    42,    19,    20,    19,    20,
      44,     0,    25,    26,   269,    42,    44,    82,    69,    32,
      85,    32,    43,    74,     6,     7,     8,     9,    50,    51,
      52,    53,   287,    87,   289,   290,    19,    20,    28,    29,
      44,    42,    25,    26,    87,    27,    80,    87,    30,    32,
      87,    58,    59,    35,    36,    88,    38,    39,    40,    41,
     111,    91,     4,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    91,    55,    87,    83,    87,    43,    60,    61,
      62,    63,   132,    86,    66,    67,    68,    69,    70,    71,
      86,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      90,    86,    85,    69,    86,    86,    30,    31,    74,    33,
      34,    33,    34,   269,     3,     4,     5,   141,   142,   143,
     144,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   289,   185,    88,   141,   142,   143,   144,
      88,   141,   142,   143,   144,   111,    30,    31,   199,    33,
      34,    57,    58,    59,    43,     3,    16,     5,    88,   210,
      84,    88,   213,   214,   215,   216,   217,   218,   219,   220,
      30,   222,   223,    19,    20,    21,    22,    23,    24,    86,
      19,    20,    19,    20,    30,    31,    32,    33,    34,   141,
     142,   143,   144,    32,    82,    32,   247,    56,    57,    58,
      59,    30,    31,    87,    33,    34,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,    85,   185,
      87,    30,    31,   478,    33,    34,   481,    45,    46,    47,
      48,    49,    50,   199,    40,    82,   141,   142,   143,   144,
     495,    87,    83,   498,   210,    85,   501,   213,   214,   215,
     216,   217,   218,   219,   220,   510,   511,    85,    87,   225,
     226,    87,    30,    31,   269,    33,    34,    33,    34,   269,
      19,    20,    86,   421,   296,    84,    25,    26,    89,    90,
      91,   247,   287,    32,   289,   290,    85,   287,    87,   289,
     290,    83,    85,    44,    87,   443,    86,    19,    20,    85,
     322,    87,    86,    25,    26,    86,   141,   142,   143,   144,
      32,   141,   142,   143,   144,    86,    86,   269,   466,    86,
     370,    72,    73,    74,    75,    76,    77,    78,   379,   380,
      86,    86,   383,   384,   482,   287,    86,   289,   290,   390,
     391,   392,   393,   394,   395,   396,   397,   398,   399,    19,
      20,    85,    85,    87,    87,    25,    26,   408,   409,   507,
     508,   412,    32,   414,   269,    87,    45,    46,    47,    48,
      49,    50,    86,    86,    30,    31,    55,    33,    34,   430,
      59,    85,   287,    87,   289,   290,    87,   141,   142,   143,
     144,    85,    85,    87,    87,    85,    87,   421,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,    85,
     185,    87,    84,   379,   380,    88,   421,    87,   384,   443,
      88,   421,   444,   389,   390,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   269,    85,    40,    87,   443,   269,
      87,    40,   408,   443,   410,    40,   412,   413,    85,    85,
      87,    87,   287,    88,   289,   290,    32,   287,   482,   289,
     290,   466,    91,    85,   430,    87,   466,    91,    85,   421,
      87,    83,    85,   478,    87,    40,   481,   482,   478,    88,
      42,   481,   482,    42,   141,   142,   143,   144,    88,    86,
     495,   443,    85,   498,    87,   495,   501,    85,   498,    87,
      86,   501,   507,   508,    88,   510,   511,   507,   508,    82,
     510,   511,    86,    90,   466,   269,   421,    86,    85,    30,
      87,    85,    85,    87,    87,    36,   478,    16,    39,   481,
     482,    86,    85,   287,    87,   289,   290,    48,   443,    85,
      85,    87,    87,   495,    85,    85,   498,    87,    90,   501,
      86,    90,    90,    90,    86,   507,   508,    68,   510,   511,
      86,   466,    86,    86,    75,    86,    86,    19,    20,    21,
      22,    23,    24,   478,    89,    86,   481,   482,    30,    31,
      32,    33,    34,    82,    82,    82,   421,    90,    90,    90,
     495,   421,    82,   498,    90,    90,   501,    90,    90,    90,
      84,    89,   507,   508,    32,   510,   511,    88,   443,    82,
      40,    89,   269,   443,    89,    87,   391,   392,   393,   394,
     395,   396,   397,   398,   399,    44,    88,    82,    91,    82,
     287,   466,   289,   290,    91,    87,   466,    82,    88,    88,
       4,    36,    69,   478,   240,     8,   481,   482,   478,    49,
      47,   481,   482,    72,    73,    74,    75,    76,    77,    78,
     495,    22,    18,   498,   405,   495,   501,   421,   498,   430,
      -1,   501,   507,   508,    -1,   510,   511,   507,   508,    -1,
     510,   511,    45,    -1,    -1,    -1,    49,    -1,    51,   443,
      -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    62,
      63,    -1,    -1,    -1,    -1,    -1,    69,    70,    43,    -1,
      -1,    -1,   466,    76,    77,    -1,    79,    80,    19,    20,
      21,    22,    23,    24,   478,    -1,    -1,   481,   482,    30,
      31,    32,    33,    34,    69,    -1,    -1,    -1,    -1,    74,
      75,   495,    77,    -1,   498,    -1,    -1,   501,    -1,    -1,
      -1,    -1,    -1,   507,   508,    -1,   510,   511,    -1,    43,
      -1,    -1,    -1,    -1,   421,    -1,    19,    20,    21,    22,
      23,    24,    -1,    -1,    -1,    -1,   111,    30,    31,    32,
      33,    34,    -1,    -1,    43,    69,   443,    -1,    -1,    -1,
      74,    75,    76,    21,    22,    23,    24,    -1,    -1,    -1,
      -1,    -1,    30,    31,    -1,    33,    34,    -1,    -1,   466,
      69,    -1,    -1,    -1,    -1,    74,    -1,    -1,    -1,   154,
      -1,   478,    -1,    -1,   481,   482,    -1,   111,   163,   164,
      -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,   495,    -1,
      -1,   498,    43,    -1,   501,    -1,    -1,    -1,    -1,    -1,
     507,   508,   111,   510,   511,    -1,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,   201,    -1,    69,    -1,
     154,    -1,    -1,    74,    -1,   210,   160,   161,   213,   214,
     215,   216,   217,   218,   219,   220,    21,    22,    23,    24,
      -1,    -1,    -1,    -1,    -1,    30,    31,    -1,    33,    34,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     111,    75,   247,    77,    -1,   199,    -1,    -1,   202,   203,
     204,   205,   206,   207,   208,   209,   210,   211,   212,   213,
     214,   215,   216,   217,   218,   219,   220,    -1,    -1,    -1,
     199,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   210,    -1,    -1,   213,   214,   215,   216,   217,   218,
     219,   220,    -1,   247,    -1,   300,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   247,    -1,
     154,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   199,   163,
     164,    -1,   166,    -1,    -1,    -1,    -1,    -1,    -1,   210,
      -1,    -1,   213,   214,   215,   216,   217,   218,   219,   220,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   191,   192,   193,
     194,   195,   196,   197,   198,    -1,   200,   201,    -1,    -1,
      -1,    -1,    -1,    -1,   379,   380,   247,    -1,    -1,   384,
      -1,    -1,   387,    -1,    -1,   390,    -1,    -1,   222,   223,
      -1,   225,   226,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   407,   408,    -1,    -1,   411,   412,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   379,   380,    -1,    -1,    -1,
     384,     8,    -1,    -1,   388,    12,   390,    14,    15,    -1,
      17,    18,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     379,   380,    -1,    -1,   408,   384,    -1,    -1,   412,    -1,
      -1,   390,    -1,    -1,    -1,    -1,   300,    -1,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    -1,    55,   408,
      -1,    -1,    -1,   412,    -1,    62,    63,    -1,    65,    66,
      67,    68,    69,    70,    71,    72,    -1,    -1,    -1,    76,
      77,    -1,    79,    80,    -1,    -1,    -1,    -1,   379,   380,
      -1,    -1,    89,   384,    -1,    -1,    -1,    -1,    -1,   390,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   408,    -1,    -1,
      -1,   412,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   383,
       8,    -1,    -1,   387,    12,   389,    14,    15,    -1,    17,
      18,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   407,    -1,   409,   410,   411,    -1,   413,
     414,    -1,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    62,    63,    -1,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    76,    77,
       8,    79,    80,    -1,    12,    -1,    14,    15,    -1,    17,
      18,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    62,    63,    -1,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    76,    77,
       8,    79,    80,    -1,    12,    -1,    14,    15,    -1,    17,
      18,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    62,    63,    -1,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    76,    77,
       8,    79,    80,    -1,    12,    -1,    14,    15,    -1,    17,
      18,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    62,    63,    -1,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    76,    77,
       8,    79,    80,    -1,    12,    -1,    14,    15,    -1,    17,
      18,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    62,    63,    -1,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    76,    77,
       8,    79,    80,    -1,    12,    -1,    14,    15,    -1,    17,
      18,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    62,    63,    -1,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    76,    77,
       8,    79,    80,    -1,    12,    -1,    14,    15,    -1,    17,
      18,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    62,    63,    -1,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    76,    77,
      -1,    79,    80,     6,     7,     8,     9,    -1,    -1,    -1,
      -1,    89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    27,    -1,    -1,    30,    -1,    -1,
      -1,    -1,    35,    36,    -1,    38,    39,    40,    41,    -1,
      -1,    -1,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    -1,    55,    -1,    -1,    -1,    -1,    60,    61,    62,
      63,    -1,    -1,    66,    67,    68,    69,    70,    71,    -1,
      73,    74,    75,    76,    77,    78,    79,    80,    81,     8,
      -1,    -1,    11,    86,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    27,    -1,
      -1,    30,    -1,    -1,    -1,    -1,    35,    36,    -1,    38,
      39,    40,    41,    -1,    -1,    -1,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    -1,    55,    -1,    -1,    -1,
      -1,    60,    61,    62,    63,    -1,     8,    66,    67,    68,
      69,    70,    71,    -1,    73,    74,    75,    76,    77,    78,
      79,    80,    -1,    -1,    -1,    27,    -1,    86,    30,    -1,
      -1,    -1,    -1,    35,    36,    -1,    38,    39,    40,    41,
      -1,    -1,    -1,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    -1,    55,    -1,    -1,    -1,    -1,    60,    61,
      62,    63,    -1,     8,    66,    67,    68,    69,    70,    71,
      -1,    73,    74,    75,    76,    77,    78,    79,    80,    -1,
      82,    -1,    27,    -1,    86,    30,    -1,    -1,    -1,    -1,
      35,    36,    -1,    38,    39,    40,    41,    -1,    -1,    -1,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    -1,
      55,    -1,    -1,    -1,    -1,    60,    61,    62,    63,    -1,
       8,    66,    67,    68,    69,    70,    71,    -1,    73,    74,
      75,    76,    77,    78,    79,    80,    -1,    -1,    -1,    -1,
      -1,    86,    30,    -1,    -1,    -1,    -1,    35,    36,    -1,
      -1,    39,    40,    -1,    -1,    -1,    -1,    45,    -1,    47,
      48,    49,    -1,    51,    -1,    53,    -1,    55,     8,    -1,
      -1,    -1,    -1,    61,    62,    63,    -1,    -1,    -1,    67,
      68,    69,    70,    -1,    -1,    -1,    74,    75,    76,    77,
      30,    79,    80,    -1,    -1,    35,    -1,    -1,    86,    -1,
      40,    -1,    -1,    -1,    -1,    45,    -1,    47,    -1,    49,
       8,    51,    -1,    53,    12,    55,    14,    15,    -1,    17,
      18,    61,    62,    63,    -1,    -1,    -1,    67,    -1,    69,
      70,    -1,    -1,    -1,    74,    -1,    76,    77,    -1,    79,
      80,    -1,    -1,    -1,    -1,    43,    86,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    62,    63,    -1,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    76,    77,
       8,    79,    80,    -1,    12,    -1,    14,    15,    -1,    17,
      18,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    62,    63,    -1,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    76,    77,
      -1,    79,    80
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    43,    93,    94,    95,    96,    97,   101,     4,     5,
      42,     0,     3,     4,     5,    95,   102,   104,   105,   106,
     107,   109,   110,   111,   114,   115,   118,   119,   123,    96,
     102,    44,   100,    82,    85,    13,    42,    44,    44,    80,
     104,   123,    82,    86,    86,    86,   117,    86,   117,    86,
      88,    88,    88,    88,    83,    98,    99,   100,    44,    72,
      73,    74,    75,    76,    77,    78,   108,    44,   108,    86,
       6,     7,     8,     9,    27,    30,    35,    36,    38,    39,
      40,    41,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    55,    60,    61,    62,    63,    66,    67,    68,    69,
      70,    71,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    86,   103,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   154,   155,   156,   157,    42,    87,
     112,   113,    42,    87,   116,    82,    87,   116,    82,    87,
     113,    95,    95,    95,    95,    40,    83,     8,    79,   103,
      85,    85,    87,   144,    86,   141,   142,   145,   146,   147,
      30,    86,   142,    30,    86,   141,    83,   152,   153,   153,
     153,   153,    86,    86,    86,    86,    86,    86,    86,    86,
      86,    86,    86,    86,    87,    85,   141,   142,   143,   144,
      87,    19,    20,    21,    22,    23,    24,    30,    31,    32,
      33,    34,    19,    20,    21,    22,    23,    24,    30,    31,
      32,    33,    34,    19,    20,    32,    19,    20,    25,    26,
      32,    85,    19,    20,    91,    19,    20,    91,    56,    57,
      58,    59,    45,    46,    47,    48,    49,    50,    55,    59,
      85,    87,   100,    85,    87,    87,    87,    12,    14,    15,
      17,    18,    45,    46,    47,    48,    49,    50,    65,    66,
      67,    68,    69,    70,    71,    72,    80,   120,   121,   122,
     125,   127,   128,   132,   133,   135,   138,   140,   148,   149,
     150,   151,   154,   155,   156,   157,   121,   122,   121,   122,
     122,    84,    40,    87,    40,    40,    88,   141,   142,   141,
      83,    11,   129,   130,   131,   141,   142,   143,   144,   145,
     146,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   129,    88,   129,    87,    87,    87,    87,   124,   141,
     141,   141,   141,   141,   141,   141,   141,   143,   144,   141,
     141,   142,   142,   142,   142,   142,   142,   142,   142,   143,
     142,   142,   143,   143,   141,   142,   143,   144,   144,   144,
     144,   144,   143,    40,   145,   147,   145,   146,   146,   112,
      42,    82,   126,   141,   142,   143,   144,   145,   146,    86,
      86,    82,    86,    90,    90,    28,    29,    90,    90,    90,
      90,    86,    86,    86,    86,    86,    86,    86,    86,    86,
      89,   121,   125,    82,    82,    16,    82,    90,    90,    90,
      90,    90,    90,    90,    90,    89,    89,   121,    89,    84,
      82,    95,    84,   141,    45,    46,    47,    48,    49,    50,
      85,    87,    87,    87,    87,    87,    87,    87,    87,    87,
      87,    87,    87,    95,    88,   100,    82,   144,   144,    40,
     145,   144,   141,   142,   146,   143,   129,   129,   129,   129,
     129,   129,   129,   129,   129,    89,    88,   134,   135,   141,
     144,   145,   146,   141,   144,   146,   145,    89,   122,    84,
     130,   122,    95,    87,    87,    87,    87,    87,    87,    87,
      87,    87,    87,    87,    87,   122,    89,    89,   122,   139,
     136,   137,    82,    82,    82,    89,    89,    88,    88,   125,
     122,   122,    89,    89
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* This macro is provided for backward compatibility. */

#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = 0;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                yysize1 = yysize + yytnamerr (0, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1806 of yacc.c  */
#line 1755 "script_parser.ypp"
    {
					unsigned int i, numArrays;
					SDWORD			size = 0, debug_i = 0, totalArraySize = 0;
					UDWORD			base;
					VAR_SYMBOL		*psCurr;
					TRIGGER_SYMBOL	*psTrig;
					EVENT_SYMBOL	*psEvent;
					UDWORD			numVars;

					RULE("script: var_list");

					// Calculate the code size
					for(psTrig = psTriggers; psTrig; psTrig = psTrig->psNext)
					{
						// Add the trigger code size
						size += psTrig->size;
						debug_i += psTrig->debugEntries;
					}

					for(psEvent = psEvents; psEvent; psEvent = psEvent->psNext)
					{
						// Add the trigger code size
						size += psEvent->size;
						debug_i += psEvent->debugEntries;
					}

					// Allocate the program
					numVars = psGlobalVars ? psGlobalVars->index+1 : 0;
					numArrays = psGlobalArrays ? psGlobalArrays->index+1 : 0;
					for(psCurr=psGlobalArrays; psCurr; psCurr=psCurr->psNext)
					{
						unsigned int arraySize = 1, dimension;
						for(dimension = 0; dimension < psCurr->dimensions; dimension++)
						{
							arraySize *= psCurr->elements[dimension];
						}
						totalArraySize += arraySize;
					}
					ALLOC_PROG(psFinalProg, size, psCurrBlock->pCode,
						numVars, numArrays, numTriggers, numEvents);

					//store local vars
					//allocate array for holding an array of local vars for each event
					psFinalProg->ppsLocalVars = (INTERP_TYPE **)malloc(sizeof(INTERP_TYPE*) * numEvents);
					psFinalProg->ppsLocalVarVal = NULL;
					psFinalProg->numLocalVars = (UDWORD *)malloc(sizeof(UDWORD) * numEvents);	//how many local vars each event has
					psFinalProg->numParams = (UDWORD *)malloc(sizeof(UDWORD) * numEvents);	//how many arguments each event has

					for(psEvent = psEvents, i = 0; psEvent; psEvent = psEvent->psNext, i++)
					{
						psEvent->numLocalVars = numEventLocalVars[i];

						psFinalProg->numLocalVars[i] = numEventLocalVars[i];	//remember how many local vars this event has
						psFinalProg->numParams[i] = psEvent->numParams;	//remember how many parameters this event has

						if(numEventLocalVars[i] > 0)
						{
							unsigned int j;
							INTERP_TYPE * pCurEvLocalVars = (INTERP_TYPE*)malloc(sizeof(INTERP_TYPE) * numEventLocalVars[i]);

							for(psCurr = psLocalVarsB[i], j = 0; psCurr != NULL; psCurr = psCurr->psNext, j++)
							{
								pCurEvLocalVars[numEventLocalVars[i] - j - 1] = psCurr->type;	//save type, order is reversed
							}

							psFinalProg->ppsLocalVars[i] = pCurEvLocalVars;
						}
						else
						{
							psFinalProg->ppsLocalVars[i] = NULL; //this event has no local vars
						}
					}

					ALLOC_DEBUG(psFinalProg, debug_i);
					psFinalProg->debugEntries = 0;
					ip = psFinalProg->pCode;

					// Add the trigger code
					for(psTrig = psTriggers, i = 0; psTrig; psTrig = psTrig->psNext, i++)
					{
						// Store the trigger offset
						psFinalProg->pTriggerTab[i] = (UWORD)(ip - psFinalProg->pCode);
						if (psTrig->pCode != NULL)
						{
							// Store the label
							DEBUG_LABEL(psFinalProg, psFinalProg->debugEntries,
								psTrig->pIdent);
							// Store debug info
							APPEND_DEBUG(psFinalProg, ip - psFinalProg->pCode, psTrig);
							// Store the code
							PUT_BLOCK(ip, psTrig);
							psFinalProg->psTriggerData[i].code = true;
						}
						else
						{
							psFinalProg->psTriggerData[i].code = false;
						}
						// Store the data
						psFinalProg->psTriggerData[i].type = psTrig->type;
						psFinalProg->psTriggerData[i].time = psTrig->time;
					}
					// Note the end of the final trigger
					psFinalProg->pTriggerTab[i] = (UWORD)(ip - psFinalProg->pCode);

					// Add the event code
					for(psEvent = psEvents, i = 0; psEvent; psEvent = psEvent->psNext, i++)
					{
						// Check the event was declared and has a code body
						if (psEvent->pCode == NULL)
						{
							scr_error("Event %s declared without being defined",
								psEvent->pIdent);
							YYABORT;
						}

						// Store the event offset
						psFinalProg->pEventTab[i] = (UWORD)(ip - psFinalProg->pCode);
						// Store the trigger link
						psFinalProg->pEventLinks[i] = (SWORD)(psEvent->trigger);
						// Store the label
						DEBUG_LABEL(psFinalProg, psFinalProg->debugEntries,
							psEvent->pIdent);
						// Store debug info
						APPEND_DEBUG(psFinalProg, ip - psFinalProg->pCode, psEvent);
						// Store the code
						PUT_BLOCK(ip, psEvent);
					}
					// Note the end of the final event
					psFinalProg->pEventTab[i] = (UWORD)(ip - psFinalProg->pCode);

					// Allocate debug info for the variables if necessary
					if (genDebugInfo)
					{
						if (numVars > 0)
						{
							psFinalProg->psVarDebug = (VAR_DEBUG *)malloc(sizeof(VAR_DEBUG) * numVars);
							if (psFinalProg->psVarDebug == NULL)
							{
								scr_error("Out of memory");
								YYABORT;
							}
						}
						else
						{
							psFinalProg->psVarDebug = NULL;
						}
						if (numArrays > 0)
						{
							psFinalProg->psArrayDebug = (ARRAY_DEBUG *)malloc(sizeof(ARRAY_DEBUG) * numArrays);
							if (psFinalProg->psArrayDebug == NULL)
							{
								scr_error("Out of memory");
								YYABORT;
							}
						}
						else
						{
							psFinalProg->psArrayDebug = NULL;
						}
					}
					else
					{
						psFinalProg->psVarDebug = NULL;
						psFinalProg->psArrayDebug = NULL;
					}

					/* Now set the types for the global variables */
					for(psCurr = psGlobalVars; psCurr != NULL; psCurr = psCurr->psNext)
					{
						unsigned int i = psCurr->index;
						psFinalProg->pGlobals[i] = psCurr->type;

						if (genDebugInfo)
						{
							psFinalProg->psVarDebug[i].pIdent = strdup(psCurr->pIdent);
							if (psFinalProg->psVarDebug[i].pIdent == NULL)
							{
								scr_error("Out of memory");
								YYABORT;
							}
							psFinalProg->psVarDebug[i].storage = psCurr->storage;
						}
					}

					/* Now store the array info */
					psFinalProg->arraySize = totalArraySize;
					for(psCurr = psGlobalArrays; psCurr != NULL; psCurr = psCurr->psNext)
					{
						unsigned int i = psCurr->index, dimension;

						psFinalProg->psArrayInfo[i].type = psCurr->type;
						psFinalProg->psArrayInfo[i].dimensions = (UBYTE)psCurr->dimensions;
						for(dimension = 0; dimension < psCurr->dimensions; dimension++)
						{
							psFinalProg->psArrayInfo[i].elements[dimension] = (UBYTE)psCurr->elements[dimension];
						}

						if (genDebugInfo)
						{
							psFinalProg->psArrayDebug[i].pIdent = strdup(psCurr->pIdent);
							if (psFinalProg->psArrayDebug[i].pIdent == NULL)
							{
								scr_error("Out of memory");
								YYABORT;
							}
							psFinalProg->psArrayDebug[i].storage = psCurr->storage;
						}
					}
					// calculate the base index of each array
					base = psFinalProg->numGlobals;
					for(i=0; i<numArrays; i++)
					{
						unsigned int arraySize = 1, dimension;

						psFinalProg->psArrayInfo[i].base = base;
						for(dimension = 0; dimension < psFinalProg->psArrayInfo[i].dimensions; dimension++)
						{
							arraySize *= psFinalProg->psArrayInfo[i].elements[dimension];
						}

						base += arraySize;
					}

					RULE("END script: var_list");
				}
    break;

  case 3:

/* Line 1806 of yacc.c  */
#line 1983 "script_parser.ypp"
    {
							RULE("script:var_list trigger_list");
						}
    break;

  case 4:

/* Line 1806 of yacc.c  */
#line 1987 "script_parser.ypp"
    {
							RULE("script:script event_list");
						}
    break;

  case 5:

/* Line 1806 of yacc.c  */
#line 1991 "script_parser.ypp"
    {
							RULE("script:script var_list");
						}
    break;

  case 6:

/* Line 1806 of yacc.c  */
#line 1995 "script_parser.ypp"
    {
							RULE("script:script trigger_list");
						}
    break;

  case 7:

/* Line 1806 of yacc.c  */
#line 2001 "script_parser.ypp"
    {
					RULE("var_list: NULL");
				}
    break;

  case 8:

/* Line 1806 of yacc.c  */
#line 2005 "script_parser.ypp"
    {
					RULE("var_list: var_line");
					FREE_VARDECL((yyvsp[(1) - (1)].vdecl));
				}
    break;

  case 9:

/* Line 1806 of yacc.c  */
#line 2010 "script_parser.ypp"
    {
					FREE_VARDECL((yyvsp[(2) - (2)].vdecl));
				}
    break;

  case 10:

/* Line 1806 of yacc.c  */
#line 2021 "script_parser.ypp"
    {
			/* remember that local var declaration is over */
			localVariableDef = false;
			//debug(LOG_SCRIPT, "localVariableDef = false 0");
			(yyval.vdecl) = (yyvsp[(1) - (2)].vdecl);
		}
    break;

  case 11:

/* Line 1806 of yacc.c  */
#line 2031 "script_parser.ypp"
    {
							//debug(LOG_SCRIPT, "variable_decl_head:		STORAGE TYPE");

							ALLOC_VARDECL(psCurrVDecl);
							psCurrVDecl->storage = (yyvsp[(1) - (2)].stype);
							psCurrVDecl->type = (yyvsp[(2) - (2)].tval);

							/* allow local vars to have the same names as global vars (don't check global vars) */
							if((yyvsp[(1) - (2)].stype) == ST_LOCAL)
							{
								localVariableDef = true;
								//debug(LOG_SCRIPT, "localVariableDef = true 0");
							}

							(yyval.vdecl) = psCurrVDecl;
							//debug(LOG_SCRIPT, "END variable_decl_head:		STORAGE TYPE (TYPE=%d)", $2);
						}
    break;

  case 12:

/* Line 1806 of yacc.c  */
#line 2049 "script_parser.ypp"
    {

							ALLOC_VARDECL(psCurrVDecl);
							psCurrVDecl->storage = (yyvsp[(1) - (2)].stype);
							psCurrVDecl->type = VAL_TRIGGER;

							(yyval.vdecl) = psCurrVDecl;
						}
    break;

  case 13:

/* Line 1806 of yacc.c  */
#line 2058 "script_parser.ypp"
    {
							ALLOC_VARDECL(psCurrVDecl);
							psCurrVDecl->storage = (yyvsp[(1) - (2)].stype);
							psCurrVDecl->type = VAL_EVENT;

							(yyval.vdecl) = psCurrVDecl;
						}
    break;

  case 14:

/* Line 1806 of yacc.c  */
#line 2068 "script_parser.ypp"
    {
						if ((yyvsp[(2) - (3)].ival) <= 0 || (yyvsp[(2) - (3)].ival) >= VAR_MAX_ELEMENTS)
						{
							scr_error("Invalid array size %d", (yyvsp[(2) - (3)].ival));
							YYABORT;
						}

						ALLOC_VARIDENTDECL(psCurrVIdentDecl, NULL, 1);
						psCurrVIdentDecl->elements[0] = (yyvsp[(2) - (3)].ival);

						(yyval.videcl) = psCurrVIdentDecl;
					}
    break;

  case 15:

/* Line 1806 of yacc.c  */
#line 2083 "script_parser.ypp"
    {
						(yyval.videcl) = (yyvsp[(1) - (1)].videcl);
					}
    break;

  case 16:

/* Line 1806 of yacc.c  */
#line 2088 "script_parser.ypp"
    {
						if ((yyvsp[(1) - (4)].videcl)->dimensions >= VAR_MAX_DIMENSIONS)
						{
							scr_error("Too many dimensions for array");
							YYABORT;
						}
						if ((yyvsp[(3) - (4)].ival) <= 0 || (yyvsp[(3) - (4)].ival) >= VAR_MAX_ELEMENTS)
						{
							scr_error("Invalid array size %d", (yyvsp[(3) - (4)].ival));
							YYABORT;
						}

						(yyvsp[(1) - (4)].videcl)->elements[(yyvsp[(1) - (4)].videcl)->dimensions] = (yyvsp[(3) - (4)].ival);
						(yyvsp[(1) - (4)].videcl)->dimensions += 1;

						(yyval.videcl) = (yyvsp[(1) - (4)].videcl);
					}
    break;

  case 17:

/* Line 1806 of yacc.c  */
#line 2108 "script_parser.ypp"
    {
						ALLOC_VARIDENTDECL(psCurrVIdentDecl, (yyvsp[(1) - (1)].sval), 0);

						(yyval.videcl) = psCurrVIdentDecl;
					}
    break;

  case 18:

/* Line 1806 of yacc.c  */
#line 2115 "script_parser.ypp"
    {
						(yyvsp[(2) - (2)].videcl)->pIdent = strdup((yyvsp[(1) - (2)].sval));
						if ((yyvsp[(2) - (2)].videcl)->pIdent == NULL)
						{
							scr_error("Out of memory");
							YYABORT;
						}

						(yyval.videcl) = (yyvsp[(2) - (2)].videcl);
					}
    break;

  case 19:

/* Line 1806 of yacc.c  */
#line 2128 "script_parser.ypp"
    {
						if (!scriptAddVariable((yyvsp[(1) - (2)].vdecl), (yyvsp[(2) - (2)].videcl)))
						{
							/* Out of memory - error already given */
							YYABORT;
						}

						freeVARIDENTDECL((yyvsp[(2) - (2)].videcl));

						/* return the variable type */
						(yyval.vdecl) = (yyvsp[(1) - (2)].vdecl);
					}
    break;

  case 20:

/* Line 1806 of yacc.c  */
#line 2141 "script_parser.ypp"
    {
						if (!scriptAddVariable((yyvsp[(1) - (3)].vdecl), (yyvsp[(3) - (3)].videcl)))
						{
							/* Out of memory - error already given */
							YYABORT;
						}

						freeVARIDENTDECL((yyvsp[(3) - (3)].videcl));

						/* return the variable type */
						(yyval.vdecl) = (yyvsp[(1) - (3)].vdecl);
					}
    break;

  case 24:

/* Line 1806 of yacc.c  */
#line 2166 "script_parser.ypp"
    {
						ALLOC_TSUBDECL(psCurrTDecl, TR_CODE, (yyvsp[(1) - (3)].cblock)->size, (yyvsp[(3) - (3)].ival));
						ip = psCurrTDecl->pCode;
						PUT_BLOCK(ip, (yyvsp[(1) - (3)].cblock));
						FREE_BLOCK((yyvsp[(1) - (3)].cblock));

						(yyval.tdecl) = psCurrTDecl;
					}
    break;

  case 25:

/* Line 1806 of yacc.c  */
#line 2175 "script_parser.ypp"
    {
						ALLOC_TSUBDECL(psCurrTDecl, TR_WAIT, 0, (yyvsp[(3) - (3)].ival));

						(yyval.tdecl) = psCurrTDecl;
					}
    break;

  case 26:

/* Line 1806 of yacc.c  */
#line 2181 "script_parser.ypp"
    {
						ALLOC_TSUBDECL(psCurrTDecl, TR_EVERY, 0, (yyvsp[(3) - (3)].ival));

						(yyval.tdecl) = psCurrTDecl;
					}
    break;

  case 27:

/* Line 1806 of yacc.c  */
#line 2187 "script_parser.ypp"
    {
						ALLOC_TSUBDECL(psCurrTDecl, TR_INIT, 0, 0);

						(yyval.tdecl) = psCurrTDecl;
					}
    break;

  case 28:

/* Line 1806 of yacc.c  */
#line 2193 "script_parser.ypp"
    {
						if ((yyvsp[(1) - (1)].cbSymbol)->numParams != 0)
						{
							scr_error("Expected parameters for callback trigger");
							YYABORT;
						}

						ALLOC_TSUBDECL(psCurrTDecl, (yyvsp[(1) - (1)].cbSymbol)->type, 0, 0);

						(yyval.tdecl) = psCurrTDecl;
					}
    break;

  case 29:

/* Line 1806 of yacc.c  */
#line 2205 "script_parser.ypp"
    {
						RULE("trigger_subdecl: CALLBACK_SYM ',' param_list");
						codeRet = scriptCodeCallbackParams((yyvsp[(1) - (3)].cbSymbol), (yyvsp[(3) - (3)].pblock), &psCurrTDecl);
						CHECK_CODE_ERROR(codeRet);

						(yyval.tdecl) = psCurrTDecl;
					}
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 2215 "script_parser.ypp"
    {
						SDWORD	line;
						char	*pDummy;

						scriptGetErrorData(&line, &pDummy);
						if (!scriptAddTrigger((yyvsp[(2) - (6)].sval), (yyvsp[(4) - (6)].tdecl), (UDWORD)line))
						{
							YYABORT;
						}
						FREE_TSUBDECL((yyvsp[(4) - (6)].tdecl));
					}
    break;

  case 33:

/* Line 1806 of yacc.c  */
#line 2238 "script_parser.ypp"
    {
						EVENT_SYMBOL	*psEvent;

						RULE("event_subdecl:		EVENT IDENT");

						if (!scriptDeclareEvent((yyvsp[(2) - (2)].sval), &psEvent,0))
						{
							YYABORT;
						}

						psCurEvent = psEvent;

						(yyval.eSymbol) = psEvent;

						//debug(LOG_SCRIPT, "END event_subdecl:		EVENT IDENT");
					}
    break;

  case 34:

/* Line 1806 of yacc.c  */
#line 2255 "script_parser.ypp"
    {

						RULE("EVENT EVENT_SYM");

						psCurEvent = (yyvsp[(2) - (2)].eSymbol);
						(yyval.eSymbol) = (yyvsp[(2) - (2)].eSymbol);

						//debug(LOG_SCRIPT, "END EVENT EVENT_SYM");
					}
    break;

  case 35:

/* Line 1806 of yacc.c  */
#line 2268 "script_parser.ypp"
    {

					RULE("function_def: lexFUNCTION TYPE function_type");

					psCurEvent = (yyvsp[(3) - (3)].eSymbol);

					/* check if this event was declared as function before */
					if(!(yyvsp[(3) - (3)].eSymbol)->bFunction)
					{
						debug(LOG_ERROR, "'%s' was declared as event before and can't be redefined to function", (yyvsp[(3) - (3)].eSymbol)->pIdent);
						scr_error("Wrong event definition");
						YYABORT;
					}

					(yyval.eSymbol) = (yyvsp[(3) - (3)].eSymbol);
				}
    break;

  case 43:

/* Line 1806 of yacc.c  */
#line 2297 "script_parser.ypp"
    {
						EVENT_SYMBOL	*psEvent;

						RULE("func_subdecl: lexFUNCTION TYPE IDENT");

						/* allow local vars to have the same names as global vars (don't check global vars) */
						localVariableDef = true;
						//debug(LOG_SCRIPT, "localVariableDef = true 1");

						if (!scriptDeclareEvent((yyvsp[(3) - (3)].sval), &psEvent,0))
						{
							YYABORT;
						}

						psEvent->retType = (yyvsp[(2) - (3)].tval);
						psCurEvent = psEvent;
						psCurEvent->bFunction = true;

						(yyval.eSymbol) = psEvent;

						//debug(LOG_SCRIPT, "END func_subdecl:lexFUNCTION TYPE IDENT. ");
					}
    break;

  case 44:

/* Line 1806 of yacc.c  */
#line 2324 "script_parser.ypp"
    {
							EVENT_SYMBOL	*psEvent;

							RULE("void_func_subdecl: lexFUNCTION _VOID IDENT");

							/* allow local vars to have the same names as global vars (don't check global vars) */
							localVariableDef = true;
							//debug(LOG_SCRIPT, "localVariableDef = true 1");

							if (!scriptDeclareEvent((yyvsp[(3) - (3)].sval), &psEvent,0))
							{
								YYABORT;
							}

							psEvent->retType = VAL_VOID;
							psCurEvent = psEvent;
							psCurEvent->bFunction = true;

							(yyval.eSymbol) = psEvent;

							//debug(LOG_SCRIPT, "END func_subdecl:lexFUNCTION TYPE IDENT. ");
						}
    break;

  case 45:

/* Line 1806 of yacc.c  */
#line 2349 "script_parser.ypp"
    {
							//debug(LOG_SCRIPT, "func_subdecl:lexFUNCTION EVENT_SYM ");
							psCurEvent = (yyvsp[(3) - (3)].eSymbol);


							/* check if this event was declared as function before */
							if(!(yyvsp[(3) - (3)].eSymbol)->bFunction)
							{
								debug(LOG_ERROR, "'%s' was declared as event before and can't be redefined to function", (yyvsp[(3) - (3)].eSymbol)->pIdent);
								scr_error("Wrong event definition");
								YYABORT;
							}

							/* psCurEvent->bFunction = true; */
							/* psEvent->retType = $2; */
							(yyval.eSymbol) = (yyvsp[(3) - (3)].eSymbol);
							//debug(LOG_SCRIPT, "func_subdecl:lexFUNCTION EVENT_SYM. ");
						}
    break;

  case 46:

/* Line 1806 of yacc.c  */
#line 2372 "script_parser.ypp"
    {
							(yyval.integer_val)=(yyvsp[(1) - (2)].tval);
						}
    break;

  case 47:

/* Line 1806 of yacc.c  */
#line 2376 "script_parser.ypp"
    {
							(yyval.integer_val)=(yyvsp[(1) - (2)].tval);
						}
    break;

  case 48:

/* Line 1806 of yacc.c  */
#line 2380 "script_parser.ypp"
    {
							(yyval.integer_val)=(yyvsp[(1) - (2)].tval);
						}
    break;

  case 49:

/* Line 1806 of yacc.c  */
#line 2384 "script_parser.ypp"
    {
							(yyval.integer_val)=(yyvsp[(1) - (2)].tval);
						}
    break;

  case 50:

/* Line 1806 of yacc.c  */
#line 2388 "script_parser.ypp"
    {
							(yyval.integer_val)=(yyvsp[(1) - (2)].tval);
						}
    break;

  case 51:

/* Line 1806 of yacc.c  */
#line 2392 "script_parser.ypp"
    {
							(yyval.integer_val)=(yyvsp[(1) - (2)].tval);
						}
    break;

  case 52:

/* Line 1806 of yacc.c  */
#line 2396 "script_parser.ypp"
    {
							(yyval.integer_val)=(yyvsp[(1) - (2)].tval);
						}
    break;

  case 53:

/* Line 1806 of yacc.c  */
#line 2400 "script_parser.ypp"
    {
							(yyval.integer_val)=(yyvsp[(1) - (2)].tval);
						}
    break;

  case 54:

/* Line 1806 of yacc.c  */
#line 2407 "script_parser.ypp"
    {
					if(!checkFuncParamType(0, (yyvsp[(1) - (1)].integer_val)))
					{
						scr_error("Wrong event argument definition in '%s'", psCurEvent->pIdent);
						YYABORT;
					}

					//debug(LOG_SCRIPT, "funcbody_var_def_body=%d \n", $1);
					(yyval.integer_val)=1;
				}
    break;

  case 55:

/* Line 1806 of yacc.c  */
#line 2418 "script_parser.ypp"
    {
					if(!checkFuncParamType((yyvsp[(1) - (3)].integer_val), (yyvsp[(3) - (3)].integer_val)))
					{
						scr_error("Wrong event argument definition");
						YYABORT;
					}

					//debug(LOG_SCRIPT, "funcbody_var_def_body2=%d \n", $3);
					(yyval.integer_val)=(yyvsp[(1) - (3)].integer_val)+1;
				}
    break;

  case 56:

/* Line 1806 of yacc.c  */
#line 2432 "script_parser.ypp"
    {

						RULE("funcbody_var_def: '(' funcvar_decl_types ')'");

						/* remember that local var declaration is over */
						localVariableDef = false;

						if(psCurEvent == NULL)
						{
							scr_error("psCurEvent == NULL");
							YYABORT;
						}

						if(!psCurEvent->bDeclared)
						{
							debug(LOG_ERROR, "Event %s's definition doesn't match with declaration.", psCurEvent->pIdent);
							scr_error("Wrong event definition:\n event %s's definition doesn't match with declaration", psCurEvent->pIdent);
							YYABORT;
						}

						/* check if number of parameter in body defenition match number of params in the declaration */
						if((yyvsp[(3) - (4)].integer_val) != psCurEvent->numParams)
						{
							scr_error("Wrong number of arguments in function definition (or declaration-definition argument type/names mismatch) \n in event: '%s'", psCurEvent->pIdent);
							YYABORT;
						}

						(yyval.eSymbol) = (yyvsp[(1) - (4)].eSymbol);
					}
    break;

  case 57:

/* Line 1806 of yacc.c  */
#line 2462 "script_parser.ypp"
    {

						RULE( "funcbody_var_def: funcbody_var_def_body '(' ')'");

						/* remember that local var declaration is over */
						localVariableDef = false;

						if(psCurEvent == NULL)
						{
							scr_error("psCurEvent == NULL");
							YYABORT;
						}

						if(!psCurEvent->bDeclared)
						{
							debug(LOG_ERROR, "Event %s's definition doesn't match with declaration.", psCurEvent->pIdent);
							scr_error("Wrong event definition:\n event %s's definition doesn't match with declaration", psCurEvent->pIdent);
							YYABORT;
						}

						/* check if number of parameter in body defenition match number of params in the declaration */
						if(0 != psCurEvent->numParams)
						{
							scr_error("Wrong number of arguments in function definition (or declaration-definition argument type/names mismatch) \n in event: '%s'", psCurEvent->pIdent);
							YYABORT;
						}

						(yyval.eSymbol) = (yyvsp[(1) - (3)].eSymbol);
					}
    break;

  case 58:

/* Line 1806 of yacc.c  */
#line 2497 "script_parser.ypp"
    {

						RULE( "funcbody_var_def: '(' funcvar_decl_types ')'");

						/* remember that local var declaration is over */
						localVariableDef = false;

						if(psCurEvent == NULL)
						{
							scr_error("psCurEvent == NULL");
							YYABORT;
						}

						if(!psCurEvent->bDeclared)
						{
							debug(LOG_ERROR, "Event %s's definition doesn't match with declaration.", psCurEvent->pIdent);
							scr_error("Wrong event definition:\n event %s's definition doesn't match with declaration", psCurEvent->pIdent);
							YYABORT;
						}

						/* check if number of parameter in body defenition match number of params in the declaration */
						if((yyvsp[(3) - (4)].integer_val) != psCurEvent->numParams)
						{
							scr_error("Wrong number of arguments in function definition (or declaration-definition argument type/names mismatch) \n in event: '%s'", psCurEvent->pIdent);
							YYABORT;
						}

						(yyval.eSymbol) = (yyvsp[(1) - (4)].eSymbol);
					}
    break;

  case 59:

/* Line 1806 of yacc.c  */
#line 2527 "script_parser.ypp"
    {

						RULE( "funcbody_var_def: funcbody_var_def_body '(' ')'");

						/* remember that local var declaration is over */
						localVariableDef = false;

						if(psCurEvent == NULL)
						{
							scr_error("psCurEvent == NULL");
							YYABORT;
						}

						if(!psCurEvent->bDeclared)
						{
							debug(LOG_ERROR, "Event %s's definition doesn't match with declaration.", psCurEvent->pIdent);
							scr_error("Wrong event definition:\n event %s's definition doesn't match with declaration", psCurEvent->pIdent);
							YYABORT;
						}

						/* check if number of parameter in body defenition match number of params in the declaration */
						if(0 != psCurEvent->numParams)
						{
							scr_error("Wrong number of arguments in function definition (or declaration-definition argument type/names mismatch) \n in event: '%s'", psCurEvent->pIdent);
							YYABORT;
						}

						(yyval.eSymbol) = (yyvsp[(1) - (3)].eSymbol);
					}
    break;

  case 60:

/* Line 1806 of yacc.c  */
#line 2562 "script_parser.ypp"
    {

					RULE( "argument_decl_head: TYPE variable_ident");


					/* handle type part */
					ALLOC_VARDECL(psCurrVDecl);
					psCurrVDecl->storage =ST_LOCAL;	/* can only be local */
					psCurrVDecl->type = (yyvsp[(1) - (2)].tval);

					/* handle ident part */
					if (!scriptAddVariable(psCurrVDecl, (yyvsp[(2) - (2)].videcl)))
					{
						YYABORT;
					}

					freeVARIDENTDECL((yyvsp[(2) - (2)].videcl));

					FREE_VARDECL(psCurrVDecl);

					/* return the variable type */
					/* $$ = psCurrVDecl; */ /* not needed? */

					if(psCurEvent == NULL)
						debug(LOG_ERROR, "argument_decl_head 0:  - psCurEvent == NULL");

					psCurEvent->numParams = psCurEvent->numParams + 1;	/* remember a parameter was declared */

					/* remember parameter type */
					psCurEvent->aParams[0] = (yyvsp[(1) - (2)].tval);

					//debug(LOG_SCRIPT, "argument_decl_head 0. ");
				}
    break;

  case 61:

/* Line 1806 of yacc.c  */
#line 2596 "script_parser.ypp"
    {
					//debug(LOG_SCRIPT, "argument_decl_head 1 ");

					/* handle type part */
					ALLOC_VARDECL(psCurrVDecl);
					psCurrVDecl->storage =ST_LOCAL;	/* can only be local */
					psCurrVDecl->type = (yyvsp[(3) - (4)].tval);

					/* remember parameter type */
					psCurEvent->aParams[psCurEvent->numParams] = (yyvsp[(3) - (4)].tval);
					//debug(LOG_SCRIPT, "argument_decl_head 10 ");

					/* handle ident part */
					if (!scriptAddVariable(psCurrVDecl, (yyvsp[(4) - (4)].videcl)))
					{
						YYABORT;
					}
					//debug(LOG_SCRIPT, "argument_decl_head 11 ");
					freeVARIDENTDECL((yyvsp[(4) - (4)].videcl));
					FREE_VARDECL(psCurrVDecl);

					/* return the variable type */
					/* $$ = psCurrVDecl; */ /* not needed? */

					if(psCurEvent == NULL)
						debug(LOG_ERROR, "argument_decl_head 0:  - psCurEvent == NULL");
					psCurEvent->numParams = psCurEvent->numParams + 1;	/* remember a parameter was declared */

					//debug(LOG_SCRIPT, "argument_decl_head 1. ");
				}
    break;

  case 63:

/* Line 1806 of yacc.c  */
#line 2629 "script_parser.ypp"
    {
					/* remember that local var declaration is over */
					localVariableDef = false;
					//debug(LOG_SCRIPT, "localVariableDef = false 1");
				}
    break;

  case 64:

/* Line 1806 of yacc.c  */
#line 2637 "script_parser.ypp"
    {
					RULE( "function_declaration: func_subdecl '(' argument_decl_head ')'");

					/* remember that local var declaration is over */
					localVariableDef = false;
					//debug(LOG_SCRIPT, "localVariableDef = false 2");

					if(psCurEvent->bDeclared) /* can only occur if different (new) var names are used in the event definition that don't match with declaration*/
					{
						scr_error("Wrong event definition: \nEvent %s's definition doesn't match with declaration", psCurEvent->pIdent);
						YYABORT;
					}

					(yyval.eSymbol) = (yyvsp[(1) - (4)].eSymbol);
				}
    break;

  case 65:

/* Line 1806 of yacc.c  */
#line 2653 "script_parser.ypp"
    {
					RULE( "function_declaration: func_subdecl '(' ')'");

					/* remember that local var declaration is over */
					localVariableDef = false;
					//debug(LOG_SCRIPT, "localVariableDef = false 3");

					if((yyvsp[(1) - (3)].eSymbol)->numParams > 0 && psCurEvent->bDeclared) /* can only occur if no parameters or different (new) var names are used in the event definition that don't match with declaration */
					{
						scr_error("Wrong event definition: \nEvent %s's definition doesn't match with declaration", psCurEvent->pIdent);
						YYABORT;
					}

					(yyval.eSymbol) = (yyvsp[(1) - (3)].eSymbol);
				}
    break;

  case 66:

/* Line 1806 of yacc.c  */
#line 2674 "script_parser.ypp"
    {
					/* remember that local var declaration is over */
					localVariableDef = false;
					//debug(LOG_SCRIPT, "localVariableDef = false 2");

					if(psCurEvent->bDeclared) /* can only occur if different (new) var names are used in the event definition that don't match with declaration*/
					{
						scr_error("Wrong event definition: \nEvent %s's definition doesn't match with declaration", psCurEvent->pIdent);
						YYABORT;
					}

					(yyval.eSymbol) = (yyvsp[(1) - (4)].eSymbol);
				}
    break;

  case 67:

/* Line 1806 of yacc.c  */
#line 2688 "script_parser.ypp"
    {

						RULE( "void_function_declaration: void_func_subdecl '(' ')'");

					/* remember that local var declaration is over */
					localVariableDef = false;
					//debug(LOG_SCRIPT, "localVariableDef = false 3");

					if((yyvsp[(1) - (3)].eSymbol)->numParams > 0 && psCurEvent->bDeclared) /* can only occur if no parameters or different (new) var names are used in the event definition that don't match with declaration */
					{
						scr_error("Wrong event definition: \nEvent %s's definition doesn't match with declaration", psCurEvent->pIdent);
						YYABORT;
					}

					(yyval.eSymbol) = (yyvsp[(1) - (3)].eSymbol);
				}
    break;

  case 69:

/* Line 1806 of yacc.c  */
#line 2712 "script_parser.ypp"
    {

						RULE( "return_statement: return_statement_void");

						if(psCurEvent == NULL)	/* no events declared or defined yet */
						{
							scr_error("return statement outside of function");
							YYABORT;
						}

						if(!psCurEvent->bFunction)
						{
							scr_error("return statement inside of an event '%s'", psCurEvent->pIdent);
							YYABORT;
						}

						if(psCurEvent->retType != VAL_VOID)
						{
							scr_error("wrong return statement syntax for a non-void function '%s'", psCurEvent->pIdent);
							YYABORT;
						}

						/* Allocate code block for exit instruction */
						ALLOC_BLOCK(psCurrBlock, 1);	//1 for opcode
						ip = psCurrBlock->pCode;
						PUT_OPCODE(ip, OP_EXIT);

						psCurrBlock->type = VAL_VOID;	/* make return statement of type VOID manually */

						(yyval.cblock) = psCurrBlock;
					}
    break;

  case 70:

/* Line 1806 of yacc.c  */
#line 2744 "script_parser.ypp"
    {

						RULE( "return_statement: RET return_exp ';'");

						if(psCurEvent == NULL)	/* no events declared or defined yet */
						{
							debug(LOG_ERROR, "return statement outside of function");
							YYABORT;
						}

						if(!psCurEvent->bFunction)
						{
							debug(LOG_ERROR, "return statement inside of an event '%s'", psCurEvent->pIdent);
							YYABORT;
						}

						if(psCurEvent->retType != (yyvsp[(2) - (3)].cblock)->type)
						{
							if(!interpCheckEquiv(psCurEvent->retType, (yyvsp[(2) - (3)].cblock)->type))
							{
								debug(LOG_ERROR, "return type mismatch");
								debug(LOG_ERROR, "wrong return statement syntax for function '%s' (%d - %d)", psCurEvent->pIdent, psCurEvent->retType, (yyvsp[(2) - (3)].cblock)->type);
								YYABORT;
							}
						}

						/* Allocate code block for exit instruction */
						ALLOC_BLOCK(psCurrBlock, (yyvsp[(2) - (3)].cblock)->size + 1);	//+1 for OPCODE

						ip = psCurrBlock->pCode;

						PUT_BLOCK(ip, (yyvsp[(2) - (3)].cblock));

						PUT_OPCODE(ip, OP_EXIT);

						/* store the type of the exp */
						psCurrBlock->type = (yyvsp[(2) - (3)].cblock)->type;

						FREE_BLOCK((yyvsp[(2) - (3)].cblock));

						(yyval.cblock) = psCurrBlock;

						//debug(LOG_SCRIPT, "END RET  return_exp  ';'");
					}
    break;

  case 71:

/* Line 1806 of yacc.c  */
#line 2792 "script_parser.ypp"
    {
							RULE( "statement_list: NULL");

							// Allocate a dummy code block
							ALLOC_BLOCK(psCurrBlock, 1);
							psCurrBlock->size = 0;

							(yyval.cblock) = psCurrBlock;
						}
    break;

  case 72:

/* Line 1806 of yacc.c  */
#line 2802 "script_parser.ypp"
    {
							RULE("statement_list: statement");
							(yyval.cblock) = (yyvsp[(1) - (1)].cblock);
						}
    break;

  case 73:

/* Line 1806 of yacc.c  */
#line 2807 "script_parser.ypp"
    {
							RULE("statement_list: statement_list statement");

							ALLOC_BLOCK(psCurrBlock, (yyvsp[(1) - (2)].cblock)->size + (yyvsp[(2) - (2)].cblock)->size);

							ALLOC_DEBUG(psCurrBlock, (yyvsp[(1) - (2)].cblock)->debugEntries +
													 (yyvsp[(2) - (2)].cblock)->debugEntries);
							ip = psCurrBlock->pCode;

							/* Copy the two code blocks */
							PUT_BLOCK(ip, (yyvsp[(1) - (2)].cblock));
							PUT_BLOCK(ip, (yyvsp[(2) - (2)].cblock));
							PUT_DEBUG(psCurrBlock, (yyvsp[(1) - (2)].cblock));
							APPEND_DEBUG(psCurrBlock, (yyvsp[(1) - (2)].cblock)->size / sizeof(INTERP_VAL), (yyvsp[(2) - (2)].cblock));

							ASSERT((yyvsp[(1) - (2)].cblock) != NULL, "$1 == NULL");
							ASSERT((yyvsp[(1) - (2)].cblock)->psDebug != NULL, "psDebug == NULL");

							FREE_DEBUG((yyvsp[(1) - (2)].cblock));
							FREE_DEBUG((yyvsp[(2) - (2)].cblock));
							FREE_BLOCK((yyvsp[(1) - (2)].cblock));
							FREE_BLOCK((yyvsp[(2) - (2)].cblock));

							/* Return the code block */
							(yyval.cblock) = psCurrBlock;
						}
    break;

  case 74:

/* Line 1806 of yacc.c  */
#line 2837 "script_parser.ypp"
    {
						RULE( "event_decl: event_subdecl ';'");

						psCurEvent->bDeclared = true;
					}
    break;

  case 75:

/* Line 1806 of yacc.c  */
#line 2843 "script_parser.ypp"
    {
						//debug(LOG_SCRIPT, "localVariableDef = false new ");
						localVariableDef = false;
						psCurEvent->bDeclared = true;
					}
    break;

  case 76:

/* Line 1806 of yacc.c  */
#line 2849 "script_parser.ypp"
    {
						RULE( "event_decl: void_func_subdecl argument_decl ';'");

						//debug(LOG_SCRIPT, "localVariableDef = false new ");
						localVariableDef = false;
						psCurEvent->bDeclared = true;
					}
    break;

  case 77:

/* Line 1806 of yacc.c  */
#line 2857 "script_parser.ypp"
    {
						RULE( "event_decl: event_subdecl '(' TRIG_SYM ')'");

						/* make sure this event is not declared as function */
						if(psCurEvent->bFunction)
						{
							debug(LOG_ERROR, "Event '%s' is declared as function and can't have a trigger assigned", psCurEvent->pIdent);
							scr_error("Wrong event definition");
							YYABORT;
						}

						/* pop required number of paramerets passed to this event (if any) */
						/* popArguments($1, $7, &psCurrBlock); */

						/* if (!scriptDefineEvent($1, $6, $3->index)) */
						if (!scriptDefineEvent((yyvsp[(1) - (8)].eSymbol), (yyvsp[(7) - (8)].cblock), (yyvsp[(3) - (8)].tSymbol)->index))
						{
							YYABORT;
						}

						/* end of event */
						psCurEvent = NULL;

						FREE_DEBUG((yyvsp[(7) - (8)].cblock));
						FREE_BLOCK((yyvsp[(7) - (8)].cblock));

						RULE( "END event_decl: event_subdecl '(' TRIG_SYM ')'");

					}
    break;

  case 78:

/* Line 1806 of yacc.c  */
#line 2887 "script_parser.ypp"
    {
						// Get the line for the implicit trigger declaration
						char	*pDummy;

						RULE( "event_decl:event_subdecl '(' trigger_subdecl ')' ");

						/* make sure this event is not declared as function */
						if(psCurEvent->bFunction)
						{
							debug(LOG_ERROR, "Event '%s' is declared as function and can't have a trigger assigned", psCurEvent->pIdent);
							scr_error("Wrong event definition");
							YYABORT;
						}

						scriptGetErrorData((SDWORD *)&debugLine, &pDummy);
					}
    break;

  case 79:

/* Line 1806 of yacc.c  */
#line 2904 "script_parser.ypp"
    {
						RULE("event_decl: '{' var_list statement_list '}'");

						// Create a trigger for this event
						if (!scriptAddTrigger("", (yyvsp[(3) - (9)].tdecl), debugLine))
						{
							YYABORT;
						}
						FREE_TSUBDECL((yyvsp[(3) - (9)].tdecl));

						/* if (!scriptDefineEvent($1, $7, numTriggers - 1)) */
						if (!scriptDefineEvent((yyvsp[(1) - (9)].eSymbol), (yyvsp[(8) - (9)].cblock), numTriggers - 1))
						{
							YYABORT;
						}

						/* end of event */
						psCurEvent = NULL;

						FREE_DEBUG((yyvsp[(8) - (9)].cblock));
						FREE_BLOCK((yyvsp[(8) - (9)].cblock));

						RULE( "END event_decl:event_subdecl '(' trigger_subdecl ')' .");
					}
    break;

  case 80:

/* Line 1806 of yacc.c  */
#line 2931 "script_parser.ypp"
    {
						RULE( "event_subdecl '(' INACTIVE ')' '{' var_list statement_list '}'");

						/* make sure this event is not declared as function */
						if(psCurEvent->bFunction)
						{
							debug(LOG_ERROR, "Event '%s' is declared as function and can't have a trigger assigned", psCurEvent->pIdent);
							scr_error("Wrong event definition");
							YYABORT;
						}

						if (!scriptDefineEvent((yyvsp[(1) - (8)].eSymbol), (yyvsp[(7) - (8)].cblock), -1))
						{
							YYABORT;
						}

						/* end of event */
						psCurEvent = NULL;

						FREE_DEBUG((yyvsp[(7) - (8)].cblock));
						FREE_BLOCK((yyvsp[(7) - (8)].cblock));

						RULE( "END event_subdecl '(' INACTIVE ')' '{' var_list statement_list '}'");
					}
    break;

  case 81:

/* Line 1806 of yacc.c  */
#line 2958 "script_parser.ypp"
    {
						RULE( "void_function_declaration  '{' var_list statement_list  '}'");

						/* stays the same if no params (just gets copied) */
						ALLOC_BLOCK(psCurrBlock, (yyvsp[(4) - (5)].cblock)->size + 1 + (yyvsp[(1) - (5)].eSymbol)->numParams); /* statement_list + opcode + numParams */
						ip = psCurrBlock->pCode;

						/* pop required number of paramerets passed to this event (if any) */
						popArguments(&ip, (yyvsp[(1) - (5)].eSymbol)->numParams);

						/* Copy the two code blocks */
						PUT_BLOCK(ip, (yyvsp[(4) - (5)].cblock));
						PUT_OPCODE(ip, OP_EXIT);		/* must exit after return */

						ALLOC_DEBUG(psCurrBlock, (yyvsp[(4) - (5)].cblock)->debugEntries);
						PUT_DEBUG(psCurrBlock, (yyvsp[(4) - (5)].cblock));

						if (!scriptDefineEvent((yyvsp[(1) - (5)].eSymbol), psCurrBlock, -1))
						{
							YYABORT;
						}

						FREE_DEBUG((yyvsp[(4) - (5)].cblock));
						FREE_BLOCK((yyvsp[(4) - (5)].cblock));

						/* end of event */
						psCurEvent = NULL;

						/* free block since code was copied in scriptDefineEvent() */
						FREE_DEBUG(psCurrBlock);
						FREE_BLOCK(psCurrBlock);

						RULE( "END void_function_declaration  '{' var_list statement_list  '}'");
					}
    break;

  case 82:

/* Line 1806 of yacc.c  */
#line 2995 "script_parser.ypp"
    {

						RULE( "void_funcbody_var_def '{' var_list statement_list '}'");

						/* stays the same if no params (just gets copied) */
						ALLOC_BLOCK(psCurrBlock, (yyvsp[(4) - (5)].cblock)->size + 1 + (yyvsp[(1) - (5)].eSymbol)->numParams);	/* statements + opcode + numparams */
						ip = psCurrBlock->pCode;

						/* pop required number of paramerets passed to this event (if any) */
						popArguments(&ip, (yyvsp[(1) - (5)].eSymbol)->numParams);

						/* Copy the old (main) code and free it */
						PUT_BLOCK(ip, (yyvsp[(4) - (5)].cblock));
						PUT_OPCODE(ip, OP_EXIT);		/* must exit after return */

						/* copy debug info */
						ALLOC_DEBUG(psCurrBlock, (yyvsp[(4) - (5)].cblock)->debugEntries);
						PUT_DEBUG(psCurrBlock, (yyvsp[(4) - (5)].cblock));

						if (!scriptDefineEvent((yyvsp[(1) - (5)].eSymbol), psCurrBlock, -1))
						{
							YYABORT;
						}

						FREE_DEBUG((yyvsp[(4) - (5)].cblock));
						FREE_BLOCK((yyvsp[(4) - (5)].cblock));
						FREE_DEBUG(psCurrBlock);
						FREE_BLOCK(psCurrBlock);

						/* end of event */
						psCurEvent = NULL;

						RULE( "END void_func_subdecl '(' funcbody_var_def_body ')' '{' var_list statement_list '}'");
					}
    break;

  case 83:

/* Line 1806 of yacc.c  */
#line 3032 "script_parser.ypp"
    {
						RULE( "function_declaration  '{' var_list statement_list return_statement  '}'");

						/* stays the same if no params (just gets copied) */
						//ALLOC_BLOCK(psCurrBlock, $4->size + $5->size + sizeof(OPCODE) + (sizeof(OPCODE) * $1->numParams)); /* statement_list + expression + EXIT + numParams */
						ALLOC_BLOCK(psCurrBlock, (yyvsp[(4) - (6)].cblock)->size + (yyvsp[(5) - (6)].cblock)->size + 1 + (yyvsp[(1) - (6)].eSymbol)->numParams); /* statement_list + return_expr + EXIT opcode + numParams */
						ip = psCurrBlock->pCode;

						/* pop required number of paramerets passed to this event (if any) */
						popArguments(&ip, (yyvsp[(1) - (6)].eSymbol)->numParams);

						/* Copy the two code blocks */
						PUT_BLOCK(ip, (yyvsp[(4) - (6)].cblock));

						PUT_BLOCK(ip, (yyvsp[(5) - (6)].cblock));

						PUT_OPCODE(ip, OP_EXIT);		/* must exit after return */

						ALLOC_DEBUG(psCurrBlock, (yyvsp[(4) - (6)].cblock)->debugEntries);

						PUT_DEBUG(psCurrBlock, (yyvsp[(4) - (6)].cblock));

						if (!scriptDefineEvent((yyvsp[(1) - (6)].eSymbol), psCurrBlock, -1))
						{
							YYABORT;
						}

						FREE_DEBUG((yyvsp[(4) - (6)].cblock));
						FREE_BLOCK((yyvsp[(4) - (6)].cblock));
						FREE_BLOCK((yyvsp[(5) - (6)].cblock));

						/* end of event */
						psCurEvent = NULL;

						/* free block since code was copied in scriptDefineEvent() */
						FREE_DEBUG(psCurrBlock);
						FREE_BLOCK(psCurrBlock);

						RULE( "END function_declaration  '{' var_list statement_list return_statement  '}'");
					}
    break;

  case 84:

/* Line 1806 of yacc.c  */
#line 3075 "script_parser.ypp"
    {
						RULE( "func_subdecl '(' funcbody_var_def_body ')' '{' var_list statement_list return_statement '}'");

						/* stays the same if no params (just gets copied) */
						//ALLOC_BLOCK(psCurrBlock, $4->size + $5->size + sizeof(OPCODE) + (sizeof(OPCODE) * $1->numParams));
						ALLOC_BLOCK(psCurrBlock, (yyvsp[(4) - (6)].cblock)->size + (yyvsp[(5) - (6)].cblock)->size + 1 + (yyvsp[(1) - (6)].eSymbol)->numParams);	/* statements + return expr + opcode + numParams */
						ip = psCurrBlock->pCode;

						/* pop required number of paramerets passed to this event (if any) */
						popArguments(&ip, (yyvsp[(1) - (6)].eSymbol)->numParams);

						/* Copy the old (main) code and free it */
						PUT_BLOCK(ip, (yyvsp[(4) - (6)].cblock));
						PUT_BLOCK(ip, (yyvsp[(5) - (6)].cblock));
						PUT_OPCODE(ip, OP_EXIT);		/* must exit after return */

						/* copy debug info */
						ALLOC_DEBUG(psCurrBlock, (yyvsp[(4) - (6)].cblock)->debugEntries);
						PUT_DEBUG(psCurrBlock, (yyvsp[(4) - (6)].cblock));

						RULE( "debug entries %d", (yyvsp[(4) - (6)].cblock)->debugEntries);

						if (!scriptDefineEvent((yyvsp[(1) - (6)].eSymbol), psCurrBlock, -1))
						{
							YYABORT;
						}

						FREE_DEBUG((yyvsp[(4) - (6)].cblock));
						FREE_BLOCK((yyvsp[(4) - (6)].cblock));
						FREE_BLOCK((yyvsp[(5) - (6)].cblock));
						FREE_DEBUG(psCurrBlock);
						FREE_BLOCK(psCurrBlock);

						/* end of event */
						psCurEvent = NULL;

						RULE( "END func_subdecl '(' funcbody_var_def_body ')' '{' var_list statement_list return_statement '}'");
					}
    break;

  case 85:

/* Line 1806 of yacc.c  */
#line 3116 "script_parser.ypp"
    {
						RULE( "funcbody_var_def '{' var_list return_statement '}'");

						/* stays the same if no params (just gets copied) */
						ALLOC_BLOCK(psCurrBlock, (yyvsp[(4) - (5)].cblock)->size + 1 + (yyvsp[(1) - (5)].eSymbol)->numParams);	/*  return expr + opcode + numParams */
						ip = psCurrBlock->pCode;

						/* pop required number of paramerets passed to this event (if any) */
						popArguments(&ip, (yyvsp[(1) - (5)].eSymbol)->numParams);

						/* Copy the old (main) code and free it */
						PUT_BLOCK(ip, (yyvsp[(4) - (5)].cblock));
						PUT_OPCODE(ip, OP_EXIT);		/* must exit after return */

						if (!scriptDefineEvent((yyvsp[(1) - (5)].eSymbol), psCurrBlock, -1))
						{
							YYABORT;
						}

						FREE_BLOCK((yyvsp[(4) - (5)].cblock));
						FREE_BLOCK(psCurrBlock);
						psCurEvent = NULL;

						RULE( "END funcbody_var_def '{' var_list return_statement '}'");
					}
    break;

  case 86:

/* Line 1806 of yacc.c  */
#line 3143 "script_parser.ypp"
    {
						RULE( "function_declaration  '{' var_list statement_list return_statement  '}'");

						/* stays the same if no params (just gets copied) */
						ALLOC_BLOCK(psCurrBlock, (yyvsp[(4) - (5)].cblock)->size + 1 + (yyvsp[(1) - (5)].eSymbol)->numParams); /* return_expr + EXIT opcode + numParams */
						ip = psCurrBlock->pCode;

						/* pop required number of paramerets passed to this event (if any) */
						popArguments(&ip, (yyvsp[(1) - (5)].eSymbol)->numParams);

						/* Copy code block */
						PUT_BLOCK(ip, (yyvsp[(4) - (5)].cblock));
						PUT_OPCODE(ip, OP_EXIT);		/* must exit after return */

						if (!scriptDefineEvent((yyvsp[(1) - (5)].eSymbol), psCurrBlock, -1))
						{
							YYABORT;
						}

						FREE_BLOCK((yyvsp[(4) - (5)].cblock));
						psCurEvent = NULL;
						FREE_BLOCK(psCurrBlock);

						RULE( "END function_declaration  '{' var_list statement_list return_statement  '}'");
					}
    break;

  case 87:

/* Line 1806 of yacc.c  */
#line 3176 "script_parser.ypp"
    {
						UDWORD line;
						char *pDummy;

						RULE("statement: assignment");

						/* Put in debugging info */
						if (genDebugInfo)
						{
							ALLOC_DEBUG((yyvsp[(1) - (2)].cblock), 1);
							(yyvsp[(1) - (2)].cblock)->psDebug[0].offset = 0;
							scriptGetErrorData((SDWORD *)&line, &pDummy);
							(yyvsp[(1) - (2)].cblock)->psDebug[0].line = line;
						}

						(yyval.cblock) = (yyvsp[(1) - (2)].cblock);
					}
    break;

  case 88:

/* Line 1806 of yacc.c  */
#line 3194 "script_parser.ypp"
    {
						UDWORD line;
						char *pDummy;

						RULE("statement: inc_dec_exp");

						/* Put in debugging info */
						if (genDebugInfo)
						{
							ALLOC_DEBUG((yyvsp[(1) - (2)].cblock), 1);
							(yyvsp[(1) - (2)].cblock)->psDebug[0].offset = 0;
							scriptGetErrorData((SDWORD *)&line, &pDummy);
							(yyvsp[(1) - (2)].cblock)->psDebug[0].line = line;
						}

						(yyval.cblock) = (yyvsp[(1) - (2)].cblock);
					}
    break;

  case 89:

/* Line 1806 of yacc.c  */
#line 3212 "script_parser.ypp"
    {
						UDWORD line;
						char *pDummy;

						RULE("statement: func_call ';'");

						/* Put in debugging info */
						if (genDebugInfo)
						{
							ALLOC_DEBUG((yyvsp[(1) - (2)].cblock), 1);
							(yyvsp[(1) - (2)].cblock)->psDebug[0].offset = 0;
							scriptGetErrorData((SDWORD *)&line, &pDummy);
							(yyvsp[(1) - (2)].cblock)->psDebug[0].line = line;
						}

						(yyval.cblock) = (yyvsp[(1) - (2)].cblock);
					}
    break;

  case 90:

/* Line 1806 of yacc.c  */
#line 3230 "script_parser.ypp"
    {
						UDWORD line,paramNumber;
						char *pDummy;

						RULE( "statement: VOID_FUNC_CUST '(' param_list ')'  ';'");

						/* allow to call EVENTs to reuse the code only if no actual parameters are specified in function call, like "myEvent();" */
						if(!(yyvsp[(1) - (5)].eSymbol)->bFunction && (yyvsp[(3) - (5)].pblock)->numParams > 0)
						{
							scr_error("Can't pass any parameters in an event call:\nEvent: '%s'", (yyvsp[(1) - (5)].eSymbol)->pIdent);
							return CE_PARSE;
						}

						if((yyvsp[(3) - (5)].pblock)->numParams != (yyvsp[(1) - (5)].eSymbol)->numParams)
						{
							scr_error("Wrong number of arguments for function call: '%s'. Expected %d parameters instead of  %d.", (yyvsp[(1) - (5)].eSymbol)->pIdent, (yyvsp[(1) - (5)].eSymbol)->numParams, (yyvsp[(3) - (5)].pblock)->numParams);
							return CE_PARSE;
						}

						/* check if right parameters were passed */
						paramNumber = checkFuncParamTypes((yyvsp[(1) - (5)].eSymbol), (yyvsp[(3) - (5)].pblock));
						if(paramNumber > 0)
						{
							debug(LOG_ERROR, "Parameter mismatch in function call: '%s'. Mismatch in parameter  %d.", (yyvsp[(1) - (5)].eSymbol)->pIdent, paramNumber);
							YYABORT;
						}

						/* Allocate the code block */
						ALLOC_BLOCK(psCurrBlock, (yyvsp[(3) - (5)].pblock)->size + 1 + 1);	//paramList + opcode + event index
						ALLOC_DEBUG(psCurrBlock, 1);
						ip = psCurrBlock->pCode;

						if((yyvsp[(3) - (5)].pblock)->numParams > 0)	/* if any parameters declared */
						{
							/* Copy in the code for the parameters */
							PUT_BLOCK(ip, (yyvsp[(3) - (5)].pblock));
						}

						FREE_PBLOCK((yyvsp[(3) - (5)].pblock));

						/* Store the instruction */
						PUT_OPCODE(ip, OP_FUNC);
						PUT_EVENT(ip,(yyvsp[(1) - (5)].eSymbol)->index);			//Put event index

						/* Add the debugging information */
						if (genDebugInfo)
						{
							psCurrBlock->psDebug[0].offset = 0;
							scriptGetErrorData((SDWORD *)&line, &pDummy);
							psCurrBlock->psDebug[0].line = line;
						}

						(yyval.cblock) = psCurrBlock;
					}
    break;

  case 91:

/* Line 1806 of yacc.c  */
#line 3285 "script_parser.ypp"
    {
						UDWORD line;
						char *pDummy;

						RULE( "statement: EVENT_SYM '(' param_list ')'  ';'");

						/* allow to call EVENTs to reuse the code only if no actual parameters are specified in function call, like "myEvent();" */
						if(!(yyvsp[(1) - (5)].eSymbol)->bFunction && (yyvsp[(3) - (5)].pblock)->numParams > 0)
						{
							scr_error("Can't pass any parameters in an event call:\nEvent: '%s'", (yyvsp[(1) - (5)].eSymbol)->pIdent);
							return CE_PARSE;
						}

						if((yyvsp[(3) - (5)].pblock)->numParams != (yyvsp[(1) - (5)].eSymbol)->numParams)
						{
							scr_error("Wrong number of arguments for function call: '%s'. Expected %d parameters instead of  %d.", (yyvsp[(1) - (5)].eSymbol)->pIdent, (yyvsp[(1) - (5)].eSymbol)->numParams, (yyvsp[(3) - (5)].pblock)->numParams);
							return CE_PARSE;
						}

						/* Allocate the code block */
						//ALLOC_BLOCK(psCurrBlock, $3->size + sizeof(OPCODE) + sizeof(UDWORD));	//Params + Opcode + event index
						ALLOC_BLOCK(psCurrBlock, (yyvsp[(3) - (5)].pblock)->size + 1 + 1);	//Params + Opcode + event index
						ALLOC_DEBUG(psCurrBlock, 1);
						ip = psCurrBlock->pCode;

						if((yyvsp[(3) - (5)].pblock)->numParams > 0)	/* if any parameters declared */
						{
							/* Copy in the code for the parameters */
							PUT_BLOCK(ip, (yyvsp[(3) - (5)].pblock));
						}

						FREE_PBLOCK((yyvsp[(3) - (5)].pblock));

							/* Store the instruction */
						PUT_OPCODE(ip, OP_FUNC);
						PUT_EVENT(ip,(yyvsp[(1) - (5)].eSymbol)->index);			//Put event index as VAL_EVENT

						/* Add the debugging information */
						if (genDebugInfo)
						{
							psCurrBlock->psDebug[0].offset = 0;
							scriptGetErrorData((SDWORD *)&line, &pDummy);
							psCurrBlock->psDebug[0].line = line;
						}

						(yyval.cblock) = psCurrBlock;
					}
    break;

  case 92:

/* Line 1806 of yacc.c  */
#line 3333 "script_parser.ypp"
    {
						(yyval.cblock) = (yyvsp[(1) - (1)].cblock);
					}
    break;

  case 93:

/* Line 1806 of yacc.c  */
#line 3337 "script_parser.ypp"
    {
						(yyval.cblock) = (yyvsp[(1) - (1)].cblock);
					}
    break;

  case 94:

/* Line 1806 of yacc.c  */
#line 3341 "script_parser.ypp"
    {
						UDWORD line;
						char *pDummy;

						/* Allocate the code block */
						ALLOC_BLOCK(psCurrBlock, 1);	//1 - Exit opcode
						ALLOC_DEBUG(psCurrBlock, 1);
						ip = psCurrBlock->pCode;

						/* Store the instruction */
						PUT_OPCODE(ip, OP_EXIT);

						/* Add the debugging information */
						if (genDebugInfo)
						{
							psCurrBlock->psDebug[0].offset = 0;
							scriptGetErrorData((SDWORD *)&line, &pDummy);
							psCurrBlock->psDebug[0].line = line;
						}

						(yyval.cblock) = psCurrBlock;
					}
    break;

  case 95:

/* Line 1806 of yacc.c  */
#line 3364 "script_parser.ypp"
    {
						UDWORD line;
						char *pDummy;

						RULE( "statement: return_statement");

						if(psCurEvent == NULL)
						{
							scr_error("'return' outside of function body");
							YYABORT;
						}

						if(!psCurEvent->bFunction)
						{
							debug(LOG_ERROR, "'return' can only be used in functions, not in events, event: '%s'", psCurEvent->pIdent);
							scr_error("'return' in event");
							YYABORT;
						}

						if((yyvsp[(1) - (1)].cblock)->type != psCurEvent->retType)
						{
							debug(LOG_ERROR, "'return' type doesn't match with function return type, function: '%s' (%d / %d)", psCurEvent->pIdent, (yyvsp[(1) - (1)].cblock)->type, psCurEvent->retType);
							scr_error("'return' type mismatch 0");
							YYABORT;
						}

						ALLOC_BLOCK(psCurrBlock, (yyvsp[(1) - (1)].cblock)->size + 1);	//return statement + EXIT opcode
						ALLOC_DEBUG(psCurrBlock, 1);
						ip = psCurrBlock->pCode;

						PUT_BLOCK(ip, (yyvsp[(1) - (1)].cblock));
						PUT_OPCODE(ip, OP_EXIT);

						FREE_BLOCK((yyvsp[(1) - (1)].cblock));

						if (genDebugInfo)
						{
							psCurrBlock->psDebug[0].offset = 0;
							scriptGetErrorData((SDWORD *)&line, &pDummy);
							psCurrBlock->psDebug[0].line = line;
						}

						(yyval.cblock) = psCurrBlock;
					}
    break;

  case 96:

/* Line 1806 of yacc.c  */
#line 3409 "script_parser.ypp"
    {
						UDWORD line;
						char *pDummy;

						// can only have a positive pause
						if ((yyvsp[(3) - (5)].ival) < 0)
						{
							scr_error("Invalid pause time");
							YYABORT;
						}

						/* Allocate the code block */
						ALLOC_BLOCK(psCurrBlock, 1);	//1 - for OP_PAUSE
						ALLOC_DEBUG(psCurrBlock, 1);
						ip = psCurrBlock->pCode;

						/* Store the instruction */
						PUT_PKOPCODE(ip, OP_PAUSE, (yyvsp[(3) - (5)].ival));

						/* Add the debugging information */
						if (genDebugInfo)
						{
							psCurrBlock->psDebug[0].offset = 0;
							scriptGetErrorData((SDWORD *)&line, &pDummy);
							psCurrBlock->psDebug[0].line = line;
						}

						(yyval.cblock) = psCurrBlock;
					}
    break;

  case 97:

/* Line 1806 of yacc.c  */
#line 3442 "script_parser.ypp"
    {
					RULE("return_exp: expression");

					/* Just pass the code up the tree */
					(yyvsp[(1) - (1)].cblock)->type = VAL_INT;
					(yyval.cblock) = (yyvsp[(1) - (1)].cblock);
				}
    break;

  case 98:

/* Line 1806 of yacc.c  */
#line 3450 "script_parser.ypp"
    {
					RULE("return_exp: floatexp");

					/* Just pass the code up the tree */
					(yyvsp[(1) - (1)].cblock)->type = VAL_FLOAT;
					(yyval.cblock) = (yyvsp[(1) - (1)].cblock);
				}
    break;

  case 99:

/* Line 1806 of yacc.c  */
#line 3458 "script_parser.ypp"
    {
					RULE( "return_exp: stringexp");

					/* Just pass the code up the tree */
					(yyvsp[(1) - (1)].cblock)->type = VAL_STRING;
					(yyval.cblock) = (yyvsp[(1) - (1)].cblock);
				}
    break;

  case 100:

/* Line 1806 of yacc.c  */
#line 3466 "script_parser.ypp"
    {
					RULE( "return_exp: boolexp");

					/* Just pass the code up the tree */
					(yyvsp[(1) - (1)].cblock)->type = VAL_BOOL;
					(yyval.cblock) = (yyvsp[(1) - (1)].cblock);
				}
    break;

  case 101:

/* Line 1806 of yacc.c  */
#line 3474 "script_parser.ypp"
    {
					RULE( "return_exp: objexp");

					/* Just pass the code up the tree */
					/* $1->type =  */
					(yyval.cblock) = (yyvsp[(1) - (1)].cblock);
				}
    break;

  case 102:

/* Line 1806 of yacc.c  */
#line 3482 "script_parser.ypp"
    {
					RULE( "return_exp: userexp");

					/* Just pass the code up the tree */
					/* $1->type =  */
					(yyval.cblock) = (yyvsp[(1) - (1)].cblock);
				}
    break;

  case 103:

/* Line 1806 of yacc.c  */
#line 3496 "script_parser.ypp"
    {
							RULE( "assignment: NUM_VAR '=' expression");

							codeRet = scriptCodeAssignment((yyvsp[(1) - (3)].vSymbol), (yyvsp[(3) - (3)].cblock), &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							/* Return the code block */
							(yyval.cblock) = psCurrBlock;
						}
    break;

  case 104:

/* Line 1806 of yacc.c  */
#line 3506 "script_parser.ypp"
    {
							RULE( "assignment: BOOL_VAR '=' boolexp");

							codeRet = scriptCodeAssignment((yyvsp[(1) - (3)].vSymbol), (yyvsp[(3) - (3)].cblock), &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							/* Return the code block */
							(yyval.cblock) = psCurrBlock;
						}
    break;

  case 105:

/* Line 1806 of yacc.c  */
#line 3516 "script_parser.ypp"
    {
							RULE( "assignment: FLOAT_VAR '=' floatexp");

							codeRet = scriptCodeAssignment((yyvsp[(1) - (3)].vSymbol), (yyvsp[(3) - (3)].cblock), &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							/* Return the code block */
							(yyval.cblock) = psCurrBlock;
						}
    break;

  case 106:

/* Line 1806 of yacc.c  */
#line 3526 "script_parser.ypp"
    {
							RULE("assignment: STRING_VAR '=' stringexp");

							codeRet = scriptCodeAssignment((yyvsp[(1) - (3)].vSymbol), (yyvsp[(3) - (3)].cblock), &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							/* Return the code block */
							(yyval.cblock) = psCurrBlock;
						}
    break;

  case 107:

/* Line 1806 of yacc.c  */
#line 3536 "script_parser.ypp"
    {
							RULE("assignment: OBJ_VAR '=' objexp");

							if (!interpCheckEquiv((yyvsp[(1) - (3)].vSymbol)->type, (yyvsp[(3) - (3)].cblock)->type))
							{
								scr_error("User type mismatch for assignment (%d - %d) 4", (yyvsp[(1) - (3)].vSymbol)->type, (yyvsp[(3) - (3)].cblock)->type);
								YYABORT;
							}
							codeRet = scriptCodeAssignment((yyvsp[(1) - (3)].vSymbol), (yyvsp[(3) - (3)].cblock), &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							/* Return the code block */
							(yyval.cblock) = psCurrBlock;
						}
    break;

  case 108:

/* Line 1806 of yacc.c  */
#line 3551 "script_parser.ypp"
    {
							RULE( "assignment: VAR '=' userexp");

							if (!interpCheckEquiv((yyvsp[(1) - (3)].vSymbol)->type, (yyvsp[(3) - (3)].cblock)->type))
							{
								scr_error("User type mismatch for assignment (%d - %d) 3", (yyvsp[(1) - (3)].vSymbol)->type, (yyvsp[(3) - (3)].cblock)->type);
								YYABORT;
							}
							codeRet = scriptCodeAssignment((yyvsp[(1) - (3)].vSymbol), (CODE_BLOCK *)(yyvsp[(3) - (3)].cblock), &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							/* Return the code block */
							(yyval.cblock) = psCurrBlock;
						}
    break;

  case 109:

/* Line 1806 of yacc.c  */
#line 3566 "script_parser.ypp"
    {
							RULE( "assignment: num_objvar '=' expression");

							codeRet = scriptCodeObjAssignment((yyvsp[(1) - (3)].objVarBlock), (yyvsp[(3) - (3)].cblock), &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							/* Return the code block */
							(yyval.cblock) = psCurrBlock;
						}
    break;

  case 110:

/* Line 1806 of yacc.c  */
#line 3576 "script_parser.ypp"
    {
							RULE( "assignment: bool_objvar '=' boolexp");

							codeRet = scriptCodeObjAssignment((yyvsp[(1) - (3)].objVarBlock), (yyvsp[(3) - (3)].cblock), &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							/* Return the code block */
							(yyval.cblock) = psCurrBlock;
						}
    break;

  case 111:

/* Line 1806 of yacc.c  */
#line 3586 "script_parser.ypp"
    {
							RULE( "assignment: user_objvar '=' userexp");

							if (!interpCheckEquiv((yyvsp[(1) - (3)].objVarBlock)->psObjVar->type,(yyvsp[(3) - (3)].cblock)->type))
							{
								scr_error("User type mismatch for assignment (%d - %d) 2", (yyvsp[(1) - (3)].objVarBlock)->psObjVar->type, (yyvsp[(3) - (3)].cblock)->type);
								YYABORT;
							}
							codeRet = scriptCodeObjAssignment((yyvsp[(1) - (3)].objVarBlock), (CODE_BLOCK *)(yyvsp[(3) - (3)].cblock), &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							/* Return the code block */
							(yyval.cblock) = psCurrBlock;
						}
    break;

  case 112:

/* Line 1806 of yacc.c  */
#line 3601 "script_parser.ypp"
    {
							RULE( "assignment: obj_objvar '=' objexp");

							if (!interpCheckEquiv((yyvsp[(1) - (3)].objVarBlock)->psObjVar->type, (yyvsp[(3) - (3)].cblock)->type))
							{
								scr_error("User type mismatch for assignment (%d - %d) 1", (yyvsp[(1) - (3)].objVarBlock)->psObjVar->type, (yyvsp[(3) - (3)].cblock)->type);
								YYABORT;
							}
							codeRet = scriptCodeObjAssignment((yyvsp[(1) - (3)].objVarBlock), (yyvsp[(3) - (3)].cblock), &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							/* Return the code block */
							(yyval.cblock) = psCurrBlock;
						}
    break;

  case 113:

/* Line 1806 of yacc.c  */
#line 3616 "script_parser.ypp"
    {
							RULE( "assignment: num_array_var '=' expression");

							codeRet = scriptCodeArrayAssignment((yyvsp[(1) - (3)].arrayBlock), (yyvsp[(3) - (3)].cblock), &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							/* Return the code block */
							(yyval.cblock) = psCurrBlock;
						}
    break;

  case 114:

/* Line 1806 of yacc.c  */
#line 3626 "script_parser.ypp"
    {
							RULE( "assignment: bool_array_var '=' boolexp");

							codeRet = scriptCodeArrayAssignment((yyvsp[(1) - (3)].arrayBlock), (yyvsp[(3) - (3)].cblock), &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							/* Return the code block */
							(yyval.cblock) = psCurrBlock;
						}
    break;

  case 115:

/* Line 1806 of yacc.c  */
#line 3636 "script_parser.ypp"
    {
							RULE( "assignment: user_array_var '=' userexp");

							if (!interpCheckEquiv((yyvsp[(1) - (3)].arrayBlock)->psArrayVar->type,(yyvsp[(3) - (3)].cblock)->type))
							{
								scr_error("User type mismatch for assignment (%d - %d) 0", (yyvsp[(1) - (3)].arrayBlock)->psArrayVar->type, (yyvsp[(3) - (3)].cblock)->type);
								YYABORT;
							}
							codeRet = scriptCodeArrayAssignment((yyvsp[(1) - (3)].arrayBlock), (CODE_BLOCK *)(yyvsp[(3) - (3)].cblock), &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							/* Return the code block */
							(yyval.cblock) = psCurrBlock;
						}
    break;

  case 116:

/* Line 1806 of yacc.c  */
#line 3651 "script_parser.ypp"
    {
							RULE( "assignment: obj_array_var '=' objexp");

							if (!interpCheckEquiv((yyvsp[(1) - (3)].arrayBlock)->psArrayVar->type, (yyvsp[(3) - (3)].cblock)->type))
							{
								scr_error("User type mismatch for assignment");
								YYABORT;
							}
							codeRet = scriptCodeArrayAssignment((yyvsp[(1) - (3)].arrayBlock), (yyvsp[(3) - (3)].cblock), &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							/* Return the code block */
							(yyval.cblock) = psCurrBlock;
						}
    break;

  case 117:

/* Line 1806 of yacc.c  */
#line 3674 "script_parser.ypp"
    {
						RULE( "func_call: NUM_FUNC '(' param_list ')'");

						/* Generate the code for the function call */
						codeRet = scriptCodeFunction((yyvsp[(1) - (4)].fSymbol), (yyvsp[(3) - (4)].pblock), false, &psCurrBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.cblock) = psCurrBlock;
					}
    break;

  case 118:

/* Line 1806 of yacc.c  */
#line 3685 "script_parser.ypp"
    {
						RULE( "func_call: BOOL_FUNC '(' param_list ')'");

						/* Generate the code for the function call */
						codeRet = scriptCodeFunction((yyvsp[(1) - (4)].fSymbol), (yyvsp[(3) - (4)].pblock), false, &psCurrBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.cblock) = psCurrBlock;
					}
    break;

  case 119:

/* Line 1806 of yacc.c  */
#line 3696 "script_parser.ypp"
    {
						RULE( "func_call: USER_FUNC '(' param_list ')'");

						/* Generate the code for the function call */
						codeRet = scriptCodeFunction((yyvsp[(1) - (4)].fSymbol), (yyvsp[(3) - (4)].pblock), false, &psCurrBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.cblock) = psCurrBlock;
					}
    break;

  case 120:

/* Line 1806 of yacc.c  */
#line 3707 "script_parser.ypp"
    {
						RULE( "func_call: OBJ_FUNC '(' param_list ')'");

						/* Generate the code for the function call */
						codeRet = scriptCodeFunction((yyvsp[(1) - (4)].fSymbol), (yyvsp[(3) - (4)].pblock), false, &psCurrBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.cblock) = psCurrBlock;
					}
    break;

  case 121:

/* Line 1806 of yacc.c  */
#line 3718 "script_parser.ypp"
    {
						RULE("func_call: FUNC '(' param_list ')'");

						/* Generate the code for the function call */
						codeRet = scriptCodeFunction((yyvsp[(1) - (4)].fSymbol), (yyvsp[(3) - (4)].pblock), false, &psCurrBlock);

						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.cblock) = psCurrBlock;
					}
    break;

  case 122:

/* Line 1806 of yacc.c  */
#line 3730 "script_parser.ypp"
    {
						RULE( "func_call: STRING_FUNC '(' param_list ')'");

						/* Generate the code for the function call */
						codeRet = scriptCodeFunction((yyvsp[(1) - (4)].fSymbol), (yyvsp[(3) - (4)].pblock), false, &psCurrBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.cblock) = psCurrBlock;
					}
    break;

  case 123:

/* Line 1806 of yacc.c  */
#line 3741 "script_parser.ypp"
    {
						RULE( "func_call: FLOAT_FUNC '(' param_list ')'");

						/* Generate the code for the function call */
						codeRet = scriptCodeFunction((yyvsp[(1) - (4)].fSymbol), (yyvsp[(3) - (4)].pblock), false, &psCurrBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.cblock) = psCurrBlock;
					}
    break;

  case 124:

/* Line 1806 of yacc.c  */
#line 3763 "script_parser.ypp"
    {
						/* create a dummy pblock containing nothing */
						//ALLOC_PBLOCK(psCurrPBlock, sizeof(UDWORD), 1);
						ALLOC_PBLOCK(psCurrPBlock, 1, 1);	//1 is enough
						psCurrPBlock->size = 0;
						psCurrPBlock->numParams = 0;

						(yyval.pblock) = psCurrPBlock;
					}
    break;

  case 125:

/* Line 1806 of yacc.c  */
#line 3773 "script_parser.ypp"
    {
						RULE("param_list: parameter");
						(yyval.pblock) = (yyvsp[(1) - (1)].pblock);
					}
    break;

  case 126:

/* Line 1806 of yacc.c  */
#line 3778 "script_parser.ypp"
    {
						RULE("param_list: param_list ',' parameter");

						ALLOC_PBLOCK(psCurrPBlock, (yyvsp[(1) - (3)].pblock)->size + (yyvsp[(3) - (3)].pblock)->size, (yyvsp[(1) - (3)].pblock)->numParams + (yyvsp[(3) - (3)].pblock)->numParams);
						ip = psCurrPBlock->pCode;

						/* Copy in the code for the parameters */
						PUT_BLOCK(ip, (yyvsp[(1) - (3)].pblock));
						PUT_BLOCK(ip, (yyvsp[(3) - (3)].pblock));

						/* Copy the parameter types */
						memcpy(psCurrPBlock->aParams, (yyvsp[(1) - (3)].pblock)->aParams,
								(yyvsp[(1) - (3)].pblock)->numParams * sizeof(INTERP_TYPE));
						memcpy(psCurrPBlock->aParams + (yyvsp[(1) - (3)].pblock)->numParams,
								(yyvsp[(3) - (3)].pblock)->aParams,
								(yyvsp[(3) - (3)].pblock)->numParams * sizeof(INTERP_TYPE));

						/* Free the old pblocks */
						FREE_PBLOCK((yyvsp[(1) - (3)].pblock));
						FREE_PBLOCK((yyvsp[(3) - (3)].pblock));

						/* return the pblock */
						(yyval.pblock) = psCurrPBlock;
					}
    break;

  case 127:

/* Line 1806 of yacc.c  */
#line 3804 "script_parser.ypp"
    {
						RULE("parameter: expression");

						/* Generate the code for the parameter                     */
						codeRet = scriptCodeParameter((yyvsp[(1) - (1)].cblock), VAL_INT, &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.pblock) = psCurrPBlock;
					}
    break;

  case 128:

/* Line 1806 of yacc.c  */
#line 3815 "script_parser.ypp"
    {
						RULE("parameter: boolexp (boolexp size: %d)", (yyvsp[(1) - (1)].cblock)->size);

						/* Generate the code for the parameter */
						codeRet = scriptCodeParameter((yyvsp[(1) - (1)].cblock), VAL_BOOL, &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.pblock) = psCurrPBlock;
					}
    break;

  case 129:

/* Line 1806 of yacc.c  */
#line 3826 "script_parser.ypp"
    {
						RULE("parameter: floatexp");

						/* Generate the code for the parameter */
						codeRet = scriptCodeParameter((yyvsp[(1) - (1)].cblock), VAL_FLOAT, &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.pblock) = psCurrPBlock;
					}
    break;

  case 130:

/* Line 1806 of yacc.c  */
#line 3837 "script_parser.ypp"
    {
						RULE("parameter: stringexp");

						/* Generate the code for the parameter */
						codeRet = scriptCodeParameter((yyvsp[(1) - (1)].cblock), VAL_STRING, &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.pblock) = psCurrPBlock;
					}
    break;

  case 131:

/* Line 1806 of yacc.c  */
#line 3848 "script_parser.ypp"
    {
						RULE("parameter: userexp");

						/* Generate the code for the parameter */
						codeRet = scriptCodeParameter((CODE_BLOCK *)(yyvsp[(1) - (1)].cblock), (yyvsp[(1) - (1)].cblock)->type, &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.pblock) = psCurrPBlock;
					}
    break;

  case 132:

/* Line 1806 of yacc.c  */
#line 3859 "script_parser.ypp"
    {
						RULE( "parameter: objexp");

						/* Generate the code for the parameter */
						codeRet = scriptCodeParameter((yyvsp[(1) - (1)].cblock), (yyvsp[(1) - (1)].cblock)->type, &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.pblock) = psCurrPBlock;
					}
    break;

  case 133:

/* Line 1806 of yacc.c  */
#line 3870 "script_parser.ypp"
    {
						/* just pass the variable reference up the tree */
						(yyval.pblock) = (yyvsp[(1) - (1)].pblock);
					}
    break;

  case 134:

/* Line 1806 of yacc.c  */
#line 3877 "script_parser.ypp"
    {
						codeRet = scriptCodeVarRef((yyvsp[(2) - (2)].vSymbol), &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.pblock) = psCurrPBlock;
					}
    break;

  case 135:

/* Line 1806 of yacc.c  */
#line 3885 "script_parser.ypp"
    {
						codeRet = scriptCodeVarRef((yyvsp[(2) - (2)].vSymbol), &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.pblock) = psCurrPBlock;
					}
    break;

  case 136:

/* Line 1806 of yacc.c  */
#line 3893 "script_parser.ypp"
    {
						codeRet = scriptCodeVarRef((yyvsp[(2) - (2)].vSymbol), &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.pblock) = psCurrPBlock;
					}
    break;

  case 137:

/* Line 1806 of yacc.c  */
#line 3901 "script_parser.ypp"
    {
						codeRet = scriptCodeVarRef((yyvsp[(2) - (2)].vSymbol), &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.pblock) = psCurrPBlock;
					}
    break;

  case 138:

/* Line 1806 of yacc.c  */
#line 3909 "script_parser.ypp"
    {
						codeRet = scriptCodeVarRef((yyvsp[(2) - (2)].vSymbol), &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.pblock) = psCurrPBlock;
					}
    break;

  case 139:

/* Line 1806 of yacc.c  */
#line 3917 "script_parser.ypp"
    {
						codeRet = scriptCodeVarRef((yyvsp[(2) - (2)].vSymbol), &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.pblock) = psCurrPBlock;
					}
    break;

  case 140:

/* Line 1806 of yacc.c  */
#line 3932 "script_parser.ypp"
    {
						RULE( "conditional: cond_clause_list");

						codeRet = scriptCodeConditional((yyvsp[(1) - (1)].condBlock), &psCurrBlock);
						CHECK_CODE_ERROR(codeRet);

						(yyval.cblock) = psCurrBlock;
					}
    break;

  case 141:

/* Line 1806 of yacc.c  */
#line 3941 "script_parser.ypp"
    {
						RULE( "conditional: cond_clause_list ELSE terminal_cond");

						ALLOC_CONDBLOCK(psCondBlock, (yyvsp[(1) - (3)].condBlock)->numOffsets + (yyvsp[(3) - (3)].condBlock)->numOffsets, (yyvsp[(1) - (3)].condBlock)->size + (yyvsp[(3) - (3)].condBlock)->size);
						ALLOC_DEBUG(psCondBlock, (yyvsp[(1) - (3)].condBlock)->debugEntries + (yyvsp[(3) - (3)].condBlock)->debugEntries);
						ip = psCondBlock->pCode;

						/* Store the two blocks of code */
						PUT_BLOCK(ip, (yyvsp[(1) - (3)].condBlock));
						PUT_BLOCK(ip, (yyvsp[(3) - (3)].condBlock));

						/* Copy over the offset information       */
						/* (There isn't any in the terminal_cond) */
						memcpy(psCondBlock->aOffsets, (yyvsp[(1) - (3)].condBlock)->aOffsets,
							   (yyvsp[(1) - (3)].condBlock)->numOffsets * sizeof(SDWORD));
						psCondBlock->numOffsets = (yyvsp[(1) - (3)].condBlock)->numOffsets;

						/* Put in the debugging information */
						PUT_DEBUG(psCondBlock, (yyvsp[(1) - (3)].condBlock));
						APPEND_DEBUG(psCondBlock, (yyvsp[(1) - (3)].condBlock)->size, (yyvsp[(3) - (3)].condBlock));

						/* Free the code blocks */
						FREE_DEBUG((yyvsp[(1) - (3)].condBlock));
						FREE_DEBUG((yyvsp[(3) - (3)].condBlock));
						FREE_CONDBLOCK((yyvsp[(1) - (3)].condBlock));
						FREE_CONDBLOCK((yyvsp[(3) - (3)].condBlock));

						/* Do the final processing of the conditional */
						codeRet = scriptCodeConditional(psCondBlock, &psCurrBlock);
						CHECK_CODE_ERROR(codeRet);

						(yyval.cblock) = psCurrBlock;
					}
    break;

  case 142:

/* Line 1806 of yacc.c  */
#line 3977 "script_parser.ypp"
    {
						RULE( "cond_clause_list:	cond_clause");

						(yyval.condBlock) = (yyvsp[(1) - (1)].condBlock);
					}
    break;

  case 143:

/* Line 1806 of yacc.c  */
#line 3983 "script_parser.ypp"
    {
						RULE( "cond_clause_list:	cond_clause_list ELSE cond_clause");

						ALLOC_CONDBLOCK(psCondBlock,
										(yyvsp[(1) - (3)].condBlock)->numOffsets + (yyvsp[(3) - (3)].condBlock)->numOffsets,
										(yyvsp[(1) - (3)].condBlock)->size + (yyvsp[(3) - (3)].condBlock)->size);
						ALLOC_DEBUG(psCondBlock, (yyvsp[(1) - (3)].condBlock)->debugEntries + (yyvsp[(3) - (3)].condBlock)->debugEntries);
						ip = psCondBlock->pCode;

						/* Store the two blocks of code */
						PUT_BLOCK(ip, (yyvsp[(1) - (3)].condBlock));
						PUT_BLOCK(ip, (yyvsp[(3) - (3)].condBlock));

						/* Copy over the offset information */
						memcpy(psCondBlock->aOffsets, (yyvsp[(1) - (3)].condBlock)->aOffsets,
							   (yyvsp[(1) - (3)].condBlock)->numOffsets * sizeof(SDWORD));
						psCondBlock->aOffsets[(yyvsp[(1) - (3)].condBlock)->numOffsets] =
							(yyvsp[(3) - (3)].condBlock)->aOffsets[0] + (yyvsp[(1) - (3)].condBlock)->size;
						psCondBlock->numOffsets = (yyvsp[(1) - (3)].condBlock)->numOffsets + 1;

						/* Put in the debugging information */
						PUT_DEBUG(psCondBlock, (yyvsp[(1) - (3)].condBlock));
						APPEND_DEBUG(psCondBlock, (yyvsp[(1) - (3)].condBlock)->size, (yyvsp[(3) - (3)].condBlock));

						/* Free the code blocks */
						FREE_DEBUG((yyvsp[(1) - (3)].condBlock));
						FREE_DEBUG((yyvsp[(3) - (3)].condBlock));
						FREE_CONDBLOCK((yyvsp[(1) - (3)].condBlock));
						FREE_CONDBLOCK((yyvsp[(3) - (3)].condBlock));

						(yyval.condBlock) = psCondBlock;
					}
    break;

  case 144:

/* Line 1806 of yacc.c  */
#line 4019 "script_parser.ypp"
    {
						RULE( "terminal_cond: '{' statement_list '}'");

						/* Allocate the block */
						ALLOC_CONDBLOCK(psCondBlock, 1, (yyvsp[(2) - (3)].cblock)->size);
						ALLOC_DEBUG(psCondBlock, (yyvsp[(2) - (3)].cblock)->debugEntries);
						ip = psCondBlock->pCode;

						/* Put in the debugging information */
						PUT_DEBUG(psCondBlock, (yyvsp[(2) - (3)].cblock));

						/* Store the statements */
						PUT_BLOCK(ip, (yyvsp[(2) - (3)].cblock));
						FREE_DEBUG((yyvsp[(2) - (3)].cblock));
						FREE_BLOCK((yyvsp[(2) - (3)].cblock));

						(yyval.condBlock) = psCondBlock;
					}
    break;

  case 145:

/* Line 1806 of yacc.c  */
#line 4040 "script_parser.ypp"
    {
						char *pDummy;

						RULE( "cond_clause: IF '(' boolexp ')' '{' statement_list '}'");

						/* Get the line number for the end of the boolean expression */
						/* and store it in debugLine.                                 */
						scriptGetErrorData((SDWORD *)&debugLine, &pDummy);
					}
    break;

  case 146:

/* Line 1806 of yacc.c  */
#line 4050 "script_parser.ypp"
    {
						/* Allocate the block */
						ALLOC_CONDBLOCK(psCondBlock, 1,	//1 offset
										(yyvsp[(3) - (8)].cblock)->size + (yyvsp[(7) - (8)].cblock)->size +
										1 + 1);		//size + size + OP_JUMPFALSE + OP_JUMP

						ALLOC_DEBUG(psCondBlock, (yyvsp[(7) - (8)].cblock)->debugEntries + 1);
						ip = psCondBlock->pCode;

						/* Store the boolean expression code */
						PUT_BLOCK(ip, (yyvsp[(3) - (8)].cblock));
						FREE_BLOCK((yyvsp[(3) - (8)].cblock));

						/* Put in the jump to the end of the block if the */
						/* condition is false */
						PUT_PKOPCODE(ip, OP_JUMPFALSE, (yyvsp[(7) - (8)].cblock)->size + 2);

						/* Put in the debugging information */
						if (genDebugInfo)
						{
							psCondBlock->debugEntries = 1;
							psCondBlock->psDebug->line = debugLine;
							psCondBlock->psDebug->offset = 0;
							APPEND_DEBUG(psCondBlock, ip - psCondBlock->pCode, (yyvsp[(7) - (8)].cblock));
						}

						/* Store the statements */
						PUT_BLOCK(ip, (yyvsp[(7) - (8)].cblock));
						FREE_DEBUG((yyvsp[(7) - (8)].cblock));
						FREE_BLOCK((yyvsp[(7) - (8)].cblock));

						/* Store the location that has to be filled in   */
						//TODO: don't want to store pointers as ints
						psCondBlock->aOffsets[0] = (UDWORD)(ip - psCondBlock->pCode);

						/* Put in a jump to skip the rest of the conditional */
						/* The correct offset will be set once the whole   */
						/* conditional has been parsed                     */
						/* The jump should be to the instruction after the */
						/* entire conditonal block                         */
						PUT_PKOPCODE(ip, OP_JUMP, 0);

						(yyval.condBlock) = psCondBlock;
					}
    break;

  case 147:

/* Line 1806 of yacc.c  */
#line 4096 "script_parser.ypp"
    {
						char *pDummy;

						RULE( "cond_clause: IF '(' boolexp ')' statement");

						/* Get the line number for the end of the boolean expression */
						/* and store it in debugLine.                                 */
						scriptGetErrorData((SDWORD *)&debugLine, &pDummy);
					}
    break;

  case 148:

/* Line 1806 of yacc.c  */
#line 4106 "script_parser.ypp"
    {
						/* Allocate the block */
						ALLOC_CONDBLOCK(psCondBlock, 1,
										(yyvsp[(3) - (6)].cblock)->size + (yyvsp[(6) - (6)].cblock)->size +
										1 + 1);		//size + size + opcode + opcode

						ALLOC_DEBUG(psCondBlock, (yyvsp[(6) - (6)].cblock)->debugEntries + 1);
						ip = psCondBlock->pCode;

						/* Store the boolean expression code */
						PUT_BLOCK(ip, (yyvsp[(3) - (6)].cblock));
						FREE_BLOCK((yyvsp[(3) - (6)].cblock));


						/* Put in the jump to the end of the block if the */
						/* condition is false */
						PUT_PKOPCODE(ip, OP_JUMPFALSE, (yyvsp[(6) - (6)].cblock)->size + 2);

						/* Put in the debugging information */
						if (genDebugInfo)
						{
							psCondBlock->debugEntries = 1;
							psCondBlock->psDebug->line = debugLine;
							psCondBlock->psDebug->offset = 0;
							APPEND_DEBUG(psCondBlock, ip - psCondBlock->pCode, (yyvsp[(6) - (6)].cblock));
						}

						/* Store the statements */
						PUT_BLOCK(ip, (yyvsp[(6) - (6)].cblock));
						FREE_DEBUG((yyvsp[(6) - (6)].cblock));
						FREE_BLOCK((yyvsp[(6) - (6)].cblock));

						/* Store the location that has to be filled in   */
						psCondBlock->aOffsets[0] = (UDWORD)(ip - psCondBlock->pCode);

						/* Put in a jump to skip the rest of the conditional */
						/* The correct offset will be set once the whole   */
						/* conditional has been parsed                     */
						/* The jump should be to the instruction after the */
						/* entire conditonal block                         */
						PUT_PKOPCODE(ip, OP_JUMP, 0);

						(yyval.condBlock) = psCondBlock;
					}
    break;

  case 149:

/* Line 1806 of yacc.c  */
#line 4163 "script_parser.ypp"
    {
					char *pDummy;

					/* Get the line number for the end of the boolean expression */
					/* and store it in debugLine.                                 */
					scriptGetErrorData((SDWORD *)&debugLine, &pDummy);
				}
    break;

  case 150:

/* Line 1806 of yacc.c  */
#line 4171 "script_parser.ypp"
    {
					RULE("loop: WHILE '(' boolexp ')'");

					//ALLOC_BLOCK(psCurrBlock, $3->size + $7->size + sizeof(OPCODE) * 2);
					ALLOC_BLOCK(psCurrBlock, (yyvsp[(3) - (8)].cblock)->size + (yyvsp[(7) - (8)].cblock)->size + 1 + 1);	//size + size + opcode + opcode

					ALLOC_DEBUG(psCurrBlock, (yyvsp[(7) - (8)].cblock)->debugEntries + 1);
					ip = psCurrBlock->pCode;

					/* Copy in the loop expression */
					PUT_BLOCK(ip, (yyvsp[(3) - (8)].cblock));
					FREE_BLOCK((yyvsp[(3) - (8)].cblock));

					/* Now a conditional jump out of the loop if the */
					/* expression is false.                          */
					PUT_PKOPCODE(ip, OP_JUMPFALSE, (yyvsp[(7) - (8)].cblock)->size + 2);

					/* Now put in the debugging information */
					if (genDebugInfo)
					{
						psCurrBlock->debugEntries = 1;
						psCurrBlock->psDebug->line = debugLine;
						psCurrBlock->psDebug->offset = 0;
						APPEND_DEBUG(psCurrBlock, ip - psCurrBlock->pCode, (yyvsp[(7) - (8)].cblock));
					}

					/* Copy in the body of the loop */
					PUT_BLOCK(ip, (yyvsp[(7) - (8)].cblock));
					FREE_DEBUG((yyvsp[(7) - (8)].cblock));
					FREE_BLOCK((yyvsp[(7) - (8)].cblock));

					/* Put in a jump back to the start of the loop expression */
					PUT_PKOPCODE(ip, OP_JUMP, (SWORD)( -(SWORD)(psCurrBlock->size) + 1));

					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 151:

/* Line 1806 of yacc.c  */
#line 4211 "script_parser.ypp"
    {
					RULE("expression: NUM_VAR++");

					scriptCodeIncDec((yyvsp[(1) - (2)].vSymbol), &psCurrBlock, OP_INC);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 152:

/* Line 1806 of yacc.c  */
#line 4220 "script_parser.ypp"
    {
					RULE("expression: NUM_VAR--");

					scriptCodeIncDec((yyvsp[(1) - (2)].vSymbol), &psCurrBlock, OP_DEC);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 153:

/* Line 1806 of yacc.c  */
#line 4236 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_ADD, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 154:

/* Line 1806 of yacc.c  */
#line 4244 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_SUB, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 155:

/* Line 1806 of yacc.c  */
#line 4252 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_MUL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 156:

/* Line 1806 of yacc.c  */
#line 4260 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_DIV, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 157:

/* Line 1806 of yacc.c  */
#line 4269 "script_parser.ypp"
    {
					//ALLOC_BLOCK(psCurrBlock, $2->size + sizeof(OPCODE));
					ALLOC_BLOCK(psCurrBlock, (yyvsp[(2) - (2)].cblock)->size + 1);	//size + unary minus opcode

					ip = psCurrBlock->pCode;

					/* Copy the already generated bits of code into the code block */
					PUT_BLOCK(ip, (yyvsp[(2) - (2)].cblock));

					/* Now put a negation operator into the code */
					PUT_PKOPCODE(ip, OP_UNARYOP, OP_NEG);

					/* Free the two code blocks that have been copied */
					FREE_BLOCK((yyvsp[(2) - (2)].cblock));

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 158:

/* Line 1806 of yacc.c  */
#line 4288 "script_parser.ypp"
    {
					/* Just pass the code up the tree */
					(yyval.cblock) = (yyvsp[(2) - (3)].cblock);
				}
    break;

  case 159:

/* Line 1806 of yacc.c  */
#line 4293 "script_parser.ypp"
    {
					RULE("expression: (int) floatexp");

					/* perform cast */
					(yyvsp[(2) - (2)].cblock)->type = VAL_INT;

					//ALLOC_BLOCK(psCurrBlock, $4->size + sizeof(OPCODE));
					ALLOC_BLOCK(psCurrBlock, (yyvsp[(2) - (2)].cblock)->size + 1);		//size + opcode
					ip = psCurrBlock->pCode;

					PUT_BLOCK(ip, (yyvsp[(2) - (2)].cblock));
					PUT_OPCODE(ip, OP_TO_INT);

					FREE_BLOCK((yyvsp[(2) - (2)].cblock));		// free floatexp

					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 160:

/* Line 1806 of yacc.c  */
#line 4311 "script_parser.ypp"
    {

					RULE( "expression: NUM_FUNC '(' param_list ')'");

					/* Generate the code for the function call */
					codeRet = scriptCodeFunction((yyvsp[(1) - (4)].fSymbol), (yyvsp[(3) - (4)].pblock), true, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 161:

/* Line 1806 of yacc.c  */
#line 4324 "script_parser.ypp"
    {
					UDWORD paramNumber;

					RULE( "expression: NUM_FUNC_CUST '(' param_list ')'");

					/* if($4->numParams != $3->numParams) */
					if((yyvsp[(3) - (4)].pblock)->numParams != (yyvsp[(1) - (4)].eSymbol)->numParams)
					{
						debug(LOG_ERROR, "Wrong number of arguments for function call: '%s'. Expected %d parameters instead of  %d.", (yyvsp[(1) - (4)].eSymbol)->pIdent, (yyvsp[(1) - (4)].eSymbol)->numParams, (yyvsp[(3) - (4)].pblock)->numParams);
						scr_error("Wrong number of arguments in function call");
						return CE_PARSE;
					}

					if(!(yyvsp[(1) - (4)].eSymbol)->bFunction)
					{
						debug(LOG_ERROR, "'%s' is not a function", (yyvsp[(1) - (4)].eSymbol)->pIdent);
						scr_error("Can't call an event");
						return CE_PARSE;
					}

					/* make sure function has a return type */
					if((yyvsp[(1) - (4)].eSymbol)->retType != VAL_INT)
					{
						debug(LOG_ERROR, "'%s' does not return an integer value", (yyvsp[(1) - (4)].eSymbol)->pIdent);
						scr_error("assignment type conflict");
						return CE_PARSE;
					}

					/* check if right parameters were passed */
					paramNumber = checkFuncParamTypes((yyvsp[(1) - (4)].eSymbol), (yyvsp[(3) - (4)].pblock));
					if(paramNumber > 0)
					{
						debug(LOG_ERROR, "Parameter mismatch in function call: '%s'. Mismatch in parameter  %d.", (yyvsp[(1) - (4)].eSymbol)->pIdent, paramNumber);
						YYABORT;
					}

					/* Allocate the code block */
					ALLOC_BLOCK(psCurrBlock, (yyvsp[(3) - (4)].pblock)->size + 1 + 1);	//Params + Opcode + event index

					ip = psCurrBlock->pCode;

					if((yyvsp[(3) - (4)].pblock)->numParams > 0)	/* if any parameters declared */
					{
						/* Copy in the code for the parameters */
						PUT_BLOCK(ip, (yyvsp[(3) - (4)].pblock));
					}

					FREE_PBLOCK((yyvsp[(3) - (4)].pblock));

					/* Store the instruction */
					PUT_OPCODE(ip, OP_FUNC);
					PUT_EVENT(ip,(yyvsp[(1) - (4)].eSymbol)->index);			//Put event index

					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 162:

/* Line 1806 of yacc.c  */
#line 4380 "script_parser.ypp"
    {
					RULE("expression: NUM_VAR");

					codeRet = scriptCodeVarGet((yyvsp[(1) - (1)].vSymbol), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 163:

/* Line 1806 of yacc.c  */
#line 4390 "script_parser.ypp"
    {
					RULE("expression: NUM_CONSTANT");

					codeRet = scriptCodeConstant((yyvsp[(1) - (1)].cSymbol), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 164:

/* Line 1806 of yacc.c  */
#line 4400 "script_parser.ypp"
    {
					RULE( "expression: num_objvar");
					RULE( "type=%d", (yyvsp[(1) - (1)].objVarBlock)->psObjVar->type);

					codeRet = scriptCodeObjGet((yyvsp[(1) - (1)].objVarBlock), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 165:

/* Line 1806 of yacc.c  */
#line 4411 "script_parser.ypp"
    {
					codeRet = scriptCodeArrayGet((yyvsp[(1) - (1)].arrayBlock), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 166:

/* Line 1806 of yacc.c  */
#line 4419 "script_parser.ypp"
    {
					RULE("expression: INTEGER");

					//ALLOC_BLOCK(psCurrBlock, sizeof(OPCODE) + sizeof(UDWORD));
					ALLOC_BLOCK(psCurrBlock, 1 + 1);	//opcode + integer value

					ip = psCurrBlock->pCode;

					/* Code to store the value on the stack */
					PUT_PKOPCODE(ip, OP_PUSH, VAL_INT);
					PUT_DATA_INT(ip, (yyvsp[(1) - (1)].ival));

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 167:

/* Line 1806 of yacc.c  */
#line 4437 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_ADD, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 168:

/* Line 1806 of yacc.c  */
#line 4445 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_SUB, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 169:

/* Line 1806 of yacc.c  */
#line 4453 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_MUL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 170:

/* Line 1806 of yacc.c  */
#line 4461 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_DIV, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 171:

/* Line 1806 of yacc.c  */
#line 4469 "script_parser.ypp"
    {
					RULE("floatexp: (float) expression");

					/* perform cast */
					 (yyvsp[(2) - (2)].cblock)->type = VAL_FLOAT;

					ALLOC_BLOCK(psCurrBlock, (yyvsp[(2) - (2)].cblock)->size + 1);	//size + opcode
					ip = psCurrBlock->pCode;

					PUT_BLOCK(ip, (yyvsp[(2) - (2)].cblock));
					PUT_OPCODE(ip, OP_TO_FLOAT);

					FREE_BLOCK((yyvsp[(2) - (2)].cblock));		// free 'expression'

					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 172:

/* Line 1806 of yacc.c  */
#line 4486 "script_parser.ypp"
    {
					ALLOC_BLOCK(psCurrBlock, (yyvsp[(2) - (2)].cblock)->size + 1);	//size + opcode

					ip = psCurrBlock->pCode;

					/* Copy the already generated bits of code into the code block */
					PUT_BLOCK(ip, (yyvsp[(2) - (2)].cblock));

					/* Now put a negation operator into the code */
					PUT_PKOPCODE(ip, OP_UNARYOP, OP_NEG);

					/* Free the code block that have been copied */
					FREE_BLOCK((yyvsp[(2) - (2)].cblock));

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 173:

/* Line 1806 of yacc.c  */
#line 4504 "script_parser.ypp"
    {
					/* Just pass the code up the tree */
					(yyval.cblock) = (yyvsp[(2) - (3)].cblock);
				}
    break;

  case 174:

/* Line 1806 of yacc.c  */
#line 4509 "script_parser.ypp"
    {
					RULE("floatexp: FLOAT_FUNC '(' param_list ')'");

					/* Generate the code for the function call */
					codeRet = scriptCodeFunction((yyvsp[(1) - (4)].fSymbol), (yyvsp[(3) - (4)].pblock), true, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 175:

/* Line 1806 of yacc.c  */
#line 4520 "script_parser.ypp"
    {
						UDWORD line,paramNumber;
						char *pDummy;

						RULE("floatexp: FLOAT_FUNC_CUST '(' param_list ')'");

						if((yyvsp[(3) - (4)].pblock)->numParams != (yyvsp[(1) - (4)].eSymbol)->numParams)
						{
							debug(LOG_ERROR, "Wrong number of arguments for function call: '%s'. Expected %d parameters instead of  %d.", (yyvsp[(1) - (4)].eSymbol)->pIdent, (yyvsp[(1) - (4)].eSymbol)->numParams, (yyvsp[(3) - (4)].pblock)->numParams);
							scr_error("Wrong number of arguments in function call");
							return CE_PARSE;
						}

						if(!(yyvsp[(1) - (4)].eSymbol)->bFunction)
						{
							debug(LOG_ERROR, "'%s' is not a function", (yyvsp[(1) - (4)].eSymbol)->pIdent);
							scr_error("Can't call an event");
							return CE_PARSE;
						}

						/* make sure function has a return type */
						if((yyvsp[(1) - (4)].eSymbol)->retType != VAL_FLOAT)
						{
							debug(LOG_ERROR, "'%s' does not return a float value", (yyvsp[(1) - (4)].eSymbol)->pIdent);
							scr_error("assignment type conflict");
							return CE_PARSE;
						}

						/* check if right parameters were passed */
						paramNumber = checkFuncParamTypes((yyvsp[(1) - (4)].eSymbol), (yyvsp[(3) - (4)].pblock));
						if(paramNumber > 0)
						{
							debug(LOG_ERROR, "Parameter mismatch in function call: '%s'. Mismatch in parameter  %d.", (yyvsp[(1) - (4)].eSymbol)->pIdent, paramNumber);
							YYABORT;
						}

						/* Allocate the code block */
						ALLOC_BLOCK(psCurrBlock, (yyvsp[(3) - (4)].pblock)->size + 1 + 1);	//Params + Opcode + event index

						ALLOC_DEBUG(psCurrBlock, 1);
						ip = psCurrBlock->pCode;

						if((yyvsp[(3) - (4)].pblock)->numParams > 0)	/* if any parameters declared */
						{
							/* Copy in the code for the parameters */
							PUT_BLOCK(ip, (yyvsp[(3) - (4)].pblock));
						}

						FREE_PBLOCK((yyvsp[(3) - (4)].pblock));

						/* Store the instruction */
						PUT_OPCODE(ip, OP_FUNC);
						PUT_EVENT(ip,(yyvsp[(1) - (4)].eSymbol)->index);		//Put event/function index

						/* Add the debugging information */
						if (genDebugInfo)
						{
							psCurrBlock->psDebug[0].offset = 0;
							scriptGetErrorData((SDWORD *)&line, &pDummy);
							psCurrBlock->psDebug[0].line = line;
						}

						(yyval.cblock) = psCurrBlock;
				}
    break;

  case 176:

/* Line 1806 of yacc.c  */
#line 4585 "script_parser.ypp"
    {
					RULE( "floatexp: FLOAT_VAR");

					codeRet = scriptCodeVarGet((yyvsp[(1) - (1)].vSymbol), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 177:

/* Line 1806 of yacc.c  */
#line 4595 "script_parser.ypp"
    {
					RULE( "floatexp: FLOAT_T");

					//ALLOC_BLOCK(psCurrBlock, sizeof(OPCODE) + sizeof(float));
					ALLOC_BLOCK(psCurrBlock, 1 + 1);		//opcode + float

					ip = psCurrBlock->pCode;

					/* Code to store the value on the stack */
					PUT_PKOPCODE(ip, OP_PUSH, VAL_FLOAT);
					PUT_DATA_FLOAT(ip, (yyvsp[(1) - (1)].fval));

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 178:

/* Line 1806 of yacc.c  */
#line 4619 "script_parser.ypp"
    {
					RULE("stringexp: stringexp '&' stringexp");

					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_CONC, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;

					//debug(LOG_SCRIPT, "END stringexp: stringexp '&' stringexp");
				}
    break;

  case 179:

/* Line 1806 of yacc.c  */
#line 4631 "script_parser.ypp"
    {
					RULE( "stringexp: stringexp '&' expression");

					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_CONC, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 180:

/* Line 1806 of yacc.c  */
#line 4641 "script_parser.ypp"
    {
					RULE( "stringexp: expression '&' stringexp");

					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_CONC, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 181:

/* Line 1806 of yacc.c  */
#line 4651 "script_parser.ypp"
    {
					RULE( "stringexp: stringexp '&' boolexp");

					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_CONC, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 182:

/* Line 1806 of yacc.c  */
#line 4661 "script_parser.ypp"
    {
					RULE( "stringexp: boolexp '&' stringexp");

					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_CONC, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 183:

/* Line 1806 of yacc.c  */
#line 4671 "script_parser.ypp"
    {
					RULE( "stringexp: stringexp '&' floatexp");

					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_CONC, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 184:

/* Line 1806 of yacc.c  */
#line 4681 "script_parser.ypp"
    {
					RULE( "stringexp: floatexp '&' stringexp");

					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_CONC, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 185:

/* Line 1806 of yacc.c  */
#line 4691 "script_parser.ypp"
    {
					/* Just pass the code up the tree */
					(yyval.cblock) = (yyvsp[(2) - (3)].cblock);
				}
    break;

  case 186:

/* Line 1806 of yacc.c  */
#line 4697 "script_parser.ypp"
    {
					RULE("stringexp: STRING_FUNC '(' param_list ')'");

					/* Generate the code for the function call */
					codeRet = scriptCodeFunction((yyvsp[(1) - (4)].fSymbol), (yyvsp[(3) - (4)].pblock), true, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 187:

/* Line 1806 of yacc.c  */
#line 4708 "script_parser.ypp"
    {
						RULE("stringexp: STRING_FUNC_CUST '(' param_list ')'");

						if((yyvsp[(3) - (4)].pblock)->numParams != (yyvsp[(1) - (4)].eSymbol)->numParams)
						{
							debug(LOG_ERROR, "Wrong number of arguments for function call: '%s'. Expected %d parameters instead of  %d.", (yyvsp[(1) - (4)].eSymbol)->pIdent, (yyvsp[(1) - (4)].eSymbol)->numParams, (yyvsp[(3) - (4)].pblock)->numParams);
							scr_error("Wrong number of arguments in function call");
							return CE_PARSE;
						}

						if(!(yyvsp[(1) - (4)].eSymbol)->bFunction)
						{
							debug(LOG_ERROR, "'%s' is not a function", (yyvsp[(1) - (4)].eSymbol)->pIdent);
							scr_error("Can't call an event");
							return CE_PARSE;
						}

						/* make sure function has a return type */
						if((yyvsp[(1) - (4)].eSymbol)->retType != VAL_STRING)
						{
							debug(LOG_ERROR, "'%s' does not return a string value", (yyvsp[(1) - (4)].eSymbol)->pIdent);
							scr_error("assignment type conflict");
							return CE_PARSE;
						}

						/* Allocate the code block */
						ALLOC_BLOCK(psCurrBlock, (yyvsp[(3) - (4)].pblock)->size + 1 + 1);	//Params + Opcode + event index

						ip = psCurrBlock->pCode;

						if((yyvsp[(3) - (4)].pblock)->numParams > 0)	/* if any parameters declared */
						{
							/* Copy in the code for the parameters */
							PUT_BLOCK(ip, (yyvsp[(3) - (4)].pblock));
						}

						FREE_PBLOCK((yyvsp[(3) - (4)].pblock));

						/* Store the instruction */
						PUT_OPCODE(ip, OP_FUNC);
						PUT_EVENT(ip,(yyvsp[(1) - (4)].eSymbol)->index);			//Put event index

						(yyval.cblock) = psCurrBlock;
				}
    break;

  case 188:

/* Line 1806 of yacc.c  */
#line 4754 "script_parser.ypp"
    {
					RULE("stringexpr: STRING_VAR");

					codeRet = scriptCodeVarGet((yyvsp[(1) - (1)].vSymbol), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 189:

/* Line 1806 of yacc.c  */
#line 4764 "script_parser.ypp"
    {
					RULE("QTEXT: '%s'", yyvsp[0].sval);

					//ALLOC_BLOCK(psCurrBlock, sizeof(OPCODE) + sizeof(UDWORD));
					ALLOC_BLOCK(psCurrBlock, 1 + 1);	//opcode + string pointer

					ip = psCurrBlock->pCode;

					/* Code to store the value on the stack */
					PUT_PKOPCODE(ip, OP_PUSH, VAL_STRING);
					PUT_DATA_STRING(ip, STRSTACK[CURSTACKSTR]);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;

					/* Manage string stack */
					sstrcpy(STRSTACK[CURSTACKSTR], yyvsp[0].sval);

					CURSTACKSTR = CURSTACKSTR + 1;		/* Increment 'pointer' to the top of the string stack */

					if(CURSTACKSTR >= MAXSTACKLEN)
					{
						scr_error("Can't store more than %d strings", MAXSTACKLEN);
					}

					//debug(LOG_SCRIPT, "END QTEXT found");
				}
    break;

  case 190:

/* Line 1806 of yacc.c  */
#line 4792 "script_parser.ypp"
    {
					RULE( "stringexp: expression");

					/* Just pass the code up the tree */
					(yyval.cblock) = (yyvsp[(1) - (1)].cblock);
				}
    break;

  case 191:

/* Line 1806 of yacc.c  */
#line 4807 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_AND, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 192:

/* Line 1806 of yacc.c  */
#line 4815 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_OR, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 193:

/* Line 1806 of yacc.c  */
#line 4823 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_EQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 194:

/* Line 1806 of yacc.c  */
#line 4831 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_NOTEQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 195:

/* Line 1806 of yacc.c  */
#line 4839 "script_parser.ypp"
    {
					//ALLOC_BLOCK(psCurrBlock, $2->size + sizeof(OPCODE));
					ALLOC_BLOCK(psCurrBlock, (yyvsp[(2) - (2)].cblock)->size + 1);	//size + opcode

					ip = psCurrBlock->pCode;

					/* Copy the already generated bits of code into the code block */
					PUT_BLOCK(ip, (yyvsp[(2) - (2)].cblock));

					/* Now put a negation operator into the code */
					PUT_PKOPCODE(ip, OP_UNARYOP, OP_NOT);

					/* Free the two code blocks that have been copied */
					FREE_BLOCK((yyvsp[(2) - (2)].cblock));

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 196:

/* Line 1806 of yacc.c  */
#line 4858 "script_parser.ypp"
    {
					/* Just pass the code up the tree */
					(yyval.cblock) = (yyvsp[(2) - (3)].cblock);
				}
    break;

  case 197:

/* Line 1806 of yacc.c  */
#line 4863 "script_parser.ypp"
    {
					RULE("boolexp: BOOL_FUNC '(' param_list ')'");

					/* Generate the code for the function call */
					codeRet = scriptCodeFunction((yyvsp[(1) - (4)].fSymbol), (yyvsp[(3) - (4)].pblock), true, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 198:

/* Line 1806 of yacc.c  */
#line 4874 "script_parser.ypp"
    {
						UDWORD paramNumber;

						RULE("boolexp: BOOL_FUNC_CUST '(' param_list ')'");

						if((yyvsp[(3) - (4)].pblock)->numParams != (yyvsp[(1) - (4)].eSymbol)->numParams)
						{
							debug(LOG_ERROR, "Wrong number of arguments for function call: '%s'. Expected %d parameters instead of  %d.", (yyvsp[(1) - (4)].eSymbol)->pIdent, (yyvsp[(1) - (4)].eSymbol)->numParams, (yyvsp[(3) - (4)].pblock)->numParams);
							scr_error("Wrong number of arguments in function call");
							return CE_PARSE;
						}

						if(!(yyvsp[(1) - (4)].eSymbol)->bFunction)
						{
							debug(LOG_ERROR, "'%s' is not a function", (yyvsp[(1) - (4)].eSymbol)->pIdent);
							scr_error("Can't call an event");
							return CE_PARSE;
						}

						/* make sure function has a return type */
						if((yyvsp[(1) - (4)].eSymbol)->retType != VAL_BOOL)
						{
							debug(LOG_ERROR, "'%s' does not return a boolean value", (yyvsp[(1) - (4)].eSymbol)->pIdent);
							scr_error("assignment type conflict");
							return CE_PARSE;
						}

						/* check if right parameters were passed */
						paramNumber = checkFuncParamTypes((yyvsp[(1) - (4)].eSymbol), (yyvsp[(3) - (4)].pblock));
						if(paramNumber > 0)
						{
							debug(LOG_ERROR, "Parameter mismatch in function call: '%s'. Mismatch in parameter  %d.", (yyvsp[(1) - (4)].eSymbol)->pIdent, paramNumber);
							YYABORT;
						}

						/* Allocate the code block */
						ALLOC_BLOCK(psCurrBlock, (yyvsp[(3) - (4)].pblock)->size + 1 + 1);	//Params + Opcode + event index

						ip = psCurrBlock->pCode;

						if((yyvsp[(3) - (4)].pblock)->numParams > 0)	/* if any parameters declared */
						{
							/* Copy in the code for the parameters */
							PUT_BLOCK(ip, (yyvsp[(3) - (4)].pblock));
						}

						FREE_PBLOCK((yyvsp[(3) - (4)].pblock));

						/* Store the instruction */
						PUT_OPCODE(ip, OP_FUNC);
						PUT_EVENT(ip,(yyvsp[(1) - (4)].eSymbol)->index);		//Put event/function index

						(yyval.cblock) = psCurrBlock;
				}
    break;

  case 199:

/* Line 1806 of yacc.c  */
#line 4929 "script_parser.ypp"
    {
					RULE("boolexp: BOOL_VAR");

					codeRet = scriptCodeVarGet((yyvsp[(1) - (1)].vSymbol), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 200:

/* Line 1806 of yacc.c  */
#line 4939 "script_parser.ypp"
    {
					RULE("boolexp: BOOL_CONSTANT");

					codeRet = scriptCodeConstant((yyvsp[(1) - (1)].cSymbol), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 201:

/* Line 1806 of yacc.c  */
#line 4949 "script_parser.ypp"
    {
					codeRet = scriptCodeObjGet((yyvsp[(1) - (1)].objVarBlock), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 202:

/* Line 1806 of yacc.c  */
#line 4957 "script_parser.ypp"
    {
					codeRet = scriptCodeArrayGet((yyvsp[(1) - (1)].arrayBlock), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 203:

/* Line 1806 of yacc.c  */
#line 4965 "script_parser.ypp"
    {
					RULE("boolexp: BOOLEAN_T");

					//ALLOC_BLOCK(psCurrBlock, sizeof(OPCODE) + sizeof(UDWORD));
					ALLOC_BLOCK(psCurrBlock, 1 + 1);	//opcode + value
					ip = psCurrBlock->pCode;

					/* Code to store the value on the stack */
					PUT_PKOPCODE(ip, OP_PUSH, VAL_BOOL);
					PUT_DATA_BOOL(ip, (yyvsp[(1) - (1)].bval));

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 204:

/* Line 1806 of yacc.c  */
#line 4980 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_EQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 205:

/* Line 1806 of yacc.c  */
#line 4988 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_EQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 206:

/* Line 1806 of yacc.c  */
#line 4996 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_EQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 207:

/* Line 1806 of yacc.c  */
#line 5004 "script_parser.ypp"
    {
					if (!interpCheckEquiv((yyvsp[(1) - (3)].cblock)->type,(yyvsp[(3) - (3)].cblock)->type))
					{
						scr_error("Type mismatch for equality");
						YYABORT;
					}
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_EQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 208:

/* Line 1806 of yacc.c  */
#line 5017 "script_parser.ypp"
    {
					if (!interpCheckEquiv((yyvsp[(1) - (3)].cblock)->type,(yyvsp[(3) - (3)].cblock)->type))
					{
						scr_error("Type mismatch for equality");
						YYABORT;
					}
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_EQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 209:

/* Line 1806 of yacc.c  */
#line 5030 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_NOTEQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 210:

/* Line 1806 of yacc.c  */
#line 5038 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_NOTEQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 211:

/* Line 1806 of yacc.c  */
#line 5046 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_NOTEQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 212:

/* Line 1806 of yacc.c  */
#line 5054 "script_parser.ypp"
    {
					if (!interpCheckEquiv((yyvsp[(1) - (3)].cblock)->type,(yyvsp[(3) - (3)].cblock)->type))
					{
						scr_error("Type mismatch for inequality");
						YYABORT;
					}
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_NOTEQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 213:

/* Line 1806 of yacc.c  */
#line 5067 "script_parser.ypp"
    {
					if (!interpCheckEquiv((yyvsp[(1) - (3)].cblock)->type,(yyvsp[(3) - (3)].cblock)->type))
					{
						scr_error("Type mismatch for inequality");
						YYABORT;
					}
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_NOTEQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 214:

/* Line 1806 of yacc.c  */
#line 5080 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_LESSEQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 215:

/* Line 1806 of yacc.c  */
#line 5088 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_LESSEQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 216:

/* Line 1806 of yacc.c  */
#line 5096 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_GREATEREQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 217:

/* Line 1806 of yacc.c  */
#line 5104 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_GREATEREQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 218:

/* Line 1806 of yacc.c  */
#line 5112 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_GREATER, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 219:

/* Line 1806 of yacc.c  */
#line 5120 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_GREATER, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 220:

/* Line 1806 of yacc.c  */
#line 5128 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_LESS, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 221:

/* Line 1806 of yacc.c  */
#line 5136 "script_parser.ypp"
    {
					codeRet = scriptCodeBinaryOperator((yyvsp[(1) - (3)].cblock), (yyvsp[(3) - (3)].cblock), OP_LESS, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 222:

/* Line 1806 of yacc.c  */
#line 5151 "script_parser.ypp"
    {
					codeRet = scriptCodeVarGet((yyvsp[(1) - (1)].vSymbol), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 223:

/* Line 1806 of yacc.c  */
#line 5159 "script_parser.ypp"
    {
					RULE("userexp: USER_CONSTANT");

					codeRet = scriptCodeConstant((yyvsp[(1) - (1)].cSymbol), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 224:

/* Line 1806 of yacc.c  */
#line 5169 "script_parser.ypp"
    {
					codeRet = scriptCodeObjGet((yyvsp[(1) - (1)].objVarBlock), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);
					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 225:

/* Line 1806 of yacc.c  */
#line 5176 "script_parser.ypp"
    {
					RULE("userexp: user_array_var");

					codeRet = scriptCodeArrayGet((yyvsp[(1) - (1)].arrayBlock), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 226:

/* Line 1806 of yacc.c  */
#line 5186 "script_parser.ypp"
    {
					/* Generate the code for the function call */
					codeRet = scriptCodeFunction((yyvsp[(1) - (4)].fSymbol), (yyvsp[(3) - (4)].pblock), true, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 227:

/* Line 1806 of yacc.c  */
#line 5195 "script_parser.ypp"
    {
						UDWORD line,paramNumber;
						char *pDummy;

						if((yyvsp[(3) - (4)].pblock)->numParams != (yyvsp[(1) - (4)].eSymbol)->numParams)
						{
							debug(LOG_ERROR, "Wrong number of arguments for function call: '%s'. Expected %d parameters instead of  %d.", (yyvsp[(1) - (4)].eSymbol)->pIdent, (yyvsp[(1) - (4)].eSymbol)->numParams, (yyvsp[(3) - (4)].pblock)->numParams);
							scr_error("Wrong number of arguments in function call");
							return CE_PARSE;
						}

						if(!(yyvsp[(1) - (4)].eSymbol)->bFunction)
						{
							debug(LOG_ERROR, "'%s' is not a function", (yyvsp[(1) - (4)].eSymbol)->pIdent);
							scr_error("Can't call an event");
							return CE_PARSE;
						}

						/* check if right parameters were passed */
						paramNumber = checkFuncParamTypes((yyvsp[(1) - (4)].eSymbol), (yyvsp[(3) - (4)].pblock));
						if(paramNumber > 0)
						{
							debug(LOG_ERROR, "Parameter mismatch in function call: '%s'. Mismatch in parameter  %d.", (yyvsp[(1) - (4)].eSymbol)->pIdent, paramNumber);
							YYABORT;
						}

						/* Allocate the code block */
						ALLOC_BLOCK(psCurrBlock, (yyvsp[(3) - (4)].pblock)->size + 1 + 1);	//Params + Opcode + event index

						ALLOC_DEBUG(psCurrBlock, 1);
						ip = psCurrBlock->pCode;

						if((yyvsp[(3) - (4)].pblock)->numParams > 0)	/* if any parameters declared */
						{
							/* Copy in the code for the parameters */
							PUT_BLOCK(ip, (yyvsp[(3) - (4)].pblock));
							FREE_PBLOCK((yyvsp[(3) - (4)].pblock));
						}

						/* Store the instruction */
						PUT_OPCODE(ip, OP_FUNC);
						PUT_EVENT(ip,(yyvsp[(1) - (4)].eSymbol)->index);			//Put event index

						/* Add the debugging information */
						if (genDebugInfo)
						{
							psCurrBlock->psDebug[0].offset = 0;
							scriptGetErrorData((SDWORD *)&line, &pDummy);
							psCurrBlock->psDebug[0].line = line;
						}

						/* remember objexp type for further stuff, like myVar = objFunc(); to be able to check type equivalency */
						psCurrBlock->type = (yyvsp[(1) - (4)].eSymbol)->retType;

						(yyval.cblock) = psCurrBlock;
				}
    break;

  case 228:

/* Line 1806 of yacc.c  */
#line 5252 "script_parser.ypp"
    {
					//ALLOC_BLOCK(psCurrBlock, sizeof(OPCODE) + sizeof(UDWORD));
					ALLOC_BLOCK(psCurrBlock, 1 + 1);	//opcode + trigger index

					ip = psCurrBlock->pCode;

					/* Code to store the value on the stack */
					PUT_PKOPCODE(ip, OP_PUSH, VAL_TRIGGER);		//TODO: don't need to put VAL_TRIGGER here, since stored in ip->type now
					PUT_TRIGGER(ip, (yyvsp[(1) - (1)].tSymbol)->index);

					psCurrBlock->type = VAL_TRIGGER;

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 229:

/* Line 1806 of yacc.c  */
#line 5268 "script_parser.ypp"
    {
					//ALLOC_BLOCK(psCurrBlock, sizeof(OPCODE) + sizeof(UDWORD));
					ALLOC_BLOCK(psCurrBlock, 1 + 1);	//opcode + '-1' for 'inactive'

					ip = psCurrBlock->pCode;

					/* Code to store the value on the stack */
					PUT_PKOPCODE(ip, OP_PUSH, VAL_TRIGGER);
					PUT_TRIGGER(ip, -1);

					psCurrBlock->type = VAL_TRIGGER;

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 230:

/* Line 1806 of yacc.c  */
#line 5284 "script_parser.ypp"
    {
					//ALLOC_BLOCK(psCurrBlock, sizeof(OPCODE) + sizeof(UDWORD));
					ALLOC_BLOCK(psCurrBlock, 1 + 1);	//opcode + event index

					ip = psCurrBlock->pCode;

					/* Code to store the value on the stack */
					PUT_PKOPCODE(ip, OP_PUSH, VAL_EVENT);
					PUT_EVENT(ip, (yyvsp[(1) - (1)].eSymbol)->index);

					psCurrBlock->type = VAL_EVENT;

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 231:

/* Line 1806 of yacc.c  */
#line 5307 "script_parser.ypp"
    {
					codeRet = scriptCodeVarGet((yyvsp[(1) - (1)].vSymbol), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 232:

/* Line 1806 of yacc.c  */
#line 5315 "script_parser.ypp"
    {
					RULE("objexp: OBJ_CONSTANT");

					codeRet = scriptCodeConstant((yyvsp[(1) - (1)].cSymbol), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 233:

/* Line 1806 of yacc.c  */
#line 5325 "script_parser.ypp"
    {
					/* Generate the code for the function call */
					codeRet = scriptCodeFunction((yyvsp[(1) - (4)].fSymbol), (yyvsp[(3) - (4)].pblock), true, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 234:

/* Line 1806 of yacc.c  */
#line 5334 "script_parser.ypp"
    {
						UDWORD paramNumber;

						if((yyvsp[(3) - (4)].pblock)->numParams != (yyvsp[(1) - (4)].eSymbol)->numParams)
						{
							debug(LOG_ERROR, "Wrong number of arguments for function call: '%s'. Expected %d parameters instead of  %d.", (yyvsp[(1) - (4)].eSymbol)->pIdent, (yyvsp[(1) - (4)].eSymbol)->numParams, (yyvsp[(3) - (4)].pblock)->numParams);
							scr_error("Wrong number of arguments in function call");
							return CE_PARSE;
						}

						if(!(yyvsp[(1) - (4)].eSymbol)->bFunction)
						{
							debug(LOG_ERROR, "'%s' is not a function", (yyvsp[(1) - (4)].eSymbol)->pIdent);
							scr_error("Can't call an event");
							return CE_PARSE;
						}

						/* make sure function has a return type */
						/* if($1->retType != OBJ_VAR) */
						if(asScrTypeTab[(yyvsp[(1) - (4)].eSymbol)->retType - VAL_USERTYPESTART].accessType != AT_OBJECT)
						{
							scr_error("'%s' does not return an object value (%d )", (yyvsp[(1) - (4)].eSymbol)->pIdent, (yyvsp[(1) - (4)].eSymbol)->retType);
							return CE_PARSE;
						}

						/* check if right parameters were passed */
						paramNumber = checkFuncParamTypes((yyvsp[(1) - (4)].eSymbol), (yyvsp[(3) - (4)].pblock));
						if(paramNumber > 0)
						{
							debug(LOG_ERROR, "Parameter mismatch in function call: '%s'. Mismatch in parameter  %d.", (yyvsp[(1) - (4)].eSymbol)->pIdent, paramNumber);
							YYABORT;
						}

						/* Allocate the code block */
						ALLOC_BLOCK(psCurrBlock, (yyvsp[(3) - (4)].pblock)->size + 1 + 1);	//Params + Opcode + event index

						ip = psCurrBlock->pCode;

						if((yyvsp[(3) - (4)].pblock)->numParams > 0)	/* if any parameters declared */
						{
							/* Copy in the code for the parameters */
							PUT_BLOCK(ip, (yyvsp[(3) - (4)].pblock));
						}

						FREE_PBLOCK((yyvsp[(3) - (4)].pblock));

						/* Store the instruction */
						PUT_OPCODE(ip, OP_FUNC);
						PUT_EVENT(ip,(yyvsp[(1) - (4)].eSymbol)->index);			//Put event index

						/* remember objexp type for further stuff, like myVar = objFunc(); to be able to check type equivalency */
						psCurrBlock->type = (yyvsp[(1) - (4)].eSymbol)->retType;

						(yyval.cblock) = psCurrBlock;
				}
    break;

  case 235:

/* Line 1806 of yacc.c  */
#line 5390 "script_parser.ypp"
    {
					codeRet = scriptCodeObjGet((yyvsp[(1) - (1)].objVarBlock), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 236:

/* Line 1806 of yacc.c  */
#line 5398 "script_parser.ypp"
    {
					codeRet = scriptCodeArrayGet((yyvsp[(1) - (1)].arrayBlock), &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					/* Return the code block */
					(yyval.cblock) = psCurrBlock;
				}
    break;

  case 237:

/* Line 1806 of yacc.c  */
#line 5412 "script_parser.ypp"
    {
					RULE( "objexp_dot: objexp '.', type=%d", (yyvsp[(1) - (2)].cblock)->type);

					// Store the object type for the variable lookup
					objVarContext = (yyvsp[(1) - (2)].cblock)->type;
				}
    break;

  case 238:

/* Line 1806 of yacc.c  */
#line 5420 "script_parser.ypp"
    {
					RULE( "objexp_dot: userexp '.', type=%d", (yyvsp[(1) - (2)].cblock)->type);

					// Store the object type for the variable lookup
					objVarContext = (yyvsp[(1) - (2)].cblock)->type;
				}
    break;

  case 239:

/* Line 1806 of yacc.c  */
#line 5433 "script_parser.ypp"
    {

					RULE( "num_objvar: objexp_dot NUM_OBJVAR");

					codeRet = scriptCodeObjectVariable((yyvsp[(1) - (2)].cblock), (yyvsp[(2) - (2)].vSymbol), &psObjVarBlock);
					CHECK_CODE_ERROR(codeRet);

					// reset the object type
					objVarContext = (INTERP_TYPE)0;

					/* Return the code block */
					(yyval.objVarBlock) = psObjVarBlock;
				}
    break;

  case 240:

/* Line 1806 of yacc.c  */
#line 5449 "script_parser.ypp"
    {
					codeRet = scriptCodeObjectVariable((yyvsp[(1) - (2)].cblock), (yyvsp[(2) - (2)].vSymbol), &psObjVarBlock);
					CHECK_CODE_ERROR(codeRet);

					// reset the object type
					objVarContext = (INTERP_TYPE)0;

					/* Return the code block */
					(yyval.objVarBlock) = psObjVarBlock;
				}
    break;

  case 241:

/* Line 1806 of yacc.c  */
#line 5462 "script_parser.ypp"
    {
					codeRet = scriptCodeObjectVariable((yyvsp[(1) - (2)].cblock), (yyvsp[(2) - (2)].vSymbol), &psObjVarBlock);
					CHECK_CODE_ERROR(codeRet);

					// reset the object type
					objVarContext = (INTERP_TYPE)0;

					/* Return the code block */
					(yyval.objVarBlock) = psObjVarBlock;
				}
    break;

  case 242:

/* Line 1806 of yacc.c  */
#line 5474 "script_parser.ypp"
    {
					codeRet = scriptCodeObjectVariable((yyvsp[(1) - (2)].cblock), (yyvsp[(2) - (2)].vSymbol), &psObjVarBlock);
					CHECK_CODE_ERROR(codeRet);

					// reset the object type
					objVarContext = (INTERP_TYPE)0;

					/* Return the code block */
					(yyval.objVarBlock) = psObjVarBlock;
				}
    break;

  case 243:

/* Line 1806 of yacc.c  */
#line 5493 "script_parser.ypp"
    {
						ALLOC_ARRAYBLOCK(psCurrArrayBlock, (yyvsp[(2) - (3)].cblock)->size, NULL);
						ip = psCurrArrayBlock->pCode;

						/* Copy the index expression code into the code block */
						PUT_BLOCK(ip, (yyvsp[(2) - (3)].cblock));
						FREE_BLOCK((yyvsp[(2) - (3)].cblock));

						(yyval.arrayBlock) = psCurrArrayBlock;
					}
    break;

  case 244:

/* Line 1806 of yacc.c  */
#line 5506 "script_parser.ypp"
    {
						(yyval.arrayBlock) = (yyvsp[(1) - (1)].arrayBlock);
					}
    break;

  case 245:

/* Line 1806 of yacc.c  */
#line 5511 "script_parser.ypp"
    {
						ALLOC_ARRAYBLOCK(psCurrArrayBlock, (yyvsp[(1) - (4)].arrayBlock)->size + (yyvsp[(3) - (4)].cblock)->size, NULL);

						ip = psCurrArrayBlock->pCode;

						/* Copy the index expression code into the code block */
						psCurrArrayBlock->dimensions = (yyvsp[(1) - (4)].arrayBlock)->dimensions + 1;
						PUT_BLOCK(ip, (yyvsp[(1) - (4)].arrayBlock));
						PUT_BLOCK(ip, (yyvsp[(3) - (4)].cblock));
						FREE_ARRAYBLOCK((yyvsp[(1) - (4)].arrayBlock));
						FREE_ARRAYBLOCK((yyvsp[(3) - (4)].cblock));

						(yyval.arrayBlock) = psCurrArrayBlock;
					}
    break;

  case 246:

/* Line 1806 of yacc.c  */
#line 5528 "script_parser.ypp"
    {
						codeRet = scriptCodeArrayVariable((yyvsp[(2) - (2)].arrayBlock), (yyvsp[(1) - (2)].vSymbol), &psCurrArrayBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.arrayBlock) = psCurrArrayBlock;
					}
    break;

  case 247:

/* Line 1806 of yacc.c  */
#line 5538 "script_parser.ypp"
    {
						codeRet = scriptCodeArrayVariable((yyvsp[(2) - (2)].arrayBlock), (yyvsp[(1) - (2)].vSymbol), &psCurrArrayBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.arrayBlock) = psCurrArrayBlock;
					}
    break;

  case 248:

/* Line 1806 of yacc.c  */
#line 5548 "script_parser.ypp"
    {
						codeRet = scriptCodeArrayVariable((yyvsp[(2) - (2)].arrayBlock), (yyvsp[(1) - (2)].vSymbol), &psCurrArrayBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.arrayBlock) = psCurrArrayBlock;
					}
    break;

  case 249:

/* Line 1806 of yacc.c  */
#line 5558 "script_parser.ypp"
    {
						RULE("user_array_var:		VAR_ARRAY array_index_list");

						codeRet = scriptCodeArrayVariable((yyvsp[(2) - (2)].arrayBlock), (yyvsp[(1) - (2)].vSymbol), &psCurrArrayBlock);
						CHECK_CODE_ERROR(codeRet);

						/* Return the code block */
						(yyval.arrayBlock) = psCurrArrayBlock;
					}
    break;



/* Line 1806 of yacc.c  */
#line 8600 "script_parser.cpp"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 2067 of yacc.c  */
#line 5569 "script_parser.ypp"


// Reset all the symbol tables
static void scriptResetTables(void)
{
	VAR_SYMBOL		*psCurr, *psNext;
	TRIGGER_SYMBOL	*psTCurr, *psTNext;
	EVENT_SYMBOL	*psECurr, *psENext;

	SDWORD			i;

	/* start with global vars definition */
	localVariableDef = false;
	//debug(LOG_SCRIPT, "localVariableDef = false 4");

	/* Reset the global variable symbol table */
	for(psCurr = psGlobalVars; psCurr != NULL; psCurr = psNext)
	{
		psNext = psCurr->psNext;
		free(psCurr);
	}
	psGlobalVars = NULL;

	/* Reset the global variable symbol table */
	psCurEvent = NULL;
	for(i=0; i<maxEventsLocalVars; i++)
	{
		numEventLocalVars[i] = 0;

		for(psCurr = psLocalVarsB[i]; psCurr != NULL; psCurr = psNext)
		{
			psNext = psCurr->psNext;
			free(psCurr);
		}
		psLocalVarsB[i] = NULL;
	}

	/* Reset the global array symbol table */
	for(psCurr = psGlobalArrays; psCurr != NULL; psCurr = psNext)
	{
		psNext = psCurr->psNext;
		free(psCurr);
	}
	psGlobalArrays = NULL;

	// Reset the trigger table
	for(psTCurr = psTriggers; psTCurr; psTCurr = psTNext)
	{
		psTNext = psTCurr->psNext;
		if (psTCurr->psDebug)
		{
			free(psTCurr->psDebug);
		}
		if (psTCurr->pCode)
		{
			free(psTCurr->pCode);
		}
		free(psTCurr->pIdent);
		free(psTCurr);
	}
	psTriggers = NULL;
	numTriggers = 0;

	// Reset the event table
	for(psECurr = psEvents; psECurr; psECurr = psENext)
	{
		psENext = psECurr->psNext;
		if (psECurr->psDebug)
		{
			free(psECurr->psDebug);
		}
		free(psECurr->pIdent);
		free(psECurr->pCode);
		free(psECurr);
	}
	psEvents = NULL;
	numEvents = 0;
}

/* Compile a script program */
SCRIPT_CODE* scriptCompile(PHYSFS_file* fileHandle, SCR_DEBUGTYPE debugType)
{
	/* Feed flex with initial input buffer */
	scriptSetInputFile(fileHandle);

	scriptResetTables();
	psFinalProg = NULL;
	if (debugType == SCR_DEBUGINFO)
	{
		genDebugInfo = true;
	}
	else
	{
		genDebugInfo = false;
	}

	if (scr_parse() != 0 || bError)
	{
		scr_lex_destroy();
		return NULL;
	}
	scr_lex_destroy();

	scriptResetTables();

	return psFinalProg;
}


/* A simple error reporting routine */
#ifndef DEBUG
[[ noreturn ]]
#endif
void scr_error(const char *pMessage, ...)
{
	int		line;
	char	*text;
	va_list	args;
	char	aBuff[1024];

	va_start(args, pMessage);
	vsnprintf(aBuff, sizeof(aBuff), pMessage, args);
	va_end(args);

	scriptGetErrorData(&line, &text);

	bError = true;

#ifdef DEBUG
	debug( LOG_ERROR, "script parse error:\n%s at %s:%d\nToken: %d, Text: '%s'\n",
			  aBuff, GetLastResourceFilename(), line, scr_char, text );
	ASSERT( false, "script parse error:\n%s at %s:%d\nToken: %d, Text: '%s'\n",
			  aBuff, GetLastResourceFilename(), line, scr_char, text );
#else
	//DBERROR(("script parse error:\n%s at line %d\nToken: %d, Text: '%s'\n",
	//		  pMessage, line, scr_char, text));
	debug( LOG_ERROR, "script parse error:\n'%s' at %s:%d\nToken: %d, Text: '%s'\n",
			  aBuff, GetLastResourceFilename(), line, scr_char, text );
	abort();
#endif
}


/* Look up a type symbol */
bool scriptLookUpType(const char *pIdent, INTERP_TYPE *pType)
{
	UDWORD	i;

	//debug(LOG_SCRIPT, "scriptLookUpType");

	if (asScrTypeTab)
	{
		for(i=0; asScrTypeTab[i].typeID != 0; i++)
		{
			if (strcmp(asScrTypeTab[i].pIdent, pIdent) == 0)
			{
				*pType = asScrTypeTab[i].typeID;
				return true;
			}
		}
	}

	//debug(LOG_SCRIPT, "END scriptLookUpType");

	return false;
}

/* pop passed arguments (if any) */
bool popArguments(INTERP_VAL **ip_temp, SDWORD numParams)
{
	SDWORD			i;

	/* code to pop passed params right before the main code begins */
	for(i = numParams-1; i >= 0 ; i--)
	{
		PUT_PKOPCODE(*ip_temp, OP_POPLOCAL, i);		//pop parameters into first i local params
	}

	return true;
}


/* Add a new variable symbol.
 * If localVariableDef is true a local variable symbol is defined,
 * otherwise a global variable symbol is defined.
 */
bool scriptAddVariable(VAR_DECL *psStorage, VAR_IDENT_DECL *psVarIdent)
{
	VAR_SYMBOL		*psNew;
	unsigned int i;
	char* ident;

	/* Allocate the memory for the symbol structure plus the symbol name */
	psNew = (VAR_SYMBOL *)malloc(sizeof(VAR_SYMBOL) + strlen(psVarIdent->pIdent) + 1);
	if (psNew == NULL)
	{
		scr_error("Out of memory");
		return false;
	}

	// This ensures we only need to call free() on the VAR_SYMBOL* pointer and not on its members
	ident = (char*)(psNew + 1);
	strcpy(ident, psVarIdent->pIdent);
	psNew->pIdent = ident;

	psNew->type = psStorage->type;
	psNew->storage = psStorage->storage;
	psNew->dimensions = psVarIdent->dimensions;


	for(i=0; i<psNew->dimensions; i++)
	{
		psNew->elements[i] = psVarIdent->elements[i];
	}
	if (psNew->dimensions == 0)
	{
		if(psStorage->storage != ST_LOCAL)	//if not a local var
		{
			if (psGlobalVars == NULL)
			{
				psNew->index = 0;
			}
			else
			{
				psNew->index = psGlobalVars->index + 1;
			}

			/* Add the symbol to the list */
			psNew->psNext = psGlobalVars;
			psGlobalVars = psNew;
		}
		else		//local var
		{
			if(psCurEvent == NULL)
				debug(LOG_ERROR, "Can't declare local variables before defining an event");

			//debug(LOG_SCRIPT, "local variable declared for event %d, type=%d \n", psCurEvent->index, psNew->type);
			//debug(LOG_SCRIPT, "%s \n", psNew->pIdent);

			if (psLocalVarsB[psCurEvent->index] == NULL)
			{
				psNew->index = 0;
			}
			else
			{
				psNew->index = psLocalVarsB[psCurEvent->index]->index + 1;
			}

			numEventLocalVars[psCurEvent->index] = numEventLocalVars[psCurEvent->index] + 1;

			psNew->psNext = psLocalVarsB[psCurEvent->index];
			psLocalVarsB[psCurEvent->index] = psNew;

			//debug(LOG_SCRIPT, "local variable declared. ");
		}
	}
	else
	{
		if (psGlobalArrays == NULL)
		{
			psNew->index = 0;
		}
		else
		{
			psNew->index = psGlobalArrays->index + 1;
		}

		psNew->psNext = psGlobalArrays;
		psGlobalArrays = psNew;
	}


	return true;
}

/* Look up a variable symbol */
bool scriptLookUpVariable(const char *pIdent, VAR_SYMBOL **ppsSym)
{
	VAR_SYMBOL		*psCurr;
	UDWORD			i;

	//debug(LOG_SCRIPT, "scriptLookUpVariable");

	/* See if the symbol is an object variable */
	if (asScrObjectVarTab && objVarContext != 0)
	{
		for(psCurr = asScrObjectVarTab; psCurr->pIdent != NULL; psCurr++)
		{
			if (interpCheckEquiv((INTERP_TYPE)psCurr->objType, objVarContext) &&
				strcmp(psCurr->pIdent, pIdent) == 0)
			{
				*ppsSym = psCurr;
				return true;
			}
		}
	}

	/* See if the symbol is an external variable */
	if (asScrExternalTab)
	{
		for(psCurr = asScrExternalTab; psCurr->pIdent != NULL; psCurr++)
		{
			if (strcmp(psCurr->pIdent, pIdent) == 0)
			{
				//debug(LOG_SCRIPT, "scriptLookUpVariable: extern");
				*ppsSym = psCurr;
				return true;
			}
		}
	}

	/* check local vars if we are inside of an event */
	if(psCurEvent != NULL)
	{
		if(psCurEvent->index >= maxEventsLocalVars)
			debug(LOG_ERROR, "psCurEvent->index (%d) >= maxEventsLocalVars", psCurEvent->index);

		i = psCurEvent->index;

		if(psLocalVarsB[i] != NULL)	//any vars stored for this event
		{
		int		line;
		char	*text;

			//debug(LOG_SCRIPT, "now checking event %s; index = %d\n", psCurEvent->pIdent, psCurEvent->index);
			scriptGetErrorData(&line, &text);
			for(psCurr = psLocalVarsB[i]; psCurr != NULL; psCurr = psCurr->psNext)
			{

				if(psCurr->pIdent == NULL)
				{
					debug(LOG_ERROR, "psCurr->pIdent == NULL");
					debug(LOG_ERROR, "psCurr->index = %d", psCurr->index);
				}

				//debug(LOG_SCRIPT, "start comparing, num local vars=%d, at line %d\n", numEventLocalVars[i], line);
				//debug(LOG_SCRIPT, "current var=%s\n", psCurr->pIdent);
				//debug(LOG_SCRIPT, "passed string=%s\n", pIdent);

				//debug(LOG_SCRIPT, "comparing %s with %s \n", psCurr->pIdent, pIdent);
				if (strcmp(psCurr->pIdent, pIdent) == 0)
				{
					//debug(LOG_SCRIPT, "scriptLookUpVariable - local var found, type=%d\n", psCurr->type);
					*ppsSym = psCurr;
					return true;
				}
			}
		}
	}

	/* See if the symbol is in the global variable list.
	 * This is not checked for when local variables are being defined.
	 * This allows local variables to have the same name as global ones.
	 */
	if (!localVariableDef)
	{
		for(psCurr = psGlobalVars; psCurr != NULL; psCurr = psCurr->psNext)
		{
			if (strcmp(psCurr->pIdent, pIdent) == 0)
			{
				*ppsSym = psCurr;
				return true;
			}
		}
		for(psCurr = psGlobalArrays; psCurr != NULL; psCurr = psCurr->psNext)
		{
			if (strcmp(psCurr->pIdent, pIdent) == 0)
			{
				*ppsSym = psCurr;
				return true;
			}
		}
	}

	/* Failed to find the variable */
	*ppsSym = NULL;

	//debug(LOG_SCRIPT, "END scriptLookUpVariable");
	return false;
}


/* Add a new trigger symbol */
bool scriptAddTrigger(const char *pIdent, TRIGGER_DECL *psDecl, UDWORD line)
{
	TRIGGER_SYMBOL		*psTrigger, *psCurr, *psPrev;

	// Allocate the trigger
	psTrigger = (TRIGGER_SYMBOL *)malloc(sizeof(TRIGGER_SYMBOL));
	if (!psTrigger)
	{
		scr_error("Out of memory");
		return false;
	}
	psTrigger->pIdent = (char *)malloc(strlen(pIdent) + 1);
	if (!psTrigger->pIdent)
	{
		scr_error("Out of memory");
		return false;
	}
	strcpy(psTrigger->pIdent, pIdent);
	if (psDecl->size > 0)
	{
		psTrigger->pCode = (INTERP_VAL *)malloc(psDecl->size * sizeof(INTERP_VAL));
		if (!psTrigger->pCode)
		{
			scr_error("Out of memory");
			return false;
		}
		memcpy(psTrigger->pCode, psDecl->pCode, psDecl->size * sizeof(INTERP_VAL));
	}
	else
	{
		psTrigger->pCode = NULL;
	}
	psTrigger->size = psDecl->size;
	psTrigger->type = psDecl->type;
	psTrigger->time = psDecl->time;
	psTrigger->index = numTriggers++;
	psTrigger->psNext = NULL;

	// Add debug info
	if (genDebugInfo)
	{
		psTrigger->psDebug = (SCRIPT_DEBUG *)malloc(sizeof(SCRIPT_DEBUG));
		psTrigger->psDebug[0].offset = 0;
		psTrigger->psDebug[0].line = line;
		psTrigger->debugEntries = 1;
	}
	else
	{
		psTrigger->debugEntries = 0;
		psTrigger->psDebug = NULL;
	}


	// Store the trigger
	psPrev = NULL;
	for(psCurr = psTriggers; psCurr; psCurr = psCurr->psNext)
	{
		psPrev = psCurr;
	}
	if (psPrev)
	{
		psPrev->psNext = psTrigger;
	}
	else
	{
		psTriggers = psTrigger;
	}

	return true;
}


/* Lookup a trigger symbol */
bool scriptLookUpTrigger(const char *pIdent, TRIGGER_SYMBOL **ppsTrigger)
{
	TRIGGER_SYMBOL	*psCurr;

	//debug(LOG_SCRIPT, "scriptLookUpTrigger");

	for(psCurr = psTriggers; psCurr; psCurr=psCurr->psNext)
	{
		if (strcmp(pIdent, psCurr->pIdent) == 0)
		{
			//debug(LOG_SCRIPT, "scriptLookUpTrigger: found");
			*ppsTrigger = psCurr;
			return true;
		}
	}

	//debug(LOG_SCRIPT, "END scriptLookUpTrigger");

	return false;
}


/* Lookup a callback trigger symbol */
bool scriptLookUpCallback(const char *pIdent, CALLBACK_SYMBOL **ppsCallback)
{
	CALLBACK_SYMBOL		*psCurr;

	//debug(LOG_SCRIPT, "scriptLookUpCallback");

	if (!asScrCallbackTab)
	{
		return false;
	}

	for(psCurr = asScrCallbackTab; psCurr->type != 0; psCurr += 1)
	{
		if (strcmp(pIdent, psCurr->pIdent) == 0)
		{
			//debug(LOG_SCRIPT, "scriptLookUpCallback: found");
			*ppsCallback = psCurr;
			return true;
		}
	}

	//debug(LOG_SCRIPT, "END scriptLookUpCallback: found");
	return false;
}

/* Add a new event symbol */
bool scriptDeclareEvent(const char *pIdent, EVENT_SYMBOL **ppsEvent, SDWORD numArgs)
{
	EVENT_SYMBOL		*psEvent, *psCurr, *psPrev;

	// Allocate the event
	psEvent = (EVENT_SYMBOL *)malloc(sizeof(EVENT_SYMBOL));
	if (!psEvent)
	{
		scr_error("Out of memory");
		return false;
	}
	psEvent->pIdent = (char *)malloc(strlen(pIdent) + 1);
	if (!psEvent->pIdent)
	{
		scr_error("Out of memory");
		return false;
	}
	strcpy(psEvent->pIdent, pIdent);
	psEvent->pCode = NULL;
	psEvent->size = 0;
	psEvent->psDebug = NULL;
	psEvent->debugEntries = 0;
	psEvent->index = numEvents++;
	psEvent->psNext = NULL;

	/* remember how many params this event has */
	psEvent->numParams = numArgs;
	psEvent->bFunction = false;
	psEvent->bDeclared = false;
	psEvent->retType = VAL_VOID;	/* functions can return a value */

	// Add the event to the list
	psPrev = NULL;
	for(psCurr = psEvents; psCurr; psCurr = psCurr->psNext)
	{
		psPrev = psCurr;
	}
	if (psPrev)
	{
		psPrev->psNext = psEvent;
	}
	else
	{
		psEvents = psEvent;
	}

	*ppsEvent = psEvent;

	return true;
}

// Add the code to a defined event
bool scriptDefineEvent(EVENT_SYMBOL *psEvent, CODE_BLOCK *psCode, SDWORD trigger)
{
	ASSERT(psCode != NULL, "scriptDefineEvent: psCode == NULL");
	ASSERT(psCode->size > 0,
		"Event '%s' is empty, please add at least 1 statement", psEvent->pIdent);

	// events with arguments can't have a trigger assigned
	ASSERT(!(psEvent->numParams > 0 && trigger >= 0),
		"Events with parameters can't have a trigger assigned, event: '%s' ", psEvent->pIdent);

	// Store the event code
	psEvent->pCode = (INTERP_VAL *)malloc(psCode->size * sizeof(INTERP_VAL));
	if (!psEvent->pCode)
	{
		scr_error("Out of memory");
		return false;
	}

	memcpy(psEvent->pCode, psCode->pCode, psCode->size * sizeof(INTERP_VAL));
	psEvent->size = psCode->size;
	psEvent->trigger = trigger;

	// Add debug info
	if (genDebugInfo && (psCode->debugEntries > 0))
	{
		psEvent->psDebug = (SCRIPT_DEBUG *)malloc(sizeof(SCRIPT_DEBUG) * psCode->debugEntries);

		if (!psEvent->psDebug)
		{
			scr_error("Out of memory");
			return false;
		}

		memcpy(psEvent->psDebug, psCode->psDebug,
			sizeof(SCRIPT_DEBUG) * psCode->debugEntries);
		psEvent->debugEntries = psCode->debugEntries;
	}
	else
	{
		psEvent->debugEntries = 0;
		psEvent->psDebug = NULL;
	}

	//debug(LOG_SCRIPT, "before define event");

	/* store local vars */
	if(psEvent->index >= maxEventsLocalVars)
		debug(LOG_ERROR, "scriptDefineEvent - psEvent->index >= maxEventsLocalVars");

	return true;
}

/* Lookup an event symbol */
bool scriptLookUpEvent(const char *pIdent, EVENT_SYMBOL **ppsEvent)
{
	EVENT_SYMBOL	*psCurr;
	//debug(LOG_SCRIPT, "scriptLookUpEvent");

	for(psCurr = psEvents; psCurr; psCurr=psCurr->psNext)
	{
		if (strcmp(pIdent, psCurr->pIdent) == 0)
		{
			//debug(LOG_SCRIPT, "scriptLookUpEvent:found");
			*ppsEvent = psCurr;
			return true;
		}
	}
	//debug(LOG_SCRIPT, "END scriptLookUpEvent");
	return false;
}


/* Look up a constant variable symbol */
bool scriptLookUpConstant(const char *pIdent, CONST_SYMBOL **ppsSym)
{
	CONST_SYMBOL	*psCurr;

	//debug(LOG_SCRIPT, "scriptLookUpConstant");

	/* Scan the Constant list */
	if (asScrConstantTab)
	{
		for(psCurr = asScrConstantTab; psCurr->type != VAL_VOID; psCurr++)
		{
			if (strcmp(psCurr->pIdent, pIdent) == 0)
			{
				*ppsSym = psCurr;
				return true;
			}
		}
	}

	//debug(LOG_SCRIPT, "END scriptLookUpConstant");

	return false;
}


/* Look up a function symbol */
bool scriptLookUpFunction(const char *pIdent, FUNC_SYMBOL **ppsSym)
{
	UDWORD i;

	//debug(LOG_SCRIPT, "scriptLookUpFunction");

	/* See if the function is defined as an instinct function */
	if (asScrInstinctTab)
	{
		for(i = 0; asScrInstinctTab[i].pFunc != NULL; i++)
		{
			if (strcmp(asScrInstinctTab[i].pIdent, pIdent) == 0)
			{
				*ppsSym = asScrInstinctTab + i;
				return true;
			}
		}
	}

	/* Failed to find the identifier */
	*ppsSym = NULL;

	//debug(LOG_SCRIPT, "END scriptLookUpFunction");
	return false;
}

/* Look up a function symbol defined in script */
bool scriptLookUpCustomFunction(const char *pIdent, EVENT_SYMBOL **ppsSym)
{
	EVENT_SYMBOL	*psCurr;

	//debug(LOG_SCRIPT, "scriptLookUpCustomFunction");

	/* See if the function is defined as a script function */
	for(psCurr = psEvents; psCurr; psCurr = psCurr->psNext)
	{
		if(psCurr->bFunction)	/* event defined as function */
		{
			if (strcmp(psCurr->pIdent, pIdent) == 0)
			{
				//debug(LOG_SCRIPT, "scriptLookUpCustomFunction: %s is a custom function", pIdent);
				*ppsSym = psCurr;
				return true;
			}
		}
	}

	/* Failed to find the identifier */
	*ppsSym = NULL;

	//debug(LOG_SCRIPT, "END scriptLookUpCustomFunction");
	return false;
}


/* Set the type table */
void scriptSetTypeTab(TYPE_SYMBOL *psTypeTab)
{
#ifdef DEBUG
	SDWORD			i;
	INTERP_TYPE		type;

	for(i=0, type=VAL_USERTYPESTART; psTypeTab[i].typeID != 0; i++)
	{
		ASSERT( psTypeTab[i].typeID == type,
			"scriptSetTypeTab: ID's must be >= VAL_USERTYPESTART and sequential" );
		type = (INTERP_TYPE)(type + 1);
	}
#endif

	asScrTypeTab = psTypeTab;
}


/* Set the function table */
void scriptSetFuncTab(FUNC_SYMBOL *psFuncTab)
{
	asScrInstinctTab = psFuncTab;
}

/* Set the object variable table */
void scriptSetObjectTab(VAR_SYMBOL *psObjTab)
{
	asScrObjectVarTab = psObjTab;
}

/* Set the external variable table */
void scriptSetExternalTab(VAR_SYMBOL *psExtTab)
{
	asScrExternalTab = psExtTab;
}

/* Set the constant table */
void scriptSetConstTab(CONST_SYMBOL *psConstTab)
{
	asScrConstantTab = psConstTab;
}

/* Set the callback table */
void scriptSetCallbackTab(CALLBACK_SYMBOL *psCallTab)
{
#ifdef DEBUG
	SDWORD			i;
	TRIGGER_TYPE	type;

	for(i=0, type=TR_CALLBACKSTART; psCallTab[i].type != 0; i++)
	{
		ASSERT( psCallTab[i].type == type,
			"scriptSetCallbackTab: ID's must be >= VAL_CALLBACKSTART and sequential" );
		type = (TRIGGER_TYPE)(type + 1);
	}
#endif

	asScrCallbackTab = psCallTab;
}

