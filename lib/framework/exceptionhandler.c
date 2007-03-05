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
# include <stdint.h>
# include <signal.h>
# include <string.h>

// POSIX headers:
# include <unistd.h>
# include <fcntl.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/wait.h>
# include <sys/utsname.h>

// GNU header:
# include <execinfo.h>


# define MAX_BACKTRACE 20
# define MAX_PID_STRING 16


typedef void(*SigHandler)(int);


static BOOL gdbIsAvailable = FALSE;
static SigHandler oldHandler[NSIG] = {SIG_DFL};
static char programPID[MAX_PID_STRING] = {'\0'}, gdbPath[MAX_PATH] = {'\0'}, gdmpPath[MAX_PATH] = {'\0'};

/* Ugly, but arguably correct */
const char * wz_strsignal(int signum)
{
	switch (signum)
	{
	/* Standard signals */
	case SIGHUP : return "SIGHUP, Hangup";
	case SIGINT : return "SIGINT, Interrupt";
	case SIGQUIT : return "SIGQUIT, Quit";
	case SIGILL : return "SIGILL, Illegal instruction";
	case SIGTRAP : return "SIGTRAP, Trace trap";
	case SIGABRT : return "SIGABRT, Abort";
	case SIGBUS : return "SIGBUS, BUS error";
	case SIGFPE : return "SIGFPE, Floating-point exception";
	case SIGKILL : return "SIGKILL, Kill";
	case SIGUSR1 : return "SIGUSR1, User-defined signal 1";
	case SIGUSR2 : return "SIGUSR2, User-defined signal 2";
	case SIGSEGV : return "SIGSEGV, Segmentation fault";
	case SIGPIPE : return "SIGPIPE, Broken pipe";
	case SIGALRM : return "SIGALRM, Alarm clock";
	case SIGTERM : return "SIGTERM, Termination";

	/* Less standard signals */
# if defined(SIGSTKFLT)
	case SIGSTKFLT : return "SIGSTKFLT, Stack fault";
# endif

# if defined(SIGCHLD)
	case SIGCHLD : return "SIGCHLD, Child status has changed";
# elif defined(SIGCLD)
	case SIGCLD : return "SIGCLD, Child status has changed";
# endif

# if defined(SIGCONT)
	case SIGCONT : return "SIGCONT, Continue";
# endif

# if defined(SIGSTOP)
	case SIGSTOP : return "SIGSTOP, Stop";
# endif

# if defined(SIGTSTP)
	case SIGTSTP : return "SIGTSTP, Keyboard stop";
# endif

# if defined(SIGTTIN)
	case SIGTTIN : return "SIGTTIN, Background read from tty";
# endif

# if defined(SIGTTOU)
	case SIGTTOU : return "SIGTTOU, Background write to tty";
# endif

# if defined(SIGURG)
	case SIGURG : return "SIGURG, Urgent condition on socket";
# endif

# if defined(SIGXCPU)
	case SIGXCPU : return "SIGXCPU, CPU limit exceeded";
# endif

# if defined(SIGXFSZ)
	case SIGXFSZ : return "SIGXFSZ, File size limit exceeded";
# endif

# if defined(SIGVTALRM)
	case SIGVTALRM : return "SIGVTALRM, Virtual alarm clock";
# endif

# if defined(SIGPROF)
	case SIGPROF : return "SIGPROF, Profiling alarm clock";
# endif

# if defined(SIGWINCH)
	case SIGWINCH : return "SIGWINCH, Window size change";
# endif

# if defined(SIGIO)
	case SIGIO : return "SIGIO, I/O now possible";
# elif defined(SIGPOLL)
	case SIGPOLL : return "SIGPOLL, I/O now possible";
# endif

# if defined(SIGPWR)
	case SIGPWR : return "SIGPWR, Power failure restart";
# endif

# if defined(SIGSYS)
	case SIGSYS : return "SIGSYS, Bad system call";
# endif
	}

	return "Unknown signal";
}

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

	pid_t pid = 0;
	struct utsname sysInfo;
	void * btBuffer[MAX_BACKTRACE] = {NULL};
	uint32_t btSize = backtrace(btBuffer, MAX_BACKTRACE);

	int gdbPipe[2] = {0}, dumpFile = open(gdmpPath, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);


	if (!dumpFile)
	{
		printf("Failed to create dump file '%s'", gdmpPath);
		return;
	}


	write(dumpFile, "Program command: ", strlen("Program command: "));
	write(dumpFile, programCommand, strlen(programCommand));
	write(dumpFile, "\n", 1);

	write(dumpFile, "Version: ", strlen("Version: "));
	write(dumpFile, VERSION, strlen(VERSION));
	write(dumpFile, "\n", 1);

# if defined(DEBUG)
	write(dumpFile, "Type: Debug\n", strlen("Type: Debug\n"));
# else
	write(dumpFile, "Type: Release\n", strlen("Type: Release\n"));
# endif

	write(dumpFile, "Compiled on: ", strlen("Compiled on: "));
	write(dumpFile, __DATE__, strlen(__DATE__));
	write(dumpFile, "\n\n", 2);


	if (uname(&sysInfo) != 0)
		write(dumpFile, "System information may be invalid!\n",
			  strlen("System information may be invalid!\n\n"));

	write(dumpFile, "Operating system: ", strlen("Operating system: "));
	write(dumpFile, sysInfo.sysname, strlen(sysInfo.sysname));
	write(dumpFile, "\n", 1);

	write(dumpFile, "Node name: ", strlen("Node name: "));
	write(dumpFile, sysInfo.nodename, strlen(sysInfo.nodename));
	write(dumpFile, "\n", 1);

	write(dumpFile, "Release: ", strlen("Release: "));
	write(dumpFile, sysInfo.release, strlen(sysInfo.release));
	write(dumpFile, "\n", 1);

	write(dumpFile, "Version: ", strlen("Version: "));
	write(dumpFile, sysInfo.version, strlen(sysInfo.version));
	write(dumpFile, "\n", 1);

	write(dumpFile, "Machine: ", strlen("Machine: "));
	write(dumpFile, sysInfo.machine, strlen(sysInfo.machine));
	write(dumpFile, "\n\n", 2);


	write(dumpFile, "Dump caused by signal: ",
		  strlen("Dump caused by signal: "));
	write(dumpFile, wz_strsignal(sig), strlen(wz_strsignal(sig)));
	write(dumpFile, "\n\n", 2);


	backtrace_symbols_fd(btBuffer, btSize, dumpFile);
	write(dumpFile, "\n", 1);


	fsync(dumpFile);


	if (gdbIsAvailable)
	{
		if (pipe(gdbPipe) == 0)
		{
			pid = fork();
			if (pid == (pid_t)0)
			{
				char * gdbArgv[] = { gdbPath, programCommand, programPID, NULL },
				     * gdbEnv[] = {NULL};

				close(gdbPipe[1]); // No output to pipe

				dup2(gdbPipe[0], STDIN_FILENO); // STDIN from pipe
				dup2(dumpFile, STDOUT_FILENO); // STDOUT to dumpFile

				execve(gdbPath, gdbArgv, gdbEnv);
			}
			else if (pid > (pid_t)0)
			{
				close(gdbPipe[0]); // No input from pipe

				write(gdbPipe[1], "backtrace full\n" "quit\n",
					strlen("backtrace full\n" "quit\n"));

				if (waitpid(pid, NULL, 0) < 0)
				{
					printf("GDB failed\n");
				}

				close(gdbPipe[1]);
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
	}
	else
	{
		write(dumpFile, "GDB not available, no extended backtrace created\n",
			  strlen("GDB not available, no extended backtrace created\n"));
	}


	printf("Saved dump file to '%s'\n", gdmpPath);
	close(dumpFile);


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
	fread(gdbPath, 1, MAX_PATH, whichStream);
	pclose(whichStream);

	if (strlen(gdbPath) > 0)
	{
		gdbIsAvailable = TRUE;
		*(strrchr(gdbPath, '\n')) = '\0'; // `which' adds a \n which confuses exec()
	}
	else
	{
		debug(LOG_WARNING, "GDB not available, will not create extended backtrace\n");
	}

	snprintf( programPID, MAX_PID_STRING, "%i", getpid() );
	snprintf( gdmpPath, MAX_PATH, "/tmp/warzone2100.gdmp" );

	setErrorHandler(errorHandler);
#endif // WZ_OS_*
}
