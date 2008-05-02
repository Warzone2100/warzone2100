# strlfuncs.m4
dnl Copyright (C) 2008 Giel van Schijndel
dnl Copyright (C) 2008 Warzone Resurrection Project
dnl
dnl This file is free software; I (Giel van Schijndel) give unlimited permission
dnl to copy and/or distribute it, with or without modifications, as long as this
dnl notice is preserved.

# Define HAVE_STRLCPY when strlcpy(char* dst, const char* src, size_t size) is
# available and copies at the most (size - 1) into (dst) and always NUL
# terminates (dst) when (size) is at least one (1). The return value also _must_
# be strlen(src).

AC_DEFUN([AC_STRLCPY_CHECK],
[
	AC_CACHE_CHECK([for OpenBSD or MacOSX strlcpy], ac_cv_have_strlcpy,[
		AC_TRY_RUN([
			#include <stdlib.h>
			#include <string.h>
			#include <stdio.h>
			#define MIN(a, b) a > b ? b : a

			int main()
			{
				static const char str[] = "teststring";
				char b[10] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
				size_t retval = strlcpy(b, str, sizeof(b));

				/* Check for a proper return value */
				if (retval != strlen(str))
				{
					fprintf(stderr, "Function strlcpy should return strlen(src) (%u) but returned %u.\n", (unsigned int)strlen(str), (unsigned int)retval);
					return EXIT_FAILURE;
				}
				/* Check for proper NUL-termination */
				else if (b[MIN(sizeof(b) - 1, strlen(str))] != '\0')
				{
					fprintf(stderr, "Function strlcpy didn't properly NUL-terminate it's target string.\n");
					return EXIT_FAILURE;
				}

				return EXIT_SUCCESS;
			}
		], ac_cv_have_strlcpy=yes, ac_cv_have_strlcpy=no, ac_cv_have_strlcpy=no)])
	if test "$ac_cv_have_strlcpy" = "yes"; then
		AC_DEFINE(HAVE_STRLCPY, 1, [Have function strlcpy])
	fi
])

# Define HAVE_STRLCAT when strlcat(char* dst, const char* src, size_t size) is
# available and appends (src) to (dst) but copies at the most
# (size - strlen(dst) - 1) characters from (src) into (dst). It must also NUL
# terminate (dst) when (size) is at least one (1). The return value also _must_
# be strlen(src) + min(size, strlen((dst) before execution)).

AC_DEFUN([AC_STRLCAT_CHECK],
[
	AC_CACHE_CHECK([for OpenBSD or MacOSX strlcat], ac_cv_have_strlcat,[
		AC_TRY_RUN([
			#include <stdlib.h>
			#include <string.h>
			#include <stdio.h>

			int main()
			{
				static const char catstr[] = "very long concatenation string that shouldn't get fully concatenated";
				char str[20] = "teststring";
				size_t retval = strlcat(str, catstr, 0);
				size_t expretval;

				/* Check for a proper return value */
				if (retval != strlen(catstr))
				{
					fprintf(stderr, "strlcat(dst, str, 0) should return strlen(str) (%u) but it returned %u.\n", (unsigned int)strlen(str), (unsigned int)retval);
					return EXIT_FAILURE;
				}

				/* Check for a proper return value in case of truncation */
				expretval = strlen(str) + strlen(catstr);
				retval = strlcat(str, catstr, sizeof(str));
				if (retval != expretval)
				{
					fprintf(stderr, "strlcat(dst, str, sizeof(dst)) should return strlen(dst) + strlen(src) (%u) but it returned %u.\n", (unsigned int)expretval, (unsigned int)retval);
					return EXIT_FAILURE;
				}

				return EXIT_SUCCESS;
			}
		], ac_cv_have_strlcat=yes, ac_cv_have_strlcat=no, ac_cv_have_strlcat=no)])
	if test "$ac_cv_have_strlcat" = "yes"; then
		AC_DEFINE(HAVE_STRLCAT, 1, [Have function strlcat])
	fi
])
