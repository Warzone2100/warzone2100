/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007-2017  Warzone 2100 Project

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

	$Revision$
	$Id$
	$HeadURL$
*/

#ifndef __INCLUDE_LIB_FRAMEWORK_UTF8_H__
#define __INCLUDE_LIB_FRAMEWORK_UTF8_H__

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include "types.h"

#include <string>
#include <cstdlib>

/** Used to store a UTF-32 character in
 */
typedef uint32_t utf_32_char;

/** Used to store a UTF-16 character in (this is <em>not</em> necessarily a
 *  full Unicode codepoint)
 */
typedef uint16_t utf_16_char;

/** Decodes a single Unicode character from the given UTF-16 string.
 *
 *  \param utf16_char     Points to a character string that should contain at
 *                        least one valid UTF-16 character sequence.
 *  \param[out] next_char Will be modified to point to the first character
 *                        following the UTF-16 character sequence.
 *
 *  \return The Unicode character encoded as UTF-32 with native endianness.
 */
utf_32_char UTF16DecodeChar(const utf_16_char *utf16_char, const utf_16_char **next_char);

/** Decodes a single Unicode character from the given UTF-8 string.
 *
 *  \param utf8_char      Points to a character string that should contain at
 *                        least one valid UTF-8 character sequence.
 *  \param[out] next_char Will be modified to point to the first character
 *                        following the UTF-8 character sequence.
 *
 *  \return The Unicode character encoded as UTF-32 with native endianness.
 */
utf_32_char UTF8DecodeChar(const char *utf8_char, const char **next_char);

/** Determines the amount of unicode codepoints in a UTF-8 encoded string
 *  \param utf8_string the UTF-8 encoded string to count
 *  \return the amount of codepoints found in the UTF-8 string
 */
size_t UTF8CharacterCount(const char *utf8_string);

size_t UTF16CharacterCount(const uint16_t *utf16);

/** Encodes a UTF-16 encoded unicode string to a UTF-8 encoded string
 *  \param unicode_string the UTF-16 encoded unicode string to encode into UTF-8
 *  \param[out] nbytes the number of bytes allocated, may be NULL
 *  \return a UTF-8 encoded unicode nul terminated string (use free() to deallocate it)
 */
char *UTF16toUTF8(const utf_16_char *unicode_string, size_t *nbytes);

/** Decodes a UTF-8 encoded string to a UTF-16 encoded string (native endianess)
 *  \param utf8_string a UTF-8 encoded nul terminated string
 *  \param[out] nbytes the number of bytes allocated, may be NULL
 *  \return a UTF-16 encoded unicode nul terminated string (use free() to deallocate it)
 */
utf_16_char *UTF8toUTF16(const char *utf8_string, size_t *nbytes);

const char *UTF8CharacterAtOffset(const char *utf8_string, size_t index);
const utf_16_char *UTF16CharacterAtOffset(const utf_16_char *utf16_string, size_t index);

/** Encodes a UTF-32 string to a UTF-8 encoded string
 *  \param unicode_string the UTF-32 string to encode into UTF-8
 *  \param[out] nbytes the number of bytes allocated, may be NULL
 *  \return a UTF-8 encoded unicode nul terminated string (use free() to deallocate it)
 */
char *UTF32toUTF8(const utf_32_char *unicode_string, size_t *nbytes);

/** Decodes a UTF-8 encoded string to a UTF-32 string
 *  \param utf8_string a UTF-8 encoded nul terminated string
 *  \param[out] nbytes the number of bytes allocated, may be NULL
 *  \return a UTF-32 nul terminated string (use free() to deallocate it)
 */
utf_32_char *UTF8toUTF32(const char *utf8_string, size_t *nbytes);

#endif // __INCLUDE_LIB_FRAMEWORK_UTF8_H__
