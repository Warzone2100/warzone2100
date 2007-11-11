/*
	This file is part of Warzone 2100.
	Copyright (C) 1992-2007  Trolltech ASA.
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

#include "frame.h"
#include "printf_ext.h"
#include <stdio.h>

#if defined(WZ_CC_MSVC)
int vsnprintf(char* str, size_t size, const char* format, va_list ap)
{
	int count;

	// Find out how long the resulting string is
	count = _vscprintf(format, ap);

	if (count > 0
	 && str != NULL)
	{
		// Perfrom the actual string formatting
		_vsnprintf_s(str, size, _TRUNCATE, format, ap);
	}

	// Return the amount of characters that would be written if _no_ truncation occurred
	return count;
}

int snprintf(char* str, size_t size, const char* format, ...)
{
	va_list ap;
	int count;

	va_start(ap, format);
		count = vsnprintf(str, size, format, ap);
	va_end(ap);

	return count;
}
#endif
