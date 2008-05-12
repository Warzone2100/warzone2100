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
/*! \file strresly.h
 *  * \brief Interface to the string resource lex and yacc functions.
 */
#ifndef _strresly_h
#define _strresly_h

#include "lib/framework/lexer_input.h"

/* Maximum number of TEXT items in any one Yacc rule */
#define TEXT_BUFFERS	10

/* The string resource currently being loaded */
extern STR_RES	*psCurrRes;

/* Set the current input buffer for the lexer - used by strresLoad */
extern void strres_set_extra(YY_EXTRA_TYPE user_defined);

/* Give access to the line number and current text for error messages */
extern void strresGetErrorData(int *pLine, char **ppText);

/* Call the yacc parser */
extern int strres_parse(void);

/* Destroy the lexer */
extern int strres_lex_destroy(void);

void strres_error(const char* msg);

/* Store a string */
extern BOOL strresStoreString(STR_RES *psRes, char *pID, const char *pString);

#endif

