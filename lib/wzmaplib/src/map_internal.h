/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2021  Warzone 2100 Project

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

#pragma once

#include "../include/wzmaplib/map_debug.h"
#include <cstdarg>
#include <cstdio>

namespace WzMap {

#if defined __GNUC__ && defined __GNUC_MINOR__
#  define MAPLIB_CC_GNU_PREREQ(maj, min) \
	((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#  define MAPLIB_CC_GNU_PREREQ(maj, min) 0
#endif

/*! \def MAPLIB_DECL_FORMAT
 * GCC: "The format attribute specifies that a function takes printf, scanf, strftime or strfmon
 *       style arguments which should be type-checked against a format string."
 */
#if MAPLIB_CC_GNU_PREREQ(2,5) && !defined(__INTEL_COMPILER)
#  define MAPLIB_DECL_FORMAT(archetype, string_index, first_to_check) \
	__attribute__((__format__(archetype, string_index, first_to_check)))
#else
#  define MAPLIB_DECL_FORMAT(archetype, string_index, first_to_check)
#endif

#define LOG_INFO_VERBOSE LoggingProtocol::LogLevel::Info_Verbose
#define LOG_INFO LoggingProtocol::LogLevel::Info
#define LOG_SYNTAX_WARNING LoggingProtocol::LogLevel::Warning
#define LOG_WARNING LoggingProtocol::LogLevel::Warning
#define LOG_ERROR LoggingProtocol::LogLevel::Error
template <unsigned N>
static inline int vssprintf(char (&dest)[N], char const *format, va_list params) { return vsnprintf(dest, N, format, params); }
#define debug(logger, part, ...) do { _printLog(logger, __LINE__, part, __FUNCTION__, __VA_ARGS__); } while(0)
#if defined(__GNUC__) && (defined(__MINGW32__) || defined(__MINGW64__))
void _printLog(WzMap::LoggingProtocol* logger, int line, WzMap::LoggingProtocol::LogLevel level, const char *function, const char *str, ...) MAPLIB_DECL_FORMAT(__MINGW_PRINTF_FORMAT, 5, 6);
#else
void _printLog(WzMap::LoggingProtocol* logger, int line, WzMap::LoggingProtocol::LogLevel level, const char *function, const char *str, ...) MAPLIB_DECL_FORMAT(printf, 5, 6);
#endif

} // namespace WzMap
