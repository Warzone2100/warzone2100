/*
 * Screen.h
 *
 * Interface to the Direct Draw double buffered display.
 *
 */
#ifndef _screen_h
#define _screen_h

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */
#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif

#ifdef WIN32
#define INIT_GUID
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#endif

#include "lib/framework/types.h"

/* Free up a COM object */
#undef RELEASE
#define RELEASE(x) if ((x) != NULL) {(void)(x)->lpVtbl->Release(x); (x) = NULL;}

/* Return a pointer to the Direct Draw objects */
extern LPDIRECTDRAW4 screenGetDDObject(void);

/* Return a pointer to the Direct Draw back buffer surface */
extern LPDIRECTDRAWSURFACE4 screenGetSurface(void);

/* Return a pointer to the front buffer pixel format */
extern DDPIXELFORMAT *screenGetFrontBufferPixelFormat(void);

/* Return a pointer to the back buffer pixel format */
extern DDPIXELFORMAT *screenGetBackBufferPixelFormat(void);

/* Return a bit depth of the Front buffer */
extern UDWORD screenGetFrontBufferBitDepth(void);

/* Return a bit depth of the Back buffer */
extern UDWORD screenGetBackBufferBitDepth(void);

/* Return a pixel masks of the Front buffer */
extern BOOL screenGetFrontBufferPixelFormatMasks(ULONG *amask, ULONG *rmask, ULONG *gmask, ULONG *bmask);

/* Return a pixel masks of the Back buffer */
extern BOOL screenGetBackBufferPixelFormatMasks(ULONG *amask, ULONG *rmask, ULONG *gmask, ULONG *bmask);

/* Flip back and front buffers */
extern void screenFlip(BOOL clearBackBuffer);

/* backDrop */
extern void screen_SetBackDrop(UWORD *newBackDropBmp, UDWORD width, UDWORD height);
extern void screen_StopBackDrop(void);
extern void screen_RestartBackDrop(void);
extern UWORD* screen_GetBackDrop(void);
extern UDWORD screen_GetBackDropWidth(void);
extern void screen_Upload(UWORD *newBackDropBmp);

/* screendump */
char* screenDumpToDisk(char* path);

/* fog */
void screen_SetFogColour(UDWORD newFogColour);

/* Toggle the display between full screen or windowed */
extern void	screenToggleMode(void);

/* Toggle the display between 8 bit and 16 bit */
extern BOOL	screenToggleVideoPlaybackMode(void);

typedef enum _screen_mode
{
	SCREEN_FULLSCREEN,
	SCREEN_WINDOWED,
	SCREEN_FULLSCREEN_VIDEO
} SCREEN_MODE;

/* get screen window handle */
extern HWND screenGetHWnd( void );

extern SDL_Surface *screenGetSDL(void);

/* Set whether the display is windowed or full screen */
extern void screenSetMode(SCREEN_MODE mode);

/* Get display mode (windowed or full screen) */
extern SCREEN_MODE screenGetMode( void );

/* Set palette entries for the display buffer
 * first specifies the first palette entry. count the number of entries
 * The psPalette should have at least first + count entries in it.
 */
extern void screenSetPalette(UDWORD first, UDWORD count, PALETTEENTRY *psPalette);

/* Return the best colour match when in a palettised mode */
extern UBYTE screenGetPalEntry(UBYTE red, UBYTE green, UBYTE blue);

/* Set the colour for text */
extern void screenSetTextColour(UBYTE red, UBYTE green, UBYTE blue);

/* Output text to the display screen at location x,y.
 * The remaining arguments are as printf.
 */
extern void screenTextOut(UDWORD x, UDWORD y, STRING *pFormat, ...);

/* Blit the source rectangle of the surface
 * to the back buffer at the given location.
 * The blit is clipped to the screen size.
 */
extern void screenBlit(SDWORD destX, SDWORD destY,		// The location on screen
				LPDIRECTDRAWSURFACE4 psSurf,		// The surface to blit from
				UDWORD	srcX, UDWORD srcY,
				UDWORD	width, UDWORD height);	// The source rectangle from the surface

/*
 * Blit the source rectangle to the back buffer scaling it to
 * the size of the destination rectangle.
 * This clips to the size of the back buffer.
 */
extern void screenScaleBlit(SDWORD destX, SDWORD destY,
					 SDWORD destWidth, SDWORD destHeight,
					 LPDIRECTDRAWSURFACE4 psSurf,
					 SDWORD srcX, SDWORD srcY,
					 SDWORD srcWidth, SDWORD srcHeight);

/* Blit a tile (rectangle) from the surface
 * to the back buffer at the given location.
 * The tile is specified by it's size and number, numbering
 * across from top left to bottom right.
 * The blit is clipped to the screen size.
 */
extern void screenBlitTile(SDWORD destX, SDWORD destY,	// The location on screen
				LPDIRECTDRAWSURFACE4 psSurf,		// The surface to blit from
				UDWORD	width, UDWORD height,	// The size of the tile
				UDWORD  tile);					// The tile number

/* Return the actual value that will be poked into screen memory
 * given an RGB value.  The value is padded with zeros up to a
 * UDWORD but is based on the bit depth of the screen mode.
 */
extern UDWORD screenGetCacheColour(UBYTE red, UBYTE green, UBYTE blue);

/* Set the colour for drawing lines.
 *
 * This caches a colour value that can be poked directly into the
 * current screen mode's video memory.  There is some overhead to this
 * call so all lines of the same colour should be drawn at the same time.
 */
extern void screenSetLineColour(UBYTE red, UBYTE green, UBYTE blue);

/* Set the value to be poked into screen memory for line drawing.
 * The colour value used should be one returned by screenGetCacheColour.
 */
extern void screenSetLineCacheColour(UDWORD colour);

/* Draw a line to the display in the current colour. */
extern void screenDrawLine(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1);

/* Draw a rectangle (unfilled) */
extern void screenDrawRect(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1);

/* Set the colour for fill operations */
extern void screenSetFillColour(UBYTE red, UBYTE green, UBYTE blue);

/* Set the value to be poked into screen memory for filling.
 * The colour value used should be one returned by screenGetCacheColour.
 */
extern void screenSetFillCacheColour(UDWORD colour);

/* Draw a filled rectangle */
extern void screenFillRect(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1);

/* Draw an ellipse, and therefore circles too by specifying bounding box */
/* x0,y0 - top left, x1,y1 - bottom right */
extern void screenDrawEllipse(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1);

extern BOOL screenInitialiseGlide(UDWORD	width, UDWORD height, HANDLE hWindow);

extern BOOL screenReInit( void );

#endif

