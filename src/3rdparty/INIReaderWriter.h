// Read / write an INI file into easy-to-access name/value pairs.

// SPDX-License-Identifier: BSD-3-Clause

// Originally based on: IniReader (https://github.com/benhoyt/inih)

// Copyright (C) 2009-2020, Ben Hoyt
// Copyright (C) 2023 Warzone 2100 Project

#ifndef INIREADER_H
#define INIREADER_H

#include <map>
#include <string>
#include <cstdint>

namespace INIDetail
{
	int ascii_strcasecmp(const char* s1, const char* s2);

	struct CaseInsensitiveComparator
	{
		bool operator()(const std::string& a, const std::string& b) const noexcept
		{
			return INIDetail::ascii_strcasecmp(a.c_str(), b.c_str()) < 0;
		}
	};
}

class INIReaderWriter
{
public:
	class IniSection
	{
	public:
		bool HasValue(const std::string& name) const;

	public: // GETTERS
		// Get a string value from INI file, returning default_value if not found.
		std::string Get(const std::string& name,
						const std::string& default_value) const;

		// Get a string value from INI file, returning default_value if not found,
		// empty, or contains only whitespace.
		std::string GetString(const std::string& name,
						const std::string& default_value) const;

		// Get an integer (long) value from INI file, returning default_value if
		// not found or not a valid integer (decimal "1234", "-1234", or hex "0x4d2").
		long GetInteger(const std::string& name, long default_value) const;

		// Get a 64-bit integer (int64_t) value from INI file, returning default_value if
		// not found or not a valid integer (decimal "1234", "-1234", or hex "0x4d2").
		int64_t GetInteger64(const std::string& name, int64_t default_value) const;

		// Get an unsigned integer (unsigned long) value from INI file, returning default_value if
		// not found or not a valid unsigned integer (decimal "1234", or hex "0x4d2").
		unsigned long GetUnsigned(const std::string& name, unsigned long default_value) const;

		// Get an unsigned 64-bit integer (uint64_t) value from INI file, returning default_value if
		// not found or not a valid unsigned integer (decimal "1234", or hex "0x4d2").
		uint64_t GetUnsigned64(const std::string& name, uint64_t default_value) const;

		// Get a real (floating point double) value from INI file, returning
		// default_value if not found or not a valid floating point value
		// according to strtod().
		double GetReal(const std::string& name, double default_value) const;

		// Get a boolean value from INI file, returning default_value if not found or if
		// not a valid true/false value. Valid true values are "true", "yes", "on", "1",
		// and valid false values are "false", "no", "off", "0" (not case sensitive).
		bool GetBoolean(const std::string& name, bool default_value) const;

	public: // SETTERS

		// Set a string value in a section
		void SetString(const std::string& name, const std::string& value);

		void SetBool(const std::string& name, bool value);

	public:

		// old WZ compat functions
		inline bool has(const std::string& name) const { return HasValue(name); }

	public:

		void dumpTo(std::string& outputStr, std::string endl = "\n") const;

	private:

		typedef std::map<std::string, std::string, INIDetail::CaseInsensitiveComparator> ValuesMap;
		ValuesMap _values;
	};
public:
	// Construct INIReaderWriter and parse given filename. See ini.h for more info
	// about the parsing.
	explicit INIReaderWriter(const std::string& filename);

	// Construct INIReaderWriter and parse given buffer. See ini.h for more info
	// about the parsing.
	explicit INIReaderWriter(const char *buffer, size_t buffer_size);

	// Construct a blank IniReaderWriter (usually for writing)
	INIReaderWriter();

	// Return the result of ini_parse(), i.e., 0 on success, line number of
	// first error on parse error, or -1 on file open error.
	int ParseError() const;

	// Whether this IniReaderWriter was initialized from loaded data (returns true), or a fresh blank instance (returns false)
	bool LoadedFromData() const;

	IniSection& GetSection(const std::string& section);

	// Return true if the given section exists (section must contain at least
	// one name=value pair).
	bool HasSection(const std::string& section) const;

	// Return true if a value exists with the given section and field names.
	bool HasValue(const std::string& section, const std::string& name) const;

	// Dump the current ini state to a string (ex. for output to a file)
	std::string dump() const;

private:
	int _error;
	bool _loaded = false;

	struct CaseInsensitiveComparator
	{
		bool operator()(const std::string& a, const std::string& b) const noexcept
		{
			return INIDetail::ascii_strcasecmp(a.c_str(), b.c_str()) < 0;
		}
	};

	std::map<std::string, IniSection, INIDetail::CaseInsensitiveComparator> _sections;
	static int ValueHandler(void* user, const char* section, const char* name,
							const char* value);
};

#endif  // INIREADER_H
