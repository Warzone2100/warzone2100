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

#if defined(WZ_OS_UNIX)
# include <sys/utsname.h>
#endif

#ifndef PACKAGE_DISTRIBUTOR
# define PACKAGE_DISTRIBUTOR "UNKNOWN"
#endif

static char* dbgHeader = NULL;

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
		dumpstr(file, "No debug header available (yet)!\n" );
}

static std::string getProgramPath(const char* programCommand)
{
	std::vector<char> buf(PATH_MAX);

#if defined(WZ_OS_WIN)
	while (GetModuleFileNameA(NULL, &buf[0], buf.size()) == buf.size())
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

	std::string programPath = &buf[0];

	if (!programPath.empty())
	{
		programPath.erase(programPath.find('\n')); // `which' adds a \n which confuses exec()
		debug(LOG_WZ, "Found us at %s", programPath.c_str());
	}
	else
	{
		debug(LOG_WARNING, "Could not retrieve full path to %s, will not create extended backtrace\n", programCommand);
	}

	return programPath;
}

static std::string getSysinfo()
{
#if defined(WZ_OS_WIN)
	return std::string();
#elif defined(WZ_OS_UNIX)
	struct utsname sysInfo;
	std::ostringstream os;

	if (uname(&sysInfo) != 0)
		os << "System information may be invalid!" << std::endl
		   << std::endl;

	os << "Operating system: " << sysInfo.sysname  << std::endl
	   << "Node name: "        << sysInfo.nodename << std::endl
	   << "Release: "          << sysInfo.release  << std::endl
	   << "Version: "          << sysInfo.version  << std::endl
	   << "Machine: "          << sysInfo.machine  << std::endl;

	return os.str();
#endif
}

static void createHeader(const char* programCommand)
{
	time_t currentTime = time(NULL);
	std::ostringstream os;

	os << "Program: "     << getProgramPath(programCommand) << "(" PACKAGE ")" << std::endl
	   << "Version: "     << version_getFormattedVersionString() << std::endl
	   << "Distributor: " PACKAGE_DISTRIBUTOR << std::endl
	   << "Compiled on: " __DATE__ " " __TIME__ << std::endl
	   << "Compiled by: "
#if defined(WZ_CC_GNU) && !defined(WZ_CC_INTEL)
	       << "GCC " __VERSION__ << std::endl
#elif defined(WZ_CC_INTEL)
	// Intel includes the compiler name within the version string
	       << __VERSION__ << std::endl
#else
	       << "UNKNOWN" << std::endl
#endif
	   << "Executed on: " << ctime(&currentTime) << std::endl
	   << getSysinfo() << std::endl
	   << "Pointers: " << (sizeof(void*) * CHAR_BIT) << "bit" << std::endl
	   << std::endl;

	dbgHeader = strdup(os.str().c_str());
	if (dbgHeader == NULL)
	{
		debug(LOG_ERROR, "createHeader: Out of memory!");
		abort();
		return;
	}
}

void dbgDumpInit(const char* programCommand)
{
	createHeader(programCommand);
}
