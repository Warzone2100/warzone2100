/*
 * Debug.c
 *
 * Various debugging output functions.
 *
 */

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <stdio.h>
#endif

#include "frame.h"
#include "frameint.h"

#define MAX_FILENAME_SIZE 200
#define MAX_LEN_LOG_LINE 512
#define MAX_LEN_DEBUG_PART 12

static debug_callback * callbackRegistry = NULL;
static BOOL enabled_debug_parts[LOG_LAST];

/* This list _must_ match the enum in debug.h! */
static const char *code_part_names[] = {
  "all", "main", "sound", "video", "wz", "3d", "texture",
  "net", "memory", "error", "never", "script", "last"
};

/**********************************************************************
 cat_snprintf is like a combination of snprintf and strlcat;
 it does snprintf to the end of an existing string.

 Like mystrlcat, n is the total length available for str, including
 existing contents and trailing nul.  If there is no extra room
 available in str, does not change the string.

 Also like mystrlcat, returns the final length that str would have
 had without truncation.  I.e., if return is >= n, truncation occurred.
**********************************************************************/
static int cat_snprintf(char *str, size_t n, const char *format, ...)
{
	size_t len;
	int ret;
	va_list ap;

	assert(format != NULL);
	assert(str != NULL);
	assert(n > 0);

	len = strlen(str);
	assert(len < n);

	va_start(ap, format);
	ret = vsnprintf(str + len, n - len, format, ap);
	va_end(ap);
	return (int) (ret + len);
}

/**
 * Convert code_part names to enum. Case insensitive.
 *
 * \return	Codepart number or LOG_LAST if can't match.
 */
static int code_part_from_str(const char *str)
{
	int i;

	for (i = 0; i < LOG_LAST; i++) {
		if (strcasecmp(code_part_names[i], str) == 0) {
			return i;
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
void debug_callback_stderr( void ** data, const char * outputBuffer )
{
	if ( !strchr( outputBuffer, '\n' ) ) {
		fprintf( stderr, "%s\n", outputBuffer );
	} else {
		fprintf( stderr, "%s", outputBuffer );
	}
}


/**
 * Callback for outputting to a win32 debugger
 *
 * \param	data			Ignored. Use NULL.
 * \param	outputBuffer	Buffer containing the preprocessed text to output.
 */
void debug_callback_win32debug( void ** data, const char * outputBuffer )
{
#if defined WIN32 && defined DEBUG
	char tmpStr[MAX_LEN_LOG_LINE];

	strcpy( tmpStr, outputBuffer );
	if ( !strchr( tmpStr, '\n' ) ) {
		strcat( tmpStr, "\n" );
	}
	OutputDebugStringA( tmpStr );
#endif // WIN32
}


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
void debug_callback_file_init( void ** data )
{
	const char * filename = (const char *)*data;
	FILE * logfile = NULL;

	assert( strlen( filename ) < MAX_FILENAME_SIZE );

	logfile = fopen( filename, "a" );
	if (!logfile) {
		fprintf( stderr, "Could not open %s for appending!\n", filename );
	} else {
		fprintf( logfile, "\n--- Starting log ---\n" );
		*data = logfile;
	}
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


void debug_init(void)
{
	int count = 0;

	while (strcmp(code_part_names[count], "last") != 0) {
		count++;
	}
	if (count != LOG_LAST) {
		fprintf( stderr, "LOG_LAST != last; whoever edited the debug code last "
		        "did a mistake.\n" );
		fprintf( stderr, "Always edit both the enum in debug.h and the string "
		        "list in debug.c!\n" );
		exit(1);
	}
	memset( enabled_debug_parts, FALSE, sizeof(enabled_debug_parts) );
	enabled_debug_parts[LOG_ERROR] = TRUE;
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

	tmpCallback = (debug_callback*)malloc(sizeof(debug_callback));

	tmpCallback->next = NULL;
	tmpCallback->callback = callback;
	tmpCallback->init = init;
	tmpCallback->exit = exit;
	tmpCallback->data = data;

	if ( tmpCallback->init )
		tmpCallback->init( &tmpCallback->data );

	if ( !curCallback )
	{
		callbackRegistry = tmpCallback;
		return;
	}

	while ( curCallback->next )
		curCallback = curCallback->next;

	curCallback->next = tmpCallback;
}


BOOL debug_enable_switch(const char *str)
{
	int part = code_part_from_str(str);

	if (part != LOG_LAST) {
		enabled_debug_parts[part] = !enabled_debug_parts[part];
	}
	if (part == LOG_ALL) {
		memset(enabled_debug_parts, TRUE, sizeof(enabled_debug_parts));
	}
	return (part != LOG_LAST);
}


void debug( code_part part, const char *str, ... )
{
	va_list ap;
	static char inputBuffer[2][MAX_LEN_LOG_LINE];
	static char outputBuffer[MAX_LEN_LOG_LINE+MAX_LEN_DEBUG_PART];
	static BOOL useInputBuffer1 = FALSE;

	debug_callback * curCallback = callbackRegistry;

	static unsigned int repeated = 0; /* times current message repeated */
	static unsigned int next = 2;     /* next total to print update */
	static unsigned int prev = 0;     /* total on last update */

	/* Not enabled debugging for this part? Punt! */
	if (!enabled_debug_parts[part]) {
		return;
	}

	va_start(ap, str);
	vsnprintf( useInputBuffer1 ? inputBuffer[1] : inputBuffer[0], MAX_LEN_LOG_LINE, str, ap );
	va_end(ap);

	if ( strncmp( inputBuffer[0], inputBuffer[1], MAX_LEN_LOG_LINE - 1 ) == 0 ) {
		// Recieved again the same line
		repeated++;
		if (repeated == next) {
			snprintf( outputBuffer, sizeof(outputBuffer), "last message repeated %d times", repeated - prev );
			if (repeated > 2) {
				cat_snprintf( outputBuffer, sizeof(outputBuffer), " (total %d repeats)", repeated );
			}
			while (curCallback)
			{
				curCallback->callback( &curCallback->data, outputBuffer );
				curCallback = curCallback->next;
			}
			curCallback = callbackRegistry;
			prev = repeated;
			next *= 2;
		}
	} else {
		// Recieved another line, cleanup the old
		if (repeated > 0 && repeated != prev && repeated != 1) {
			/* just repeat the previous message when only one repeat occured */
			snprintf( outputBuffer, sizeof(outputBuffer), "last message repeated %d times", repeated - prev );
			if (repeated > 2) {
				cat_snprintf( outputBuffer, sizeof(outputBuffer), " (total %d repeats)", repeated );
			}
			while (curCallback)
			{
				curCallback->callback( &curCallback->data, outputBuffer );
				curCallback = curCallback->next;
			}
			curCallback = callbackRegistry;
		}
		repeated = 0;
		next = 2;
		prev = 0;
	}

	if (!repeated)
	{
		// Assemble the outputBuffer:
		sprintf( outputBuffer, "%s:", code_part_names[part] );
		memset( outputBuffer + strlen( code_part_names[part] ) + 1, ' ', MAX_LEN_DEBUG_PART - strlen( code_part_names[part] ) - 1 ); // Fill with whitespaces
		snprintf( outputBuffer + MAX_LEN_DEBUG_PART, MAX_LEN_LOG_LINE, useInputBuffer1 ? inputBuffer[1] : inputBuffer[0] ); // Append the message

		while (curCallback)
		{
			curCallback->callback( &curCallback->data, outputBuffer );
			curCallback = curCallback->next;
		}
	}
	useInputBuffer1 = !useInputBuffer1; // Swap buffers
}
