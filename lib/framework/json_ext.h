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

// Whether the arrays and objects in the JSON text [begin, end) nest no deeper than maxDepth (string contents are skipped)
bool jsonNestingWithinLimit(const char *begin, const char *end, size_t maxDepth);

#endif // _LIB_FRAMEWORK_JSON_EXT_H
