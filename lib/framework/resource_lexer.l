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
 * resource.l
 *
 * Lex file for parsing res files
 */

#include "lib/framework/frame.h"

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include "lib/framework/resly.h"

/* Get the Yacc definitions */
#include "resource_parser.tab.h"

void res_error(const char *pMessage,...);

/* Maximum length for any TEXT_T value */
#define YYLMAX 8192

/* Store for any string values */
char aText[TEXT_BUFFERS][YYLMAX];		// No longer static ... lets use this area globally
static UDWORD currText=0;

// Note if we are in a comment
static BOOL inComment = false;

/* Pointer to the input buffer */
static const char *pInputBuffer = NULL;
static const char *pEndBuffer = NULL;

#define YY_INPUT(buf,result,max_size) \
			if (pInputBuffer != pEndBuffer) { \
				buf[0] = *(pInputBuffer++); result = 1; \
			} else { \
				buf[0] = EOF; result = YY_NULL; \
			}

%}

%option nounput
%option prefix="res_"
%option yylineno

%x COMMENT
%x QUOTE
%x SLCOMMENT

%%
	/* Match to key words */
directory					{ return DIRECTORY; }
file						{ return FILETOKEN; }

	/* Match text values */
[a-zA-Z][-0-9_a-zA-Z]*		{
								strlcpy(aText[currText], res_text, sizeof(aText[currText]));
								res_lval.sval = aText[currText];
								currText = (currText + 1) % TEXT_BUFFERS;
								return TEXT_T;
							}

	/* Match quoted text */
\"							{ BEGIN QUOTE; }
<QUOTE>\"					{ BEGIN 0; }
<QUOTE>\n					{ res_error("Unexpected end of line in string"); }
<QUOTE>[^\"\n]*				{
								strlcpy(aText[currText], res_text, sizeof(aText[currText]));
								res_lval.sval = aText[currText];
								currText = (currText + 1) % TEXT_BUFFERS;
								return QTEXT_T;
							}
\"\"						{
								aText[currText][0] = '\0';
								aText[currText][1] = '\0';
								res_lval.sval = aText[currText];
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
.							return res_text[0];

%%

/* Set the current input buffer for the lexer */
void resSetInputBuffer(char *pBuffer, UDWORD size)
{
	pInputBuffer = pBuffer;
	pEndBuffer = pBuffer + size;

	/* Reset the lexer incase it's been used before */
	res__flush_buffer(YY_CURRENT_BUFFER);
	inComment = false;
}

void resGetErrorData(int *pLine, char **ppText)
{
	*pLine = res_lineno;
	*ppText = res_text;
}

int res_wrap(void)
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
#if !defined(YY_FLEX_SUBMINOR_VERSION) || (YY_FLEX_SUBMINOR_VERSION < 30)
int res_lex_destroy(void)
{
}
#endif
