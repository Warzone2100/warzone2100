/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Giel van Schijndel
	Copyright (C) 2008  Warzone Resurrection Project

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

#include <climits>
#include <ctime>
#include <string>
#include <vector>
#include <sstream>
#include "dumpinfo.h"
extern "C"
{
// FIXME: #include from src/
#include "src/version.h"
}

#if defined(WZ_OS_WIN)
# define EOL "\r\n"
#else
# define EOL "\n"
# include <sys/utsname.h>
#endif

using std::string;
using std::vector;

static const char eol[] = EOL;

static char* dbgHeader = NULL;
static string programPath;

static void dumpstr(const DumpFileHandle file, const char * const str)
{
#if defined(WZ_OS_WIN)
	DWORD lNumberOfBytesWritten;
	WriteFile(file, str, strlen(str), &lNumberOfBytesWritten, NULL);
#else
	write(file, str, strlen(str));
#endif
}

void dbgDumpHeader(DumpFileHandle file)
{
	if (dbgHeader)
		dumpstr(file, dbgHeader);
	else
		dumpstr(file, "No debug header available (yet)!" EOL);
}

static void initProgramPath(const char* programCommand)
{
	vector<char> buf(PATH_MAX);
	size_t bufsize = PATH_MAX;

#if defined(WZ_OS_WIN)
	while (GetModuleFileName(NULL, &buf[0], buf.size()) == buf.size())
	{
		buf.resize(buf.size() * 2);
	}
#elif defined(WZ_OS_UNIX) && !defined(WZ_OS_MAC)
	{
		FILE * whichProgramStream;
		char* whichProgramCommand;

		sasprintf(&whichProgramCommand, "which %s", programCommand);
		whichProgramStream = popen(whichProgramCommand, "r");
		fread(&buf[0], 1, buf.size(), whichProgramStream);
		pclose(whichProgramStream);
	}
#endif

	programPath = buf.data();

	if (!programPath.empty())
	{
		programPath.erase(programPath.find('\n')); // `which' adds a \n which confuses exec()
		debug(LOG_WZ, "Found us at %s", programPath.c_str());
	}
	else
	{
		debug(LOG_WARNING, "Could not retrieve full path to %s, will not create extended backtrace\n", programCommand);
	}
}

static std::string getSysinfo()
{
#if defined(WZ_OS_WIN)
	return string();
#elif defined(WZ_OS_UNIX)
	struct utsname sysInfo;
	std::ostringstream os;

	if (uname(&sysInfo) != 0)
		os << "System information may be invalid!" EOL EOL;

	os << "Operating system: " << sysInfo.sysname  << EOL
	   << "Node name: "        << sysInfo.nodename << EOL
	   << "Release: "          << sysInfo.release  << EOL
	   << "Version: "          << sysInfo.version  << EOL
	   << "Machine: "          << sysInfo.machine  << EOL;

	return os.str();
#endif
}

static void createHeader(void)
{
	time_t currentTime = time(NULL);
	int ret;
	std::ostringstream os;

	os << "Program: "     << programPath << "(" PACKAGE ")" EOL
	   << "Version: "     << version_getFormattedVersionString() << EOL
	   << "Distributor: " PACKAGE_DISTRIBUTOR EOL
	   << "Compiled on: " __DATE__ " " __TIME__ EOL
	   << "Compiled by: "
#if defined(WZ_CC_GNU) && !defined(WZ_CC_INTEL)
	       << "GCC " __VERSION__ EOL
#elif defined(WZ_CC_INTEL)
	// Intel includes the compiler name within the version string
	       << __VERSION__ EOL
#else
	       "UNKNOWN" EOL
#endif
	   << "Executed on: " << ctime(&currentTime) << EOL
	   << getSysinfo() << EOL
	   << "Pointers: " << (sizeof(void*) * CHAR_BIT) << "bit" EOL
	   << EOL;

	dbgHeader = strdup(os.str().c_str());
	if (dbgHeader == NULL)
	{
		debug(LOG_ERROR, "createHeader: Out of memory!");
		abort();
		return;
	}
}

void dbgDumpInit()
{
	createHeader();
}
