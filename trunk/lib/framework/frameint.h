/*
 * FrameInt.h
 *
 * Internal definitions for the framework library.
 *
 */
#ifndef _frameint_h
#define _frameint_h

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */
#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif


/* Define the style and extended style of the window.
 * Need these to calculate the size the window should be when returning to
 * window mode.
 *
 * create a title bar, minimise button on the title bar,
 * automatic ShowWindow, get standard system menu on title bar
 */
#define WIN_STYLE (WS_CAPTION | WS_MINIMIZEBOX | WS_VISIBLE | WS_SYSMENU)

#define WIN_EXSTYLE	 WS_EX_APPWINDOW	// Go on task bar when iconified


/* Program hInstance */
extern HINSTANCE	hInstance;

/* Handle for the main window */
extern HANDLE		hWndMain;

/* Initialise the double buffered display */


extern BOOL screenInitialise(UDWORD		width,			// Display width
							 UDWORD		height,			// Display height
							 UDWORD		bitDepth,		// Display bit depth
							 BOOL		fullScreen,		// Whether to start windowed
														// or full screen.
							 BOOL		bVidMem,		// Whether to put surfaces in
														// video memory
							 BOOL		bDDraw,			// Whether to create ddraw surfaces												// video memory
							 HANDLE		hWindow);		// The main windows handle







/* Release the DD objects */
extern void screenShutDown(void);

/* Restore the direct draw surfaces - internal use only */
extern void screenRestoreSurfaces(void);

/* In full screen mode flip to the GDI buffer.
 * Use this if you want the user to see any GDI output.
 * This is mainly used so that ASSERTs and message boxes appear
 * even in full screen mode.
 */
extern void screenFlipToGDI(void);

/* Deal with windows messages to maintain the state of the keyboard and mouse */
extern void inputProcessMessages(UINT message, WPARAM wParam, LPARAM lParam);

/* This is called once a frame so that the system can tell
 * whether a key was pressed this turn or held down from the last frame.
 */
extern void inputNewFrame(void);

/* The list of surfaces structure */
typedef struct _surface_list
{
	void *psSurface;
	struct _surface_list	*psNext;
} SURFACE_LIST;

/* The list of surfaces */
extern SURFACE_LIST	*psSurfaces;

/* Release all the allocated surfaces */
extern void surfShutDown(void);

/* Free current currently open widget file */
BOOL FreeCurrentWDG(void);

/* The Current screen size and bit depth */
extern UDWORD		screenWidth;
extern UDWORD		screenHeight;
extern UDWORD		screenDepth;

// The critical section for the screen flipping
extern CRITICAL_SECTION sScreenFlipCritical;

// The semaphore for the screen flipping
extern HANDLE	hScreenFlipSemaphore;


#endif

