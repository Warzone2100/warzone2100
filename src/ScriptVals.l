%{
/*
 * ScriptVals.l
 *
 * lexer for loading script variable values
 *
 */

#ifdef WIN32
#include <stdio.h>
#else
/* A few definitions so the lex generated code will compile on the PSX.
 * These shouldn't actually be used by any code that is run on the PSX, it
 * just keeps the compiler happy.
 */
#ifndef _FILE_
#define _FILE_
typedef int FILE;	// in stdio.h ?
#endif
#define stderr 0
#define stdin  0
#define stdout 0
static int fprintf(FILE* f,char* c,...)
{
	return 0;
}
#endif

#include "Frame.h"
#include "Script.h"
#include "ScriptVals.h"

/* Get the Yacc definitions */
#include "ScriptVals_y.h"

/* Turn off a couple of warnings that the lex generated code gives */
#pragma warning ( disable : 4102 4305 )

/* Maximum length for any TEXT value */
#define YYLMAX	255

/* Store for any string values */
static STRING aText[TEXT_BUFFERS][YYLMAX];
static UDWORD currText=0;

// Note if we are in a comment
static BOOL inComment = FALSE;

/* Pointer to the input buffer */
static UBYTE *pInputBuffer = NULL;
static UBYTE *pEndBuffer = NULL;

#undef scrv_getc
#define scrv_getc() (pInputBuffer != pEndBuffer ? *(pInputBuffer++) : EOF)

%}

%x COMMENT
%x SLCOMMENT
%x QUOTE

%%

	/* Keywords */
int							{ scrv_lval.tval = VAL_INT; return TYPE; }
INT							{ scrv_lval.tval = VAL_INT; return TYPE; }
bool						{ scrv_lval.tval = VAL_BOOL; return TYPE; }
BOOL						{ scrv_lval.tval = VAL_BOOL; return TYPE; }
script						return SCRIPT;
store						return STORE;
run							return RUN;

true						{ scrv_lval.bval = TRUE;	 return BOOLEAN; }
TRUE						{ scrv_lval.bval = TRUE;	 return BOOLEAN; }
false						{ scrv_lval.bval = FALSE;	 return BOOLEAN; }
FALSE						{ scrv_lval.bval = FALSE;	 return BOOLEAN; }

	/* Match text values */
[a-zA-Z][-0-9_a-zA-Z]*		{
								INTERP_TYPE type;
								UDWORD		index;

								/* See if this is a variable id or a type */
								if (scrvLookUpType(scrv_text, &type))
								{
									scrv_lval.tval = type;
									return TYPE;
								}
								else if (scrvLookUpVar(scrv_text, &index))
								{
									scrv_lval.vindex = index;
									return VAR;
								}
								else if (scrvLookUpArray(scrv_text, &index))
								{
									scrv_lval.vindex = index;
									return ARRAY;
								}
								else
								{
									strcpy(aText[currText], scrv_text);
									scrv_lval.sval = aText[currText];
									currText = (currText + 1) % TEXT_BUFFERS;
									return IDENT;
								}
							}

	/* Match integer numbers */
-?[0-9]+					{ scrv_lval.ival = atol(scrv_text); return INTEGER; }

	/* Match quoted text */
\"							{ BEGIN QUOTE; }
<QUOTE>\"					{ BEGIN 0; }
<QUOTE>\n					{ scrv_error("Unexpected end of line in string"); }
<QUOTE>[^\"\n]*				{
								strcpy(aText[currText], scrv_text);
								scrv_lval.sval = aText[currText];
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
.							return scrv_text[0];

%%

/* Set the current input buffer for the lexer */
void scrvSetInputBuffer(UBYTE *pBuffer, UDWORD size)
{
	pInputBuffer = pBuffer;
	pEndBuffer = pBuffer + size;

	/* Reset the lexer incase it's been used before */
	scrv__reset();
}

void scrvGetErrorData(int *pLine, char **ppText)
{
	*pLine = scrv_lineno;
	*ppText = scrv_text;
}

int scrv_wrap(void)
{
	if (inComment)
	{
		DBERROR(("Warning: reched end of file in a comment"));
	}
	return 1;
}

