/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/*! \file font.h
 *  \brief Definitions for the font system.
 */
#ifndef _font_h
#define _font_h

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */
#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif

/*************************************************************************************
 *
 * Definitions for the fixed size font
 *
 */
#define PRINTABLE_START 32
#define PRINTABLE_CHARS 97

#define FONT_WIDTH 8
#define FONT_HEIGHT 14

extern UBYTE aFontData[PRINTABLE_CHARS][FONT_HEIGHT];

/*************************************************************************************
 *
 * Definitions for proportional fonts.
 *
 */

// these are file formats ... and hence can not be altered !!!!!
/* The data for a single proportional font character */
typedef struct _prop_char
{
	/* The number of pixels across the character */
	UWORD	width;
	/* The number of bytes used to store one horizontal line of the character */
	UWORD	pitch;

	/* The pixel data
	 * This is a square array of bytes wide enough to store width bits
	 * and the same height as the font.
	 */
	UBYTE	*pData;
} PROP_CHAR;

/* Store ranges of character codes that are printable */
typedef struct _prop_printable
{
	UWORD		end;		// End of the character code range covered by this struct
	UWORD		offset;		// Amount to subtract from char code if this range
							// is printable
	BOOL		printable;	// Whether the range is printable
} PROP_PRINTABLE;

/* The proportional font data */
typedef struct _prop_font
{
	UWORD		height;			// Number of pixels high
	UWORD		spaceWidth;		// Number of pixels gap to leave for a space
	UWORD		baseLine;		// Position of bottom of letters with no tail
								// i.e. where the line on lined paper would be
								// This is relative to the absolute bottom of all characters.

	UWORD		numOffset;		// Number of PROP_OFFSET stored
	UWORD		numChars;		// Number of PROP_CHARS stored

	PROP_PRINTABLE	*psOffset;
	PROP_CHAR		*psChars;
} PROP_FONT;

/* Set the current font */
extern void fontSet(PROP_FONT *psFont);

/* Get the current font */
extern PROP_FONT *fontGet(void);

/* Set the current font colour */
extern void fontSetColour(UBYTE red, UBYTE green, UBYTE blue);

/* Set the value to be poked into screen memory for font drawing.
 * The colour value used should be one returned by screenGetCacheColour.
 */
extern void fontSetCacheColour(UDWORD colour);

/* Print text in the current font at location x,y */
extern void fontPrint(SDWORD x, SDWORD y, char *pFormat, ...);

/* Directly print a single font character from the PROP_CHAR struct */
extern void fontPrintChar(SDWORD x,SDWORD y, PROP_CHAR *psChar, UDWORD height);

/* Return the pixel width of a string */
extern UDWORD fontPixelWidth(char *pString);

/* Return the index into the PROP_CHAR array for a character code.
 * If the code isn't printable, return 0 (space).
 */
extern UWORD fontGetCharIndex(UWORD code);

/* Save font information into a file buffer */
extern BOOL fontSave(PROP_FONT *psFont, UBYTE **ppFileData, UDWORD *pFileSize);

/* Load in a font file */
extern BOOL fontLoad(UBYTE *pFileData, UDWORD fileSize, PROP_FONT **ppsFont);

/* Release all the memory used by a font */
extern void fontFree(PROP_FONT *psFont);

#endif
