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
/*! \file frame.h
 * \brief The framework library initialisation and shutdown routines.
 */
#ifndef _frame_h
#define _frame_h

#include "wzglobal.h"

/* This one must be invoked *after* wzglobal.h to get _GNU_SOURCE! */
#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <ctype.h>

// Provides the safer functions strlcpy and strlcat
#include "strlfuncs.h"

#include "gettext.h"

#if !defined(LC_MESSAGES)
# define LC_MESSAGES 0
#endif

#define _(String) gettext(String)
#define N_(String) gettext_noop(String)

// Context sensitive strings
#define P_(Context, String) pgettext(Context, String)
// Non literal context sensitive strings
#define PE_(Context, String) pgettext_expr(Context, String)
// Make xgettext recognize the context
#define NP_(Context, String) gettext_noop(String)


#include "macros.h"
#include "types.h"
#include "debug.h"

#include "treap.h"
#include "printf_ext.h"

#include "fractions.h"
#include "trig.h"
#include <physfs.h>

extern UDWORD selectedPlayer;

/** Initialise the frame work library */
extern BOOL frameInitialise(
					const char *pWindowName,// The text to appear in the window title bar
					UDWORD width,			// The display width
					UDWORD height,			// The display height
					UDWORD bitDepth,		// The display bit depth
					BOOL fullScreen		// Whether to start full screen or windowed
					);

/** Shut down the framework library.
 * This clears up all the Direct Draw stuff and ensures
 * that Windows gets restored properly after Full screen mode.
 */
extern void frameShutDown(void);

typedef enum _focus_state
{
	FOCUS_OUT,		// Window does not have the focus
	FOCUS_IN,		// Window has got the focus
} FOCUS_STATE;

/*!
 * Set the framerate limit
 *
 * \param fpsLimit Desired framerate
 */
extern void setFramerateLimit(int fpsLimit);

/*!
 * Get the framerate limit
 *
 * \return Desired framerate
 */
extern int getFramerateLimit(void);

/** Call this each cycle to allow the framework to deal with
 * windows messages, and do general house keeping.
 */
extern void frameUpdate(void);

/** Set the current cursor from a Resource ID
 * This is the same as calling:
 *       frameSetCursor(LoadCursor(MAKEINTRESOURCE(resID)));
 * but with a bit of extra error checking.
 */
extern void frameSetCursorFromRes(SWORD resID);

/** Returns the current frame we're on - used to establish whats on screen. */
extern UDWORD frameGetFrameNumber(void);

/** Return average framerate of the last seconds. */
extern UDWORD frameGetAverageRate(void);


/** Load the file with name pointed to by pFileName into a memory buffer. */
extern BOOL loadFile(const char *pFileName,		// The filename
              char **ppFileData,	// A buffer containing the file contents
              UDWORD *pFileSize);	// The size of this buffer

extern PHYSFS_file* openLoadFile(const char* fileName, bool hard_fail);
extern PHYSFS_file* openSaveFile(const char* fileName);

/** Save the data in the buffer into the given file */
extern BOOL saveFile(const char *pFileName, const char *pFileData, UDWORD fileSize);

/** Load a file from disk into a fixed memory buffer. */
BOOL loadFileToBuffer(const char *pFileName, char *pFileBuffer, UDWORD bufferSize, UDWORD *pSize);

/** Load a file from disk, but returns quietly if no file found. */
BOOL loadFileToBufferNoError(const char *pFileName, char *pFileBuffer, UDWORD bufferSize,
                             UDWORD *pSize);

extern SDWORD ftol(float f);

UDWORD HashString( const char *String );
UDWORD HashStringIgnoreCase( const char *String );


/* Endianness hacks */
// TODO Use SDL_SwapXXXX instead

#ifdef __BIG_ENDIAN__
static inline void endian_uword(UWORD *uword) {
  UBYTE tmp, *ptr;

  ptr = (UBYTE *) uword;
  tmp = ptr[0];
  ptr[0] = ptr[1];
  ptr[1] = tmp;
}
#else
# define endian_uword(x) ((void) (x))
#endif

#ifdef __BIG_ENDIAN__
static inline void endian_sword(SWORD *sword) {
  UBYTE tmp, *ptr;

  ptr = (UBYTE *) sword;
  tmp = ptr[0];
  ptr[0] = ptr[1];
  ptr[1] = tmp;
}
#else
# define endian_sword(x) ((void) (x))
#endif

#ifdef __BIG_ENDIAN__
static inline void endian_udword(UDWORD *udword) {
  UBYTE tmp, *ptr;

  ptr = (UBYTE *) udword;
  tmp = ptr[0];
  ptr[0] = ptr[3];
  ptr[3] = tmp;
  tmp = ptr[1];
  ptr[1] = ptr[2];
  ptr[2] = tmp;
}
#else
# define endian_udword(x) ((void) (x))
#endif

#ifdef __BIG_ENDIAN__
static inline void endian_sdword(SDWORD *sdword) {
  UBYTE tmp, *ptr;

  ptr = (UBYTE *) sdword;
  tmp = ptr[0];
  ptr[0] = ptr[3];
  ptr[3] = tmp;
  tmp = ptr[1];
  ptr[1] = ptr[2];
  ptr[2] = tmp;
}
#else
# define endian_sdword(x) ((void) (x))
#endif

#ifdef __BIG_ENDIAN__
static inline void endian_fract(float *fract) {
  UBYTE tmp, *ptr;

  ptr = (UBYTE *) fract;
  tmp = ptr[0];
  ptr[0] = ptr[3];
  ptr[3] = tmp;
  tmp = ptr[1];
  ptr[1] = ptr[2];
  ptr[2] = ptr[1];
}
#else
# define endian_fract(x) ((void) (x))
#endif

void setupExceptionHandler(const char * programCommand);

static inline bool PHYSFS_writeSBE8(PHYSFS_file* file, int8_t val)
{
	return (PHYSFS_write(file, &val, sizeof(int8_t), 1) == 1);
}

static inline bool PHYSFS_writeUBE8(PHYSFS_file* file, uint8_t val)
{
	return (PHYSFS_write(file, &val, sizeof(uint8_t), 1) == 1);
}

static inline bool PHYSFS_readSBE8(PHYSFS_file* file, int8_t* val)
{
	return (PHYSFS_read(file, val, sizeof(int8_t), 1) == 1);
}

static inline bool PHYSFS_readUBE8(PHYSFS_file* file, uint8_t* val)
{
	return (PHYSFS_read(file, val, sizeof(uint8_t), 1) == 1);
}

static inline bool PHYSFS_writeBEFloat(PHYSFS_file* file, float val)
{
	// For the purpose of endian conversions a IEEE754 float can be considered
	// the same to a 32bit integer.
	// We're using a union here to prevent type punning of pointers.
	union {
		float f;
		uint32_t i;
	} writeValue;
	writeValue.f = val;
	return (PHYSFS_writeUBE32(file, writeValue.i) != 0);
}

static inline bool PHYSFS_readBEFloat(PHYSFS_file* file, float* val)
{
	// For the purpose of endian conversions a IEEE754 float can be considered
	// the same to a 32bit integer.
	uint32_t* readValue = (uint32_t*)val;
	return (PHYSFS_readUBE32(file, readValue) != 0);
}

#endif
