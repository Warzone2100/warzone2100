/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __INCLUDED_FRAMEWORK_STRLFUNCS_H__
#define __INCLUDED_FRAMEWORK_STRLFUNCS_H__

#include "frame.h"
#include <string.h>
#include <stddef.h>
#include <assert.h>

/** 
 *	A safer variant of \c strncpy and its completely unsafe variant \c strcpy.
 *	Copy src to string dst of size siz.  At most siz-1 characters
 *	will be copied.  Always NUL terminates (unless siz == 0).
 *	Returns strlen(src); if retval >= siz, truncation occurred.
 */
static inline size_t strlcpy(char *WZ_DECL_RESTRICT dst, const char *WZ_DECL_RESTRICT src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	assert(dst != NULL);
	assert(src != NULL);

	/* Copy as many bytes as will fit */
	if (n != 0)
	{
		while (--n != 0)
		{
			if ((*d++ = *s++) == '\0')
			{
				break;
			}
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0)
	{
		if (siz != 0)
		{
			*d = '\0';                /* NUL-terminate dst */
		}
		while (*s++) ;
	}

	return(s - src - 1);        /* count does not include NUL */
}

/** 
 *	A safer variant of \c strncat and its completely unsafe variant \c strcat.
 *	Appends src to string dst of size siz (unlike strncat, siz is the
 *	full size of dst, not space left).  At most siz-1 characters
 *	will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 *	Returns strlen(src) + MIN(siz, strlen(initial dst)).
 *	If retval >= siz, truncation occurred.
 */
static inline size_t strlcat(char *WZ_DECL_RESTRICT dst, const char *WZ_DECL_RESTRICT src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;

	assert(dst != NULL);
	assert(src != NULL);

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
	{
		d++;
	}
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
	{
                return(dlen + strlen(s));
	}
	while (*s != '\0')
	{
		if (n != 1)
		{
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));        /* count does not include NUL */
}

/* Static array versions of common string functions. Safer because one less parameter to screw up. 
 * Can only be used on strings longer than the length of a pointer, because we use this for debugging. */
#ifndef DEBUG
#define sstrcpy(dest, src) strlcpy((dest), (src), sizeof(dest))
#define sstrcat(dest, src) strlcat((dest), (src), sizeof(dest))
#define ssprintf(dest, ...) snprintf((dest), sizeof(dest), __VA_ARGS__)
#define vssprintf(dest, format, ap) vsnprintf((dest), sizeof(dest), format, ap)
#define sstrcmp(str1, str2) strncmp((str1), (str2), sizeof(str1) > sizeof(str2) ? sizeof(str2) : sizeof(str1))
#else
#define sstrcpy(dest, src) (WZ_ASSERT_STATIC_STRING(dest), strlcpy((dest), (src), sizeof(dest)))
#define sstrcat(dest, src) (WZ_ASSERT_STATIC_STRING(dest), strlcat((dest), (src), sizeof(dest)))
#define ssprintf(dest, ...) (WZ_ASSERT_STATIC_STRING(dest), snprintf((dest), sizeof(dest), __VA_ARGS__))
#define vssprintf(dest, format, ap) (WZ_ASSERT_STATIC_STRING(dest), vsnprintf((dest), sizeof(dest), format, ap))
#define sstrcmp(str1, str2) (WZ_ASSERT_STATIC_STRING(str1), WZ_ASSERT_STATIC_STRING(str2), strncmp((str1), (str2), sizeof(str1) > sizeof(str2) ? sizeof(str2) : sizeof(str1)))
#endif

#endif // __INCLUDED_FRAMEWORK_STRLFUNCS_H__
