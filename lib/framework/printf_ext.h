/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007  Warzone Resurrection Project

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

#ifndef __INCLUDE_LIB_FRAMEWORK_PRINTF_EXT_H__
#define __INCLUDE_LIB_FRAMEWORK_PRINTF_EXT_H__

#include "frame.h"

#if defined(WZ_OS_WIN)
// These functions are GNU extensions; so make sure they are available on Windows also
extern int vasprintf(char** strp, const char* format, va_list ap);
extern int asprintf(char** strp, const char* format, ...);
#endif

#if defined(WZ_CC_MSVC)
// Make sure that these functions are available, and work according to the C99 spec on MSVC also
extern int vsnprintf(char* str, size_t size, const char* format, va_list ap);
extern int snprintf(char* str, size_t size, const char* format, ...);
#endif

#endif // __INCLUDE_LIB_FRAMEWORK_PRINTF_EXT_H__
