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
#include "frame.h"
#include "lexer_input.h"
#include "physfs_ext.h"

int lexer_input(lexerinput_t *input, char *buf, size_t max_size, int nullvalue)
{
	switch (input->type)
	{
	case LEXINPUT_STDIO:
		if (feof(input->input.stdiofile))
		{
			buf[0] = EOF;
			return nullvalue;
		}
		else
		{
			return fread(buf, 1, max_size, input->input.stdiofile);
		}
		break;

	case LEXINPUT_PHYSFS:
		if (PHYSFS_eof(input->input.physfsfile))
		{
			buf[0] = EOF;
			return nullvalue;
		}
		else
		{
			int result = WZ_PHYSFS_readBytes(input->input.physfsfile, buf, max_size);
			if (result == -1)
			{
				buf[0] = EOF;
				return nullvalue;
			}
			return result;
		}
		break;

	case LEXINPUT_BUFFER:
		if (input->input.buffer.begin != input->input.buffer.end)
		{
			buf[0] = *input->input.buffer.begin++;
			return 1;
		}
		else
		{
			buf[0] = EOF;
			return nullvalue;
		}
		break;
	}

	ASSERT(!"Invalid input type!", "Invalid input type used for lexer (numeric value: %u)", (unsigned int)input->type);
	return nullvalue;
}
