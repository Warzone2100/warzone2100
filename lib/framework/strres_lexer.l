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
 * StrRes.l
 *
 * Lex file for parsing string resource files
 */

#include "lib/framework/frame.h"

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include "lib/framework/strres.h"
#include "lib/framework/strresly.h"

#include <physfs.h>

/* Get the Yacc definitions */
#include "strres_parser.tab.h"

void strres_error(const char *pMessage,...);

/* Maximum length for any TEXT_T value */
#define YYLMAX 8192

/* Store for any string values */
extern char aText[TEXT_BUFFERS][YYLMAX];
static UDWORD currText=0;

// Note if in a comment
static BOOL inComment;

/* Handle to the input file */
static PHYSFS_file* pReadFile = NULL;

#define YY_INPUT(buf,result,max_size) \
		if (PHYSFS_eof(pReadFile)) \
		{ \
			buf[0] = EOF; result = YY_NULL; \
		} \
		else { \
			result = PHYSFS_read(pReadFile, buf, 1, max_size); \
			if (result == -1) \
			{ \
				buf[0] = EOF; result = YY_NULL; \
			} \
		}
%}

%option nounput
%option prefix="strres_"
%option yylineno

%x COMMENT
%x QUOTE
%x SLCOMMENT

%%

	/* Match text values */
[a-zA-Z][-0-9_a-zA-Z]*		{
								strlcpy(aText[currText], strres_text, sizeof(aText[currText]));
								strres_lval.sval = aText[currText];
								currText = (currText + 1) % TEXT_BUFFERS;
								return TEXT_T;
							}

	/* Match quoted text */
\"							{ BEGIN QUOTE; }
<QUOTE>\"					{ BEGIN 0; }
<QUOTE>\n					{ strres_error("Unexpected end of line in string"); }
<QUOTE>[^\"\n]*				{
								strlcpy(aText[currText], strres_text, sizeof(aText[currText]));
								strres_lval.sval = aText[currText];
								currText = (currText + 1) % TEXT_BUFFERS;
								return QTEXT_T;
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
.							return strres_text[0];

%%

/* Set the current input file for the lexer */
void strresSetInputFile(PHYSFS_file* fileHandle)
{
	pReadFile = fileHandle;

	/* Reset the lexer incase it's been used before */
	yy_flush_buffer( YY_CURRENT_BUFFER );
}

void strresGetErrorData(int *pLine, char **ppText)
{
	*pLine = strres_lineno;
	*ppText = strres_text;
}

int strres_wrap(void)
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
int strres_lex_destroy(void)
{
	/* For non-reentrant C scanner only. */
	yy_delete_buffer(YY_CURRENT_BUFFER);
	yy_init = 1;
}
#endif
