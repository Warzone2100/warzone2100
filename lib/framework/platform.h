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
#    if defined()_DEBUG)
#      define DEBUG
#      define _CRTDBG_MAP_ALLOC
#      include <stdlib.h>
#      include <crtdbg.h>
#    endif /* _DEBUG */
#  endif /* WZ_CC_MSVC */

#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>

#  if defined(WZ_CC_MSVC)
#    define strcasecmp _stricmp
#    define strncasecmp _strnicmp
#    define strlwr _strlwr
#    define vsnprintf _vsnprintf
#    define snprintf  _snprintf
#    define fileno _fileno
#    define inline __inline
#  endif /* WZ_CC_MSVC */

#endif /* WZ_OS_WIN */


#if defined(WZ_OS_UNIX)

#  include <stdio.h>

#  define MAX_PATH 260
#  define fopen unix_fopen

char *unix_path(const char *path);
FILE *unix_fopen(const char *filename, const char *mode);

#endif /* WZ_OS_UNIX */


#endif /* PLATFORM_H */
