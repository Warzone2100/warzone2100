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

#include <stdlib.h>
#include <string.h>
#include "lib/ivis_common/ivisdef.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/pieclip.h"
#include "lib/ivis_common/pieblitfunc.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_common/ivispatch.h"
#include "lib/ivis_common/textdraw.h"
#include "lib/ivis_common/bitimage.h"

#include <GL/gl.h>

/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/

#define MAX_IVIS_FONTS	8

typedef struct
{
	IMAGEFILE *FontFile;	// The image data that contains the font.
//	UWORD FontStartID;		// The image ID of character ASCII 33.
//	UWORD FontEndID;		// The image ID of last character in the font.
	int FontAbove;			// Max pixels above the base line.
	int FontBelow;			// Max pixels below the base line.
	int FontLineSize;		// Pixel spacing used for new lines.
	int FontSpaceSize;		// Pixel spacing used for spaces.
	SWORD FontColourIndex;	// The colour index to use.
//	BOOL	bGameFont;
	UWORD *AsciiTable;
} IVIS_FONT;

/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/

static SWORD TextColourIndex;
static int NumFonts;
static int ActiveFontID;
static IVIS_FONT iVFonts[MAX_IVIS_FONTS];

/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/
static void pie_BeginTextRender(SWORD ColourIndex);
static void pie_TextRender(IMAGEFILE *ImageFile, UWORD ID, int x, int y);

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

void iV_ClearFonts(void)
{
	NumFonts = 0;
	ActiveFontID = -1;
}

// Create a font using an ascii lookup table.
//
// IMAGEFILE *ImageFile		Image file containing the font graphics.
// UWORD *AsciiTable		Array of 256 Ascii to ImageID lookups.
// int SpaceSize			Pixel size of a space.
// BOOL bInGame				Specifies that the font is used in game (WHY?)
//
int iV_CreateFontIndirect(IMAGEFILE *ImageFile, UWORD *AsciiTable, int SpaceSize)
{
	int Above, Below;
	int Height;
	UWORD Index, c;
	IVIS_FONT *Font;

	assert(NumFonts < MAX_IVIS_FONTS - 1);

	Font = &iVFonts[NumFonts];

	Font->FontFile = ImageFile;
	Font->AsciiTable = AsciiTable;
	Font->FontSpaceSize = SpaceSize;
	Font->FontLineSize = 0;
	Font->FontAbove = 0;
	Font->FontBelow = 0;

	// Initialise the font metrics.
	for (c = 0; c < 256; c++)
	{
		Index = (UWORD)AsciiTable[c];
		Above = iV_GetImageYOffset(Font->FontFile, Index);
		Below = Above + iV_GetImageHeight(Font->FontFile, Index);

		Height = abs(Above) + abs(Below);

		if (Above  < Font->FontAbove)
		{
			Font->FontAbove = Above;
		}

		if (Below  > Font->FontBelow)
		{
			Font->FontBelow = Below;
		}

		if (Height > Font->FontLineSize)
		{
			Font->FontLineSize = Height;
		}
	}

	ActiveFontID = NumFonts;

	NumFonts++;

	return NumFonts - 1;
}

void iV_SetFont(int FontID)
{
	assert(FontID < NumFonts);
	ActiveFontID = FontID;
}


int iV_GetTextLineSize(void)
{
	IVIS_FONT *Font = &iVFonts[ActiveFontID];
	return abs(Font->FontAbove) + abs(Font->FontBelow);
}

int iV_GetTextAboveBase(void)
{
	IVIS_FONT *Font = &iVFonts[ActiveFontID];
	return Font->FontAbove;
}

int iV_GetTextBelowBase(void)
{
	IVIS_FONT *Font = &iVFonts[ActiveFontID];
	return Font->FontBelow;
}



unsigned int iV_GetTextWidth(const char *String)
{
	unsigned int width = 0;
	while (*String != 0)
	{
		width += iV_GetCharWidth(*(String++));
	}

	return width;
}


unsigned int iV_GetCharWidth(char Char)
{
	UWORD ImageID;
	IVIS_FONT* Font = &iVFonts[ActiveFontID];

	if (Char == ASCII_COLOURMODE)
		return 0;

	if (Char == ASCII_SPACE)
		return Font->FontSpaceSize;

	ImageID = Font->AsciiTable[(unsigned char)Char];
	return iV_GetImageWidth(Font->FontFile, ImageID) + 1;
}

void iV_SetTextColour(SWORD Index)
{
	IVIS_FONT *Font = &iVFonts[ActiveFontID];
	Font->FontColourIndex = Index;
}

// --------------------------------------------------------------------------

enum {
	EXTENTS_NONE,
	EXTENTS_START,
	EXTENTS_END
};

static char FString[256];		// Must do something about these wastefull static arrays.
static char FWord[256];
static int LastX;				// Cursor position after last draw.
static int LastY;
static int LastTWidth;			// Pixel width of the last string draw.
static int RecordExtents = EXTENTS_NONE;
static int ExtentsStartX;
static int ExtentsStartY;
static int ExtentsEndX;
static int ExtentsEndY;

// Draws formatted text with word wrap, long word splitting, embedded
// newlines ( uses @ rather than \n ) and colour mode toggle ( # ) which enables
// or disables font colouring.
//
//	UBYTE *String		The string to display.
//	UDWORD x			x coord of top left of formatted text window.
//	UDWORD y			y coord of top left of formatted text window.
//	UDWORD Width		Width of formatted text window.
//	UDWORD Justify		Justify mode, one of the following:
//							FTEXT_LEFTJUSTIFY
//							FTEXT_CENTRE
//							FTEXT_RIGHTJUSTIFY
//	BOOL DrawBack		If TRUE then draws transparent box behind text.
//
// Returns y coord of next text line.
//
UDWORD iV_DrawFormattedText(const char* String, UDWORD x, UDWORD y, UDWORD Width, UDWORD Justify)
{
	int i;
	int jx = x;		// Default to left justify.
	int jy = y;
	UDWORD WWidth;
	int TWidth;

	const char* curChar = String;

//	DBPRINTF(("[%s] @(%d,%d) extentsmode=%d just=%d\n",String,x,y,ExtentsMode,Justify));

	curChar = String;
	while (*curChar != 0)
	{
		char* curSpaceChar;

		bool GotSpace = false;
		bool NewLine = false;

		// Reset text draw buffer
		FString[0] = 0;

		WWidth = 0;

		// Parse through the string, adding words until width is achieved.
		while (*curChar != 0 && WWidth < Width && !NewLine)
		{
			const char* startOfWord = curChar;

			// Get the next word.
			i = 0;
			for (; *curChar != 0
			    && *curChar != ASCII_SPACE
			    && *curChar != ASCII_NEWLINE;
			     ++i, ++curChar)
			{
				if (*curChar == ASCII_COLOURMODE) // If it's a colour mode toggle char then just add it to the word.
				{
					FWord[i] = *curChar;

					// this character won't be drawn so don't deal with its width
					continue;
				}

				// Update this lines pixel width.
				WWidth += iV_GetCharWidth(*curChar);

				// If this word doesn't fit on the current line then break out
				if (WWidth > Width)
					break;

				// If width ok then add this character to the current word.
				FWord[i] = *curChar;
			}

			// Don't forget the space.
			if (*curChar == ASCII_SPACE)
			{
				WWidth += iV_GetCharWidth(' ');
				if (WWidth <= Width)
				{
					FWord[i] = ' ';
					++i;
					++curChar;
					GotSpace = true;
				}
			}
			// Check for new line character.
			else if (*curChar == ASCII_NEWLINE)
			{
				NewLine = true;
				++curChar;
			}

			// If we've passed a space on this line and the word goes past the
			// maximum width and this isn't caused by the appended space then
			// rewind to the start of this word and finish this line.
			if (GotSpace
			 && WWidth > Width
			 && FWord[i - 1] != ' ')
			{
				// Skip back to the beginning of this
				// word and draw it on the next line
				curChar = startOfWord;
				break;
			}

			// Terminate the word.
			FWord[i] = 0;

			// And add it to the output string.
			strcat(FString, FWord);
		}


		// Remove trailing spaces, useful when doing center alignment.
		curSpaceChar = &FString[strlen(FString) - 1];
		while (curSpaceChar != &FString[-1] && *curSpaceChar == ASCII_SPACE)
		{
			*(curSpaceChar--) = 0;
		}

		TWidth = iV_GetTextWidth(FString);


//		DBPRINTF(("string[%s] is %d of %d pixels wide (according to DrawFormattedText)\n",FString,TWidth,Width));

		// Do justify.
		switch (Justify)
		{
			case FTEXT_CENTRE:
				jx = x + (Width - TWidth) / 2;
				break;

			case FTEXT_RIGHTJUSTIFY:

				jx = x + Width - TWidth;
				break;

			case FTEXT_LEFTJUSTIFY:
				jx = x;
				break;
		}

		// draw the text.
		iV_DrawText(FString, jx, jy);


//DBPRINTF(("[%s] @ %d,%d\n",FString,jx,jy));

		/* callback type for resload display callback*/
		// remember where we were..
		LastX = jx + TWidth;
		LastY = jy;
		LastTWidth = TWidth;

		// and move down a line.
		jy += iV_GetTextLineSize();
	}

	if (RecordExtents == EXTENTS_START)
	{
		RecordExtents = EXTENTS_END;

		ExtentsStartY = y + iV_GetTextAboveBase();
		ExtentsEndY = jy - iV_GetTextLineSize() + iV_GetTextBelowBase();

		ExtentsStartX = x;	// Was jx, but this broke the console centre justified text background.
//		ExtentsEndX = jx + TWidth;
		ExtentsEndX = x + Width;
	}
	else if (RecordExtents == EXTENTS_END)
	{
		ExtentsEndY = jy - iV_GetTextLineSize() + iV_GetTextBelowBase();

		ExtentsEndX = x + Width;
	}

	return jy;
}



void iV_DrawTextRotated(const char* string, float x, float y, float rotation)
{
	IVIS_FONT *Font = &iVFonts[ActiveFontID];

	// Move to the correct position and rotation for text rendering
	glTranslatef(x, y, 0.f);
	glRotatef(rotation, 0.f, 0.f, 1.f);

	/* Colour selection */
	pie_BeginTextRender(Font->FontColourIndex);

	for (unsigned int curX = 0; *string != 0; ++string)
	{
		unsigned int Index = (unsigned char)*string;
		UWORD ImageID;

		// Toggle colour mode?
		if (Index == ASCII_COLOURMODE)
		{
			static SWORD OldTextColourIndex = -1;

			if (TextColourIndex >= 0)
			{
				OldTextColourIndex = TextColourIndex;
				TextColourIndex = -1;
			}
			else
			{
				if (OldTextColourIndex >= 0)
				{
					TextColourIndex = OldTextColourIndex;
				}
			}

			// Don't draw this character
			continue;
		}
		else if (Index == ASCII_SPACE)
		{
			curX += Font->FontSpaceSize;

			// Don't draw this character
			continue;
		}

		// Draw the character
		ImageID = Font->AsciiTable[Index];
		pie_TextRender(Font->FontFile, ImageID, curX, 0);

		// Advance the drawing position
		curX += iV_GetImageWidth(Font->FontFile, ImageID) + 1;
	}

	// Reset the tranlation matrix
	glLoadIdentity();
}

void pie_BeginTextRender(SWORD ColourIndex)
{
	TextColourIndex = ColourIndex;
	pie_SetRendMode(REND_TEXT);
	pie_SetBilinear(FALSE);
}

#define PIE_TEXT_WHITE_COLOUR		(0xffffffff)
#define PIE_TEXT_LIGHTBLUE_COLOUR	(0xffa0a0ff)
#define PIE_TEXT_DARKBLUE_COLOUR	(0xff6060c0)

static void pie_TextRender(IMAGEFILE *ImageFile, UWORD ID, int x, int y)
{
	UDWORD Red;
	UDWORD Green;
	UDWORD Blue;
	UDWORD Alpha = MAX_UB_LIGHT;
	iColour* psPalette;


	if (TextColourIndex == PIE_TEXT_WHITE
	 || TextColourIndex == 255)
	{
		pie_SetColour(MAX_LIGHT);
	}
	else
	{
		if (TextColourIndex == PIE_TEXT_WHITE)
		{
			pie_SetColour(PIE_TEXT_WHITE_COLOUR);
		}
		else if (TextColourIndex == PIE_TEXT_LIGHTBLUE)
		{
			pie_SetColour(PIE_TEXT_LIGHTBLUE_COLOUR);
		}
		else if (TextColourIndex == PIE_TEXT_DARKBLUE)
		{
			pie_SetColour(PIE_TEXT_DARKBLUE_COLOUR);
		}
		else
		{
			psPalette = pie_GetGamePal();
			Red  = psPalette[TextColourIndex].r;
			Green = psPalette[TextColourIndex].g;
			Blue = psPalette[TextColourIndex].b;
			pie_SetColour(((Alpha << 24) | (Red << 16) | (Green << 8) | Blue));
		}
	}
	pie_SetColourKeyedBlack(TRUE);
	pie_DrawImageFileID(ImageFile, ID, x, y);
	pie_SetColourKeyedBlack(FALSE);
}
