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

#include "utf8.h"
#include "debug.h"

size_t utf8_character_count(const char* utf8_string)
{
	// Yet to implement
}

size_t unicode_utf8_buffer_length(const uint_fast32_t* unicode_string)
{
	const uint_fast32_t* curChar;

	// Determine length of string (in octets) when encoded in UTF-8
	size_t length = 0;
	for (curChar = unicode_string; *curChar != 0; ++curChar)
	{
		// an ASCII character, which uses 7 bit at most, which is one byte in UTF-8
		if      (*curChar < 0x00000080)
			length += 1; // stores 7 bits
		else if (*curChar < 0x00000800)
			length += 2; // stores 11 bits
		else if (*curChar < 0x00010000)
			length += 3; // stores 16 bits
		else if (*curChar < 0x00200000)
			length += 4; // stores 21 bits
		else if (*curChar < 0x04000000)
			length += 5; // stores 26 bits
		else if (*curChar < 0x80000000)
			length += 6; // stores 31 bits
		else // if (*curChar < 0x1000000000)
			length += 7; // stores 36 bits
	}

	return length;
}

char* utf8_encode(const uint_fast32_t* unicode_string)
{
	const uint_fast32_t* curChar;

	const size_t utf8_length = unicode_utf8_buffer_length(unicode_string);

	// Allocate memory to hold the UTF-8 encoded string (plus a terminating nul char)
	char* utf8_string = malloc(utf8_length + 1);
	char* curOutPos = utf8_string;

	if (utf8_string == NULL)
	{
		debug(LOG_ERROR, "utf8_encode: Out of memory");
		return NULL;
	}

	for (curChar = unicode_string; *curChar != 0; ++curChar)
	{
		// 7 bits
		if      (*curChar < 0x00000080)
		{
			*(curOutPos++) = *curChar;
		}
		// 11 bits
		else if (*curChar < 0x00000800)
		{
			// 0xc0 provides the counting bits: 110
			// then append the 5 most significant bits
			*(curOutPos++) = 0xc0 | (*curChar >> 6);
			// Put the next 6 bits in a byte of their own
			*(curOutPos++) = 0x80 | (*curChar & 0x3f);
		}
		// 16 bits
		else if (*curChar < 0x00010000)
		{
			// 0xe0 provides the counting bits: 1110
			// then append the 4 most significant bits
			*(curOutPos++) = 0xe0 | (*curChar >> 12);
			// Put the next 12 bits in two bytes of their own
			*(curOutPos++) = 0x80 | ((*curChar >> 6) & 0x3f);
			*(curOutPos++) = 0x80 | (*curChar & 0x3f);
		}
		// 21 bits
		else if (*curChar < 0x00200000)
		{
			// 0xf0 provides the counting bits: 11110
			// then append the 3 most significant bits
			*(curOutPos++) = 0xf0 | (*curChar >> 18);
			// Put the next 18 bits in three bytes of their own
			*(curOutPos++) = 0x80 | ((*curChar >> 12) & 0x3f);
			*(curOutPos++) = 0x80 | ((*curChar >> 6) & 0x3f);
			*(curOutPos++) = 0x80 | (*curChar & 0x3f);
		}
		// 26 bits
		else if (*curChar < 0x04000000)
		{
			// 0xf8 provides the counting bits: 111110
			// then append the 2 most significant bits
			*(curOutPos++) = 0xf8 | (*curChar >> 24 );
			// Put the next 24 bits in four bytes of their own
			*(curOutPos++) = 0x80 | ((*curChar >> 18) & 0x3f);
			*(curOutPos++) = 0x80 | ((*curChar >> 12) & 0x3f);
			*(curOutPos++) = 0x80 | ((*curChar >> 6) & 0x3f);
			*(curOutPos++) = 0x80 | (*curChar & 0x3f);
		}
		// 31 bits
		else if (*curChar < 0x80000000)
		{
			// 0xfc provides the counting bits: 1111110
			// then append the 1 most significant bit
			*(curOutPos++) = 0xfc | (*curChar >> 30);
			// Put the next 30 bits in five bytes of their own
			*(curOutPos++) = 0x80 | ((*curChar >> 24) & 0x3f);
			*(curOutPos++) = 0x80 | ((*curChar >> 18) & 0x3f);
			*(curOutPos++) = 0x80 | ((*curChar >> 12) & 0x3f);
			*(curOutPos++) = 0x80 | ((*curChar >> 6) & 0x3f);
			*(curOutPos++) = 0x80 | (*curChar & 0x3f);
		}
		// 36 bits
		else
		{
			// 0xfe provides the counting bits: 11111110
			*(curOutPos++) = 0xfe;
			// Put the next 36 bits in six bytes of their own
			*(curOutPos++) = 0x80 | ((*curChar >> 30) & 0x3f);
			*(curOutPos++) = 0x80 | ((*curChar >> 24) & 0x3f);
			*(curOutPos++) = 0x80 | ((*curChar >> 18) & 0x3f);
			*(curOutPos++) = 0x80 | ((*curChar >> 12) & 0x3f);
			*(curOutPos++) = 0x80 | ((*curChar >> 6) & 0x3f);
			*(curOutPos++) = 0x80 | (*curChar & 0x3f);
		}
	}

	// Terminate the string with a nul character
	utf8_string[utf8_length] = '\0';

	return utf8_string;
}

uint_fast32_t* utf8_decode(const char* utf8_string)
{
	// Yet to implement
}
