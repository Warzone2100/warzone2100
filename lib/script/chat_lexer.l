%{
/*
 * chat_lexer.l
 *
 * lexer for multiplayer chat messages.
 * IMPORTANT: must be compiled with -i switch (case-insensitive scanner)
 *
 */

#include <stdio.h>

#include "lib/framework/frame.h"
#include "src/base.h"
#include "src/droiddef.h"
#include "src/structuredef.h"
//#include "src/scriptfuncs.h"
#include "lib/script/script.h"
#include "src/scriptfuncs.h"
#include "lib/script/chat_processing.h"

/* Get the Yacc definitions */
#include "lib/script/chat_parser.h"

/* Maximum length for any TEXT value */
#ifndef YYLMAX
#define YYLMAX	255
#endif

/* Store for any string values */
static char aText[TEXT_BUFFERS][YYLMAX];
static UDWORD currText=0;

/* Pointer to the input buffer */
static char *pInputBuffer = NULL;
static char *pEndBuffer = NULL;

static SDWORD playerIndex;
static INTERP_VAL parameter;

#define YY_INPUT(buf,result,max_size) \
      if (pInputBuffer != pEndBuffer) { \
        buf[0] = *(pInputBuffer++); result = 1; \
      } else { \
        buf[0] = EOF; result = YY_NULL; \
      }

#undef chat_getc
#define chat_getc() (pInputBuffer != pEndBuffer ? *(pInputBuffer++) : EOF)

%}

%option yylineno
%option prefix="chat_"
%option nounput

%%

	/* Match integer numbers */
-?[0-9]+					{ chat_lval.ival = atol(chat_text); return R_INTEGER; }

	/* Keywords - Terminals */
"?"+						{ return _T_QM; }
"!"+						{ return _T_EM; }
"."+						{ return _T_FULLSTOP; }
":"							{ return _T_COLON; };
";"							{ return _T_SEMICOLON; };
","							{ return _T_COMMA; };
"a"							{ return _T_A; }
"affirmative"				{ return _T_AFFIRMATIVE; }
"after"						{ return _T_AFTER; }
"ally"						{ return _T_ALLY; }
"am"						{ return _T_AM; }
"and"						{ return _T_AND; }
"any"						{ return _T_ANY; }
"attack"					{ return _T_ATTACK; }
"attacking"					{ return _T_ATTACKING; }
"beacon"					{ return _T_BEACON; }
"building"					{ return _T_BUILDING; }
"can't"						{ return _T_CANT; }
"center"					{ return _T_CENTER; }
"dead"						{ return _T_DEAD; }
"derrick"					{ return _T_DERRICK; }
"do"						{ return _T_DO; }
"drop"						{ return _T_DROP; }
"fine"						{ return _T_FINE; }
"get"						{ return _T_GET; }
"getting"					{ return _T_GETTING; }
"go"						{ return _T_GO; }
"going"						{ return _T_GOING; }
"gonna"						{ return _T_GONNA; }
"got"						{ return _T_GOT; }
"great"						{ return _T_GREAT; }
"have"						{ return _T_HAVE; }
"has"						{ return _T_HAS; }
"help"						{ return _T_HELP; }
"i"							{ return _T_I; }
"i'm"						{ return _T_IM; }
"is"						{ return _T_IS; }
"let's"						{ return _T_LETS; }
"me"						{ return _T_ME; }
"no"						{ return _T_NO; }
"of course"					{ return _T_OFCOURSE; }
"ok"+						{ return _T_OK; }
"place"						{ return _T_PLACE; }
"possession"				{ return _T_POSSESSION; }
"power"						{ return _T_POWER; }
"pumping"					{ return _T_PUMPING; }
"put"						{ return _T_PUT; }
"roger"						{ return _T_ROGER; }
"see"						{ return _T_SEE; }
"some"						{ return _T_SOME; }
"status"					{ return _T_STATUS; }
"stop"						{ return _T_STOP; }
"sure"						{ return _T_SURE; }
"u"							{ return _T_U; }
"units"						{ return _T_UNITS; }
"vtols"						{ return _T_VTOLS; }
"wait"						{ return _T_WAIT; }
"where"						{ return _T_WHERE; }
"yea"						{ return _T_YEA; }
"yeah"						{ return _T_YEAH; }
"yes"						{ return _T_YES; }
"you"						{ return _T_YOU; }

	/* <<EOF>>					{ return _T_EOF; } */

-?[0-9]+					{
								chat_lval.ival = atol(chat_text);
								return R_INTEGER;
							}

[0-9_a-zA-Z_]*			{
								playerIndex = getPlayerFromString(chat_text);

								if(playerIndex >= 0)
								{
									//console( "matched 'player'");
									chat_lval.ival = playerIndex;

									return R_PLAYER;
								}

							}

	/* [^ \t\n\<\>\[\]\(\)]+		{ return T_WORD; } */
	/* [^ \t\n]+					{ console( "matched 'T_WORD'"); return T_WORD; } */


	/* Skip white space */
[ \t\n\x0d\x0a]						;


	/* Match anything that's been missed and pass it as a char */
	/* .							{console( "matched 'anything else '%s'", chat_text[0]); return chat_text[0];} */

%%

/* Set the current input buffer for the lexer */
void chatSetInputBuffer(char *pBuffer, UDWORD size)
{
	pInputBuffer = pBuffer;
	pEndBuffer = pBuffer + size;

	/* Reset the lexer in case it's been used before */
	chat__flush_buffer(YY_CURRENT_BUFFER);
}

void chatGetErrorData(int *pLine, char **ppText)
{
	*pLine = chat_lineno;
	*ppText = chat_text;
}

int chat_wrap(void)
{
	return 1;
}

