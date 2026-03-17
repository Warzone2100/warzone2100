/*
 *	This file is part of Warzone 2100.
 *	Copyright (C) 2018-2026  Warzone 2100 Project
 *
 *	Warzone 2100 is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Warzone 2100 is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with Warzone 2100; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _LIB_FRAMEWORK_WZSTRING_JSON_H
#define _LIB_FRAMEWORK_WZSTRING_JSON_H

#include "lib/framework/wzstring.h"

#if (defined(WZ_OS_WIN) && defined(WZ_CC_MINGW))
#  if (defined(snprintf) && !defined(_GL_STDIO_H) && defined(_LIBINTL_H))
// On mingw / MXE builds, libintl's define of snprintf breaks json.hpp
// So undef it here and restore it later (HACK)
#    define _wz_restore_libintl_snprintf
#    undef snprintf
#  endif
#endif

#if defined(__clang__) && defined(__has_cpp_attribute)
#  pragma clang diagnostic push
#  if __has_cpp_attribute(nodiscard)
#    // Work around json.hpp / Clang issue where Clang reports the nodiscard attribute is available in C++11 mode,
#    // but then `-Wc++17-extensions` warns that it is a C++17 feature
#    pragma clang diagnostic ignored "-Wc++1z-extensions" // -Wc++1z-extensions // -Wc++17-extensions
#  endif
#endif

#include <nlohmann/json.hpp>

#if defined(__clang__) && defined(__has_cpp_attribute)
#  pragma clang diagnostic pop
#endif

#if defined(_wz_restore_libintl_snprintf)
#  undef _wz_restore_libintl_snprintf
#  undef snprintf
#  define snprintf libintl_snprintf
#endif

// Enable JSON support for custom types

// WzString
inline void to_json(nlohmann::json& j, const WzString& p) {
	j = nlohmann::json(p.toUtf8());
}
inline void to_json(nlohmann::ordered_json& j, const WzString& p) {
	j = nlohmann::ordered_json(p.toUtf8());
}

inline void from_json(const nlohmann::json& j, WzString& p) {
	std::string str = j.get<std::string>();
	p = WzString::fromUtf8(str.c_str());
}
inline void from_json(const nlohmann::ordered_json& j, WzString& p) {
	std::string str = j.get<std::string>();
	p = WzString::fromUtf8(str.c_str());
}

#endif // _LIB_FRAMEWORK_WZSTRING_JSON_H
