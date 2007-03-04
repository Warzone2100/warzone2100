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

static char * programCommand = NULL;


#if defined(WZ_OS_WIN)

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
			MINIDUMP_USER_STREAM uStream = { LastReservedStream+1, strlen(VERSION), VERSION };
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

#elif defined(WZ_OS_LINUX)

// C99 headers:
#include <stdint.h>
#include <signal.h>
#include <string.h>

// POSIX headers:
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/utsname.h>

// GNU header:
#include <execinfo.h>


#define MAX_BACKTRACE 20
#define MAX_PID_STRING 16


typedef void(*SigHandler)(int);


static SigHandler oldHandler[NSIG] = {SIG_DFL};
static char programPID[MAX_PID_STRING] = {'\0'}, gdbPath[MAX_PATH] = {'\0'};


static void setErrorHandler(SigHandler signalHandler)
{
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

	struct utsname sysInfo;
	void * btBuffer[MAX_BACKTRACE] = {NULL};
	char * gDumpPath = "/tmp/warzone2100.gdmp";
	uint32_t btSize = backtrace(btBuffer, MAX_BACKTRACE);
	pid_t pid = 0;

	int gdbPipe[2] = {0}, dumpFile = open(gDumpPath, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);


	if (!dumpFile)
	{
		printf("Failed to create dump file '%s'", gDumpPath);
		return;
	}


	write(dumpFile, "Program command: ", strlen("Program command: "));
	write(dumpFile, programCommand, sizeof(char)*strlen(programCommand));
	write(dumpFile, "\n", sizeof(char));

	write(dumpFile, "Version: ", strlen("Version: "));
	write(dumpFile, VERSION, sizeof(char)*strlen(VERSION));
	write(dumpFile, "\n", sizeof(char));

# if defined(DEBUG)
	write(dumpFile, "Type: Debug\n", strlen("Type: Debug\n"));
# else
	write(dumpFile, "Type: Release\n", strlen("Type: Release\n"));
# endif

	write(dumpFile, "Compiled on: ", strlen("Compiled on: "));
	write(dumpFile, __DATE__, sizeof(char)*strlen(__DATE__));
	write(dumpFile, "\n\n", sizeof(char)*2);


	if (uname(&sysInfo) != 0)
		write(dumpFile, "System information may be invalid!\n",
			  strlen("System information may be invalid!\n\n"));

	write(dumpFile, "Operating system: ", strlen("Operating system: "));
	write(dumpFile, sysInfo.sysname, strlen(sysInfo.sysname));
	write(dumpFile, "\n", sizeof(char));

	write(dumpFile, "Node name: ", strlen("Node name: "));
	write(dumpFile, sysInfo.nodename, strlen(sysInfo.nodename));
	write(dumpFile, "\n", sizeof(char));

	write(dumpFile, "Release: ", strlen("Release: "));
	write(dumpFile, sysInfo.release, strlen(sysInfo.release));
	write(dumpFile, "\n", sizeof(char));

	write(dumpFile, "Version: ", strlen("Version: "));
	write(dumpFile, sysInfo.version, strlen(sysInfo.version));
	write(dumpFile, "\n", sizeof(char));

	write(dumpFile, "Machine: ", strlen("Machine: "));
	write(dumpFile, sysInfo.machine, strlen(sysInfo.machine));
	write(dumpFile, "\n\n", sizeof(char)*2);


	write(dumpFile, "Dump caused by signal: ",
		  strlen("Dump caused by signal: "));
	write(dumpFile, strsignal(sig), strlen(strsignal(sig)));
	write(dumpFile, "\n\n", sizeof(char)*2);


	backtrace_symbols_fd(btBuffer, btSize, dumpFile);


	fsync(dumpFile);


	if (pipe(gdbPipe) == 0)
	{
		pid = fork();
		if ( pid == (pid_t)0 )
		{
			close(gdbPipe[1]); // No output to pipe

			dup2(gdbPipe[0], STDIN_FILENO); // STDIN from pipe
			dup2(dumpFile, STDOUT_FILENO); // STDOUT to dumpFile

			execle(gdbPath, gdbPath, programCommand, programPID, NULL, NULL);

			fsync(dumpFile);
			close(dumpFile);
			close(gdbPipe[0]);
		}
		else if ( pid > (pid_t)0 )
		{
			close(dumpFile); // No output to dumpFile
			close(gdbPipe[0]); // No input from pipe

			write(gdbPipe[1], "backtrace full\n", strlen("backtrace full\n"));
			write(gdbPipe[1], "quit\n", strlen("quit\n"));

			waitpid(pid, NULL, 0);

			close(gdbPipe[1]);

			printf("Saved dump file to '%s'\n", gDumpPath);
		}
		else
		{
			printf("Fork failed\n");
		}
	}
	else
	{
		printf("Pipe failed\n");
	}

	signal(sig, oldHandler[sig]);
	raise(sig);
}

#endif // WZ_OS_*


void setupExceptionHandler(char * programCommand_x)
{
	programCommand = programCommand_x;

#if defined(WZ_OS_WIN)
	SetUnhandledExceptionFilter(windowsExceptionHandler);
#elif defined(WZ_OS_LINUX)
	// Get full path to 'gdb'
	FILE * whichStream = popen("which gdb", "r");
	fread(gdbPath, sizeof(char), MAX_PATH, whichStream);
	pclose(whichStream);

	*(strrchr(gdbPath, '\n')) = '\0'; // `which' adds a \n which confuses execle
	snprintf( programPID, MAX_PID_STRING, "%i", getpid() );

	setErrorHandler(errorHandler);
#endif // WZ_OS_*
}
