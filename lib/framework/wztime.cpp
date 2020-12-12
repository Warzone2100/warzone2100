/*
	This file is part of Warzone 2100.
	Copyright (C) 2020  Warzone 2100 Project

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

#include "wztime.h"
#include <iomanip>
#include <sstream>
#include "wzglobal.h"
#include "frame.h"

tm getUtcTime(std::time_t const &timer)
{
	struct tm timeinfo = {};
#if defined(WZ_OS_WIN)
	if (gmtime_s(&timeinfo, &timer) != 0)
	{
		// Invalid argument to gmtime_s
		debug(LOG_ERROR, "Invalid time_t argument");
	}
#else
	if (!gmtime_r(&timer, &timeinfo))
	{
		debug(LOG_ERROR, "gmtime_r failed");
	}
#endif
	return timeinfo;
}

optional<tm> getLocalTimeOpt(std::time_t const &timer)
{
	struct tm timeinfo = {};
#if defined(WZ_OS_WIN)
	errno_t result = localtime_s(&timeinfo, &timer);
	if (result != 0)
	{
		char sys_msg[80];
		if (strerror_s(sys_msg, sizeof(sys_msg), result) != 0)
		{
			strncpy(sys_msg, "unknown error", sizeof(sys_msg));
		}
		debug(LOG_ERROR, "localtime_s failed with error: %s", sys_msg);
		return nullopt;
	}
#else
	if (!localtime_r(&timer, &timeinfo))
	{
		debug(LOG_ERROR, "localtime_r failed");
		return nullopt;
	}
#endif
	return timeinfo;
}

tm getLocalTime(std::time_t const &timer)
{
	auto result = getLocalTimeOpt(timer);
	if (!result.has_value())
	{
		struct tm zeroResult = {};
		return zeroResult;
	}
	return result.value();
}

std::string const formatLocalDateTime(char const *format, std::time_t const &timer)
{
	std::stringstream ss;
	auto timeinfo = getLocalTimeOpt(timer);
	ASSERT_OR_RETURN("", timeinfo.has_value(), "getLocalTime failed");
	ss << std::put_time(&(timeinfo.value()), format);
	return ss.str();
}

std::string const formatLocalDateTime(char const *format)
{
	return formatLocalDateTime(format, std::time(nullptr));
}
