/*
 * Frame.h
 *
 * The framework library initialisation and shutdown routines.
 *
 */
#ifndef _frame_h
#define _frame_h

#pragma warning (disable : 4201 4214 4115 4514)
#include <windows.h>
#pragma warning (default : 4201 4214 4115)

#ifdef PSX		// If Playstation version then compile lean version.

#ifdef FINALBUILD
#define scrv_error(a...) ;	// don't want the yackky errors on final version
#endif

#define FRAMEWORK_LEAN_AND_MEAN
#include "cfunc.h"	// redefines memset & cpy to fast version - calls cfunc.lib
#include "printf.h"	// in framepsx
#endif

#include "types.h"
#include "debug.h"
#include "mem.h"
#include "screen.h"
#include "ddraw.h"
#include "dderror.h"
#include "input.h"
#include "surface.h"
#include "Image.h"
#include "Font.h"
#include "Heap.h"
#include "Treap.h"
#include "w95trace.h"
#include "Fractions.h"
#include "Trig.h"
#include "FrameResource.h"
#include "StrRes.h"
#ifdef WIN32
#include "DXInput.h"
#endif
#include "Block.h"
#include "ListMacs.h"

/* Initialise the frame work library */
extern BOOL frameInitialise(HANDLE hInstance,		// The windows application instance
					 STRING *pWindowName,	// The text to appear in the window title bar
					 UDWORD	width,			// The display width
					 UDWORD height,			// The display height
					 UDWORD bitDepth,		// The display bit depth
					 BOOL	fullScreen,		// Whether to start full screen or windowed
					 BOOL	bVidMem,	 	// Whether to put surfaces in video memory
					 BOOL	bGlide );		// Whether to create surfaces

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
extern BOOL loadFile(STRING *pFileName,		// The filename
					 UBYTE **ppFileData,	// A buffer containing the file contents
					 UDWORD *pFileSize);	// The size of this buffer

/* Load the file with name pointed to by pFileName into a memory buffer. */
// if allocate mem is true then the memory is allocated ... else it is already in ppFileData, and the max size is in pFileSize ... this is adjusted to the actual loaded file size
//   
BOOL loadFile2(STRING *pFileName, UBYTE **ppFileData, UDWORD *pFileSize, BOOL AllocateMem );

/* Save the data in the buffer into the given file */
extern BOOL saveFile(STRING *pFileName, UBYTE *pFileData, UDWORD fileSize);

#ifdef WIN32
// load a file from disk into a fixed memory buffer
extern BOOL loadFileToBuffer(STRING *pFileName, UBYTE *pFileBuffer, UDWORD bufferSize, UDWORD *pSize);
// as above but returns quietly if no file found
extern BOOL loadFileToBufferNoError(STRING *pFileName, UBYTE *pFileBuffer, UDWORD bufferSize, UDWORD *pSize);

extern SDWORD ftol(float f);
extern BOOL	bRunningUnderGlide;

#endif

UINT HashString( char *String );
UINT HashStringIgnoreCase( char *String );


#endif

