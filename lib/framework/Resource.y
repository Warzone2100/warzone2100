%{
/*
 * resource.y
 *
 * Yacc file for parsing RES files
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

// directory printfs
#define DEBUG_GROUP0
// file printfs
//#define DEBUG_GROUP1
#include "types.h"
#include "debug.h"
#include "ResLY.h"
#include "FrameResource.h"

/* Turn off a couple of warnings that the yacc generated code gives */
#pragma warning ( disable : 4305 4102)

%}

%union {
	STRING  *sval;
}

	/* value tokens */
%token <sval> TEXT
%token <sval> QTEXT			/* Text with double quotes surrounding it */

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

dir_line:			DIRECTORY QTEXT		{
											UDWORD len;

											// set a new input directory
											DBP0(("directory: %s\n", $2));
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


file_line:			FILETOKEN TEXT QTEXT
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


/*
 * A simple error reporting routine
 */

void res_error(char *pMessage,...)
{
	int		line;
	char	*pText;

	resGetErrorData(&line, &pText);
	DBERROR(("RES file parse error:\n%s at line %d\nToken: %d, Text: '%s'\n",
			  pMessage, line, res_char, pText));
}



