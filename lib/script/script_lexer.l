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

	//debug( LOG_SCRIPT, "scriptGetVarToken" );

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
				//debug( LOG_SCRIPT, "scriptGetVarToken: VAL_INT (NUM_VAR)" );
				return NUM_VAR;
				break;
			default:
				return VAR;
				break;
			}
		}
	}

	//debug( LOG_SCRIPT, "END scriptGetVarToken" );
}

/* Get the token type for a constant symbol */
SDWORD scriptGetConstToken(CONST_SYMBOL *psConst)
{
	BOOL	object;

	//debug( LOG_SCRIPT, "scriptGetConstToken" );

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
	default:
		if (object)
		{
			return OBJ_CONSTANT;
		}
		else
		{
			return USER_CONSTANT;
		}
		break;
	}

	//debug( LOG_SCRIPT, "END scriptGetConstToken" );
}

/* Get the token type for a function symbol */
SDWORD scriptGetFuncToken(FUNC_SYMBOL *psFunc)
{
	BOOL	object;

	//debug( LOG_SCRIPT, "scriptGetFuncToken" );

	// See if this is an object pointer
	if (!asScrTypeTab || psFunc->type < VAL_USERTYPESTART)
	{
		object = FALSE;
	}
	else
	{
		object = asScrTypeTab[psFunc->type - VAL_USERTYPESTART].accessType == AT_OBJECT;
	}

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
		case VAL_VOID:
			return FUNC;
			break;
		default:
			return USER_FUNC;
			break;
		}
	}

	//debug( LOG_SCRIPT, "END scriptGetFuncToken" );
}


/* Get the token type for a custom function symbol */
SDWORD scriptGetCustomFuncToken(EVENT_SYMBOL *psFunc)
{
	BOOL	object;

	//debug( LOG_SCRIPT, "scriptGetCustomFuncToken" );

	// See if this is an object pointer
	object = asScrTypeTab[psFunc->retType - VAL_USERTYPESTART].accessType == AT_OBJECT;

	//debug( LOG_SCRIPT, "scriptGetCustomFuncToken 3" );

	if (object)
	{
		//debug( LOG_SCRIPT, "scriptGetCustomFuncToken: object" );
		return OBJ_FUNC_CUST;
	}
	else
	{
		switch (psFunc->retType)
		{
		case VAL_BOOL:
			//debug( LOG_SCRIPT, "scriptGetCustomFuncToken: VAL_BOOL" );
			return BOOL_FUNC_CUST;
			break;
		case VAL_INT:
//		case VAL_FLOAT:
			//debug( LOG_SCRIPT, "scriptGetCustomFuncToken: VAL_INT" );
			return NUM_FUNC_CUST;
			break;
		case VAL_STRING:		/* <NEW> */
			//debug( LOG_SCRIPT, "scriptGetCustomFuncToken: VAL_STRING" );
			return STRING_FUNC_CUST;
			break;
		case VAL_VOID:
			//debug( LOG_SCRIPT, "scriptGetCustomFuncToken: void function" );
			return VOID_FUNC_CUST;
			break;
		default:
			//debug( LOG_SCRIPT, "scriptGetCustomFuncToken: default" );
			return USER_FUNC_CUST;
			break;
		}
	}

	//debug( LOG_SCRIPT, "END scriptGetCustomFuncToken" );
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

	/* new stuff */
function				return FUNCTION;
return					{ return RET; }

public					{ scr_lval.stype = ST_PUBLIC; return STORAGE; }
private					{ scr_lval.stype = ST_PRIVATE; return STORAGE; }
local					{ scr_lval.stype = ST_LOCAL; return STORAGE; }
while					return WHILE;
if						return IF;
else					return ELSE;
exit					return EXIT;
pause					return PAUSE;

	/* Match to type key words */
bool					{ scr_lval.tval = VAL_BOOL; return TYPE; }
BOOL					{ scr_lval.tval = VAL_BOOL; return TYPE; }
int						{ scr_lval.tval = VAL_INT; return TYPE; }
INT						{ scr_lval.tval = VAL_INT; return TYPE; }

	/* return value for void functions */
void					{ scr_lval.tval = VAL_VOID; return TYPE; }
VOID					{ scr_lval.tval = VAL_VOID; return TYPE; }

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
[a-zA-Z][0-9_a-zA-Z]*		{
								//debug( LOG_SCRIPT, "FLEX: looking up '%s'", scr_text );

								/* See if this identifier has been defined as a type */
								if (scriptLookUpType(scr_text, &scr_lval.tval))
								{
									DBP0(("[lex] TYPE\n"));
									return TYPE;
								}
								/* See if this identifier has been defined as a variable */
								else if (scriptLookUpVariable(scr_text, &scr_lval.vSymbol))
								{
									DBP0(("[lex] variable\n"));
									return scriptGetVarToken(scr_lval.vSymbol);
								}
								/* See if this identifier has been defined as a constant */
								else if (scriptLookUpConstant(scr_text, &scr_lval.cSymbol))
								{
									DBP0(("[lex] constant\n"));
									return scriptGetConstToken(scr_lval.cSymbol);
								}
								/* See if this identifier has been defined as a function */
								else if (scriptLookUpFunction(scr_text, &scr_lval.fSymbol))
								{
									DBP0(("[lex] func\n"));
									return scriptGetFuncToken(scr_lval.fSymbol);
								}

								/* See if this identifier has been defined as a custom function */
								else if (scriptLookUpCustomFunction(scr_text, &scr_lval.eSymbol))
								{
									//debug( LOG_SCRIPT, "scriptLookUpCustomFunction: '%s' - custom function", scr_text );
									return scriptGetCustomFuncToken(scr_lval.eSymbol);
								}

								else if (scriptLookUpTrigger(scr_text, &scr_lval.tSymbol))
								{
									DBP0(("[lex] TRIG_SYM\n"));
									return TRIG_SYM;
								}
								else if (scriptLookUpEvent(scr_text, &scr_lval.eSymbol))
								{
									DBP0(("[lex] EVENT_SYM\n"));
									return EVENT_SYM;
								}
								else if (scriptLookUpCallback(scr_text, &scr_lval.cbSymbol))
								{
									DBP0(("[lex] CALLBACK_SYM\n"));
									return CALLBACK_SYM;
								}
								else
								{
									//debug( LOG_SCRIPT, "FLEX: '%s' is an ident", scr_text );

									strcpy(aText[currText], scr_text);
									scr_lval.sval = aText[currText];
									currText = (currText + 1) % TEXT_BUFFERS;
									DBP0(("[lex] IDENT\n"));
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
		DBERROR(("Warning: reched end of file in a comment"));
	}
	return 1;
}
