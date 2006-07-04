/*
 * Frame.h
 *
 * The framework library initialisation and shutdown routines.
 *
 */
#ifndef _frame_h
#define _frame_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _MSC_VER
# ifdef _DEBUG
#  define _CRTDBG_MAP_ALLOC
#  include <stdlib.h>
#  include <crtdbg.h>
# endif // _DEBUG
#endif

#ifdef WIN32
#include <windows.h>
#endif

#define MAX_MODS 100

/* Linux specific stuff */
#ifndef WIN32

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_PATH 250

char *unix_path(const char *path);
FILE *unix_fopen(const char *filename, const char *mode);

#define fopen unix_fopen

#endif

#define InitializeCriticalSection(x)
#define DeleteCriticalSection(x)
#define EnterCriticalSection(x)
#define LeaveCriticalSection(x)

#include "macros.h"

#include "types.h"
#include "debug.h"
#include "mem.h"

#include "input.h"
#include "font.h"
#include "heap.h"
#include "treap.h"
#include "fractions.h"
#include "trig.h"
#include "frameresource.h"
#include "strres.h"
#include "block.h"
#include "listmacs.h"

#ifdef _MSC_VER
#include "win32fixes.h"
#endif

#ifndef WIN32
DWORD GetTickCount();
#endif

/* Initialise the frame work library */
extern BOOL frameInitialise(HANDLE hInstance,		// The windows application instance
					 STRING *pWindowName,	// The text to appear in the window title bar
					 UDWORD	width,			// The display width
					 UDWORD height,			// The display height
					 UDWORD bitDepth,		// The display bit depth
					 BOOL	fullScreen,		// Whether to start full screen or windowed
					 BOOL	bVidMem);	 	// Whether to put surfaces in video memory

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

/* Set the current cursor from a cursor handle */
extern void frameSetCursor(HCURSOR hNewCursor);

/* Set the current cursor from a Resource ID
 * This is the same as calling:
 *       frameSetCursor(LoadCursor(MAKEINTRESOURCE(resID)));
 * but with a bit of extra error checking.
 */
extern void frameSetCursorFromRes(WORD resID);

/* Returns the current frame we're on - used to establish whats on screen */
extern UDWORD	frameGetFrameNumber(void);

/* Return the current frame rate */
extern UDWORD frameGetFrameRate(void);

/* Return the overall frame rate */
extern UDWORD frameGetOverallRate(void);

/* Return the frame rate for the last second */
extern UDWORD frameGetRecentRate(void);

/* The handle for the application window */
extern HANDLE	frameGetWinHandle(void);

//enumerate all available direct draw devices
extern BOOL frameDDEnumerate(void);

extern SDWORD frameGetNumDDDevices(void);

extern char* frameGetDDDeviceName(SDWORD);

// Return a string for a windows error code
extern STRING *winErrorToString(SDWORD error);

/* The default window procedure for the library.
 * This is initially set to the standard DefWindowProc, but can be changed
 * by this function.
 * Call this function with NULL to reset to DefWindowProc.
 */
typedef LRESULT (* DEFWINPROCTYPE)(HWND hWnd, UINT Msg,
										 WPARAM wParam, LPARAM lParam);
extern void frameSetWindowProc(DEFWINPROCTYPE winProc);

/* Load the file with name pointed to by pFileName into a memory buffer. */
BOOL loadFile(const char *pFileName,		// The filename
              UBYTE **ppFileData,	// A buffer containing the file contents
              UDWORD *pFileSize);	// The size of this buffer

/* Save the data in the buffer into the given file */
extern BOOL saveFile(const char *pFileName, const char *pFileData, UDWORD fileSize);

// load a file from disk into a fixed memory buffer
BOOL loadFileToBuffer(char *pFileName, UBYTE *pFileBuffer, UDWORD bufferSize, UDWORD *pSize);

// as above but returns quietly if no file found
BOOL loadFileToBufferNoError(char *pFileName, UBYTE *pFileBuffer, UDWORD bufferSize,
                             UDWORD *pSize);

extern SDWORD ftol(float f);

UINT HashString( char *String );
UINT HashStringIgnoreCase( char *String );

#endif
