/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007-2015  Warzone 2100 Project

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

/** \file
 *  Functions to convert between different Unicode Transformation Formats (UTF for short)
 */

#include "utf.h"
#include <assert.h>
#include <stdlib.h>

#if defined(LIB_COMPILE)
# define ASSERT(expr, ...) (assert(expr))
# define debug(part, ...) ((void)0)
#else
# include "debug.h"
#endif

// Assert that non-starting octets are of the form 10xxxxxx
#define ASSERT_NON_START_OCTET(octet) \
	assert((octet & 0xC0) == 0x80 && "invalid non-start UTF-8 octet")

// Assert that starting octets are either of the form 0xxxxxxx (ASCII) or 11xxxxxx
#define ASSERT_START_OCTECT(octet) \
	assert((octet & 0x80) == 0x00 || (octet & 0xC0) == 0xC0 || !"invalid starting UTF-8 octet")

// Assert that hexadect (16bit sequence) 1 of UTF-16 surrogate pair sequences are of the form 110110XXXXXXXXXX
#define ASSERT_START_HEXADECT(hexadect) \
	assert(((hexadect) & 0xD800) == 0xD800 && "invalid first UTF-16 hexadect")

// Assert that hexadect (16bit sequence) 2 of UTF-16 surrogate pair sequences are of the form 110111XXXXXXXXXX
#define ASSERT_FINAL_HEXADECT(hexadect) \
	assert(((hexadect) & 0xDC00) == 0xDC00 && "invalid first UTF-16 hexadect")

utf_32_char UTF8DecodeChar(const char *utf8_char, const char **next_char)
{
	utf_32_char decoded = '\0';
	*next_char = utf8_char;

	ASSERT_START_OCTECT(*utf8_char);

	// first octect: 0xxxxxxx: 7 bit (ASCII)
	if ((*utf8_char & 0x80) == 0x00)
	{
		// 1 byte long encoding
		decoded = *((*next_char)++);
	}
	// first octect: 110xxxxx: 11 bit
	else if ((*utf8_char & 0xe0) == 0xc0)
	{
		// 2 byte long encoding
		ASSERT_NON_START_OCTET(utf8_char[1]);

		decoded  = (*((*next_char)++) & 0x1f) << 6;
		decoded |= (*((*next_char)++) & 0x3f) << 0;
	}
	// first octect: 1110xxxx: 16 bit
	else if ((*utf8_char & 0xf0) == 0xe0)
	{
		// 3 byte long encoding
		ASSERT_NON_START_OCTET(utf8_char[1]);
		ASSERT_NON_START_OCTET(utf8_char[2]);

		decoded  = (*((*next_char)++) & 0x0f) << 12;
		decoded |= (*((*next_char)++) & 0x3f) << 6;
		decoded |= (*((*next_char)++) & 0x3f) << 0;
	}
	// first octect: 11110xxx: 21 bit
	else if ((*utf8_char & 0xf8) == 0xf0)
	{
		// 4 byte long encoding
		ASSERT_NON_START_OCTET(utf8_char[1]);
		ASSERT_NON_START_OCTET(utf8_char[2]);
		ASSERT_NON_START_OCTET(utf8_char[3]);

		decoded  = (*((*next_char)++) & 0x07) << 18;
		decoded |= (*((*next_char)++) & 0x3f) << 12;
		decoded |= (*((*next_char)++) & 0x3f) << 6;
		decoded |= (*((*next_char)++) & 0x3f) << 0;
	}
	else
	{
		// apparently this character uses more than 21 bit
		// this decoder is not developed to cope with those
		// characters so error out
		ASSERT(!"out-of-range UTF-8 character", "this UTF-8 character is too large (> 21bits) for this UTF-8 decoder and too large to be a valid Unicode codepoint");
	}

	return decoded;
}

size_t UTF8CharacterCount(const char *utf8_string)
{
	size_t length = 0;

	while (*utf8_string != '\0')
	{
		UTF8DecodeChar(utf8_string, &utf8_string);

		++length;
	}

	return length;
}

size_t UTF16CharacterCount(const uint16_t *utf16)
{
	size_t length = 0;

	while (*utf16)
	{
		UTF16DecodeChar(utf16, &utf16);

		++length;
	}

	return length;
}

static size_t unicode_utf8_char_length(const utf_32_char unicode_char)
{
	// an ASCII character, which uses 7 bit at most, which is one byte in UTF-8
	if (unicode_char < 0x00000080)
	{
		return 1;    // stores 7 bits
	}
	else if (unicode_char < 0x00000800)
	{
		return 2;    // stores 11 bits
	}
	else if (unicode_char < 0x00010000)
	{
		return 3;    // stores 16 bits
	}
	/* This encoder can deal with < 0x00200000, but Unicode only ranges
	 * from 0x0 to 0x10FFFF. Thus we don't accept anything else.
	 */
	else if (unicode_char < 0x00110000)
	{
		return 4;    // stores 21 bits
	}

	/* Apparently this character lies outside the 0x0 - 0x10FFFF
	 * Unicode range, so don't accept it.
	 */
	ASSERT(!"out-of-range Unicode codepoint", "This Unicode codepoint is too large (%u > 0x10FFFF) to be a valid Unicode codepoint", (unsigned int)unicode_char);

	// Dummy value to prevent warnings about missing return from function
	return 0;
}

char *UTF8CharacterAtOffset(const char *utf8_string, size_t index)
{
	while (*utf8_string != '\0'
	       && index != 0)
	{
		// Move to the next character
		UTF8DecodeChar(utf8_string, &utf8_string);

		--index;
	}

	if (*utf8_string == '\0')
	{
		return NULL;
	}

	return (char *)utf8_string;
}

/** Encodes a single Unicode character to a UTF-8 encoded string.
 *
 *  \param unicode_char A UTF-32 encoded Unicode codepoint that will be encoded
 *                      into UTF-8. This should be a valid Unicode codepoint
 *                      (i.e. ranging from 0x0 to 0x10FFFF inclusive).
 *  \param out_char     Points to the position in a buffer where the UTF-8
 *                      encoded character can be stored.
 *
 *  \return A pointer pointing to the first byte <em>after</em> the encoded
 *          UTF-8 sequence. This can be used as the \c out_char parameter for a
 *          next invocation of encode_utf8_char().
 */
static char *encode_utf8_char(const utf_32_char unicode_char, char *out_char)
{
	char *next_char = out_char;

	// 7 bits
	if (unicode_char < 0x00000080)
	{
		*(next_char++) = unicode_char;
	}
	// 11 bits
	else if (unicode_char < 0x00000800)
	{
		// 0xc0 provides the counting bits: 110
		// then append the 5 most significant bits
		*(next_char++) = 0xc0 | (unicode_char >> 6);
		// Put the next 6 bits in a byte of their own
		*(next_char++) = 0x80 | (unicode_char & 0x3f);
	}
	// 16 bits
	else if (unicode_char < 0x00010000)
	{
		// 0xe0 provides the counting bits: 1110
		// then append the 4 most significant bits
		*(next_char++) = 0xe0 | (unicode_char >> 12);
		// Put the next 12 bits in two bytes of their own
		*(next_char++) = 0x80 | ((unicode_char >> 6) & 0x3f);
		*(next_char++) = 0x80 | (unicode_char & 0x3f);
	}
	// 21 bits
	/* This encoder can deal with < 0x00200000, but Unicode only ranges
	 * from 0x0 to 0x10FFFF. Thus we don't accept anything else.
	 */
	else if (unicode_char < 0x00110000)
	{
		// 0xf0 provides the counting bits: 11110
		// then append the 3 most significant bits
		*(next_char++) = 0xf0 | (unicode_char >> 18);
		// Put the next 18 bits in three bytes of their own
		*(next_char++) = 0x80 | ((unicode_char >> 12) & 0x3f);
		*(next_char++) = 0x80 | ((unicode_char >> 6) & 0x3f);
		*(next_char++) = 0x80 | (unicode_char & 0x3f);
	}
	else
	{
		/* Apparently this character lies outside the 0x0 - 0x10FFFF
		 * Unicode range, so don't accept it.
		 */
		ASSERT(!"out-of-range Unicode codepoint", "This Unicode codepoint is too large (%u > 0x10FFFF) to be a valid Unicode codepoint", (unsigned int)unicode_char);
	}

	return next_char;
}

utf_32_char UTF16DecodeChar(const utf_16_char *utf16_char, const utf_16_char **next_char)
{
	utf_32_char decoded;
	*next_char = utf16_char;

	// Are we dealing with a surrogate pair
	if (*utf16_char >= 0xD800
	    && *utf16_char <= 0xDFFF)
	{
		ASSERT_START_HEXADECT(utf16_char[0]);
		ASSERT_FINAL_HEXADECT(utf16_char[1]);

		decoded  = (*((*next_char)++) & 0x3ff) << 10;
		decoded |= *((*next_char)++) & 0x3ff;

		decoded += 0x10000;
	}
	// Not a surrogate pair, so it's a valid Unicode codepoint right away
	else
	{
		decoded = *((*next_char)++);
	}

	return decoded;
}

/** Encodes a single Unicode character to a UTF-16 encoded string.
 *
 *  \param unicode_char A UTF-32 encoded Unicode codepoint that will be encoded
 *                      into UTF-16. This should be a valid Unicode codepoint
 *                      (i.e. ranging from 0x0 to 0x10FFFF inclusive).
 *  \param out_char     Points to the position in a buffer where the UTF-16
 *                      encoded character can be stored.
 *
 *  \return A pointer pointing to the first byte <em>after</em> the encoded
 *          UTF-16 sequence. This can be used as the \c out_char parameter for a
 *          next invocation of encode_utf16_char().
 */
static utf_16_char *encode_utf16_char(const utf_32_char unicode_char, utf_16_char *out_char)
{
	utf_16_char *next_char = out_char;

	// 16 bits
	if (unicode_char < 0x10000)
	{
		*(next_char++) = unicode_char;
	}
	else if (unicode_char < 0x110000)
	{
		const utf_16_char v = unicode_char - 0x10000;

		*(next_char++) = 0xD800 | (v >> 10);
		*(next_char++) = 0xDC00 | (v & 0x3ff);

		ASSERT_START_HEXADECT(out_char[0]);
		ASSERT_FINAL_HEXADECT(out_char[1]);
	}
	else
	{
		/* Apparently this character lies outside the 0x0 - 0x10FFFF
		 * Unicode range, and UTF-16 cannot cope with that, so error
		 * out.
		 */
		ASSERT(!"out-of-range Unicode codepoint", "This Unicode codepoint is too large (%u > 0x10FFFF) to be a valid Unicode codepoint", (unsigned int)unicode_char);
	}

	return next_char;
}

static size_t utf16_utf8_buffer_length(const utf_16_char *unicode_string)
{
	const utf_16_char *curChar = unicode_string;

	// Determine length of string (in octets) when encoded in UTF-8
	size_t length = 0;

	while (*curChar)
	{
		length += unicode_utf8_char_length(UTF16DecodeChar(curChar, &curChar));
	}

	return length;
}

char *UTF16toUTF8(const utf_16_char *unicode_string, size_t *nbytes)
{
	const utf_16_char *curChar;

	const size_t utf8_length = utf16_utf8_buffer_length(unicode_string);

	// Allocate memory to hold the UTF-8 encoded string (plus a terminating nul char)
	char *utf8_string = (char *)malloc(utf8_length + 1);
	char *curOutPos = utf8_string;

	if (utf8_string == NULL)
	{
		debug(LOG_ERROR, "Out of memory");
		return NULL;
	}

	curChar = unicode_string;
	while (*curChar)
	{
		curOutPos = encode_utf8_char(UTF16DecodeChar(curChar, &curChar), curOutPos);
	}

	// Terminate the string with a nul character
	utf8_string[utf8_length] = '\0';

	// Set the number of bytes allocated
	if (nbytes)
	{
		*nbytes = utf8_length + 1;
	}

	return utf8_string;
}

static size_t utf8_as_utf16_buf_size(const char *utf8_string)
{
	const char *curChar = utf8_string;

	size_t length = 0;
	while (*curChar != '\0')
	{
		const utf_32_char unicode_char = UTF8DecodeChar(curChar, &curChar);

		if (unicode_char < 0x10000)
		{
			length += 1;
		}
		else if (unicode_char < 0x110000)
		{
			length += 2;
		}
		else
		{
			/* Apparently this character lies outside the 0x0 - 0x10FFFF
			 * Unicode range, and UTF-16 cannot cope with that, so error
			 * out.
			 */
			ASSERT(!"out-of-range Unicode codepoint", "This Unicode codepoint too large (%u > 0x10FFFF) for the UTF-16 encoding", (unsigned int)unicode_char);
		}
	}

	return length;
}

utf_16_char *UTF8toUTF16(const char *utf8_string, size_t *nbytes)
{
	const char *curChar = utf8_string;
	const size_t unicode_length = utf8_as_utf16_buf_size(utf8_string);

	// Allocate memory to hold the UTF-16 encoded string (plus a terminating nul)
	utf_16_char *unicode_string = (utf_16_char *)malloc(sizeof(utf_16_char) * (unicode_length + 1));
	utf_16_char *curOutPos = unicode_string;

	if (unicode_string == NULL)
	{
		debug(LOG_ERROR, "Out of memory");
		return NULL;
	}

	while (*curChar != '\0')
	{
		curOutPos = encode_utf16_char(UTF8DecodeChar(curChar, &curChar), curOutPos);
	}

	// Terminate the string with a nul
	unicode_string[unicode_length] = '\0';

	// Set the number of bytes allocated
	if (nbytes)
	{
		*nbytes = sizeof(utf_16_char) * (unicode_length + 1);
	}

	return unicode_string;
}

utf_16_char *UTF16CharacterAtOffset(const utf_16_char *utf16_string, size_t index)
{
	while (*utf16_string != '\0'
	       && index != 0)
	{
		// Move to the next character
		UTF16DecodeChar(utf16_string, &utf16_string);

		--index;
	}

	if (*utf16_string == '\0')
	{
		return NULL;
	}

	return (utf_16_char *)utf16_string;
}


static size_t utf32_utf8_buffer_length(const utf_32_char *unicode_string)
{
	const utf_32_char *curChar;

	// Determine length of string (in octets) when encoded in UTF-8
	size_t length = 0;
	for (curChar = unicode_string; *curChar != '\0'; ++curChar)
	{
		length += unicode_utf8_char_length(*curChar);
	}

	return length;
}

char *UTF32toUTF8(const utf_32_char *unicode_string, size_t *nbytes)
{
	const utf_32_char *curChar;

	const size_t utf8_length = utf32_utf8_buffer_length(unicode_string);

	// Allocate memory to hold the UTF-8 encoded string (plus a terminating nul char)
	char *utf8_string = (char *)malloc(utf8_length + 1);
	char *curOutPos = utf8_string;

	if (utf8_string == NULL)
	{
		debug(LOG_ERROR, "Out of memory");
		return NULL;
	}

	for (curChar = unicode_string; *curChar != 0; ++curChar)
	{
		curOutPos = encode_utf8_char(*curChar, curOutPos);
	}

	// Terminate the string with a nul character
	utf8_string[utf8_length] = '\0';

	// Set the number of bytes allocated
	if (nbytes)
	{
		*nbytes = utf8_length + 1;
	}

	return utf8_string;
}

utf_32_char *UTF8toUTF32(const char *utf8_string, size_t *nbytes)
{
	const char *curChar = utf8_string;
	const size_t unicode_length = UTF8CharacterCount(utf8_string);

	// Allocate memory to hold the UTF-32 encoded string (plus a terminating nul)
	utf_32_char *unicode_string = (utf_32_char *)malloc(sizeof(utf_32_char) * (unicode_length + 1));
	utf_32_char *curOutPos = unicode_string;

	if (unicode_string == NULL)
	{
		debug(LOG_ERROR, "Out of memory");
		return NULL;
	}

	while (*curChar != '\0')
	{
		*(curOutPos++) = UTF8DecodeChar(curChar, &curChar);
	}

	// Terminate the string with a nul
	unicode_string[unicode_length] = '\0';

	// Set the number of bytes allocated
	if (nbytes)
	{
		*nbytes = sizeof(utf_32_char) * (unicode_length + 1);
	}

	return unicode_string;
}
