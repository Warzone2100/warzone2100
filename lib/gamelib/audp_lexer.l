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

/* include framework */
#include "lib/framework/frame.h"

#include <physfs.h>

#include "lib/gamelib/parser.h"

/* Get the Yacc definitions */
#include "audp_parser.tab.h"

/* Maximum length for any TEXT value */
#define YYLMAX	255

/* global variables */
static	BOOL	g_bParsingSubFile;
static	FILE	*g_fpOld;

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

void audp_error(const char* fmt, ...);

%}

%option nounput
%option prefix="audp_"
%option yylineno

%x COMMENT
%x QUOTE

%%
	/* Match to key words */
oneshot							{	return ONESHOT;			}
loop							{	return LOOP;			}
audio							{	return AUDIO;			}
anim3dfile						{	return ANIM3DFILE;		}
audio_module					{	return AUDIO_MODULE;	}
anim_module						{	return ANIM_MODULE;		}
ANIM3DFRAMES					{	return ANIM3DFRAMES;	}
ANIM3DTRANS						{	return ANIM3DTRANS;		}
ANIMOBJECT						{	return ANIMOBJECT;		}

	/* Match floating point numbers */
	/* This is a problem with the PSX so is disabled
-?[0-9]*"."[0-9]+				{	audp_lval.fval = (float) atof(audp_text);
									return FLOAT_T;
								}
	*/

	/* Match integer numbers */
-?[0-9]+						{	audp_lval.ival = atoi(audp_text);
									return INTEGER;
								}

	/* Match quoted text */
\"[^\"\n]*[\"\n]				{
									/* skip opening quote */
									strcpy( audp_lval.sval, audp_text+1 );

									/* check for unterminated string */
									if ( audp_text[audp_leng-1] != '"' )
									{
										sprintf( audp_lval.sval, "Unterminated string %s\n", audp_lval.sval );
										audp_error( audp_lval.sval );
										return (1);
									}

									/* set final quote in string to blank */
									audp_lval.sval[audp_leng-2] = (char) '\0';

									return QTEXT;
								}

	/* Skip white space */
[ \t\n\x0d\x0a]						;

	/* Strip comments */
"/*"						{ BEGIN COMMENT; }
<COMMENT>"*/" |
<COMMENT>"*/"\n				{ BEGIN 0; }
<COMMENT>. |
<COMMENT>\n					;

	/* Match anything that's been missed and pass it as a char */
.							return audp_text[0];


%%

/***************************************************************************/

int
audp_wrap( void )
{
	if ( g_bParsingSubFile == true )
	{
		/* close current file and restore old file pointer */
		fclose( audp_in );
		audp_in = g_fpOld;

		g_bParsingSubFile = false;

		return 0;
	}
	else
	{
		return 1;
	}
}

/***************************************************************************/
/* Set the current input file for the lexer */
/***************************************************************************/

void parserSetInputFile(PHYSFS_file* fileHandle)
{
	pReadFile = fileHandle;
}

/***************************************************************************/

void
parseGetErrorData(int *pLine, char **ppText)
{
	*pLine  = audp_lineno;
	*ppText = audp_text;
}

/***************************************************************************/

/* Older GNU Flex versions don't define yylex_destroy()
 * (and neither define a subminor version)
 */
#if !defined(YY_FLEX_SUBMINOR_VERSION) || (YY_FLEX_SUBMINOR_VERSION < 9)
int audp_lex_destroy(void)
{
	/* For non-reentrant C scanner only. */
	yy_delete_buffer(YY_CURRENT_BUFFER);
	yy_init = 1;
}
#endif
