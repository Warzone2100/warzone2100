%{
/*
 * StrRes.y
 *
 * Yacc file for parsing String Resource files
 */

#ifdef PSX
/* A few definitions so the yacc generated code will compile on the PSX.
 * These shouldn't actually be used by any code that is run on the PSX, it
 * just keeps the compiler happy.
 */
static int printf(char* c, ...)
{
	return 0;
}
#endif

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include "types.h"
#include "debug.h"
#include "mem.h"
#include "heap.h"
#include "treap.h"
#include "StrRes.h"
#include "StrResLY.h"

/* Turn off a couple of warnings that the yacc generated code gives */
#pragma warning ( disable : 4305 4102)

%}

%union {
	STRING  *sval;
}

	/* value tokens */
%token <sval> TEXT
%token <sval> QTEXT			/* Text with double quotes surrounding it */

%%

file:			line
			|	file line
			;

line:			TEXT QTEXT
							{
								/* Pass the text string to the string manager */
								if (!strresStoreString(psCurrRes, $1, $2))
								{
									YYABORT;
								}
							}
			;

%%


/*
 * A simple error reporting routine
 */

void strres_error(char *pMessage,...)
{
	int		line;
	char	*pText;

	strresGetErrorData(&line, &pText);
	DBERROR(("RES file parse error:\n%s at line %d\nToken: %d, Text: '%s'\n",
			  pMessage, line, strres_char, pText));
}



