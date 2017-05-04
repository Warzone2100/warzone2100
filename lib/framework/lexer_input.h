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
#ifndef __INCLUDED_LIB_FRAMEWORK_LEXER_INPUT_H__
#define __INCLUDED_LIB_FRAMEWORK_LEXER_INPUT_H__

#include <physfs.h>
#include <stdio.h>

enum lexinput_type
{
	LEXINPUT_STDIO,
	LEXINPUT_PHYSFS,
	LEXINPUT_BUFFER,
};

struct lexerinput_t
{
	union
	{
		FILE *stdiofile;
		PHYSFS_file *physfsfile;
		struct
		{
			const char *begin;
			const char *end;
		} buffer;
	} input;
	lexinput_type type;
};

#ifdef YY_EXTRA_TYPE
# undef YY_EXTRA_TYPE
#endif

#define YY_EXTRA_TYPE lexerinput_t *

int lexer_input(lexerinput_t *input, char *buf, size_t max_size, int nullvalue);

#define YY_INPUT(buf, result, max_size) \
	do \
	{ \
		result = lexer_input(yyextra, buf, max_size, YY_NULL); \
	} while(0)

#endif // __INCLUDED_LIB_FRAMEWORK_LEXER_INPUT_H__
