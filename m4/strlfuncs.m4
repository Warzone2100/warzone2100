# strlfuncs.m4
dnl Copyright (C) 2009  Giel van Schijndel
dnl Copyright (C) 2009  Warzone Resurrection Project
dnl
dnl This file is free software; I, Giel van Schijndel give unlimited permission
dnl to copy and/or distribute it, with or without modifications, as long as
dnl this notice is preserved.
dnl
dnl This file can be used in projects which are not available under the GNU
dnl General Public License or the GNU Library General Public License.  Please
dnl note that the the rest of the Warzone 2100 package is covered by the GNU
dnl General Public License.
dnl It is not *not* in the public domain.

dnl Authors:
dnl   Giel van Schijndel <muggenhor@gna.org>, 2009

AC_PREREQ(2.56)

AC_DEFUN([AX_CHECK_STRLCPY],[
	AC_CHECK_FUNC([strlcpy],[
		AC_DEFINE([HAVE_SYSTEM_STRLCPY], [], [The system provides strlcpy])
		AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <stdlib.h>
#include <string.h>

static const char srcstr1[] = "source string 1";
static char buf1[sizeof(srcstr1)];
static char buf2[sizeof(srcstr1) * 2];
static char buf3[sizeof(srcstr1) / 2];
]],[[
static const char initial_value = 0x55;
/* Make sure that all buffers are filled with an arbitrary non-NULL value, to
 * check that overflow doesn't occur.
 */
memset(buf1, initial_value, sizeof(buf1));
memset(buf2, initial_value, sizeof(buf2));
memset(buf3, initial_value, sizeof(buf3));

/* Check whether behaviour is consistent with strcpy/strncpy for strings where
 * the buffer size matches
 */
if (strlcpy(buf1, srcstr1, sizeof(buf1)) != strlen(srcstr1)
 || memcmp(srcstr1, buf1, sizeof(buf1)) != 0)
	exit(EXIT_FAILURE);
/* Check whether behaviour is consistent with strcpy/strncpy for strings where
 * the buffer size is larger than required
 */
if (strlcpy(buf2, srcstr1, sizeof(buf2)) != strlen(srcstr1)
 || memcmp(srcstr1, buf2, sizeof(srcstr1)) != 0)
	exit(EXIT_FAILURE);
/* Check whether strlcpy *always* returns the amount of characters in the
 * source string */
if (strlcpy(buf3, srcstr1, 0) != strlen(srcstr1)
 || buf3[0] != initial_value /* verify that no data has been written */
 || strlcpy(buf3, srcstr1, sizeof(buf3)) != strlen(srcstr1)
/* Check whether strlcpy fills the buffer properly with the first part of the
 * source string if the buffer is too small
 */
 || memcmp(srcstr1, buf3, sizeof(buf3) - 1) != 0
/* Check whether strlcpy properly guarantees NUL-termination */
 || buf3[sizeof(buf3) - 1] != '\0')
	exit(EXIT_FAILURE);
]])
		],[AC_DEFINE([HAVE_VALID_STRLCPY], [], [The system provides a strlcpy we can use])])
	])
])

AC_DEFUN([AX_CHECK_STRLCAT],[
	AC_CHECK_FUNC([strlcat],[
		AC_DEFINE([HAVE_SYSTEM_STRLCAT], [], [The system provides strlcat])
		AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <stdlib.h>
#include <string.h>

static const char srcstr1[] = "source string 1";
static const char srcstr2[] = "concatenation string";
static char buf1[sizeof(srcstr1) + sizeof(srcstr2)];
static char buf2[(sizeof(srcstr1) + sizeof(srcstr2)) * 2];
static char buf3[sizeof(srcstr1) / 2];
static char buf4[(sizeof(srcstr1) + sizeof(srcstr2) / 2)];
]],[[
static const char initial_value = 0x55;
/* Make sure that all buffers are filled with an arbitrary non-NULL value, to
 * check that overflow doesn't occur.
 */
memset(buf1, initial_value, sizeof(buf1));
memset(buf2, initial_value, sizeof(buf2));
memset(buf3, initial_value, sizeof(buf3));
memset(buf4, initial_value, sizeof(buf4));

/* Prefill the buffers so that we can check concatenation */
strncpy(buf1, srcstr1, sizeof(buf1));
strncpy(buf2, srcstr1, sizeof(buf2));
strncpy(buf3, srcstr1, sizeof(buf3));
strncpy(buf4, srcstr1, sizeof(buf4));

/* Check whether behaviour is consistent with strcat/strncat for strings where
 * the buffer size matches
 */
if (strlcat(buf1, srcstr2, sizeof(buf1)) != (strlen(srcstr1) + strlen(srcstr2))
 || memcmp(srcstr1, buf1, strlen(srcstr1)) != 0
 || memcmp(srcstr2, buf1 + strlen(srcstr1), sizeof(srcstr2)) != 0)
	exit(EXIT_FAILURE);
/* Check whether behaviour is consistent with strcat/strncat for strings where
 * the buffer size is larger than required
 */
if (strlcat(buf2, srcstr2, sizeof(buf2)) != (strlen(srcstr1) + strlen(srcstr2))
 || memcmp(srcstr1, buf2, strlen(srcstr1)) != 0
 || memcmp(srcstr2, buf2 + strlen(srcstr1), sizeof(srcstr2)) != 0)
	exit(EXIT_FAILURE);
/* Check whether strlcat *always* returns the amount of characters the
 * resulting string would have if the destination buffer was large enough.
 */
if (strlcat(buf3, srcstr2, 0) != strlen(srcstr2)
 || strlcat(buf3, srcstr2, sizeof(buf3)) != (sizeof(buf3) + strlen(srcstr2))
/* Check whether strlcat fills the buffer properly with the first part of the
 * source string if the buffer is too small
 */
 || memcmp(srcstr1, buf3, sizeof(buf3) - 1) != 0)
	exit(EXIT_FAILURE);
/* Check whether strlcat properly guarantees NUL-termination */
if (strlcat(buf4, srcstr2, sizeof(buf4)) != (strlen(srcstr1) + strlen(srcstr2))
 || buf4[sizeof(buf4) - 1] != '\0')
	exit(EXIT_FAILURE);
]])
		],[AC_DEFINE([HAVE_VALID_STRLCAT], [], [The system provides a strlcat we can use])])
	])
])
