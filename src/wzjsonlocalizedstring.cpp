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

#include "wzjsonlocalizedstring.h"
#include "lib/framework/frame.h"
#include "lib/framework/i18n.h"

#include <nlohmann/json.hpp>

void from_json(const nlohmann::json& j, WzJsonLocalizedString& v)
{
	if (j.is_object())
	{
		for (auto it : j.items())
		{
			v.mappings.push_back({it.key(), it.value().get<std::string>()});
		}
	}
	else if (j.is_string())
	{
		v.mappings.push_back({"en", j.get<std::string>()});
	}
	else
	{
		throw nlohmann::json::type_error::create(302, "type must be an object or string, but is " + std::string(j.type_name()), &j);
	}
}

optional<std::string> WzJsonLocalizedString::getLocalizedString() const
{
	const char* pLangCode = getLanguage();
	std::string currentLanguage = (pLangCode) ? pLangCode : "en";

	auto findMapping = [&](const std::string& languageCode) {
		return std::find_if(mappings.begin(), mappings.end(), [languageCode](const TranslationMapping& m) -> bool {
			return m.languageCode == languageCode;
		});
	};

	auto it = findMapping(currentLanguage);
	if (it == mappings.end())
	{
		// fall back to "en" if available
		it = findMapping("en");
		if (it == mappings.end())
		{
			// just use the first entry, if available
			it = mappings.begin();
		}
	}

	if (it == mappings.end())
	{
		return nullopt;
	}

	return it->str;
}
