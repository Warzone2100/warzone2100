/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

#include "wzglobal.h"

#include <assert.h>
#if !defined(WZ_OS_WIN)
#include <signal.h>
#endif
#include "macros.h"
#include "types.h"

/****************************************************************************************
 *
 * Basic debugging macro's
 *
 */

#define MAX_EVENT_NAME_LEN	100

/** Stores name of the last function or event called by scripts. */
extern char last_called_script_event[MAX_EVENT_NAME_LEN];

/** Whether asserts are currently enabled. */
extern bool assertEnabled;


/* Do the correct assert call for each compiler */
#if defined(WZ_OS_WIN)
#define wz_assert(expr) assert(expr)
#else
#define wz_assert(expr) raise(SIGTRAP)
#endif


/** Deals with failure in an assert. Expression is (re-)evaluated for output in the assert() call. */
#define ASSERT_FAILURE(expr, expr_string, location_description, function, ...) \
	( \
		(void)_debug(LOG_INFO, function, __VA_ARGS__), \
		(void)_debug(LOG_INFO, function, "Assert in Warzone: %s (%s), last script event: '%s'", \
	                                  location_description, expr_string, last_called_script_event), \
		( assertEnabled ? (void)wz_assert(expr) : (void)0 )\
	)

/**
 * Internal assert helper macro to allow some debug functions to use an alternate calling location.
 * Expression is only evaluated once if true, if false it is evaluated another time to provide decent 
 * feedback on OSes that have good GUI facilities for asserts and lousy backtrace facilities. 
 *
 * \param expr                 Expression to assert on.
 * \param location_description A string describing the calling location, e.g.:
 *                             "filename:linenum".
 * \param function             The name of the function that called
 *                             ASSERT_HELPER or the debug function that uses ASSERT_HELPER.
 *
 * \param ...                  printf-style format string followed by its parameters
 *
 * \return Will return whatever assert(expr) returns. That's undefined though,
 *         so unless you have a good reason to, don't depend on it.
 */
#define ASSERT_HELPER(expr, location_description, function, ...) \
( \
	(expr) ? /* if (expr) */ \
		(void)0 \
	: /* else */\
	ASSERT_FAILURE(expr, #expr, location_description, function, __VA_ARGS__) \
)

/**
 *
 * Rewritten version of assert that allows a printf format text string to be passed
 * to ASSERT along with the condition.
 *
 * Arguments:	ASSERT( condition, "Format string with variables: %d, %d", var1, var2 );
 */
#define ASSERT(expr, ...) \
	ASSERT_HELPER(expr, AT_MACRO, __FUNCTION__, __VA_ARGS__)

/**
 *
 * Assert-or-return macro that returns given return value (can also be a mere comma if function has no return value) on failure,
 * and also provides asserts and debug output for debugging.
 */
#define ASSERT_OR_RETURN(retval, expr, ...) \
	do { bool _wzeval = (expr); if (!_wzeval) { ASSERT_FAILURE(expr, #expr, AT_MACRO, __FUNCTION__, __VA_ARGS__); return retval; } } while (0)


/**
 * Compile time assert
 *
 * Forces a compilation error if condition is true, but also produce a result
 * (of value 0 and type size_t), so the expression can be used anywhere that
 * a comma expression isn't permitted.
 *
 * \param expr Expression to evaluate
 *
 * \note BUILD_BUG_ON_ZERO from <linux/kernel.h>
 */
template<bool> class StaticAssert;
template<> class StaticAssert<true>{};
#define STATIC_ASSERT_EXPR(expr) \
	(0*sizeof(StaticAssert<(expr)>))
/**
 * Compile time assert
 * Not to be used in global context!
 * \param expr Expression to evaluate
 */
#define STATIC_ASSERT( expr ) \
	(void)STATIC_ASSERT_EXPR(expr)


/***
 ***
 ***  New debug logging output interface below. Heavily inspired
 ***  by similar code in Freeciv. Parts ripped directly.
 ***
 ***/

/** Debug enums. Must match code_part_names in debug.c */
enum code_part
{
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
  LOG_SAVE,
  LOG_SYNC,
  LOG_DEATH,
  LOG_LIFE,
  LOG_GATEWAY,
  LOG_MSG,
  LOG_INFO, /**< special; on by default, for both debug & release builds */
  LOG_TERRAIN,
  LOG_FEATURE,
  LOG_FATAL,	/**< special; on by default, for both debug & release builds  */
  LOG_INPUT,	// mouse / keyboard events
  LOG_POPUP,	// special, on by default, for both debug & release builds (used for OS dependent popup code)
  LOG_CONSOLE,	// send console messages to file
  LOG_LAST /**< _must_ be last! */
};

extern bool enabled_debug[LOG_LAST];

typedef void (*debug_callback_fn)(void**, const char*);
typedef bool (*debug_callback_init)(void**);
typedef void (*debug_callback_exit)(void**);

struct debug_callback
{
	debug_callback * next;
	debug_callback_fn callback; /// Function which does the output
	debug_callback_init init; /// Setup function
	debug_callback_exit exit; /// Cleaning function
	void * data; /// Used to pass data to the above functions. Eg a filename or handle.
};

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
 * Have the stderr output callback flush its output before returning.
 *
 * NOTE: This may cause significant slowdowns on some systems.
 */
extern void debugFlushStderr(void);

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
bool debug_callback_file_init(void **data);
void debug_callback_file_exit(void **data);

void debug_callback_stderr(void **data, const char *outputBuffer);

#if defined WIN32 && defined DEBUG
void debug_callback_win32debug(void** data, const char* outputBuffer);
#endif

/**
 * Toggle debug output for part associated with str
 *
 * \param	str	Codepart in textformat
 */
bool debug_enable_switch(const char *str);
// macro for always outputting informational responses on both debug & release builds
#define info(...) do { _debug(LOG_INFO, __FUNCTION__, __VA_ARGS__); } while(0)
/**
 * Output printf style format str with additional arguments.
 *
 * Only outputs if debugging of part was formerly enabled with debug_enable_switch.
 */
#define debug(part, ...) do { if (enabled_debug[part]) _debug(part, __FUNCTION__, __VA_ARGS__); } while(0)
void _debug( code_part part, const char *function, const char *str, ...)
		WZ_DECL_FORMAT(printf, 3, 4);

#define debugBacktrace(part, ...) do { if (enabled_debug[part]) { _debug(part, __FUNCTION__, __VA_ARGS__); _debugBacktrace(part); }} while(0)
void _debugBacktrace(code_part part);

/** Global to keep track of which game object to trace. */
extern UDWORD traceID;

/**
 * Output printf style format str for debugging a specific game object whose debug part
 * has been enabled.
 * @see debug
 */
#define objTrace(id, ...) do { if (id == traceID) _realObjTrace(id, __FUNCTION__, __VA_ARGS__); } while(0)
void _realObjTrace(int id, const char *function, const char *str, ...) WZ_DECL_FORMAT(printf, 3, 4);
static inline void objTraceEnable(UDWORD id) { traceID = id; }
static inline void objTraceDisable(void) { traceID = (UDWORD)-1; }

// MSVC specific rotuines to set/clear allocation tracking
#if defined(WZ_CC_MSVC) && defined(DEBUG)
void debug_MEMCHKOFF(void);
void debug_MEMCHKON(void);
void debug_MEMSTATS(void);
#endif

/** Checks if a particular debub flag was enabled */
extern bool debugPartEnabled(code_part codePart);

void debugDisableAssert(void);

#endif // __INCLUDED_LIB_FRAMEWORK_DEBUG_H__
