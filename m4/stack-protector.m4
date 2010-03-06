dnl
dnl AX_STACK_PROTECT_CC and AX_STACK_PROTECT_CXX
dnl
dnl checks for -fstack-protector with the C compiler and C++ compiler
dnl respectively, if it exists then updates CFLAGS and CXXFLAGS respectively
dnl
dnl Test program that should definitly cause -fstack-protector to
dnl insert stack checking code.
m4_define([stack_protector_c_test_program],
[
	AC_LANG_PROGRAM([[
		#include <stdio.h>
		#include <string.h>

		extern char *test_a, *test_b;
		char *test_a, *test_b;
		char* test_concat(void)
		{
			char buf[2048];

			strcpy(buf, test_a);
			strcat(buf, test_b);

			return strdup(buf);
		}
	]],[[
		puts(test_concat());
	]])
])

AC_DEFUN([AX_STACK_PROTECT_CC],[
	AC_MSG_CHECKING([whether ${CC} accepts -fstack-protector])

	AC_LANG_PUSH([C])
	AC_LANG_COMPILER_REQUIRE()

	# Keep a copy of the old CFLAGS and CXXFLAGS
	OLD_CFLAGS="$CFLAGS"
	OLD_CXXFLAGS="$CXXFLAGS"
	CFLAGS="$CFLAGS -fstack-protector"
	CXXFLAGS="$CXXFLAGS -fstack-protector"

	dnl Execute the actual checks
	dnl Try to compile and link as C both
	AC_LINK_IFELSE([stack_protector_c_test_program], [
		dnl Try to compile as C and link as C++
		AC_LANG_CONFTEST([stack_protector_c_test_program])
		AS_IF([{
			rm -f conftest.$ac_objext conftest$ac_exeext
			AS_IF([_AC_DO_STDERR($ac_compile) &&
			       _AC_DO_STDERR($CXX -o conftest$ac_exeext $CXXFLAGS $CPPFLAGS $LDFLAGS conftest.$ac_objext $LIBS >&AS_MESSAGE_LOG_FD) && {
			       test -z "$ac_c_werror_flag" ||
			       test ! -s conftest.err
			     } && test -s conftest$ac_exeext && {
			       test "$cross_compiling" = yes ||
			       AS_TEST_X([conftest$ac_exeext])
			     }],
			    [ac_retval=0],
			    [_AC_MSG_LOG_CONFTEST
			      ac_retval=1])
			# Delete the IPA/IPO (Inter Procedural Analysis/Optimization) information
			# created by the PGI compiler (conftest_ipa8_conftest.oo), as it would
			# interfere with the next link command; also delete a directory that is
			# left behind by Apple's compiler.  We do this before executing the actions.
			rm -rf conftest.dSYM conftest_ipa8_conftest.oo
			test "$ac_retval" = 0
		}], AC_MSG_RESULT(yes), [
			CFLAGS="$OLD_CFLAGS"
			AC_MSG_RESULT(no)
		])
	],[
		CFLAGS="$OLD_CFLAGS"
		AC_MSG_RESULT(no)
	])

	# Restore CXXFLAGS as we're testing the C compiler here, not the C++ compiler.
	CXXFLAGS="$OLD_CXXFLAGS"

	AC_LANG_POP([C])
])
AC_DEFUN([AX_STACK_PROTECT_CXX],[
	AC_MSG_CHECKING([whether ${CXX} accepts -fstack-protector])

	AC_LANG_PUSH([C++])

	# Keep a copy of the old CXXFLAGS
	OLD_CXXFLAGS="$CXXFLAGS"
	CXXFLAGS="$CXXFLAGS -fstack-protector"
	AC_LINK_IFELSE([stack_protector_c_test_program], AC_MSG_RESULT(yes), [
		CXXFLAGS="$OLD_CXXFLAGS"
		AC_MSG_RESULT(no)
	])

	AC_LANG_POP([C++])
])
