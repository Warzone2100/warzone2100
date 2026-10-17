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

#include "json_ext.h"

#include <nlohmann/json.hpp>

#include <utility>
#include <vector>

namespace
{

// Keeps the members of each open object (and the elements of each open array) in a vector of movable values,
// and builds the object at its end, at its final size
class OrderedJsonBuilder
{
public:
	using json = nlohmann::ordered_json;
	using number_integer_t = json::number_integer_t;
	using number_unsigned_t = json::number_unsigned_t;
	using number_float_t = json::number_float_t;
	using string_t = json::string_t;
	using binary_t = json::binary_t;

	explicit OrderedJsonBuilder(json &root) : root(root) {}

	bool null() { return value(nullptr); }
	bool boolean(bool v) { return value(v); }
	bool number_integer(number_integer_t v) { return value(v); }
	bool number_unsigned(number_unsigned_t v) { return value(v); }
	bool number_float(number_float_t v, const string_t &) { return value(v); }
	bool string(string_t &v) { return value(std::move(v)); }
	bool binary(binary_t &v) { return value(json::binary(std::move(v))); }

	bool start_object(std::size_t = static_cast<std::size_t>(-1))
	{
		open.emplace_back();
		open.back().isObject = true;
		return true;
	}
	bool key(string_t &k)
	{
		open.back().key = std::move(k);
		return true;
	}
	bool end_object()
	{
		Container c = std::move(open.back());
		open.pop_back();
		json object = json::object();
		auto &members = object.get_ref<json::object_t &>();
		members.reserve(c.members.size());
		for (auto &member : c.members)
		{
			// (operator[]: a duplicate key's last value wins, at the first one's place, as with ordered_json::parse)
			members[std::move(member.first)] = std::move(member.second);
		}
		return value(std::move(object));
	}
	bool start_array(std::size_t = static_cast<std::size_t>(-1))
	{
		open.emplace_back();
		return true;
	}
	bool end_array()
	{
		Container c = std::move(open.back());
		open.pop_back();
		json array = json::array();
		array.get_ref<json::array_t &>() = std::move(c.elements);
		return value(std::move(array));
	}

	template<class Exception>
	[[noreturn]]
	bool parse_error(std::size_t, const std::string &, const Exception &ex)
	{
		throw ex;  // (as ordered_json::parse does)
	}

private:
	struct Container
	{
		bool isObject = false;
		string_t key;                                      ///< an object's key for the value being parsed
		std::vector<std::pair<string_t, json>> members;    ///< an object's members so far
		std::vector<json> elements;                        ///< an array's elements so far
	};

	template<class Value>
	bool value(Value &&v)
	{
		if (open.empty())
		{
			root = json(std::forward<Value>(v));
		}
		else if (open.back().isObject)
		{
			Container &c = open.back();
			c.members.emplace_back(std::move(c.key), json(std::forward<Value>(v)));
		}
		else
		{
			open.back().elements.emplace_back(std::forward<Value>(v));
		}
		return true;
	}

	json &root;
	std::vector<Container> open;
};

} // anonymous namespace

nlohmann::ordered_json parseOrderedJson(const char *begin, const char *end)
{
	nlohmann::ordered_json result;
	OrderedJsonBuilder builder(result);
	nlohmann::ordered_json::sax_parse(begin, end, &builder);
	return result;
}

bool jsonNestingWithinLimit(const char *begin, const char *end, size_t maxDepth)
{
	size_t depth = 0;
	bool inString = false;
	for (const char *p = begin; p < end; ++p)
	{
		const char c = *p;
		if (inString)
		{
			if (c == '\\' && p + 1 < end)
			{
				++p;
			}
			else if (c == '"')
			{
				inString = false;
			}
		}
		else if (c == '"')
		{
			inString = true;
		}
		else if (c == '[' || c == '{')
		{
			if (++depth > maxDepth)
			{
				return false;
			}
		}
		else if ((c == ']' || c == '}') && depth > 0)
		{
			--depth;
		}
	}
	return true;
}
