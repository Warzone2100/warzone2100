#ifndef _debug_h
#define _debug_h

//#define ALWAYS_ASSERT	// Define this to always process ASSERT even on release builds.

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

/* DBMB used to be 'show message box' */
#define DBMB(x) _db_debug x
#define DBPRINTF(x) _db_debug x
#define _db_debug(...) debug(LOG_NEVER, __VA_ARGS__)

/*
 *
 * ASSERT
 *
 * Rewritten version of assert that allows a printf format text string to be passed
 * to ASSERT along with the condition.
 *
 * Arguments:	ASSERT((condition, "Format string with variables: %d, %d", var1, var2));
 */
#define ASSERT(x) my_assert x
#define my_assert(x, y, ...) assert(y)

/*
 *
 * DBERROR
 *
 * Error message macro - use this if the error should be reported even in
 * production code (i.e. out of memory errors, file not found etc.)
 *
 * Arguments as for printf
 */

#define DBERROR(x) _debug_error x
#define _debug_error(...) 			\
	do {					\
		debug(LOG_ERROR, __VA_ARGS__);	\
		abort();			\
	} while (FALSE);


/****************************************************************************************
 *
 * Conditional debugging macro's that can be selectively turned on or off on a file
 * by file basis.
 *
 */

#ifdef DEBUG_GROUP0
#define DBP0(x)							DBPRINTF(x)
#define DBMB0(x)						DBMB(x)
#else
#define DBP0(x)
#define DBMB0(x)
#endif

#ifdef DEBUG_GROUP1
#define DBP1(x)							DBPRINTF(x)
#define DBMB1(x)						DBMB(x)
#else
#define DBP1(x)
#define DBMB1(x)
#endif

#ifdef DEBUG_GROUP2
#define DBP2(x)							DBPRINTF(x)
#define DBMB2(x)						DBMB(x)
#else
#define DBP2(x)
#define DBMB2(x)
#endif

#ifdef DEBUG_GROUP3
#define DBP3(x)							DBPRINTF(x)
#define DBMB3(x)						DBMB(x)
#else
#define DBP3(x)
#define DBMB3(x)
#endif

#ifdef DEBUG_GROUP4
#define DBP4(x)							DBPRINTF(x)
#define DBMB4(x)						DBMB(x)
#else
#define DBP4(x)
#define DBMB4(x)
#endif

#ifdef DEBUG_GROUP5
#define DBP5(x)							DBPRINTF(x)
#define DBMB5(x)						DBMB(x)
#else
#define DBP5(x)
#define DBMB5(x)
#endif

#ifdef DEBUG_GROUP6
#define DBP6(x)							DBPRINTF(x)
#define DBMB6(x)						DBMB(x)
#else
#define DBP6(x)
#define DBMB6(x)
#endif

#ifdef DEBUG_GROUP7
#define DBP7(x)							DBPRINTF(x)
#define DBMB7(x)						DBMB(x)
#else
#define DBP7(x)
#define DBMB7(x)
#endif

#ifdef DEBUG_GROUP8
#define DBP8(x)							DBPRINTF(x)
#define DBMB8(x)						DBMB(x)
#else
#define DBP8(x)
#define DBMB8(x)
#endif

#ifdef DEBUG_GROUP9
#define DBP9(x)							DBPRINTF(x)
#define DBMB9(x)						DBMB(x)
#else
#define DBP9(x)
#define DBMB9(x)
#endif

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

enum code_part {
  LOG_ALL, /* special: sets all to on */
  LOG_SOUND, 
  LOG_VIDEO, 
  LOG_3D,
  LOG_ERROR, /* special; on by default */
  LOG_NEVER, /* if too verbose for anything but dedicated debugging... */
  LOG_LAST /* _must_ be last! */
};

typedef void (*log_callback_fn)(const char*);

void debug_init(void);
void debug_to_file(char *file);
void debug_use_callback(log_callback_fn use_callback);
void debug_enable_switch(const char *str);
void debug(enum code_part part, const char *str, ...)
           wz__attribute((format (printf, 2, 3)));

#endif
