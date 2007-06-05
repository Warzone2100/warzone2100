/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
/*! \file wzglobal.h
 *  \brief Global definitions

	Shamelessly stolen from Qt4 (Qt/qglobal.h) by Dennis.
	This has been greatly stripped down, feel free to add checks as you need them.
*/

#ifndef WZGLOBAL_H
#define WZGLOBAL_H


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef __MACOSX__
#include "config-macosx.h"
#endif


/*
   The operating system, must be one of: (WZ_OS_x)

     MAC      - Mac OS
     CYGWIN   - Cygwin
     WIN32    - Win32 (Windows 95/98/ME and Windows NT/2000/XP)
     WIN64    - Win64
     LINUX    - Linux
     FREEBSD  - FreeBSD
     NETBSD   - NetBSD
     OPENBSD  - OpenBSD
     BSDI     - BSD/OS
     QNX      - QNX
     QNX6     - QNX RTP 6.1
     BSD4     - Any BSD 4.4 system
     UNIX     - Any UNIX BSD/SYSV system
     WIN      - Any Windows system
*/

#if defined(__MACOSX__)
#  define WZ_OS_MAC
#elif defined(__CYGWIN__)
#  define WZ_OS_CYGWIN
#elif !defined(SAG_COM) && (defined(WIN64) || defined(_WIN64) || defined(__WIN64__))
#  define WZ_OS_WIN32
#  define WZ_OS_WIN64
#elif !defined(SAG_COM) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#  define WZ_OS_WIN32
#elif defined(__MWERKS__) && defined(__INTEL__)
#  define WZ_OS_WIN32
#elif defined(__linux__) || defined(__linux)
#  define WZ_OS_LINUX
#elif defined(__FreeBSD__) || defined(__DragonFly__)
#  define WZ_OS_FREEBSD
#elif defined(__NetBSD__)
#  define WZ_OS_NETBSD
#elif defined(__OpenBSD__)
#  define WZ_OS_OPENBSD
#elif defined(__bsdi__)
#  define WZ_OS_BSDI
#elif defined(__QNXNTO__)
#  define WZ_OS_QNX6
#elif defined(__QNX__)
#  define WZ_OS_QNX
#else
#  error "Warzone has not been tested on this OS. Please contact warzone-dev@gna.org"
#endif /* WZ_OS_x */

#if defined(WZ_OS_WIN32) || defined(WZ_OS_WIN64)
#  define WZ_OS_WIN
#endif /* WZ_OS_WIN32 */

#if defined(WZ_OS_FREEBSD) || defined(WZ_OS_NETBSD) || defined(WZ_OS_OPENBSD) || defined(WZ_OS_BSDI)
#  define WZ_OS_BSD4
#endif /* WZ_OS_FREEBSD */

#if defined(WZ_OS_WIN)
#  undef WZ_OS_UNIX
#elif !defined(WZ_OS_UNIX)
#  define WZ_OS_UNIX
#endif /* WZ_OS_WIN */


/*
   The compiler, must be one of: (WZ_CC_x)

     MSVC     - Microsoft Visual C/C++
     GNU      - GNU C++

   Should be sorted most to least authoritative.
*/

#if defined(_MSC_VER)
#  define WZ_CC_MSVC
/* Visual C++.Net issues for _MSC_VER >= 1300 */
#  if _MSC_VER >= 1300
#    define WZ_CC_MSVC_NET
#  endif
#elif defined(__GNUC__)
#  define WZ_CC_GNU
#else
#  error "Warzone has not been tested on this compiler. Please contact warzone-dev@gna.org"
#endif /* WZ_CC_x */


/*
   The supported C standard, must be one of: (WZ_Cxx)

     99       - ISO/IEC 9899:1999 / C99

*/
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
# define WZ_C99
#endif /* WZ_Cxx */


/**
 * \def WZ_DECL_DEPRECATED
 *
 * The WZ_DECL_DEPRECATED macro can be used to trigger compile-time warnings
 * with newer compilers when deprecated functions are used.
 *
 * For non-inline functions, the macro gets inserted at front of the
 * function declaration, right before the return type:
 *
 * \code
 * WZ_DECL_DEPRECATED void deprecatedFunctionA();
 * WZ_DECL_DEPRECATED int deprecatedFunctionB() const;
 * \endcode
 *
 * For functions which are implemented inline,
 * the WZ_DECL_DEPRECATED macro is inserted at the front, right before the return
 * type, but after "static", "inline" or "virtual":
 *
 * \code
 * WZ_DECL_DEPRECATED void deprecatedInlineFunctionA() { .. }
 * virtual WZ_DECL_DEPRECATED int deprecatedInlineFunctionB() { .. }
 * static WZ_DECL_DEPRECATED bool deprecatedInlineFunctionC() { .. }
 * inline WZ_DECL_DEPRECATED bool deprecatedInlineFunctionD() { .. }
 * \endcode
 *
 * You can also mark whole structs or classes as deprecated, by inserting the
 * WZ_DECL_DEPRECATED macro after the struct/class keyword, but before the
 * name of the struct/class:
 *
 * \code
 * class WZ_DECL_DEPRECATED DeprecatedClass { };
 * struct WZ_DECL_DEPRECATED DeprecatedStruct { };
 * \endcode
 *
 * \note
 * Description copied from KDE4, code copied from Qt4.
 *
 */
#if defined(WZ_CC_GNU) && !defined(WZ_CC_INTEL) && (__GNUC__ - 0 > 3 || (__GNUC__ - 0 == 3 && __GNUC_MINOR__ - 0 >= 2))
#  define WZ_DECL_DEPRECATED __attribute__ ((__deprecated__))
#elif defined(WZ_CC_MSVC) && (_MSC_VER >= 1300)
#  define WZ_DECL_DEPRECATED __declspec(deprecated)
#else
#  define WZ_DECL_DEPRECATED
#endif


/*!
 * \def WZ_DECL_UNUSED
 * This function is not used, but shall not generate an unused warning either.
 */
#if defined(WZ_CC_GNU) && !defined(WZ_CC_INTEL) && (__GNUC__ - 0 > 3 || (__GNUC__ - 0 == 3 && __GNUC_MINOR__ - 0 >= 2))
#  define WZ_DECL_UNUSED __attribute__((__unused__))
#else
#  define WZ_DECL_UNUSED
#endif


/*!
 * \def WZ_DECL_PURE
 * "Many functions have no effects except the return value and their return value depends only on the parameters and/or global variables. Such a function can be subject to common subexpression elimination and loop optimization just as an arithmetic operator would be."
 */
#if defined(WZ_CC_GNU) && !defined(WZ_CC_INTEL) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 2))
#  define WZ_DECL_PURE __attribute__((__pure__))
#else
#  define WZ_DECL_PURE
#endif


/*!
 * \def WZ_DECL_CONST
 * "Many functions do not examine any values except their arguments, and have no effects except the return value. Basically this is just slightly more strict class than the pure attribute below, since function is not allowed to read global memory."
 */
#if defined(WZ_CC_GNU) && !defined(WZ_CC_INTEL) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 2))
#  define WZ_DECL_CONST __attribute__((__const__))
#else
#  define WZ_DECL_CONST
#endif

/*
 * \note
 * The compiler must support 8bit long bytes
 */
#include <limits.h>
#if !(CHAR_BIT == 8)
# error "Warzone currently depends on having 8bit long bytes, therefore this system is not supported."
#endif

#endif /* WZGLOBAL_H */
