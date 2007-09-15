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

/** \file strnlen1.h
 *  \brief provides a safe variant of strlen
 */
#ifndef __INCLUDED_LIB_FRAMEWORK_STRNLEN1_H__
#define __INCLUDED_LIB_FRAMEWORK_STRNLEN1_H__

#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

/** Finds the length of the required buffer to store string.
 *  \param string holds the string to scan the required buffer size for
 *  \param maxlen the maximum amount of bytes to scan in string
 *  \return the required size for a buffer to hold \c string or \c maxlen if
 *          no terminating '\\0' is found.
 *
 *  \note This is the same as strnlen(string, maxlen - 1) + 1 when using the
 *        GNU C library.
 */
extern size_t strnlen1 (const char* string, size_t maxlen);

#ifdef __cplusplus
}
#endif


#endif // __INCLUDED_LIB_FRAMEWORK_STRNLEN1_H__
