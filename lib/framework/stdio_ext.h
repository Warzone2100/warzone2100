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
*/
#ifndef STDIO_EXT_H
#define STDIO_EXT_H

#include "wzglobal.h"

#include <stdio.h>
#include <stdarg.h>

// These functions are GNU extensions; so make sure they are available on Windows also

#if defined(WZ_CC_MSVC) || defined(DOXYGEN)
/**
 * This function is analogue to vsprintf, except that it allocates a string
 * large enough to hold the output including the terminating NUL character.
 *
 * \param[out] strp   If successful will hold a pointer to the formatted
 *                    string. This pointer should be passed to free() to
 *                    release the allocated storage when it is no longer
 *                    needed. When unsuccessful the contents of \c strp are
 *                    undefined (no memory will have been allocated though).
 *
 * \param      format The format specifier (the same to vsprintf).
 *
 * \param      ap     An argument list to be used in combination with \c format
 *                    to print the string.
 *
 * \return When successful the amount of characters printed, just like
 *         vsprintf. If memory allocation wasn't possible or some other error
 *         occurred, -1 is returned.
 */
WZ_DECL_NONNULL(1, 2) int vasprintf(char **strp, const char *format, va_list ap);


/**
 * This function is analogue to sprintf, except that it allocates a string
 * large enough to hold the output including the terminating NUL character.
 *
 * @see vasprintf()
 */
WZ_DECL_NONNULL(1, 2) int asprintf(char **strp, const char *format, ...) WZ_DECL_FORMAT(printf, 2, 3);
#endif

#if defined(WZ_CC_MSVC)
// Make sure that these functions are available, and work according to the C99 spec on MSVC also

 int wz_vsnprintf(char *str, size_t size, const char *format, va_list ap);
WZ_DECL_NONNULL(3) int wz_snprintf(char *str, size_t size, const char *format, ...);

// Necessary to prevent conflicting symbols with MSVC's own (incorrect!) implementations of these functions
# define vsnprintf wz_vsnprintf
# define snprintf  wz_snprintf
#elif defined(__cplusplus) && defined(WZ_CC_GNU)
// Do nothing here, and assume that G++ has a proper implementation of snprintf and vsnprintf
#elif !defined(WZ_CC_GNU) && !defined(WZ_C99)
# error "This code depends on a C99-compliant implementation of snprintf and vsnprintf; please compile as C99 or provide a compliant implementation!"
#endif


// A stack-allocating variant of sprintf
#define sasprintf(strp, format, ...) \
	do { \
		/* Make sure to evaluate "format" just once */ \
		const char* fmt = format; \
		/* Determine the size of the string we're going to produce */ \
		size_t size = snprintf(NULL, 0, fmt, __VA_ARGS__); \
		\
		/* Let the compiler perform some static type-checking */ \
		char** var = strp; \
		\
		/* Allocate a buffer large enough to hold our string on the stack*/ \
		*var = (char*)alloca(size + 1); \
		/* Print into our newly created string-buffer */ \
		sprintf(*var, fmt,  __VA_ARGS__); \
	} while(0)

/// Equivalent to vasprintf, except that strp is NULL instead of undefined, if the function returns -1. Does not give compiler warnings/-Werrors if not checking the return value.
WZ_DECL_NONNULL(1, 2) int vasprintfNull(char **strp, const char *format, va_list ap);
/// Equivalent to asprintf, except that strp is NULL instead of undefined, if the function returns -1. Does not give compiler warnings/-Werrors if not checking the return value.
WZ_DECL_NONNULL(1, 2) int asprintfNull(char **strp, const char *format, ...);

#endif // STDIO_EXT_H
