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

} // namespace WzMap
