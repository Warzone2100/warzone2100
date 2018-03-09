#include "wzstring.h"
#include <sstream>
#include <iomanip>
#include <utfcpp/utf8.h>
#include "frame.h"

WzUniCodepoint WzUniCodepoint::fromUT8(unsigned char utf_8_char)
{
	uint8_t* p_utf8 = &utf_8_char;
	uint32_t codepoint = 0;
	try {
		codepoint = utf8::next(p_utf8, p_utf8 + 1);
	}
	catch (const utf8::not_enough_room &e)
	{
		ASSERT(false, "WzUniCodepoint::fromUT8 was passed an invalid utf8 char (not_enough_room): %u", utf_8_char);
	}
	catch (const utf8::invalid_utf8 &e)
	{
		ASSERT(false, "WzUniCodepoint::fromUT8 was passed an invalid utf8 char: %u", utf_8_char);
	}
	catch (const utf8::invalid_code_point &e)
	{
		ASSERT(false, "WzUniCodepoint::fromUT8 was passed an invalid utf8 char (invalid_code_point): %u", utf_8_char);
	}
	return WzUniCodepoint(codepoint);
}

std::vector<unsigned char> WzUniCodepoint::toUTF8() const
{
	uint32_t utf32string[] = {_codepoint, 0};
	std::vector<unsigned char> utf8result;
	utf8::utf32to8(utf32string, utf32string + 1, back_inserter(utf8result));
	return utf8result;
}

// Constructs a string of the given size with every codepoint set to ch.
WzString::WzString(int size, const WzUniCodepoint& ch)
{
	for (size_t i = 0; i < size; i++)
	{
		utf8::append(ch.UTF32(), back_inserter(_utf8String));
	}
}

WzString WzString::fromUtf8(const char *str, int size /*= -1*/)
{
	if (str == nullptr) { return WzString(); }
	if (size < 0)
	{
		size = strlen(str);
	}
	ASSERT(utf8::is_valid(str, str + size), "Input text is not valid UTF-8");
	std::string replace_invalid_result;
	try {
		utf8::replace_invalid(str, str + size, back_inserter(replace_invalid_result), '?');
	}
	catch (const std::exception &e) {
		// Likely passed an incomplete UTF-8 sequence
		ASSERT(false, "Encountered error parsing input UTF-8 sequence: %s", e.what());
		replace_invalid_result.clear();
	}
	return WzString(replace_invalid_result);
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
	catch (const std::exception &e) {
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
	return utf8::distance(_utf8String.begin(), _utf8String.end());
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
	auto cUtf8Codepoints = c.toUTF8();
	_utf8String.insert(it, cUtf8Codepoints.begin(), cUtf8Codepoints.end());
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
	auto cUtf8After = after.toUTF8();
	std::vector<unsigned char> cUtf8Codepoints;
	for (size_t i = 0; i < numCodepoints; i++)
	{
		cUtf8Codepoints.insert(cUtf8Codepoints.end(), cUtf8After.begin(), cUtf8After.end());
	}
	_utf8String.replace(it_replace_start, it_replace_end, cUtf8Codepoints.begin(), cUtf8Codepoints.end());
	return *this;
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

// MARK: - Create from numbers

WzString WzString::number(int32_t n)
{
	WzString newString;
	newString._utf8String = std::to_string(n);
	return newString;
}

WzString WzString::number(uint32_t n)
{
	WzString newString;
	newString._utf8String = std::to_string(n);
	return newString;
}

WzString WzString::number(int64_t n)
{
	WzString newString;
	newString._utf8String = std::to_string(n);
	return newString;
}

WzString WzString::number(uint64_t n)
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

const WzString WzString::operator+(const WzString &other) const
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

template <typename octet_iterator, typename distance_type>
bool WzString::_utf8_advance (octet_iterator& it, distance_type n, octet_iterator end) const
{
	try {
		utf8::advance(it, n, end);
	}
	catch (const utf8::not_enough_room& e) {
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
