%{
/*
 * StrRes.y
 *
 * Yacc file for parsing String Resource files
 */

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include "types.h"
#include "debug.h"
#include "mem.h"
#include "heap.h"
#include "treap.h"
#include "strres.h"
#include "strresly.h"

/*
 * A simple error reporting routine
 */

int strres_error(const char *pMessage,...)
{
	int		line;
	char	*pText;

	strresGetErrorData(&line, &pText);
	DBERROR(("STRRES file parse error:\n%s at line %d\nText: '%s'\n",
			  pMessage, line, pText));
}

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

