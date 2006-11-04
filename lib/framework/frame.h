/*! \file frame.h
 * \brief The framework library initialisation and shutdown routines.
 */
#ifndef _frame_h
#define _frame_h

#include "platform.h"
#include "macros.h"

#include "types.h"
#include "debug.h"
#include "mem.h"

#include "heap.h"
#include "treap.h"
#include "block.h"

#include "fractions.h"
#include "trig.h"


/* Initialise the frame work library */
extern BOOL frameInitialise(
					const char *pWindowName,// The text to appear in the window title bar
					UDWORD width,			// The display width
					UDWORD height,			// The display height
					UDWORD bitDepth,		// The display bit depth
					BOOL fullScreen		// Whether to start full screen or windowed
					);

/* Shut down the framework library.
 * This clears up all the Direct Draw stuff and ensures
 * that Windows gets restored properly after Full screen mode.
 */
extern void frameShutDown(void);

/* The current status of the framework */
typedef enum _frame_status
{
	FRAME_OK,			// Everything normal
	FRAME_KILLFOCUS,	// The main app window has lost focus (might well want to pause)
	FRAME_SETFOCUS,		// The main app window has focus back
	FRAME_QUIT,			// The main app window has been told to quit
} FRAME_STATUS;

/* Call this each cycle to allow the framework to deal with
 * windows messages, and do general house keeping.
 *
 * Returns FRAME_STATUS.
 */
extern FRAME_STATUS frameUpdate(void);

/* If cursor on is TRUE the windows cursor will be displayed over the game window
 * (and in full screen mode).  If it is FALSE the cursor will not be displayed.
 */
extern void frameShowCursor(BOOL cursorOn);

/* Set the current cursor from a Resource ID
 * This is the same as calling:
 *       frameSetCursor(LoadCursor(MAKEINTRESOURCE(resID)));
 * but with a bit of extra error checking.
 */
extern void frameSetCursorFromRes(SWORD resID);

/* Returns the current frame we're on - used to establish whats on screen */
extern UDWORD	frameGetFrameNumber(void);

/**
 * Average framerate of the last seconds
 *
 * \return Average framerate
 */
extern UDWORD frameGetAverageRate(void);


/* Load the file with name pointed to by pFileName into a memory buffer. */
BOOL loadFile(const char *pFileName,		// The filename
              char **ppFileData,	// A buffer containing the file contents
              UDWORD *pFileSize);	// The size of this buffer

/* Save the data in the buffer into the given file */
extern BOOL saveFile(const char *pFileName, const char *pFileData, UDWORD fileSize);

// load a file from disk into a fixed memory buffer
BOOL loadFileToBuffer(const char *pFileName, char *pFileBuffer, UDWORD bufferSize, UDWORD *pSize);

// as above but returns quietly if no file found
BOOL loadFileToBufferNoError(const char *pFileName, char *pFileBuffer, UDWORD bufferSize,
                             UDWORD *pSize);

extern SDWORD ftol(float f);

UDWORD HashString( const char *String );
UDWORD HashStringIgnoreCase( const char *String );


/* Endianness hacks */

static inline void endian_uword(UWORD *uword) {
#ifdef __BIG_ENDIAN__
  UBYTE tmp, *ptr;

  ptr = (UBYTE *) uword;
  tmp = ptr[0];
  ptr[0] = ptr[1];
  ptr[1] = tmp;
#endif
}

static inline void endian_sword(SWORD *sword) {
#ifdef __BIG_ENDIAN__
  UBYTE tmp, *ptr;

  ptr = (UBYTE *) sword;
  tmp = ptr[0];
  ptr[0] = ptr[1];
  ptr[1] = tmp;
#endif
}

static inline void endian_udword(UDWORD *udword) {
#ifdef __BIG_ENDIAN__
  UBYTE tmp, *ptr;

  ptr = (UBYTE *) udword;
  tmp = ptr[0];
  ptr[0] = ptr[3];
  ptr[3] = tmp;
  tmp = ptr[1];
  ptr[1] = ptr[2];
  ptr[2] = tmp;
#endif
}

static inline void endian_sdword(SDWORD *sdword) {
#ifdef __BIG_ENDIAN__
  UBYTE tmp, *ptr;

  ptr = (UBYTE *) sdword;
  tmp = ptr[0];
  ptr[0] = ptr[3];
  ptr[3] = tmp;
  tmp = ptr[1];
  ptr[1] = ptr[2];
  ptr[2] = tmp;
#endif
}

static inline void endian_fract(FRACT *fract) {
#ifdef __BIG_ENDIAN__
  UBYTE tmp, *ptr;

  ptr = (UBYTE *) fract;
  tmp = ptr[0];
  ptr[0] = ptr[3];
  ptr[3] = tmp;
  tmp = ptr[1];
  ptr[1] = ptr[2];
  ptr[2] = ptr[1];
#endif
}


#endif
