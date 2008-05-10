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
/* @file
 *
 * lexer for loading level description files
 */

#include "lib/framework/frame.h"

#include "levels.h"
#include "levelint.h"

// Get the parser's definitions
#include "level_parser.tab.h"

/* Maximum length for any TEXT value */
#ifndef YYLMAX
#define YYLMAX	255
#endif

#include "lib/framework/lexer_input.h"

#ifndef yyextra
# define yyextra yyget_extra()
#endif

/* Older GNU Flex versions don't define yyget_extra(), yyset_extra(),
 * yyget_text() and yyget_lineno().
 * (and neither define a subminor version)
 */
#if !defined(YY_FLEX_SUBMINOR_VERSION) || (YY_FLEX_SUBMINOR_VERSION < 9)
# define yyget_extra  lev_get_extra
# define yyset_extra  lev_set_extra
# define yyget_lineno lev_get_lineno
# define yyget_text   lev_get_text
extern void yyset_extra(YY_EXTRA_TYPE user_defined);
extern YY_EXTRA_TYPE yyget_extra(void);
extern int yyget_lineno(void);
int yyget_lineno()
{
	return yylineno;
}

extern char* yyget_text(void);
char* yyget_text()
{
	return yytext;
}
#endif

%}

%option yylineno noyywrap nounput
%option prefix="lev_"

%x COMMENT
%x SLCOMMENT
%x QUOTE

%%

	/* Keywords */
level                                           return LEVEL;
players                                         return PLAYERS;
type                                            return TYPE;
data                                            return DATA;
game                                            return GAME;
campaign                                        return CAMPAIGN;
camstart                                        return CAMSTART;
camchange                                       return CAMCHANGE;
dataset                                         return DATASET;
expand                                          return EXPAND;
expand_limbo                                    return EXPAND_LIMBO;
between                                         return BETWEEN;
miss_keep                                       return MISS_KEEP;
miss_keep_limbo                                 return MISS_KEEP_LIMBO;
miss_clear                                      return MISS_CLEAR;

	/* Match text values */
[a-zA-Z][-0-9_a-zA-Z]*		{
								lev_lval.sval = strdup(yytext);
								return IDENTIFIER;
							}

	/* Match quoted text */
\"							{ BEGIN QUOTE; }
<QUOTE>\"					{ BEGIN 0; }
<QUOTE>\n					{ lev_error("Unexpected end of line in string"); }
<QUOTE>[^\"]*				{
								lev_lval.sval = strdup(yytext);
								return QTEXT;
							}

	/* Match integer numbers */
-?[0-9]+					{ lev_lval.ival = atol(yytext); return INTEGER; }

	/* Skip white space */
[ \t\n\x0d\x0a]						;

	/* Strip comments */
"/*"						{ BEGIN COMMENT; }
<COMMENT>"*/" |
<COMMENT>"*/"\n				{ BEGIN 0; }
<COMMENT>. |
<COMMENT>\n					;

	/* Strip single line comments */
"//"						{ BEGIN SLCOMMENT; }
<SLCOMMENT>\n				{ BEGIN 0; }
<SLCOMMENT>[^\n]*			;

	/* Match anything that's been missed and pass it as a char */
.							return yytext[0];

%%

static YY_EXTRA_TYPE pBuffer = NULL;

void yyset_extra(YY_EXTRA_TYPE user_defined)
{
	pBuffer = user_defined;
}

YY_EXTRA_TYPE yyget_extra()
{
	return pBuffer;
}

/* Older GNU Flex versions don't define yylex_destroy()
 * (and neither define a subminor version)
 */
#if !defined(YY_FLEX_SUBMINOR_VERSION) || (YY_FLEX_SUBMINOR_VERSION < 9)
int lev_lex_destroy(void)
{
	/* For non-reentrant C scanner only. */
	yy_delete_buffer(YY_CURRENT_BUFFER);
	yy_init = 1;
}
#endif
