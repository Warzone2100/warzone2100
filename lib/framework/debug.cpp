/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
/*
 * Debug.c
 *
 * Various debugging output functions.
 *
 */

#include "frame.h"
#include "wztime.h"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include "string_ext.h"
#include "wzapp.h"
#include <map>
#include <string>
#include <regex>

#if defined(WZ_OS_LINUX) && defined(__GLIBC__)
#include <execinfo.h>  // Nonfatal runtime backtraces.
#include <cxxabi.h>
#endif // defined(WZ_OS_LINUX) && defined(__GLIBC__)

#if defined(WZ_OS_UNIX)
# include <fcntl.h>
# ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 1
# endif
# ifndef _XOPEN_SOURCE
#  define _XOPEN_SOURCE
# endif
# ifndef _POSIX_SOURCE
#  define _POSIX_SOURCE
# endif
# include <stdio.h>
#endif

#define MAX_LEN_LOG_LINE 1024

char last_called_script_event[MAX_EVENT_NAME_LEN];
UDWORD traceID = -1;

static debug_callback *callbackRegistry = nullptr;
bool enabled_debug[LOG_LAST]; // global
#ifdef DEBUG
bool assertEnabled = true;
#else
bool assertEnabled = false;
#endif
#ifdef WZ_OS_MAC
#include "cocoa_wrapper.h"
#endif
/*
 * This list _must_ match the enum in debug.h!
 * Names must be 8 chars long at max!
 */
static const char *code_part_names[] =
{
	"all",
	"main",
	"sound",
	"video",
	"wz",
	"3d",
	"texture",
	"net",
	"memory",
	"warning",
	"error",
	"never",
	"script",
	"movement",
	"attack",
	"fog",
	"sensor",
	"gui",
	"map",
	"save",
	"sync",
	"death",
	"life",
	"gateway",
	"message",
	"info",
	"terrain",
	"feature",
	"fatal",
	"input",
	"popup",
	"console",
	"activity",
	"research",
	"savegame",
	"repairs",
	"flowfield",
	"last"
};

static char inputBuffer[2][MAX_LEN_LOG_LINE];
static bool useInputBuffer1 = false;
static bool debug_flush_stderr = false;

static std::map<std::string, int> warning_list;	// only used for LOG_WARNING

/**
 * Convert code_part names to enum. Case insensitive.
 *
 * \return	Codepart number or LOG_LAST if can't match.
 */
static code_part code_part_from_str(const char *str)
{
	unsigned int i;

	for (i = 0; i < LOG_LAST; i++)
	{
		if (strcasecmp(code_part_names[i], str) == 0)
		{
			return (code_part)i;
		}
	}
	return LOG_LAST;
}


/**
 * Callback for outputing to stderr
 *
 * \param	data			Ignored. Use NULL.
 * \param	outputBuffer	Buffer containing the preprocessed text to output.
 */
void debug_callback_stderr(WZ_DECL_UNUSED void **data, const char *outputBuffer, code_part)
{
	if (outputBuffer[strlen(outputBuffer) - 1] != '\n')
	{
		fprintf(stderr, "%s\n", outputBuffer);
	}
	else
	{
		fprintf(stderr, "%s", outputBuffer);
	}

	// Make sure that all output is flushed to stderr when requested by the user
	if (debug_flush_stderr)
	{
		fflush(stderr);
	}
}


/**
 * Callback for outputting to a win32 debugger
 *
 * \param	data			Ignored. Use NULL.
 * \param	outputBuffer	Buffer containing the preprocessed text to output.
 */
#if defined(_WIN32) && defined(DEBUG)
void debug_callback_win32debug(WZ_DECL_UNUSED void **data, const char *outputBuffer, code_part)
{
	char tmpStr[MAX_LEN_LOG_LINE];

	sstrcpy(tmpStr, outputBuffer);
	if (!strchr(tmpStr, '\n'))
	{
		sstrcat(tmpStr, "\n");
	}

	OutputDebugStringA(tmpStr);
}
#endif // WIN32


/**
 * Callback for outputing to a file
 *
 * \param	data			Filehandle to output to.
 * \param	outputBuffer	Buffer containing the preprocessed text to output.
 */
void debug_callback_file(void **data, const char *outputBuffer, code_part)
{
	FILE *logfile = (FILE *)*data;

	if (!strchr(outputBuffer, '\n'))
	{
		fprintf(logfile, "%s\n", outputBuffer);
	}
	else
	{
		fprintf(logfile, "%s", outputBuffer);
	}
}

static WzString replaceUsernameInPath(WzString path)
{
	// Some common forms:
	// C:\Users\<USERNAME>\... (Windows Vista+)
	// C:\Documents and Settings\<USERNAME>\... (Windows XP)
	// /Users/<USERNAME>/... (macOS)
	// /home/<USERNAME>/... (Linux)

	// First pass, do a simple string regex replacement
	const auto win_re = std::regex("^[A-Za-z]:\\\\(Users|Documents and Settings)\\\\[^\\\\]+", std::regex_constants::ECMAScript);
	auto result = std::regex_replace(path.toUtf8(), win_re, "[WIN_USER_FOLDER]");
	const auto unix_re = std::regex("^\\/(Users|home)\\/[^\\/]+", std::regex_constants::ECMAScript);
	result = std::regex_replace(result, unix_re, "[USER_HOME]");

	// POSSIBLE FUTURE TODO: Do OS-specific handling where we query the "home"/user folder?
	return WzString::fromUtf8(result);
}

char WZ_DBGFile[PATH_MAX] = {0};	//Used to save path of the created log file
/**
 * Setup the file callback
 *
 * Sets data to the filehandle opened for the filename found in data.
 *
 * \param[in,out]	data	In: 	The filename to output to.
 * 							Out:	The filehandle.
 */
WzString WZDebugfilename;
bool debug_callback_file_init(void **data)
{
	WzString * pFileName = (WzString *)*data;
	ASSERT(pFileName, "Debug filename required");
	WZDebugfilename = *pFileName;

#if defined(WZ_OS_WIN)
	// On Windows, path strings passed to fopen() are interpreted using the ANSI or OEM codepage
	// (and not as UTF-8). To support Unicode paths, the string must be converted to a wide-char
	// string and passed to _wfopen.
	int wstr_len = MultiByteToWideChar(CP_UTF8, 0, WZDebugfilename.toUtf8().c_str(), -1, NULL, 0);
	if (wstr_len <= 0)
	{
		fprintf(stderr, "Could not not convert string from UTF-8; MultiByteToWideChar failed with error %lu: %s\n", GetLastError(), WZDebugfilename.toUtf8().c_str());
		return false;
	}
	std::vector<wchar_t> wstr_filename(wstr_len, 0);
	if (MultiByteToWideChar(CP_UTF8, 0, WZDebugfilename.toUtf8().c_str(), -1, &wstr_filename[0], wstr_len) == 0)
	{
		fprintf(stderr, "Could not not convert string from UTF-8; MultiByteToWideChar[2] failed with error %lu: %s\n", GetLastError(), WZDebugfilename.toUtf8().c_str());
		return false;
	}
	FILE *const logfile = _wfopen(&wstr_filename[0], L"w");
#else
	FILE *const logfile = fopen(WZDebugfilename.toUtf8().c_str(), "w");
#endif
	if (!logfile)
	{
		fprintf(stderr, "Could not open %s for appending!\n", WZDebugfilename.toUtf8().c_str());
		return false;
	}
#if defined(WZ_OS_UNIX)
	int fd = fileno(logfile);
	if (fd != -1)
	{
		fcntl(fd, F_SETFD, FD_CLOEXEC);
	}
#endif
	snprintf(WZ_DBGFile, sizeof(WZ_DBGFile), "%s", WZDebugfilename.toUtf8().c_str());
	setbuf(logfile, nullptr);
	WzString fileNameStripped = replaceUsernameInPath(WZDebugfilename);
	fprintf(logfile, "--- Starting log [%s]---\n", fileNameStripped.toUtf8().c_str());
	*data = logfile;

	return true;
}


/**
 * Shutdown the file callback
 *
 * Closes the logfile.
 *
 * \param	data	The filehandle to close.
 */
void debug_callback_file_exit(void **data)
{
	FILE *logfile = (FILE *)*data;
	fclose(logfile);
	*data = nullptr;
}

void debugFlushStderr()
{
	debug_flush_stderr = true;
}
// MSVC specific rotuines to set/clear allocation tracking
#if defined(_MSC_VER) && defined(DEBUG)
void debug_MEMCHKOFF()
{
	// Disable allocation tracking
	int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	flags &= ~_CRTDBG_ALLOC_MEM_DF;
	_CrtSetDbgFlag(flags);
}
void debug_MEMCHKON()
{
	// Enable allocation tracking
	int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	flags |= _CRTDBG_ALLOC_MEM_DF;
	_CrtSetDbgFlag(flags);
}
void debug_MEMSTATS()
{
	_CrtMemState state;
	_CrtMemCheckpoint(&state);
	_CrtMemDumpStatistics(&state);
}
#endif

void debug_init()
{
	/*** Initialize the debug subsystem ***/
#if defined(WZ_CC_MSVC) && defined(DEBUG)
	int tmpDbgFlag;
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);   // Output CRT info to debugger

	tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);   // Grab current flags
# if defined(DEBUG_MEMORY)
	tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF; // Check every (de)allocation
# endif // DEBUG_MEMORY
	tmpDbgFlag |= _CRTDBG_ALLOC_MEM_DF; // Check allocations
	tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF; // Check for memleaks
	_CrtSetDbgFlag(tmpDbgFlag);
#endif // WZ_CC_MSVC && DEBUG

	STATIC_ASSERT(ARRAY_SIZE(code_part_names) - 1 == LOG_LAST); // enums start at 0

	memset(enabled_debug, false, sizeof(enabled_debug));
	enabled_debug[LOG_ERROR] = true;
	enabled_debug[LOG_INFO] = true;
	enabled_debug[LOG_FATAL] = true;
	enabled_debug[LOG_POPUP] = true;
	inputBuffer[0][0] = '\0';
	inputBuffer[1][0] = '\0';
#ifdef DEBUG
	enabled_debug[LOG_WARNING] = true;
#endif
}


void debug_exit()
{
	debug_callback *curCallback = callbackRegistry, * tmpCallback = nullptr;

	while (curCallback)
	{
		if (curCallback->exit)
		{
			curCallback->exit(&curCallback->data);
		}
		tmpCallback = curCallback->next;
		free(curCallback);
		curCallback = tmpCallback;
	}
	warning_list.clear();
	callbackRegistry = nullptr;
}


void debug_register_callback(debug_callback_fn callback, debug_callback_init init, debug_callback_exit exit, void *data)
{
	debug_callback *curCallback = callbackRegistry, * tmpCallback = nullptr;

	tmpCallback = (debug_callback *)malloc(sizeof(*tmpCallback));

	tmpCallback->next = nullptr;
	tmpCallback->callback = callback;
	tmpCallback->init = init;
	tmpCallback->exit = exit;
	tmpCallback->data = data;

	if (tmpCallback->init
	    && !tmpCallback->init(&tmpCallback->data))
	{
		fprintf(stderr, "Failed to initialise debug callback, debugfile set to [%s]!\n", WZDebugfilename.toUtf8().c_str());
		free(tmpCallback);
		return;
	}

	if (!curCallback)
	{
		callbackRegistry = tmpCallback;
		return;
	}

	while (curCallback->next)
	{
		curCallback = curCallback->next;
	}

	curCallback->next = tmpCallback;
}


bool debug_enable_switch(const char *str)
{
	code_part part = code_part_from_str(str);

	if (part != LOG_LAST)
	{
		enabled_debug[part] = !enabled_debug[part];
	}
	if (part == LOG_ALL)
	{
		memset(enabled_debug, true, sizeof(enabled_debug));
	}
	return (part != LOG_LAST);
}

/** Send the given string to all debug callbacks.
 *
 *  @param str The string to send to debug callbacks.
 */
static void printToDebugCallbacks(const char *const str, code_part part)
{
	debug_callback *curCallback;

	// Loop over all callbacks, invoking them with the given data string
	for (curCallback = callbackRegistry; curCallback != nullptr; curCallback = curCallback->next)
	{
		curCallback->callback(&curCallback->data, str, part);
	}
}

void _realObjTrace(int id, const char *function, const char *str, ...)
{
	char vaBuffer[MAX_LEN_LOG_LINE];
	char outputBuffer[MAX_LEN_LOG_LINE];
	va_list ap;

	va_start(ap, str);
	vssprintf(vaBuffer, str, ap);
	va_end(ap);

	ssprintf(outputBuffer, "[%6d]: [%s] %s", id, function, vaBuffer);
	printToDebugCallbacks(outputBuffer, LOG_INFO);
}

// Thread local to prevent a race condition on read and write to this buffer if multiple
// threads log errors. This means we will not be reporting any errors to console from threads
// other than main. If we want to fix this, make sure accesses are protected by a mutex.
static WZ_DECL_THREAD char errorStore[512];
static WZ_DECL_THREAD bool errorWaiting = false;
const char *debugLastError()
{
	if (errorWaiting)
	{
		errorWaiting = false;
		return errorStore;
	}
	else
	{
		return nullptr;
	}
}

void _debug(int line, code_part part, const char *function, const char *str, ...)
{
	va_list ap;
	static char outputBuffer[MAX_LEN_LOG_LINE];
	static unsigned int repeated = 0; /* times current message repeated */
	static unsigned int next = 2;     /* next total to print update */
	static unsigned int prev = 0;     /* total on last update */

	va_start(ap, str);
	vssprintf(outputBuffer, str, ap);
	va_end(ap);

	if (part == LOG_WARNING)
	{
		std::pair<std::map<std::string, int>::iterator, bool> ret;
		ret = warning_list.insert(std::pair<std::string, int>(std::string(function) + "-" + std::string(outputBuffer), line));
		if (ret.second)
		{
			ssprintf(inputBuffer[useInputBuffer1 ? 1 : 0], "[%s:%d] %s (**Further warnings of this type are suppressed.)", function, line, outputBuffer);
		}
		else
		{
			return;	// don't bother adding any more
		}
	}
	else
	{
		ssprintf(inputBuffer[useInputBuffer1 ? 1 : 0], "[%s:%d] %s", function, line, outputBuffer);
	}

	if (sstrcmp(inputBuffer[0], inputBuffer[1]) == 0)
	{
		// Received again the same line
		repeated++;
		if (repeated == next)
		{
			if (repeated > 2)
			{
				ssprintf(outputBuffer, "last message repeated %u times (total %u repeats)", repeated - prev, repeated);
			}
			else
			{
				ssprintf(outputBuffer, "last message repeated %u times", repeated - prev);
			}
			printToDebugCallbacks(outputBuffer, part);
			prev = repeated;
			next *= 2;
		}
	}
	else
	{
		// Received another line, cleanup the old
		if (repeated > 0 && repeated != prev && repeated != 1)
		{
			/* just repeat the previous message when only one repeat occurred */
			if (repeated > 2)
			{
				ssprintf(outputBuffer, "last message repeated %u times (total %u repeats)", repeated - prev, repeated);
			}
			else
			{
				ssprintf(outputBuffer, "last message repeated %u times", repeated - prev);
			}
			printToDebugCallbacks(outputBuffer, part);
		}
		repeated = 0;
		next = 2;
		prev = 0;
	}

	if (!repeated)
	{
		time_t rawtime;
		struct tm timeinfo = {};
		char ourtime[15];		//HH:MM:SS

		time(&rawtime);
		timeinfo = getLocalTime(rawtime);
		strftime(ourtime, 15, "%H:%M:%S", &timeinfo);

		// Assemble the outputBuffer:
		ssprintf(outputBuffer, "%-8s|%s: %s", code_part_names[part], ourtime, useInputBuffer1 ? inputBuffer[1] : inputBuffer[0]);

		printToDebugCallbacks(outputBuffer, part);

		if (part == LOG_ERROR)
		{
			// used to signal user that there was a error condition, and to check the logs.
			sstrcpy(errorStore, useInputBuffer1 ? inputBuffer[1] : inputBuffer[0]);
			errorWaiting = true;
		}

		// Throw up a dialog box for users since most don't have a clue to check the dump file for information. Use for (duh) Fatal errors, that force us to terminate the game.
		if (part == LOG_FATAL)
		{
			if (wzIsFullscreen())
			{
				wzChangeWindowMode(WINDOW_MODE::windowed);
			}
#if defined(WZ_OS_WIN)
			char wbuf[MAX_LEN_LOG_LINE+512];
			ssprintf(wbuf, "%s\n\nPlease check the file (%s) in your configuration directory for more details. \
				\nDo not forget to upload the %s file, WZdebuginfo.txt and the warzone2100.rpt files in your bug reports at https://github.com/Warzone2100/warzone2100/issues/new!", useInputBuffer1 ? inputBuffer[1] : inputBuffer[0], WZ_DBGFile, WZ_DBGFile);
			wzDisplayDialog(Dialog_Error, "Warzone has terminated unexpectedly", wbuf);
#elif defined(WZ_OS_MAC)
			char wbuf[MAX_LEN_LOG_LINE+128];
			ssprintf(wbuf, "%s\n\nPlease check your logs and attach them along with a bug report. Thanks!", useInputBuffer1 ? inputBuffer[1] : inputBuffer[0]);
			int clickedIndex = \
			                   cocoaShowAlert("Warzone has quit unexpectedly.",
			                                  wbuf,
			                                  2, "Show Log Files & Open Bug Reporter", "Ignore", NULL);
			if (clickedIndex == 0)
			{
				if (!cocoaOpenURL("https://github.com/Warzone2100/warzone2100/issues/new"))
                {
                    cocoaShowAlert("Failed to open URL",
                                   "Could not open URL: https://github.com/Warzone2100/warzone2100/issues/new\nPlease open this URL manually in your web browser.",
                                   2, "Continue", NULL);
                }
                if (strnlen(WZ_DBGFile, sizeof(WZ_DBGFile)/sizeof(WZ_DBGFile[0])) <= 0)
				{
					cocoaShowAlert("Unable to open debug log.",
					               "The debug log subsystem has not yet been initialised.",
					               2, "Continue", NULL);
				}
				else
				{
                    if (!cocoaSelectFileInFinder(WZ_DBGFile))
                    {
                        cocoaShowAlert("Cannot Display Log File",
                                       "The attempt to open a Finder window highlighting the log file from this run failed.",
                                       2, "Continue", NULL);
                    }
				}
				cocoaOpenUserCrashReportFolder();
			}
#else
			const char *popupBuf = useInputBuffer1 ? inputBuffer[1] : inputBuffer[0];
			wzDisplayDialog(Dialog_Error, "Warzone has terminated unexpectedly", popupBuf);
#endif
		}

		// Throw up a dialog box for windows users since most don't have a clue to check the stderr.txt file for information
		// This is a popup dialog used for times when the error isn't fatal, but we still need to notify user what is going on.
		if (part == LOG_POPUP)
		{
			wzDisplayDialog(Dialog_Information, "Warzone has detected a problem.", inputBuffer[useInputBuffer1 ? 1 : 0]);
		}

	}
	useInputBuffer1 = !useInputBuffer1; // Swap buffers
}

void _debugBacktrace(code_part part)
{
#if defined(WZ_OS_LINUX) && defined(__GLIBC__)
	void *btv[20];
	unsigned num = backtrace(btv, sizeof(btv) / sizeof(*btv));
	char **btc = backtrace_symbols(btv, num);
	unsigned i;
	auto trim = [](const char *in, char *out) {
		const char *begin = strchr(in, '_');
		if (begin)
		{
			const char *end = strchr(begin, '+');
			if (end)
			{
				memcpy(out, begin, end - begin);
				out[end - begin] = '\0';
			}
		}
	};
	for (i = 1; i + 2 < num; ++i) 
	{
		int status = -1;
		char buf[1024];
		const char *p = btc[i];
		trim(p, buf);
		char *readableName = abi::__cxa_demangle(buf, NULL, NULL, &status);
		if (status == 0)
		{
			_debug(0, part, "BT", "%s", readableName);
		}
		else
		{
			_debug(0, part, "BT", "%s [status: %i]", btc[i], status);
		}
	}
	free(btc);
#else
	// debugBacktrace not implemented.
#endif
}

bool debugPartEnabled(code_part codePart)
{
	return enabled_debug[codePart];
}

void debugDisableAssert()
{
	assertEnabled = false;
}

#include <sstream>

void _debug_multiline(int line, code_part part, const char *function, const std::string &multilineString)
{
	std::stringstream ss(multilineString);
	std::string to;

	while(std::getline(ss,to,'\n')){
		_debug(line, part, function, "%s", to.c_str());
	}
}

