/*
	This file is part of Warzone 2100.
	Copyright (C) 1992-2007  Trolltech ASA.
	Copyright (C) 2005-2013  Warzone 2100 Project

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
#include "stdio_ext.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#if defined(WZ_CC_MSVC)
int vslcatprintf(char* str, size_t size, const char* format, va_list ap)
{
	size_t str_len;

	if (str == NULL
	 || size == 0)
	{
		return vsnprintf(NULL, 0, format, ap);
	}
	
	str_len = strlen(str);

	assert(str_len < size);

	return vsnprintf(&str[str_len], size - str_len, format, ap);
}


int slcatprintf(char* str, size_t size, const char* format, ...)
{
	va_list ap;
	int count;

	va_start(ap, format);
		count = vslcatprintf(str, size, format, ap);
	va_end(ap);

	return count;
}


int vasprintf(char** strp, const char* format, va_list ap)
{
	int count;
	va_list aq;

	va_copy(aq, ap);

	// Find out how long the resulting string is
	count = vsnprintf(NULL, 0, format, aq);
	va_end(aq);

	if (count == 0)
	{
		*strp = strdup("");
		return 0;
	}
	else if (count < 0)
	{
		// Something went wrong, so return the error code (probably still requires checking of "errno" though)
		return -1;
	}

	assert(strp != NULL);

	// Allocate memory for our string
	*strp = (char *)malloc(count + 1);
	if (*strp == NULL)
	{
		debug(LOG_FATAL, "Out of memory!");
		abort();
		return -1;
	}

	// Do the actual printing into our newly created string
	return vsprintf(*strp, format, ap);
}


int asprintf(char** strp, const char* format, ...)
{
	va_list ap;
	int count;

	va_start(ap, format);
		count = vasprintf(strp, format, ap);
	va_end(ap);

	return count;
}
#endif


#if defined(WZ_CC_MSVC)
int wz_vsnprintf(char* str, size_t size, const char* format, va_list ap)
{
	int count;
	va_list aq;

	va_copy(aq, ap);

	// Find out how long the resulting string is
	count = _vscprintf(format, aq);
	va_end(aq);

	if (count >= 0
	 && str != NULL)
	{
		// Perfrom the actual string formatting
		_vsnprintf_s(str, size, _TRUNCATE, format, ap);
	}

	// Return the amount of characters that would be written if _no_ truncation occurred
	return count;
}


int wz_snprintf(char* str, size_t size, const char* format, ...)
{
	va_list ap;
	int count;

	va_start(ap, format);
		count = vsnprintf(str, size, format, ap);
	va_end(ap);

	return count;
}
#endif

int vasprintfNull(char** strp, const char* format, va_list ap)
{
	int count = vasprintf(strp, format, ap);

	if (count == -1)  // If count == -1, strp is currently undefined.
		strp = NULL;

	return count;
}

int asprintfNull(char** strp, const char* format, ...)
{
	va_list ap;
	int count;

	va_start(ap, format);
		count = vasprintfNull(strp, format, ap);
	va_end(ap);

	return count;
}
