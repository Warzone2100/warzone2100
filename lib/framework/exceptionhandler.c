/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
#include "frame.h"


static char * programName = NULL, * programVersion = NULL;


#ifdef WIN32
# include "dbghelp.h"
static LONG WINAPI windowsExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
	LPCSTR applicationName = "Warzone 2100", resultMessage = NULL;

	char miniDumpPath[MAX_PATH], buffer[MAX_PATH];

	// Write to temp dir, to support unprivileged users
	if (!GetTempPath( MAX_PATH, miniDumpPath ))
		strcpy( miniDumpPath, "c:\\temp\\" );
	strcat( miniDumpPath, "warzone2100.mdmp" );

	/*
	Alternative:
	GetModuleFileName( NULL, miniDumpPath, MAX_PATH );
	strcat( miniDumpPath, ".mdmp" );
	*/

	if ( MessageBox( NULL, "Warzone crashed unexpectedly, would you like to save a diagnostic file?", applicationName, MB_YESNO ) == IDYES )
	{
		HANDLE miniDumpFile = CreateFile( miniDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

		if (miniDumpFile != INVALID_HANDLE_VALUE)
		{
			MINIDUMP_USER_STREAM uStream = { LastReservedStream+1, strlen(programVersion), programVersion };
			MINIDUMP_USER_STREAM_INFORMATION uInfo = { 1, &uStream };
			MINIDUMP_EXCEPTION_INFORMATION eInfo = { GetCurrentThreadId(), pExceptionInfo, FALSE };

			if ( MiniDumpWriteDump(
				 	GetCurrentProcess(),
					GetCurrentProcessId(),
					miniDumpFile,
					MiniDumpNormal,
					pExceptionInfo ? &eInfo : NULL,
					&uInfo,
					NULL ) )
			{
				sprintf( buffer, "Saved dump file to '%s'", miniDumpPath );
				resultMessage = buffer;
			}
			else
			{
				sprintf( buffer, "Failed to save dump file to '%s' (error %d)", miniDumpPath, GetLastError() );
				resultMessage = buffer;
			}
			CloseHandle(miniDumpFile);
		}
		else
		{
			sprintf( buffer, "Failed to create dump file '%s' (error %d)", miniDumpPath, GetLastError() );
			resultMessage = buffer;
		}
	}

	if (resultMessage)
		MessageBox( NULL, resultMessage, applicationName, MB_OK );

	return EXCEPTION_CONTINUE_SEARCH;
}
#else
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <execinfo.h>
#include <sys/utsname.h>


static sighandler_t oldHandler[NSIG] = {SIG_DFL};

static struct utsname sysInfo;
static uint32_t sysInfoValid = 0, programNameSize = 0, programVersionSize = 0;

static const uint32_t gdmpVersion = 1;
static const uint32_t sizeOfVoidP = sizeof(void*), sizeOfUtsname = sizeof(struct utsname), sizeOfChar = sizeof(char);


static void setErrorHandler(sighandler_t signalHandler)
{
	sysInfoValid = (uname(&sysInfo) == 0);
	programNameSize = strlen(programName);
	programVersionSize = strlen(programVersion);

	// Save previous signal handler, eg. SDL parachute
	oldHandler[SIGFPE] = signal(SIGFPE, signalHandler);
	if ( oldHandler[SIGFPE] == SIG_IGN )
		signal(SIGFPE, SIG_IGN);

	oldHandler[SIGILL] = signal(SIGILL, signalHandler);
	if ( oldHandler[SIGILL] == SIG_IGN )
		signal(SIGILL, SIG_IGN);

	oldHandler[SIGSEGV] = signal(SIGSEGV, signalHandler);
	if ( oldHandler[SIGSEGV] == SIG_IGN )
		signal(SIGSEGV, SIG_IGN);

	oldHandler[SIGBUS] = signal(SIGBUS, signalHandler);
	if ( oldHandler[SIGBUS] == SIG_IGN )
		signal(SIGBUS, SIG_IGN);

	oldHandler[SIGABRT] = signal(SIGABRT, signalHandler);
	if ( oldHandler[SIGABRT] == SIG_IGN )
		signal(SIGABRT, SIG_IGN);

	oldHandler[SIGSYS] = signal(SIGSYS, signalHandler);
	if ( oldHandler[SIGSYS] == SIG_IGN )
		signal(SIGSYS, SIG_IGN);
}


static void errorHandler(int sig)
{
	static sig_atomic_t allreadyRunning = 0;

	if (allreadyRunning)
		raise(sig);
	allreadyRunning = 1;

	void * btBuffer[128];
	char * gDumpPath = "/tmp/warzone2100.gdmp";
	uint32_t btSize = backtrace(btBuffer, 128), signum = sig;

	FILE * dumpFile = fopen(gDumpPath, "w");

	if (!dumpFile)
	{
		printf("Failed to create dump file '%s'", gDumpPath);
		return;
	}

	fwrite(&gdmpVersion, sizeof(uint32_t), 1, dumpFile);

	fwrite(&sizeOfChar, sizeof(uint32_t), 1, dumpFile);
	fwrite(&sizeOfVoidP, sizeof(uint32_t), 1, dumpFile);
	fwrite(&sizeOfUtsname, sizeof(uint32_t), 1, dumpFile);

	fwrite(&sysInfoValid, sizeof(uint32_t), 1, dumpFile);
	fwrite(&sysInfo, sizeOfUtsname, 1, dumpFile);

	fwrite(&programNameSize, sizeof(uint32_t), 1, dumpFile);
	fwrite(programName, sizeOfChar, programNameSize, dumpFile);
	fwrite(&programVersionSize, sizeof(uint32_t), 1, dumpFile);
	fwrite(programVersion, sizeOfChar, programVersionSize, dumpFile);

	fwrite(&signum, sizeof(uint32_t), 1, dumpFile);

	fwrite(&btSize, sizeof(uint32_t), 1, dumpFile);
	fwrite(btBuffer, sizeOfVoidP, btSize, dumpFile);

	fclose(dumpFile);

	printf("Saved dump file to '%s'\n", gDumpPath);

	signal(sig, oldHandler[sig]);
	raise(sig);
}
#endif // WIN32


void setupExceptionHandler(char * programName_x, char * programVersion_x)
{
	programName = programName_x;
	programVersion = programVersion_x;

#ifdef WIN32
	SetUnhandledExceptionFilter(windowsExceptionHandler);
#else
	setErrorHandler(errorHandler);
#endif // WIN32
}
