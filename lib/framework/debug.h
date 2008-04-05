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
/*! \file debug.h
 *  \brief Debugging functions
 */

#ifndef __INCLUDED_LIB_FRAMEWORK_DEBUG_H__
#define __INCLUDED_LIB_FRAMEWORK_DEBUG_H__

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */
#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
# error Framework header files MUST be included from Frame.h ONLY.
#endif

#include <assert.h>

#if defined(__cplusplus)
extern "C"
{
#endif

/****************************************************************************************
 *
 * Basic debugging macro's
 *
 */

#define MAX_EVENT_NAME_LEN	100

/** Stores name of the last function or event called by scripts. */
extern char last_called_script_event[MAX_EVENT_NAME_LEN];

/**
 *
 * Rewritten version of assert that allows a printf format text string to be passed
 * to ASSERT along with the condition.
 *
 * Arguments:	ASSERT( condition, "Format string with variables: %d, %d", var1, var2 );
 */
#define ASSERT( expr, ... ) \
	( (expr) ? (void)0 : (void)_debug( LOG_ERROR, __VA_ARGS__ ) ); \
	( (expr) ? (void)0 : (void)_debug( LOG_ERROR, "Assert in Warzone: %s:%d : %s (%s), last script event: '%s'", \
		__FILE__, __LINE__, __FUNCTION__, (#expr), last_called_script_event ) ); \
	assert( expr );


/**
 * Compile time assert
 * Not to be used in global context!
 * \param expr Expression to evaluate
 */
#define STATIC_ASSERT( expr ) \
	do { enum { assert_static__ = 1/(expr) }; } while(0)


/***
 ***
 ***  New debug logging output interface below. Heavily inspired
 ***  by similar code in Freeciv. Parts ripped directly.
 ***
 ***/

/** Debug enums. Must match code_part_names in debug.c */
typedef enum {
  LOG_ALL, /* special: sets all to on */
  LOG_MAIN,
  LOG_SOUND,
  LOG_VIDEO,
  LOG_WZ,
  LOG_3D,
  LOG_TEXTURE,
  LOG_NET,
  LOG_MEMORY,
  LOG_WARNING, /**< special; on in debug mode */
  LOG_ERROR, /**< special; on by default */
  LOG_NEVER, /**< if too verbose for anything but dedicated debugging... */
  LOG_SCRIPT,
  LOG_MOVEMENT,
  LOG_ATTACK,
  LOG_FOG,
  LOG_SENSOR,
  LOG_GUI,
  LOG_MAP,
  LOG_SAVEGAME,
  LOG_MULTISYNC,
  LOG_DEATH,
  LOG_GATEWAY,
  LOG_LAST /**< _must_ be last! */
} code_part;

extern BOOL enabled_debug[LOG_LAST];

typedef void (*debug_callback_fn)(void**, const char*);
typedef void (*debug_callback_init)(void**);
typedef void (*debug_callback_exit)(void**);

typedef struct _debug_callback {
	struct _debug_callback * next;
	debug_callback_fn callback; /// Function which does the output
	debug_callback_init init; /// Setup function
	debug_callback_exit exit; /// Cleaning function
	void * data; /// Used to pass data to the above functions. Eg a filename or handle.
} debug_callback;

/**
 * Call once to initialize the debug logging system.
 *
 * Doesn't register any callbacks!
 */
void debug_init( void );

/**
 * Shutdown the debug system and remove all output callbacks
 */
void debug_exit( void );

/**
 * Register a callback to be called on every call to debug()
 *
 * \param	callback	Function which does the output
 * \param	init		Initializer function which does all setup for the callback (optional, may be NULL)
 * \param	exit		Cleanup function called when unregistering the callback (optional, may be NULL)
 * \param	data		Data to be passed to all three functions (optional, may be NULL)
 */
void debug_register_callback( debug_callback_fn callback, debug_callback_init init, debug_callback_exit exit, void * data );

void debug_callback_file(void **data, const char *outputBuffer);
void debug_callback_file_init(void **data);
void debug_callback_file_exit(void **data);

void debug_callback_stderr(void **data, const char *outputBuffer);

/**
 * Toggle debug output for part associated with str
 *
 * \param	str	Codepart in textformat
 */
BOOL debug_enable_switch(const char *str);

/**
 * Output printf style format str with additional arguments.
 *
 * Only outputs if debugging of part was formerly enabled with debug_enable_switch.
 *
 * \param	part	Code part to associate with this message
 * \param	str		printf style formatstring
 */
#define debug(part, ...) do { if (enabled_debug[part]) _debug(part, __VA_ARGS__); } while(0)
void _debug( code_part part, const char *str, ...)
		WZ_DECL_FORMAT(printf, 2, 3);

/** Global to keep track of which game object to trace. */
extern UDWORD traceID;

/**
 * Output printf style format str for debugging a specific game object whose debug part
 * has been enabled.
 * @see debug
 */
#define objTrace(part, id, ...) do { if (enabled_debug[part] && id == traceID) _debug(part, __VA_ARGS__); } while(0)
static inline void objTraceEnable(UDWORD id) { traceID = id; }
static inline void objTraceDisable(void) { traceID = 0; }

#if defined(__cplusplus)
}
#endif

/** Dump last two debug log calls into given file descriptor. For exception handler. */
#if defined(WZ_OS_WIN)
extern void dumpLog(HANDLE file);
#else
extern void dumpLog(int file);
#endif

/** Checks if a particular debub flag was enabled */
extern bool debugPartEnabled(code_part codePart);

#endif // __INCLUDED_LIB_FRAMEWORK_DEBUG_H__
