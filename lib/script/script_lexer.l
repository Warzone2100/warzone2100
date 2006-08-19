%{
/*
 * script.l
 *
 * Script file lexer.
 */

#include "lib/framework/frame.h"
#include "interp.h"
#include "parse.h"
#include "script.h"

/* Get the Yacc definitions */
#include "script_parser.h"

/* Maximum length for any TEXT value */
#ifndef YYLMAX
#define YYLMAX	255
#endif

/* Store for any string values */
static STRING aText[TEXT_BUFFERS][YYLMAX];
static UDWORD currText=0;

// Note if we are in a comment
static BOOL inComment = FALSE;

/* Pointer to the input buffer */
static char *pInputBuffer = NULL;
static char *pEndBuffer = NULL;

#define YY_INPUT(buf, result, max_size) \
      if (pInputBuffer != pEndBuffer) { \
        buf[0] = *(pInputBuffer++); result = 1; \
      } else { \
        buf[0] = EOF; result = YY_NULL; \
      }

#undef scr_getc
#define scr_getc() (pInputBuffer != pEndBuffer ? *(pInputBuffer++) : EOF)

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
				return NUM_OBJVAR;
				break;
			default:
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
			case VAL_INT:
//			case VAL_FLOAT:
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
			return OBJ_VAR;
		}
		else
		{
			switch (psVar->type)
			{
			case VAL_BOOL:
				return BOOL_VAR;
				break;
			case VAL_INT:
//			case VAL_FLOAT:
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
	case VAL_INT:
//	case VAL_FLOAT:
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
	object = asScrTypeTab[psFunc->type - VAL_USERTYPESTART].accessType == AT_OBJECT;

	if (object)
	{
		return OBJ_FUNC;
	}
	else
	{
		switch (psFunc->type)
		{
		case VAL_BOOL:
			return BOOL_FUNC;
			break;
		case VAL_INT:
//		case VAL_FLOAT:
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
		case VAL_INT:
//		case VAL_FLOAT:
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



%}

%option prefix="scr_"
%option nounput
%option yylineno

%x COMMENT
%x SLCOMMENT
%x QUOTE

%%
	/* Match to key words */
	/*begin					return START;*/
	/*end						return END;*/
wait					return WAIT;
every					return EVERY;
trigger					return TRIGGER;
event					return EVENT;
inactive				return INACTIVE;
init					return INITIALISE;
link					return LINK;
ref					return REF;
return					return	RET;
function					return	FUNCTION;

	/* function				return FUNCTION;*/
	/* cond					return COND; */


public					{ scr_lval.stype = ST_PUBLIC; return STORAGE; }
private					{ scr_lval.stype = ST_PRIVATE; return STORAGE; }
local					{ scr_lval.stype = ST_LOCAL; return STORAGE; }
while					return WHILE;
if						return IF;
else					return ELSE;
exit					return EXIT;
pause					return PAUSE;

	/* Match to type key words */
void					{ scr_lval.tval = VAL_VOID; return TYPE; }
VOID					{ scr_lval.tval = VAL_VOID; return TYPE; }
string					{ scr_lval.tval = VAL_STRING; return TYPE; }
STRING					{ scr_lval.tval = VAL_STRING; return TYPE; }
bool					{ scr_lval.tval = VAL_BOOL; return TYPE; }
BOOL					{ scr_lval.tval = VAL_BOOL; return TYPE; }
int						{ scr_lval.tval = VAL_INT; return TYPE; }
INT						{ scr_lval.tval = VAL_INT; return TYPE; }
	/*float					{ ais_lval.tval = VAL_FLOAT; return TYPE; }*/
	/* string type isn't implemented yet */
	/* string					{ ais_lval.tval = VAL_STRING; return TYPE; } */
	/* object					{ scr_lval.tval = VAL_OBJECT; return TYPE; } */

	/* Match boolean values */
TRUE					{ scr_lval.bval = TRUE; return BOOLEAN_T; }
true					{ scr_lval.bval = TRUE; return BOOLEAN_T; }
FALSE					{ scr_lval.bval = FALSE; return BOOLEAN_T; }
false					{ scr_lval.bval = FALSE; return BOOLEAN_T; }

	/* Match boolean operators */
"=="					return BOOLEQUAL;
"!="					return NOTEQUAL;
">="					return GREATEQUAL;
"<="					return LESSEQUAL;
">"						return GREATER;
"<"						return LESS;
and						return _AND;
AND						return _AND;
or						return _OR;
OR						return _OR;
not						return _NOT;
NOT						return _NOT;

	/* Match floating point numbers */
	/*-?[0-9]*"."[0-9]+			{ scr_lval.fval = (float)atof(scr_text); return FLOAT; }*/

	/* Match integer numbers */
-?[0-9]+					{ scr_lval.ival = atol(scr_text); return INTEGER; }

	/* Match identifiers */
[a-zA-Z_][0-9_a-zA-Z_]*		{
						debug(LOG_SCRIPT, "looking up '%s'", scr_text);
								/* See if this identifier has been defined as a type */
								if (scriptLookUpType(scr_text, &scr_lval.tval))
								{
									debug(LOG_SCRIPT, "'%s' is a type", scr_text);
									return TYPE;
								}
								/* See if this identifier has been defined as a variable */
								else if (scriptLookUpVariable(scr_text, &scr_lval.vSymbol))
								{
									debug(LOG_SCRIPT, "'%s' is a var", scr_text);
									return scriptGetVarToken(scr_lval.vSymbol);
								}
								/* See if this identifier has been defined as a constant */
								else if (scriptLookUpConstant(scr_text, &scr_lval.cSymbol))
								{
									debug(LOG_SCRIPT, "'%s' is a constant", scr_text);
									return scriptGetConstToken(scr_lval.cSymbol);
								}
								/* See if this identifier has been defined as a function */
								else if (scriptLookUpFunction(scr_text, &scr_lval.fSymbol))
								{
									debug(LOG_SCRIPT, "'%s' is a function", scr_text);
									return scriptGetFuncToken(scr_lval.fSymbol);
								}

								/* See if this identifier has been defined as a custom function */
								else if (scriptLookUpCustomFunction(scr_text, &scr_lval.eSymbol))
								{
									debug(LOG_SCRIPT, "'%s' is a cust func", scr_text);
									return scriptGetCustomFuncToken(scr_lval.eSymbol);
								}

								else if (scriptLookUpTrigger(scr_text, &scr_lval.tSymbol))
								{
									debug(LOG_SCRIPT, "'%s' is a trigger", scr_text);
									return TRIG_SYM;
								}
								else if (scriptLookUpEvent(scr_text, &scr_lval.eSymbol))
								{
									debug(LOG_SCRIPT, "'%s' is an event", scr_text);
								
									return EVENT_SYM;
								}
								else if (scriptLookUpCallback(scr_text, &scr_lval.cbSymbol))
								{
									debug(LOG_SCRIPT, "'%s' is a callback", scr_text);
									return CALLBACK_SYM;
								}
								else
								{
									debug(LOG_SCRIPT, "'%s' is an ident", scr_text);
								
									strcpy(aText[currText], scr_text);
									scr_lval.sval = aText[currText];
									currText = (currText + 1) % TEXT_BUFFERS;
									return IDENT;
								}
							}

	/* Match quoted text */
\"							{ BEGIN QUOTE; }
<QUOTE>\"					{ BEGIN 0; }
<QUOTE>[^\"\n]*				{
								strcpy(aText[currText], scr_text);
								scr_lval.sval = aText[currText];
								currText = (currText + 1) % TEXT_BUFFERS;
								debug(LOG_SCRIPT, "%s is QTEXT", scr_text);
								return QTEXT;
							}

	/* Skip white space */
[ \t\n\x0d\x0a]						;

	/* Strip comments */
"/*"						{ inComment=TRUE; BEGIN COMMENT; }
<COMMENT>"*/" |
<COMMENT>"*/"\n				{ inComment=FALSE; BEGIN 0; }
<COMMENT>. |
<COMMENT>\n					;

	/* Strip single line comments */
"//"						{ BEGIN SLCOMMENT; }
<SLCOMMENT>\n				{ BEGIN 0; }
<SLCOMMENT>[^\n]*			;

	/* Match anything that's been missed and pass it as a char */
.							return scr_text[0];

%%

/* Set the current input buffer for the lexer */
void scriptSetInputBuffer(char *pBuffer, UDWORD size)
{
	pInputBuffer = pBuffer;
	pEndBuffer = pBuffer + size;

	/* Reset the lexer in case it's been used before */
	scr__flush_buffer( YY_CURRENT_BUFFER );
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
		DBERROR(("Warning: reached end of file in a comment"));
	}
	return 1;
}
