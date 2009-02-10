/*
	This file is part of Warzone 2100.
	Copyright (C) 1992-2007  Trolltech ASA.
	Copyright (C) 2005-2009  Warzone Resurrection Project

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
 *  \brief Platform detection, workarounds and compat fixes
 *
 *  OS and CC detection code shamelessly stolen from Qt4 (Qt/qglobal.h) by Dennis.
 *  This has been stripped down, feel free to add checks as you need them.
 */

#ifndef WZGLOBAL_H
#define WZGLOBAL_H


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef __MACOSX__
#  include "config-macosx.h"
#endif


/* ---- Platform detection ---- */


/*
   The operating system, must be one of: (WZ_OS_x)

     DARWIN   - Darwin OS (synonym for WZ_OS_MAC)
     OS2      - OS/2
     OS2EMX   - XFree86 on OS/2 (not PM)
     WIN32    - Win32 (Windows 95/98/ME and Windows NT/2000/XP)
     CYGWIN   - Cygwin
     SOLARIS  - Sun Solaris
     HPUX     - HP-UX
     ULTRIX   - DEC Ultrix
     LINUX    - Linux
     FREEBSD  - FreeBSD
     NETBSD   - NetBSD
     OPENBSD  - OpenBSD
     BSDI     - BSD/OS
     IRIX     - SGI Irix
     OSF      - HP Tru64 UNIX
     SCO      - SCO OpenServer 5
     UNIXWARE - UnixWare 7, Open UNIX 8
     AIX      - AIX
     HURD     - GNU Hurd
     DGUX     - DG/UX
     RELIANT  - Reliant UNIX
     DYNIX    - DYNIX/ptx
     QNX      - QNX
     QNX6     - QNX RTP 6.1
     LYNX     - LynxOS
     BSD4     - Any BSD 4.4 system
     UNIX     - Any UNIX BSD/SYSV system
*/

#if defined(__APPLE__) && (defined(__GNUC__) || defined(__xlC__) || defined(__xlc__))
#  define WZ_OS_DARWIN
#  define WZ_OS_BSD4
#  ifdef __LP64__
#    define WZ_OS_DARWIN64
#  else
#    define WZ_OS_DARWIN32
#  endif
#elif defined(__CYGWIN__)
#  define WZ_OS_CYGWIN
#elif defined(__OS2__)
#  if defined(__EMX__)
#    define WZ_OS_OS2EMX
#  else
#    define WZ_OS_OS2
#  endif
#elif !defined(SAG_COM) && (defined(WIN64) || defined(_WIN64) || defined(__WIN64__))
#  define WZ_OS_WIN32
#  define WZ_OS_WIN64
#elif !defined(SAG_COM) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#  define WZ_OS_WIN32
#elif defined(__MWERKS__) && defined(__INTEL__)
#  define WZ_OS_WIN32
#elif defined(__sun) || defined(sun)
#  define WZ_OS_SOLARIS
#elif defined(hpux) || defined(__hpux)
#  define WZ_OS_HPUX
#elif defined(__ultrix) || defined(ultrix)
#  define WZ_OS_ULTRIX
#elif defined(sinix)
#  define WZ_OS_RELIANT
#elif defined(__linux__) || defined(__linux)
#  define WZ_OS_LINUX
#elif defined(__FreeBSD__) || defined(__DragonFly__)
#  define WZ_OS_FREEBSD
#  define WZ_OS_BSD4
#elif defined(__NetBSD__)
#  define WZ_OS_NETBSD
#  define WZ_OS_BSD4
#elif defined(__OpenBSD__)
#  define WZ_OS_OPENBSD
#  define WZ_OS_BSD4
#elif defined(__bsdi__)
#  define WZ_OS_BSDI
#  define WZ_OS_BSD4
#elif defined(__sgi)
#  define WZ_OS_IRIX
#elif defined(__osf__)
#  define WZ_OS_OSF
#elif defined(_AIX)
#  define WZ_OS_AIX
#elif defined(__Lynx__)
#  define WZ_OS_LYNX
#elif defined(__GNU__)
#  define WZ_OS_HURD
#elif defined(__DGUX__)
#  define WZ_OS_DGUX
#elif defined(__QNXNTO__)
#  define WZ_OS_QNX6
#elif defined(__QNX__)
#  define WZ_OS_QNX
#elif defined(_SEQUENT_)
#  define WZ_OS_DYNIX
#elif defined(_SCO_DS) /* SCO OpenServer 5 + GCC */
#  define WZ_OS_SCO
#elif defined(__USLC__) /* all SCO platforms + UDK or OUDK */
#  define WZ_OS_UNIXWARE
#elif defined(__svr4__) && defined(i386) /* Open UNIX 8 + GCC */
#  define WZ_OS_UNIXWARE
#elif defined(__INTEGRITY)
#  define WZ_OS_INTEGRITY
#elif defined(__MAKEDEPEND__)
#else
#  error "Warzone has not been tested on this OS. Please contact warzone-dev@gna.org"
#endif /* WZ_OS_x */

#if defined(WZ_OS_WIN32) || defined(WZ_OS_WIN64)
#  define WZ_OS_WIN
#endif /* WZ_OS_WIN32 */

#if defined(WZ_OS_DARWIN)
#  define WZ_OS_MAC /* WZ_OS_MAC is mostly for compatibility, but also more clear */
#  define WZ_OS_MACX /* WZ_OS_MACX is only for compatibility.*/
#  if defined(WZ_OS_DARWIN64)
#     define WZ_OS_MAC64
#  elif defined(WZ_OS_DARWIN32)
#     define WZ_OS_MAC32
#  endif
#endif /* WZ_OS_DARWIN */

#if defined(WZ_OS_MSDOS) || defined(WZ_OS_OS2) || defined(WZ_OS_WIN)
#  undef WZ_OS_UNIX
#elif !defined(WZ_OS_UNIX)
#  define WZ_OS_UNIX
#endif /* WZ_OS_* */


/*
   The compiler, must be one of: (WZ_CC_x)

     MSVC     - Microsoft Visual C/C++, Intel C++ for Windows
     GNU      - GNU C++
     INTEL    - Intel C++ for Linux, Intel C++ for Windows
     TINYC    - Fabrice Bellard's Tiny C Compiler

   Should be sorted most to least authoritative.
*/

#if defined(_MSC_VER)
#  define WZ_CC_MSVC
/* Visual C++.Net issues for _MSC_VER >= 1300 */
#  if _MSC_VER >= 1300
#    define WZ_CC_MSVC_NET
#  endif
/* Intel C++ disguising as Visual C++: the `using' keyword avoids warnings */
#  if defined(__INTEL_COMPILER)
#    define WZ_CC_INTEL
#  endif
/* x64 does not support mmx intrinsics on windows */
#  if (defined(ZS_OS_WIN64) && defined(_M_X64))
#    undef ZS_HAVE_SSE
#    undef ZS_HAVE_SSE2
#    undef ZS_HAVE_MMX
#    undef ZS_HAVE_3DNOW
#  endif

#elif defined(__GNUC__)
#  define WZ_CC_GNU
#  if defined(__MINGW32__)
#    define WZ_CC_MINGW
#  endif
#  if defined(__INTEL_COMPILER)
/* Intel C++ also masquerades as GCC 3.2.0 */
#    define WZ_CC_INTEL
#  endif

#elif defined(__TINYC__)
#  define WZ_CC_TINYC

#else
#  error "Warzone has not been tested on this compiler. Please contact warzone-dev@gna.org"
#endif /* WZ_CC_x */


/*
   The window system, must be one of: (WZ_WS_x)

     MACX     - Mac OS X
     WIN32    - Windows
     X11      - X Window System
     QNX      - QNX
*/

#if defined(_WIN32_X11_)
#  define WZ_WS_X11

#elif defined(WZ_OS_WIN32)
#  define WZ_WS_WIN32
#  if defined(WZ_OS_WIN64)
#    define WZ_WS_WIN64
#  endif

#elif defined(WZ_OS_MAC)
#  define WZ_WS_MAC
#  define WZ_WS_MACX
#  if defined(WZ_OS_MAC64)
#    define WZ_WS_MAC64
#  elif defined(WZ_OS_MAC32)
#    define WZ_WS_MAC32
#  endif

#elif defined(WZ_OS_QNX)
#  define WZ_WS_QNX

#elif defined(WZ_OS_UNIX)
#  define WZ_WS_X11

#else
#  error "Warzone has not been tested on this window system. Please contact warzone-dev@gna.org"
#endif /* WZ_WS_x */

#if defined(WZ_WS_WIN16) || defined(WZ_WS_WIN32)
#  define WZ_WS_WIN
#endif


/*
   The supported C standard, must be one of: (WZ_Cxx)

     99       - ISO/IEC 9899:1999 / C99
*/
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
# define WZ_C99
#endif /* WZ_Cxx */


/*
   Convenience macros to test the versions of gcc.
   Copied from glibc's features.h.
*/
#if defined(WZ_CC_GNU) && defined __GNUC__ && defined __GNUC_MINOR__
#  define WZ_CC_GNU_PREREQ(maj, min) \
          ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#  define WZ_CC_GNU_PREREQ(maj, min) 0
#endif


/*
   Convenience macros to test the versions of icc.
*/
#if defined(WZ_CC_INTEL) && defined __ICC
#  define WZ_CC_INTEL_PREREQ(maj, min) \
          ((__ICC) >= ((maj) * 100) + (min))
#else
#  define WZ_CC_INTEL_PREREQ(maj, min) 0
#endif




/* ---- Declaration attributes ---- */


/*!
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
#if WZ_CC_GNU_PREREQ(3,2) || WZ_CC_INTEL_PREREQ(10,0)
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
#if WZ_CC_GNU_PREREQ(3,2) || WZ_CC_INTEL_PREREQ(10,0)
#  define WZ_DECL_UNUSED __attribute__((__unused__))
#else
#  define WZ_DECL_UNUSED
#endif


/*!
 * \def WZ_DECL_PURE
 * "Many functions have no effects except the return value and their return value depends only on the parameters and/or global variables. Such a function can be subject to common subexpression elimination and loop optimization just as an arithmetic operator would be."
 */
#if defined(WZ_CC_GNU) && !defined(WZ_CC_INTEL) && WZ_CC_GNU_PREREQ(2,96)
#  define WZ_DECL_PURE __attribute__((__pure__))
#else
#  define WZ_DECL_PURE
#endif


/*!
 * \def WZ_DECL_CONST
 * "Many functions do not examine any values except their arguments, and have no effects except the return value. Basically this is just slightly more strict class than the pure attribute below, since function is not allowed to read global memory."
 */
#if defined(WZ_CC_GNU) && !defined(WZ_CC_INTEL) && WZ_CC_GNU_PREREQ(2,5)
#  define WZ_DECL_CONST __attribute__((__const__))
#else
#  define WZ_DECL_CONST
#endif


/*! \def WZ_DECL_FORMAT
 * "The format attribute specifies that a function takes printf, scanf, strftime or strfmon style arguments which should be type-checked against a format string."
 */
#if defined(WZ_CC_GNU) && !defined(WZ_CC_INTEL) && WZ_CC_GNU_PREREQ(2,5)
#  define WZ_DECL_FORMAT(archetype, string_index, first_to_check) \
          __attribute__((__format__ (archetype, string_index, first_to_check)))
#else
#  define WZ_DECL_FORMAT(archetype, string_index, first_to_check)
#endif


/* ---- Platform specific setup ---- */


#if defined(WZ_OS_WIN)
#  if defined(WZ_CC_MINGW)
#    include <unistd.h>
#    include <sys/param.h>
#    include <w32api.h>
#    define _WIN32_IE IE5
// Required for alloca
#    include <malloc.h>

#  elif defined(WZ_CC_MSVC)
#    define _CRT_SECURE_NO_DEPRECATE
#    if defined(_DEBUG)
#      define DEBUG
#      define _CRTDBG_MAP_ALLOC
#      include <stdlib.h>
#      include <crtdbg.h>
#    endif /* _DEBUG */
#  endif /* WZ_CC_* */

#  define WIN32_LEAN_AND_MEAN
#  define WIN32_EXTRA_LEAN
#  include <windows.h>

#  if defined(WZ_CC_MSVC)
#    define snprintf _snprintf
#    define strcasecmp _stricmp
#    define strncasecmp _strnicmp
#    define inline __inline
#    define alloca _alloca
#    define fileno _fileno
#    define isfinite _finite
#  endif /* WZ_CC_MSVC */

#elif defined(WZ_OS_UNIX)
#  include <unistd.h>
#  include <limits.h>
#  include <alloca.h>
#  define MAX_PATH PATH_MAX
#endif /* WZ_OS_* */


#endif /* WZGLOBAL_H */
