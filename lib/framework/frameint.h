/*! \file frameint.h
 *  \brief Internal definitions for the framework library.
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


/* Initialise the double buffered display */
extern BOOL screenInitialise(UDWORD		width,			// Display width
							 UDWORD		height,			// Display height
							 UDWORD		bitDepth,		// Display bit depth
							 BOOL		fullScreen		// Whether to start windowed
														// or full screen
							 );


/* Release the DD objects */
extern void screenShutDown(void);

/* In full screen mode flip to the GDI buffer.
 * Use this if you want the user to see any GDI output.
 * This is mainly used so that ASSERTs and message boxes appear
 * even in full screen mode.
 */
extern void screenFlipToGDI(void);

/* This is called once a frame so that the system can tell
 * whether a key was pressed this turn or held down from the last frame.
 */
extern void inputNewFrame(void);

/* Release all the allocated surfaces */
extern void surfShutDown(void);

/* The Current screen size and bit depth */
extern UDWORD		screenWidth;
extern UDWORD		screenHeight;
extern UDWORD		screenDepth;

#endif

