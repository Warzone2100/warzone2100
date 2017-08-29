/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2017  Warzone 2100 Project

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
#include "lib/framework/string_ext.h"
#include "exceptionhandler.h"
#include "dumpinfo.h"

#if defined(WZ_OS_WIN)
#include <tchar.h>
#include <shlobj.h>
#include <shlwapi.h>

# include "dbghelp.h"
# include "exchndl.h"

#if !defined(WZ_CC_MINGW)
static LPTOP_LEVEL_EXCEPTION_FILTER prevExceptionHandler = NULL;

/**
 * Exception handling on Windows.
 * Ask the user whether he wants to safe a Minidump and then dump it into the temp directory.
 * NOTE: This is only for MSVC compiled programs.
 *
 * \param pExceptionInfo Information on the exception, passed from Windows
 * \return whether further exception handlers (i.e. the Windows internal one) should be invoked
 */
static LONG WINAPI windowsExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
	LPCSTR applicationName = "Warzone 2100";

	char miniDumpPath[PATH_MAX] = {'\0'}, resultMessage[PATH_MAX] = {'\0'};

	// Write to temp dir, to support unprivileged users
	if (!GetTempPathA(sizeof(miniDumpPath), miniDumpPath))
	{
		sstrcpy(miniDumpPath, "c:\\temp\\");
	}

	// Append the filename
	sstrcat(miniDumpPath, "warzone2100.mdmp");

	/*
	Alternative:
	GetModuleFileName( NULL, miniDumpPath, MAX_PATH );

	// Append extension
	sstrcat(miniDumpPath, ".mdmp");
	*/

	if (MessageBoxA(NULL, "Warzone crashed unexpectedly, would you like to save a diagnostic file?", applicationName, MB_YESNO) == IDYES)
	{
		HANDLE miniDumpFile = CreateFileA(miniDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (miniDumpFile != INVALID_HANDLE_VALUE)
		{
			MINIDUMP_USER_STREAM uStream = { LastReservedStream + 1, strlen(PACKAGE_VERSION), PACKAGE_VERSION };
			MINIDUMP_USER_STREAM_INFORMATION uInfo = { 1, &uStream };
			MINIDUMP_EXCEPTION_INFORMATION eInfo = { GetCurrentThreadId(), pExceptionInfo, false };

			if (MiniDumpWriteDump(
			        GetCurrentProcess(),
			        GetCurrentProcessId(),
			        miniDumpFile,
			        MiniDumpNormal,
			        pExceptionInfo ? &eInfo : NULL,
			        &uInfo,
			        NULL))
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

		MessageBoxA(NULL, resultMessage, applicationName, MB_OK);
	}

	if (prevExceptionHandler)
	{
		return prevExceptionHandler(pExceptionInfo);
	}
	else
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
}
#endif

#elif defined(WZ_OS_UNIX) && !defined(WZ_OS_MAC)

// C99 headers:
# include <stdint.h>
# include <signal.h>
# include <string.h>
# include <errno.h>

// POSIX headers:
# include <unistd.h>
# include <fcntl.h>
# include <time.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/wait.h>
#ifdef HAVE_SYS_UCONTEXT_H
# include <sys/ucontext.h>
#endif
# include <sys/utsname.h>
#ifdef WZ_OS_LINUX
# include <sys/prctl.h>
#ifndef PR_SET_PTRACER
# define PR_SET_PTRACER 0x59616d61  // prctl will ignore unknown options
#endif
#endif

// GNU extension for backtrace():
# if defined(__GLIBC__)
#  include <execinfo.h>
#  define MAX_BACKTRACE 20
# endif

# define MAX_PID_STRING 16
# define MAX_DATE_STRING 256


#ifdef SA_SIGINFO
typedef void(*SigActionHandler)(int, siginfo_t *, void *);
#else
typedef void(*SigActionHandler)(int);
#endif


#ifdef WZ_OS_MAC
static struct sigaction oldAction[32];
#elif defined(_NSIG)
static struct sigaction oldAction[_NSIG];
#else
static struct sigaction oldAction[NSIG];
#endif


static struct utsname sysInfo;
static bool gdbIsAvailable = false, programIsAvailable = false, sysInfoValid = false;
static char
executionDate[MAX_DATE_STRING] = {'\0'},
                                 programPID[MAX_PID_STRING] = {'\0'},
                                         programPath[PATH_MAX] = {'\0'},
                                                 gdbPath[PATH_MAX] = {'\0'},
                                                         WritePath[PATH_MAX] = {'\0'};


/**
 * Signal number to string mapper.
 * Also takes into account the signal code with details about the signal.
 *
 * \param signum Signal number
 * \param sigcode Signal code
 * \return String with the description of the signal. "Unknown signal" when no description is available.
 */
#ifdef SA_SIGINFO
static const char *wz_strsignal(int signum, int sigcode)
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
	case SIGTERM:
		return "SIGTERM: Termination signal";
	case SIGUSR1:
		return "SIGUSR1: User-defined signal 1";
	case SIGUSR2:
		return "SIGUSR2: User-defined signal 2";
#if _XOPEN_UNIX
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
#endif // SA_SIGINFO


/**
 * Set signal handlers for fatal signals on POSIX systems
 *
 * \param signalHandler Pointer to the signal handler function
 */
static void setFatalSignalHandler(SigActionHandler signalHandler)
{
	struct sigaction new_handler;

	sigemptyset(&new_handler.sa_mask);
#ifdef SA_SIGINFO
	new_handler.sa_flags = SA_SIGINFO;
	new_handler.sa_sigaction = signalHandler;
#else
	new_handler.sa_handler = signalHandler;
#endif

	sigaction(SIGABRT, nullptr, &oldAction[SIGABRT]);
	if (oldAction[SIGABRT].sa_handler != SIG_IGN)
	{
		sigaction(SIGABRT, &new_handler, nullptr);
	}

	sigaction(SIGBUS, nullptr, &oldAction[SIGBUS]);
	if (oldAction[SIGBUS].sa_handler != SIG_IGN)
	{
		sigaction(SIGBUS, &new_handler, nullptr);
	}

	sigaction(SIGFPE, nullptr, &oldAction[SIGFPE]);
	if (oldAction[SIGFPE].sa_handler != SIG_IGN)
	{
		sigaction(SIGFPE, &new_handler, nullptr);
	}

	sigaction(SIGILL, nullptr, &oldAction[SIGILL]);
	if (oldAction[SIGILL].sa_handler != SIG_IGN)
	{
		sigaction(SIGILL, &new_handler, nullptr);
	}

	sigaction(SIGQUIT, nullptr, &oldAction[SIGQUIT]);
	if (oldAction[SIGQUIT].sa_handler != SIG_IGN)
	{
		sigaction(SIGQUIT, &new_handler, nullptr);
	}

	sigaction(SIGSEGV, nullptr, &oldAction[SIGSEGV]);
	if (oldAction[SIGSEGV].sa_handler != SIG_IGN)
	{
		sigaction(SIGSEGV, &new_handler, nullptr);
	}

#if _XOPEN_UNIX
	sigaction(SIGSYS, nullptr, &oldAction[SIGSYS]);
	if (oldAction[SIGSYS].sa_handler != SIG_IGN)
	{
		sigaction(SIGSYS, &new_handler, nullptr);
	}

	sigaction(SIGXCPU, nullptr, &oldAction[SIGXCPU]);
	if (oldAction[SIGXCPU].sa_handler != SIG_IGN)
	{
		sigaction(SIGXCPU, &new_handler, nullptr);
	}

	sigaction(SIGXFSZ, nullptr, &oldAction[SIGXFSZ]);
	if (oldAction[SIGXFSZ].sa_handler != SIG_IGN)
	{
		sigaction(SIGXFSZ, &new_handler, nullptr);
	}

	// ignore SIGTRAP
	new_handler.sa_handler = SIG_IGN;
	sigaction(SIGTRAP, &new_handler, &oldAction[SIGTRAP]);
#endif // _XOPEN_UNIX
}

static ssize_t writeAll(int const fd, const char* const str)
{
	size_t const to_write = strlen(str);
	size_t written = 0;
	while (written < to_write)
	{
		ssize_t ret = write(fd, str + written, to_write - written);
		if (ret == -1)
		{
			switch (errno)
			{
				case EAGAIN:
#if defined(EWOULDBLOCK) && EAGAIN != EWOULDBLOCK
				case EWOULDBLOCK:
#endif
				case EINTR:
					continue;
				default:
					return -1;
			}
		}
		written += ret;
	}

	return written;
}

/**
 * Spawn a new GDB process and attach it to the current process.
 *
 * @param dumpFile          a POSIX file descriptor to write GDB's output to,
 *                          it will also be used to write failure messages to.
 * @param[out] gdbWritePipe a POSIX file descriptor linked to GDB's stdin.
 *
 * @return 0 if we failed to spawn a new process, a non-zero process ID if we
 *           successfully spawned a new process.
 *
 * @post If the function returned a non-zero process ID a new process has
 *       successfully been spawned. This doesn't mean that 'gdb' was
 *       successfully started though. If 'gdb' failed to start the read end of
 *       the pipe will be closed, also the spawned process will give 1 as its
 *       return code.
 *
 * @post If the function returned a non-zero process ID *gdbWritePipe will
 *       contain a valid POSIX file descriptor representing GDB's stdin. If the
 *       function was unsuccessful and returned zero *gdbWritePipe's value will
 *       be unchanged.
 */
static pid_t execGdb(int const dumpFile, int *gdbWritePipe)
{
	int gdbPipe[2];
	pid_t pid;
	char *gdbArgv[] = { gdbPath, programPath, programPID, nullptr };
	char *gdbEnv[] = { nullptr };
	/* Check if the "bare minimum" is available: GDB and an absolute path
	 * to our program's binary.
	 */
	if (!programIsAvailable
	    || !gdbIsAvailable)
	{
		writeAll(dumpFile, "No extended backtrace dumped:\n");

		if (!programIsAvailable)
		{
			writeAll(dumpFile, "- Program path not available\n");
		}
		if (!gdbIsAvailable)
		{
			writeAll(dumpFile, "- GDB not available\n");
		}

		return 0;
	}

	// Create a pipe to use for communication with 'gdb'
	if (pipe(gdbPipe) == -1)
	{
		writeAll(dumpFile, "Pipe failed\n");

		printf("Pipe failed\n");

		return 0;
	}

	// Fork a new child process
	pid = fork();
	if (pid == -1)
	{
		writeAll(dumpFile, "Fork failed\n");

		printf("Fork failed\n");

		// Clean up our pipe
		close(gdbPipe[0]);
		close(gdbPipe[1]);

		return 0;
	}

	// Check to see if we're the parent
	if (pid != 0)
	{
#ifdef WZ_OS_LINUX
		// Allow tracing the process, some hardened kernel configurations disallow this.
		prctl(PR_SET_PTRACER, pid, 0, 0, 0);
#endif

		// Return the write end of the pipe
		*gdbWritePipe = gdbPipe[1];

		return pid;
	}

	close(gdbPipe[1]); // No output to pipe

	dup2(gdbPipe[0], STDIN_FILENO); // STDIN from pipe
	dup2(dumpFile, STDOUT_FILENO); // STDOUT to dumpFile

	writeAll(dumpFile, "GDB extended backtrace:\n");

	/* If execve() is successful it effectively prevents further
	 * execution of this function.
	 */
	execve(gdbPath, (char **)gdbArgv, (char **)gdbEnv);

	// If we get here it means that execve failed!
	writeAll(dumpFile, "execcv(\"gdb\") failed\n");

	// Terminate the child, indicating failure
	_exit(1);
}

/**
 * Dumps a backtrace of the stack to the given output stream.
 *
 * @param dumpFile a POSIX file descriptor to write the resulting backtrace to.
 *
 * @return false if any failure occurred, preventing a full "extended"
 *               backtrace.
 */
#ifdef SA_SIGINFO
static bool gdbExtendedBacktrace(int const dumpFile, const ucontext_t *sigcontext)
#else
static bool gdbExtendedBacktrace(int const dumpFile)
#endif
{
	/*
	 * Pointer to the stackframe of the function containing faulting
	 * instruction (assuming
	 * -fomit-frame-pointer has *not* been used).
	 *
	 * The frame pointer register (x86: %ebp, x64: %rbp) point's to the
	 * local variables of the current function (which are preceded by the
	 * previous frame pointer and the return address).  Hence the
	 * additions to the frame-pointer register's content.
	 */
	void const *const frame =
#if   defined(SA_SIGINFO) && defined(REG_RBP)
	    sigcontext ? (void *)(sigcontext->uc_mcontext.gregs[REG_RBP] + sizeof(greg_t) + sizeof(void (*)(void))) : nullptr;
#elif defined(SA_SIGINFO) && defined(REG_EBP)
	    sigcontext ? (void *)(sigcontext->uc_mcontext.gregs[REG_EBP] + sizeof(greg_t) + sizeof(void (*)(void))) : NULL;
#else
	    NULL;
#endif

	/*
	 * Faulting instruction.
	 */
	void (*instruction)(void) =
#if   defined(SA_SIGINFO) && defined(REG_RIP)
	    sigcontext ? (void (*)(void))sigcontext->uc_mcontext.gregs[REG_RIP] : nullptr;
#elif defined(SA_SIGINFO) && defined(REG_EIP)
	    sigcontext ? (void (*)(void))sigcontext->uc_mcontext.gregs[REG_EIP] : NULL;
#else
	    NULL;
#endif

	// Spawn a GDB instance and retrieve a pipe to its stdin
	int gdbPipe;
	int status;
	pid_t wpid;
	// Retrieve a full stack backtrace
	static const char gdbCommands[] = "thread apply all backtrace full\n"

	                                  // Move to the stack frame where we triggered the crash
	                                  "frame 4\n"

	                                  // Show the assembly code associated with that stack frame
	                                  "disassemble /m\n"

	                                  // Show the content of all registers
	                                  "info registers\n"
	                                  "quit\n";
	const pid_t pid = execGdb(dumpFile, &gdbPipe);
	if (pid == 0)
	{
		return false;
	}

	// Disassemble the faulting instruction (if we know it)
	if (instruction)
	{
		dprintf(gdbPipe, "x/i %p\n", (void *)instruction);
	}

	// We have an intelligent guess for the *correct* frame, disassemble *that* one.
	if (frame)
	{
		dprintf(gdbPipe, "frame %p\n"
		        "disassemble /m\n", frame);
	}

	writeAll(gdbPipe, gdbCommands);

	/* Flush our end of the pipe to make sure that GDB has all commands
	 * directly available to it.
	 */
	fsync(gdbPipe);

	// Wait for our child to terminate
	wpid = waitpid(pid, &status, 0);

	// Clean up our end of the pipe
	close(gdbPipe);

	// waitpid(): on error, -1 is returned
	if (wpid == -1)
	{
		writeAll(dumpFile, "GDB failed\n");
		printf("GDB failed\n");

		return false;
	}

	/* waitpid(): on success, returns the process ID of the child whose
	 * state has changed
	 *
	 * We only have one child, from our fork() call above, thus these PIDs
	 * should match.
	 */
	assert(pid == wpid);

	/* Check whether our child (which presumably was GDB, but doesn't
	 * necessarily have to be) didn't terminate normally or had a non-zero
	 * return code.
	 */
	if (!WIFEXITED(status)
	    || WEXITSTATUS(status) != 0)
	{
		writeAll(dumpFile, "GDB failed\n");
		printf("GDB failed\n");

		return false;
	}

	return true;
}


/**
 * Exception (signal) handling on POSIX systems.
 * Dumps info about the system incl. backtrace (when GLibC or GDB is present) to /tmp/warzone2100.gdmp
 *
 * \param signum Signal number
 * \param siginfo Signal info
 * \param sigcontext Signal context
 */
#ifdef SA_SIGINFO
static void posixExceptionHandler(int signum, siginfo_t *siginfo, void *sigcontext)
#else
static void posixExceptionHandler(int signum)
#endif
{
	static sig_atomic_t allreadyRunning = 0;
	// XXXXXX will be converted into random characters by mkstemp(3)
	static const char gdmpFile[] = "warzone2100.gdmp-XXXXXX";
	char gdmpPath[PATH_MAX] = {'\0'};
	char dumpFilename[PATH_MAX];
	int dumpFile;
	const char *signal;
# if defined(__GLIBC__)
	void *btBuffer[MAX_BACKTRACE] = {nullptr};
	uint32_t btSize = backtrace(btBuffer, MAX_BACKTRACE);
# endif

	if (allreadyRunning)
	{
		raise(signum);
	}
	allreadyRunning = 1;
	// we use our write directory (which is the only directory that wz should have access to)
	// and stuff it into our logs directory (same as on windows)
	ssprintf(gdmpPath, "%slogs/%s", WritePath, gdmpFile);
	sstrcpy(dumpFilename, gdmpPath);

	dumpFile = mkstemp(dumpFilename);

	if (dumpFile == -1)
	{
		printf("Failed to create dump file '%s'", dumpFilename);
		return;
	}

	// Dump a generic info header
	dbgDumpHeader(dumpFile);

#ifdef SA_SIGINFO
	writeAll(dumpFile, "Dump caused by signal: ");

	signal = wz_strsignal(siginfo->si_signo, siginfo->si_code);
	writeAll(dumpFile, signal);
	writeAll(dumpFile, "\n\n");
#endif

	dbgDumpLog(dumpFile); // dump out the last several log calls

# if defined(__GLIBC__)
	// Dump raw backtrace in case GDB is not available or fails
	writeAll(dumpFile, "GLIBC raw backtrace:\n");
	backtrace_symbols_fd(btBuffer, btSize, dumpFile);
	writeAll(dumpFile, "\n");
# else
	writeAll(dumpFile, "GLIBC not available, no raw backtrace dumped\n\n");
# endif


	// Make sure everything is written before letting GDB write to it
	fsync(dumpFile);

	// Use 'gdb' to provide an "extended" backtrace
#ifdef SA_SIGINFO
	gdbExtendedBacktrace(dumpFile, (ucontext_t *)sigcontext);
#else
	gdbExtendedBacktrace(dumpFile);
#endif

	printf("Saved dump file to '%s'\n"
	       "If you create a bugreport regarding this crash, please include this file.\n", dumpFilename);
	close(dumpFile);


	sigaction(signum, &oldAction[signum], nullptr);
	raise(signum);
}


#endif // WZ_OS_*

#if defined(WZ_OS_UNIX) && !defined(WZ_OS_MAC)
static bool fetchProgramPath(char *const programPath, size_t const bufSize, const char *const programCommand)
{
	FILE *whichProgramStream;
	size_t bytesRead;
	char *linefeed;
	// Construct the "which $(programCommand)" string
	char whichProgramCommand[PATH_MAX];
	snprintf(whichProgramCommand, sizeof(whichProgramCommand), "which %s", programCommand);

	/* Fill the output buffer with zeroes so that we can rely on the output
	 * string being NUL-terminated.
	 */
	memset(programPath, 0, bufSize);

	/* Execute the "which" command (constructed above) and collect its
	 * output in programPath.
	 */
	whichProgramStream = popen(whichProgramCommand, "r");
	bytesRead = fread(programPath, 1, bufSize, whichProgramStream);
	pclose(whichProgramStream);

	// Check whether our buffer is too small, indicate failure if it is
	if (bytesRead == bufSize)
	{
		debug(LOG_WARNING, "Could not retrieve full path to \"%s\", as our buffer is too small. This may prevent creation of an extended backtrace.", programCommand);
		return false;
	}

	// Cut of the linefeed (and everything following it) if it's present.
	linefeed = strchr(programPath, '\n');
	if (linefeed)
	{
		*linefeed = '\0';
	}

	// Check to see whether we retrieved any meaning ful result
	if (strlen(programPath) == 0)
	{
		debug(LOG_WARNING, "Could not retrieve full path to \"%s\". This may prevent creation of an extended backtrace.", programCommand);
		return false;
	}

	debug(LOG_WZ, "Found program \"%s\" at path \"%s\"", programCommand, programPath);
	return true;
}
#endif

/**
 * Setup the exception handler responsible for target OS.
 *
 * \param programCommand Command used to launch this program. Only used for POSIX handler.
 */
void setupExceptionHandler(int argc, const char **argv, const char *packageVersion)
{
#if defined(WZ_OS_UNIX) && !defined(WZ_OS_MAC)
	const char *programCommand;
	time_t currentTime;
#endif
#if !defined(WZ_OS_MAC)
	// Initialize info required for the debug dumper
	dbgDumpInit(argc, argv, packageVersion);
#endif

#if defined(WZ_OS_WIN)
# if defined(WZ_CC_MINGW)
	ExchndlSetup(packageVersion);
# else
	prevExceptionHandler = SetUnhandledExceptionFilter(windowsExceptionHandler);
# endif // !defined(WZ_CC_MINGW)
#elif defined(WZ_OS_UNIX) && !defined(WZ_OS_MAC)
	programCommand = argv[0];

	// Get full path to this program. Needed for gdb to find the binary.
	programIsAvailable = fetchProgramPath(programPath, sizeof(programPath), programCommand);

	// Get full path to 'gdb'
	gdbIsAvailable = fetchProgramPath(gdbPath, sizeof(gdbPath), "gdb");

	sysInfoValid = (uname(&sysInfo) == 0);

	currentTime = time(nullptr);
	sstrcpy(executionDate, ctime(&currentTime));

	snprintf(programPID, sizeof(programPID), "%i", getpid());

	setFatalSignalHandler(posixExceptionHandler);
#endif // WZ_OS_*
}
bool OverrideRPTDirectory(char *newPath)
{
# if defined(WZ_CC_MINGW)
	wchar_t buf[MAX_PATH];

	if (!MultiByteToWideChar(CP_UTF8, 0, newPath, -1, buf, MAX_PATH))
	{
		//conversion failed-- we won't use the user's directory.

		LPVOID lpMsgBuf;
		DWORD dw = GetLastError();
		TCHAR szBuffer[4196];

		FormatMessage(
		    FORMAT_MESSAGE_ALLOCATE_BUFFER |
		    FORMAT_MESSAGE_FROM_SYSTEM |
		    FORMAT_MESSAGE_IGNORE_INSERTS,
		    NULL,
		    dw,
		    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		    (LPTSTR) &lpMsgBuf,
		    0, NULL);

		wsprintf(szBuffer, _T("Exception handler failed setting new directory with error %d: %s\n"), dw, lpMsgBuf);
		MessageBox((HWND)MB_ICONEXCLAMATION, szBuffer, _T("Error"), MB_OK);

		LocalFree(lpMsgBuf);

		return false;
	}
	PathRemoveFileSpecW(buf);
	wcscat(buf, L"\\logs\\"); // stuff it in the logs directory
	wcscat(buf, L"Warzone2100.RPT");
	ResetRPTDirectory(buf);
#elif defined(WZ_OS_UNIX) && !defined(WZ_OS_MAC)
	sstrcpy(WritePath, newPath);
#endif
	return true;
}
