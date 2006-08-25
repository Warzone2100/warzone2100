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

int strres_lex (void);

/*
 * A simple error reporting routine
 */

void strres_error(const char *pMessage,...)
{
	int		line;
	char	*pText;

	strresGetErrorData(&line, &pText);
	debug( LOG_ERROR, "STRRES file parse error:\n%s at line %d\nText: '%s'\n", pMessage, line, pText );
	abort();
}

%}

%name-prefix="strres_"

%union {
	STRING  *sval;
}

	/* value tokens */
%token <sval> TEXT_T
%token <sval> QTEXT_T			/* Text with double quotes surrounding it */

%%

file:			line
			|	file line
			;

line:			TEXT_T QTEXT_T
							{
								/* Pass the text string to the string manager */
								if (!strresStoreString(psCurrRes, $1, $2))
								{
									YYABORT;
								}
							}
			;

%%

