/*
 * Surface.h
 *
 * Surface utility function prototypes.
 */
#ifndef _surface_h
#define _surface_h

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */
#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif

/* Create a DD surface.
 *
 * If caps is 0 and psPixFormat is NULL the surface will be created in
 * video memory if there is room (system memory otherwise) and will use
 * the pixel format of the display.
 */
extern BOOL surfCreate(LPDIRECTDRAWSURFACE4	*ppsSurface,	// The created surface
					   UDWORD width, UDWORD height,			// The size of the surface
					   UDWORD				caps,			// The caps bits for the surface
					   DDPIXELFORMAT		*psPixFormat,	// The pixel format for the surface
					   BOOL					bColourKey,		// Colour key flag
					   BOOL					bAddToList   );	// Add pointer to surface list
/* Release a surface.
 * This should be used for all surfaces allocated with surfCreate.
 */
extern void surfRelease(LPDIRECTDRAWSURFACE4	psSurface);

/* Re-create a surface - useful for video surfaces after a mode change */
extern BOOL surfRecreate(LPDIRECTDRAWSURFACE4 *ppsSurface);

/* Load image data into a Direct Draw surface */
extern BOOL surfLoadFrom8Bit(
			LPDIRECTDRAWSURFACE4		psSurf,			// The surface to load to
			UDWORD width, UDWORD height,			// The size of the image data
			UBYTE				*pImageData,		// The image data
			PALETTEENTRY		*psPalette);		// The image palette data

/* Copy the data from one surface to another - useful for loading
 * a video memory surface from a system memory surface.
 */
extern BOOL surfLoadFromSurface(
			LPDIRECTDRAWSURFACE4		psDest,		// The surface to load to
			LPDIRECTDRAWSURFACE4		psSrc);		// The surface to load from

/* Load a BMP file and create a system memory surface to store it in */
extern BOOL surfCreateFromBMP(
				STRING				*pFileName,	// The BMP file
				LPDIRECTDRAWSURFACE4	*ppsSurface,// The created surface
				UDWORD				*pWidth,	// The width of the image
				UDWORD				*pHeight);	// The height of the image

/* set surface colour key */
extern HRESULT DDSetColorKey(IDirectDrawSurface4 *pdds, COLORREF rgb);

#endif

