#ifndef _debug_h
#define _debug_h

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */

#include <stdio.h>

#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif

#include <assert.h>
#include <stdarg.h>
#include "types.h"

/****************************************************************************************
 *
 * Basic debugging macro's
 *
 */
#ifdef _MSC_VER
#ifdef _DEBUG
#define DEBUG
#endif
#endif

/*
 *
 * ASSERT
 *
 * Rewritten version of assert that allows a printf format text string to be passed
 * to ASSERT along with the condition.
 *
 * Arguments:	ASSERT( condition, "Format string with variables: %d, %d", var1, var2 );
 */
#define ASSERT( expr, ... ) \
	( (expr) ? (void)0 : (void)debug( LOG_ERROR, __VA_ARGS__ ) ); \
	( (expr) ? (void)0 : (void)debug( LOG_ERROR, "Assert in Warzone: %s:%d : %s (%s)", \
		__FILE__, __LINE__, __FUNCTION__, (#expr) ) ); \
	assert( expr );


/****************************************************************************************
 *
 * Conditional debugging macro's that can be selectively turned on or off on a file
 * by file basis.
 *
 * Modified to not output nothing under no conditions
 *
 */

#define DBMB0(x)
#define DBP1(x)
#define DBMB1(x)
#define DBP2(x)
#define DBMB2(x)
#define DBP3(x)
#define DBMB3(x)
#define DBP4(x)
#define DBMB4(x)
#define DBP5(x)
#define DBMB5(x)
#define DBP6(x)
#define DBMB6(x)
#define DBP7(x)
#define DBMB7(x)
#define DBP8(x)
#define DBMB8(x)
#define DBP9(x)
#define DBMB9(x)

/***
 ***
 ***  New debug logging output interface below. Heavily inspired
 ***  by similar code in Freeciv. Parts ripped directly.
 ***
 ***/

/* Want to use GCC's __attribute__ keyword to check variadic
 * parameters to printf-like functions, without upsetting other
 * compilers: put any required defines magic here.
 * If other compilers have something equivalent, could also
 * work that out here.   Should this use configure stuff somehow?
 * --dwp
 */
#if defined(__GNUC__)
#define wz__attribute(x)  __attribute__(x)
#else
#define wz__attribute(x)
#endif

/* Must match code_part_names in debug.c */
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
  LOG_ERROR, /* special; on by default */
  LOG_NEVER, /* if too verbose for anything but dedicated debugging... */
  LOG_SCRIPT,
  LOG_LAST /* _must_ be last! */
} code_part;

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
void debug( code_part part, const char *str, ...)
		wz__attribute((format (printf, 2, 3)) );

#endif
