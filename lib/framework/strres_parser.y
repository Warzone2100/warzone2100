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
/** @file
 *
 *  Parser for string resource files
 */

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/strresly.h"

extern int strres_lex (void);

void yyerror(const char* msg)
{
	int		line;
	char	*pText;

	strresGetErrorData(&line, &pText);
	debug(LOG_ERROR, "STRRES file parse error:\n%s at line %d\nText: '%s'", msg, line, pText);
	abort();
}

%}

%name-prefix="strres_"

%union {
	char  *sval;
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
            | TEXT_T '_' '(' QTEXT_T ')'
							{
								/* Pass the text string to the string manager */
								if (!strresStoreString(psCurrRes, $1, gettext($4)))
								{
									YYABORT;
								}
							}

			;

%%

