/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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

#include <string.h>
#include <stdio.h>
#include <time.h>
#include "string_ext.h"
#include "wzapp.h"
#include "lib/gamelib/gtime.h"

#ifdef WZ_OS_LINUX
#include <execinfo.h>  // Nonfatal runtime backtraces.
#endif //WZ_OS_LINUX

extern void NotifyUserOfError(char *);		// will throw up a notifier on error

#define MAX_LEN_LOG_LINE 512

char last_called_script_event[MAX_EVENT_NAME_LEN];
UDWORD traceID = -1;

static debug_callback * callbackRegistry = NULL;
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
static const char *code_part_names[] = {
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
	"last"
};

static char inputBuffer[2][MAX_LEN_LOG_LINE];
static bool useInputBuffer1 = false;
static bool debug_flush_stderr = false;

/**
 * Convert code_part names to enum. Case insensitive.
 *
 * \return	Codepart number or LOG_LAST if can't match.
 */
static code_part code_part_from_str(const char *str)
{
	unsigned int i;

	for (i = 0; i < LOG_LAST; i++) {
		if (strcasecmp(code_part_names[i], str) == 0) {
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
void debug_callback_stderr( WZ_DECL_UNUSED void ** data, const char * outputBuffer )
{
	if ( outputBuffer[strlen(outputBuffer) - 1] != '\n' ) {
		fprintf( stderr, "%s\n", outputBuffer );
	} else {
		fprintf( stderr, "%s", outputBuffer );
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
#if defined WIN32 && defined DEBUG
void debug_callback_win32debug(WZ_DECL_UNUSED void ** data, const char * outputBuffer)
{
	char tmpStr[MAX_LEN_LOG_LINE];

	sstrcpy(tmpStr, outputBuffer);
	if (!strchr(tmpStr, '\n'))
	{
		sstrcat(tmpStr, "\n");
	}

	OutputDebugStringA( tmpStr );
}
#endif // WIN32


/**
 * Callback for outputing to a file
 *
 * \param	data			Filehandle to output to.
 * \param	outputBuffer	Buffer containing the preprocessed text to output.
 */
void debug_callback_file( void ** data, const char * outputBuffer )
{
	FILE * logfile = (FILE*)*data;

	if ( !strchr( outputBuffer, '\n' ) ) {
		fprintf( logfile, "%s\n", outputBuffer );
	} else {
		fprintf( logfile, "%s", outputBuffer );
	}
}


/**
 * Setup the file callback
 *
 * Sets data to the filehandle opened for the filename found in data.
 *
 * \param[in,out]	data	In: 	The filename to output to.
 * 							Out:	The filehandle.
 */
bool debug_callback_file_init(void ** data)
{
	const char * filename = (const char *)*data;

	FILE* const logfile = fopen(filename, "w");
	if (!logfile)
	{
		fprintf(stderr, "Could not open %s for appending!\n", filename);
		return false;
	}

	setbuf(logfile, NULL);
	fprintf(logfile, "\n--- Starting log ---\n");
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
void debug_callback_file_exit( void ** data )
{
	FILE * logfile = (FILE*)*data;
	fclose( logfile );
	*data = NULL;
}

void debugFlushStderr()
{
	debug_flush_stderr = true;
}
// MSVC specific rotuines to set/clear allocation tracking
#if defined(WZ_CC_MSVC) && defined(DEBUG)
void debug_MEMCHKOFF(void)
{
	// Disable allocation tracking
	int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	flags &= ~_CRTDBG_ALLOC_MEM_DF;
	_CrtSetDbgFlag(flags);
}
void debug_MEMCHKON(void)
{
	// Enable allocation tracking
	int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	flags |= _CRTDBG_ALLOC_MEM_DF;
	_CrtSetDbgFlag(flags);
}
void debug_MEMSTATS(void)
{
	_CrtMemState state;
	_CrtMemCheckpoint(&state);
	_CrtMemDumpStatistics(&state);
}
#endif

void debug_init(void)
{
	/*** Initialize the debug subsystem ***/
#if defined(WZ_CC_MSVC) && defined(DEBUG)
	int tmpDbgFlag;
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG ); // Output CRT info to debugger

	tmpDbgFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ); // Grab current flags
# if defined(DEBUG_MEMORY)
	tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF; // Check every (de)allocation
# endif // DEBUG_MEMORY
	tmpDbgFlag |= _CRTDBG_ALLOC_MEM_DF; // Check allocations
	tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF; // Check for memleaks
	_CrtSetDbgFlag( tmpDbgFlag );
#endif // WZ_CC_MSVC && DEBUG

	STATIC_ASSERT(ARRAY_SIZE(code_part_names) - 1 == LOG_LAST); // enums start at 0

	memset( enabled_debug, false, sizeof(enabled_debug) );
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


void debug_exit(void)
{
	debug_callback * curCallback = callbackRegistry, * tmpCallback = NULL;

	while ( curCallback )
	{
		if ( curCallback->exit )
			curCallback->exit( &curCallback->data );
		tmpCallback = curCallback->next;
		free( curCallback );
		curCallback = tmpCallback;
	}

	callbackRegistry = NULL;
}


void debug_register_callback( debug_callback_fn callback, debug_callback_init init, debug_callback_exit exit, void * data )
{
	debug_callback * curCallback = callbackRegistry, * tmpCallback = NULL;

	tmpCallback = (debug_callback*)malloc(sizeof(*tmpCallback));

	tmpCallback->next = NULL;
	tmpCallback->callback = callback;
	tmpCallback->init = init;
	tmpCallback->exit = exit;
	tmpCallback->data = data;

	if (tmpCallback->init
	 && !tmpCallback->init(&tmpCallback->data))
	{
		debug(LOG_ERROR, "Failed to initialise debug callback");
		free(tmpCallback);
		return;
	}

	if ( !curCallback )
	{
		callbackRegistry = tmpCallback;
		return;
	}

	while ( curCallback->next )
		curCallback = curCallback->next;

	curCallback->next = tmpCallback;
}


bool debug_enable_switch(const char *str)
{
	code_part part = code_part_from_str(str);

	if (part != LOG_LAST) {
		enabled_debug[part] = !enabled_debug[part];
	}
	if (part == LOG_ALL) {
		memset(enabled_debug, true, sizeof(enabled_debug));
	}
	return (part != LOG_LAST);
}

/** Send the given string to all debug callbacks.
 *
 *  @param str The string to send to debug callbacks.
 */
static void printToDebugCallbacks(const char * const str)
{
	debug_callback * curCallback;

	// Loop over all callbacks, invoking them with the given data string
	for (curCallback = callbackRegistry; curCallback != NULL; curCallback = curCallback->next)
	{
		curCallback->callback(&curCallback->data, str);
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
	printToDebugCallbacks(outputBuffer);
}

void _debug( code_part part, const char *function, const char *str, ... )
{
	va_list ap;
	static char outputBuffer[MAX_LEN_LOG_LINE];
	static unsigned int repeated = 0; /* times current message repeated */
	static unsigned int next = 2;     /* next total to print update */
	static unsigned int prev = 0;     /* total on last update */

	va_start(ap, str);
	vssprintf(outputBuffer, str, ap);
	va_end(ap);

	ssprintf(inputBuffer[useInputBuffer1 ? 1 : 0], "[%s] %s", function, outputBuffer);

	if (sstrcmp(inputBuffer[0], inputBuffer[1]) == 0)
	{
		// Received again the same line
		repeated++;
		if (repeated == next) {
			if (repeated > 2) {
				ssprintf(outputBuffer, "last message repeated %u times (total %u repeats)", repeated - prev, repeated);
			} else {
				ssprintf(outputBuffer, "last message repeated %u times", repeated - prev);
			}
			printToDebugCallbacks(outputBuffer);
			prev = repeated;
			next *= 2;
		}
	} else {
		// Received another line, cleanup the old
		if (repeated > 0 && repeated != prev && repeated != 1) {
			/* just repeat the previous message when only one repeat occurred */
			if (repeated > 2) {
				ssprintf(outputBuffer, "last message repeated %u times (total %u repeats)", repeated - prev, repeated);
			} else {
				ssprintf(outputBuffer, "last message repeated %u times", repeated - prev);
			}
			printToDebugCallbacks(outputBuffer);
		}
		repeated = 0;
		next = 2;
		prev = 0;
	}

	if (!repeated)
	{
		time_t rawtime;
		struct tm * timeinfo;
		char ourtime[15];		//HH:MM:SS

		time ( &rawtime );
		timeinfo = localtime ( &rawtime );
		strftime (ourtime,15,"%I:%M:%S",timeinfo);

		// Assemble the outputBuffer:
		ssprintf(outputBuffer, "%-8s|%s: %s", code_part_names[part], ourtime, useInputBuffer1 ? inputBuffer[1] : inputBuffer[0]);

		printToDebugCallbacks(outputBuffer);

		if (part == LOG_ERROR)
		{
			// used to signal user that there was a error condition, and to check the logs.
			NotifyUserOfError(useInputBuffer1 ? inputBuffer[1] : inputBuffer[0]);
		}

		// Throw up a dialog box for users since most don't have a clue to check the dump file for information. Use for (duh) Fatal errors, that force us to terminate the game.
		if (part == LOG_FATAL)
		{
#if defined(WZ_OS_WIN)
			char wbuf[512];
			ssprintf(wbuf, "%s\n\nPlease check your stderr.txt file in the same directory as the program file for more details. \
				\nDo not forget to upload both the stderr.txt file and the warzone2100.rpt file in your bug reports!", useInputBuffer1 ? inputBuffer[1] : inputBuffer[0]);
			MessageBoxA( NULL,
				wbuf,
				"Warzone has terminated unexpectedly", MB_OK|MB_ICONERROR);
#elif defined(WZ_OS_MAC)
			cocoaShowAlert("Warzone has terminated unexpectedly.",
			               "Please check your logs for more details."
			               "\n\nRun Console.app, search for \"wz2100\", and copy that to a file."
			               "\n\nIf you are on 10.4 (Tiger) or 10.5 (Leopard) the crash report"
			               " is in ~/Library/Logs/CrashReporter."
			               " If you are on 10.6 (Snow Leopard), it is in"
			               "\n~/Library/Logs/DiagnosticReports."
			               "\n\nDo not forget to upload and attach those to a bug report at http://developer.wz2100.net/newticket"
			               "\nThanks!", 2);
#else
			const char* popupBuf = useInputBuffer1 ? inputBuffer[1] : inputBuffer[0];
			wzFatalDialog(popupBuf);
#endif
		}

		// Throw up a dialog box for windows users since most don't have a clue to check the stderr.txt file for information
		// This is a popup dialog used for times when the error isn't fatal, but we still need to notify user what is going on.
		if (part == LOG_POPUP)
		{
#if defined(WZ_OS_WIN)
			char wbuf[512];
			ssprintf(wbuf, "A non fatal error has occurred.\n\n%s\n\n", useInputBuffer1 ? inputBuffer[1] : inputBuffer[0]);
			MessageBoxA( NULL,
				wbuf,
				"Warzone has detected a problem.", MB_OK|MB_ICONINFORMATION);
#elif defined(WZ_OS_MAC)
			cocoaShowAlert("Warzone has detected a problem.", inputBuffer[useInputBuffer1 ? 1 : 0], 0);
#endif
		}

	}
	useInputBuffer1 = !useInputBuffer1; // Swap buffers
}

void _debugBacktrace(code_part part)
{
#ifdef WZ_OS_LINUX
	void *btv[20];
	unsigned num = backtrace(btv, sizeof(btv)/sizeof(*btv));
	char **btc = backtrace_symbols(btv, num);
	unsigned i;
	for (i = 1; i + 2 < num; ++i)  // =1: Don't print "src/warzone2100(syncDebugBacktrace+0x16) [0x6312d1]". +2: Don't print last two lines of backtrace such as "/lib/libc.so.6(__libc_start_main+0xe6) [0x7f91e040ea26]", since the address varies (even with the same binary).
	{
		_debug(part, "BT", "%s", btc[i]);
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
