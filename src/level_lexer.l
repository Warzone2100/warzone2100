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

// Note if we are in a comment
static BOOL inComment = false;

/* Pointer to the input buffer */
static const char *pInputBuffer = NULL;
static const char *pEndBuffer = NULL;

#define YY_INPUT(buf, result, max_size) \
      if (pInputBuffer != pEndBuffer) { \
        buf[0] = *(pInputBuffer++); result = 1; \
      } else { \
        buf[0] = EOF; result = YY_NULL; \
      }

%}

%option yylineno nounput
%option prefix="lev_"
%option header-file="level_lexer.lex.h"

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

/* Set the current input buffer for the lexer */
void levSetInputBuffer(const char* buffer, size_t size)
{
	pInputBuffer = buffer;
	pEndBuffer = &buffer[size];

	/* Reset the lexer incase it's been used before */
	lev__flush_buffer(YY_CURRENT_BUFFER);
	inComment = false;
}

int lev_wrap(void)
{
	if (inComment)
	{
		debug( LOG_ERROR, "Warning: reched end of file in a comment" );
		abort();
	}
	return 1;
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
