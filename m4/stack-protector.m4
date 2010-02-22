dnl
dnl AX_STACK_PROTECT_CC and AX_STACK_PROTECT_CXX
dnl
dnl checks for -fstack-protector with the C compiler and C++ compiler
dnl respectively, if it exists then updates CFLAGS and CXXFLAGS respectively
dnl
AC_DEFUN([AX_STACK_PROTECT_CC],[
	AC_MSG_CHECKING([whether ${CC} accepts -fstack-protector])

	AC_LANG_PUSH([C])

	# Keep a copy of the old CFLAGS
	OLD_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS -fstack-protector"

	AC_TRY_LINK(,, AC_MSG_RESULT(yes), [
		CFLAGS="$OLD_CFLAGS"
		AC_MSG_RESULT(no)
	])

	AC_LANG_POP([C])
])
AC_DEFUN([AX_STACK_PROTECT_CXX],[
	AC_MSG_CHECKING([whether ${CXX} accepts -fstack-protector])

	AC_LANG_PUSH([C++])

	# Keep a copy of the old CXXFLAGS
	OLD_CXXFLAGS="$CXXFLAGS"
	CXXFLAGS="$CXXFLAGS -fstack-protector"
	AC_TRY_LINK(,, AC_MSG_RESULT(yes), [
		CXXFLAGS="$OLD_CXXFLAGS"
		AC_MSG_RESULT(no)
	])

	AC_LANG_POP([C++])
])
