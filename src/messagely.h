/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Giel van Schijndel
	Copyright (C) 2008-2011  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_MESSAGELY_H__
#define __INCLUDED_SRC_MESSAGELY_H__

#include "lib/framework/lexer_input.h"

extern int message_parse(void* ppsViewData);
extern void message_set_extra(YY_EXTRA_TYPE user_defined);
extern int message_lex_destroy(void);

extern int message_lex(void);
extern int message_get_lineno(void);
extern char* message_get_text(void);

extern void message_error(const char* msg);

#endif // __INCLUDED_SRC_MESSAGELY_H__
