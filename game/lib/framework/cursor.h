/*
 * Cursor.h
 *
 * A thread based cursor implementation that allows a cursor to update faster
 * than the program framerate.
 *
 */
#ifndef _cursor_h
#define _cursor_h

// data required to load a new cursor description
typedef struct _cursor_load
{
	SDWORD			cursorWidth,cursorHeight;	// cursor size
	SDWORD			numFrames;					// number of frames (1 for no animation)
	SDWORD			frameTime;					// time to display one frame for (in ms)
	SDWORD			imgWidth,imgHeight;			// image data size
	UBYTE			*pImageData;				// image data
	PALETTEENTRY	*psPalette;					// palette data
} CURSOR_LOAD;

// Initialise the cursor system, specifying the maximum size of cursor bitmap
extern BOOL cursorInitialise(SDWORD width, SDWORD height);

// Shutdown the cursor system
extern void cursorShutDown(void);

// Load the cursor image data
extern BOOL cursorLoad(CURSOR_LOAD *psLoadData);

// Start displaying the cursor
extern BOOL cursorDisplay(void);

// Stop displaying the cursor
extern BOOL cursorHide(void);

#endif


