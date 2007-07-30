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
#ifndef _INCLUDED_TEXTDRAW_
#define _INCLUDED_TEXTDRAW_

#include "ivisdef.h"
#define iV_DrawText			pie_DrawText
#define iV_DrawText270		pie_DrawText270

#define PIE_TEXT_WHITE				(-1)
#define PIE_TEXT_LIGHTBLUE			(-2)
#define PIE_TEXT_DARKBLUE			(-3)

#define PIE_TEXT_WHITE_COLOUR		(0xffffffff)
#define PIE_TEXT_LIGHTBLUE_COLOUR	(0xffa0a0ff)
#define PIE_TEXT_DARKBLUE_COLOUR	(0xff6060c0)

extern void iV_ClearFonts(void);
extern void iV_SetFont(int FontID);
extern int iV_CreateFontIndirect(IMAGEFILE *ImageFile,UWORD *AsciiTable,int SpaceSize);
extern int iV_CreateFont(IMAGEFILE *ImageFile,UWORD StartID,UWORD EndID,int SpaceSize,BOOL bInGame);
extern void iV_GetTextExtents(char *String,int *Width,int *y0,int *y1);
extern int iV_GetTextAboveBase(void);
extern int iV_GetTextBelowBase(void);
extern int iV_GetTextLineSize(void);
extern unsigned int iV_GetTextWidth(const char* String);
extern unsigned int iV_GetCharWidth(char Char);
extern void iV_SetTextColour(SWORD Index);

#define ASCII_SPACE			(32)
#define ASCII_NEWLINE		('@')
#define ASCII_COLOURMODE	('#')



// Valid values for "Justify" argument of pie_DrawFormattedText().

enum {
	FTEXT_LEFTJUSTIFY,			// Left justify.
	FTEXT_CENTRE,				// Centre justify.
	FTEXT_RIGHTJUSTIFY,			// Right justify.
	FTEXT_LEFTJUSTIFYAPPEND,	// Start from end of last print and then left justify.
};


// Valid values for paramaters for pie_SetFormattedTextFlags().

// Skip leading spaces at the start of each line of text. Improves centre justification
// but may result in unwanted word breaks.
#define	FTEXTF_SKIP_LEADING_SPACES		1
// Skip trailing spaces at the end of each line of text, improves centre justification.
#define	FTEXTF_SKIP_TRAILING_SPACES		2
// Inserts a space before the first word in the string, usefull when use FTEXT_LEFTJUSTIFYAPPEND
#define	FTEXTF_INSERT_SPACE_ON_APPEND	4

extern void pie_SetFormattedTextFlags(UDWORD Flags);
extern UDWORD pie_GetFormattedTextFlags(void);
extern void pie_StartTextExtents(void);
extern void pie_FillTextExtents(int BorderThickness,UBYTE r,UBYTE g,UBYTE b,BOOL Alpha);
extern UDWORD pie_DrawFormattedText(char *String, UDWORD x, UDWORD y, UDWORD Width, UDWORD Justify );

extern void pie_DrawText(const char *string, UDWORD x, UDWORD y);
extern void pie_DrawText270(const char *String, int XPos, int YPos);

typedef void (*RENDERTEXT_CALLBACK)(const char *String, UDWORD X, UDWORD Y);
// routines used for textdraw
void SetIndirectDrawTextCallback(RENDERTEXT_CALLBACK routine);
RENDERTEXT_CALLBACK GetIndirectDrawTextCallback(void);

#endif
