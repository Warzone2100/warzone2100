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
 * resource.y
 *
 * Yacc file for parsing RES files
 */

extern int res_lex(void);
extern int res_get_lineno(void);
extern char* res_get_text(void);

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

// directory printfs
#define DEBUG_GROUP0

#include "lib/framework/frame.h"
#include <string.h>
#include <stdlib.h>

#include "lib/framework/frameresource.h"
#include "lib/framework/resly.h"

extern void yyerror(const char* msg);
void yyerror(const char* msg)
{
	debug(LOG_ERROR, "RES file parse error:\n%s at line %d\nText: '%s'\n", msg, res_get_lineno(), res_get_text());
}

%}

%name-prefix="res_"

%union {
	char  *sval;
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
												$2[0] == '/')
											{
												// the new dir is rooted
												strlcpy(aCurrResDir, $2, sizeof(aCurrResDir));
											}
											else
											{
												strlcpy(aCurrResDir, aResDir, sizeof(aCurrResDir));
												strlcat(aCurrResDir, $2, sizeof(aCurrResDir));
											}
											if (strlen($2) > 0)
											{
												// Add a trailing '/'
												len = strlen(aCurrResDir);
												aCurrResDir[len] = '/';
												aCurrResDir[len+1] = 0;
//												debug( LOG_NEVER, "aCurrResDir: %s\n", aCurrResDir);
											}
										}
				;


file_line:			FILETOKEN TEXT_T QTEXT_T
										{
											/* load a data file */
											debug( LOG_NEVER, "file: %s %s\n", $2, $3);
											if (!resLoadFile($2, $3))
											{
												YYABORT;
											}
										}
				;

%%




