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

#define PIE_TEXT_WHITE				(-1)
#define PIE_TEXT_LIGHTBLUE			(-2)
#define PIE_TEXT_DARKBLUE			(-3)

extern void iV_ClearFonts(void);
extern void iV_SetFont(int FontID);
extern int iV_CreateFontIndirect(IMAGEFILE *ImageFile,UWORD *AsciiTable,int SpaceSize);
extern int iV_GetTextAboveBase(void);
extern int iV_GetTextBelowBase(void);
extern int iV_GetTextLineSize(void);
extern unsigned int iV_GetTextWidth(const char* String);
extern unsigned int iV_GetCharWidth(char Char);
extern void iV_SetTextColour(SWORD Index);

#define ASCII_SPACE			(32)
#define ASCII_NEWLINE		('@')
#define ASCII_COLOURMODE	('#')

// Valid values for "Justify" argument of iV_DrawFormattedText().

enum {
	FTEXT_LEFTJUSTIFY,			// Left justify.
	FTEXT_CENTRE,				// Centre justify.
	FTEXT_RIGHTJUSTIFY,			// Right justify.
};


extern UDWORD iV_DrawFormattedText(const char* String, UDWORD x, UDWORD y, UDWORD Width, UDWORD Justify);

extern void iV_DrawTextRotated(const char* string, float x, float y, float rotation);

static inline void iV_DrawText(const char* string, float x, float y)
{
    iV_DrawTextRotated(string, x, y, 0.f);
}

#endif // _INCLUDED_TEXTDRAW_
