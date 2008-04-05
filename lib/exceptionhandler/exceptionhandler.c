/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2008  Warzone Resurrection Project

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

#include "lib/framework/frame.h"
#include "exceptionhandler.h"

#if defined(WZ_OS_WIN)

# include "dbghelp.h"
# include "exchndl.h"


/**
 * Exception handling on Windows.
 * Ask the user whether he wants to safe a Minidump and then dump it into the temp directory.
 *
 * \param pExceptionInfo Information on the exception, passed from Windows
 * \return whether further exception handlers (i.e. the Windows internal one) should be invoked
 */
static LONG WINAPI windowsExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
	LPCSTR applicationName = "Warzone 2100";

	char miniDumpPath[PATH_MAX] = {'\0'}, resultMessage[PATH_MAX] = {'\0'};

	// Write to temp dir, to support unprivileged users
	if (!GetTempPathA( PATH_MAX, miniDumpPath ))
	{
		strlcpy(miniDumpPath, "c:\\temp\\", sizeof(miniDumpPath));
	}

	// Append the filename
	strlcat(miniDumpPath, "warzone2100.mdmp", sizeof(miniDumpPath));

	/*
	Alternative:
	GetModuleFileName( NULL, miniDumpPath, MAX_PATH );

	// Append extension
	strlcat(miniDumpPath, ".mdmp", sizeof(miniDumpPath));
	*/

	if ( MessageBoxA( NULL, "Warzone crashed unexpectedly, would you like to save a diagnostic file?", applicationName, MB_YESNO ) == IDYES )
	{
		HANDLE miniDumpFile = CreateFileA( miniDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

		if (miniDumpFile != INVALID_HANDLE_VALUE)
		{
			MINIDUMP_USER_STREAM uStream = { LastReservedStream+1, strlen(PACKAGE_VERSION), PACKAGE_VERSION };
			MINIDUMP_USER_STREAM_INFORMATION uInfo = { 1, &uStream };
			MINIDUMP_EXCEPTION_INFORMATION eInfo = { GetCurrentThreadId(), pExceptionInfo, false };

			if ( MiniDumpWriteDump(
				 	GetCurrentProcess(),
					GetCurrentProcessId(),
					miniDumpFile,
					MiniDumpNormal,
					pExceptionInfo ? &eInfo : NULL,
					&uInfo,
					NULL ) )
			{
				snprintf(resultMessage, sizeof(resultMessage), "Saved dump file to '%s'", miniDumpPath);
			}
			else
			{
				snprintf(resultMessage, sizeof(resultMessage), "Failed to save dump file to '%s' (error %d)", miniDumpPath, (int)GetLastError());
			}

			CloseHandle(miniDumpFile);
		}
		else
		{
			snprintf(resultMessage, sizeof(resultMessage), "Failed to create dump file '%s' (error %d)", miniDumpPath, (int)GetLastError());
		}

		// Guarantee to nul-terminate
		resultMessage[sizeof(resultMessage) - 1] = '\0';

		MessageBoxA( NULL, resultMessage, applicationName, MB_OK );
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

#elif defined(WZ_OS_UNIX) && !defined(WZ_OS_MAC)

// C99 headers:
# include <stdint.h>
# include <signal.h>
# include <string.h>

// POSIX headers:
# include <unistd.h>
# include <fcntl.h>
# include <time.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/wait.h>
# include <sys/utsname.h>

// GNU extension for backtrace():
# if defined(__GLIBC__)
#  include <execinfo.h>
#  define MAX_BACKTRACE 20
# endif


# define MAX_PID_STRING 16
# define MAX_DATE_STRING 256


typedef void(*SigActionHandler)(int, siginfo_t *, void *);


#ifdef WZ_OS_MAC
static struct sigaction oldAction[32];
#else
static struct sigaction oldAction[NSIG];
#endif


static struct utsname sysInfo;
static BOOL gdbIsAvailable = false, programIsAvailable = false, sysInfoValid = false;
static char
	executionDate[MAX_DATE_STRING] = {'\0'},
	programPID[MAX_PID_STRING] = {'\0'},
	programPath[PATH_MAX] = {'\0'},
	gdbPath[PATH_MAX] = {'\0'};
static const char * gdmpPath = "/tmp/warzone2100.gdmp";


/**
 * Signal number to string mapper.
 * Also takes into account the signal code with details about the signal.
 *
 * \param signum Signal number
 * \param sigcode Signal code
 * \return String with the description of the signal. "Unknown signal" when no description is available.
 */
static const char * wz_strsignal(int signum, int sigcode)
{
	switch (signum)
	{
		case SIGABRT:
			return "SIGABRT: Process abort signal";
		case SIGALRM:
			return "SIGALRM: Alarm clock";
		case SIGBUS:
			switch (sigcode)
			{
				case BUS_ADRALN:
					return "SIGBUS: Access to an undefined portion of a memory object: Invalid address alignment";
				case BUS_ADRERR:
					return "SIGBUS: Access to an undefined portion of a memory object: Nonexistent physical address";
				case BUS_OBJERR:
					return "SIGBUS: Access to an undefined portion of a memory object: Object-specific hardware error";
				default:
					return "SIGBUS: Access to an undefined portion of a memory object";
			}
		case SIGCHLD:
			switch (sigcode)
			{
				case CLD_EXITED:
					return "SIGCHLD: Child process terminated, stopped, or continued: Child has exited";
				case CLD_KILLED:
					return "SIGCHLD: Child process terminated, stopped, or continued: Child has terminated abnormally and did not create a core file";
				case CLD_DUMPED:
					return "SIGCHLD: Child process terminated, stopped, or continued: Child has terminated abnormally and created a core file";
				case CLD_TRAPPED:
					return "SIGCHLD: Child process terminated, stopped, or continued: Traced child has trapped";
				case CLD_STOPPED:
					return "SIGCHLD: Child process terminated, stopped, or continued: Child has stopped";
				case CLD_CONTINUED:
					return "SIGCHLD: Child process terminated, stopped, or continued: Stopped child has continued";
			}
		case SIGCONT:
			return "SIGCONT: Continue executing, if stopped";
		case SIGFPE:
			switch (sigcode)
			{
				case FPE_INTDIV:
					return "SIGFPE: Erroneous arithmetic operation: Integer divide by zero";
				case FPE_INTOVF:
					return "SIGFPE: Erroneous arithmetic operation: Integer overflow";
				case FPE_FLTDIV:
					return "SIGFPE: Erroneous arithmetic operation: Floating-point divide by zero";
				case FPE_FLTOVF:
					return "SIGFPE: Erroneous arithmetic operation: Floating-point overflow";
				case FPE_FLTUND:
					return "SIGFPE: Erroneous arithmetic operation: Floating-point underflow";
				case FPE_FLTRES:
					return "SIGFPE: Erroneous arithmetic operation: Floating-point inexact result";
				case FPE_FLTINV:
					return "SIGFPE: Erroneous arithmetic operation: Invalid floating-point operation";
				case FPE_FLTSUB:
					return "SIGFPE: Erroneous arithmetic operation: Subscript out of range";
				default:
					return "SIGFPE: Erroneous arithmetic operation";
			};
		case SIGHUP:
			return "SIGHUP: Hangup";
		case SIGILL:
			switch (sigcode)
			{
				case ILL_ILLOPC:
					return "SIGILL: Illegal instruction: Illegal opcode";
				case ILL_ILLOPN:
					return "SIGILL: Illegal instruction: Illegal operand";
				case ILL_ILLADR:
					return "SIGILL: Illegal instruction: Illegal addressing mode";
				case ILL_ILLTRP:
					return "SIGILL: Illegal instruction: Illegal trap";
				case ILL_PRVOPC:
					return "SIGILL: Illegal instruction: Privileged opcode";
				case ILL_PRVREG:
					return "SIGILL: Illegal instruction: Privileged register";
				case ILL_COPROC:
					return "SIGILL: Illegal instruction: Coprocessor error";
				case ILL_BADSTK:
					return "SIGILL: Illegal instruction: Internal stack error";
				default:
					return "SIGILL: Illegal instruction";
			}
		case SIGINT:
			return "SIGINT: Terminal interrupt signal";
		case SIGKILL:
			return "SIGKILL: Kill";
		case SIGPIPE:
			return "SIGPIPE: Write on a pipe with no one to read it";
		case SIGQUIT:
			return "SIGQUIT: Terminal quit signal";
		case SIGSEGV:
			switch (sigcode)
			{
				case SEGV_MAPERR:
					return "SIGSEGV: Invalid memory reference: Address not mapped to object";
				case SEGV_ACCERR:
					return "SIGSEGV: Invalid memory reference: Invalid permissions for mapped object";
				default:
					return "SIGSEGV: Invalid memory reference";
			}
		case SIGSTOP:
			return "SIGSTOP: Stop executing";
		case SIGTERM:
			return "SIGTERM: Termination signal";
		case SIGTSTP:
			return "SIGTSTP: Terminal stop signal";
		case SIGTTIN:
			return "SIGTTIN: Background process attempting read";
		case SIGTTOU:
			return "SIGTTOU: Background process attempting write";
		case SIGUSR1:
			return "SIGUSR1: User-defined signal 1";
		case SIGUSR2:
			return "SIGUSR2: User-defined signal 2";
#if _XOPEN_UNIX
		case SIGPOLL:
			switch (sigcode)
			{
				case POLL_IN:
					return "SIGPOLL: Pollable event: Data input available";
				case POLL_OUT:
					return "SIGPOLL: Pollable event: Output buffers available";
				case POLL_MSG:
					return "SIGPOLL: Pollable event: Input message available";
				case POLL_ERR:
					return "SIGPOLL: Pollable event: I/O error";
				case POLL_PRI:
					return "SIGPOLL: Pollable event: High priority input available";
				case POLL_HUP:
					return "SIGPOLL: Pollable event: Device disconnected.";
				default:
					return "SIGPOLL: Pollable event";
			}
		case SIGPROF:
			return "SIGPROF: Profiling timer expired";
		case SIGSYS:
			return "SIGSYS: Bad system call";
		case SIGTRAP:
			switch (sigcode)
			{
				case TRAP_BRKPT:
					return "SIGTRAP: Trace/breakpoint trap: Process breakpoint";
				case TRAP_TRACE:
					return "SIGTRAP: Trace/breakpoint trap: Process trace trap";
				default:
					return "SIGTRAP: Trace/breakpoint trap";
			}
#endif // _XOPEN_UNIX
		case SIGURG:
			return "SIGURG: High bandwidth data is available at a socket";
#if _XOPEN_UNIX
		case SIGVTALRM:
			return "SIGVTALRM: Virtual timer expired";
		case SIGXCPU:
			return "SIGXCPU: CPU time limit exceeded";
		case SIGXFSZ:
			return "SIGXFSZ: File size limit exceeded";
#endif // _XOPEN_UNIX
		default:
			return "Unknown signal";
	}
}


/**
 * Set signal handlers for fatal signals on POSIX systems
 *
 * \param signalHandler Pointer to the signal handler function
 */
static void setFatalSignalHandler(SigActionHandler signalHandler)
{
	struct sigaction new_handler;

	new_handler.sa_sigaction = signalHandler;
	sigemptyset(&new_handler.sa_mask);
	new_handler.sa_flags = SA_SIGINFO;

	sigaction(SIGABRT, NULL, &oldAction[SIGABRT]);
	if (oldAction[SIGABRT].sa_handler != SIG_IGN)
		sigaction(SIGABRT, &new_handler, NULL);

	sigaction(SIGBUS, NULL, &oldAction[SIGBUS]);
	if (oldAction[SIGBUS].sa_handler != SIG_IGN)
		sigaction(SIGBUS, &new_handler, NULL);

	sigaction(SIGFPE, NULL, &oldAction[SIGFPE]);
	if (oldAction[SIGFPE].sa_handler != SIG_IGN)
		sigaction(SIGFPE, &new_handler, NULL);

	sigaction(SIGILL, NULL, &oldAction[SIGILL]);
	if (oldAction[SIGILL].sa_handler != SIG_IGN)
		sigaction(SIGILL, &new_handler, NULL);

	sigaction(SIGQUIT, NULL, &oldAction[SIGQUIT]);
	if (oldAction[SIGQUIT].sa_handler != SIG_IGN)
		sigaction(SIGQUIT, &new_handler, NULL);

	sigaction(SIGSEGV, NULL, &oldAction[SIGSEGV]);
	if (oldAction[SIGSEGV].sa_handler != SIG_IGN)
		sigaction(SIGSEGV, &new_handler, NULL);

#if _XOPEN_UNIX
	sigaction(SIGSYS, NULL, &oldAction[SIGSYS]);
	if (oldAction[SIGSYS].sa_handler != SIG_IGN)
		sigaction(SIGSYS, &new_handler, NULL);

	sigaction(SIGTRAP, NULL, &oldAction[SIGTRAP]);
	if (oldAction[SIGTRAP].sa_handler != SIG_IGN)
		sigaction(SIGTRAP, &new_handler, NULL);

	sigaction(SIGXCPU, NULL, &oldAction[SIGXCPU]);
	if (oldAction[SIGXCPU].sa_handler != SIG_IGN)
		sigaction(SIGXCPU, &new_handler, NULL);

	sigaction(SIGXFSZ, NULL, &oldAction[SIGXFSZ]);
	if (oldAction[SIGXFSZ].sa_handler != SIG_IGN)
		sigaction(SIGXFSZ, &new_handler, NULL);
#endif // _XOPEN_UNIX
}


/**
 * Exception (signal) handling on POSIX systems.
 * Dumps info about the system incl. backtrace (when GLibC or GDB is present) to /tmp/warzone2100.gdmp
 *
 * \param signum Signal number
 * \param siginfo Signal info
 * \param sigcontext Signal context
 */
static void posixExceptionHandler(int signum, siginfo_t * siginfo, WZ_DECL_UNUSED void * sigcontext)
{
	static sig_atomic_t allreadyRunning = 0;

	if (allreadyRunning)
		raise(signum);
	allreadyRunning = 1;

# if defined(__GLIBC__)
	void * btBuffer[MAX_BACKTRACE] = {NULL};
	uint32_t btSize = backtrace(btBuffer, MAX_BACKTRACE);
# endif

	pid_t pid = 0;
	int gdbPipe[2] = {0}, dumpFile = open(gdmpPath, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);


	if (!dumpFile)
	{
		printf("Failed to create dump file '%s'", gdmpPath);
		return;
	}


	write(dumpFile, "Program: ", strlen("Program: "));
	write(dumpFile, programPath, strlen(programPath));
	write(dumpFile, "\n", 1);

	write(dumpFile, "Version: ", strlen("Version: "));
	write(dumpFile, PACKAGE_VERSION, strlen(PACKAGE_VERSION));
	write(dumpFile, "\n", 1);

	write(dumpFile, "Distributor: ", strlen("Distributor: "));
	write(dumpFile, PACKAGE_DISTRIBUTOR, strlen(PACKAGE_DISTRIBUTOR));
	write(dumpFile, "\n", 1);

# if defined(DEBUG)
	write(dumpFile, "Type: Debug\n", strlen("Type: Debug\n"));
# else
	write(dumpFile, "Type: Release\n", strlen("Type: Release\n"));
# endif

	write(dumpFile, "Compiled on: ", strlen("Compiled on: "));
	write(dumpFile, __DATE__ " " __TIME__, strlen(__DATE__ " " __TIME__));
	write(dumpFile, "\n", 1);

	write(dumpFile, "Compiled by: ", strlen("Compiled by: "));
# if defined(WZ_CC_GNU) && !defined(WZ_CC_INTEL)
	write(dumpFile, "GCC " __VERSION__, strlen("GCC " __VERSION__));
# elif defined(WZ_CC_INTEL)
	// Intel includes the compiler name within the version string
	write(dumpFile, __VERSION__, strlen(__VERSION__));
# else
	write(dumpFile, "UNKNOWN", strlen("UNKNOWN"));
# endif
	write(dumpFile, "\n", 1);

	write(dumpFile, "Executed on: ", strlen("Executed on: "));
	write(dumpFile, executionDate, strlen(executionDate));
	write(dumpFile, "\n", 1);

	if (!sysInfoValid)
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


	if (sizeof(void*) == 4)
		write(dumpFile, "Pointers: 32bit\n\n", strlen("Pointers: 32bit\n\n"));
	else if (sizeof(void*) == 8)
		write(dumpFile, "Pointers: 64bit\n\n", strlen("Pointers: 64bit\n\n"));
	else
		write(dumpFile, "Pointers: Unknown\n\n", strlen("Pointers: Unknown\n\n"));


	write(dumpFile, "Dump caused by signal: ", strlen("Dump caused by signal: "));

	const char * signal = wz_strsignal(siginfo->si_signo, siginfo->si_code);
	write(dumpFile, signal, strlen(signal));
	write(dumpFile, "\n\n", 2);

	dumpLog(dumpFile); // dump out the last two log calls

# if defined(__GLIBC__)
	// Dump raw backtrace in case GDB is not available or fails
	write(dumpFile, "GLIBC raw backtrace:\n", strlen("GLIBC raw backtrace:\n"));
	backtrace_symbols_fd(btBuffer, btSize, dumpFile);
	write(dumpFile, "\n", 1);
# else
	write(dumpFile, "GLIBC not available, no raw backtrace dumped\n\n",
		  strlen("GLIBC not available, no raw backtrace dumped\n\n"));
# endif


	// Make sure everything is written before letting GDB write to it
	fsync(dumpFile);


	if (programIsAvailable && gdbIsAvailable)
	{
		if (pipe(gdbPipe) == 0)
		{
			pid = fork();
			if (pid == (pid_t)0)
			{
				char *gdbArgv[] = { gdbPath, programPath, programPID, NULL };
				char *gdbEnv[] = { NULL };

				close(gdbPipe[1]); // No output to pipe

				dup2(gdbPipe[0], STDIN_FILENO); // STDIN from pipe
				dup2(dumpFile, STDOUT_FILENO); // STDOUT to dumpFile

				write(dumpFile, "GDB extended backtrace:\n",
					  strlen("GDB extended backtrace:\n"));

				execve(gdbPath, (char **)gdbArgv, (char **)gdbEnv);
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
		write(dumpFile, "No extended backtrace dumped:\n",
			strlen("No extended backtrace dumped:\n"));
		if (!programIsAvailable)
		{
			write(dumpFile, "- Program path not available\n",
				strlen("- Program path not available\n"));
		}
		if (!gdbIsAvailable)
		{
			write(dumpFile, "- GDB not available\n",
				strlen("- GDB not available\n"));
		}
	}


	printf("Saved dump file to '%s'\n", gdmpPath);
	close(dumpFile);


	sigaction(signum, &oldAction[signum], NULL);
	raise(signum);
}


#endif // WZ_OS_*


/**
 * Setup the exception handler responsible for target OS.
 *
 * \param programCommand Command used to launch this program. Only used for POSIX handler.
 */
void setupExceptionHandler(const char * programCommand)
{
#if defined(WZ_OS_WIN)
	SetUnhandledExceptionFilter(windowsExceptionHandler);
	ExchndlSetup();
#elif defined(WZ_OS_UNIX) && !defined(WZ_OS_MAC)
	// Prepare 'which' command for popen
	char whichProgramCommand[PATH_MAX] = {'\0'};
	snprintf( whichProgramCommand, PATH_MAX, "which %s", programCommand );

	// Get full path to this program. Needed for gdb to find the binary.
	FILE * whichProgramStream = popen(whichProgramCommand, "r");
	fread( programPath, 1, PATH_MAX, whichProgramStream );
	pclose(whichProgramStream);

	// Were we able to find ourselves?
	if (strlen(programPath) > 0)
	{
		programIsAvailable = true;
		*(strrchr(programPath, '\n')) = '\0'; // `which' adds a \n which confuses exec()
		debug(LOG_WZ, "Found us at %s", programPath);
	}
	else
	{
		debug(LOG_WARNING, "Could not retrieve full path to %s, will not create extended backtrace\n", programCommand);
	}

	// Get full path to 'gdb'
	FILE * whichGDBStream = popen("which gdb", "r");
	fread( gdbPath, 1, PATH_MAX, whichGDBStream );
	pclose(whichGDBStream);

	// Did we find GDB?
	if (strlen(gdbPath) > 0)
	{
		gdbIsAvailable = true;
		*(strrchr(gdbPath, '\n')) = '\0'; // `which' adds a \n which confuses exec()
		debug(LOG_WZ, "Found gdb at %s", gdbPath);
	}
	else
	{
		debug(LOG_WARNING, "GDB not available, will not create extended backtrace\n");
	}

	sysInfoValid = (uname(&sysInfo) == 0);

	time_t currentTime = time(NULL);
	strlcpy(executionDate, ctime(&currentTime), sizeof(executionDate));

	snprintf(programPID, sizeof(programPID), "%i", getpid());

	setFatalSignalHandler(posixExceptionHandler);
#endif // WZ_OS_*
}
