/*
 *	This file is part of Warzone 2100.
 *	Copyright (C) 2018  Warzone 2100 Project
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

#ifndef _LIB_FRAMEWORK_WZSTRING_H
#define _LIB_FRAMEWORK_WZSTRING_H

#include <string>
#include <vector>
#include <locale>

// Stores a unicode codepoint
// Internally, this stores the codepoint as UTF-32
class WzUniCodepoint {
public:
	static WzUniCodepoint fromUTF32(uint32_t utf_32_codepoint)
	{
		return WzUniCodepoint(utf_32_codepoint);
	}
	static WzUniCodepoint fromASCII(unsigned char charLiteral);

	uint32_t UTF32() const { return _codepoint; }

	bool isNull() const { return _codepoint == 0; }

public:
	WzUniCodepoint& operator=(const WzUniCodepoint& ch) { _codepoint = ch._codepoint; return *this; }

protected:
	WzUniCodepoint(uint32_t utf_32_codepoint)
	: _codepoint(utf_32_codepoint)
	{ }

private:
	uint32_t _codepoint;
};

class WzString {
public:
	WzString() { }
	WzString(int size, const WzUniCodepoint& ch);
	WzString(WzString&& other) noexcept : _utf8String(std::move(other._utf8String)) { }
	WzString(const WzString& other) noexcept : _utf8String(other._utf8String) { }
	WzString(const char * str, int size = -1);

	static WzString fromUtf8(const char *str, int size = -1);
	static WzString fromUtf8(const std::string &str);
	static WzString fromUtf16(const std::vector<uint16_t>& utf16);
	static WzString fromCodepoint(const WzUniCodepoint& codepoint);

	const std::string& toUtf8() const;

	// Same as `toUtf8()`
	const std::string& toStdString() const;

	const std::vector<uint16_t> toUtf16() const;
	const std::vector<uint32_t> toUtf32() const;

	int toInt(bool *ok = nullptr, int base = 10) const;

	bool isEmpty() const;

	// Returns the number of Unicode codepoints in this string.
	int length() const;

	// Same as `length()`
	int size() const { return length(); }

	const WzUniCodepoint at(int position) const;

	WzString& append(const WzString &str);
	WzString& append(const WzUniCodepoint &c);
	WzString& append(const char* str);

	WzString& insert(size_t position, const WzString &str);
	WzString& insert(size_t i, WzUniCodepoint c);
	WzString& remove(size_t i, int len);

	WzString& replace(size_t position, int n, const WzUniCodepoint &after);
	WzString& replace(const WzUniCodepoint &before, const WzUniCodepoint &after);
	WzString& replace(const WzUniCodepoint &before, const WzString &after);
	WzString& replace(const WzString &before, const WzString &after);

	void truncate(int position);
	void clear();

	WzString toLower() const;
	WzString trimmed(const std::locale &loc = std::locale::classic()) const;

	std::vector<WzString> split(const WzString &delimiter) const;

public:
	// Create from numbers
	static WzString number(int32_t n);
	static WzString number(uint32_t n);
	static WzString number(int64_t n);
	static WzString number(uint64_t n);
	static WzString number(double n);

	// Useful when padding is needed
	WzString& leftPadToMinimumLength(const WzUniCodepoint &ch, size_t minimumStringLength);

public:
	WzString& operator+=(const WzString &other);
	WzString& operator+=(const WzUniCodepoint &ch);
	WzString& operator+=(const char* str);
	WzString& operator=(const WzString &other);
	WzString& operator=(const WzUniCodepoint& ch);
	WzString& operator=(WzString&& other);

	const WzString operator+(const WzString &other) const;
	const WzString operator+(const char* other) const;

	bool operator==(const WzString &other) const;
	bool operator!=(const WzString &other) const;
	bool operator < (const WzString& str) const;

	int compare(const WzString &other) const;
	int compare(const char *other) const;

	bool startsWith(const WzString &other) const;
	bool startsWith(const char* other) const;
	bool endsWith(const WzString &other) const;

	bool contains(const WzUniCodepoint &codepoint) const;
	bool contains(const WzString &other) const;

	// Used to expose a modifiable "view" of a WzUniCodepoint inside a WzString
	class WzUniCodepointRef {
	public:
		WzUniCodepointRef(uint32_t utf_32_codepoint, WzString& parentString, int position)
		: _codepoint(WzUniCodepoint::fromUTF32(utf_32_codepoint)),
		_parentString(parentString),
		_position(position)
		{ }
	public:
		WzUniCodepointRef& operator=(const WzUniCodepoint& ch);
	private:
		WzUniCodepoint _codepoint;
		WzString& _parentString;
		const int _position;
	};

	WzUniCodepointRef operator[](int position);

private:
	WzString(std::string utf8String)
	: _utf8String(utf8String)
	{ }

	template <typename octet_iterator, typename distance_type>
	bool _utf8_advance (octet_iterator& it, distance_type n, octet_iterator end) const;

	static bool isValidUtf8(const char * str, size_t len);

private:
	std::string _utf8String;
};

// NOTE: The char * should be valid UTF-8.
inline WzString operator+ (const char* a, const WzString &b)
{
	return WzString::fromUtf8(a) + b;
}

namespace std
{
	template <>
	struct hash<WzString>
	{
		typedef std::size_t result_type;
		result_type operator()( const WzString& k ) const
		{
			// Simply use the hash of the internal _utf8String
			return std::hash<std::string>{}(k.toUtf8());
		}
	};
}

#endif // _LIB_FRAMEWORK_WZSTRING_H
