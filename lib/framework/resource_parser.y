%{
/*
 * resource.y
 *
 * Yacc file for parsing RES files
 */

int res_lex (void);

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

// directory printfs
#define DEBUG_GROUP0

#include <string.h>
#include <stdlib.h>

#include "lib/framework/frame.h"
#include "lib/framework/resly.h"

/*
 * A simple error reporting routine
 */

void res_error(const char *pMessage,...)
{
	int	line;
	char	*pText;

	resGetErrorData(&line, &pText);
	debug( LOG_ERROR, "RES file parse error:\n%s at line %d\nText: '%s'\n", pMessage, line, pText );
	abort();
}

%}

%name-prefix="res_"

%union {
	STRING  *sval;
}

	/* value tokens */
%token <sval> TEXT_T
%token <sval> QTEXT_T			/* Text with double quotes surrounding it */

	/* keywords */
%token DIRECTORY
%token FILETOKEN

%%

res_file:			res_line
				|	res_file res_line
				;

res_line:			dir_line
				|	file_line
				;

dir_line:			DIRECTORY QTEXT_T		{
											UDWORD len;

											// set a new input directory
											debug( LOG_NEVER, "directory: %s\n", $2 );
											if ($2[1] == ':' ||
												$2[0] == '\\')
											{
												// the new dir is rooted
												strcpy(aCurrResDir, $2);
											}
											else
											{
												strcpy(aCurrResDir, aResDir);
												strcpy(aCurrResDir + strlen(aResDir),
													   $2);
											}
											if (strlen($2) > 0)
											{
												// Add a trailing '\\'
												len = strlen(aCurrResDir);
												aCurrResDir[len] = '\\';
												aCurrResDir[len+1] = 0;
//												DBP0(("aCurrResDir: %s\n", aCurrResDir));
											}
										}
				;


file_line:			FILETOKEN TEXT_T QTEXT_T
										{
											/* load a data file */
											DBP1(("file: %s %s\n", $2, $3));
											if (!resLoadFile($2, $3))
											{
												YYABORT;
											}
										}
				;

%%




