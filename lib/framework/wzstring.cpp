/*
 *	This file is part of Warzone 2100.
 *	Copyright (C) 2018-2019  Warzone 2100 Project
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

#include "frame.h"
#include "wzstring.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <utfcpp/source/utf8.h>
#include <utf8proc/utf8proc.h>

WzUniCodepoint WzUniCodepoint::fromASCII(unsigned char charLiteral)
{
	uint8_t* p_utf8 = &charLiteral;
	if ((charLiteral >> 7) != 0)
	{
		ASSERT(false, "Invalid character literal - only proper 7-bit ASCII is supported");
		return WzUniCodepoint(0);
	}

	// This should not throw an exception, since we check for valid 7-bit ASCII above.
	uint32_t codepoint = utf8::next(p_utf8, p_utf8 + 1);
	return WzUniCodepoint(codepoint);
}

std::vector<WzUniCodepoint> WzUniCodepoint::caseFolded() const
{
	#define CODEPOINT_CASEFOLDING_BUFF_SIZE 100
	utf8proc_int32_t dst[CODEPOINT_CASEFOLDING_BUFF_SIZE] = { 0 };
	utf8proc_ssize_t retVal = utf8proc_decompose_char(_codepoint, dst, CODEPOINT_CASEFOLDING_BUFF_SIZE, UTF8PROC_CASEFOLD, nullptr);
	if (retVal < 0)
	{
		// case folding failed
		ASSERT(retVal > 0, "Case folding attempt failed with error code: %ld", retVal);
		return std::vector<WzUniCodepoint>();
	}
	std::vector<WzUniCodepoint> resultCodepoints;
	resultCodepoints.reserve(retVal);
	for (utf8proc_ssize_t i = 0; i < retVal; ++i)
	{
		resultCodepoints.push_back(WzUniCodepoint(dst[i]));
	}
	return resultCodepoints;
}

WzString WzString::fromCodepoint(const WzUniCodepoint& codepoint)
{
	uint32_t utf32string[] = {codepoint.UTF32(), 0};
	std::string utf8result;
	utf8::utf32to8(utf32string, utf32string + 1, back_inserter(utf8result));
	return WzString(utf8result);
}

// Constructs a string of the given size with every codepoint set to ch.
WzString::WzString(size_t size, const WzUniCodepoint& ch)
{
	try {
		for (size_t i = 0; i < size; i++)
		{
			utf8::append(ch.UTF32(), back_inserter(_utf8String));
		}
	}
	catch (const std::exception &e) {
		// Likely an invalid codepoint
		ASSERT(false, "Encountered error parsing input codepoint: %s", e.what());
	}
}

// NOTE: The char * should be valid UTF-8.
WzString::WzString(const char * str, int size /*= -1*/)
{
	if (str == nullptr) { return; }
	if (size < 0)
	{
		size = strlen(str);
	}
	ASSERT(utf8::is_valid(str, str + size), "Input text is not valid UTF-8");
	try {
		utf8::replace_invalid(str, str + size, back_inserter(_utf8String), '?');
	}
	catch (const std::exception &e) {
		// Likely passed an incomplete UTF-8 sequence
		ASSERT(false, "Encountered error parsing input UTF-8 sequence: %s", e.what());
		_utf8String.clear();
	}
}

WzString WzString::fromUtf8(const char *str, int size /*= -1*/)
{
	return WzString(str, size);
}

WzString WzString::fromUtf8(const std::string &str)
{
	return WzString(str.c_str(), str.length());
}

WzString WzString::fromUtf16(const std::vector<uint16_t>& utf16)
{
	std::string utf8str;
	try {
		utf8::utf16to8(utf16.begin(), utf16.end(), back_inserter(utf8str));
	}
	catch (const std::exception &e) {
		ASSERT(false, "Conversion from UTF16 failed with error: %s", e.what());
		utf8str.clear();
	}
	return WzString(utf8str);
}

bool WzString::isValidUtf8(const char * str, size_t len)
{
	return utf8::is_valid(str, str + len);
}

const std::string& WzString::toUtf8() const
{
	return _utf8String;
}

const std::string& WzString::toStdString() const
{
	return toUtf8();
}

const std::vector<uint16_t> WzString::toUtf16() const
{
	std::vector<uint16_t> utf16result;
	utf8::utf8to16(_utf8String.begin(), _utf8String.end(), back_inserter(utf16result));
	return utf16result;
}

const std::vector<uint32_t> WzString::toUtf32() const
{
	std::vector<uint32_t> utf32result;
	utf8::utf8to32(_utf8String.begin(), _utf8String.end(), back_inserter(utf32result));
	return utf32result;
}

int WzString::toInt(bool *ok /*= nullptr*/, int base /*= 10*/) const
{
	int result = 0;
	try {
		result = std::stoi(_utf8String, 0, base);
		if (ok != nullptr)
		{
			*ok = true;
		}
	}
	catch (const std::exception&) {
		if (ok != nullptr)
		{
			*ok = false;
		}
		result = 0;
	}
	return result;
}

bool WzString::isEmpty() const
{
	return _utf8String.empty();
}

int WzString::length() const
{
	auto len_distance = utf8::distance(_utf8String.begin(), _utf8String.end());
	if (len_distance > std::numeric_limits<int>::max())
	{
		ASSERT(len_distance <= std::numeric_limits<int>::max(), "The length (in codepoints) of a WzString exceeds the max of the return type for length()");
		return std::numeric_limits<int>::max();
	}
	return (int)len_distance;
}

const WzUniCodepoint WzString::at(int position) const
{
	auto it = _utf8String.begin();
	_utf8_advance(it, position, _utf8String.end());
	ASSERT(it != _utf8String.end(), "Cannot find position in string prior to end of string.");
	return WzUniCodepoint::fromUTF32(utf8::next(it, _utf8String.end()));
}

WzString& WzString::append(const WzString &str)
{
	_utf8String.append(str._utf8String);
	return *this;
}
WzString& WzString::append(const WzUniCodepoint &c)
{
	utf8::append(c.UTF32(), back_inserter(_utf8String));
	return *this;
}
// NOTE: The char * should be valid UTF-8.
WzString& WzString::append(const char* str)
{
	_utf8String.append(WzString::fromUtf8(str)._utf8String);
	return *this;
}

WzString& WzString::insert(size_t position, const WzString &str)
{
	auto it = _utf8String.begin();
	_utf8_advance(it, position, _utf8String.end());
	if (it == _utf8String.end())
	{
		size_t distance = it - _utf8String.begin();
		if (distance > position)
		{
			// TODO: To match QString behavior, we need to extend the string?
			ASSERT(it != _utf8String.end(), "Cannot find position in string prior to end of string.");
		}
		// deliberately fall-through
	}
	_utf8String.insert(it, str._utf8String.begin(), str._utf8String.end());
	return *this;
}

WzString& WzString::insert(size_t i, WzUniCodepoint c)
{
	auto it = _utf8String.begin();
	_utf8_advance(it, i, _utf8String.end());
	if (it == _utf8String.end())
	{
		size_t distance = it - _utf8String.begin();
		if (distance > i)
		{
			// TODO: To match QString behavior, we need to extend the string?
			ASSERT(it != _utf8String.end(), "Cannot find position in string prior to end of string.");
		}
	}
	auto cUtf8Codepoints = WzString::fromCodepoint(c);
	_utf8String.insert(it, cUtf8Codepoints._utf8String.begin(), cUtf8Codepoints._utf8String.end());
	return *this;
}

// Returns the codepoint at the specified position in the string as a modifiable reference.
WzString::WzUniCodepointRef WzString::operator[](int position)
{
	auto it = _utf8String.begin();
	_utf8_advance(it, position, _utf8String.end());
	ASSERT(it != _utf8String.end(), "Specified position is outside of the string");
	return WzString::WzUniCodepointRef(utf8::peek_next(it, _utf8String.end()), *this, position);
}

// Removes n codepoints from the string, starting at the given position index, and returns a reference to the string.
WzString& WzString::remove(size_t i, int len)
{
	if (len <= 0) { return *this; }
	auto it = _utf8String.begin();
	_utf8_advance(it, i, _utf8String.end());
	if (it == _utf8String.end()) { return *this; }
	auto it_last = it;
	if (!_utf8_advance(it_last, len, _utf8String.end()))
	{
		// just truncate the string at position it
		_utf8String.resize(it - _utf8String.begin());
		return *this;
	}
	_utf8String.erase(it, it_last);
	return *this;
}

// Replaces n codepoints beginning at index position with the codepoint after and returns a reference to this string.
//
// Note: If the specified position index is within the string, but position + n goes outside the strings range,
//       then n will be adjusted to stop at the end of the string.
WzString& WzString::replace(size_t position, int n, const WzUniCodepoint &after)
{
	auto it_replace_start = _utf8String.begin();
	if (!_utf8_advance(it_replace_start, position, _utf8String.end()))
	{
		// position index is not within the string
		return *this;
	}
	auto it_replace_end = it_replace_start;
	_utf8_advance(it_replace_end, n, _utf8String.end());
	size_t numCodepoints = utf8::distance(it_replace_start, it_replace_end);
	auto cUtf8After = WzString::fromCodepoint(after);
	std::vector<unsigned char> cUtf8Codepoints;
	for (size_t i = 0; i < numCodepoints; i++)
	{
		cUtf8Codepoints.insert(cUtf8Codepoints.end(), cUtf8After._utf8String.begin(), cUtf8After._utf8String.end());
	}
	_utf8String.replace(it_replace_start, it_replace_end, cUtf8Codepoints.begin(), cUtf8Codepoints.end());
	return *this;
}

WzString& WzString::replace(const WzUniCodepoint &before, const WzUniCodepoint &after)
{
	WzString cUtf8Before = WzString::fromCodepoint(before);
	WzString cUtf8After = WzString::fromCodepoint(after);
	return replace(cUtf8Before, cUtf8After);
}

WzString& WzString::replace(const WzUniCodepoint &before, const WzString &after)
{
	WzString cUtf8Before = WzString::fromCodepoint(before);
	return replace(cUtf8Before, after);
}

WzString& WzString::replace(const WzString &before, const WzString &after)
{
	if (before._utf8String.empty()) { return *this; }
	std::string::size_type pos = 0;
	while((pos = _utf8String.find(before._utf8String, pos)) != std::string::npos) {
		_utf8String.replace(pos, before._utf8String.length(), after._utf8String);
		pos += after._utf8String.length();
	}
	return *this;
}

WzString& WzString::remove(const WzString &substr)
{
	return replace(substr, "");
}

void WzString::truncate(int position)
{
	if (position < 0) { position = 0; }
	if (position == 0)
	{
		_utf8String.clear();
		return;
	}
	auto it = _utf8String.begin();
	_utf8_advance(it, position, _utf8String.end());
	if (it != _utf8String.end())
	{
		_utf8String.resize(it - _utf8String.begin());
	}
}

void WzString::clear()
{
	_utf8String.clear();
}

// Returns a lowercase copy of the string.
// The case conversion will always happen in the 'C' locale.
WzString WzString::toLower() const
{
	std::string lowercasedUtf8String = _utf8String;
	for (auto &ch : lowercasedUtf8String)
	{
		ch = std::tolower(ch, std::locale::classic());
	}
	return WzString::fromUtf8(lowercasedUtf8String);
}

// Trim whitespace from start (in place)
static inline void ltrim(std::string &s, const std::locale &loc = std::locale::classic()) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [loc](char ch) {
		return !std::isspace(ch, loc);
	}));
}

// Trim whitespace from end (in place)
static inline void rtrim(std::string &s, const std::locale &loc = std::locale::classic()) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [loc](char ch) {
		return !std::isspace(ch, loc);
	}).base(), s.end());
}

// Returns a string that has whitespace removed from the start and the end.
// Whitespace is any character for which std::isspace returns true - for the specified locale.
// (The default locale is the "C" locale, which treats the following ASCII characters as whitespace: ' ', '\f', '\n', '\r', '\t', '\v')
WzString WzString::trimmed(const std::locale &loc /*= std::locale::classic()*/) const
{
	std::string utf8StringCopy = _utf8String;
	ltrim(utf8StringCopy, loc);
	rtrim(utf8StringCopy, loc);
	return WzString::fromUtf8(utf8StringCopy);
}

std::vector<WzString> WzString::split(const WzString & delimiter) const
{
	std::vector<WzString> tokens;
	const std::vector<uint32_t> delimiterUtf32Codepoints = delimiter.toUtf32();
	auto currentDelimiterCodepoint = delimiterUtf32Codepoints.begin();
	auto it = _utf8String.begin();
	auto it_currenttokenstart = _utf8String.begin();
	bool lastCodepointWasDelimiterEnd = false;
	uint32_t codepoint = 0;
	while (it != _utf8String.end())
	{
		codepoint = utf8::next(it, _utf8String.end());
		if (!delimiterUtf32Codepoints.empty())
		{
			if (codepoint != *currentDelimiterCodepoint)
			{
				currentDelimiterCodepoint = delimiterUtf32Codepoints.begin();
				lastCodepointWasDelimiterEnd = false;
			}
			else
			{
				++currentDelimiterCodepoint;
			}
		}

		if (currentDelimiterCodepoint == delimiterUtf32Codepoints.end())
		{
			tokens.push_back(WzString(std::string(it_currenttokenstart, it - delimiter._utf8String.length())));
			it_currenttokenstart = it;
			currentDelimiterCodepoint = delimiterUtf32Codepoints.begin();
			lastCodepointWasDelimiterEnd = true;
		}
	}

	if (it_currenttokenstart != _utf8String.end())
	{
		tokens.push_back(WzString(std::string(it_currenttokenstart, it)));
	}
	else if (lastCodepointWasDelimiterEnd && !delimiterUtf32Codepoints.empty())
	{
		// string ends with delimiter (and delimiter is not an empty string)
		// append an empty string to the end of the list
		tokens.push_back(WzString());
	}
	return tokens;
}

// MARK: - Normalization

WzString WzString::normalized(WzString::NormalizationForm mode) const
{
	static_assert(std::is_same<unsigned char, utf8proc_uint8_t>::value, "uint8_t is not unsigned char. This function requires this to avoid a violation of the strict aliasing rule.");
	// The reinterpret_cast from char* to const uint8_t* is only safe when std::uint8_t is *not* implemented as an extended unsigned integer type.
	// See: https://stackoverflow.com/a/16261758
	utf8proc_uint8_t *result = utf8proc_NFKD(reinterpret_cast<const utf8proc_uint8_t*>(_utf8String.c_str()));
	if (result == nullptr)
	{
		// Normalization failed
		debug(LOG_ERROR, "Unicode normalization failed for string: '%s'", _utf8String.c_str());
		return WzString();
	}
	WzString retVal = WzString::fromUtf8(reinterpret_cast<char *>(result));
	free(result);
	return retVal;
}

// MARK: - Create from numbers

WzString WzString::number(int n)
{
	WzString newString;
	newString._utf8String = std::to_string(n);
	return newString;
}

WzString WzString::number(unsigned int n)
{
	WzString newString;
	newString._utf8String = std::to_string(n);
	return newString;
}

WzString WzString::number(long n)
{
	WzString newString;
	newString._utf8String = std::to_string(n);
	return newString;
}

WzString WzString::number(unsigned long n)
{
	WzString newString;
	newString._utf8String = std::to_string(n);
	return newString;
}

WzString WzString::number(long long n)
{
	WzString newString;
	newString._utf8String = std::to_string(n);
	return newString;
}

WzString WzString::number(unsigned long long n)
{
	WzString newString;
	newString._utf8String = std::to_string(n);
	return newString;
}

WzString WzString::number(double n)
{
	WzString newString;
	// Note: Don't use std::to_string because it respects the current C locale
	// and we want locale-independent conversion (plus control over precision).
	std::stringstream ss;
	ss.imbue(std::locale::classic());
	ss << std::setprecision(std::numeric_limits<double>::digits10) << n;
	newString._utf8String = ss.str();
	return newString;
}

// Left-pads the current string with codepoint ch up to the minimumStringLength
// If the current string length() is already >= minimumStringLength, no padding occurs.
WzString& WzString::leftPadToMinimumLength(const WzUniCodepoint &ch, size_t minimumStringLength)
{
	if (length() >= minimumStringLength)
	{
		return *this;
	}

	size_t leftPaddingRequired = minimumStringLength - length();
	const WzString chUtf8 = WzString::fromCodepoint(ch);
	WzString utf8Padding;
	for (size_t i = 0; i < leftPaddingRequired; i++)
	{
		utf8Padding._utf8String.insert(utf8Padding._utf8String.end(), chUtf8._utf8String.begin(), chUtf8._utf8String.end());
	}
	insert(0, utf8Padding);
	return *this;
}

// MARK: - Operators

WzString& WzString::operator+=(const WzString &other)
{
	append(other);
	return *this;
}

WzString& WzString::operator+=(const WzUniCodepoint &ch)
{
	append(ch);
	return *this;
}

WzString& WzString::operator+=(const char* str)
{
	append(str);
	return *this;
}

const WzString WzString::operator+(const WzString &other) const
{
	WzString newString = *this;
	newString.append(other);
	return newString;
}

// NOTE: The char * should be valid UTF-8.
const WzString WzString::operator+(const char* other) const
{
	WzString newString = *this;
	newString.append(other);
	return newString;
}

WzString& WzString::operator=(const WzString &other)
{
	_utf8String = other._utf8String;
	return *this;
}

WzString& WzString::operator=(const WzUniCodepoint& ch)
{
	_utf8String.clear();
	append(ch);
	return *this;
}

WzString& WzString::operator=(WzString&& other)
{
	if (this != &other)
	{
		_utf8String = std::move(other._utf8String);
		other._utf8String.clear();
	}
	return *this;
}

bool WzString::operator==(const WzString &other) const
{
	return _utf8String == other._utf8String;
}

bool WzString::operator!=(const WzString &other) const
{
	return ! (*this == other);
}

bool WzString::operator < (const WzString& str) const
{
	return (_utf8String < str._utf8String);
}

int WzString::compare(const WzString &other) const
{
	if (_utf8String < other._utf8String) { return -1; }
	else if (_utf8String == other._utf8String) { return 0; }
	else { return 1; }
}

int WzString::compare(const char *other) const
{
	auto first1 = _utf8String.begin();
	auto last1 = _utf8String.end();
	auto first2 = other;
	while (first1 != last1)
	{
		if (*first2 == 0 || *first2 < *first1) return 1;
		else if (*first1 < *first2) return -1;
		++first1; ++first2;
	}
	if (*first2 == 0)
	{
		// reached the end of 1st and 2nd strings - they are equal
		return 0;
	}
	else
	{
		// reached the end of the 1st string, but not the 2nd
		return -1;
	}
}

bool WzString::startsWith(const WzString &other) const
{
	return _utf8String.compare(0, other._utf8String.length(), other._utf8String) == 0;
}

bool WzString::startsWith(const char* other) const
{
	// NOTE: This currently assumes that the char* is UTF-8-encoded.
	return _utf8String.compare(0, strlen(other), other) == 0;
}

bool WzString::endsWith(const WzString &other) const
{
	if (_utf8String.length() >= other._utf8String.length())
	{
		return _utf8String.compare(_utf8String.length() - other._utf8String.length(), other._utf8String.length(), other._utf8String) == 0;
	}
	else
	{
		return false;
	}
}

bool WzString::contains(const WzUniCodepoint &codepoint) const
{
	return contains(WzString::fromCodepoint(codepoint));
}

bool WzString::contains(const WzString &other) const
{
	return _utf8String.find(other._utf8String) != std::string::npos;
}

template <typename octet_iterator, typename distance_type>
bool WzString::_utf8_advance (octet_iterator& it, distance_type n, octet_iterator end) const
{
	try {
		utf8::advance(it, n, end);
	}
	catch (const utf8::not_enough_room&) {
		// reached end of the utf8 string before n codepoints were traversed
		it = end;
		return false;
	}
	return true;
}

WzString::WzUniCodepointRef& WzString::WzUniCodepointRef::operator=(const WzUniCodepoint& ch)
{
	// Used to modify the codepoint in the parent WzString (based on the position of this codepoint)
	_parentString.replace(_position, 1, ch);

	return *this;
}

bool WzString::WzUniCodepointRef::operator==(const WzUniCodepointRef& ch) const
{
	return _codepoint == ch._codepoint;
}
