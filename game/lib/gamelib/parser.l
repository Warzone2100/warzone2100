%{

#ifndef PSX
#include <stdio.h>
#else
/* A few definitions so the lex generated code will compile on the PSX.
 * These shouldn't actually be used by any code that is run on the PSX, it
 * just keeps the compiler happy.
 */
#ifndef _FILE_
typedef signed int FILE;
#define _FILE_
#endif
#define stderr 0
#define stdin  0
#define stdout 0
static int fprintf(FILE* f,char* c,...)
{
	return 0;
}
#endif

/* Turn off a couple of warnings that the lex generated code gives */
#pragma warning ( disable : 4102 4129 4305 )

/* include framework */
#include "frame.h"

#include "parser.h"

/* Get the Yacc definitions */
#include "parser_y.h"

/* Maximum length for any TEXT value */
#define YYLMAX	255

/* extern funcs */
extern int	audp_parse YY_ARGS( (void) );

/* global variables */
static	BOOL	g_bParsingSubFile;
static	FILE	*g_fpOld;

/* Pointer to the input buffer */
static UBYTE *pInputBuffer = NULL;
static UBYTE *pEndBuffer = NULL;

static int	audp_GetChar( void );

#undef audp_getc
#define audp_getc() ( pInputBuffer != pEndBuffer ? *(pInputBuffer++) : EOF )

%}

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
									return FLOAT;
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
									audp_lval.sval[audp_leng-2] = (char) NULL;

									return QTEXT;
								}

	/* Match text values */
[a-zA-Z"."#][-0-9_a-zA-Z"."#]*	{	strcpy( audp_lval.sval, audp_text );
									return TEXT;
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

BOOL
ParseFile( char szFileName[] )
{
	FILE	*fp;

	/* open input file */
	if ( (fp = fopen( szFileName, "rt" )) == NULL )
	{
		DBERROR( ("ParseFile: file not found\n") );
		return FALSE;
	}

	/* point input to input file */
	audp_in = fp;

	audp_parse();

	fclose( fp );

	return TRUE;
}

/***************************************************************************/

void
IncludeFile( char szFileName[] )
{
	FILE		*fpNew;

	/* open module file */
	if ( (fpNew = fopen( szFileName, "rt" )) != NULL )
	{
		/* save current file pointer and switch to new */
		g_fpOld = audp_in;
		audp_in = fpNew;

		g_bParsingSubFile = TRUE;
	}
	else
	{
		DBERROR( ("Included file %s not found\n", szFileName) );
	}
}

/***************************************************************************/

int
audp_wrap( void )
{
	if ( g_bParsingSubFile == TRUE )
	{
		/* close current file and restore old file pointer */
		fclose( audp_in );
		audp_in = g_fpOld;

		g_bParsingSubFile = FALSE;

		return 0;
	}
	else
	{
		return 1;
	}
}

/***************************************************************************/
/* Set the current input buffer for the lexer */
/***************************************************************************/

void
parserSetInputBuffer( UBYTE *pBuffer, UDWORD size )
{
	pInputBuffer = pBuffer;
	pEndBuffer = pBuffer + size;

	/* Reset the lexer in case it's been used before */
	audp__reset();
}

/***************************************************************************/

void
parseGetErrorData(int *pLine, char **ppText)
{
	*pLine  = audp_lineno;
	*ppText = audp_text;
}

/***************************************************************************/
