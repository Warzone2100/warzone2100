/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007  Warzone Resurrection Project

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

#ifndef __INCLUDED_FRAMEWORK_STRLFUNCS_H__
#define __INCLUDED_FRAMEWORK_STRLFUNCS_H__

#include <string.h>
#include <stddef.h>

/*
 *  The "original" strlcpy() and strlcat() functions will return the amount
 *  of characters the destination string would be long, if no truncation has
 *  occurred.
 */

/** A safer variant of \c strncpy and its completely unsafe variant \c strcpy.
 *  This function will guarantee that (as long as the destination buffer is at
 *  least 1 byte large), the destination buffer will hold a valid C string
 *  (with zero termination).
 *  \param dest a pointer to the destination buffer
 *  \param src the source string to copy into the \c dest buffer
 *  \param size the buffer size (in bytes) of buffer \c dest
 */
static inline size_t strlcpy(char* dest, const char* src, size_t size)
{
	strncpy(dest, src, size - 1);

	// Guarantee to nul-terminate
	dest[size - 1] = '\0';

	return strlen(src);
}

/** A safer variant of \c strncpy and its completely unsafe variant \c strcpy.
 *  This function will guarantee that (as long as the destination buffer is at
 *  least 1 byte large), the destination buffer will hold a valid C string
 *  (with zero termination).
 *  \param dest a pointer to the destination buffer
 *  \param src the source string to concatenate to the \c dest buffer
 *  \param size the buffer size (in bytes) of buffer \c dest
 */
static inline size_t strlcat(char* dest, const char* src, size_t size)
{
	size_t dest_len = strlen(dest);

	strncat(dest, src, size - dest_len - 1);

	return strlen(src) + dest_len;
}

#endif // __INCLUDED_FRAMEWORK_STRLFUNCS_H__
