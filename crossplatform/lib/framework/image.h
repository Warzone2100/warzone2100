/*
 * Image.h
 *
 * Prototypes for the image file parsing routines
 */
#ifndef _image_h
#define _image_h

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */
#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif

/* Take a memory buffer that contains a PCX file and convert it
 * to an image buffer and a palette buffer.
 * If the returned palette pointer is NULL a true colour PCX has
 * been loaded.  In this case the image data will be 32 bit true colour.
 */
extern BOOL imageParsePCX(UBYTE			*pFileData,			// Original file
				  UDWORD			fileSize,			// File size
				  UDWORD			*pWidth,			// Image width
				  UDWORD			*pHeight,			// Image height
				  UBYTE			**ppImageData,		// Image data from file
				  PALETTEENTRY	**ppsPalette);		// Palette data from file

/* Take a memory buffer that contains a BMP file and convert it
 * to an image buffer and a palette buffer.
 * If the returned palette pointer is NULL a true colour BMP has
 * been loaded.  In this case the image data will be 32 bit true colour.
 */
extern BOOL imageParseBMP(UBYTE			*pFileData,			// Original file
				   UDWORD			fileSize,			// File size
				   UDWORD			*pWidth,			// Image width
				   UDWORD			*pHeight,			// Image height
				   UBYTE			**ppImageData,		// Image data from file
				   PALETTEENTRY		**ppsPalette);		// Palette data from file


/* Take a memory buffer that contains a image buffer and convert it 
 * to a BMP file. 
 */
extern BOOL imageCreateBMP(UBYTE			*pImageData,		// Original file
					PALETTEENTRY	*pPaletteData,		// Palette data
				   UDWORD			Width,				// Image width
				   UDWORD			Height,				// Image height
				   UBYTE			**ppBMPFile,		// Image data from file
   				   UDWORD			*fileSize);			// Generated BMP File size




#endif

