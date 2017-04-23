/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

/* Set the current input buffer for the lexer - used by strresLoad */
void strres_set_extra(YY_EXTRA_TYPE user_defined);

/* Call the yacc parser */
WZ_DECL_NONNULL(1) int strres_parse(void *psStrRes);

/* Destroy the lexer */
int strres_lex_destroy();

WZ_DECL_NONNULL(1) void strres_error(const char *msg);

/* Store a string */
WZ_DECL_NONNULL(1, 2, 3) bool strresStoreString(struct STR_RES *psRes, const char *pID, const char *pString);

#endif
