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

	$Revision$
	$Id$
	$HeadURL$
*/

#ifndef __INCLUDE_LIB_FRAMEWORK_UTF8_H__
#define __INCLUDE_LIB_FRAMEWORK_UTF8_H__

#include "frame.h"

/** Used to store a UTF-32 character in
 */
typedef uint32_t utf_32_char;

/** Used to store a UTF-16 character in (this is <em>not</em> necessarily a
 *  full Unicode codepoint)
 */
typedef uint16_t utf_16_char;

/** Determines the amount of unicode codepoints in a UTF-8 encoded string
 *  \param utf8_string the UTF-8 encoded string to count
 *  \return the amount of codepoints found in the UTF-8 string
 */
size_t utf8_character_count(const char* utf8_string);

/** Determines the amount of octets required to store this string if it's encoded as UTF-8
 *  \param unicode_string the UTF-32 encoded string to determine the UTF-8 encoded length of
 *  \return the size required to hold unicode_string if encoded in UTF-8
 */
size_t utf32_utf8_buffer_length(const utf_32_char* unicode_string);

/** Determines the amount of octets required to store this string if it's encoded as UTF-8
 *  \param unicode_string the UTF-16 encoded string to determine the UTF-8 encoded length of
 *  \return the size required to hold unicode_string if encoded in UTF-8
 */
size_t utf16_utf8_buffer_length(const utf_16_char* unicode_string);

/** Encodes a UTF-32 encoded unicode string to a UTF-8 encoded string
 *  \param unicode_string the UTF-32 encoded unicode string to encode into UTF-8
 *  \return a UTF-8 encoded unicode nul terminated string (use free() to deallocate it)
 */
char* utf8_encode_utf32(const utf_32_char* unicode_string);

/** Decodes a UTF-8 encode string to a UTF-32 encoded string (native endianess)
 *  \param utf8_string a UTF-8 encoded nul terminated string
 *  \return a UTF-32 encoded unicode nul terminated string (use free() to deallocate it)
 */
utf_32_char* utf8_decode_utf32(const char* utf8_string);

/** Encodes a UTF-16 encoded unicode string to a UTF-8 encoded string
 *  \param unicode_string the UTF-16 encoded unicode string to encode into UTF-8
 *  \return a UTF-8 encoded unicode nul terminated string (use free() to deallocate it)
 */
char* utf8_encode_utf16(const utf_16_char* unicode_string);

/** Decodes a UTF-8 encode string to a UTF-16 encoded string (native endianess)
 *  \param utf8_string a UTF-8 encoded nul terminated string
 *  \return a UTF-16 encoded unicode nul terminated string (use free() to deallocate it)
 */
utf_16_char* utf8_decode_utf16(const char* utf8_string);

/** Determines the amount of characters found in a UTF-32 encoded string
 *  This function is similar to strlen, except that it operates on a string of utf_32_char's
 *  \param unicode_string the string to determine the length of
 *  \return the amount of characters found
 */
size_t utf32_strlen(const utf_32_char* unicode_string);

#endif // __INCLUDE_LIB_FRAMEWORK_UTF8_H__
