/*
	This file is part of Warzone 2100.
	Copyright (C) 2020  Warzone 2100 Project

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
/*! \file demangle.hpp
 *  \brief Demangle type names
 */
#ifndef WZ_DEMANGLE_H
#define WZ_DEMANGLE_H

#if defined(__GNUG__) || defined(__clang__)

// See: https://gcc.gnu.org/onlinedocs/libstdc++/manual/ext_demangling.html

#include <memory>
#include <cxxabi.h>

inline std::string cxxDemangle(const char* typeName)
{
	if (!typeName) { return std::string(); }

	int status = -1;
	char* pResultName = abi::__cxa_demangle(typeName, nullptr, nullptr, &status);

	auto deallocFunc = [](char *pt) {
		if (!pt) { return; }
		std::free((void*)pt);
	};
	std::unique_ptr<char, decltype(deallocFunc)> result(pResultName, deallocFunc);

	return (status == 0 && result != nullptr) ? std::string(result.get()) : std::string(typeName);
}

#else

// No current implementation, so just return the original name

inline std::string cxxDemangle(const char* typeName)
{
	if (!typeName) { return std::string(); }
	return std::string(typeName);
}

#endif

#endif // WZ_DEMANGLE_H
