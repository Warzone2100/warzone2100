# ===========================================================================
#              http://autoconf-archive.cryp.to/ac_pkg_swig.html
# ===========================================================================
#
# SYNOPSIS
#
#   AC_PROG_SWIG([major.minor.micro])
#
# DESCRIPTION
#
#   This macro searches for a SWIG installation on your system. If found you
#   should call SWIG via $(SWIG). You can use the optional first argument to
#   check if the version of the available SWIG is greater than or equal to
#   the value of the argument. It should have the format: N[.N[.N]] (N is a
#   number between 0 and 999. Only the first N is mandatory.)
#
#   If the version argument is given (e.g. 1.3.17), AC_PROG_SWIG checks that
#   the swig package is this version number or higher.
#
#   In configure.in, use as:
#
#     AC_PROG_SWIG(1.3.17)
#     SWIG_ENABLE_CXX
#     SWIG_MULTI_MODULE_SUPPORT
#     SWIG_PYTHON
#
# LAST MODIFICATION
#
#   2008-04-12
#
# COPYLEFT
#
#   Copyright (c) 2008 Sebastian Huber <sebastian-huber@web.de>
#   Copyright (c) 2008 Alan W. Irwin <irwin@beluga.phys.uvic.ca>
#   Copyright (c) 2008 Rafael Laboissiere <rafael@laboissiere.net>
#   Copyright (c) 2008 Andrew Collier <colliera@ukzn.ac.za>
#   Copyright (c) 2008 Giel van Schijndel <muggenhor@gna.org>
#
#   This program is free software; you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation; either version 2 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <http://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Macro Archive. When you make and
#   distribute a modified version of the Autoconf Macro, you may extend this
#   special exception to the GPL to apply to your modified version as well.

AC_DEFUN([AC_PROG_SWIG],[
	AC_REQUIRE([AC_PROG_SED])
	AC_REQUIRE([AC_PROG_GREP])

	AX_WITH_PROG([SWIG], [swig])
	AS_IF([test -n "$SWIG"],[
		ax_swig_version="$1"

		AC_MSG_CHECKING([for swig version])
		changequote(<<,>>)
		swig_version=`$SWIG -version 2>&1 | $GREP -i 'version' | $SED -e 's/.*Version\s\+\([0-9]\+\.[0-9]*\.[0-9]*\).*/\1/'`
		changequote([,])
		AC_MSG_RESULT($swig_version)

		AC_SUBST([SWIG_VERSION],[$swig_version])

		AX_COMPARE_VERSION([$ax_swig_version],[le],[$swig_version],[
			:
			AC_SUBST([SWIG_LIB], `$SWIG -swiglib`)
		],[
			:
			AC_MSG_ERROR([could not find the swig interpreter])
		])
	],[
		AC_MSG_ERROR([could not find the swig interpreter])
	])
])
