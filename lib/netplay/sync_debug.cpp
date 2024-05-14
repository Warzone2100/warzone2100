/*
	This file is part of Warzone 2100.
	Copyright (C) 2024  Warzone 2100 Project

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

#include "sync_debug.h"

#include "lib/framework/wzglobal.h"
#include "lib/framework/crc.h"
#include "lib/framework/macros.h"
#include "lib/framework/string_ext.h"
#include "lib/framework/file.h"
#include "lib/framework/debug.h"
#include "lib/framework/physfs_ext.h"
#include "lib/gamelib/gtime.h"
#include "nettypes.h"
#include "netplay.h"
#include "netsocket.h" // solely to bring in `htonl` function

#include <physfs.h>

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#if defined(WZ_OS_LINUX) && defined(__GLIBC__)
#include <execinfo.h>  // Nonfatal runtime backtraces.
#endif // defined(WZ_OS_LINUX) && defined(__GLIBC__)

static uint32_t verboseDebugOutputAllSyncLogsUntilGameTime = 0;

struct SyncDebugEntry
{
	char const* function;
};

struct SyncDebugString : public SyncDebugEntry
{
	void set(uint32_t& crc, char const* f, char const* string)
	{
		function = f;
		crc = wz::crc_update(crc, function, strlen(function) + 1);
		crc = wz::crc_update(crc, string, strlen(string) + 1);
	}
	int snprint(char* buf, size_t bufSize, char const*& string) const
	{
		int ret = snprintf(buf, bufSize, "[%s] %s\n", function, string);
		string += strlen(string) + 1;
		return ret;
	}
};

struct SyncDebugValueChange : public SyncDebugEntry
{
	void set(uint32_t& crc, char const* f, char const* vn, int nv, int i)
	{
		function = f;
		variableName = vn;
		newValue = nv;
		id = i;
		uint32_t valueBytes = htonl(newValue);
		crc = wz::crc_update(crc, function, strlen(function) + 1);
		crc = wz::crc_update(crc, variableName, strlen(variableName) + 1);
		crc = wz::crc_update(crc, &valueBytes, 4);
	}
	int snprint(char* buf, size_t bufSize) const
	{
		if (id != -1)
		{
			return snprintf(buf, bufSize, "[%s] %d %s = %d\n", function, id, variableName, newValue);
		}
		return snprintf(buf, bufSize, "[%s] %s = %d\n", function, variableName, newValue);
	}

	int         newValue;
	int         id;
	char const* variableName;
};

struct SyncDebugIntList : public SyncDebugEntry
{
	void set(uint32_t& crc, char const* f, char const* s, int const* ints, size_t num)
	{
		function = f;
		string = s;
		uint32_t valueBytes[40];
		numInts = std::min(num, ARRAY_SIZE(valueBytes));
		for (unsigned n = 0; n < numInts; ++n)
		{
			valueBytes[n] = htonl(ints[n]);
		}
		crc = wz::crc_update(crc, valueBytes, 4 * numInts);
	}
	int snprint(char* buf, size_t bufSize, int const*& ints) const
	{
		size_t index = 0;
		if (index < bufSize)
		{
			index += snprintf(buf + index, bufSize - index, "[%s] ", function);
		}
		if (index < bufSize)
		{
			switch (numInts)
			{
			case  0: index += snprintf(buf + index, bufSize - index, "%s", string); break;
			case  1: index += snprintf(buf + index, bufSize - index, string, ints[0]); break;
			case  2: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1]); break;
			case  3: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2]); break;
			case  4: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3]); break;
			case  5: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4]); break;
			case  6: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5]); break;
			case  7: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6]); break;
			case  8: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7]); break;
			case  9: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8]); break;
			case 10: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9]); break;
			case 11: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10]); break;
			case 12: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11]); break;
			case 13: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12]); break;
			case 14: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13]); break;
			case 15: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14]); break;
			case 16: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15]); break;
			case 17: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16]); break;
			case 18: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17]); break;
			case 19: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18]); break;
			case 20: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19]); break;
			case 21: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20]); break;
			case 22: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21]); break;
			case 23: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22]); break;
			case 24: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22], ints[23]); break;
			case 25: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22], ints[23], ints[24]); break;
			case 26: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22], ints[23], ints[24], ints[25]); break;
			case 27: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22], ints[23], ints[24], ints[25], ints[26]); break;
			case 28: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22], ints[23], ints[24], ints[25], ints[26], ints[27]); break;
			case 29: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22], ints[23], ints[24], ints[25], ints[26], ints[27], ints[28]); break;
			case 30: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22], ints[23], ints[24], ints[25], ints[26], ints[27], ints[28], ints[29]); break;
			case 31: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22], ints[23], ints[24], ints[25], ints[26], ints[27], ints[28], ints[29], ints[30]); break;
			case 32: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22], ints[23], ints[24], ints[25], ints[26], ints[27], ints[28], ints[29], ints[30], ints[31]); break;
			case 33: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22], ints[23], ints[24], ints[25], ints[26], ints[27], ints[28], ints[29], ints[30], ints[31], ints[32]); break;
			case 34: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22], ints[23], ints[24], ints[25], ints[26], ints[27], ints[28], ints[29], ints[30], ints[31], ints[32], ints[33]); break;
			case 35: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22], ints[23], ints[24], ints[25], ints[26], ints[27], ints[28], ints[29], ints[30], ints[31], ints[32], ints[33], ints[34]); break;
			case 36: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22], ints[23], ints[24], ints[25], ints[26], ints[27], ints[28], ints[29], ints[30], ints[31], ints[32], ints[33], ints[34], ints[35]); break;
			case 37: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22], ints[23], ints[24], ints[25], ints[26], ints[27], ints[28], ints[29], ints[30], ints[31], ints[32], ints[33], ints[34], ints[35], ints[36]); break;
			case 38: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22], ints[23], ints[24], ints[25], ints[26], ints[27], ints[28], ints[29], ints[30], ints[31], ints[32], ints[33], ints[34], ints[35], ints[36], ints[37]); break;
			case 39: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22], ints[23], ints[24], ints[25], ints[26], ints[27], ints[28], ints[29], ints[30], ints[31], ints[32], ints[33], ints[34], ints[35], ints[36], ints[37], ints[38]); break;
			case 40: index += snprintf(buf + index, bufSize - index, string, ints[0], ints[1], ints[2], ints[3], ints[4], ints[5], ints[6], ints[7], ints[8], ints[9], ints[10], ints[11], ints[12], ints[13], ints[14], ints[15], ints[16], ints[17], ints[18], ints[19], ints[20], ints[21], ints[22], ints[23], ints[24], ints[25], ints[26], ints[27], ints[28], ints[29], ints[30], ints[31], ints[32], ints[33], ints[34], ints[35], ints[36], ints[37], ints[38], ints[39]); break;
			default: index += snprintf(buf + index, bufSize - index, "Too many ints in intlist."); break;
			}
		}
		if (index < bufSize)
		{
			index += snprintf(buf + index, bufSize - index, "\n");
		}
		ints += numInts;
		return index;
	}

	char const* string;
	unsigned numInts;
};

struct SyncDebugLog
{
	SyncDebugLog() : time(0), crc(0x00000000) {}
	void clear()
	{
		log.clear();
		time = 0;
		crc = wz::crc_init();
		//printf("Freeing %d strings, %d valueChanges, %d intLists, %d chars, %d ints\n", (int)strings.size(), (int)valueChanges.size(), (int)intLists.size(), (int)chars.size(), (int)ints.size());
		strings.clear();
		valueChanges.clear();
		intLists.clear();
		chars.clear();
		ints.clear();
	}
	void string(char const* f, char const* s)
	{
		size_t offset = chars.size();
		chars.resize(chars.size() + strlen(s) + 1);
		char* buf = &chars[offset];
		strcpy(buf, s);

		strings.resize(strings.size() + 1);
		strings.back().set(crc, f, buf);

		log.push_back('s');
	}
	void valueChange(char const* f, char const* vn, int nv, int i)
	{
		valueChanges.resize(valueChanges.size() + 1);
		valueChanges.back().set(crc, f, vn, nv, i);
		log.push_back('v');
	}
	void intList(char const* f, char const* s, int* begin, size_t num)
	{
		size_t offset = ints.size();
		ints.resize(ints.size() + num);
		int* buf = &ints[offset];
		std::copy(begin, begin + num, buf);

		intLists.resize(intLists.size() + 1);
		intLists.back().set(crc, f, s, buf, num);
		log.push_back('i');
	}
	int snprint(char* buf, size_t bufSize)
	{
		SyncDebugString const* stringPtr = strings.empty() ? nullptr : &strings[0]; // .empty() check, since &strings[0] is undefined if strings is empty(), even if it's likely to work, anyway.
		SyncDebugValueChange const* valueChangePtr = valueChanges.empty() ? nullptr : &valueChanges[0];
		SyncDebugIntList const* intListPtr = intLists.empty() ? nullptr : &intLists[0];
		char const* charPtr = chars.empty() ? nullptr : &chars[0];
		int const* intPtr = ints.empty() ? nullptr : &ints[0];

		int index = 0;
		for (size_t n = 0; n < log.size() && (size_t)index < bufSize; ++n)
		{
			char type = log[n];
			switch (type)
			{
			case 's':
				index += stringPtr++->snprint(buf + index, bufSize - index, charPtr);
				break;
			case 'v':
				index += valueChangePtr++->snprint(buf + index, bufSize - index);
				break;
			case 'i':
				index += intListPtr++->snprint(buf + index, bufSize - index, intPtr);
				break;
			default:
				abort();
				break;
			}
		}
		return index;
	}
	uint32_t getGameTime() const
	{
		return time;
	}
	uint32_t getCrc() const
	{
		return ~crc;  // Invert bits, since everyone else seems to do that with CRCs...
	}
	size_t getNumEntries() const
	{
		return log.size();
	}
	void setGameTime(uint32_t newTime)
	{
		time = newTime;
	}
	void setCrc(uint32_t newCrc)
	{
		crc = ~newCrc;  // Invert bits, since everyone else seems to do that with CRCs...
	}

private:
	std::vector<char> log;
	uint32_t time;
	uint32_t crc;

	std::vector<SyncDebugString> strings;
	std::vector<SyncDebugValueChange> valueChanges;
	std::vector<SyncDebugIntList> intLists;

	std::vector<char> chars;
	std::vector<int> ints;

private:
	SyncDebugLog(SyncDebugLog const&)/* = delete*/;
	SyncDebugLog& operator =(SyncDebugLog const&)/* = delete*/;
};

#define MAX_LEN_LOG_LINE 512  // From debug.c - no use printing something longer.
#define MAX_SYNC_HISTORY 12

static unsigned syncDebugNext = 0;
static SyncDebugLog syncDebugLog[MAX_SYNC_HISTORY];
static uint32_t syncDebugExtraGameTime;
static uint32_t syncDebugExtraCrc;

static uint32_t syncDebugNumDumps = 0;

void _syncDebug(const char* function, const char* str, ...)
{
#ifdef WZ_CC_MSVC
	char const* f = function; while (*f != '\0') if (*f++ == ':')
	{
		function = f;    // Strip "Class::" from "Class::myFunction".
	}
#endif

	va_list ap;
	char outputBuffer[MAX_LEN_LOG_LINE];

	va_start(ap, str);
	vssprintf(outputBuffer, str, ap);
	va_end(ap);

	syncDebugLog[syncDebugNext].string(function, outputBuffer);
}

void _syncDebugIntList(const char* function, const char* str, int* ints, size_t numInts)
{
#ifdef WZ_CC_MSVC
	char const* f = function; while (*f != '\0') if (*f++ == ':')
	{
		function = f;    // Strip "Class::" from "Class::myFunction".
	}
#endif

	syncDebugLog[syncDebugNext].intList(function, str, ints, numInts);
}

void _syncDebugBacktrace(const char* function)
{
#ifdef WZ_CC_MSVC
	char const* f = function; while (*f != '\0') if (*f++ == ':')
	{
		function = f;    // Strip "Class::" from "Class::myFunction".
	}
#endif

	uint32_t backupCrc = syncDebugLog[syncDebugNext].getCrc();  // Ignore CRC changes from _syncDebug(), since identical backtraces can be printed differently.

#if defined(WZ_OS_LINUX) && defined(__GLIBC__)
	void* btv[20];
	unsigned num = backtrace(btv, sizeof(btv) / sizeof(*btv));
	char** btc = backtrace_symbols(btv, num);
	unsigned i;
	for (i = 1; i + 2 < num; ++i)  // =1: Don't print "src/warzone2100(syncDebugBacktrace+0x16) [0x6312d1]". +2: Don't print last two lines of backtrace such as "/lib/libc.so.6(__libc_start_main+0xe6) [0x7f91e040ea26]", since the address varies (even with the same binary).
	{
		_syncDebug("BT", "%s", btc[i]);
	}
	free(btc);
#else
	_syncDebug("BT", "Sorry, syncDebugBacktrace() not implemented on your system. Called from %s.", function);
#endif

	// Use CRC of something platform-independent, to avoid false positive desynchs.
	backupCrc = ~wz::crc_update(~backupCrc, function, strlen(function) + 1);
	syncDebugLog[syncDebugNext].setCrc(backupCrc);
}

uint32_t syncDebugGetCrc()
{
	return syncDebugLog[syncDebugNext].getCrc();
}

void syncDebugSetCrc(uint32_t crc)
{
	syncDebugLog[syncDebugNext].setCrc(crc);
}

void resetSyncDebug()
{
	for (unsigned i = 0; i < MAX_SYNC_HISTORY; ++i)
	{
		syncDebugLog[i].clear();
	}

	syncDebugExtraGameTime = 0;
	syncDebugExtraCrc = 0xFFFFFFFF;

	syncDebugNext = 0;

	syncDebugNumDumps = 0;
}

GameCrcType nextDebugSync()
{
	uint32_t ret = syncDebugLog[syncDebugNext].getCrc();

	// Save gameTime, so we know which CRC to compare with, later.
	syncDebugLog[syncDebugNext].setGameTime(gameTime);

	// Go to next position, and free it ready for use.
	syncDebugNext = (syncDebugNext + 1) % MAX_SYNC_HISTORY;
	syncDebugLog[syncDebugNext].clear();

	return (GameCrcType)ret;
}

static void dumpDebugSyncImpl(uint8_t* buf, size_t bufLen, const char* fname)
{
	PHYSFS_file* fp = openSaveFile(fname);
	if (!fp)
	{
		debug(LOG_ERROR, "Failed to open file for writing: %s", fname);
		return;
	}
	ASSERT(bufLen <= static_cast<size_t>(std::numeric_limits<PHYSFS_uint32>::max()), "bufLen (%zu) exceeds PHYSFS_uint32::max", bufLen);
	WZ_PHYSFS_writeBytes(fp, buf, static_cast<PHYSFS_uint32>(bufLen));
	PHYSFS_close(fp);
}

static void dumpDebugSync(uint8_t* buf, size_t bufLen, uint32_t time, unsigned player, bool syncError = true)
{
	char fname[100];
	ssprintf(fname, "logs/desync%u_p%u.txt", time, player);

	dumpDebugSyncImpl(buf, bufLen, fname);

	bool isSpectator = player < NetPlay.players.size() && NetPlay.players[player].isSpectator;
	std::string typeDescription = (syncError) ? "sync error" : "sync log";
	std::string playerDescription = (isSpectator) ? "spectator" : "player";
	debug(LOG_ERROR, "Dumped %s %u's %s at gameTime %u to file: %s%s", playerDescription.c_str(), player, typeDescription.c_str(), time, WZ_PHYSFS_getRealDir_String(fname).c_str(), fname);
}

static void sendDebugSync(uint8_t* buf, uint32_t bufLen, uint32_t time)
{
	// Save our own, before sending, so that if we have 2 clients running on the same computer, to guarantee that it is done saving before the other client saves on top.
	dumpDebugSync(buf, bufLen, time, selectedPlayer);

	NETbeginEncode(NETbroadcastQueue(), NET_DEBUG_SYNC);
	NETuint32_t(&time);
	NETuint32_t(&bufLen);
	NETbin(buf, bufLen);
	NETend();
}

static uint8_t debugSyncTmpBuf[2000000];

static size_t dumpLocalDebugSyncLog(unsigned logIndex)
{
	size_t bufIndex = 0;
	// Dump our version, and also erase it, so we only dump it at most once.
	bufIndex += snprintf((char*)debugSyncTmpBuf + bufIndex, ARRAY_SIZE(debugSyncTmpBuf) - bufIndex, "===== BEGIN gameTime=%u, %zu entries, CRC 0x%08X =====\n", syncDebugLog[logIndex].getGameTime(), syncDebugLog[logIndex].getNumEntries(), syncDebugLog[logIndex].getCrc());
	bufIndex = MIN(bufIndex, ARRAY_SIZE(debugSyncTmpBuf));  // snprintf will not overflow debugSyncTmpBuf, but returns as much as it would have printed if possible.
	bufIndex += syncDebugLog[logIndex].snprint((char*)debugSyncTmpBuf + bufIndex, ARRAY_SIZE(debugSyncTmpBuf) - bufIndex);
	bufIndex = MIN(bufIndex, ARRAY_SIZE(debugSyncTmpBuf));  // snprintf will not overflow debugSyncTmpBuf, but returns as much as it would have printed if possible.
	bufIndex += snprintf((char*)debugSyncTmpBuf + bufIndex, ARRAY_SIZE(debugSyncTmpBuf) - bufIndex, "===== END gameTime=%u, %zu entries, CRC 0x%08X =====\n", syncDebugLog[logIndex].getGameTime(), syncDebugLog[logIndex].getNumEntries(), syncDebugLog[logIndex].getCrc());
	bufIndex = MIN(bufIndex, ARRAY_SIZE(debugSyncTmpBuf));  // snprintf will not overflow debugSyncTmpBuf, but returns as much as it would have printed if possible.

	return bufIndex;
}

static size_t dumpLocalDebugSyncLogByTime(uint32_t time)
{
	unsigned logIndex;
	for (logIndex = 0; logIndex < MAX_SYNC_HISTORY; ++logIndex)
	{
		if (syncDebugLog[logIndex].getGameTime() == time)
		{
			break; // found the log index for this time
		}
	}

	if (logIndex >= MAX_SYNC_HISTORY)
	{
		// did not find it
		debug(LOG_WARNING, "Couldn't find gameTime: %" PRIu32 " in history", time);
		return 0;
	}

	return dumpLocalDebugSyncLog(logIndex);
}

void recvDebugSync(NETQUEUE queue)
{
	uint32_t time = 0;
	uint32_t bufLen = 0;

	NETbeginDecode(queue, NET_DEBUG_SYNC);
	NETuint32_t(&time);
	NETuint32_t(&bufLen);
	bufLen = MIN(bufLen, ARRAY_SIZE(debugSyncTmpBuf));
	NETbin(debugSyncTmpBuf, bufLen);
	NETend();

	dumpDebugSync(debugSyncTmpBuf, bufLen, time, queue.index);

	// Also dump the debug sync log for this local client (if possible)
	bufLen = dumpLocalDebugSyncLogByTime(time);
	if (bufLen > 0)
	{
		dumpDebugSync(debugSyncTmpBuf, bufLen, time, selectedPlayer, false);
	}
}

// Set whether verbose debug mode - outputting the current player's sync log for every single game tick - is enabled
// WARNING: This may significantly impact performance *and* will fill up your drive with a lot of logs data!
// It is only intended to be used for debugging issues such as: replays desyncing when gameplay does not, etc. (And don't let the game run too long!)
void NET_setDebuggingModeVerboseOutputAllSyncLogs(uint32_t untilGameTime)
{
	verboseDebugOutputAllSyncLogsUntilGameTime = untilGameTime;
	if (verboseDebugOutputAllSyncLogsUntilGameTime)
	{
		debug(LOG_INFO, "WARNING: Verbose sync log output is enabled!");
	}
}

void debugVerboseLogSyncIfNeeded()
{
	if (!verboseDebugOutputAllSyncLogsUntilGameTime || gameTime > verboseDebugOutputAllSyncLogsUntilGameTime)
	{
		return;
	}

	auto bufLen = dumpLocalDebugSyncLogByTime(gameTime);
	if (bufLen > 0)
	{
		char fname[100];
		ssprintf(fname, "logs/sync%u_p%u.txt", gameTime, selectedPlayer);
		dumpDebugSyncImpl(debugSyncTmpBuf, bufLen, fname);
	}
}

bool checkDebugSync(uint32_t checkGameTime, GameCrcType checkCrc)
{
	if (checkGameTime == syncDebugLog[syncDebugNext].getGameTime())  // Can't happen - and syncDebugGameTime[] == 0, until just before sending the CRC, anyway.
	{
		debug(LOG_ERROR, "Huh? We aren't done yet...");
		return true;
	}

	unsigned logIndex;
	for (logIndex = 0; logIndex < MAX_SYNC_HISTORY; ++logIndex)
	{
		if (syncDebugLog[logIndex].getGameTime() == checkGameTime)
		{
			if ((GameCrcType)syncDebugLog[logIndex].getCrc() == checkCrc)
			{
				return true;                    // Check passed. (So far... There might still be more players to compare CRCs with.)
			}

			debug(LOG_NET, "Non-matching crc for gameTime: %" PRIu32 "", checkGameTime);
			break;                                  // Check failed!
		}
	}

	if (logIndex >= MAX_SYNC_HISTORY && syncDebugExtraGameTime == checkGameTime)
	{
		if ((GameCrcType)syncDebugExtraCrc == checkCrc)
		{
			return true;
		}
	}

	if (logIndex >= MAX_SYNC_HISTORY)
	{
		debug(LOG_NET, "Couldn't check gameTime: %" PRIu32 "", checkGameTime);
		return false;                                   // Couldn't check. May have dumped already, or MAX_SYNC_HISTORY isn't big enough compared to the maximum latency.
	}

	size_t bufIndex = dumpLocalDebugSyncLog(logIndex);
	if (syncDebugNumDumps < 2)
	{
		++syncDebugNumDumps;
		sendDebugSync(debugSyncTmpBuf, static_cast<uint32_t>(bufIndex), syncDebugLog[logIndex].getGameTime());
	}

	// Backup correct CRC for checking against remaining players, even though we erased the logs (which were dumped already).
	syncDebugExtraGameTime = syncDebugLog[logIndex].getGameTime();
	syncDebugExtraCrc = syncDebugLog[logIndex].getCrc();

	// Finish erasing our version.
	syncDebugLog[logIndex].clear();

	return false;  // Ouch.
}
