// Read/write an INI file into easy-to-access name/value pairs.

// SPDX-License-Identifier: BSD-3-Clause

// Originally based on: IniReader (https://github.com/benhoyt/inih)

// Copyright (C) 2009-2020, Ben Hoyt
// Copyright (C) 2023 Warzone 2100 Project

#include <algorithm>
#include <cctype>
#include <cstdlib>
#ifndef INI_API
# define INI_API
#endif
#include <inih/ini.h>
#include "INIReaderWriter.h"

using std::string;

namespace INIDetail
{
	inline unsigned char ascii_tolower(unsigned char c)
	{
		if ('A' <= c && c <= 'Z')
		{
			c += 'a' - 'A';
		}
		return c;
	}

	int ascii_strcasecmp(const char* s1, const char* s2)
	{
		const unsigned char *us1 = (const unsigned char *)s1;
		const unsigned char *us2 = (const unsigned char *)s2;

		while (ascii_tolower(*us1) == ascii_tolower(*us2++))
			if (*us1++ == '\0')
				return 0;

		return ascii_tolower(*us1) - ascii_tolower(*--us2);
	}
}

INIReaderWriter::INIReaderWriter(const string& filename)
{
	_error = ini_parse(filename.c_str(), ValueHandler, this);
	_loaded = true;
}

INIReaderWriter::INIReaderWriter(const char *buffer, size_t buffer_size)
{
	string content(buffer, buffer_size);
	_error = ini_parse_string(content.c_str(), ValueHandler, this);
	_loaded = true;
}

INIReaderWriter::INIReaderWriter()
{
	_error = 0;
	_loaded = false;
}

int INIReaderWriter::ParseError() const
{
	return _error;
}

bool INIReaderWriter::LoadedFromData() const
{
	return _loaded;
}

void INIReaderWriter::IniSection::dumpTo(std::string& outputStr, std::string endl /*= "\n"*/) const
{
	for (const auto& i : _values)
	{
		outputStr += i.first + "=" + i.second + endl;
	}
}

bool INIReaderWriter::IniSection::HasValue(const std::string& name) const
{
	auto it = _values.find(name);
	return it != _values.end();
}

string INIReaderWriter::IniSection::Get(const string& name, const string& default_value) const
{
	auto it = _values.find(name);
	if (it == _values.end())
	{
		return default_value;
	}
	return it->second;
}

string INIReaderWriter::IniSection::GetString(const string& name, const string& default_value) const
{
	const string str = Get(name, "");
	return str.empty() ? default_value : str;
}

long INIReaderWriter::IniSection::GetInteger(const string& name, long default_value) const
{
	string valstr = Get(name, "");
	const char* value = valstr.c_str();
	char* end;
	// This parses "1234" (decimal) and also "0x4D2" (hex)
	long n = strtol(value, &end, 0);
	return end > value ? n : default_value;
}

int64_t INIReaderWriter::IniSection::GetInteger64(const std::string& name, int64_t default_value) const
{
	string valstr = Get(name, "");
	const char* value = valstr.c_str();
	char* end;
	// This parses "1234" (decimal) and also "0x4D2" (hex)
	int64_t n = strtoll(value, &end, 0);
	return end > value ? n : default_value;
}

unsigned long INIReaderWriter::IniSection::GetUnsigned(const string& name, unsigned long default_value) const
{
	string valstr = Get(name, "");
	const char* value = valstr.c_str();
	char* end;
	// This parses "1234" (decimal) and also "0x4D2" (hex)
	unsigned long n = strtoul(value, &end, 0);
	return end > value ? n : default_value;
}

uint64_t INIReaderWriter::IniSection::GetUnsigned64(const std::string& name, uint64_t default_value) const
{
	string valstr = Get(name, "");
	const char* value = valstr.c_str();
	char* end;
	// This parses "1234" (decimal) and also "0x4D2" (hex)
	uint64_t n = strtoull(value, &end, 0);
	return end > value ? n : default_value;
}

double INIReaderWriter::IniSection::GetReal(const string& name, double default_value) const
{
	string valstr = Get(name, "");
	const char* value = valstr.c_str();
	char* end;
	double n = strtod(value, &end);
	return end > value ? n : default_value;
}

bool INIReaderWriter::IniSection::GetBoolean(const string& name, bool default_value) const
{
	string valstr = Get(name, "");
	// Convert to lower case to make string comparisons case-insensitive
	std::transform(valstr.begin(), valstr.end(), valstr.begin(),
				   [](const unsigned char& ch) { return static_cast<unsigned char>(::tolower(ch)); });
    if (valstr == "true" || valstr == "yes" || valstr == "on" || valstr == "1")
		return true;
    else if (valstr == "false" || valstr == "no" || valstr == "off" || valstr == "0")
		return false;
    else
		return default_value;
}

// Set a string value in a section
void INIReaderWriter::IniSection::SetString(const std::string& name, const std::string& value)
{
	_values[name] = value;
}

void INIReaderWriter::IniSection::SetBool(const string& name, bool value)
{
	SetString(name, (value) ? "true" : "false");
}

INIReaderWriter::IniSection& INIReaderWriter::GetSection(const std::string& section)
{
	return _sections[section];
}

bool INIReaderWriter::HasSection(const string& section) const
{
	return _sections.find(section) != _sections.end();
}

bool INIReaderWriter::HasValue(const string& section, const string& name) const
{
	auto it = _sections.find(section);
	if (it == _sections.end())
	{
		return false;
	}
    return it->second.HasValue(name);
}

int INIReaderWriter::ValueHandler(void* user, const char* section, const char* name,
                            const char* value)
{
	if (!name)  // Happens when INI_CALL_HANDLER_ON_NEW_SECTION enabled
		return 1;
	INIReaderWriter* reader = static_cast<INIReaderWriter*>(user);
	auto& iniSection = reader->_sections[(section) ? section : ""];
	iniSection.SetString((name) ? name : "", value ? value : "");
	return 1;
}

// Dump the current ini state to a string (ex. for output to a file)
std::string INIReaderWriter::dump() const
{
#ifdef _WIN32
	const std::string endl = "\r\n";
#else
	const std::string endl = "\n";
#endif

	std::string result;
	result.reserve(2048);

	for (const auto& section : _sections)
	{
		result += std::string("[") + section.first + "]" + endl;
		section.second.dumpTo(result, endl);
	}

	return result;
}
