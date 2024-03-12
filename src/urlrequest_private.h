/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2022  Warzone 2100 Project

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
