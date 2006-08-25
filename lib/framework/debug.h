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

/* DBMB used to be 'show message box' */
/*
#ifndef _MSC_VER
#define DBMB(x) _db_debug x
#define DBPRINTF(x) _db_debug x
#define _db_debug(...) debug(LOG_NEVER, __VA_ARGS__)
#else
#define DBMB(x)
#define DBPRINTF(x)
#endif
*/

/*
 *
 * ASSERT
 *
 * Rewritten version of assert that allows a printf format text string to be passed
 * to ASSERT along with the condition.
 *
 * Arguments:	ASSERT( condition, "Format string with variables: %d, %d", var1, var2 );
 */
/*
# define ASSERT(x) wz_assert x
#ifdef _MSC_VER
// MSVC doesn't understand __VAR_ARGS__ macros
# define wz_assert(x, ...) \
do { \
	if (!(x)) { \
		debug( LOG_ERROR, "Assert in Warzone: file %s, line %d", \
			__FILE__, __LINE__ ); \
		assert(x); \
} \
} while (FALSE)
#else
# define wz_assert(x, ...) \
do { \
	if (!(x)) { \
		debug( LOG_ERROR, "Assert in Warzone: file %s, function %s, line %d", \
			__FILE__, __FUNCTION__, __LINE__ ); \
		debug( LOG_ERROR, __VA_ARGS__ ); \
		assert(x); \
} \
} while (FALSE)
#endif
*/
#define ASSERT( x, ... ) \
   (void)( x ? 0 : debug( LOG_ERROR, __VA_ARGS__ ) ); \
   (void)( x ? 0 : debug( LOG_ERROR, "Assert in Warzone: %s:%d : %s (%s)", \
		__FILE__, __LINE__, __FUNCTION__, (#x) ) ); \
	assert( x );

/*
 *
 * DBERROR
 *
 * Error message macro - use this if the error should be reported even in
 * production code (i.e. out of memory errors, file not found etc.)
 *
 * Arguments as for printf
 */
/*
#ifndef _MSC_VER
#define DBERROR(x) _debug_error x
#define _debug_error(...) 			\
	do {					\
		debug(LOG_ERROR, __VA_ARGS__);	\
		abort();			\
	} while (FALSE);
#else
#define DBERROR(x)
#endif
*/


/****************************************************************************************
 *
 * Conditional debugging macro's that can be selectively turned on or off on a file
 * by file basis.
 *
 */

#define DBPRINTF(x)
#define DBMB(x)
#define DBP0(x)							DBPRINTF(x)
#define DBMB0(x)						DBMB(x)
#define DBP1(x)							DBPRINTF(x)
#define DBMB1(x)						DBMB(x)
#define DBP2(x)							DBPRINTF(x)
#define DBMB2(x)						DBMB(x)
#define DBP3(x)							DBPRINTF(x)
#define DBMB3(x)						DBMB(x)
#define DBP4(x)							DBPRINTF(x)
#define DBMB4(x)						DBMB(x)
#define DBP5(x)							DBPRINTF(x)
#define DBMB5(x)						DBMB(x)
#define DBP6(x)							DBPRINTF(x)
#define DBMB6(x)						DBMB(x)
#define DBP7(x)							DBPRINTF(x)
#define DBMB7(x)						DBMB(x)
#define DBP8(x)							DBPRINTF(x)
#define DBMB8(x)						DBMB(x)
#define DBP9(x)							DBPRINTF(x)
#define DBMB9(x)						DBMB(x)

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
