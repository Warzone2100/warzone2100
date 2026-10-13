/*
 *	This file is part of Warzone 2100.
 *	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)
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

#ifndef _LIB_FRAMEWORK_JSON_EXT_H
#define _LIB_FRAMEWORK_JSON_EXT_H

#include <cstddef>

#include <nlohmann/json_fwd.hpp>

// Whether the arrays and objects in the JSON text [begin, end) nest no deeper than maxDepth (string contents are skipped)
bool jsonNestingWithinLimit(const char *begin, const char *end, size_t maxDepth);

// Parse the JSON text [begin, end) into an ordered_json, as nlohmann::ordered_json::parse(begin, end) does
// (same result and exceptions, and for a duplicate key, the last value at the first one's place),
// but building each object at its final size once its members are parsed.
// (An ordered_json object is a vector of pair<const key, value>, which can't be moved,
// so ordered_json::parse copies every member so far each time an object grows.)
nlohmann::ordered_json parseOrderedJson(const char *begin, const char *end);

#endif // _LIB_FRAMEWORK_JSON_EXT_H
