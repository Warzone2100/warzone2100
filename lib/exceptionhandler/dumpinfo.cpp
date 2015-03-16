/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Giel van Schijndel
	Copyright (C) 2008-2013  Warzone 2100 Project

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

#include "dumpinfo.h"
#include <cerrno>
#include <climits>
#include <ctime>
#include <cstdlib>
#include <string>
#include <vector>
#include <deque>
#include <sstream>
#include <physfs.h>
#include "lib/framework/stdio_ext.h"
#include "lib/framework/wzglobal.h" // required for config.h

#if defined(WZ_OS_UNIX)
# include <sys/utsname.h>
#endif

#ifndef PACKAGE_DISTRIBUTOR
# define PACKAGE_DISTRIBUTOR "UNKNOWN"
#endif

static const char endl[] =
#if defined(WZ_OS_WIN)
    "\r\n";
#else
    "\n";
#endif

using std::string;

static const std::size_t max_debug_messages = 20;

static char *dbgHeader = NULL;
static std::deque<std::vector<char> > dbgMessages;

// used to add custom info to the crash log
static std::vector<char> miscData;

static void dumpstr(const DumpFileHandle file, const char *const str, std::size_t const size)
{
#if defined(WZ_OS_WIN)
	DWORD lNumberOfBytesWritten;
	WriteFile(file, str, size, &lNumberOfBytesWritten, NULL);
#else
	std::size_t written = 0;
	while (written < size)
	{
		const ssize_t ret = write(file, str + written, size - written);
		if (ret == -1)
		{
			switch (errno)
			{
			case EAGAIN:
				// Sleep to prevent wasting of CPU in case of non-blocking I/O
				usleep(1);
			case EINTR:
				continue;
			default:
				// TODO find a decent way to deal with the fatal errors
				return;
			}
		}

		written += ret;
	}
#endif
}

static void dumpstr(const DumpFileHandle file, const char *const str)
{
	dumpstr(file, str, strlen(str));
}

static void dumpEOL(const DumpFileHandle file)
{
	dumpstr(file, endl);
}

static void debug_exceptionhandler_data(void **, const char *const str)
{
	/* Can't use ASSERT here as that will cause us to be invoked again.
	 * Possibly resulting in infinite recursion.
	 */
	assert(str != NULL && "Empty string sent to debug callback");

	// For non-debug builds
	if (str == NULL)
	{
		return;
	}

	// Push this message on the message list
	const char *last = &str[strlen(str)];

	// Strip finishing newlines
	while (last != str
	       && *(last - 1) == '\n')
	{
		--last;
	}

	dbgMessages.push_back(std::vector<char>(str, last));

	// Ensure the message list's maximum size is maintained
	while (dbgMessages.size() > max_debug_messages)
	{
		dbgMessages.pop_front();
	}
}

void dbgDumpHeader(DumpFileHandle file)
{
	if (dbgHeader)
	{
		dumpstr(file, dbgHeader);
		// Now get any other data that we need to include in bug report
		dumpstr(file, "Misc Data:");
		dumpEOL(file);
		dumpstr(file, &miscData[0], miscData.size());
		dumpEOL(file);
	}
	else
	{
		dumpstr(file, "No debug header available (yet)!");
		dumpEOL(file);
	}
}

void dbgDumpLog(DumpFileHandle file)
{
	// Write all messages to the given file
	for (std::deque<std::vector<char> >::const_iterator
	     msg  = dbgMessages.begin();
	     msg != dbgMessages.end();
	     ++msg)
	{
		dumpstr(file, "Log message: ");
		dumpstr(file, &(*msg)[0], msg->size());
		dumpEOL(file);
	}

	// Terminate with a separating newline
	dumpEOL(file);
}

static std::string getProgramPath(const char *programCommand)
{
	std::vector<char> buf(PATH_MAX);

#if defined(WZ_OS_WIN)
	while (GetModuleFileNameA(NULL, &buf[0], buf.size()) == buf.size())
	{
		buf.resize(buf.size() * 2);
	}
#elif defined(WZ_OS_UNIX) && !defined(WZ_OS_MAC)
	{
		FILE *whichProgramStream;
		char *whichProgramCommand;

		sasprintf(&whichProgramCommand, "which %s", programCommand);
		whichProgramStream = popen(whichProgramCommand, "r");
		if (whichProgramStream == NULL)
		{
			debug(LOG_WARNING, "Failed to run \"%s\", will not create extended backtrace", whichProgramCommand);
			return std::string();
		}

		size_t read = 0;
		while (!feof(whichProgramStream))
		{
			if (read == buf.size())
			{
				buf.resize(buf.size() * 2);
			}

			read += fread(&buf[read], 1, buf.size() - read, whichProgramStream);
		}
		pclose(whichProgramStream);
	}
#endif

	std::string programPath(buf.begin(), buf.end());

	if (!programPath.empty())
	{
		// `which' adds a \n which confuses exec()
		std::string::size_type eol = programPath.find('\n');
		if (eol != std::string::npos)
		{
			programPath.erase(eol);
		}
		// Strip any NUL chars
		std::string::size_type nul = programPath.find('\0');
		if (nul != std::string::npos)
		{
			programPath.erase(nul);
		}
		debug(LOG_WZ, "Found us at %s", programPath.c_str());
	}
	else
	{
		debug(LOG_INFO, "Could not retrieve full path to %s, will not create extended backtrace", programCommand);
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
		os << "System information may be invalid!" << endl
		   << endl;

	os << "Operating system: " << sysInfo.sysname  << endl
	   << "Node name: "        << sysInfo.nodename << endl
	   << "Release: "          << sysInfo.release  << endl
	   << "Version: "          << sysInfo.version  << endl
	   << "Machine: "          << sysInfo.machine  << endl;

	return os.str();
#endif
}

static std::string getCurTime()
{
	using std::string;

	// Get the current time
	const time_t currentTime = time(NULL);

	// Convert it to a string
	string time(ctime(&currentTime));

	// Mark finishing newlines as NUL characters
	for (string::reverse_iterator
	     newline  = time.rbegin();
	     newline != time.rend()
	     && *newline == '\n';
	     ++newline)
	{
		*newline = '\0';
	}

	// Remove everything after, and including, the first NUL character
	string::size_type newline = time.find_first_of('\0');
	if (newline != string::npos)
	{
		time.erase(newline);
	}

	return time;
}

template <typename CharT, typename Traits>
std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &os, PHYSFS_Version const &ver)
{
	return os << static_cast<unsigned int>(ver.major)
	       << "." << static_cast<unsigned int>(ver.minor)
	       << "." << static_cast<unsigned int>(ver.patch);
}

static void createHeader(int const argc, const char **argv, const char *packageVersion)
{
	std::ostringstream os;

	os << "Program: "     << getProgramPath(argv[0]) << "(" PACKAGE ")" << endl
	   << "Command line: ";

	/* Dump all command line arguments surrounded by double quotes and
	 * separated by spaces.
	 */
	for (int i = 0; i < argc; ++i)
	{
		os << "\"" << argv[i] << "\" ";
	}

	os << endl;

	os << "Version: "     << packageVersion << endl
	   << "Distributor: " PACKAGE_DISTRIBUTOR << endl
	   << "Compiled on: " __DATE__ " " __TIME__ << endl
	   << "Compiled by: "
#if defined(WZ_CC_GNU) && !defined(WZ_CC_INTEL)
	   << "GCC " __VERSION__ << endl
#elif defined(WZ_CC_INTEL)
	   // Intel includes the compiler name within the version string
	   << __VERSION__ << endl
#else
	   << "UNKNOWN" << endl
#endif
	   << "Compiled mode: "
#ifdef DEBUG
	   << "Debug build" << endl
#else
	   << "Release build" << endl
#endif
	   << "Executed on: " << getCurTime() << endl
	   << getSysinfo() << endl
	   << "Pointers: " << (sizeof(void *) * CHAR_BIT) << "bit" << endl
	   << endl;

	PHYSFS_Version physfs_version;

	// Determine PhysicsFS compile time version
	PHYSFS_VERSION(&physfs_version)
	os << "Compiled against PhysicsFS version: " << physfs_version << endl;

	// Determine PhysicsFS runtime version
	PHYSFS_getLinkedVersion(&physfs_version);
	os << "Running with PhysicsFS version: " << physfs_version << endl
	   << endl;

	dbgHeader = strdup(os.str().c_str());
	if (dbgHeader == NULL)
	{
		debug(LOG_FATAL, "createHeader: Out of memory!");
		abort();
		return;
	}
}

void addDumpInfo(const char *inbuffer)
{
	char ourtime[sizeof("HH:MM:SS")];

	const time_t curtime = time(NULL);
	struct tm *const timeinfo = localtime(&curtime);

	strftime(ourtime, sizeof(ourtime), "%H:%M:%S", timeinfo);

	// add timestamp to all strings
	std::ostringstream os;
	os << "[" << ourtime << "]" << inbuffer << endl;

	// Append message to miscData
	string msg(os.str());
	miscData.insert(miscData.end(), msg.begin(), msg.end());
}

void dbgDumpInit(int argc, const char **argv, const char *packageVersion)
{
	debug_register_callback(&debug_exceptionhandler_data, NULL, NULL, NULL);
	createHeader(argc, argv, packageVersion);
}
