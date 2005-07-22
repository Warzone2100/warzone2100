/* yacc -v -D Script_y.h -p scr_ -o Script_y.c Script.y */
#ifdef YYTRACE
#define YYDEBUG 1
#else
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#endif
/*
 * Portable way of defining ANSI C prototypes
 */
#ifndef YY_ARGS
#ifdef __STDC__
#define YY_ARGS(x)	x
#else
#define YY_ARGS(x)	()
#endif
#endif

#ifdef YACC_WINDOWS

#include <windows.h>

/*
 * the following is the handle to the current
 * instance of a windows program. The user
 * program calling scr_parse must supply this!
 */

#ifdef STRICT
extern HINSTANCE hInst;	
#else
extern HANDLE hInst;	
#endif

#endif	/* YACC_WINDOWS */

#if YYDEBUG
typedef struct yyNamedType_tag {	/* Tokens */
	char	* name;		/* printable name */
	short	token;		/* token # */
	short	type;		/* token type */
} yyNamedType;
typedef struct yyTypedRules_tag {	/* Typed rule table */
	char	* name;		/* compressed rule string */
	short	type;		/* rule result type */
} yyTypedRules;

#endif

/*
 * script.y
 *
 * The yacc grammar for the scipt files.
 */

#ifdef PSX
/* A few definitions so the yacc generated code will compile on the PSX.
 * These shouldn't actually be used by any code that is run on the PSX, it
 * just keeps the compiler happy.
 */
//static int printf(char* c, ...)
//{
//	return 0;
//}
#endif

#include <string.h>
#include <limits.h>
#include <stdio.h>

#include "frame.h"
#include "interp.h"
#include "parse.h"
#include "script.h"

/* Error return codes for code generation functions */
typedef enum _code_error
{
	CE_OK,				// No error
	CE_MEMORY,			// Out of memory
	CE_PARSE			// A parse error occured
} CODE_ERROR;

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

/* Pointer into the current code block */
static UDWORD		*ip;

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

/* The list of current local variables */
static VAR_SYMBOL	*psLocalVars=NULL;

/* The list of function definitions */
static FUNC_SYMBOL	*psFunctions=NULL;

/* The current object variable context */
static INTERP_TYPE	objVarContext = 0;

/* Control whether debug info is generated */
static BOOL			genDebugInfo = TRUE;

/* Currently defined triggers */
static TRIGGER_SYMBOL	*psTriggers;
static UDWORD			numTriggers;

/* Currently defined events */
static EVENT_SYMBOL		*psEvents;
static UDWORD			numEvents;

/* This is true when local variables are being defined.
 * (So local variables can have the same name as global ones)
 */
static BOOL localVariableDef=FALSE;

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

/* Macro to allocate a program structure */
#define ALLOC_PROG(psProg, codeSize, pAICode, numGlobs, numArys, numTrigs, numEvnts) \
	(psProg) = (SCRIPT_CODE *)MALLOC(sizeof(SCRIPT_CODE)); \
	if ((psProg) == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psProg)->pCode = (UDWORD *)MALLOC(codeSize); \
	if ((psProg)->pCode == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	if (numGlobs > 0) \
	{ \
		(psProg)->pGlobals = (INTERP_TYPE *)MALLOC(sizeof(INTERP_TYPE) * (numGlobs)); \
		if ((psProg)->pGlobals == NULL) \
		{ \
			scr_error("Out of memory"); \
			ALLOC_ERROR_ACTION; \
		} \
	} \
	else \
	{ \
		(psProg)->pGlobals = NULL; \
	} \
	if (numArys > 0) \
	{ \
		(psProg)->psArrayInfo = (ARRAY_DATA *)MALLOC(sizeof(ARRAY_DATA) * (numArys)); \
		if ((psProg)->psArrayInfo == NULL) \
		{ \
			scr_error("Out of memory"); \
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
		(psProg)->pTriggerTab = MALLOC(sizeof(UWORD) * ((numTrigs) + 1)); \
		if ((psProg)->pTriggerTab == NULL) \
		{ \
			scr_error("Out of memory"); \
			ALLOC_ERROR_ACTION; \
		} \
		(psProg)->psTriggerData = MALLOC(sizeof(TRIGGER_DATA) * (numTrigs)); \
		if ((psProg)->psTriggerData == NULL) \
		{ \
			scr_error("Out of memory"); \
			ALLOC_ERROR_ACTION; \
		} \
	} \
	else \
	{ \
		(psProg)->pTriggerTab = NULL; \
		(psProg)->psTriggerData = NULL; \
	} \
	(psProg)->pEventTab = MALLOC(sizeof(UWORD) * ((numEvnts) + 1)); \
	if ((psProg)->pEventTab == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psProg)->pEventLinks = MALLOC(sizeof(SWORD) * (numEvnts)); \
	if ((psProg)->pEventLinks == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psProg)->numGlobals = (UWORD)(numGlobs); \
	(psProg)->numTriggers = (UWORD)(numTriggers); \
	(psProg)->numEvents = (UWORD)(numEvnts); \
	(psProg)->size = (codeSize)

/* Macro to allocate a code block */
#define ALLOC_BLOCK(psBlock, blockSize) \
	(psBlock) = (CODE_BLOCK *)MALLOC(sizeof(CODE_BLOCK)); \
	if ((psBlock) == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psBlock)->pCode = (UDWORD *)MALLOC(blockSize); \
	if ((psBlock)->pCode == NULL) \
	{ \
		scr_error("Out of memory"); \
		FREE((psBlock)); \
		ALLOC_ERROR_ACTION; \
	} \
	(psBlock)->size = blockSize

/* Macro to free a code block */
#define FREE_BLOCK(psBlock) \
	FREE((psBlock)->pCode); \
	FREE((psBlock))

/* Macro to allocate a parameter block */
#define ALLOC_PBLOCK(psBlock, codeSize, paramSize) \
	(psBlock) = (PARAM_BLOCK *)MALLOC(sizeof(PARAM_BLOCK)); \
	if ((psBlock) == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psBlock)->pCode = (UDWORD *)MALLOC(codeSize); \
	if ((psBlock)->pCode == NULL) \
	{ \
		scr_error("Out of memory"); \
		FREE((psBlock)); \
		ALLOC_ERROR_ACTION; \
	} \
	(psBlock)->aParams = (INTERP_TYPE *)MALLOC(sizeof(INTERP_TYPE) * (paramSize)); \
	if ((psBlock)->aParams == NULL) \
	{ \
		scr_error("Out of memory"); \
		FREE((psBlock)->pCode); \
		FREE((psBlock)); \
		ALLOC_ERROR_ACTION; \
	} \
	(psBlock)->size = (codeSize); \
	(psBlock)->numParams = (paramSize)

/* Macro to free a parameter block */
#define FREE_PBLOCK(psBlock) \
	FREE((psBlock)->pCode); \
	FREE((psBlock)->aParams); \
	FREE((psBlock))

/* Macro to allocate a parameter declaration block */
#define ALLOC_PARAMDECL(psPDecl, num) \
	(psPDecl) = (PARAM_DECL *)MALLOC(sizeof(PARAM_DECL)); \
	if ((psPDecl) == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psPDecl)->aParams = (INTERP_TYPE *)MALLOC(sizeof(INTERP_TYPE) * (num)); \
	if ((psPDecl)->aParams == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psPDecl)->numParams = (num)

/* Macro to free a parameter declaration block */
#define FREE_PARAMDECL(psPDecl) \
	FREE((psPDecl)->aParams); \
	FREE((psPDecl))

/* Macro to allocate a conditional block */
#define ALLOC_CONDBLOCK(psCB, num, blockSize) \
	(psCB) = (COND_BLOCK *)MALLOC(sizeof(COND_BLOCK)); \
	if ((psCB) == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psCB)->aOffsets = (UDWORD *)MALLOC(sizeof(SDWORD) * (num)); \
	if ((psCB)->aOffsets == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psCB)->pCode = (UDWORD *)MALLOC(blockSize); \
	if ((psCB)->pCode == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psCB)->size = (blockSize); \
	(psCB)->numOffsets = (num)

/* Macro to free a conditional block */
#define FREE_CONDBLOCK(psCB) \
	FREE((psCB)->aOffsets); \
	FREE((psCB)->pCode); \
	FREE(psCB)

/* Macro to allocate a code block */
#define ALLOC_USERBLOCK(psBlock, blockSize) \
	(psBlock) = (USER_BLOCK *)MALLOC(sizeof(USER_BLOCK)); \
	if ((psBlock) == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psBlock)->pCode = (UDWORD *)MALLOC(blockSize); \
	if ((psBlock)->pCode == NULL) \
	{ \
		scr_error("Out of memory"); \
		FREE((psBlock)); \
		ALLOC_ERROR_ACTION; \
	} \
	(psBlock)->size = blockSize

/* Macro to free a code block */
#define FREE_USERBLOCK(psBlock) \
	FREE((psBlock)->pCode); \
	FREE((psBlock))

/* Macro to allocate an object variable block */
#define ALLOC_OBJVARBLOCK(psOV, blockSize, psVar) \
	(psOV) = (OBJVAR_BLOCK *)MALLOC(sizeof(OBJVAR_BLOCK)); \
	if ((psOV) == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psOV)->pCode = (UDWORD *)MALLOC(blockSize); \
	if ((psOV)->pCode == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psOV)->size = (blockSize); \
	(psOV)->psObjVar = (psVar)

/* Macro to free an object variable block */
#define FREE_OBJVARBLOCK(psOV) \
	FREE((psOV)->pCode); \
	FREE(psOV)

/* Macro to allocate an array variable block */
#define ALLOC_ARRAYBLOCK(psAV, blockSize, psVar) \
	(psAV) = (ARRAY_BLOCK *)MALLOC(sizeof(ARRAY_BLOCK)); \
	if ((psAV) == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psAV)->pCode = (UDWORD *)MALLOC(blockSize); \
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
	FREE((psAV)->pCode); \
	FREE(psAV)

/* Allocate a trigger subdecl */
#define ALLOC_TSUBDECL(psTSub, blockType, blockSize, blockTime) \
	(psTSub) = MALLOC(sizeof(TRIGGER_DECL)); \
	if ((psTSub) == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	(psTSub)->type = (blockType); \
	(psTSub)->time = (blockTime); \
	if ((blockSize) > 0) \
	{ \
		(psTSub)->pCode = MALLOC(blockSize); \
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
		FREE((psTSub)->pCode); \
	} \
	FREE(psTSub)

/* Allocate a variable declaration block */
#define ALLOC_VARDECL(psDcl) \
	(psDcl)=MALLOC(sizeof(VAR_DECL)); \
	if ((psDcl) == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	}

/* Free a variable declaration block */
#define FREE_VARDECL(psDcl) \
	FREE(psDcl)

/* Allocate a variable declaration block */
#define ALLOC_VARIDENTDECL(psDcl, ident, dim) \
	(psDcl)=MALLOC(sizeof(VAR_IDENT_DECL)); \
	if ((psDcl) == NULL) \
	{ \
		scr_error("Out of memory"); \
		ALLOC_ERROR_ACTION; \
	} \
	if ((ident) != NULL) \
	{ \
		(psDcl)->pIdent=MALLOC(strlen(ident)+1); \
		if ((psDcl)->pIdent == NULL) \
		{ \
			scr_error("Out of memory"); \
			ALLOC_ERROR_ACTION; \
		} \
		strcpy((psDcl)->pIdent, (ident)); \
	} \
	else \
	{ \
		(psDcl)->pIdent = NULL; \
	} \
	(psDcl)->dimensions = (dim)

/* Free a variable declaration block */
#define FREE_VARIDENTDECL(psDcl) \
	FREE(psDcl)

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

/* Macro to store an opcode in a code block */
#define PUT_OPCODE(ip, opcode) \
	*ip = (opcode) << OPCODE_SHIFT; \
	ip += 1

/* Macro to put a packed opcode in a code block */
#define PUT_PKOPCODE(ip, opcode, data) \
	*ip = ((UDWORD)(data)) & OPCODE_DATAMASK; \
	*ip = (((UDWORD)(opcode)) << OPCODE_SHIFT) | (*ip); \
	ip += 1

/* Macro to store the data part of an INTERP_VAL in a code block */
#define PUT_DATA(ip, data) \
	*ip = (UDWORD)(data); \
	ip += 1

/* Macros to store a value in a code block */
#define PUT_BOOL(ip, value) \
	((INTERP_VAL *)(ip))->type = VAL_BOOL; \
	((INTERP_VAL *)(ip))->v.bval = (value); \
	((INTERP_VAL *)(ip)) += 1
#define PUT_INT(ip, value) \
	((INTERP_VAL *)(ip))->type = VAL_INT; \
	((INTERP_VAL *)(ip))->v.ival = (value); \
	((INTERP_VAL *)(ip)) += 1
/*#define PUT_FLOAT(ip, value) \
	((INTERP_VAL *)(ip))->type = VAL_FLOAT; \
	((INTERP_VAL *)(ip))->v.fval = (value); \
	((INTERP_VAL *)(ip)) += 1*/
#define PUT_STRING(ip, value) \
	((INTERP_VAL *)(ip))->type = VAL_STRING; \
	((INTERP_VAL *)(ip))->v.sval = (value); \
	((INTERP_VAL *)(ip)) += 1
/*#define PUT_OBJECT(ip, value) \
	((INTERP_VAL *)(ip))->type = VAL_OBJECT; \
	((INTERP_VAL *)(ip))->v.oval = (value); \
	((INTERP_VAL *)(ip)) += 1*/

/* Macro to store a function pointer in a code block */
#define PUT_FUNC(ip, func) \
	*ip = (UDWORD)func; \
	ip += 1

/* Macro to store a function pointer in a code block */
#define PUT_VARFUNC(ip, func) \
	*ip = (UDWORD)func; \
	ip += 1

/* Macro to store a reference to a script function. */
/* This will be converted to a program location at the link stage */
#define PUT_SCRIPTFUNC(ip, func) \
	*ip = (UDWORD)func; \
	ip += 1

/* Macro to store a variable index number in a code block */
#define PUT_INDEX(ip, index) \
	*ip = (index); \
	ip++

/* Macro to store a jump offset in a code block */
#define PUT_OFFSET(ip, offset) \
	*((SDWORD *)ip) = (offset); \
	ip++

/* Macro to copy a code block into another code block */
#define PUT_BLOCK(ip, psBlock) \
	memcpy(ip, (psBlock)->pCode, (psBlock)->size); \
	ip = (UDWORD *)(((UBYTE *)ip) + (psBlock)->size)

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
		(psBlock)->psDebug = (SCRIPT_DEBUG *)MALLOC(sizeof(SCRIPT_DEBUG) * (num)); \
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
		FREE((psBlock)->psDebug)

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
		(psBlock)->psDebug[offset].pLabel = MALLOC(strlen(pString)+1); \
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

/* Generate the code for a function call, checking the parameter
 * types match.
 */
CODE_ERROR scriptCodeFunction(FUNC_SYMBOL		*psFSymbol,		// The function being called
							PARAM_BLOCK		*psPBlock,		// The functions parameters
							BOOL			expContext,		// Whether the function is being
															// called in an expression context
							CODE_BLOCK		**ppsCBlock)	// The generated code block
{
	UDWORD		size, i, *ip;
	BOOL		typeError = FALSE;
	STRING		aErrorString[255];

	ASSERT((psFSymbol != NULL, "ais_CodeFunction: Invalid function symbol pointer"));
	ASSERT((PTRVALID(psPBlock, sizeof(PARAM_BLOCK)),
		"scriptCodeFunction: Invalid param block pointer"));
	ASSERT(((psPBlock->size == 0) || PTRVALID(psPBlock->pCode, psPBlock->size),
		"scriptCodeFunction: Invalid parameter code pointer"));
	ASSERT((ppsCBlock != NULL,
		 "scriptCodeFunction: Invalid generated code block pointer"));

	/* Check the parameter types match what the function needs */
	for(i=0; (i<psFSymbol->numParams) && (i<psPBlock->numParams); i++)
	{
/*		if (psFSymbol->aParams[i] != VAL_VOID &&
			psFSymbol->aParams[i] != psPBlock->aParams[i])*/
		if (!interpCheckEquiv(psFSymbol->aParams[i], psPBlock->aParams[i]))
		{
			sprintf(aErrorString, "Type mismatch for paramter %d", i);
			scr_error(aErrorString);
			typeError = TRUE;
		}
	}

	/* Check the number of parameters matches that expected */
	if (psFSymbol->numParams != psPBlock->numParams)
	{
		sprintf(aErrorString, "Expected %d parameters", psFSymbol->numParams);
		scr_error(aErrorString);
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

	size = psPBlock->size + sizeof(OPCODE) + sizeof(SCRIPT_FUNC);
	if (!expContext && (psFSymbol->type != VAL_VOID))
	{
		size += sizeof(OPCODE);
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
	}
	else
	{
		/* call an instinct function */
		PUT_OPCODE(ip, OP_CALL);
		PUT_FUNC(ip, psFSymbol->pFunc);
	}

	if (!expContext && (psFSymbol->type != VAL_VOID))
	{
		/* Clear the return value from the stack */
		PUT_OPCODE(ip, OP_POP);
	}

	return CE_OK;
}

/* Generate the code for a parameter callback, checking the parameter
 * types match.
 */
CODE_ERROR scriptCodeCallbackParams(
							CALLBACK_SYMBOL	*psCBSymbol,	// The callback being called
							PARAM_BLOCK		*psPBlock,		// The callbacks parameters
							TRIGGER_DECL	**ppsTDecl)		// The generated code block
{
	UDWORD		size, i, *ip;
	BOOL		typeError = FALSE;
	STRING		aErrorString[255];

	ASSERT((PTRVALID(psPBlock, sizeof(PARAM_BLOCK)),
		"scriptCodeCallbackParams: Invalid param block pointer"));
	ASSERT(((psPBlock->size == 0) || PTRVALID(psPBlock->pCode, psPBlock->size),
		"scriptCodeCallbackParams: Invalid parameter code pointer"));
	ASSERT((ppsTDecl != NULL,
		 "scriptCodeCallbackParams: Invalid generated code block pointer"));
	ASSERT((psCBSymbol->pFunc != NULL,
		 "scriptCodeCallbackParams: Expected function pointer for callback symbol"));

	/* Check the parameter types match what the function needs */
	for(i=0; (i<psCBSymbol->numParams) && (i<psPBlock->numParams); i++)
	{
		if (!interpCheckEquiv(psCBSymbol->aParams[i], psPBlock->aParams[i]))
		{
			sprintf(aErrorString, "Type mismatch for paramter %d", i);
			scr_error(aErrorString);
			typeError = TRUE;
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
		sprintf(aErrorString, "Expected %d parameters", psCBSymbol->numParams);
		scr_error(aErrorString);
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

	size = psPBlock->size + sizeof(OPCODE) + sizeof(SCRIPT_FUNC);
	
	ALLOC_TSUBDECL(*ppsTDecl, psCBSymbol->type, size, 0);
	ip = (*ppsTDecl)->pCode;

	/* Copy in the code for the parameters */
	PUT_BLOCK(ip, psPBlock);
	FREE_PBLOCK(psPBlock);

	/* call the instinct function */
	PUT_OPCODE(ip, OP_CALL);
	PUT_FUNC(ip, psCBSymbol->pFunc);

	return CE_OK;
}

/* Generate code for assigning a value to a variable */
CODE_ERROR scriptCodeAssignment(VAR_SYMBOL	*psVariable,	// The variable to assign to
							  CODE_BLOCK	*psValue,		// The code for the value to
															// assign
							  CODE_BLOCK	**ppsBlock)		// Generated code
{
	SDWORD		size;

	ASSERT((psVariable != NULL,
		"scriptCodeAssignment: Invalid variable symbol pointer"));
	ASSERT((PTRVALID(psValue, sizeof(CODE_BLOCK)),
		"scriptCodeAssignment: Invalid value code block pointer"));
	ASSERT((PTRVALID(psValue->pCode, psValue->size),
		"scriptCodeAssignment: Invalid value code pointer"));
	ASSERT((ppsBlock != NULL,
		"scriptCodeAssignment: Invalid generated code block pointer"));

	size = psValue->size + sizeof(OPCODE);
	if (psVariable->storage == ST_EXTERN)
	{
		// Check there is a set function
		if (psVariable->set == NULL)
		{
			scr_error("No set function for external variable");
			return CE_PARSE;
		}
		size += sizeof(SCRIPT_VARFUNC);
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
CODE_ERROR scriptCodeObjAssignment(OBJVAR_BLOCK	*psVariable,// The variable to assign to
								 CODE_BLOCK		*psValue,	// The code for the value to
															// assign
								 CODE_BLOCK		**ppsBlock)	// Generated code
{
	ASSERT((PTRVALID(psVariable, sizeof(OBJVAR_BLOCK)),
		"scriptCodeObjAssignment: Invalid object variable block pointer"));
	ASSERT((PTRVALID(psVariable->pCode, psVariable->size),
		"scriptCodeObjAssignment: Invalid object variable code pointer"));
	ASSERT((psVariable->psObjVar != NULL,
		"scriptCodeObjAssignment: Invalid object variable symbol pointer"));
	ASSERT((PTRVALID(psValue, sizeof(CODE_BLOCK)),
		"scriptCodeObjAssignment: Invalid value code block pointer"));
	ASSERT((PTRVALID(psValue->pCode, psValue->size),
		"scriptCodeObjAssignment: Invalid value code pointer"));
	ASSERT((ppsBlock != NULL,
		"scriptCodeObjAssignment: Invalid generated code block pointer"));

	// Check there is an access function for the variable
	if (psVariable->psObjVar->set == NULL)
	{
		scr_error("No set function for object variable");
		return CE_PARSE;
	}

	ALLOC_BLOCK(*ppsBlock, psVariable->size + psValue->size + 
					sizeof(OPCODE) + sizeof(SCRIPT_VARFUNC));
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
CODE_ERROR scriptCodeObjGet(OBJVAR_BLOCK	*psVariable,// The variable to get from
						  CODE_BLOCK	**ppsBlock)	// Generated code
{
	ASSERT((PTRVALID(psVariable, sizeof(OBJVAR_BLOCK)),
		"scriptCodeObjAssignment: Invalid object variable block pointer"));
	ASSERT((PTRVALID(psVariable->pCode, psVariable->size),
		"scriptCodeObjAssignment: Invalid object variable code pointer"));
	ASSERT((psVariable->psObjVar != NULL,
		"scriptCodeObjAssignment: Invalid object variable symbol pointer"));
	ASSERT((ppsBlock != NULL,
		"scriptCodeObjAssignment: Invalid generated code block pointer"));

	// Check there is an access function for the variable
	if (psVariable->psObjVar->get == NULL)
	{
		scr_error("No get function for object variable");
		return CE_PARSE;
	}

	ALLOC_BLOCK(*ppsBlock, psVariable->size + 
					sizeof(OPCODE) + sizeof(SCRIPT_VARFUNC));
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
CODE_ERROR scriptCodeArrayAssignment(ARRAY_BLOCK	*psVariable,// The variable to assign to
								 CODE_BLOCK		*psValue,	// The code for the value to
															// assign
								 CODE_BLOCK		**ppsBlock)	// Generated code
{
//	SDWORD		elementDWords, i;
//	UBYTE		*pElement;

	ASSERT((PTRVALID(psVariable, sizeof(ARRAY_BLOCK)),
		"scriptCodeObjAssignment: Invalid object variable block pointer"));
	ASSERT((PTRVALID(psVariable->pCode, psVariable->size),
		"scriptCodeObjAssignment: Invalid object variable code pointer"));
	ASSERT((psVariable->psArrayVar != NULL,
		"scriptCodeObjAssignment: Invalid object variable symbol pointer"));
	ASSERT((PTRVALID(psValue, sizeof(CODE_BLOCK)),
		"scriptCodeObjAssignment: Invalid value code block pointer"));
	ASSERT((PTRVALID(psValue->pCode, psValue->size),
		"scriptCodeObjAssignment: Invalid value code pointer"));
	ASSERT((ppsBlock != NULL,
		"scriptCodeObjAssignment: Invalid generated code block pointer"));

	// Check this is an array
	if (psVariable->psArrayVar->dimensions == 0)
	{
		scr_error("Not an array variable");
		return CE_PARSE;
	}

	// calculate the number of DWORDs needed to store the number of elements for each dimension of the array
//	elementDWords = (psVariable->psArrayVar->dimensions - 1)/4 + 1;

//	ALLOC_BLOCK(*ppsBlock, psVariable->size + psValue->size + sizeof(OPCODE) + elementDWords*4);
	ALLOC_BLOCK(*ppsBlock, psVariable->size + psValue->size + sizeof(OPCODE));
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

	// store the size of each dimension
/*	pElement = (UBYTE *)ip;
	for(i=0; i<psVariable->psArrayVar->dimensions; i++)
	{
		*pElement = (UBYTE)psVariable->psArrayVar->elements[i];
		pElement += 1;
	}*/

	/* Free the variable block */
	FREE_ARRAYBLOCK(psVariable);

	return CE_OK;
}

/* Generate code for getting a value from an array variable */
CODE_ERROR scriptCodeArrayGet(ARRAY_BLOCK	*psVariable,// The variable to get from
						  CODE_BLOCK	**ppsBlock)	// Generated code
{
//	SDWORD		elementDWords, i;
//	UBYTE		*pElement;

	ASSERT((PTRVALID(psVariable, sizeof(ARRAY_BLOCK)),
		"scriptCodeObjAssignment: Invalid object variable block pointer"));
	ASSERT((PTRVALID(psVariable->pCode, psVariable->size),
		"scriptCodeObjAssignment: Invalid object variable code pointer"));
	ASSERT((psVariable->psArrayVar != NULL,
		"scriptCodeObjAssignment: Invalid object variable symbol pointer"));
	ASSERT((ppsBlock != NULL,
		"scriptCodeObjAssignment: Invalid generated code block pointer"));

	// Check this is an array
	if (psVariable->psArrayVar->dimensions == 0)
	{
		scr_error("Not an array variable");
		return CE_PARSE;
	}

	// calculate the number of DWORDs needed to store the number of elements for each dimension of the array
//	elementDWords = (psVariable->psArrayVar->dimensions - 1)/4 + 1;

//	ALLOC_BLOCK(*ppsBlock, psVariable->size + sizeof(OPCODE) + elementDWords*4);
	ALLOC_BLOCK(*ppsBlock, psVariable->size + sizeof(OPCODE));
	ip = (*ppsBlock)->pCode;

	/* Copy in the code for the array index */
	PUT_BLOCK(ip, psVariable);
	(*ppsBlock)->type = psVariable->psArrayVar->type;

	/* Code to get the value from the array onto the stack */
	PUT_PKOPCODE(ip, OP_PUSHARRAYGLOBAL,
		((psVariable->psArrayVar->dimensions << ARRAY_DIMENSION_SHIFT) & ARRAY_DIMENSION_MASK) |
		(psVariable->psArrayVar->index & ARRAY_BASE_MASK) );

	// store the size of each dimension
/*	pElement = (UBYTE *)ip;
	for(i=0; i<psVariable->psArrayVar->dimensions; i++)
	{
		*pElement = (UBYTE)psVariable->psArrayVar->elements[i];
		pElement += 1;
	}*/

	/* Free the variable block */
	FREE_ARRAYBLOCK(psVariable);

	return CE_OK;
}

/* Generate the final code block for conditional statements */
CODE_ERROR scriptCodeConditional(
					COND_BLOCK *psCondBlock,	// The intermediate conditional code
					CODE_BLOCK **ppsBlock)		// The final conditional code
{
	UDWORD		i;

	ASSERT((PTRVALID(psCondBlock, sizeof(CODE_BLOCK)),
		"scriptCodeConditional: Invalid conditional code block pointer"));
	ASSERT((PTRVALID(psCondBlock->pCode, psCondBlock->size),
		"scriptCodeConditional: Invalid conditional code pointer"));
	ASSERT((ppsBlock != NULL,
		"scriptCodeConditional: Invalid generated code block pointer"));

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
		*ip = ((*ppsBlock)->size / sizeof(UDWORD)) - (ip - (*ppsBlock)->pCode);
		*ip = (OP_JUMP << OPCODE_SHIFT) | ( (*ip) & OPCODE_DATAMASK );
	}

	/* Free the original code */
	FREE_DEBUG(psCondBlock);
	FREE_CONDBLOCK(psCondBlock);

	return CE_OK;
}

/* Generate code for function parameters */
CODE_ERROR scriptCodeParameter(CODE_BLOCK		*psParam,		// Code for the parameter
							 INTERP_TYPE		type,			// Parameter type
							 PARAM_BLOCK	**ppsBlock)		// Generated code
{
	ASSERT((PTRVALID(psParam, sizeof(CODE_BLOCK)),
		"scriptCodeParameter: Invalid parameter code block pointer"));
	ASSERT((PTRVALID(psParam->pCode, psParam->size),
		"scriptCodeParameter: Invalid parameter code pointer"));
	ASSERT((ppsBlock != NULL,
		"scriptCodeParameter: Invalid generated code block pointer"));

	ALLOC_PBLOCK(*ppsBlock, psParam->size, 1);
	ip = (*ppsBlock)->pCode;

	/* Copy in the code for the parameter */
	PUT_BLOCK(ip, psParam);
	FREE_BLOCK(psParam);
	
	(*ppsBlock)->aParams[0] = type;

	return CE_OK;
}

/* Generate code for binary operators (e.g. 2 + 2) */
CODE_ERROR scriptCodeBinaryOperator(CODE_BLOCK	*psFirst,	// Code for first parameter
								  CODE_BLOCK	*psSecond,	// Code for second parameter
								  OPCODE		opcode,		// Operator function
								  CODE_BLOCK	**ppsBlock) // Generated code
{
	ALLOC_BLOCK(*ppsBlock, psFirst->size + psSecond->size + sizeof(UDWORD));
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

/* Generate code for accessing an object variable.  The variable symbol is
 * stored with the code for the object value so that this block can be used for
 * both setting and retrieving an object value.
 */
CODE_ERROR scriptCodeObjectVariable(CODE_BLOCK	*psObjCode,	// Code for the object value
								  VAR_SYMBOL	*psVar,		// The object variable symbol
								  OBJVAR_BLOCK	**ppsBlock) // Generated code
{
	ASSERT((PTRVALID(psObjCode, sizeof(CODE_BLOCK)),
		"scriptCodeObjectVariable: Invalid object code block pointer"));
	ASSERT((PTRVALID(psObjCode->pCode, psObjCode->size),
		"scriptCodeObjectVariable: Invalid object code pointer"));
	ASSERT((psVar != NULL,
		"scriptCodeObjectVariable: Invalid variable symbol pointer"));
	ASSERT((ppsBlock != NULL,
		"scriptCodeObjectVariable: Invalid generated code block pointer"));

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
CODE_ERROR scriptCodeArrayVariable(ARRAY_BLOCK	*psArrayCode,	// Code for the array index
								  VAR_SYMBOL	*psVar,			// The array variable symbol
								  ARRAY_BLOCK	**ppsBlock)		// Generated code
{
	ASSERT((PTRVALID(psArrayCode, sizeof(CODE_BLOCK)),
		"scriptCodeObjectVariable: Invalid object code block pointer"));
	ASSERT((PTRVALID(psArrayCode->pCode, psArrayCode->size),
		"scriptCodeObjectVariable: Invalid object code pointer"));
	ASSERT((psVar != NULL,
		"scriptCodeObjectVariable: Invalid variable symbol pointer"));
	ASSERT((ppsBlock != NULL,
		"scriptCodeObjectVariable: Invalid generated code block pointer"));

/*	ALLOC_ARRAYBLOCK(*ppsBlock, psArrayCode->size, psVar);
	ip = (*ppsBlock)->pCode;

	// Copy the already generated bit of code into the code block
	PUT_BLOCK(ip, psArrayCode);
	FREE_BLOCK(psArrayCode);*/

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
CODE_ERROR scriptCodeConstant(CONST_SYMBOL	*psConst,	// The object variable symbol
							CODE_BLOCK		**ppsBlock)	// Generated code
{
	ASSERT((psConst != NULL,
		"scriptCodeConstant: Invalid constant symbol pointer"));
	ASSERT((ppsBlock != NULL,
		"scriptCodeConstant: Invalid generated code block pointer"));

	ALLOC_BLOCK(*ppsBlock, sizeof(OPCODE) + sizeof(UDWORD));
	ip = (*ppsBlock)->pCode;
	(*ppsBlock)->type = psConst->type;

	/* Put the value onto the stack */
	switch (psConst->type)
	{
	case VAL_BOOL:
		PUT_PKOPCODE(ip, OP_PUSH, VAL_BOOL);
		PUT_DATA(ip, psConst->bval);
		break;
	case VAL_INT:
		PUT_PKOPCODE(ip, OP_PUSH, VAL_INT);
		PUT_DATA(ip, psConst->ival);
		break;
/*	case VAL_FLOAT:
		PUT_PKOPCODE(ip, OP_PUSH, VAL_FLOAT);
		PUT_DATA(ip, psConst->fval);
		break;*/
	default:
		PUT_PKOPCODE(ip, OP_PUSH, psConst->type);
		PUT_DATA(ip, psConst->oval);
		break;
	}

	return CE_OK;
}

/* Generate code for getting a variables value */
CODE_ERROR scriptCodeVarGet(VAR_SYMBOL		*psVariable,	// The object variable symbol
							CODE_BLOCK		**ppsBlock)		// Generated code
{
	SDWORD	size;

	size = sizeof(OPCODE);
	if (psVariable->storage == ST_EXTERN)
	{
		// Check there is a set function
		if (psVariable->get == NULL)
		{
			scr_error("No get function for external variable");
			return CE_PARSE;
		}
		size += sizeof(SCRIPT_VARFUNC);
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

/* Generate code for getting a variables value */
CODE_ERROR scriptCodeVarRef(VAR_SYMBOL		*psVariable,	// The object variable symbol
							PARAM_BLOCK		**ppsBlock)		// Generated code
{
	SDWORD	size;

	size = sizeof(OPCODE) + sizeof(SDWORD);
	ALLOC_PBLOCK(*ppsBlock, size, 1);
	ip = (*ppsBlock)->pCode;
	(*ppsBlock)->aParams[0] = psVariable->type | VAL_REF;

	/* Code to get the value onto the stack */
	switch (psVariable->storage)
	{
	case ST_PUBLIC:
	case ST_PRIVATE:
		PUT_PKOPCODE(ip, OP_PUSHREF, (*ppsBlock)->aParams[0]);
		PUT_DATA(ip, psVariable->index);
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
		scr_error("Unknown storage type");
		return CE_PARSE;
		break;
	}

	return CE_OK;
}

/* Generate the code for a trigger and store it in the trigger list */
CODE_ERROR scriptCodeTrigger(STRING *pIdent, CODE_BLOCK *psCode)
{
	CODE_BLOCK		*psNewBlock;
	UDWORD			line;
	STRING			*pDummy;

	pIdent = pIdent;

	// Have to add the exit code to the end of the event
	ALLOC_BLOCK(psNewBlock, psCode->size + sizeof(OPCODE));
	ip = psNewBlock->pCode;
	PUT_BLOCK(ip, psCode);
	PUT_OPCODE(ip, OP_EXIT);

	// Add the debug info
	ALLOC_DEBUG(psNewBlock, psCode->debugEntries + 1);
	PUT_DEBUG(psNewBlock, psCode);
	if (genDebugInfo)
	{
		/* Add debugging info for the EXIT instruction */
		scriptGetErrorData((SDWORD *)&line, &pDummy);
		psNewBlock->psDebug[psNewBlock->debugEntries].line = line;
		psNewBlock->psDebug[psNewBlock->debugEntries].offset = 
				ip - psNewBlock->pCode;
		psNewBlock->debugEntries ++;
	}
	FREE_BLOCK(psCode);

	// Create the trigger
/*	if (!scriptAddTrigger(pIdent, psNewBlock))
	{
		return CE_MEMORY;
	}*/

	return CE_OK;
}

/* Generate the code for an event and store it in the event list */
CODE_ERROR scriptCodeEvent(EVENT_SYMBOL *psEvent, TRIGGER_SYMBOL *psTrig, CODE_BLOCK *psCode)
{
	CODE_BLOCK		*psNewBlock;
	UDWORD			line;
	STRING			*pDummy;

	// Have to add the exit code to the end of the event
	ALLOC_BLOCK(psNewBlock, psCode->size + sizeof(OPCODE));
	ip = psNewBlock->pCode;
	PUT_BLOCK(ip, psCode);
	PUT_OPCODE(ip, OP_EXIT);

	// Add the debug info
	ALLOC_DEBUG(psNewBlock, psCode->debugEntries + 1);
	PUT_DEBUG(psNewBlock, psCode);
	if (genDebugInfo)
	{
		/* Add debugging info for the EXIT instruction */
		scriptGetErrorData((SDWORD *)&line, &pDummy);
		psNewBlock->psDebug[psNewBlock->debugEntries].line = line;
		psNewBlock->psDebug[psNewBlock->debugEntries].offset = 
				ip - psNewBlock->pCode;
		psNewBlock->debugEntries ++;
	}
	FREE_BLOCK(psCode);

	// Create the event
	if (!scriptDefineEvent(psEvent, psNewBlock, psTrig->index))
	{
		return CE_MEMORY;
	}

	return CE_OK;
}

/* Store the types of a list of variables into a code block.
 * The order of the list is reversed so that the type of the
 * first variable defined is stored first.
 */
static void scriptStoreVarTypes(VAR_SYMBOL *psVar)
{
	if (psVar != NULL)
	{
		/* Recurse down the list to get to the end of it */
		scriptStoreVarTypes(psVar->psNext);

		/* Now store the current variable */
		PUT_INDEX(ip, psVar->type);
	}
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

typedef union {
	/* Types returned by the lexer */
	BOOL			bval;
	/*	float			fval; */
	SDWORD			ival;
	STRING			*sval;
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
	VAR_DECL		*vdecl;
	VAR_IDENT_DECL	*videcl;
} YYSTYPE;
#define FUNCTION	257
#define TRIGGER	258
#define EVENT	259
#define WAIT	260
#define EVERY	261
#define INACTIVE	262
#define INITIALISE	263
#define LINK	264
#define REF	265
#define WHILE	266
#define IF	267
#define ELSE	268
#define EXIT	269
#define PAUSE	270
#define BOOLEQUAL	271
#define NOTEQUAL	272
#define GREATEQUAL	273
#define LESSEQUAL	274
#define GREATER	275
#define LESS	276
#define _AND	277
#define _OR	278
#define _NOT	279
#define UMINUS	280
#define BOOLEAN	281
#define INTEGER	282
#define QTEXT	283
#define TYPE	284
#define STORAGE	285
#define IDENT	286
#define VAR	287
#define BOOL_VAR	288
#define NUM_VAR	289
#define OBJ_VAR	290
#define VAR_ARRAY	291
#define BOOL_ARRAY	292
#define NUM_ARRAY	293
#define OBJ_ARRAY	294
#define BOOL_OBJVAR	295
#define NUM_OBJVAR	296
#define USER_OBJVAR	297
#define OBJ_OBJVAR	298
#define BOOL_CONSTANT	299
#define NUM_CONSTANT	300
#define USER_CONSTANT	301
#define OBJ_CONSTANT	302
#define FUNC	303
#define BOOL_FUNC	304
#define NUM_FUNC	305
#define USER_FUNC	306
#define OBJ_FUNC	307
#define TRIG_SYM	308
#define EVENT_SYM	309
#define CALLBACK_SYM	310
extern int scr_char, scr_yyerrflag;
extern YYSTYPE scr_lval;
#if YYDEBUG
enum YY_Types { YY_t_NoneDefined, YY_t_bval, YY_t_ival, YY_t_sval, YY_t_tval, YY_t_stype, YY_t_vSymbol, YY_t_cSymbol, YY_t_fSymbol, YY_t_tSymbol, YY_t_eSymbol, YY_t_cbSymbol, YY_t_cblock, YY_t_objVarBlock, YY_t_arrayBlock, YY_t_pblock, YY_t_condBlock, YY_t_videcl, YY_t_vdecl, YY_t_tdecl
};
#endif
#if YYDEBUG
yyTypedRules yyRules[] = {
	{ "&00: %35 &00",  0},
	{ "%38:",  0},
	{ "%35: %36 %37 %38 %39 %40",  0},
	{ "%36:",  0},
	{ "%36: %41",  0},
	{ "%36: %36 %41",  0},
	{ "%41: &09 &33 &60",  0},
	{ "%37:",  0},
	{ "%37: %32 &60",  0},
	{ "%37: %37 %32 &60",  0},
	{ "%31: &34 &33",  18},
	{ "%31: &34 &03",  18},
	{ "%31: &34 &04",  18},
	{ "%28: &61 &31 &62",  17},
	{ "%29: %28",  17},
	{ "%29: %29 &61 &31 &62",  17},
	{ "%30: &35",  17},
	{ "%30: &35 %29",  17},
	{ "%32: %31 %30",  18},
	{ "%32: %32 &63 %30",  18},
	{ "%39:",  0},
	{ "%39: %42",  0},
	{ "%39: %39 %42",  0},
	{ "%33: %02 &63 &31",  19},
	{ "%33: &05 &63 &31",  19},
	{ "%33: &06 &63 &31",  19},
	{ "%33: &08",  19},
	{ "%33: &59",  19},
	{ "%33: &59 &63 %20",  19},
	{ "%42: &03 &35 &64 %33 &65 &60",  0},
	{ "%40: %43",  0},
	{ "%40: %40 %43",  0},
	{ "%34: &04 &35",  10},
	{ "%34: &04 &58",  10},
	{ "%43: %34 &60",  0},
	{ "%43: %34 &64 &57 &65 &66 %16 &67",  0},
	{ "%44:",  0},
	{ "%43: %34 &64 %33 &65 %44 &66 %16 &67",  0},
	{ "%43: %34 &64 &07 &65 &66 %16 &67",  0},
	{ "%16:",  12},
	{ "%16: %17",  12},
	{ "%16: %16 %17",  12},
	{ "%17: %18 &60",  12},
	{ "%17: %19 &60",  12},
	{ "%17: %23",  12},
	{ "%17: %27",  12},
	{ "%17: &14 &60",  12},
	{ "%17: &15 &64 &31 &65 &60",  12},
	{ "%18: &38 &68 %01",  12},
	{ "%18: &37 &68 %02",  12},
	{ "%18: &39 &68 %03",  12},
	{ "%18: &36 &68 %05",  12},
	{ "%18: %06 &68 %01",  12},
	{ "%18: %07 &68 %02",  12},
	{ "%18: %08 &68 %05",  12},
	{ "%18: %09 &68 %03",  12},
	{ "%18: %12 &68 %01",  12},
	{ "%18: %13 &68 %02",  12},
	{ "%18: %14 &68 %05",  12},
	{ "%18: %15 &68 %03",  12},
	{ "%19: &54 &64 %20 &65",  12},
	{ "%19: &53 &64 %20 &65",  12},
	{ "%19: &55 &64 %20 &65",  12},
	{ "%19: &56 &64 %20 &65",  12},
	{ "%19: &52 &64 %20 &65",  12},
	{ "%20:",  15},
	{ "%20: %21",  15},
	{ "%20: %20 &63 %21",  15},
	{ "%21: %01",  15},
	{ "%21: %02",  15},
	{ "%21: %05",  15},
	{ "%21: %03",  15},
	{ "%21: %22",  15},
	{ "%22: &10 &38",  15},
	{ "%22: &10 &37",  15},
	{ "%22: &10 &36",  15},
	{ "%22: &10 &39",  15},
	{ "%23: %26",  12},
	{ "%23: %26 &13 %25",  12},
	{ "%26: %24",  16},
	{ "%26: %26 &13 %24",  16},
	{ "%25: &66 %16 &67",  16},
	{ "%45:",  0},
	{ "%24: &12 &64 %02 &65 %45 &66 %16 &67",  16},
	{ "%46:",  0},
	{ "%27: &11 &64 %02 &65 %46 &66 %16 &67",  12},
	{ "%01: %01 &26 %01",  12},
	{ "%01: %01 &25 %01",  12},
	{ "%01: %01 &27 %01",  12},
	{ "%01: %01 &28 %01",  12},
	{ "%01: &25 %01",  12},
	{ "%01: &64 %01 &65",  12},
	{ "%01: &54 &64 %20 &65",  12},
	{ "%01: &38",  12},
	{ "%01: &49",  12},
	{ "%01: %06",  12},
	{ "%01: %12",  12},
	{ "%01: &31",  12},
	{ "%02: %02 &22 %02",  12},
	{ "%02: %02 &23 %02",  12},
	{ "%02: %02 &16 %02",  12},
	{ "%02: %02 &17 %02",  12},
	{ "%02: &24 %02",  12},
	{ "%02: &64 %02 &65",  12},
	{ "%02: &53 &64 %20 &65",  12},
	{ "%02: &37",  12},
	{ "%02: &48",  12},
	{ "%02: %07",  12},
	{ "%02: %13",  12},
	{ "%02: &30",  12},
	{ "%02: %01 &16 %01",  12},
	{ "%02: %05 &16 %05",  12},
	{ "%02: %03 &16 %03",  12},
	{ "%02: %01 &17 %01",  12},
	{ "%02: %05 &17 %05",  12},
	{ "%02: %03 &17 %03",  12},
	{ "%02: %01 &19 %01",  12},
	{ "%02: %01 &18 %01",  12},
	{ "%02: %01 &20 %01",  12},
	{ "%02: %01 &21 %01",  12},
	{ "%05: &36",  12},
	{ "%05: &50",  12},
	{ "%05: %08",  12},
	{ "%05: %14",  12},
	{ "%05: &55 &64 %20 &65",  12},
	{ "%05: &57",  12},
	{ "%05: &07",  12},
	{ "%05: &58",  12},
	{ "%03: &39",  12},
	{ "%03: &51",  12},
	{ "%03: &56 &64 %20 &65",  12},
	{ "%03: %09",  12},
	{ "%03: %15",  12},
	{ "%04: %03 &69",  12},
	{ "%06: %04 &45",  13},
	{ "%07: %04 &44",  13},
	{ "%08: %04 &46",  13},
	{ "%09: %04 &47",  13},
	{ "%10: &61 %01 &62",  14},
	{ "%11: %10",  14},
	{ "%11: %11 &61 %01 &62",  14},
	{ "%12: &42 %11",  14},
	{ "%13: &41 %11",  14},
	{ "%15: &43 %11",  14},
	{ "%14: &40 %11",  14},
{ "$accept",  0},{ "error",  0}
};
yyNamedType yyTokenTypes[] = {
	{ "$end",  0,  0},
	{ "error",  256,  0},
	{ "FUNCTION",  257,  0},
	{ "TRIGGER",  258,  0},
	{ "EVENT",  259,  0},
	{ "WAIT",  260,  0},
	{ "EVERY",  261,  0},
	{ "INACTIVE",  262,  0},
	{ "INITIALISE",  263,  0},
	{ "LINK",  264,  0},
	{ "REF",  265,  0},
	{ "WHILE",  266,  0},
	{ "IF",  267,  0},
	{ "ELSE",  268,  0},
	{ "EXIT",  269,  0},
	{ "PAUSE",  270,  0},
	{ "BOOLEQUAL",  271,  0},
	{ "NOTEQUAL",  272,  0},
	{ "GREATEQUAL",  273,  0},
	{ "LESSEQUAL",  274,  0},
	{ "GREATER",  275,  0},
	{ "LESS",  276,  0},
	{ "_AND",  277,  0},
	{ "_OR",  278,  0},
	{ "_NOT",  279,  0},
	{ "'-'",  45,  0},
	{ "'+'",  43,  0},
	{ "'*'",  42,  0},
	{ "'/'",  47,  0},
	{ "UMINUS",  280,  0},
	{ "BOOLEAN",  281,  1},
	{ "INTEGER",  282,  2},
	{ "QTEXT",  283,  3},
	{ "TYPE",  284,  4},
	{ "STORAGE",  285,  5},
	{ "IDENT",  286,  3},
	{ "VAR",  287,  6},
	{ "BOOL_VAR",  288,  6},
	{ "NUM_VAR",  289,  6},
	{ "OBJ_VAR",  290,  6},
	{ "VAR_ARRAY",  291,  6},
	{ "BOOL_ARRAY",  292,  6},
	{ "NUM_ARRAY",  293,  6},
	{ "OBJ_ARRAY",  294,  6},
	{ "BOOL_OBJVAR",  295,  6},
	{ "NUM_OBJVAR",  296,  6},
	{ "USER_OBJVAR",  297,  6},
	{ "OBJ_OBJVAR",  298,  6},
	{ "BOOL_CONSTANT",  299,  7},
	{ "NUM_CONSTANT",  300,  7},
	{ "USER_CONSTANT",  301,  7},
	{ "OBJ_CONSTANT",  302,  7},
	{ "FUNC",  303,  8},
	{ "BOOL_FUNC",  304,  8},
	{ "NUM_FUNC",  305,  8},
	{ "USER_FUNC",  306,  8},
	{ "OBJ_FUNC",  307,  8},
	{ "TRIG_SYM",  308,  9},
	{ "EVENT_SYM",  309,  10},
	{ "CALLBACK_SYM",  310,  11},
	{ "';'",  59,  0},
	{ "'['",  91,  0},
	{ "']'",  93,  0},
	{ "','",  44,  0},
	{ "'('",  40,  0},
	{ "')'",  41,  0},
	{ "'{'",  123,  0},
	{ "'}'",  125,  0},
	{ "'='",  61,  0},
	{ "'.'",  46,  0}

};
#endif
static short yydef[] = {

	65535, 65527, 65521, 65517,    5, 65511,    6,    4,    7,   56, 
	  57,   41,   40,   39,   38, 65507, 65501, 65495, 65489, 65483, 
	  24,   23,   22,   21,   33,   31,   37,   36,   35,   34, 
	  32,   30,   27,   26,    8,   29,   28, 65477, 65473,   25, 
	  50,   51,   54, 65469, 65465, 65459, 65453, 65447, 65441, 65435, 
	  18,   17,   16,   15,   14,   13,   12,   11,   10, 65431, 
	65425, -115
};
static short yyex[] = {

	 258,  146,  259,  146,  285,  146, 65535,    1,  258,  145, 
	 259,  145, 65535,    1,    0,    0, 65535,    1,  258,    3, 
	 259,    3, 65535,    1,  259,  144, 65535,    1,   44,   20, 
	  41,   20, 65535,    1,   44,   20,   41,   20, 65535,    1, 
	  44,   20,   41,   20, 65535,    1,   44,   20,   41,   20, 
	65535,    1,   44,   20,   41,   20, 65535,    1,  125,    9, 
	65535,    1,  125,    9, 65535,    1,  125,    9, 65535,    1, 
	  44,   20,   41,   20, 65535,    1,   44,   20,   41,   20, 
	65535,    1,   44,   20,   41,   20, 65535,    1,   44,   20, 
	  41,   20, 65535,    1,   44,   20,   41,   20, 65535,    1, 
	 125,    9, 65535,    1,   59,   19,   46,   52, 65535,    1, 
	 125,    9, 65535,    1,  125,    9, 65535,    1
};
static short yyact[] = {

	65340,  264, 65341,  284, 65340, 65343,  285,  264, 65245,   59, 
	65531,  286, 65249, 65250, 65248,  284,  259,  258, 65345, 65246, 
	  59,   44, 65343,  285, 65347,   91, 65345, 65247,   59,   44, 
	65348,  258, 65350,   91, 65351,  282, 65352,  286, 65348, 65354, 
	 259,  258, 65355,  282, 65251,   93, 65356,   40, 65357, 65238, 
	  59,   40, 65261, 65262,  309,  286, 65354,  259, 65253,   93, 
	65370, 65371, 65375, 65374, 65325, 65259, 65368, 65316, 65306, 65319, 
	65312, 65302, 65327, 65358, 65360, 65361, 65359, 65313, 65303, 65320, 
	65328, 65367, 65369, 65364, 65363, 65324, 65326, 65527,  310,  309, 
	 308,  307,  306,  305,  304,  302,  301,  300,  299,  294, 
	 293,  292,  291,  290,  289,  288,  287,  282,  281,  279, 
	 263,  262,  261,  260,   45,   40, 65370, 65371, 65375, 65374, 
	65526, 65259, 65368, 65316, 65306, 65319, 65312, 65302, 65327, 65358, 
	65360, 65361, 65359, 65313, 65303, 65320, 65328, 65367, 65369, 65364, 
	65363, 65525, 65326, 65527,  310,  309,  308,  307,  306,  305, 
	 304,  302,  301,  300,  299,  294,  293,  292,  291,  290, 
	 289,  288,  287,  282,  281,  279,  263,  262,  261,  260, 
	  45,   40, 65378,   91, 65334, 65333, 65335, 65336,  298,  297, 
	 296,  295, 65520,   40, 65519,   40, 65332, 65380, 65379,  272, 
	 271,   46, 65382, 65381,  272,  271, 65518,   40, 65370, 65371, 
	65325, 65368, 65316, 65306, 65319, 65312, 65302, 65327, 65358, 65360, 
	65361, 65359, 65313, 65303, 65320, 65328, 65367, 65369, 65364, 65363, 
	65324, 65326,  309,  308,  307,  306,  305,  304,  302,  301, 
	 300,  299,  294,  293,  292,  291,  290,  289,  288,  287, 
	 282,  281,  279,  262,   45,   40, 65517,   40, 65387, 65371, 
	65306, 65302, 65327, 65361, 65359, 65303, 65328, 65369, 65363,  307, 
	 305,  302,  300,  294,  293,  290,  289,  282,   45,   40, 
	65395, 65397, 65396, 65394, 65393, 65392, 65390, 65391, 65389, 65388, 
	 276,  275,  274,  273,  272,  271,   47,   45,   43,   42, 
	65398,   41, 65516,   44, 65399,   44, 65400,   44, 65405, 65402, 
	65401, 65404, 65403,  278,  277,  272,  271,   44, 65406,   41, 
	65264,   41, 65407,   41, 65408,   91, 65370, 65371, 65325, 65410, 
	65368, 65316, 65306, 65319, 65312, 65302, 65327, 65358, 65360, 65361, 
	65359, 65313, 65303, 65320, 65328, 65367, 65369, 65364, 65363, 65324, 
	65326,  309,  308,  307,  306,  305,  304,  302,  301,  300, 
	 299,  294,  293,  292,  291,  290,  289,  288,  287,  282, 
	 281,  279,  265,  262,   45,   40, 65327, 65359, 65328, 65363, 
	 307,  302,  294,  290, 65325, 65319, 65327, 65358, 65359, 65320, 
	65328, 65364, 65363, 65324, 65326,  309,  308,  307,  306,  302, 
	 301,  294,  291,  290,  287,  262, 65310, 65402, 65401, 65404, 
	65403,  278,  277,  272,  271,   41, 65300, 65395, 65397, 65396, 
	65394, 65393, 65392, 65390, 65391, 65389, 65388,  276,  275,  274, 
	 273,  272,  271,   47,   45,   43,   42,   41, 65333, 65336, 
	 298,  296, 65332,   46, 65260,   59, 65258,  282, 65257,  282, 
	65256,  282, 65498,  123, 65497,  123, 65395, 65397, 65396, 65394, 
	65337,   93,   47,   45,   43,   42, 65287, 65286, 65285, 65288, 
	 290,  289,  288,  287, 65402, 65401, 65404, 65403,  278,  277, 
	 272,  271, 65329, 65420,   44,   41, 65323, 65420,   44,   41, 
	65336,  298, 65335, 65336,  298,  297, 65311, 65420,   44,   41, 
	65301, 65420,   44,   41, 65300, 65395, 65397, 65396, 65394,   47, 
	  45,   43,   42,   41, 65395, 65397, 65396, 65394,   47,   45, 
	  43,   42, 65395, 65394,   47,   42, 65420,   44, 65402, 65401, 
	 272,  271, 65421, 65422, 65438, 65437, 65434, 65435, 65436, 65493, 
	65358, 65360, 65361, 65359, 65328, 65423, 65426, 65427, 65425, 65424, 
	 307,  306,  305,  304,  303,  302,  294,  293,  292,  291, 
	 290,  289,  288,  287,  270,  269,  267,  266, 65492,  123, 
	65395, 65397, 65396, 65394, 65339,   93,   47,   45,   43,   42, 
	65443,   40, 65444,   40, 65445,  268, 65491,   40, 65490,   40, 
	65489,   40, 65488,   40, 65487,   40, 65446,   61, 65447,   61, 
	65448,   61, 65449,   61, 65450,   61, 65451,   61, 65452,   61, 
	65453,   61, 65454,   61, 65455,   61, 65456,   61, 65457,   61, 
	65458,   40, 65273,   59, 65270,   59, 65269,   59, 65266, 65421, 
	65422, 65438, 65437, 65434, 65435, 65436, 65493, 65358, 65360, 65361, 
	65359, 65328, 65423, 65426, 65427, 65425, 65424,  307,  306,  305, 
	 304,  303,  302,  294,  293,  292,  291,  290,  289,  288, 
	 287,  270,  269,  267,  266,  125, 65263, 65421, 65422, 65438, 
	65437, 65434, 65435, 65436, 65493, 65358, 65360, 65361, 65359, 65328, 
	65423, 65426, 65427, 65425, 65424,  307,  306,  305,  304,  303, 
	 302,  294,  293,  292,  291,  290,  289,  288,  287,  270, 
	 269,  267,  266,  125, 65486, 65422,  267,  123, 65467,  282, 
	65265, 65421, 65422, 65438, 65437, 65434, 65435, 65436, 65493, 65358, 
	65360, 65361, 65359, 65328, 65423, 65426, 65427, 65425, 65424,  307, 
	 306,  305,  304,  303,  302,  294,  293,  292,  291,  290, 
	 289,  288,  287,  270,  269,  267,  266,  125, 65295, 65402, 
	65401, 65404, 65403,  278,  277,  272,  271,   41, 65293, 65402, 
	65401, 65404, 65403,  278,  277,  272,  271,   41, 65281, 65420, 
	  44,   41, 65476, 65420,   44,   41, 65280, 65420,   44,   41, 
	65279, 65420,   44,   41, 65278, 65420,   44,   41, 65469,   41, 
	65292, 65421, 65422, 65438, 65437, 65434, 65435, 65436, 65493, 65358, 
	65360, 65361, 65359, 65328, 65423, 65426, 65427, 65425, 65424,  307, 
	 306,  305,  304,  303,  302,  294,  293,  292,  291,  290, 
	 289,  288,  287,  270,  269,  267,  266,  125, 65274,   59, 
	65475,  123, 65474,  123, 65296, 65421, 65422, 65438, 65437, 65434, 
	65435, 65436, 65493, 65358, 65360, 65361, 65359, 65328, 65423, 65426, 
	65427, 65425, 65424,  307,  306,  305,  304,  303,  302,  294, 
	 293,  292,  291,  290,  289,  288,  287,  270,  269,  267, 
	 266,  125, 65294, 65421, 65422, 65438, 65437, 65434, 65435, 65436, 
	65493, 65358, 65360, 65361, 65359, 65328, 65423, 65426, 65427, 65425, 
	65424,  307,  306,  305,  304,  303,  302,  294,  293,  292, 
	 291,  290,  289,  288,  287,  270,  269,  267,  266,  125,   -1
};
static short yypact[] = {

	   1,    6,    8,   23,   25,   31,   33,   57,  293,  309, 
	 313,  315,  315,  315,  315,  341,  341,  341,  341,  341, 
	 189,  194,  468,  280,  433,  433,  508,  508,  508,  508, 
	 508,  508,  514,  514,  517,  520,  520,  540,  540,  575, 
	 587,  595,  605,  540,  341,  341,  341,  341,  341,  540, 
	 433,  468,  508,  433,  468,  508,  433,  468,  508,  818, 
	 540,  540,  881,  843,  823,  821,  819,  799,  779,  776, 
	 772,  768,  764,  760,  753,  743,  719,  699,  259,  222, 
	 370,  385,  259,  222,  385,  370,  259,  222,  385,  370, 
	 696,  222,  222,  675,  637,  617,  615,  613,  611,  609, 
	 607,  603,  601,  599,  597,  593,  591,  589,  585,  583, 
	 581,  579,  577,  573,  571,  341,  565,  559,  499,  492, 
	 488,  484,  481,  478,  474,  460,  451,  259,  445,  443, 
	 441,  222,  222,  222,  222,  439,  437,  435,  259,  259, 
	 259,  259,  259,  259,  259,  259,  259,  259,  259,  433, 
	 430,  417,  401,  385,  385,  370,  370,  259,  311,  303, 
	 297,  295,  291,  280,  259,  222,  247,  222,  197,  194, 
	 189,  185,  183,  178,  173,  173,  173,  173,  144,   88, 
	  59,   54,   50,   47,   45,   43,   40,   37,   35,   28, 
	  11,   20,   15,   11,    9,    3
};
static short yygo[] = {

	65512, 65512, 65512, 65512, 65512, 65512, 65512, 65512, 65512, 65512, 
	65477, 65480, 65483, 65512, 65419, 65502, 65503, 65297, 65298, 65504, 
	65505, 65506, 65507, 65508, 65509, 65417, 65409, 65299, 65384, 65372, 
	 165,  164,  157,  148,  147,  146,  145,  144,  143,  142, 
	 141,  140,  139,  138,  127,  115,   86,   82,   78,   48, 
	  47,   46,   45,   44,   19,   18,   17,   16,   15, 65478, 
	65481, 65484, 65461, 65460, 65499, 65500, 65307, 65308, 65383, 65309, 
	65376, 65376, 65513,  179,  178,  167,  165,  134,  133,  132, 
	 131,   92,   91,   87,   83,   79, 65515, 65515, 65515, 65515, 
	65515, 65515, 65515, 65515, 65515, 65515, 65365, 65479, 65365, 65482, 
	65365, 65485, 65365, 65365, 65515, 65365, 65365, 65365, 65365, 65510, 
	65511, 65365, 65365, 65365, 65365, 65386,  179,  178,  167,  165, 
	 156,  155,  134,  133,  132,  131,  115,   92,   91,   89, 
	  87,   85,   83,   80,   79,   48,   47,   46,   45,   44, 
	  19,   18,   17,   16,   15, 65385, 65413, 65414, 65385, 65414, 
	65413, 65385, 65414, 65413, 65385, 65385, 65385, 65385, 65385, 65385, 
	65385, 65385, 65385, 65385, 65385, 65385, 65414, 65414, 65413, 65413, 
	65385, 65385, 65362,  164,  157,  156,  155,  154,  153,  148, 
	 147,  146,  145,  144,  143,  142,  141,  140,  139,  138, 
	 127,   89,   88,   86,   85,   84,   82,   81,   80,   78, 
	65514, 65514, 65514, 65514, 65514, 65514, 65514, 65514, 65514, 65514, 
	65275, 65276, 65277, 65514, 65317, 65318, 65366,  154,  153,  115, 
	  88,   84,   81,   48,   47,   46,   45,   44,   19,   18, 
	  17,   16,   15, 65433, 65433, 65433, 65433, 65433, 65433, 65433, 
	65433, 65433, 65433, 65433, 65433, 65304,   94,   93,   76,   67, 
	  63,   62,   61,   60,   49,   43,   38,   37, 65432, 65432, 
	65432, 65432, 65432, 65432, 65432, 65432, 65432, 65432, 65432, 65432, 
	65314,   94,   93,   76,   67,   63,   62,   61,   60,   49, 
	  43,   38,   37, 65431, 65431, 65431, 65431, 65431, 65431, 65431, 
	65431, 65431, 65431, 65431, 65431, 65321,   94,   93,   76,   67, 
	  63,   62,   61,   60,   49,   43,   38,   37, 65494, 65494, 
	65494, 65494, 65494, 65494, 65494, 65494, 65494, 65494, 65494, 65494, 
	65330,   94,   93,   76,   67,   63,   62,   61,   60,   49, 
	  43,   38,   37, 65338, 65521, 65522, 65523, 65524,  176,  175, 
	 174, 65430, 65430, 65430, 65430, 65430, 65430, 65430, 65430, 65430, 
	65430, 65430, 65430, 65305,   94,   93,   76,   67,   63,   62, 
	  61,   60,   49,   43,   38,   37, 65429, 65429, 65429, 65429, 
	65429, 65429, 65429, 65429, 65429, 65429, 65429, 65429, 65315,   94, 
	  93,   76,   67,   63,   62,   61,   60,   49,   43,   38, 
	  37, 65428, 65428, 65428, 65428, 65428, 65428, 65428, 65428, 65428, 
	65428, 65428, 65428, 65322,   94,   93,   76,   67,   63,   62, 
	  61,   60,   49,   43,   38,   37, 65495, 65495, 65495, 65495, 
	65495, 65495, 65495, 65495, 65495, 65495, 65495, 65495, 65331,   94, 
	  93,   76,   67,   63,   62,   61,   60,   49,   43,   38, 
	  37, 65442, 65459, 65468, 65472, 65473, 65441,   61,   60,   49, 
	  43,   38, 65268, 65268, 65268, 65268, 65268, 65268, 65267,   94, 
	  93,   76,   67,   63,   62, 65440, 65439, 65412, 65415, 65416, 
	65501, 65462, 65463, 65464, 65465, 65466, 65411,   48,   47,   46, 
	  45,   44,   19,   18,   17,   16, 65283, 65282,  115, 65284, 
	65271, 65291, 65290,   90, 65289, 65496, 65272, 65252, 65529, 65255, 
	65254,  190, 65342, 65346, 65344,    3, 65377, 65373,  178, 65353, 
	65533, 65534, 65532, 65530, 65349, 65528, 65243, 65244,    1, 65241, 
	65242,  186, 65239, 65240,    7, 65418, 65471, 65470,   -1
};
static short yypgo[] = {

	   0,    0,    0,  513,  510,  500,  500,  507,  507,  446, 
	 465,  465,  465,  465,  465,  465,  465,  465,  465,  466, 
	 476,  487,  487,  487,  487,  490,   29,   29,   72,   72, 
	  72,   72,   72,   72,   72,   72,   72,   72,  353,  378, 
	 428,  403,  337,  337,  333,  320,  295,  270,  245,  172, 
	 115,  115,  115,  115,  115,  216,  216,  216,  216,  216, 
	 216,  216,  216,   72,   72,   72,   72,   72,   72,   72, 
	  72,   72,   72,   72,   72,   29,   29,   29,   29,   29, 
	  29,   29,   29,   29,   29,  496,  527,  492,  526,  494, 
	 495,  495,  490,  489,  489,  489,  489,  487,  476,  476, 
	 466,  466,  466,  466,  465,  465,  465,  458,  458,  458, 
	 458,  458,  458,  446,  446,  523,  523,  525,  523,  509, 
	 509,  520,  507,  507,  507,  507,  504,  504,  498,  498, 
	 497,  502,  502,  502,  512,  512,  517,  511,  511,  514, 
	 514,  515,  515,  523,  514,  512,  511,    0
};
static short yyrlen[] = {

	   0,    0,    0,    0,    5,    1,    2,    1,    3,    0, 
	   3,    3,    3,    3,    3,    3,    3,    3,    3,    4, 
	   0,    1,    1,    1,    1,    1,    3,    3,    3,    3, 
	   3,    3,    3,    3,    3,    3,    3,    3,    2,    2, 
	   2,    2,    4,    1,    3,    2,    2,    2,    2,    2, 
	   1,    1,    4,    1,    1,    1,    1,    1,    4,    1, 
	   1,    1,    1,    3,    3,    1,    1,    1,    1,    1, 
	   4,    3,    2,    3,    3,    1,    1,    1,    1,    1, 
	   4,    3,    2,    3,    3,    8,    0,    8,    0,    3, 
	   3,    1,    3,    2,    2,    2,    2,    1,    3,    1, 
	   4,    4,    4,    4,    3,    3,    3,    5,    2,    1, 
	   1,    2,    2,    2,    1,    7,    8,    0,    7,    2, 
	   2,    6,    1,    3,    3,    3,    3,    2,    4,    1, 
	   3,    2,    2,    2,    3,    2,    3,    1,    2,    1, 
	   2,    1,    2,    2,    0,    0,    0,    2
};
#define YYS0	0
#define YYDELTA	154
#define YYNPACT	196
#define YYNDEF	62

#define YYr145	0
#define YYr146	1
#define YYr147	2
#define YYr1	3
#define YYr2	4
#define YYr16	5
#define YYr17	6
#define YYr27	7
#define YYr28	8
#define YYr39	9
#define YYr48	10
#define YYr49	11
#define YYr50	12
#define YYr52	13
#define YYr53	14
#define YYr55	15
#define YYr56	16
#define YYr57	17
#define YYr59	18
#define YYr63	19
#define YYr65	20
#define YYr68	21
#define YYr69	22
#define YYr70	23
#define YYr71	24
#define YYr77	25
#define YYr86	26
#define YYr87	27
#define YYr98	28
#define YYr99	29
#define YYr110	30
#define YYr112	31
#define YYr113	32
#define YYr115	33
#define YYr116	34
#define YYr117	35
#define YYr118	36
#define YYr119	37
#define YYr141	38
#define YYr142	39
#define YYr143	40
#define YYr144	41
#define YYr140	42
#define YYr139	43
#define YYr138	44
#define YYr137	45
#define YYr136	46
#define YYr135	47
#define YYr134	48
#define YYr133	49
#define YYr132	50
#define YYr131	51
#define YYr130	52
#define YYr129	53
#define YYr128	54
#define YYr127	55
#define YYr126	56
#define YYr125	57
#define YYr124	58
#define YYr123	59
#define YYr122	60
#define YYr121	61
#define YYr120	62
#define YYr114	63
#define YYr111	64
#define YYr109	65
#define YYr108	66
#define YYr107	67
#define YYr106	68
#define YYr105	69
#define YYr104	70
#define YYr103	71
#define YYr102	72
#define YYr101	73
#define YYr100	74
#define YYr97	75
#define YYr96	76
#define YYr95	77
#define YYr94	78
#define YYr93	79
#define YYr92	80
#define YYr91	81
#define YYr90	82
#define YYr89	83
#define YYr88	84
#define YYr85	85
#define YYr84	86
#define YYr83	87
#define YYr82	88
#define YYr81	89
#define YYr80	90
#define YYr79	91
#define YYr78	92
#define YYr76	93
#define YYr75	94
#define YYr74	95
#define YYr73	96
#define YYr72	97
#define YYr67	98
#define YYr66	99
#define YYr64	100
#define YYr62	101
#define YYr61	102
#define YYr60	103
#define YYr58	104
#define YYr54	105
#define YYr51	106
#define YYr47	107
#define YYr46	108
#define YYr45	109
#define YYr44	110
#define YYr43	111
#define YYr42	112
#define YYr41	113
#define YYr40	114
#define YYr38	115
#define YYr37	116
#define YYr36	117
#define YYr35	118
#define YYr33	119
#define YYr32	120
#define YYr29	121
#define YYr26	122
#define YYr25	123
#define YYr24	124
#define YYr23	125
#define YYr19	126
#define YYr18	127
#define YYr15	128
#define YYr14	129
#define YYr13	130
#define YYr12	131
#define YYr11	132
#define YYr10	133
#define YYr9	134
#define YYr8	135
#define YYr6	136
#define YYrACCEPT	YYr145
#define YYrERROR	YYr146
#define YYrLR2	YYr147
#if YYDEBUG
char * yysvar[] = {
	"$accept",
	"expression",
	"boolexp",
	"objexp",
	"objexp_dot",
	"userexp",
	"num_objvar",
	"bool_objvar",
	"user_objvar",
	"obj_objvar",
	"array_index",
	"array_index_list",
	"num_array_var",
	"bool_array_var",
	"user_array_var",
	"obj_array_var",
	"statement_list",
	"statement",
	"assignment",
	"func_call",
	"param_list",
	"parameter",
	"var_ref",
	"conditional",
	"cond_clause",
	"terminal_cond",
	"cond_clause_list",
	"loop",
	"array_sub_decl",
	"array_sub_decl_list",
	"variable_ident",
	"variable_decl_head",
	"variable_decl",
	"trigger_subdecl",
	"event_subdecl",
	"script",
	"header",
	"var_list",
	"$1",
	"trigger_list",
	"event_list",
	"header_decl",
	"trigger_decl",
	"event_decl",
	"$36",
	"$82",
	"$84",
	0
};
short yyrmap[] = {

	 145,  146,  147,    1,    2,   16,   17,   27,   28,   39, 
	  48,   49,   50,   52,   53,   55,   56,   57,   59,   63, 
	  65,   68,   69,   70,   71,   77,   86,   87,   98,   99, 
	 110,  112,  113,  115,  116,  117,  118,  119,  141,  142, 
	 143,  144,  140,  139,  138,  137,  136,  135,  134,  133, 
	 132,  131,  130,  129,  128,  127,  126,  125,  124,  123, 
	 122,  121,  120,  114,  111,  109,  108,  107,  106,  105, 
	 104,  103,  102,  101,  100,   97,   96,   95,   94,   93, 
	  92,   91,   90,   89,   88,   85,   84,   83,   82,   81, 
	  80,   79,   78,   76,   75,   74,   73,   72,   67,   66, 
	  64,   62,   61,   60,   58,   54,   51,   47,   46,   45, 
	  44,   43,   42,   41,   40,   38,   37,   36,   35,   33, 
	  32,   29,   26,   25,   24,   23,   19,   18,   15,   14, 
	  13,   12,   11,   10,    9,    8,    6,    4,    5,   21, 
	  22,   30,   31,   34,   20,    7,    3,    0
};
short yysmap[] = {

	   0,    3,    4,   10,   13,   20,   21,   36,   83,   88, 
	  90,   91,   94,   95,   96,  101,  102,  108,  110,  128, 
	 143,  144,  145,  146,  151,  152,  161,  162,  163,  164, 
	 165,  166,  169,  170,  172,  177,  178,  180,  182,  197, 
	 203,  207,  212,  223,  230,  231,  232,  233,  234,  257, 
	 265,  267,  268,  269,  271,  272,  274,  275,  276,  283, 
	 292,  293,  295,  294,  289,  288,  287,  281,  277,  264, 
	 263,  262,  261,  260,  256,  255,  253,  247,  246,  245, 
	 244,  243,  242,  241,  240,  239,  238,  237,  236,  235, 
	 229,  228,  227,  224,  222,  220,  219,  216,  215,  214, 
	 213,  211,  210,  209,  208,  206,  205,  204,  202,  201, 
	 200,  199,  198,  195,  194,  190,  183,  181,  160,  157, 
	 156,  153,  150,  149,  147,  141,  140,  139,  138,  136, 
	 135,  134,  133,  132,  131,  130,  129,  127,  126,  125, 
	 124,  123,  122,  121,  120,  119,  118,  117,  115,  114, 
	 113,  112,  111,  107,  106,  105,  104,   93,   89,   87, 
	  86,   85,   82,   81,   80,   79,   78,   72,   71,   65, 
	  64,   59,   53,   50,   49,   48,   47,   46,   40,   39, 
	  37,   33,   32,   31,   30,   29,   28,   26,   23,   19, 
	  17,    8,    7,    6,    5,    1,  225,   92,  184,   97, 
	  98,   99,  100,  103,   51,   52,  189,   54,   55,   56, 
	  57,   58,  191,   60,   61,   62,   63,  154,  155,   66, 
	  67,   68,   69,   70,  192,  158,  109,  175,  176,   73, 
	  74,   75,   76,   77,  193,  159,  116,  167,  168,  296, 
	 279,  297,  280,  290,  258,  196,  259,  185,  186,  187, 
	 188,  142,  226,  148,  282,  284,  285,  286,  266,  270, 
	 273,  291,  248,  217,  218,  249,  250,  251,  221,  252, 
	 278,  137,  254,   42,   43,  171,   84,  173,  174,  179, 
	  24,   12,   45,   22,   38,   14,   15,   16,   25,   18, 
	  11,    2,    9,   27,   35,   34,   44,   41
};
int yyntoken = 70;
int yynvar = 47;
int yynstate = 298;
int yynrule = 148;
#endif

#if YYDEBUG
/*
 * Package up YACC context for tracing
 */
typedef struct yyTraceItems_tag {
	int	state, lookahead, errflag, done;
	int	rule, npop;
	short	* states;
	int	nstates;
	YYSTYPE * values;
	int	nvalues;
	short	* types;
} yyTraceItems;
#endif

/*
 * Copyright 1985, 1990 by Mortice Kern Systems Inc.  All rights reserved.
 * 
 * Automaton to interpret LALR(1) tables.
 *
 * Macros:
 *	yyclearin - clear the lookahead token.
 *	yyerrok - forgive a pending error
 *	YYERROR - simulate an error
 *	YYACCEPT - halt and return 0
 *	YYABORT - halt and return 1
 *	YYRETURN(value) - halt and return value.  You should use this
 *		instead of return(value).
 *	YYREAD - ensure scr_char contains a lookahead token by reading
 *		one if it does not.  See also YYSYNC.
 *	YYRECOVERING - 1 if syntax error detected and not recovered
 *		yet; otherwise, 0.
 *
 * Preprocessor flags:
 *	YYDEBUG - includes debug code if 1.  The parser will print
 *		 a travelogue of the parse if this is defined as 1
 *		 and scr_debug is non-zero.
 *		yacc -t sets YYDEBUG to 1, but not scr_debug.
 *	YYTRACE - turn on YYDEBUG, and undefine default trace functions
 *		so that the interactive functions in 'ytrack.c' will
 *		be used.
 *	YYSSIZE - size of state and value stacks (default 150).
 *	YYSTATIC - By default, the state stack is an automatic array.
 *		If this is defined, the stack will be static.
 *		In either case, the value stack is static.
 *	YYALLOC - Dynamically allocate both the state and value stacks
 *		by calling malloc() and free().
 *	YYDYNAMIC - Dynamically allocate (and reallocate, if necessary)
 *		both the state and value stacks by calling malloc(),
 *		realloc(), and free().
 *	YYSYNC - if defined, yacc guarantees to fetch a lookahead token
 *		before any action, even if it doesnt need it for a decision.
 *		If YYSYNC is defined, YYREAD will never be necessary unless
 *		the user explicitly sets scr_char = -1
 *
 * Copyright (c) 1983, by the University of Waterloo
 */
/*
 * Prototypes
 */

extern int scr_lex YY_ARGS((void));
extern void scr_error YY_ARGS((char *, ...));

#if YYDEBUG

#include <stdlib.h>		/* common prototypes */
#include <string.h>

extern char *	yyValue YY_ARGS((YYSTYPE, int));	/* print scr_lval */
extern void yyShowState YY_ARGS((yyTraceItems *));
extern void yyShowReduce YY_ARGS((yyTraceItems *));
extern void yyShowGoto YY_ARGS((yyTraceItems *));
extern void yyShowShift YY_ARGS((yyTraceItems *));
extern void yyShowErrRecovery YY_ARGS((yyTraceItems *));
extern void yyShowErrDiscard YY_ARGS((yyTraceItems *));

extern void yyShowRead YY_ARGS((int));
#endif

/*
 * If YYDEBUG defined and scr_debug set,
 * tracing functions will be called at appropriate times in scr_parse()
 * Pass state of YACC parse, as filled into yyTraceItems yyx
 * If yyx.done is set by the tracing function, scr_parse() will terminate
 * with a return value of -1
 */
#define YY_TRACE(fn) { \
	yyx.state = yystate; yyx.lookahead = scr_char; yyx.errflag =scr_yyerrflag; \
	yyx.states = yys+1; yyx.nstates = yyps-yys; \
	yyx.values = yyv+1; yyx.nvalues = yypv-yyv; \
	yyx.types = yytypev+1; yyx.done = 0; \
	yyx.rule = yyi; yyx.npop = yyj; \
	fn(&yyx); \
	if (yyx.done) YYRETURN(-1); }

#ifndef I18N
#define m_textmsg(id, str, cls)	(str)
#else /*I18N*/
#include <m_nls.h>
#endif/*I18N*/

#ifndef YYSSIZE
# define YYSSIZE	150
#endif

#ifdef YYDYNAMIC
#define YYALLOC
char *getenv();
int atoi();
int yysinc = -1; /* stack size increment, <0 = double, 0 = none, >0 = fixed */
#endif

#ifdef YYALLOC
int yyssize = YYSSIZE;
#endif

#define YYERROR		goto yyerrlabel
#define yyerrok		scr_yyerrflag = 0
#if YYDEBUG
#define yyclearin	{ if (scr_debug) yyShowRead(-1); scr_char = -1; }
#else
#define yyclearin	scr_char = -1
#endif
#define YYACCEPT	YYRETURN(0)
#define YYABORT		YYRETURN(1)
#define YYRECOVERING()	(scr_yyerrflag != 0)
#ifdef YYALLOC
#define YYRETURN(val)	{ retval = (val); goto yyReturn; }
#else
#define YYRETURN(val)	return(val);
#endif
#if YYDEBUG
/* The if..else makes this macro behave exactly like a statement */
# define YYREAD	if (scr_char < 0) {					\
			if ((scr_char = scr_lex()) < 0)	{		\
				if (scr_char == -2) YYABORT; \
				scr_char = 0;				\
			}	/* endif */			\
			if (scr_debug)					\
				yyShowRead(scr_char);			\
		} else
#else
# define YYREAD	if (scr_char < 0) {					\
			if ((scr_char = scr_lex()) < 0) {			\
				if (scr_char == -2) YYABORT; \
				scr_char = 0;				\
			}	/* endif */			\
		} else
#endif

#define YYERRCODE	82		/* value of `error' */
#define YYTOKEN_BASE	256
#define	YYQYYP	yyq[yyq-yyp]

/*
 * Simulate bitwise negation as if was done on a two's complement machine.
 * This makes the generated code portable to machines with different
 * representations of integers (ie. signed magnitude).
 */
#define	yyneg(s)	(-((s)+1))

YYSTYPE	scr_yyval;				/* $ */
YYSTYPE	*scr_yypvt;				/* $n */
YYSTYPE	scr_lval;				/* scr_lex() sets this */

int	scr_char,				/* current token */
	scr_yyerrflag,			/* error flag */
	scr_yynerrs;			/* error count */

#if YYDEBUG
int scr_debug = 0;		/* debug if this flag is set */
extern char	*yysvar[];	/* table of non-terminals (aka 'variables') */
extern yyNamedType yyTokenTypes[];	/* table of terminals & their types */
extern short	yyrmap[], yysmap[];	/* map internal rule/states */
extern int	yynstate, yynvar, yyntoken, yynrule;

extern int	yyGetType YY_ARGS((int));	/* token type */
extern char	*yyptok YY_ARGS((int));	/* printable token string */
extern int	yyExpandName YY_ARGS((int, int, char *, int));
				  /* expand yyRules[] or yyStates[] */
static char *	yygetState YY_ARGS((int));

#define yyassert(condition, msg, arg) \
	if (!(condition)) { \
		printf(m_textmsg(2824, "\nyacc bug: ", "E")); \
		printf(msg, arg); \
		YYABORT; }
#else /* !YYDEBUG */
#define yyassert(condition, msg, arg)
#endif

// Reset all the symbol tables
static void scriptResetTables(void)
{
	VAR_SYMBOL		*psCurr, *psNext;
	TRIGGER_SYMBOL	*psTCurr, *psTNext;
	EVENT_SYMBOL	*psECurr, *psENext;
	FUNC_SYMBOL		*psFCurr, *psFNext;

	/* Reset the global variable symbol table */
	for(psCurr = psGlobalVars; psCurr != NULL; psCurr = psNext)
	{
		psNext = psCurr->psNext;
		FREE(psCurr->pIdent);
		FREE(psCurr);
	}
	psGlobalVars = NULL;

	/* Reset the global array symbol table */
	for(psCurr = psGlobalArrays; psCurr != NULL; psCurr = psNext)
	{
		psNext = psCurr->psNext;
		FREE(psCurr->pIdent);
		FREE(psCurr);
	}
	psGlobalArrays = NULL;

	// Reset the trigger table
	for(psTCurr = psTriggers; psTCurr; psTCurr = psTNext)
	{
		psTNext = psTCurr->psNext;
		if (psTCurr->psDebug)
		{
			FREE(psTCurr->psDebug);
		}
		if (psTCurr->pCode)
		{
			FREE(psTCurr->pCode);
		}
		FREE(psTCurr->pIdent);
		FREE(psTCurr);
	}
	psTriggers = NULL;
	numTriggers = 0;

	// Reset the event table
	for(psECurr = psEvents; psECurr; psECurr = psENext)
	{
		psENext = psECurr->psNext;
		if (psECurr->psDebug)
		{
			FREE(psECurr->psDebug);
		}
		FREE(psECurr->pIdent);
		FREE(psECurr->pCode);
		FREE(psECurr);
	}
	psEvents = NULL;
	numEvents = 0;

	/* Reset the function symbol table */
	for(psFCurr = psFunctions; psFCurr != NULL; psFCurr = psFNext)
	{
		psFNext = psFCurr->psNext;
		FREE_DEBUG(psFCurr);
		FREE(psFCurr->pIdent);
		FREE(psFCurr->pCode);
		FREE(psFCurr);
	}
	psFunctions = NULL;
}

/* Compile a script program */
BOOL scriptCompile(UBYTE *pData, UDWORD fileSize,
				   SCRIPT_CODE **ppsProg, SCR_DEBUGTYPE debugType)
{
	// Tell lex about the input buffer
	scriptSetInputBuffer(pData, fileSize);

	scriptResetTables();
	psFinalProg = NULL;
	if (debugType == SCR_DEBUGINFO)
	{
		genDebugInfo = TRUE;
	}
	else
	{
		genDebugInfo = FALSE;
	}

	if (scr_parse() != 0)
	{
		return FALSE;
	}

	scriptResetTables();

	*ppsProg = psFinalProg;

	return TRUE;
}

/* A simple error reporting routine */
void scr_error(char *pMessage, ...)
{
	int		line;
	char	*text;
	va_list	args;
	STRING	aBuff[1024];

	va_start(args, pMessage);
	vsprintf(aBuff, pMessage, args);
	va_end(args);
	scriptGetErrorData(&line, &text);
#ifdef DEBUG
	ASSERT((FALSE, "script parse error:\n%s at line %d\nToken: %d, Text: '%s'\n",
			  aBuff, line, scr_char, text));
#else
	DBERROR(("script parse error:\n%s at line %d\nToken: %d, Text: '%s'\n",
			  pMessage, line, scr_char, text));
#endif
}

/* Look up a type symbol */
BOOL scriptLookUpType(STRING *pIdent, INTERP_TYPE *pType)
{
	UDWORD	i;

	if (asScrTypeTab)
	{
		for(i=0; asScrTypeTab[i].typeID != 0; i++)
		{
			if (strcmp(asScrTypeTab[i].pIdent, pIdent) == 0)
			{
				*pType = asScrTypeTab[i].typeID;
				return TRUE;
			}
		}
	}

	return FALSE;
}

/* Reset the local variable symbol table at the end of a function */
void scriptClearLocalVariables(void)
{
	VAR_SYMBOL	*psCurr, *psNext;

	for(psCurr = psLocalVars; psCurr != NULL; psCurr = psNext)
	{
		psNext = psCurr->psNext;
		FREE(psCurr->pIdent);
		FREE(psCurr);
	}
}

/* Add a new variable symbol.
 * If localVariableDef is true a local variable symbol is defined,
 * otherwise a global variable symbol is defined.
 */
//BOOL scriptAddVariable(STRING *pIdent, INTERP_TYPE type, STORAGE_TYPE storage, SDWORD elements)
BOOL scriptAddVariable(VAR_DECL *psStorage, VAR_IDENT_DECL *psVarIdent)
{
	VAR_SYMBOL		*psNew;
	SDWORD			i;//, size;

	/* Allocate the memory for the symbol structure */
	psNew = (VAR_SYMBOL *)MALLOC(sizeof(VAR_SYMBOL));
	if (psNew == NULL)
	{
		scr_error("Out of memory");
		return FALSE;
	}
	psNew->pIdent = psVarIdent->pIdent; //(STRING *)MALLOC(strlen(pIdent) + 1);
/*	if (psNew->pIdent == NULL)
	{
		scr_error("Out of memory");
		return FALSE;
	}*/

	/* Intialise the symbol structure */
//	strcpy(psNew->pIdent, pIdent);
	psNew->type = psStorage->type;
	psNew->storage = psStorage->storage;
	psNew->dimensions = psVarIdent->dimensions;
	for(i=0; i<psNew->dimensions; i++)
	{
		psNew->elements[i] = psVarIdent->elements[i];
	}
	if (psNew->dimensions == 0)
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

	return TRUE;
}

/* Look up a variable symbol */
BOOL scriptLookUpVariable(STRING *pIdent, VAR_SYMBOL **ppsSym)
{
	VAR_SYMBOL		*psCurr;

	/* See if the symbol is an object variable */
	if (asScrObjectVarTab && objVarContext != 0)
	{
		for(psCurr = asScrObjectVarTab; psCurr->pIdent != NULL; psCurr++)
		{
			if (interpCheckEquiv(psCurr->objType, objVarContext) &&
				strcmp(psCurr->pIdent, pIdent) == 0)
			{
				*ppsSym = psCurr;
				return TRUE;
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
				*ppsSym = psCurr;
				return TRUE;
			}
		}
	}

	/* See if the symbol is in the local variable list */
	for(psCurr = psLocalVars; psCurr != NULL; psCurr = psCurr->psNext)
	{
		if (strcmp(psCurr->pIdent, pIdent) == 0)
		{
			*ppsSym = psCurr;
			return TRUE;
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
				return TRUE;
			}
		}
		for(psCurr = psGlobalArrays; psCurr != NULL; psCurr = psCurr->psNext)
		{
			if (strcmp(psCurr->pIdent, pIdent) == 0)
			{
				*ppsSym = psCurr;
				return TRUE;
			}
		}
	}

	/* Failed to find the variable */
	*ppsSym = NULL;
	return FALSE;
}

/* Add a new trigger symbol */
BOOL scriptAddTrigger(STRING *pIdent, TRIGGER_DECL *psDecl, UDWORD line)
{
	TRIGGER_SYMBOL		*psTrigger, *psCurr, *psPrev;

	// Allocate the trigger
	psTrigger = MALLOC(sizeof(TRIGGER_SYMBOL));
	if (!psTrigger)
	{
		scr_error("Out of memory");
		return FALSE;
	}
	psTrigger->pIdent = MALLOC(strlen(pIdent) + 1);
	if (!psTrigger->pIdent)
	{
		scr_error("Out of memory");
		return FALSE;
	}
	strcpy(psTrigger->pIdent, pIdent);
	if (psDecl->size > 0)
	{
		psTrigger->pCode = MALLOC(psDecl->size);
		if (!psTrigger->pCode)
		{
			scr_error("Out of memory");
			return FALSE;
		}
		memcpy(psTrigger->pCode, psDecl->pCode, psDecl->size);
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
		psTrigger->psDebug = MALLOC(sizeof(SCRIPT_DEBUG));
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

	return TRUE;
}

/* Lookup a trigger symbol */
BOOL scriptLookUpTrigger(STRING *pIdent, TRIGGER_SYMBOL **ppsTrigger)
{
	TRIGGER_SYMBOL	*psCurr;

	for(psCurr = psTriggers; psCurr; psCurr=psCurr->psNext)
	{
		if (strcmp(pIdent, psCurr->pIdent) == 0)
		{
			*ppsTrigger = psCurr;
			return TRUE;
		}
	}

	return FALSE;
}

/* Lookup a callback trigger symbol */
BOOL scriptLookUpCallback(STRING *pIdent, CALLBACK_SYMBOL **ppsCallback)
{
	CALLBACK_SYMBOL		*psCurr;

	if (!asScrCallbackTab)
	{
		return FALSE;
	}

	for(psCurr = asScrCallbackTab; psCurr->type != 0; psCurr += 1)
	{
		if (strcmp(pIdent, psCurr->pIdent) == 0)
		{
			*ppsCallback = psCurr;
			return TRUE;
		}
	}
	return FALSE;
}

/* Add a new event symbol */
BOOL scriptDeclareEvent(STRING *pIdent, EVENT_SYMBOL **ppsEvent)
{
	EVENT_SYMBOL		*psEvent, *psCurr, *psPrev;

	// Allocate the event
	psEvent = MALLOC(sizeof(EVENT_SYMBOL));
	if (!psEvent)
	{
		scr_error("Out of memory");
		return FALSE;
	}
	psEvent->pIdent = MALLOC(strlen(pIdent) + 1);
	if (!psEvent->pIdent)
	{
		scr_error("Out of memory");
		return FALSE;
	}
	strcpy(psEvent->pIdent, pIdent);
	psEvent->pCode = NULL;
	psEvent->size = 0;
	psEvent->psDebug = NULL;
	psEvent->debugEntries = 0;
	psEvent->index = numEvents++;
	psEvent->psNext = NULL;

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

	return TRUE;
}

// Add the code to a defined event
BOOL scriptDefineEvent(EVENT_SYMBOL *psEvent, CODE_BLOCK *psCode, SDWORD trigger)
{
	// Store the event code
	psEvent->pCode = MALLOC(psCode->size);
	if (!psEvent->pCode)
	{
		scr_error("Out of memory");
		return FALSE;
	}
	memcpy(psEvent->pCode, psCode->pCode, psCode->size);
	psEvent->size = psCode->size;
	psEvent->trigger = trigger;

	// Add debug info
	if (genDebugInfo)
	{
		psEvent->psDebug = MALLOC(sizeof(SCRIPT_DEBUG) * psCode->debugEntries);
		if (!psEvent->psDebug)
		{
			scr_error("Out of memory");
			return FALSE;
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

	return TRUE;
}

/* Lookup an event symbol */
BOOL scriptLookUpEvent(STRING *pIdent, EVENT_SYMBOL **ppsEvent)
{
	EVENT_SYMBOL	*psCurr;

	for(psCurr = psEvents; psCurr; psCurr=psCurr->psNext)
	{
		if (strcmp(pIdent, psCurr->pIdent) == 0)
		{
			*ppsEvent = psCurr;
			return TRUE;
		}
	}

	return FALSE;
}

/* Look up a constant variable symbol */
BOOL scriptLookUpConstant(STRING *pIdent, CONST_SYMBOL **ppsSym)
{
	CONST_SYMBOL	*psCurr;

	/* Scan the Constant list */
	if (asScrConstantTab)
	{
		for(psCurr = asScrConstantTab; psCurr->type != VAL_VOID; psCurr++)
		{
			if (strcmp(psCurr->pIdent, pIdent) == 0)
			{
				*ppsSym = psCurr;
				return TRUE;
			}
		}
	}

	return FALSE;
}

/* Look up a function symbol */
BOOL scriptLookUpFunction(STRING *pIdent, FUNC_SYMBOL **ppsSym)
{
	UDWORD i;
	FUNC_SYMBOL	*psCurr;

	/* See if the function is defined as an instinct function */
	if (asScrInstinctTab)
	{
		for(i = 0; asScrInstinctTab[i].pFunc != NULL; i++)
		{
			if (strcmp(asScrInstinctTab[i].pIdent, pIdent) == 0)
			{
				*ppsSym = asScrInstinctTab + i;
				return TRUE;
			}
		}
	}

	/* See if the function is defined as a script function */
	for(psCurr = psFunctions; psCurr != NULL; psCurr = psCurr->psNext)
	{
		if (strcmp(psCurr->pIdent, pIdent) == 0)
		{
			*ppsSym = psCurr;
			return TRUE;
		}
	}

	/* Failed to find the indentifier */
	*ppsSym = NULL;
	return FALSE;
}

/* Set the type table */
void scriptSetTypeTab(TYPE_SYMBOL *psTypeTab)
{
#ifdef DEBUG
	SDWORD			i;
	INTERP_TYPE		type;

	for(i=0, type=VAL_USERTYPESTART; psTypeTab[i].typeID != 0; i++)
	{
		ASSERT((psTypeTab[i].typeID == type,
			"scriptSetTypeTab: ID's must be >= VAL_USERTYPESTART and sequential"));
		type += 1;
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
		ASSERT((psCallTab[i].type == type,
			"scriptSetCallbackTab: ID's must be >= VAL_CALLBACKSTART and sequential"));
		type += 1;
	}
#endif

	asScrCallbackTab = psCallTab;
}

#ifdef YACC_WINDOWS

/*
 * the following is the scr_parse() function that will be
 * callable by a windows type program. It in turn will
 * load all needed resources, obtain pointers to these
 * resources, and call a statically defined function
 * win_yyparse(), which is the original scr_parse() fn
 * When win_yyparse() is complete, it will return a
 * value to the new scr_parse(), where it will be stored
 * away temporarily, all resources will be freed, and
 * that return value will be given back to the caller
 * scr_parse(), as expected.
 */

static int win_yyparse();			/* prototype */

scr_parse() 
{
	int wReturnValue;
	HANDLE hRes_table;		/* handle of resource after loading */
	short far *old_yydef;		/* the following are used for saving */
	short far *old_yyex;		/* the current pointers */
	short far *old_yyact;
	short far *old_yypact;
	short far *old_yygo;
	short far *old_yypgo;
	short far *old_yyrlen;

	/*
	 * the following code will load the required
	 * resources for a Windows based parser.
	 */

	hRes_table = LoadResource (hInst, 
		FindResource (hInst, "UD_RES_yyYACC", "yyYACCTBL"));
	
	/*
	 * return an error code if any
	 * of the resources did not load
	 */

	if (hRes_table == NULL)
		return (1);
	
	/*
	 * the following code will lock the resources
	 * into fixed memory locations for the parser
	 * (also, save the current pointer values first)
	 */

	old_yydef = yydef;
	old_yyex = yyex;
	old_yyact = yyact;
	old_yypact = yypact;
	old_yygo = yygo;
	old_yypgo = yypgo;
	old_yyrlen = yyrlen;

	yydef = (short far *)LockResource (hRes_table);
	yyex = (short far *)(yydef + Sizeof_yydef);
	yyact = (short far *)(yyex + Sizeof_yyex);
	yypact = (short far *)(yyact + Sizeof_yyact);
	yygo = (short far *)(yypact + Sizeof_yypact);
	yypgo = (short far *)(yygo + Sizeof_yygo);
	yyrlen = (short far *)(yypgo + Sizeof_yypgo);

	/*
	 * call the official scr_parse() function
	 */

	wReturnValue = win_yyparse();

	/*
	 * unlock the resources
	 */

	UnlockResource (hRes_table);

	/*
	 * and now free the resource
	 */

	FreeResource (hRes_table);

	/*
	 * restore previous pointer values
	 */

	yydef = old_yydef;
	yyex = old_yyex;
	yyact = old_yyact;
	yypact = old_yypact;
	yygo = old_yygo;
	yypgo = old_yypgo;
	yyrlen = old_yyrlen;

	return (wReturnValue);
}	/* end scr_parse */

static int win_yyparse() 

#else /* YACC_WINDOWS */

/*
 * we are not compiling a windows resource
 * based parser, so call scr_parse() the old
 * standard way.
 */

int scr_parse() 

#endif /* YACC_WINDOWS */

{
#ifdef YACC_WINDOWS
	register short far	*yyp;	/* for table lookup */
	register short far	*yyq;
#else
	register short		*yyp;	/* for table lookup */
	register short		*yyq;
#endif	/* YACC_WINDOWS */
	register short		yyi;
	register short		*yyps;		/* top of state stack */
	register short		yystate;	/* current state */
	register YYSTYPE	*yypv;		/* top of value stack */
	register int		yyj;
#if YYDEBUG
	yyTraceItems	yyx;			/* trace block */
	short	* yytp;
	int	yyruletype = 0;
#endif
#ifdef YYSTATIC
	static short	yys[YYSSIZE + 1];
	static YYSTYPE	yyv[YYSSIZE + 1];
#if YYDEBUG
	static short	yytypev[YYSSIZE+1];	/* type assignments */
#endif
#else /* ! YYSTATIC */
#ifdef YYALLOC
	YYSTYPE *yyv;
	short	*yys;
#if YYDEBUG
	short	*yytypev;
#endif
	YYSTYPE save_yylval;
	YYSTYPE save_scr_yyval;
	YYSTYPE *save_scr_yypvt;
	int save_yychar, save_scr_yyerrflag, save_scr_yynerrs;
	int retval; 			/* return value holder */
#else
	short		yys[YYSSIZE + 1];
	static YYSTYPE	yyv[YYSSIZE + 1];	/* historically static */
#if YYDEBUG
	short	yytypev[YYSSIZE+1];		/* mirror type table */
#endif
#endif /* ! YYALLOC */
#endif /* ! YYSTATIC */
#ifdef YYDYNAMIC
	char *envp;
#endif

#ifdef YYDYNAMIC
	if ((envp = getenv("YYSTACKSIZE")) != (char *)0) {
		yyssize = atoi(envp);
		if (yyssize <= 0)
			yyssize = YYSSIZE;
	}
	if ((envp = getenv("YYSTACKINC")) != (char *)0)
		yysinc = atoi(envp);
#endif
#ifdef YYALLOC
	yys = (short *) malloc((yyssize + 1) * sizeof(short));
	yyv = (YYSTYPE *) malloc((yyssize + 1) * sizeof(YYSTYPE));
#if YYDEBUG
	yytypev = (short *) malloc((yyssize + 1) * sizeof(short));
#endif
	if (yys == (short *)0 || yyv == (YYSTYPE *)0
#if YYDEBUG
		|| yytypev == (short *) 0
#endif
	) {
		scr_error(m_textmsg(4967, "Not enough space for parser stacks",
				  "E"));
		return 1;
	}
	save_yylval = scr_lval;
	save_scr_yyval = scr_yyval;
	save_scr_yypvt = scr_yypvt;
	save_yychar = scr_char;
	save_scr_yyerrflag = scr_yyerrflag;
	save_scr_yynerrs = scr_yynerrs;
#endif

	scr_yynerrs = 0;
	scr_yyerrflag = 0;
	yyclearin;
	yyps = yys;
	yypv = yyv;
	*yyps = yystate = YYS0;		/* start state */
#if YYDEBUG
	yytp = yytypev;
	yyi = yyj = 0;			/* silence compiler warnings */
#endif

yyStack:
	yyassert((unsigned)yystate < yynstate, m_textmsg(587, "state %d\n", ""), yystate);
#ifdef YYDYNAMIC
	if (++yyps > &yys[yyssize]) {
		int yynewsize;
		int yysindex = yyps - yys;
		int yyvindex = yypv - yyv;
#if YYDEBUG
		int yytindex = yytp - yytypev;
#endif
		if (yysinc == 0) {		/* no increment */
			scr_error(m_textmsg(4968, "Parser stack overflow", "E"));
			YYABORT;
		} else if (yysinc < 0)		/* binary-exponential */
			yynewsize = yyssize * 2;
		else				/* fixed increment */
			yynewsize = yyssize + yysinc;
		if (yynewsize < yyssize) {
			scr_error(m_textmsg(4967,
					  "Not enough space for parser stacks",
					  "E"));
			YYABORT;
		}
		yyssize = yynewsize;
		yys = (short *) realloc(yys, (yyssize + 1) * sizeof(short));
		yyps = yys + yysindex;
		yyv = (YYSTYPE *) realloc(yyv, (yyssize + 1) * sizeof(YYSTYPE));
		yypv = yyv + yyvindex;
#if YYDEBUG
		yytypev = (short *)realloc(yytypev,(yyssize + 1)*sizeof(short));
		yytp = yytypev + yytindex;
#endif
		if (yys == (short *)0 || yyv == (YYSTYPE *)0
#if YYDEBUG
			|| yytypev == (short *) 0
#endif
		) {
			scr_error(m_textmsg(4967, 
					  "Not enough space for parser stacks",
					  "E"));
			YYABORT;
		}
	}
#else
	if (++yyps > &yys[YYSSIZE]) {
		scr_error(m_textmsg(4968, "Parser stack overflow", "E"));
		YYABORT;
	}
#endif /* !YYDYNAMIC */
	*yyps = yystate;	/* stack current state */
	*++yypv = scr_yyval;	/* ... and value */
#if YYDEBUG
	*++yytp = yyruletype;	/* ... and type */

	if (scr_debug)
		YY_TRACE(yyShowState)
#endif

	/*
	 *	Look up next action in action table.
	 */
yyEncore:
#ifdef YYSYNC
	YYREAD;
#endif

#ifdef YACC_WINDOWS
	if (yystate >= Sizeof_yypact) 	/* simple state */
#else /* YACC_WINDOWS */
	if (yystate >= sizeof yypact/sizeof yypact[0]) 	/* simple state */
#endif /* YACC_WINDOWS */
		yyi = yystate - YYDELTA;	/* reduce in any case */
	else {
		if(*(yyp = &yyact[yypact[yystate]]) >= 0) {
			/* Look for a shift on scr_char */
#ifndef YYSYNC
			YYREAD;
#endif
			yyq = yyp;
			yyi = scr_char;
			while (yyi < *yyp++)
				;
			if (yyi == yyp[-1]) {
				yystate = yyneg(YYQYYP);
#if YYDEBUG
				if (scr_debug) {
					yyruletype = yyGetType(scr_char);
					YY_TRACE(yyShowShift)
				}
#endif
				scr_yyval = scr_lval;	/* stack what scr_lex() set */
				yyclearin;		/* clear token */
				if (scr_yyerrflag)
					scr_yyerrflag--;	/* successful shift */
				goto yyStack;
			}
		}

		/*
	 	 *	Fell through - take default action
	 	 */

#ifdef YACC_WINDOWS
		if (yystate >= Sizeof_yydef)
#else /* YACC_WINDOWS */
		if (yystate >= sizeof yydef /sizeof yydef[0])
#endif /* YACC_WINDOWS */
			goto yyError;
		if ((yyi = yydef[yystate]) < 0)	 { /* default == reduce? */
			/* Search exception table */
#ifdef YACC_WINDOWS
			yyassert((unsigned)yyneg(yyi) < Sizeof_yyex,
				m_textmsg(2825, "exception %d\n", "I num"), yystate);
#else /* YACC_WINDOWS */
			yyassert((unsigned)yyneg(yyi) < sizeof yyex/sizeof yyex[0],
				m_textmsg(2825, "exception %d\n", "I num"), yystate);
#endif /* YACC_WINDOWS */
			yyp = &yyex[yyneg(yyi)];
#ifndef YYSYNC
			YYREAD;
#endif
			while((yyi = *yyp) >= 0 && yyi != scr_char)
				yyp += 2;
			yyi = yyp[1];
			yyassert(yyi >= 0,
				 m_textmsg(2826, "Ex table not reduce %d\n", "I num"), yyi);
		}
	}

	yyassert((unsigned)yyi < yynrule, m_textmsg(2827, "reduce %d\n", "I num"), yyi);
	yyj = yyrlen[yyi];
#if YYDEBUG
	if (scr_debug)
		YY_TRACE(yyShowReduce)
	yytp -= yyj;
#endif
	yyps -= yyj;		/* pop stacks */
	scr_yypvt = yypv;		/* save top */
	yypv -= yyj;
	scr_yyval = yypv[1];	/* default action $ = $1 */
#if YYDEBUG
	yyruletype = yyRules[yyrmap[yyi]].type;
#endif

	switch (yyi) {		/* perform semantic action */
		
case YYr1: {	/* script :  header var_list */

					
					
					
				
} break;

case YYr2: {	/* script :  header var_list $1 trigger_list event_list */

					SDWORD			size, debug, i, dimension, arraySize, totalArraySize;
					SDWORD			numArrays;
					UDWORD			base;
					VAR_SYMBOL		*psCurr;
					TRIGGER_SYMBOL	*psTrig;
					EVENT_SYMBOL	*psEvent;
					UDWORD			numVars;

					// Calculate the code size
					size = 0;
					debug = 0;
					for(psTrig = psTriggers; psTrig; psTrig = psTrig->psNext)
					{
						// Add the trigger code size
						size += psTrig->size;
						debug += psTrig->debugEntries;
					}
					for(psEvent = psEvents; psEvent; psEvent = psEvent->psNext)
					{
						// Add the trigger code size
						size += psEvent->size;
						debug += psEvent->debugEntries;
					}

					// Allocate the program
					numVars = psGlobalVars ? psGlobalVars->index+1 : 0;
					numArrays = psGlobalArrays ? psGlobalArrays->index+1 : 0;
					totalArraySize = 0;
					for(psCurr=psGlobalArrays; psCurr; psCurr=psCurr->psNext)
					{
						arraySize = 1;
						for(dimension = 0; dimension < psCurr->dimensions; dimension += 1)
						{
							arraySize *= psCurr->elements[dimension];
						}
						totalArraySize += arraySize;
					}
					ALLOC_PROG(psFinalProg, size, psCurrBlock->pCode,
						numVars, numArrays, numTriggers, numEvents);
					ALLOC_DEBUG(psFinalProg, debug);
					psFinalProg->debugEntries = 0;
					ip = psFinalProg->pCode;

					// Add the trigger code
					i=0;
					for(psTrig = psTriggers; psTrig; psTrig = psTrig->psNext)
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
							psFinalProg->psTriggerData[i].code = TRUE;
						}
						else
						{
							psFinalProg->psTriggerData[i].code = FALSE;
						}
						// Store the data
						psFinalProg->psTriggerData[i].type = (UWORD)psTrig->type;
						psFinalProg->psTriggerData[i].time = psTrig->time;
						i = i+1;
					}
					// Note the end of the final trigger
					psFinalProg->pTriggerTab[i] = (UWORD)(ip - psFinalProg->pCode);

					// Add the event code
					i=0;
					for(psEvent = psEvents; psEvent; psEvent = psEvent->psNext)
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
						i = i+1;
					}
					// Note the end of the final event
					psFinalProg->pEventTab[i] = (UWORD)(ip - psFinalProg->pCode);

					// Allocate debug info for the variables if necessary
					if (genDebugInfo)
					{
						if (numVars > 0)
						{
							psFinalProg->psVarDebug = MALLOC(sizeof(VAR_DEBUG) * numVars);
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
							psFinalProg->psArrayDebug = MALLOC(sizeof(ARRAY_DEBUG) * numArrays);
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

					
					for(psCurr = psGlobalVars; psCurr != NULL; psCurr = psCurr->psNext)
					{
						i = psCurr->index;
						psFinalProg->pGlobals[i] = psCurr->type;

						if (genDebugInfo)
						{
							psFinalProg->psVarDebug[i].pIdent =
										MALLOC(strlen(psCurr->pIdent) + 1);
							if (psFinalProg->psVarDebug[i].pIdent == NULL)
							{
								scr_error("Out of memory");
								YYABORT;
							}
							strcpy(psFinalProg->psVarDebug[i].pIdent, psCurr->pIdent);
							psFinalProg->psVarDebug[i].storage = psCurr->storage;
						}
					}

					
					psFinalProg->arraySize = totalArraySize;
					for(psCurr = psGlobalArrays; psCurr != NULL; psCurr = psCurr->psNext)
					{
						i = psCurr->index;

						psFinalProg->psArrayInfo[i].type = (UBYTE)psCurr->type;
						psFinalProg->psArrayInfo[i].dimensions = (UBYTE)psCurr->dimensions;
						for(dimension=0; dimension < psCurr->dimensions; dimension += 1)
						{
							psFinalProg->psArrayInfo[i].elements[dimension] = (UBYTE)psCurr->elements[dimension];
						}

						if (genDebugInfo)
						{
							psFinalProg->psArrayDebug[i].pIdent =
										MALLOC(strlen(psCurr->pIdent) + 1);
							if (psFinalProg->psArrayDebug[i].pIdent == NULL)
							{
								scr_error("Out of memory");
								YYABORT;
							}
							strcpy(psFinalProg->psArrayDebug[i].pIdent, psCurr->pIdent);
							psFinalProg->psArrayDebug[i].storage = psCurr->storage;
						}
					}
					// calculate the base index of each array
					base = psFinalProg->numGlobals;
					for(i=0; i<numArrays; i++)
					{
						psFinalProg->psArrayInfo[i].base = base;

						arraySize = 1;
						for(dimension = 0; dimension < psFinalProg->psArrayInfo[i].dimensions; dimension += 1)
						{
							arraySize *= psFinalProg->psArrayInfo[i].elements[dimension];
						}

						base += arraySize;
					}
				
} break;

case YYr6: {	/* header_decl :  LINK TYPE ';' */

//						if (!scriptAddVariable("owner", scr_yypvt[-1].tval, ST_PUBLIC, 0))
//						{
							// Out of memory - error already given
//							YYABORT;
//						}
					
} break;

case YYr8: {	/* var_list :  variable_decl ';' */

					FREE_VARDECL(scr_yypvt[-1].vdecl);
				
} break;

case YYr9: {	/* var_list :  var_list variable_decl ';' */

					FREE_VARDECL(scr_yypvt[-1].vdecl);
				
} break;

case YYr10: {	/* variable_decl_head :  STORAGE TYPE */

							ALLOC_VARDECL(psCurrVDecl);
							psCurrVDecl->storage = scr_yypvt[-1].stype;
							psCurrVDecl->type = scr_yypvt[0].tval;

							scr_yyval.vdecl = psCurrVDecl;
						
} break;

case YYr11: {	/* variable_decl_head :  STORAGE TRIGGER */

							ALLOC_VARDECL(psCurrVDecl);
							psCurrVDecl->storage = scr_yypvt[-1].stype;
							psCurrVDecl->type = VAL_TRIGGER;

							scr_yyval.vdecl = psCurrVDecl;
						
} break;

case YYr12: {	/* variable_decl_head :  STORAGE EVENT */

							ALLOC_VARDECL(psCurrVDecl);
							psCurrVDecl->storage = scr_yypvt[-1].stype;
							psCurrVDecl->type = VAL_EVENT;

							scr_yyval.vdecl = psCurrVDecl;
						
} break;

case YYr13: {	/* array_sub_decl :  '[' INTEGER ']' */

						if (scr_yypvt[-1].ival <= 0 || scr_yypvt[-1].ival >= VAR_MAX_ELEMENTS)
						{
							scr_error("Invalid array size %d", scr_yypvt[-1].ival);
							YYABORT;
						}

						ALLOC_VARIDENTDECL(psCurrVIdentDecl, NULL, 1);
						psCurrVIdentDecl->elements[0] = scr_yypvt[-1].ival;

						scr_yyval.videcl = psCurrVIdentDecl;
					
} break;

case YYr14: {	/* array_sub_decl_list :  array_sub_decl */

						scr_yyval.videcl = scr_yypvt[0].videcl;
					
} break;

case YYr15: {	/* array_sub_decl_list :  array_sub_decl_list '[' INTEGER ']' */

						if (scr_yypvt[-3].videcl->dimensions >= VAR_MAX_DIMENSIONS)
						{
							scr_error("Too many dimensions for array");
							YYABORT;
						}
						if (scr_yypvt[-1].ival <= 0 || scr_yypvt[-1].ival >= VAR_MAX_ELEMENTS)
						{
							scr_error("Invalid array size %d", scr_yypvt[-1].ival);
							YYABORT;
						}

						scr_yypvt[-3].videcl->elements[scr_yypvt[-3].videcl->dimensions] = scr_yypvt[-1].ival;
						scr_yypvt[-3].videcl->dimensions += 1;

						scr_yyval.videcl = scr_yypvt[-3].videcl;
					
} break;

case YYr16: {	/* variable_ident :  IDENT */

						ALLOC_VARIDENTDECL(psCurrVIdentDecl, scr_yypvt[0].sval, 0);

						scr_yyval.videcl = psCurrVIdentDecl;
					
} break;

case YYr17: {	/* variable_ident :  IDENT array_sub_decl_list */

						scr_yypvt[0].videcl->pIdent = MALLOC(strlen(scr_yypvt[-1].sval)+1);
						if (scr_yypvt[0].videcl->pIdent == NULL)
						{
							scr_error("Out of memory");
							YYABORT;
						}
						strcpy(scr_yypvt[0].videcl->pIdent, scr_yypvt[-1].sval);

						scr_yyval.videcl = scr_yypvt[0].videcl;
					
} break;

case YYr18: {	/* variable_decl :  variable_decl_head variable_ident */

						if (!scriptAddVariable(scr_yypvt[-1].vdecl, scr_yypvt[0].videcl))
						{
							
							YYABORT;
						}

						FREE_VARIDENTDECL(scr_yypvt[0].videcl);

						
						scr_yyval.vdecl = scr_yypvt[-1].vdecl;
					
} break;

case YYr19: {	/* variable_decl :  variable_decl ',' variable_ident */

						if (!scriptAddVariable(scr_yypvt[-2].vdecl, scr_yypvt[0].videcl))
						{
							
							YYABORT;
						}

						FREE_VARIDENTDECL(scr_yypvt[0].videcl);

						
						scr_yyval.vdecl = scr_yypvt[-2].vdecl;
					
} break;

case YYr23: {	/* trigger_subdecl :  boolexp ',' INTEGER */

						ALLOC_TSUBDECL(psCurrTDecl, TR_CODE, scr_yypvt[-2].cblock->size, scr_yypvt[0].ival);
						ip = psCurrTDecl->pCode;
						PUT_BLOCK(ip, scr_yypvt[-2].cblock);
						FREE_BLOCK(scr_yypvt[-2].cblock);

						scr_yyval.tdecl = psCurrTDecl;
					
} break;

case YYr24: {	/* trigger_subdecl :  WAIT ',' INTEGER */

						ALLOC_TSUBDECL(psCurrTDecl, TR_WAIT, 0, scr_yypvt[0].ival);

						scr_yyval.tdecl = psCurrTDecl;
					
} break;

case YYr25: {	/* trigger_subdecl :  EVERY ',' INTEGER */

						ALLOC_TSUBDECL(psCurrTDecl, TR_EVERY, 0, scr_yypvt[0].ival);

						scr_yyval.tdecl = psCurrTDecl;
					
} break;

case YYr26: {	/* trigger_subdecl :  INITIALISE */

						ALLOC_TSUBDECL(psCurrTDecl, TR_INIT, 0, 0);

						scr_yyval.tdecl = psCurrTDecl;
					
} break;

case YYr27: {	/* trigger_subdecl :  CALLBACK_SYM */

						if (scr_yypvt[0].cbSymbol->numParams != 0)
						{
							scr_error("Expected parameters for callback trigger");
							YYABORT;
						}

						ALLOC_TSUBDECL(psCurrTDecl, scr_yypvt[0].cbSymbol->type, 0, 0);

						scr_yyval.tdecl = psCurrTDecl;
					
} break;

case YYr28: {	/* trigger_subdecl :  CALLBACK_SYM ',' param_list */

						codeRet = scriptCodeCallbackParams(scr_yypvt[-2].cbSymbol, scr_yypvt[0].pblock, &psCurrTDecl);
						CHECK_CODE_ERROR(codeRet);

						scr_yyval.tdecl = psCurrTDecl;
					
} break;

case YYr29: {	/* trigger_decl :  TRIGGER IDENT '(' trigger_subdecl ')' ';' */

						SDWORD	line;
						STRING	*pDummy;

						scriptGetErrorData(&line, &pDummy);
						if (!scriptAddTrigger(scr_yypvt[-4].sval, scr_yypvt[-2].tdecl, (UDWORD)line))
						{
							YYABORT;
						}
						FREE_TSUBDECL(scr_yypvt[-2].tdecl);
					
} break;

case YYr32: {	/* event_subdecl :  EVENT IDENT */

						EVENT_SYMBOL	*psEvent;
						if (!scriptDeclareEvent(scr_yypvt[0].sval, &psEvent))
						{
							YYABORT;
						}

						scr_yyval.eSymbol = psEvent;
					
} break;

case YYr33: {	/* event_subdecl :  EVENT EVENT_SYM */

							scr_yyval.eSymbol = scr_yypvt[0].eSymbol;
						
} break;

case YYr35: {	/* event_decl :  event_subdecl '(' TRIG_SYM ')' '{' statement_list '}' */

						if (!scriptDefineEvent(scr_yypvt[-6].eSymbol, scr_yypvt[-1].cblock, scr_yypvt[-4].tSymbol->index))
						{
							YYABORT;
						}

						FREE_DEBUG(scr_yypvt[-1].cblock);
						FREE_BLOCK(scr_yypvt[-1].cblock);
					
} break;

case YYr36: {	/* event_decl :  event_subdecl '(' trigger_subdecl ')' */

						// Get the line for the implicit trigger declaration
						STRING	*pDummy;
						scriptGetErrorData((SDWORD *)&debugLine, &pDummy);
					
} break;

case YYr37: {	/* event_decl :  event_subdecl '(' trigger_subdecl ')' $36 '{' statement_list '}' */

						// Create a trigger for this event
						if (!scriptAddTrigger("", scr_yypvt[-5].tdecl, debugLine))
						{
							YYABORT;
						}
						FREE_TSUBDECL(scr_yypvt[-5].tdecl);

						if (!scriptDefineEvent(scr_yypvt[-7].eSymbol, scr_yypvt[-1].cblock, numTriggers - 1))
						{
							YYABORT;
						}

						FREE_DEBUG(scr_yypvt[-1].cblock);
						FREE_BLOCK(scr_yypvt[-1].cblock);
					
} break;

case YYr38: {	/* event_decl :  event_subdecl '(' INACTIVE ')' '{' statement_list '}' */

						if (!scriptDefineEvent(scr_yypvt[-6].eSymbol, scr_yypvt[-1].cblock, -1))
						{
							YYABORT;
						}

						FREE_DEBUG(scr_yypvt[-1].cblock);
						FREE_BLOCK(scr_yypvt[-1].cblock);
					
} break;

case YYr39: {	/* statement_list :  */

							// Allocate a dummy code block
							ALLOC_BLOCK(psCurrBlock, 1);
							psCurrBlock->size = 0;

							scr_yyval.cblock = psCurrBlock;
						
} break;

case YYr40: {	/* statement_list :  statement */

							scr_yyval.cblock = scr_yypvt[0].cblock;
						
} break;

case YYr41: {	/* statement_list :  statement_list statement */

							ALLOC_BLOCK(psCurrBlock, scr_yypvt[-1].cblock->size + scr_yypvt[0].cblock->size);
							ALLOC_DEBUG(psCurrBlock, scr_yypvt[-1].cblock->debugEntries +
													 scr_yypvt[0].cblock->debugEntries);
							ip = psCurrBlock->pCode;

							
							PUT_BLOCK(ip, scr_yypvt[-1].cblock);
							PUT_BLOCK(ip, scr_yypvt[0].cblock);
							PUT_DEBUG(psCurrBlock, scr_yypvt[-1].cblock);
							APPEND_DEBUG(psCurrBlock, scr_yypvt[-1].cblock->size / sizeof(UDWORD), scr_yypvt[0].cblock);
							FREE_DEBUG(scr_yypvt[-1].cblock);
							FREE_DEBUG(scr_yypvt[0].cblock);
							FREE_BLOCK(scr_yypvt[-1].cblock);
							FREE_BLOCK(scr_yypvt[0].cblock);

							
							scr_yyval.cblock = psCurrBlock;
						
} break;

case YYr42: {	/* statement :  assignment ';' */

						UDWORD line;
						STRING *pDummy;

						
						if (genDebugInfo)
						{
							ALLOC_DEBUG(scr_yypvt[-1].cblock, 1);
							scr_yypvt[-1].cblock->psDebug[0].offset = 0;
							scriptGetErrorData((SDWORD *)&line, &pDummy);
							scr_yypvt[-1].cblock->psDebug[0].line = line;
						}

						scr_yyval.cblock = scr_yypvt[-1].cblock;
					
} break;

case YYr43: {	/* statement :  func_call ';' */

						UDWORD line;
						STRING *pDummy;

						DBP0(("[compile] func_call\n"));

						
						if (genDebugInfo)
						{
							ALLOC_DEBUG(scr_yypvt[-1].cblock, 1);
							scr_yypvt[-1].cblock->psDebug[0].offset = 0;
							scriptGetErrorData((SDWORD *)&line, &pDummy);
							scr_yypvt[-1].cblock->psDebug[0].line = line;
						}

						scr_yyval.cblock = scr_yypvt[-1].cblock;
					
} break;

case YYr44: {	/* statement :  conditional */

						scr_yyval.cblock = scr_yypvt[0].cblock;
					
} break;

case YYr45: {	/* statement :  loop */

						scr_yyval.cblock = scr_yypvt[0].cblock;
					
} break;

case YYr46: {	/* statement :  EXIT ';' */

						UDWORD line;
						STRING *pDummy;

						
						ALLOC_BLOCK(psCurrBlock, sizeof(OPCODE));
						ALLOC_DEBUG(psCurrBlock, 1);
						ip = psCurrBlock->pCode;

						
						PUT_OPCODE(ip, OP_EXIT);

						
						if (genDebugInfo)
						{
							psCurrBlock->psDebug[0].offset = 0;
							scriptGetErrorData((SDWORD *)&line, &pDummy);
							psCurrBlock->psDebug[0].line = line;
						}

						scr_yyval.cblock = psCurrBlock;
					
} break;

case YYr47: {	/* statement :  PAUSE '(' INTEGER ')' ';' */

						UDWORD line;
						STRING *pDummy;

						// can only have a positive pause
						if (scr_yypvt[-2].ival < 0)
						{
							scr_error("Invalid pause time");
							YYABORT;
						}

						
						ALLOC_BLOCK(psCurrBlock, sizeof(OPCODE));
						ALLOC_DEBUG(psCurrBlock, 1);
						ip = psCurrBlock->pCode;

						
						PUT_PKOPCODE(ip, OP_PAUSE, scr_yypvt[-2].ival);

						
						if (genDebugInfo)
						{
							psCurrBlock->psDebug[0].offset = 0;
							scriptGetErrorData((SDWORD *)&line, &pDummy);
							psCurrBlock->psDebug[0].line = line;
						}

						scr_yyval.cblock = psCurrBlock;
					
} break;

case YYr48: {	/* assignment :  NUM_VAR '=' expression */

							codeRet = scriptCodeAssignment(scr_yypvt[-2].vSymbol, scr_yypvt[0].cblock, &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							
							scr_yyval.cblock = psCurrBlock;
						
} break;

case YYr49: {	/* assignment :  BOOL_VAR '=' boolexp */

							codeRet = scriptCodeAssignment(scr_yypvt[-2].vSymbol, scr_yypvt[0].cblock, &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							
							scr_yyval.cblock = psCurrBlock;
						
} break;

case YYr50: {	/* assignment :  OBJ_VAR '=' objexp */

							if (!interpCheckEquiv(scr_yypvt[-2].vSymbol->type, scr_yypvt[0].cblock->type))
							{
								scr_error("User type mismatch for assignment");
								YYABORT;
							}
							codeRet = scriptCodeAssignment(scr_yypvt[-2].vSymbol, scr_yypvt[0].cblock, &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							
							scr_yyval.cblock = psCurrBlock;
						
} break;

case YYr51: {	/* assignment :  VAR '=' userexp */

							if (!interpCheckEquiv(scr_yypvt[-2].vSymbol->type, scr_yypvt[0].cblock->type))
							{
								scr_error("User type mismatch for assignment");
								YYABORT;
							}
							codeRet = scriptCodeAssignment(scr_yypvt[-2].vSymbol, (CODE_BLOCK *)scr_yypvt[0].cblock, &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							
							scr_yyval.cblock = psCurrBlock;
						
} break;

case YYr52: {	/* assignment :  num_objvar '=' expression */

							codeRet = scriptCodeObjAssignment(scr_yypvt[-2].objVarBlock, scr_yypvt[0].cblock, &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							
							scr_yyval.cblock = psCurrBlock;
						
} break;

case YYr53: {	/* assignment :  bool_objvar '=' boolexp */

							codeRet = scriptCodeObjAssignment(scr_yypvt[-2].objVarBlock, scr_yypvt[0].cblock, &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							
							scr_yyval.cblock = psCurrBlock;
						
} break;

case YYr54: {	/* assignment :  user_objvar '=' userexp */

							if (!interpCheckEquiv(scr_yypvt[-2].objVarBlock->psObjVar->type,scr_yypvt[0].cblock->type))
							{
								scr_error("User type mismatch for assignment");
								YYABORT;
							}
							codeRet = scriptCodeObjAssignment(scr_yypvt[-2].objVarBlock, (CODE_BLOCK *)scr_yypvt[0].cblock, &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							
							scr_yyval.cblock = psCurrBlock;
						
} break;

case YYr55: {	/* assignment :  obj_objvar '=' objexp */

							if (!interpCheckEquiv(scr_yypvt[-2].objVarBlock->psObjVar->type, scr_yypvt[0].cblock->type))
							{
								scr_error("User type mismatch for assignment");
								YYABORT;
							}
							codeRet = scriptCodeObjAssignment(scr_yypvt[-2].objVarBlock, scr_yypvt[0].cblock, &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							
							scr_yyval.cblock = psCurrBlock;
						
} break;

case YYr56: {	/* assignment :  num_array_var '=' expression */

							codeRet = scriptCodeArrayAssignment(scr_yypvt[-2].arrayBlock, scr_yypvt[0].cblock, &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							
							scr_yyval.cblock = psCurrBlock;
						
} break;

case YYr57: {	/* assignment :  bool_array_var '=' boolexp */

							codeRet = scriptCodeArrayAssignment(scr_yypvt[-2].arrayBlock, scr_yypvt[0].cblock, &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							
							scr_yyval.cblock = psCurrBlock;
						
} break;

case YYr58: {	/* assignment :  user_array_var '=' userexp */

							if (!interpCheckEquiv(scr_yypvt[-2].arrayBlock->psArrayVar->type,scr_yypvt[0].cblock->type))
							{
								scr_error("User type mismatch for assignment");
								YYABORT;
							}
							codeRet = scriptCodeArrayAssignment(scr_yypvt[-2].arrayBlock, (CODE_BLOCK *)scr_yypvt[0].cblock, &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							
							scr_yyval.cblock = psCurrBlock;
						
} break;

case YYr59: {	/* assignment :  obj_array_var '=' objexp */

							if (!interpCheckEquiv(scr_yypvt[-2].arrayBlock->psArrayVar->type, scr_yypvt[0].cblock->type))
							{
								scr_error("User type mismatch for assignment");
								YYABORT;
							}
							codeRet = scriptCodeArrayAssignment(scr_yypvt[-2].arrayBlock, scr_yypvt[0].cblock, &psCurrBlock);
							CHECK_CODE_ERROR(codeRet);

							
							scr_yyval.cblock = psCurrBlock;
						
} break;

case YYr60: {	/* func_call :  NUM_FUNC '(' param_list ')' */

						DBP0(("[compile] NUM_FUNC\n"));

						
						codeRet = scriptCodeFunction(scr_yypvt[-3].fSymbol, scr_yypvt[-1].pblock, FALSE, &psCurrBlock);
						CHECK_CODE_ERROR(codeRet);

						
						scr_yyval.cblock = psCurrBlock;
					
} break;

case YYr61: {	/* func_call :  BOOL_FUNC '(' param_list ')' */

						DBP0(("[compile] BOOL_FUNC\n"));

						
						codeRet = scriptCodeFunction(scr_yypvt[-3].fSymbol, scr_yypvt[-1].pblock, FALSE, &psCurrBlock);
						CHECK_CODE_ERROR(codeRet);

						
						scr_yyval.cblock = psCurrBlock;
					
} break;

case YYr62: {	/* func_call :  USER_FUNC '(' param_list ')' */

						DBP0(("[compile] USER_FUNC\n"));

						
						codeRet = scriptCodeFunction(scr_yypvt[-3].fSymbol, scr_yypvt[-1].pblock, FALSE, &psCurrBlock);
						CHECK_CODE_ERROR(codeRet);

						
						scr_yyval.cblock = psCurrBlock;
					
} break;

case YYr63: {	/* func_call :  OBJ_FUNC '(' param_list ')' */

						DBP0(("[compile] OBJ_FUNC\n"));

						
						codeRet = scriptCodeFunction(scr_yypvt[-3].fSymbol, scr_yypvt[-1].pblock, FALSE, &psCurrBlock);
						CHECK_CODE_ERROR(codeRet);

						
						scr_yyval.cblock = psCurrBlock;
					
} break;

case YYr64: {	/* func_call :  FUNC '(' param_list ')' */

						DBP0(("[compile] FUNC\n"));
						
						codeRet = scriptCodeFunction(scr_yypvt[-3].fSymbol, scr_yypvt[-1].pblock, FALSE, &psCurrBlock);
						CHECK_CODE_ERROR(codeRet);

						
						scr_yyval.cblock = psCurrBlock;
					
} break;

case YYr65: {	/* param_list :  */

						
						ALLOC_PBLOCK(psCurrPBlock, sizeof(UDWORD), 1);
						psCurrPBlock->size = 0;
						psCurrPBlock->numParams = 0;

						scr_yyval.pblock = psCurrPBlock;
					
} break;

case YYr66: {	/* param_list :  parameter */

						DBP0(("[compile] parameter\n"));
						scr_yyval.pblock = scr_yypvt[0].pblock;
					
} break;

case YYr67: {	/* param_list :  param_list ',' parameter */

						DBP0(("[compile] param_list , parameter\n"));

						ALLOC_PBLOCK(psCurrPBlock, scr_yypvt[-2].pblock->size + scr_yypvt[0].pblock->size,
												   scr_yypvt[-2].pblock->numParams + scr_yypvt[0].pblock->numParams);
						ip = psCurrPBlock->pCode;

						
						PUT_BLOCK(ip, scr_yypvt[-2].pblock);
						PUT_BLOCK(ip, scr_yypvt[0].pblock);
						
						
						memcpy(psCurrPBlock->aParams, scr_yypvt[-2].pblock->aParams,
								scr_yypvt[-2].pblock->numParams * sizeof(INTERP_TYPE));
						memcpy(psCurrPBlock->aParams + scr_yypvt[-2].pblock->numParams,
								scr_yypvt[0].pblock->aParams,
								scr_yypvt[0].pblock->numParams * sizeof(INTERP_TYPE));

						
						FREE_PBLOCK(scr_yypvt[-2].pblock);
						FREE_PBLOCK(scr_yypvt[0].pblock);

						
						scr_yyval.pblock = psCurrPBlock;
					
} break;

case YYr68: {	/* parameter :  expression */

						
						codeRet = scriptCodeParameter(scr_yypvt[0].cblock, VAL_INT, &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						
						scr_yyval.pblock = psCurrPBlock;
					
} break;

case YYr69: {	/* parameter :  boolexp */

						
						codeRet = scriptCodeParameter(scr_yypvt[0].cblock, VAL_BOOL, &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						
						scr_yyval.pblock = psCurrPBlock;
					
} break;

case YYr70: {	/* parameter :  userexp */

						
						codeRet = scriptCodeParameter((CODE_BLOCK *)scr_yypvt[0].cblock, scr_yypvt[0].cblock->type, &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						
						scr_yyval.pblock = psCurrPBlock;
					
} break;

case YYr71: {	/* parameter :  objexp */

						
						codeRet = scriptCodeParameter(scr_yypvt[0].cblock, scr_yypvt[0].cblock->type, &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						
						scr_yyval.pblock = psCurrPBlock;
					
} break;

case YYr72: {	/* parameter :  var_ref */

						
						scr_yyval.pblock = scr_yypvt[0].pblock;
					
} break;

case YYr73: {	/* var_ref :  REF NUM_VAR */

						codeRet = scriptCodeVarRef(scr_yypvt[0].vSymbol, &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						
						scr_yyval.pblock = psCurrPBlock;
					
} break;

case YYr74: {	/* var_ref :  REF BOOL_VAR */

						codeRet = scriptCodeVarRef(scr_yypvt[0].vSymbol, &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						
						scr_yyval.pblock = psCurrPBlock;
					
} break;

case YYr75: {	/* var_ref :  REF VAR */

						codeRet = scriptCodeVarRef(scr_yypvt[0].vSymbol, &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						
						scr_yyval.pblock = psCurrPBlock;
					
} break;

case YYr76: {	/* var_ref :  REF OBJ_VAR */

						codeRet = scriptCodeVarRef(scr_yypvt[0].vSymbol, &psCurrPBlock);
						CHECK_CODE_ERROR(codeRet);

						
						scr_yyval.pblock = psCurrPBlock;
					
} break;

case YYr77: {	/* conditional :  cond_clause_list */

						codeRet = scriptCodeConditional(scr_yypvt[0].condBlock, &psCurrBlock);
						CHECK_CODE_ERROR(codeRet);

						scr_yyval.cblock = psCurrBlock;
					
} break;

case YYr78: {	/* conditional :  cond_clause_list ELSE terminal_cond */

						ALLOC_CONDBLOCK(psCondBlock,
										scr_yypvt[-2].condBlock->numOffsets + scr_yypvt[0].condBlock->numOffsets,
										scr_yypvt[-2].condBlock->size + scr_yypvt[0].condBlock->size);
						ALLOC_DEBUG(psCondBlock, scr_yypvt[-2].condBlock->debugEntries + scr_yypvt[0].condBlock->debugEntries);
						ip = psCondBlock->pCode;

						
						PUT_BLOCK(ip, scr_yypvt[-2].condBlock);
						PUT_BLOCK(ip, scr_yypvt[0].condBlock);

						
						
						memcpy(psCondBlock->aOffsets, scr_yypvt[-2].condBlock->aOffsets,
							   scr_yypvt[-2].condBlock->numOffsets * sizeof(SDWORD));
						psCondBlock->numOffsets = scr_yypvt[-2].condBlock->numOffsets;

						
						PUT_DEBUG(psCondBlock, scr_yypvt[-2].condBlock);
						APPEND_DEBUG(psCondBlock, scr_yypvt[-2].condBlock->size / sizeof(UDWORD), scr_yypvt[0].condBlock);

						
						FREE_DEBUG(scr_yypvt[-2].condBlock);
						FREE_DEBUG(scr_yypvt[0].condBlock);
						FREE_CONDBLOCK(scr_yypvt[-2].condBlock);
						FREE_CONDBLOCK(scr_yypvt[0].condBlock);

						
						codeRet = scriptCodeConditional(psCondBlock, &psCurrBlock);
						CHECK_CODE_ERROR(codeRet);

						scr_yyval.cblock = psCurrBlock;
					
} break;

case YYr79: {	/* cond_clause_list :  cond_clause */

						scr_yyval.condBlock = scr_yypvt[0].condBlock;
					
} break;

case YYr80: {	/* cond_clause_list :  cond_clause_list ELSE cond_clause */

						ALLOC_CONDBLOCK(psCondBlock,
										scr_yypvt[-2].condBlock->numOffsets + scr_yypvt[0].condBlock->numOffsets,
										scr_yypvt[-2].condBlock->size + scr_yypvt[0].condBlock->size);
						ALLOC_DEBUG(psCondBlock, scr_yypvt[-2].condBlock->debugEntries + scr_yypvt[0].condBlock->debugEntries);
						ip = psCondBlock->pCode;

						
						PUT_BLOCK(ip, scr_yypvt[-2].condBlock);
						PUT_BLOCK(ip, scr_yypvt[0].condBlock);

						
						memcpy(psCondBlock->aOffsets, scr_yypvt[-2].condBlock->aOffsets,
							   scr_yypvt[-2].condBlock->numOffsets * sizeof(SDWORD));
						psCondBlock->aOffsets[scr_yypvt[-2].condBlock->numOffsets] =
							scr_yypvt[0].condBlock->aOffsets[0] + scr_yypvt[-2].condBlock->size / sizeof(UDWORD);
						psCondBlock->numOffsets = scr_yypvt[-2].condBlock->numOffsets + 1;

						
						PUT_DEBUG(psCondBlock, scr_yypvt[-2].condBlock);
						APPEND_DEBUG(psCondBlock, scr_yypvt[-2].condBlock->size / sizeof(UDWORD), scr_yypvt[0].condBlock);

						
						FREE_DEBUG(scr_yypvt[-2].condBlock);
						FREE_DEBUG(scr_yypvt[0].condBlock);
						FREE_CONDBLOCK(scr_yypvt[-2].condBlock);
						FREE_CONDBLOCK(scr_yypvt[0].condBlock);

						scr_yyval.condBlock = psCondBlock;
					
} break;

case YYr81: {	/* terminal_cond :  '{' statement_list '}' */

						
						ALLOC_CONDBLOCK(psCondBlock, 1, scr_yypvt[-1].cblock->size);
						ALLOC_DEBUG(psCondBlock, scr_yypvt[-1].cblock->debugEntries);
						ip = psCondBlock->pCode;

						
						PUT_DEBUG(psCondBlock, scr_yypvt[-1].cblock);

						
						PUT_BLOCK(ip, scr_yypvt[-1].cblock);
						FREE_DEBUG(scr_yypvt[-1].cblock);
						FREE_BLOCK(scr_yypvt[-1].cblock);

						scr_yyval.condBlock = psCondBlock;
					
} break;

case YYr82: {	/* cond_clause :  IF '(' boolexp ')' */

						STRING *pDummy;

						
						
						scriptGetErrorData((SDWORD *)&debugLine, &pDummy);
					
} break;

case YYr83: {	/* cond_clause :  IF '(' boolexp ')' $82 '{' statement_list '}' */

						
						ALLOC_CONDBLOCK(psCondBlock, 1,
										scr_yypvt[-5].cblock->size + scr_yypvt[-1].cblock->size + 
										sizeof(OPCODE)*2);
						ALLOC_DEBUG(psCondBlock, scr_yypvt[-1].cblock->debugEntries + 1);
						ip = psCondBlock->pCode;

						
						PUT_BLOCK(ip, scr_yypvt[-5].cblock);
						FREE_BLOCK(scr_yypvt[-5].cblock);

						
						
						PUT_PKOPCODE(ip, OP_JUMPFALSE, (scr_yypvt[-1].cblock->size / sizeof(UDWORD)) + 2);

						
						if (genDebugInfo)
						{
							psCondBlock->debugEntries = 1;
							psCondBlock->psDebug->line = debugLine;
							psCondBlock->psDebug->offset = 0;
							APPEND_DEBUG(psCondBlock, ip - psCondBlock->pCode, scr_yypvt[-1].cblock);
						}

						
						PUT_BLOCK(ip, scr_yypvt[-1].cblock);
						FREE_DEBUG(scr_yypvt[-1].cblock);
						FREE_BLOCK(scr_yypvt[-1].cblock);

						
						psCondBlock->aOffsets[0] = ip - psCondBlock->pCode;

						
						
						
						
						
						PUT_PKOPCODE(ip, OP_JUMP, 0);

						scr_yyval.condBlock = psCondBlock;
					
} break;

case YYr84: {	/* loop :  WHILE '(' boolexp ')' */

					STRING *pDummy;

					
					
					scriptGetErrorData((SDWORD *)&debugLine, &pDummy);
				
} break;

case YYr85: {	/* loop :  WHILE '(' boolexp ')' $84 '{' statement_list '}' */


					ALLOC_BLOCK(psCurrBlock, scr_yypvt[-5].cblock->size + scr_yypvt[-1].cblock->size + sizeof(OPCODE) * 2);
					ALLOC_DEBUG(psCurrBlock, scr_yypvt[-1].cblock->debugEntries + 1);
					ip = psCurrBlock->pCode;

					
					PUT_BLOCK(ip, scr_yypvt[-5].cblock);
					FREE_BLOCK(scr_yypvt[-5].cblock);

					
					
					PUT_PKOPCODE(ip, OP_JUMPFALSE, (scr_yypvt[-1].cblock->size / sizeof(UDWORD)) + 2);

					
					if (genDebugInfo)
					{
						psCurrBlock->debugEntries = 1;
						psCurrBlock->psDebug->line = debugLine;
						psCurrBlock->psDebug->offset = 0;
						APPEND_DEBUG(psCurrBlock, ip - psCurrBlock->pCode, scr_yypvt[-1].cblock);
					}

					
					PUT_BLOCK(ip, scr_yypvt[-1].cblock);
					FREE_DEBUG(scr_yypvt[-1].cblock);
					FREE_BLOCK(scr_yypvt[-1].cblock);

					
					PUT_PKOPCODE(ip, OP_JUMP, (SWORD)( -(SWORD)(psCurrBlock->size / sizeof(UDWORD)) + 1));

					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr86: {	/* expression :  expression '+' expression */

					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_ADD, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr87: {	/* expression :  expression '-' expression */

					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_SUB, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr88: {	/* expression :  expression '*' expression */

					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_MUL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr89: {	/* expression :  expression '/' expression */

					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_DIV, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr90: {	/* expression :  '-' expression */

					ALLOC_BLOCK(psCurrBlock, scr_yypvt[0].cblock->size + sizeof(OPCODE));
					ip = psCurrBlock->pCode;

					
					PUT_BLOCK(ip, scr_yypvt[0].cblock);

					
					PUT_PKOPCODE(ip, OP_UNARYOP, OP_NEG);

					
					FREE_BLOCK(scr_yypvt[0].cblock);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr91: {	/* expression :  '(' expression ')' */

					
					scr_yyval.cblock = scr_yypvt[-1].cblock;
				
} break;

case YYr92: {	/* expression :  NUM_FUNC '(' param_list ')' */

					
					codeRet = scriptCodeFunction(scr_yypvt[-3].fSymbol, scr_yypvt[-1].pblock, TRUE, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr93: {	/* expression :  NUM_VAR */

					codeRet = scriptCodeVarGet(scr_yypvt[0].vSymbol, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr94: {	/* expression :  NUM_CONSTANT */

					codeRet = scriptCodeConstant(scr_yypvt[0].cSymbol, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr95: {	/* expression :  num_objvar */

					codeRet = scriptCodeObjGet(scr_yypvt[0].objVarBlock, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr96: {	/* expression :  num_array_var */

					codeRet = scriptCodeArrayGet(scr_yypvt[0].arrayBlock, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr97: {	/* expression :  INTEGER */

					ALLOC_BLOCK(psCurrBlock, sizeof(OPCODE) + sizeof(UDWORD));
					ip = psCurrBlock->pCode;

					
					PUT_PKOPCODE(ip, OP_PUSH, VAL_INT);
					PUT_DATA(ip, scr_yypvt[0].ival);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr98: {	/* boolexp :  boolexp _AND boolexp */

					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_AND, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr99: {	/* boolexp :  boolexp _OR boolexp */

					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_OR, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr100: {	/* boolexp :  boolexp BOOLEQUAL boolexp */

					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_EQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr101: {	/* boolexp :  boolexp NOTEQUAL boolexp */

					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_NOTEQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr102: {	/* boolexp :  _NOT boolexp */

					ALLOC_BLOCK(psCurrBlock, scr_yypvt[0].cblock->size + sizeof(OPCODE));
					ip = psCurrBlock->pCode;

					
					PUT_BLOCK(ip, scr_yypvt[0].cblock);

					
					PUT_PKOPCODE(ip, OP_UNARYOP, OP_NOT);

					
					FREE_BLOCK(scr_yypvt[0].cblock);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr103: {	/* boolexp :  '(' boolexp ')' */

					
					scr_yyval.cblock = scr_yypvt[-1].cblock;
				
} break;

case YYr104: {	/* boolexp :  BOOL_FUNC '(' param_list ')' */

					
					codeRet = scriptCodeFunction(scr_yypvt[-3].fSymbol, scr_yypvt[-1].pblock, TRUE, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr105: {	/* boolexp :  BOOL_VAR */

					codeRet = scriptCodeVarGet(scr_yypvt[0].vSymbol, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr106: {	/* boolexp :  BOOL_CONSTANT */

					codeRet = scriptCodeConstant(scr_yypvt[0].cSymbol, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr107: {	/* boolexp :  bool_objvar */

					codeRet = scriptCodeObjGet(scr_yypvt[0].objVarBlock, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr108: {	/* boolexp :  bool_array_var */

					codeRet = scriptCodeArrayGet(scr_yypvt[0].arrayBlock, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr109: {	/* boolexp :  BOOLEAN */

					ALLOC_BLOCK(psCurrBlock, sizeof(OPCODE) + sizeof(UDWORD));
					ip = psCurrBlock->pCode;

					
					PUT_PKOPCODE(ip, OP_PUSH, VAL_BOOL);
					PUT_DATA(ip, scr_yypvt[0].bval);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr110: {	/* boolexp :  expression BOOLEQUAL expression */

					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_EQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr111: {	/* boolexp :  userexp BOOLEQUAL userexp */

					if (!interpCheckEquiv(scr_yypvt[-2].cblock->type,scr_yypvt[0].cblock->type))
					{
						scr_error("Type mismatch for equality");
						YYABORT;
					}
					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_EQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr112: {	/* boolexp :  objexp BOOLEQUAL objexp */

					if (!interpCheckEquiv(scr_yypvt[-2].cblock->type,scr_yypvt[0].cblock->type))
					{
						scr_error("Type mismatch for equality");
						YYABORT;
					}
					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_EQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr113: {	/* boolexp :  expression NOTEQUAL expression */

					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_NOTEQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr114: {	/* boolexp :  userexp NOTEQUAL userexp */

					if (!interpCheckEquiv(scr_yypvt[-2].cblock->type,scr_yypvt[0].cblock->type))
					{
						scr_error("Type mismatch for inequality");
						YYABORT;
					}
					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_NOTEQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr115: {	/* boolexp :  objexp NOTEQUAL objexp */

					if (!interpCheckEquiv(scr_yypvt[-2].cblock->type,scr_yypvt[0].cblock->type))
					{
						scr_error("Type mismatch for inequality");
						YYABORT;
					}
					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_NOTEQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr116: {	/* boolexp :  expression LESSEQUAL expression */

					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_LESSEQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr117: {	/* boolexp :  expression GREATEQUAL expression */

					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_GREATEREQUAL, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr118: {	/* boolexp :  expression GREATER expression */

					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_GREATER, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr119: {	/* boolexp :  expression LESS expression */

					codeRet = scriptCodeBinaryOperator(scr_yypvt[-2].cblock, scr_yypvt[0].cblock, OP_LESS, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr120: {	/* userexp :  VAR */

					codeRet = scriptCodeVarGet(scr_yypvt[0].vSymbol, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr121: {	/* userexp :  USER_CONSTANT */

					codeRet = scriptCodeConstant(scr_yypvt[0].cSymbol, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr122: {	/* userexp :  user_objvar */

					codeRet = scriptCodeObjGet(scr_yypvt[0].objVarBlock, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);
					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr123: {	/* userexp :  user_array_var */

					codeRet = scriptCodeArrayGet(scr_yypvt[0].arrayBlock, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr124: {	/* userexp :  USER_FUNC '(' param_list ')' */

					
					codeRet = scriptCodeFunction(scr_yypvt[-3].fSymbol, scr_yypvt[-1].pblock, TRUE, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr125: {	/* userexp :  TRIG_SYM */

					ALLOC_BLOCK(psCurrBlock, sizeof(OPCODE) + sizeof(UDWORD));
					ip = psCurrBlock->pCode;

					
					PUT_PKOPCODE(ip, OP_PUSH, VAL_TRIGGER);
					PUT_DATA(ip, scr_yypvt[0].tSymbol->index);

					psCurrBlock->type = VAL_TRIGGER;

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr126: {	/* userexp :  INACTIVE */

					ALLOC_BLOCK(psCurrBlock, sizeof(OPCODE) + sizeof(UDWORD));
					ip = psCurrBlock->pCode;

					
					PUT_PKOPCODE(ip, OP_PUSH, VAL_TRIGGER);
					PUT_DATA(ip, -1);

					psCurrBlock->type = VAL_TRIGGER;

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr127: {	/* userexp :  EVENT_SYM */

					ALLOC_BLOCK(psCurrBlock, sizeof(OPCODE) + sizeof(UDWORD));
					ip = psCurrBlock->pCode;

					
					PUT_PKOPCODE(ip, OP_PUSH, VAL_EVENT);
					PUT_DATA(ip, scr_yypvt[0].eSymbol->index);

					psCurrBlock->type = VAL_EVENT;

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr128: {	/* objexp :  OBJ_VAR */

					codeRet = scriptCodeVarGet(scr_yypvt[0].vSymbol, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr129: {	/* objexp :  OBJ_CONSTANT */

					codeRet = scriptCodeConstant(scr_yypvt[0].cSymbol, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr130: {	/* objexp :  OBJ_FUNC '(' param_list ')' */

					
					codeRet = scriptCodeFunction(scr_yypvt[-3].fSymbol, scr_yypvt[-1].pblock, TRUE, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr131: {	/* objexp :  obj_objvar */

					codeRet = scriptCodeObjGet(scr_yypvt[0].objVarBlock, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr132: {	/* objexp :  obj_array_var */

					codeRet = scriptCodeArrayGet(scr_yypvt[0].arrayBlock, &psCurrBlock);
					CHECK_CODE_ERROR(codeRet);

					
					scr_yyval.cblock = psCurrBlock;
				
} break;

case YYr133: {	/* objexp_dot :  objexp '.' */

					// Store the object type for the variable lookup
					objVarContext = scr_yypvt[-1].cblock->type;
				
} break;

case YYr134: {	/* num_objvar :  objexp_dot NUM_OBJVAR */

					codeRet = scriptCodeObjectVariable(scr_yypvt[-1].cblock, scr_yypvt[0].vSymbol, &psObjVarBlock);
					CHECK_CODE_ERROR(codeRet);

					// reset the object type
					objVarContext = 0;

					
					scr_yyval.objVarBlock = psObjVarBlock;
				
} break;

case YYr135: {	/* bool_objvar :  objexp_dot BOOL_OBJVAR */

					codeRet = scriptCodeObjectVariable(scr_yypvt[-1].cblock, scr_yypvt[0].vSymbol, &psObjVarBlock);
					CHECK_CODE_ERROR(codeRet);

					// reset the object type
					objVarContext = 0;

					
					scr_yyval.objVarBlock = psObjVarBlock;
				
} break;

case YYr136: {	/* user_objvar :  objexp_dot USER_OBJVAR */

					codeRet = scriptCodeObjectVariable(scr_yypvt[-1].cblock, scr_yypvt[0].vSymbol, &psObjVarBlock);
					CHECK_CODE_ERROR(codeRet);

					// reset the object type
					objVarContext = 0;

					
					scr_yyval.objVarBlock = psObjVarBlock;
				
} break;

case YYr137: {	/* obj_objvar :  objexp_dot OBJ_OBJVAR */

					codeRet = scriptCodeObjectVariable(scr_yypvt[-1].cblock, scr_yypvt[0].vSymbol, &psObjVarBlock);
					CHECK_CODE_ERROR(codeRet);

					// reset the object type
					objVarContext = 0;

					
					scr_yyval.objVarBlock = psObjVarBlock;
				
} break;

case YYr138: {	/* array_index :  '[' expression ']' */

						ALLOC_ARRAYBLOCK(psCurrArrayBlock, scr_yypvt[-1].cblock->size, NULL);
						ip = psCurrArrayBlock->pCode;

						
						PUT_BLOCK(ip, scr_yypvt[-1].cblock);
						FREE_BLOCK(scr_yypvt[-1].cblock);

						scr_yyval.arrayBlock = psCurrArrayBlock;
					
} break;

case YYr139: {	/* array_index_list :  array_index */

						scr_yyval.arrayBlock = scr_yypvt[0].arrayBlock;
					
} break;

case YYr140: {	/* array_index_list :  array_index_list '[' expression ']' */

						ALLOC_ARRAYBLOCK(psCurrArrayBlock, scr_yypvt[-3].arrayBlock->size + scr_yypvt[-1].cblock->size, NULL);
						ip = psCurrArrayBlock->pCode;

						
						psCurrArrayBlock->dimensions = scr_yypvt[-3].arrayBlock->dimensions + 1;
						PUT_BLOCK(ip, scr_yypvt[-3].arrayBlock);
						PUT_BLOCK(ip, scr_yypvt[-1].cblock);
						FREE_ARRAYBLOCK(scr_yypvt[-3].arrayBlock);
						FREE_ARRAYBLOCK(scr_yypvt[-1].cblock);

						scr_yyval.arrayBlock = psCurrArrayBlock;
					
} break;

case YYr141: {	/* num_array_var :  NUM_ARRAY array_index_list */

						codeRet = scriptCodeArrayVariable(scr_yypvt[0].arrayBlock, scr_yypvt[-1].vSymbol, &psCurrArrayBlock);
						CHECK_CODE_ERROR(codeRet);

						
						scr_yyval.arrayBlock = psCurrArrayBlock;
					
} break;

case YYr142: {	/* bool_array_var :  BOOL_ARRAY array_index_list */

						codeRet = scriptCodeArrayVariable(scr_yypvt[0].arrayBlock, scr_yypvt[-1].vSymbol, &psCurrArrayBlock);
						CHECK_CODE_ERROR(codeRet);

						
						scr_yyval.arrayBlock = psCurrArrayBlock;
					
} break;

case YYr143: {	/* obj_array_var :  OBJ_ARRAY array_index_list */

						codeRet = scriptCodeArrayVariable(scr_yypvt[0].arrayBlock, scr_yypvt[-1].vSymbol, &psCurrArrayBlock);
						CHECK_CODE_ERROR(codeRet);

						
						scr_yyval.arrayBlock = psCurrArrayBlock;
					
} break;

case YYr144: {	/* user_array_var :  VAR_ARRAY array_index_list */

						codeRet = scriptCodeArrayVariable(scr_yypvt[0].arrayBlock, scr_yypvt[-1].vSymbol, &psCurrArrayBlock);
						CHECK_CODE_ERROR(codeRet);

						
						scr_yyval.arrayBlock = psCurrArrayBlock;
					
} break;
	case YYrACCEPT:
		YYACCEPT;
	case YYrERROR:
		goto yyError;
	}

	/*
	 *	Look up next state in goto table.
	 */

	yyp = &yygo[yypgo[yyi]];
	yyq = yyp++;
	yyi = *yyps;
	while (yyi < *yyp++)
		;

	yystate = yyneg(yyi == *--yyp? YYQYYP: *yyq);
#if YYDEBUG
	if (scr_debug)
		YY_TRACE(yyShowGoto)
#endif
	goto yyStack;

yyerrlabel:	;		/* come here from YYERROR	*/
/*
#pragma used yyerrlabel
 */
	scr_yyerrflag = 1;
	if (yyi == YYrERROR) {
		yyps--;
		yypv--;
#if YYDEBUG
		yytp--;
#endif
	}

yyError:
	switch (scr_yyerrflag) {

	case 0:		/* new error */
		scr_yynerrs++;
		yyi = scr_char;
		scr_error(m_textmsg(4969, "Syntax error", "E"));
		if (yyi != scr_char) {
			/* user has changed the current token */
			/* try again */
			scr_yyerrflag++;	/* avoid loops */
			goto yyEncore;
		}

	case 1:		/* partially recovered */
	case 2:
		scr_yyerrflag = 3;	/* need 3 valid shifts to recover */
			
		/*
		 *	Pop states, looking for a
		 *	shift on `error'.
		 */

		for ( ; yyps > yys; yyps--, yypv--
#if YYDEBUG
					, yytp--
#endif
		) {
#ifdef YACC_WINDOWS
			if (*yyps >= Sizeof_yypact)
#else /* YACC_WINDOWS */
			if (*yyps >= sizeof yypact/sizeof yypact[0])
#endif /* YACC_WINDOWS */
				continue;
			yyp = &yyact[yypact[*yyps]];
			yyq = yyp;
			do {
				if (YYERRCODE == *yyp) {
					yyp++;
					yystate = yyneg(YYQYYP);
					goto yyStack;
				}
			} while (*yyp++ > YYTOKEN_BASE);
		
			/* no shift in this state */
#if YYDEBUG
			if (scr_debug && yyps > yys+1)
				YY_TRACE(yyShowErrRecovery)
#endif
			/* pop stacks; try again */
		}
		/* no shift on error - abort */
		break;

	case 3:
		/*
		 *	Erroneous token after
		 *	an error - discard it.
		 */

		if (scr_char == 0)  /* but not EOF */
			break;
#if YYDEBUG
		if (scr_debug)
			YY_TRACE(yyShowErrDiscard)
#endif
		yyclearin;
		goto yyEncore;	/* try again in same state */
	}
	YYABORT;

#ifdef YYALLOC
yyReturn:
	scr_lval = save_yylval;
	scr_yyval = save_scr_yyval;
	scr_yypvt = save_scr_yypvt;
	scr_char = save_yychar;
	scr_yyerrflag = save_scr_yyerrflag;
	scr_yynerrs = save_scr_yynerrs;
	free((char *)yys);
	free((char *)yyv);
#if YYDEBUG
	free((char *)yytypev);
#endif
	return(retval);
#endif
}

		
#if YYDEBUG
/*
 * Return type of token
 */
int
yyGetType(tok)
int tok;
{
	yyNamedType * tp;
	for (tp = &yyTokenTypes[yyntoken-1]; tp > yyTokenTypes; tp--)
		if (tp->token == tok)
			return tp->type;
	return 0;
}
/*
 * Print a token legibly.
 */
char *
yyptok(tok)
int tok;
{
	yyNamedType * tp;
	for (tp = &yyTokenTypes[yyntoken-1]; tp > yyTokenTypes; tp--)
		if (tp->token == tok)
			return tp->name;
	return "";
}

/*
 * Read state 'num' from YYStatesFile
 */
#ifdef YYTRACE

static char *
yygetState(num)
int num;
{
	int	size;
	static FILE *yyStatesFile = (FILE *) 0;
	static char yyReadBuf[YYMAX_READ+1];

	if (yyStatesFile == (FILE *) 0
	 && (yyStatesFile = fopen(YYStatesFile, "r")) == (FILE *) 0)
		return "yyExpandName: cannot open states file";

	if (num < yynstate - 1)
		size = (int)(yyStates[num+1] - yyStates[num]);
	else {
		/* length of last item is length of file - ptr(last-1) */
		if (fseek(yyStatesFile, 0L, 2) < 0)
			goto cannot_seek;
		size = (int) (ftell(yyStatesFile) - yyStates[num]);
	}
	if (size < 0 || size > YYMAX_READ)
		return "yyExpandName: bad read size";
	if (fseek(yyStatesFile, yyStates[num], 0) < 0) {
	cannot_seek:
		return "yyExpandName: cannot seek in states file";
	}

	(void) fread(yyReadBuf, 1, size, yyStatesFile);
	yyReadBuf[size] = '\0';
	return yyReadBuf;
}
#endif /* YYTRACE */
/*
 * Expand encoded string into printable representation
 * Used to decode yyStates and yyRules strings.
 * If the expansion of 's' fits in 'buf', return 1; otherwise, 0.
 */
int
yyExpandName(num, isrule, buf, len)
int num, isrule;
char * buf;
int len;
{
	int	i, n, cnt, type;
	char	* endp, * cp;
	char	*s;

	if (isrule)
		s = yyRules[num].name;
	else
#ifdef YYTRACE
		s = yygetState(num);
#else
		s = "*no states*";
#endif

	for (endp = buf + len - 8; *s; s++) {
		if (buf >= endp) {		/* too large: return 0 */
		full:	(void) strcpy(buf, " ...\n");
			return 0;
		} else if (*s == '%') {		/* nonterminal */
			type = 0;
			cnt = yynvar;
			goto getN;
		} else if (*s == '&') {		/* terminal */
			type = 1;
			cnt = yyntoken;
		getN:
			if (cnt < 100)
				i = 2;
			else if (cnt < 1000)
				i = 3;
			else
				i = 4;
			for (n = 0; i-- > 0; )
				n = (n * 10) + *++s - '0';
			if (type == 0) {
				if (n >= yynvar)
					goto too_big;
				cp = yysvar[n];
			} else if (n >= yyntoken) {
			    too_big:
				cp = "<range err>";
			} else
				cp = yyTokenTypes[n].name;

			if ((i = strlen(cp)) + buf > endp)
				goto full;
			(void) strcpy(buf, cp);
			buf += i;
		} else
			*buf++ = *s;
	}
	*buf = '\0';
	return 1;
}
#ifndef YYTRACE
/*
 * Show current state of scr_parse
 */
void
yyShowState(tp)
yyTraceItems * tp;
{
	short * p;
	YYSTYPE * q;

	printf(
	    m_textmsg(2828, "state %d (%d), char %s (%d)\n", "I num1 num2 char num3"),
	      yysmap[tp->state], tp->state,
	      yyptok(tp->lookahead), tp->lookahead);
}
/*
 * show results of reduction
 */
void
yyShowReduce(tp)
yyTraceItems * tp;
{
	printf("reduce %d (%d), pops %d (%d)\n",
		yyrmap[tp->rule], tp->rule,
		tp->states[tp->nstates - tp->npop],
		yysmap[tp->states[tp->nstates - tp->npop]]);
}
void
yyShowRead(val)
int val;
{
	printf(m_textmsg(2829, "read %s (%d)\n", "I token num"), yyptok(val), val);
}
void
yyShowGoto(tp)
yyTraceItems * tp;
{
	printf(m_textmsg(2830, "goto %d (%d)\n", "I num1 num2"), yysmap[tp->state], tp->state);
}
void
yyShowShift(tp)
yyTraceItems * tp;
{
	printf(m_textmsg(2831, "shift %d (%d)\n", "I num1 num2"), yysmap[tp->state], tp->state);
}
void
yyShowErrRecovery(tp)
yyTraceItems * tp;
{
	short	* top = tp->states + tp->nstates - 1;

	printf(
	m_textmsg(2832, "Error recovery pops state %d (%d), uncovers %d (%d)\n", "I num1 num2 num3 num4"),
		yysmap[*top], *top, yysmap[*(top-1)], *(top-1));
}
void
yyShowErrDiscard(tp)
yyTraceItems * tp;
{
	printf(m_textmsg(2833, "Error recovery discards %s (%d), ", "I token num"),
		yyptok(tp->lookahead), tp->lookahead);
}
#endif	/* ! YYTRACE */
#endif	/* YYDEBUG */
