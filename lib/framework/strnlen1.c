/*
	This file is part of Warzone 2100.
	Find the length of STRING + 1, but scan at most MAXLEN bytes.
	Copyright (C) 2005-2006  Free Software Foundation, Inc.

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
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
	MA 02110-1301, USA.

	$Revision$
	$Id$
	$HeadURL$
*/

/* Specification.  */
#include "strnlen1.h"

#include <string.h>

/* Find the length of STRING + 1, but scan at most MAXLEN bytes.
   If no '\0' terminator is found in that many characters, return MAXLEN.  */
/* This is the same as strnlen (string, maxlen - 1) + 1.  */
size_t strnlen1(const char* string, size_t maxlen)
{
	// Find the first NUL char
	const char* end = memchr (string, '\0', maxlen);

	if (end != NULL)
		return end - string + 1;
	else
		return maxlen;
}
