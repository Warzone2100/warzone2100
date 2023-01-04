/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2021  Warzone 2100 Project

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

#include "map_internal.h"
#include "../include/wzmaplib/map_debug.h"

#include <cstdarg>

#if defined(_WIN32)
# define WIN32_LEAN_AND_MEAN
# define WIN32_EXTRA_LEAN
# undef NOMINMAX
# define NOMINMAX 1
# include <windows.h>
#endif

namespace WzMap {

// MARK: - Logging

void _printLog(WzMap::LoggingProtocol* logger, int line, WzMap::LoggingProtocol::LogLevel level, const char *function, const char *str, ...)
{
	va_list ap;
	static char outputBuffer[2048];
	va_start(ap, str);
	vssprintf(outputBuffer, str, ap);
	va_end(ap);

	if (logger)
	{
		logger->printLog(level, function, line, outputBuffer);
	}
}

// MARK: - Helpers

#if defined(_WIN32)
bool win_utf8ToUtf16(const char* str, std::vector<wchar_t>& outputWStr)
{
	int wstr_len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	if (wstr_len <= 0)
	{
		// Could not not convert string from UTF-8; MultiByteToWideChar failed with error
		return false;
	}
	outputWStr = std::vector<wchar_t>(wstr_len, L'\0');
	if (MultiByteToWideChar(CP_UTF8, 0, str, -1, &outputWStr[0], wstr_len) == 0)
	{
		// Could not not convert string from UTF-8; MultiByteToWideChar failed with error
		return false;
	}
	return true;
}
bool win_utf16toUtf8(const wchar_t* buffer, std::vector<char>& u8_buffer)
{
	// Convert the UTF-16 to UTF-8
	int outputLength = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, NULL, 0, NULL, NULL);
	if (outputLength <= 0)
	{
		// Conversion error
		return false;
	}
	if (u8_buffer.size() < static_cast<size_t>(outputLength))
	{
		u8_buffer.resize(outputLength, 0);
	}
	if (WideCharToMultiByte(CP_UTF8, 0, buffer, -1, &u8_buffer[0], outputLength, NULL, NULL) <= 0)
	{
		// Conversion error
		return false;
	}
	return true;
}
#endif

} // namespace WzMap
