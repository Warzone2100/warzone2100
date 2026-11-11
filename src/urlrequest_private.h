// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>
#include <cctype>
#include <unordered_map>
#include <algorithm>

class CaseInsensitiveHash
{
public:
	size_t operator() (std::string key) const
	{
		std::transform(key.begin(), key.end(), key.begin(), [](char c) { return std::tolower(c); });
		return std::hash<std::string>{}(key);
	}
};
class CaseInsensitiveEqualFunc
{
public:
	bool operator() (std::string lhs, std::string rhs) const
	{
		std::transform(lhs.begin(), lhs.end(), lhs.begin(), [](char c) { return std::tolower(c); });
		std::transform(rhs.begin(), rhs.end(), rhs.begin(), [](char c) { return std::tolower(c); });
		return lhs == rhs;
	}
};

typedef std::unordered_map<std::string, std::string, CaseInsensitiveHash, CaseInsensitiveEqualFunc> ResponseHeaderContainer;

class HTTPResponseHeadersContainer: public HTTPResponseHeaders {
public:
	virtual bool hasHeader(const std::string& name) const override;
	virtual bool getHeader(const std::string& name, std::string& output_value) const override;
public:
	ResponseHeaderContainer responseHeaders;
};

inline void trim_str(std::string& str)
{
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int c) {
		return !std::isspace(c);
	}));
	str.erase(std::find_if(str.rbegin(), str.rend(), [](int c) {
		return !std::isspace(c);
	}).base(), str.end());
}
