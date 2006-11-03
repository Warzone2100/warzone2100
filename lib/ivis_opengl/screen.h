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

#include "lib/framework/types.h"

/* ------------------------------------------------------------------------------------------- */

/* Legacy stuff 
 * - only used in the sequence video code we have not yet decided whether to port or to junk */

void *screenGetSurface(void); /* Return a pointer to the back buffer surface */

/* Set the colour for text */
void screenSetTextColour(UBYTE red, UBYTE green, UBYTE blue);

/* Output text to the display screen at location x,y. The remaining arguments are as printf. */
void screenTextOut(UDWORD x, UDWORD y, char *pFormat, ...);

/* ------------------------------------------------------------------------------------------- */

/* Image structure */

typedef struct {
	unsigned int	width;
	unsigned int	height;
	unsigned int	channels;
	unsigned char*	data;
} pie_image;

BOOL image_init(pie_image* image);
BOOL image_create(pie_image* image,
		  unsigned int width,
		  unsigned int height,
		  unsigned int channels);
BOOL image_delete(pie_image* image);
BOOL image_load_from_jpg(pie_image* image, const char* filename);

/* backDrop */
extern void screen_SetBackDrop(UWORD* newBackDropBmp, UDWORD width, UDWORD height);
extern void screen_SetBackDropFromFile(char* filename);
extern void screen_StopBackDrop(void);
extern void screen_RestartBackDrop(void);
extern BOOL screen_GetBackDrop(void);
extern void screen_Upload(UWORD* newBackDropBmp);

/* screendump */
char* screenDumpToDisk(char* path);

/* Toggle the display between full screen or windowed */
extern void	screenToggleMode(void);

#endif
