/*
	This file is part of Warzone 2100.
	Copyright (C) 2024  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_WZJSONLOCALIZEDSTRING_H__
#define __INCLUDED_SRC_WZJSONLOCALIZEDSTRING_H__

#include <string>
#include <vector>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

#include <nlohmann/json_fwd.hpp>

class WzJsonLocalizedString
{
public:
	struct TranslationMapping
	{
		std::string languageCode;
		std::string str;
	};
	std::vector<TranslationMapping> mappings;
public:
	const char* getLocalizedString() const;
	const char* getLocalizedString(const char *domainname) const;
};

void from_json(const nlohmann::json& j, WzJsonLocalizedString& v);

#endif // __INCLUDED_SRC_WZJSONLOCALIZEDSTRING_H__
