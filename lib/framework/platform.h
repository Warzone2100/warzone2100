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
/*! \file platform.h
 *  \brief Platform specific workarounds, defines and compat fixes
*/

#ifndef PLATFORM_H
#define PLATFORM_H

#include "wzglobal.h"

#if defined(WZ_OS_WIN)

#  if defined(WZ_CC_GNU)
#    include <w32api.h>
#    define _WIN32_IE IE5
#  endif /* WZ_CC_GNU */

#  if defined(WZ_CC_MSVC)
#    if defined(_DEBUG)
#      define DEBUG
#      define _CRTDBG_MAP_ALLOC
#      include <stdlib.h>
#      include <crtdbg.h>
#    endif /* _DEBUG */
#  endif /* WZ_CC_MSVC */

#  define WIN32_LEAN_AND_MEAN
#  define WIN32_EXTRA_LEAN
#  include <windows.h>

#  if defined(WZ_CC_MSVC)
#    define strcasecmp _stricmp
#    define strncasecmp _strnicmp
#    define vsnprintf _vsnprintf
#    define snprintf  _snprintf
#    define fileno _fileno
#    define inline __inline
#  endif /* WZ_CC_MSVC */

#elif defined(WZ_OS_MAC)

#  include <sys/syslimits.h>
#  define MAX_PATH PATH_MAX

#elif defined(WZ_OS_UNIX)

#  include <limits.h>
#  define MAX_PATH PATH_MAX

#endif /* WZ_OS_* */


#endif /* PLATFORM_H */
