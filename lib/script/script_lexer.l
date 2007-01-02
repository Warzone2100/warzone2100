%{
/*
 * script.l
 *
 * Script file lexer.
 */
#include <physfs.h>
#include "lib/framework/frame.h"
#include "lib/script/interp.h"
#include "lib/script/parse.h"
#include "lib/script/script.h"

/* Get the Yacc definitions */
#include "lib/script/script_parser.h"

/* Maximum length for any TEXT value */
#ifndef YYLMAX
#define YYLMAX	255
#endif

/* Store for any string values */
static char aText[TEXT_BUFFERS][YYLMAX];
static UDWORD currText=0;

// Note if we are in a comment
static BOOL inComment = FALSE;

/* FLEX include buffer stack */
static YY_BUFFER_STATE include_stack[MAX_SCR_INCLUDE_DEPTH];

/* FLEX define buffer stack */
static YY_BUFFER_STATE macro_stack[MAX_SCR_MACRO_DEPTH];
static SCR_MACRO scr_macro[MAX_SCR_MACROS];
static char *tmpMacroBuffer;

static PHYSFS_file *newInput;

static PHYSFS_sint64 InputLen = 0;

static YYSTYPE dummy;

/* Get the token type for a variable symbol */
SDWORD scriptGetVarToken(VAR_SYMBOL *psVar)
{
	BOOL	object;

	// See if this is an object pointer
	if (!asScrTypeTab || psVar->type < VAL_USERTYPESTART)
	{
		object = FALSE;
	}
	else
	{
		object = asScrTypeTab[psVar->type - VAL_USERTYPESTART].accessType == AT_OBJECT;
	}

	if (psVar->storage == ST_OBJECT)
	{
		/* This is an object member variable */
		if (object)
		{

			RULE("current variable is OBJ_OBJVAR");

			return OBJ_OBJVAR;
		}
		else
		{
			switch (psVar->type)
			{
			case VAL_BOOL:
				return BOOL_OBJVAR;
				break;
			case VAL_INT:
//			case VAL_FLOAT:

				RULE( "current variable is NUM_OBJVAR");

				return NUM_OBJVAR;
				break;
			default:

				RULE( "current variable is USER_OBJVAR");

				return USER_OBJVAR;
				break;
			}
		}
	}
	else if (psVar->dimensions > 0)
	{
		/* This is an array variable */
		if (object)
		{
			return OBJ_ARRAY;
		}
		else
		{
			switch (psVar->type)
			{
			case VAL_BOOL:
				return BOOL_ARRAY;
				break;
			case VAL_FLOAT:
				return FLOAT_ARRAY;
				break;
			case VAL_INT:
				return NUM_ARRAY;
				break;
			default:
				return VAR_ARRAY;
				break;
			}
		}
	}
	else
	{
		/* This is a standard variable */
		if (object)
		{

			RULE( "current variable is OBJ_VAR");

			return OBJ_VAR;
		}
		else
		{

			RULE( "ordinary var");

			switch (psVar->type)
			{
			case VAL_BOOL:
				return BOOL_VAR;
				break;
			case VAL_FLOAT:

				RULE( "VAL_FLOAT");

				return FLOAT_VAR;
				break;
			case VAL_INT:
				return NUM_VAR;
				break;
			case VAL_STRING:
				return STRING_VAR;
				break;
			default:
				return VAR;
				break;
			}
		}
	}
}

/* Get the token type for a constant symbol */
SDWORD scriptGetConstToken(CONST_SYMBOL *psConst)
{
	BOOL	object;

	// See if this is an object constant
	if (!asScrTypeTab || psConst->type < VAL_USERTYPESTART)
	{
		object = FALSE;
	}
	else
	{
		object = asScrTypeTab[psConst->type - VAL_USERTYPESTART].accessType == AT_OBJECT;
	}

	switch (psConst->type)
	{
	case VAL_BOOL:
		return BOOL_CONSTANT;
		break;
//	case VAL_FLOAT:
	case VAL_INT:
		return NUM_CONSTANT;
		break;
	case VAL_STRING:
		return STRING_CONSTANT;
		break;
	default:
		if (object)
		{
			//debug(LOG_SCRIPT, "scriptGetConstToken: OBJ_CONSTANT");
			return OBJ_CONSTANT;
		}
		else
		{
			return USER_CONSTANT;
		}
		break;
	}
}

/* Get the token type for a function symbol */
SDWORD scriptGetFuncToken(FUNC_SYMBOL *psFunc)
{
	BOOL	object;

	// See if this is an object pointer
	if(psFunc->type >= VAL_USERTYPESTART)
	{
		object = asScrTypeTab[psFunc->type - VAL_USERTYPESTART].accessType == AT_OBJECT;

		if (object)
		{
			return OBJ_FUNC;
		}
	}
	switch (psFunc->type)
	{
	case VAL_BOOL:
		return BOOL_FUNC;
		break;
	case VAL_FLOAT:
		return FLOAT_FUNC;
		break;
	case VAL_INT:
		return NUM_FUNC;
		break;
	case VAL_STRING:
		return STRING_FUNC;
		break;
	case VAL_VOID:
		return FUNC;
		break;
	default:
		return USER_FUNC;
		break;
	}
}

/* Get the token type for a custom function symbol */
SDWORD scriptGetCustomFuncToken(EVENT_SYMBOL *psFunc)
{
	BOOL	object;

	// See if this is an object pointer
	object = asScrTypeTab[psFunc->retType - VAL_USERTYPESTART].accessType == AT_OBJECT;

	if (object)
	{
		return OBJ_FUNC_CUST;
	}
	else
	{
		switch (psFunc->retType)
		{
		case VAL_BOOL:
			return BOOL_FUNC_CUST;
			break;
		case VAL_FLOAT:

			RULE( "current function is FLOAT_FUNC_CUST");

			return FLOAT_FUNC_CUST;
			break;
		case VAL_INT:

			RULE( "current function is NUM_FUNC_CUST");

			return NUM_FUNC_CUST;
			break;
		case VAL_STRING:
			return STRING_FUNC_CUST;
			break;
		case VAL_VOID:
			return VOID_FUNC_CUST;
			break;
		default:
			return USER_FUNC_CUST;
			break;
		}
	}
}

/* Look up defined macro and return new define buffer */
BOOL scriptLoopUpMacro(const char *pMacro, char **ppMacroBody)
{
	UDWORD i;
	for(i=0;i<scr_num_macros;i++)
	{
		if(strcmp(pMacro, &(scr_macro[i].scr_define_macro[0]) ) == 0)
		{
			*ppMacroBody = (char *)malloc(MAXSTRLEN);
			strcpy( *ppMacroBody, &(scr_macro[i].scr_define_body[0]) );		//copy macro body into buffer so we can process it
			//*pMacroBody = &(scr_macro[i].scr_define_body[0]);	//return macro body
			return TRUE;
		}
	}

	return FALSE;
}

/* Return to the previous macto buffer */
void popMacroStack(void)
{
	ASSERT(scr_macro_stack_ptr <= MAX_SCR_MACRO_DEPTH,
		"FLEX: macro stack pointer out of bounds: %d", scr_macro_stack_ptr);

	/* Free current macro buffer */
	free(pScrMacroBuffer[scr_macro_stack_ptr]);
	pScrMacroBuffer[scr_macro_stack_ptr] = NULL;

	/* Delete flex buffer */
	yy_delete_buffer( YY_CURRENT_BUFFER );

	/* Switch one level lower in the macro stack */
	yy_switch_to_buffer( macro_stack[scr_macro_stack_ptr - 1] );

	scr_macro_stack_ptr--;
}

/* Store macro name */
void storeMacroName(const char *pMacroName)
{
	ASSERT(scr_num_macros >= 0 && scr_num_macros < MAX_SCR_MACROS,
		"FLEX: macro count out of bounds: %d", scr_num_macros);

	/* Make sure this name isn't already reserved */
	if((scriptLookUpType(pMacroName, &dummy.tval)) ||
	(scriptLookUpVariable(pMacroName, &dummy.vSymbol)) ||
	(scriptLookUpConstant(pMacroName, &dummy.cSymbol)) ||
	(scriptLookUpFunction(pMacroName, &dummy.fSymbol)) ||
	(scriptLookUpCustomFunction(pMacroName, &dummy.eSymbol)) ||
	(scriptLookUpTrigger(pMacroName, &dummy.tSymbol)) ||
	(scriptLookUpEvent(pMacroName, &dummy.eSymbol)) ||
	(scriptLookUpCallback(pMacroName, &dummy.cbSymbol)))
	{
		scr_error("FLEX: macro name '%s' is already reserved", pMacroName);
	}

	/* Store define macro */
	strcpy( &(scr_macro[scr_num_macros].scr_define_macro[0]), pMacroName );
}

/* Store macro body */
void storeMacroBody(const char *pMacroBody)
{
	ASSERT(scr_num_macros >= 0 && scr_num_macros < MAX_SCR_MACROS,
		"FLEX: macro count out of bounds: %d", scr_num_macros);

	/* Store macro body */
	strcpy( &(scr_macro[scr_num_macros].scr_define_body[0]), pMacroBody);

	scr_num_macros++;
}

/* include directive encountered - push current buffer, set up new one */
void pushInclude(const char *pIncludePath)
{
	if ( scr_include_stack_ptr >= MAX_SCR_INCLUDE_DEPTH ){
		scr_error("FLEX: Includes nested too deeply" );
	}

	if(!PHYSFS_exists(pIncludePath)){
		scr_error("FLEX: '%s' include doesn't exist", pIncludePath );
	}

	/* Open include */
	newInput = PHYSFS_openRead(pIncludePath);

	if(!newInput){
		scr_error("FLEX: Couldn't open include: '%s'\n%s",
			pIncludePath, PHYSFS_getLastError() );
	}

	/* Push current flex buffer */
	include_stack[scr_include_stack_ptr] = YY_CURRENT_BUFFER;
	scr_include_stack_ptr++;

	/* Get file length */
	InputLen = PHYSFS_fileLength(newInput);

	ASSERT(scr_include_stack_ptr > 0 && scr_include_stack_ptr < MAX_SCR_INCLUDE_DEPTH,
		"FLEX: scr_include_stack_ptr out of bounds: %d", scr_include_stack_ptr);

	/* Allocate enough space for the new include */
	pScrInputBuffer[scr_include_stack_ptr] = (char *)malloc(InputLen);

	/* Read include */
	if (PHYSFS_read(newInput, pScrInputBuffer[scr_include_stack_ptr], 1, InputLen ) != InputLen ){
		scr_error( "FLEX: Couldn't read include: '%s'", pIncludePath);
	}

	/* Close include */
	if(newInput){
		(void)PHYSFS_close(newInput);
	}
}



%}

%option prefix="scr_"
%option nounput
%option yylineno

%x COMMENT
%x SLCOMMENT
%x QUOTE
%x _MACRO
%x _INCL
%x _INCL_END_QUOTE
%x _DEF_MACRO
%x _DEF_BODY_START
%x _DEF_BODY

_IDENTIFIER[a-zA-Z_][0-9_a-zA-Z_]*

%%

	/* Match keywords */
wait					return WAIT;
every					return EVERY;
trigger					return TRIGGER;
event					return EVENT;
inactive				return INACTIVE;
init					return INITIALISE;
link					return LINK;
ref						return REF;
return					return	RET;
function				return	FUNCTION;
"("int")"				{ RULE("(int)"); return TO_INT_CAST; }
"("float")"				{ RULE("(float)"); return TO_FLOAT_CAST; }
public					{ scr_lval.stype = ST_PUBLIC; return STORAGE; }
private					{ scr_lval.stype = ST_PRIVATE; return STORAGE; }
local					{ scr_lval.stype = ST_LOCAL; return STORAGE; }
while					return WHILE;
if						return IF;
else					return ELSE;
exit					return EXIT;
pause					return PAUSE;

	/* Match type keywords */
void					{ scr_lval.tval = VAL_VOID; return _VOID; }
VOID					{ scr_lval.tval = VAL_VOID; return _VOID; }
string					{ scr_lval.tval = VAL_STRING; return TYPE; }
STRING					{ scr_lval.tval = VAL_STRING; return TYPE; }
bool					{ scr_lval.tval = VAL_BOOL; return TYPE; }
BOOL					{ scr_lval.tval = VAL_BOOL; return TYPE; }
int						{ scr_lval.tval = VAL_INT; return TYPE; }
INT						{ scr_lval.tval = VAL_INT; return TYPE; }
float					{ scr_lval.tval = VAL_FLOAT; return TYPE; }
FLOAT					{ scr_lval.tval = VAL_FLOAT; return TYPE; }

	/* Match boolean values */
TRUE					{ scr_lval.bval = TRUE; return BOOLEAN_T; }
true					{ scr_lval.bval = TRUE; return BOOLEAN_T; }
FALSE					{ scr_lval.bval = FALSE; return BOOLEAN_T; }
false					{ scr_lval.bval = FALSE; return BOOLEAN_T; }

	/* Match increment/decrement operators */
"++"					{RULE("++"); return _INC; }
"--"					{RULE("++"); return _DEC; }

	/* Match boolean operators */
"=="					return BOOLEQUAL;
"!="					return NOTEQUAL;
">="					return GREATEQUAL;
"<="					return LESSEQUAL;
">"						return GREATER;
"<"						return LESS;
and						return _AND;
AND						return _AND;
"&&"					return _AND;
or						return _OR;
OR						return _OR;
"||"					return _OR;
not						return _NOT;
NOT						return _NOT;
"!"						return _NOT;

	/* Start of a #define */
"#define"[ \t]+				{ BEGIN (_DEF_MACRO); }

	/* Start of the #define macro name */
<_DEF_MACRO>{_IDENTIFIER}	{
								RULE("FLEX: matched define macro: '#define %s'", scr_text);

								/* Store macro name */
								storeMacroName(scr_text);

								BEGIN (_DEF_BODY_START);
							}

<_DEF_BODY_START>[ \t]+		{ BEGIN (_DEF_BODY); }	/* Eat space between macro and body */

<_DEF_BODY>[^\r\n]*			{	/* Anything until the end of the line is macro body */

								RULE("FLEX: matched define body: '#define %s %s' (%d)",
									scr_macro[scr_num_macros].scr_define_macro, scr_text, scr_num_macros + 1);

								/* Store macro body */
								storeMacroBody(scr_text);

								BEGIN (INITIAL);
							}

	/* Handle Includes */
"#include"[ \t]+\"			{ BEGIN (_INCL); }	/* Match '#include' and skip whitespace until quotation */

<_INCL>[^ \t\n\"]+   		{ 	/* got the include file name */
								RULE("FLEX: Opening include '%s' (scr_include_stack_ptr: %d)", scr_text, scr_include_stack_ptr);

								/* Set up new input buffer, push current one to stack */
								pushInclude(scr_text);

								/* Match the ending quote */
						        BEGIN(_INCL_END_QUOTE);
							}

<_INCL_END_QUOTE>\"			{
								ASSERT(scr_include_stack_ptr < MAX_SCR_INCLUDE_DEPTH,
									"FLEX:  scr_include_stack_ptr out of bounds: %d", scr_include_stack_ptr);

								ASSERT(pScrInputBuffer[scr_include_stack_ptr] != NULL, "FLEX: failed to load include");

								yy_switch_to_buffer( yy_scan_bytes(pScrInputBuffer[scr_include_stack_ptr], InputLen) );

								BEGIN (INITIAL);
							}

<<EOF>> 						{
								RULE("FLEX: EOF: scr_include_stack_ptr: %d; scr_macro_stack_ptr: %d",
									scr_include_stack_ptr, scr_macro_stack_ptr);

								ASSERT(pScrInputBuffer[scr_include_stack_ptr] != NULL, "FLEX: can't unload, buffer is NULL");

								if ( scr_include_stack_ptr == 0 )
								{
									/* We finished scanning top level script or macro */
									if(scr_macro_stack_ptr > 0)
									{
										RULE("FLEX: Finished parsing macro %d", scr_macro_stack_ptr);

										popMacroStack();
									}
									else	//both stacks are empty
									{
										RULE("FLEX: Finished parsing all input");
										yyterminate();
									}
								}
								else
								{
									/* First finish scanning all macros */
									if(scr_macro_stack_ptr > 0)
									{
										RULE("FLEX: Finished parsing macro %d in include %d",
											scr_macro_stack_ptr, scr_include_stack_ptr);

										popMacroStack();
									}
									else	//no macros, return to previous include
									{
										RULE("FLEX: returning from parsing include %d", scr_include_stack_ptr);

										/* Free include buffer */
										free(pScrInputBuffer[scr_include_stack_ptr]);
										pScrInputBuffer[scr_include_stack_ptr] = NULL;

										/* Delete flex buffer */
										yy_delete_buffer( YY_CURRENT_BUFFER );

										/* Pop previous script file */
										yy_switch_to_buffer( include_stack[scr_include_stack_ptr - 1] );

										scr_include_stack_ptr--;
									}
								}
					        }


	/* Match floating point numbers */
-?[0-9]*"."[0-9]+			{
								RULE( "float matched '%s'", scr_text);

								scr_lval.fval = (float)atof(scr_text);

								return FLOAT_T;
							}

	/* Match integer numbers */
-?[0-9]+					{ scr_lval.ival = atol(scr_text); return INTEGER; }

	/* Match identifiers */
{_IDENTIFIER}				{
								RULE( "looking up '%s'", scr_text);

								/* See if this identifier has been defined as a type */
								if (scriptLookUpType(scr_text, &scr_lval.tval))	//STRUCTURESTAT etc
								{

									RULE( "'%s' is a user type", scr_text);

									return TYPE;
								}
								/* See if this identifier has been defined as a variable */
								else if (scriptLookUpVariable(scr_text, &scr_lval.vSymbol))
								{

									RULE( "'%s' is a var", scr_text);

									return scriptGetVarToken(scr_lval.vSymbol);
								}
								/* See if this identifier has been defined as a constant */
								else if (scriptLookUpConstant(scr_text, &scr_lval.cSymbol))
								{

									RULE( "'%s' is a constant", scr_text);

									return scriptGetConstToken(scr_lval.cSymbol);
								}
								/* See if this identifier has been defined as a function */
								else if (scriptLookUpFunction(scr_text, &scr_lval.fSymbol))
								{

									RULE( "'%s' is a function", scr_text);

									return scriptGetFuncToken(scr_lval.fSymbol);
								}

								/* See if this identifier has been defined as a custom function */
								else if (scriptLookUpCustomFunction(scr_text, &scr_lval.eSymbol))
								{

									RULE( "'%s' is a cust func", scr_text);

									return scriptGetCustomFuncToken(scr_lval.eSymbol);
								}

								else if (scriptLookUpTrigger(scr_text, &scr_lval.tSymbol))
								{

									RULE( "'%s' is a trigger", scr_text);

									return TRIG_SYM;
								}
								else if (scriptLookUpEvent(scr_text, &scr_lval.eSymbol))
								{

									RULE( "'%s' is an event", scr_text);

									return EVENT_SYM;
								}
								else if (scriptLookUpCallback(scr_text, &scr_lval.cbSymbol))
								{

									RULE( "'%s' is a callback", scr_text);

									return CALLBACK_SYM;
								}
								else if(scriptLoopUpMacro(scr_text, &tmpMacroBuffer))
								{
									RULE( "'%s' is a macro", scr_text);

									ASSERT(tmpMacroBuffer != NULL,
										"FLEX: failed to allocate memory for macro buffer %d", scr_macro_stack_ptr);

									ASSERT(scr_macro_stack_ptr >= 0 && scr_macro_stack_ptr < MAX_SCR_MACRO_DEPTH,
										"FLEX: flex macro buffer pointer out of bounds: %d", scr_macro_stack_ptr );

									/* Push current buffer we are processing;
									 * can be either macro buffer, include buffer or top level script
									 */
									macro_stack[scr_macro_stack_ptr] = YY_CURRENT_BUFFER;
									scr_macro_stack_ptr++;

									pScrMacroBuffer[scr_macro_stack_ptr] = tmpMacroBuffer;	//assign to the new input buffer

									/* Feed flex with the new macro buffer */
									yy_switch_to_buffer( yy_scan_string( pScrMacroBuffer[scr_macro_stack_ptr] ) );
								}
								else
								{
									RULE( "'%s' is an ident", scr_text);

									strcpy(aText[currText], scr_text);
									scr_lval.sval = aText[currText];
									currText = (currText + 1) % TEXT_BUFFERS;
									return IDENT;
								}
							}
	/* Strip macros */
"#"							{ BEGIN (_MACRO); }
<_MACRO>\n					{ BEGIN (INITIAL); }
<_MACRO>. |
<_MACRO>\n					;

	/* Match quoted text */
\"							{ BEGIN (QUOTE); }
<QUOTE>\"					{ BEGIN (INITIAL); }
<QUOTE>[^\"\n]*				{
								strcpy(aText[currText], scr_text);
								scr_lval.sval = aText[currText];
								currText = (currText + 1) % TEXT_BUFFERS;
								//debug(LOG_SCRIPT, "%s is QTEXT", scr_text);
								return QTEXT;
							}

	/* Skip white space */
[ \t\n\x0d\x0a]				;

	/* Strip comments */
"/*"						{ inComment=TRUE; BEGIN (COMMENT); }
<COMMENT>"*/" |
<COMMENT>"*/"\n				{ inComment=FALSE; BEGIN (INITIAL); }
<COMMENT>. |
<COMMENT>\n					;

	/* Strip single line comments */
"//"						{ BEGIN (SLCOMMENT); }
<SLCOMMENT>\n				{ BEGIN (INITIAL); }
<SLCOMMENT>[^\n]*			;

	/* Match anything that's been missed and pass it as a char */
.							return scr_text[0];

%%

/* Set the current input buffer for the lexer */
void scriptSetInputBuffer(const char *pBuffer, UDWORD size)
{
	/* Reset the lexer in case it's been used before */
	scr__flush_buffer( YY_CURRENT_BUFFER );

	//yy_scan_string(pBuffer);
	YY_CURRENT_BUFFER = yy_scan_bytes(pBuffer, size);
}

void scriptGetErrorData(int *pLine, char **ppText)
{
	*pLine = scr_lineno;
	*ppText = scr_text;
}

int scr_wrap(void)
{
	if (inComment)
	{
		debug( LOG_ERROR, "Warning: reached end of file in a comment" );
		abort();
	}
	return 1;
}
