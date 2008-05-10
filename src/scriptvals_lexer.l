/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
%{
/*
 * ScriptVals.l
 *
 * lexer for loading script variable values
 *
 */
#include "lib/framework/frame.h"

#include <physfs.h>

#include "lib/script/script.h"
#include "src/scriptvals.h"

/* Get the Yacc definitions */
#include "scriptvals_parser.tab.h"


/* Maximum length for any TEXT value */
#ifndef YYLMAX
#define YYLMAX	255
#endif

/* Store for any string values */
static char aText[TEXT_BUFFERS][YYLMAX];
static UDWORD currText=0;

// Note if we are in a comment
static BOOL inComment = false;

#include "lib/framework/lexer_input.h"

#ifndef yyextra
# define yyextra yyget_extra()
#endif

%}

%option yylineno nounput
%option warn nodefault
%option prefix="scrv_"
%option header-file="scriptvals_lexer.lex.h"

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

true						{ scrv_lval.bval = true;	 return BOOLEAN_T; }
TRUE						{ scrv_lval.bval = true;	 return BOOLEAN_T; }
false						{ scrv_lval.bval = false;	 return BOOLEAN_T; }
FALSE						{ scrv_lval.bval = false;	 return BOOLEAN_T; }

	/* Match text values */
[a-zA-Z][-0-9_a-zA-Z]*		{
								INTERP_TYPE type;
								UDWORD		index;

								/* See if this is a variable id or a type */
								if (scrvLookUpType(yytext, &type))
								{
									scrv_lval.tval = type;
									return TYPE;
								}
								else if (scrvLookUpVar(yytext, &index))
								{
									scrv_lval.vindex = index;
									return VAR;
								}
								else if (scrvLookUpArray(yytext, &index))
								{
									scrv_lval.vindex = index;
									return ARRAY;
								}
								else
								{
									strcpy(aText[currText], yytext);
									scrv_lval.sval = aText[currText];
									currText = (currText + 1) % TEXT_BUFFERS;
									return IDENT;
								}
							}

	/* Match integer numbers */
-?[0-9]+					{ scrv_lval.ival = atol(yytext); return INTEGER; }

	/* Match quoted text */
\"							{ BEGIN QUOTE; }
<QUOTE>\"					{ BEGIN 0; }
<QUOTE>\n					{ scrv_error("Unexpected end of line in string"); }
<QUOTE>[^\"\n]*				{
								strcpy(aText[currText], yytext);
								scrv_lval.sval = aText[currText];
								currText = (currText + 1) % TEXT_BUFFERS;
								return QTEXT;
							}

	/* Skip white space */
[ \t\n\x0d\x0a]						;

	/* Strip comments */
"/*"						{ inComment=true; BEGIN COMMENT; }
<COMMENT>"*/" |
<COMMENT>"*/"\n				{ inComment=false; BEGIN 0; }
<COMMENT>. |
<COMMENT>\n					;

	/* Strip single line comments */
"//"						{ BEGIN SLCOMMENT; }
<SLCOMMENT>\n				{ BEGIN 0; }
<SLCOMMENT>[^\n]*			;

	/* Match anything that's been missed and pass it as a char */
.							return yytext[0];

%%

static YY_EXTRA_TYPE pReadFile = NULL;

void yyset_extra(YY_EXTRA_TYPE user_defined)
{
	pReadFile = user_defined;
}

YY_EXTRA_TYPE yyget_extra()
{
	return pReadFile;
}

int scrv_wrap(void)
{
	if (inComment)
	{
		debug( LOG_ERROR, "Warning: reached end of file in a comment" );
		abort();
	}
	return 1;
}


/* Older GNU Flex versions don't define yylex_destroy()
 * (and neither define a subminor version)
 */
#if !defined(YY_FLEX_SUBMINOR_VERSION) || (YY_FLEX_SUBMINOR_VERSION < 9)
int scrv_lex_destroy(void)
{
	/* For non-reentrant C scanner only. */
	yy_delete_buffer(YY_CURRENT_BUFFER);
	yy_init = 1;
}
#endif
