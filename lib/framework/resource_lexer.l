%{
/*
 * resource.l
 *
 * Lex file for parsing res files
 */

#include <stdio.h>

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include <string.h>
#include "types.h"
#include "debug.h"
#include "resly.h"

/* Get the Yacc definitions */
#include "resource_parser.h"

void res_error(const char *pMessage,...);

#ifndef YYLMAX
#define YYLMAX 8192
#endif

/* Store for any string values */
STRING aText[TEXT_BUFFERS][YYLMAX];		// No longer static ... lets use this area globally
static UDWORD currText=0;

// Note if we are in a comment
static BOOL inComment = FALSE;

/* Pointer to the input buffer */
static char *pInputBuffer = NULL;
static char *pEndBuffer = NULL;

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
								strcpy(aText[currText], res_text);
								res_lval.sval = aText[currText];
								currText = (currText + 1) % TEXT_BUFFERS;
								return TEXT_T;
							}

	/* Match quoted text */
\"							{ BEGIN QUOTE; }
<QUOTE>\"					{ BEGIN 0; }
<QUOTE>\n					{ res_error("Unexpected end of line in string"); }
<QUOTE>[^\"\n]*				{
								strcpy(aText[currText], res_text);
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
.							return res_text[0];

%%

/* Set the current input buffer for the lexer */
void resSetInputBuffer(char *pBuffer, UDWORD size)
{
	pInputBuffer = pBuffer;
	pEndBuffer = pBuffer + size;

	/* Reset the lexer incase it's been used before */
	res__flush_buffer(YY_CURRENT_BUFFER);
	inComment = FALSE;
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
		DBERROR(("Warning: reached end of file in a comment"));
	}
	return 1;
}

