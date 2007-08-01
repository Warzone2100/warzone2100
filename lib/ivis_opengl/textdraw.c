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

/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/

#define MAX_IVIS_FONTS	8

typedef struct {
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
void pie_BeginTextRender(SWORD ColourIndex);
void pie_TextRender(IMAGEFILE *ImageFile,UWORD ID,int x,int y);
static void pie_TextRender270( IMAGEFILE *ImageFile, UWORD ImageID, int x, int y );

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
	int Above,Below;
	int Height;
	UWORD Index,c;
	IVIS_FONT *Font;

	assert(NumFonts < MAX_IVIS_FONTS-1);

	Font = &iVFonts[NumFonts];

	Font->FontFile = ImageFile;
	Font->AsciiTable = AsciiTable;
	Font->FontSpaceSize = SpaceSize;
	Font->FontLineSize = 0;
	Font->FontAbove = 0;
	Font->FontBelow = 0;

	// Initialise the font metrics.
	for(c=0; c<256; c++) {
		Index = (UWORD)AsciiTable[c];
		Above = iV_GetImageYOffset(Font->FontFile,Index);
		Below = Above + iV_GetImageHeight(Font->FontFile,Index);

		Height = abs(Above) + abs(Below);

		if(Above  < Font->FontAbove) {
			Font->FontAbove = Above;
		}

		if(Below  > Font->FontBelow) {
			Font->FontBelow = Below;
		}

		if(Height > Font->FontLineSize) {
			Font->FontLineSize = Height;
		}
	}

	ActiveFontID = NumFonts;

	NumFonts++;

	return NumFonts-1;
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


unsigned int iV_GetCharWidth(const char Char)
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
static UDWORD FFlags = FTEXTF_SKIP_TRAILING_SPACES;
static int RecordExtents = EXTENTS_NONE;
static int ExtentsStartX;
static int ExtentsStartY;
static int ExtentsEndX;
static int ExtentsEndY;

void pie_SetFormattedTextFlags(UDWORD Flags)
{
	FFlags = Flags;
}

UDWORD pie_GetFormattedTextFlags(void)
{
	return FFlags;
}

#define EXTENTS_USEMAXWIDTH (0)
#define EXTENTS_USELASTX (1)
//UBYTE ExtentsMode=EXTENTS_USEMAXWIDTH;

UBYTE ExtentsMode=EXTENTS_USEMAXWIDTH;


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
UDWORD pie_DrawFormattedText(const char* String, UDWORD x, UDWORD y, UDWORD Width, UDWORD Justify)
{
	int i;
	int jx = x;		// Default to left justify.
	int jy = y;
	UDWORD WWidth;
	BOOL GotSpace;
	BOOL NewLine;
	BOOL AddLeadingSpace = FALSE;
	int TWidth;

	const char* curChar = String;
	const char* osiChar;

//	DBPRINTF(("[%s] @(%d,%d) extentsmode=%d just=%d\n",String,x,y,ExtentsMode,Justify));

	curChar = String;
	while (*curChar != 0)
	{
		// Remove leading spaces, usefull when doing center alignment.
		if(FFlags & FTEXTF_SKIP_LEADING_SPACES)
		{
			while(*curChar == ' ')
			{
				++curChar;
			}
		}

		FString[0] = 0;

		WWidth = 0;
		AddLeadingSpace = FALSE;

		GotSpace = FALSE;
		NewLine = FALSE;

		// Parse through the string, adding words until width is achieved.
		while (*curChar != 0 && WWidth <= Width && !NewLine)
		{
			osiChar = curChar;

			// Get the next word.
   			i = 0;
#ifdef TESTBED
			memset(FWord,0,256);
#endif
			if (AddLeadingSpace)
			{
				WWidth += iV_GetCharWidth(' ');
				if(WWidth <= Width)
				{
					FWord[i] = ' ';
					++i;
					GotSpace = TRUE;
					AddLeadingSpace = FALSE;
				}
			}

			while (*curChar != 0 && *curChar != ' ' && WWidth <= Width)
			{
				// Check for new line character.
				if(*curChar == ASCII_COLOURMODE) // If it's a colour mode toggle char then just add it to the word.
				{
					FWord[i] = *curChar;
					++i;
					++curChar;
				} else {
					// Update this lines pixel width.
					WWidth += iV_GetCharWidth(*curChar);

					// If width ok then add this character to the current word.
					if(WWidth <= Width)
					{
						FWord[i] = *curChar;
						++i;
						++curChar;
					}
				}
   			}

   			// Don't forget the space.
			if(*curChar == ' ')
			{
   				WWidth += iV_GetCharWidth(' ');
   				if(WWidth <= Width) {
					FWord[i] = ' ';
					++i;
					++curChar;
					GotSpace = TRUE;
				}
			}

			// If we've passed a space and the word goes past the width then rewind
			// to that space and finish this line.
			if(GotSpace) {
				if (WWidth >= Width)
				{
					if(FWord[i-1] == ' ') {
						FWord[i] = 0;
					} else {
						curChar = osiChar;
						break;
					}
				}
			}

			// Terminate the word.
			FWord[i] = 0;

			// And add it to the output string.
			strcat(FString, FWord);
   		}


		// Remove trailing spaces, useful when doing center alignment.
		if(FFlags & FTEXTF_SKIP_TRAILING_SPACES)
		{
			char* curSpaceChar = &FString[strlen(FString) - 1];
			while (curSpaceChar != &FString[-1] && *curSpaceChar != ASCII_SPACE)
			{
				*(curSpaceChar--) = 0;
			}
		}

#ifdef _TESTBED
		// Replace spaces with ~.
		for (t = 0; t < strlen(FString); t++) {
			if(FString[t] == ' ') FString[t] = '~';
		}
#endif

		TWidth = iV_GetTextWidth(FString);


//		DBPRINTF(("string[%s] is %d of %d pixels wide (according to DrawFormattedText)\n",FString,TWidth,Width));

		// Do justify.
		switch(Justify)
		{
			case FTEXT_CENTRE:
				jx = x + (Width-TWidth)/2;
				break;

			case FTEXT_RIGHTJUSTIFY:

				jx = x + Width-TWidth;
				break;

			case FTEXT_LEFTJUSTIFY:
				jx = x;
				break;
		}

		// draw the text.
		pie_DrawText(FString, jx, jy);


//DBPRINTF(("[%s] @ %d,%d\n",FString,jx,jy));

/* callback type for resload display callback*/
		// remember where we were..
		LastX = jx + TWidth;
		LastY = jy;
		LastTWidth = TWidth;


		if (ExtentsMode==EXTENTS_USELASTX)
		{
			if (RecordExtents==EXTENTS_START)
			{
//
				ExtentsStartY = y + iV_GetTextAboveBase();
				ExtentsEndY = jy - iV_GetTextLineSize()+iV_GetTextBelowBase();

				RecordExtents = EXTENTS_END;
				ExtentsStartX=jx;
				ExtentsEndX=LastX;
			}
			else
			{
				if (jx<ExtentsStartX) ExtentsStartX=jx;

				if (LastX>ExtentsEndX) ExtentsEndX=LastX;
			}

//			DBPRINTF(("extentsstartx = %d extentsendx=%d\n",ExtentsStartX,ExtentsEndX));
		}


		// and move down a line.
		jy += iV_GetTextLineSize();
	}

	if(RecordExtents == EXTENTS_START) {
		RecordExtents = EXTENTS_END;

		ExtentsStartY = y + iV_GetTextAboveBase();
		ExtentsEndY = jy - iV_GetTextLineSize()+iV_GetTextBelowBase();

		if (ExtentsMode==EXTENTS_USEMAXWIDTH)
		{
			ExtentsStartX = x;	// Was jx, but this broke the console centre justified text background.
//			ExtentsEndX = jx + TWidth;
			ExtentsEndX = x + Width;

		}
		else
		{
			if (jx<ExtentsStartX) ExtentsStartX=jx;
			if (LastX>ExtentsEndX) ExtentsEndX=LastX;

		}


	} else if(RecordExtents == EXTENTS_END) {
		ExtentsEndY = jy - iV_GetTextLineSize()+iV_GetTextBelowBase();

		if (ExtentsMode==EXTENTS_USEMAXWIDTH)
		{
			ExtentsEndX = x + Width;
		}

	}

	return jy;
}



static SWORD OldTextColourIndex = -1;

void pie_DrawText(const char *string, UDWORD x, UDWORD y)
{
	IVIS_FONT *Font = &iVFonts[ActiveFontID];

	/* Colour selection */
	pie_BeginTextRender(Font->FontColourIndex);

	while (*string != 0)
	{
		unsigned int Index = (unsigned char)*string;

		// Toggle colour mode?
		if(Index == ASCII_COLOURMODE)
		{
			if (TextColourIndex >= 0)
			{
				OldTextColourIndex = TextColourIndex;
				TextColourIndex = -1;
			}
			else
			{
				if(OldTextColourIndex >= 0)
				{
					TextColourIndex = OldTextColourIndex;
				}
			}
		}
		else if(Index == ASCII_SPACE)
		{
			x += Font->FontSpaceSize;
		}
		else
		{
			UWORD ImageID = Font->AsciiTable[Index];
			pie_TextRender(Font->FontFile, ImageID, x, y);
			x += iV_GetImageWidth(Font->FontFile, ImageID) + 1;
		}

		// Don't use this any more, If the text needs to wrap then use
		// pie_DrawFormattedText() defined above.
		/* New bit to make text wrap */
		if (Font->FontSpaceSize > (pie_GetVideoBufferWidth() - x))
		{
			/* Drop it to the next line if we hit screen edge */
			x = 0;
			y += iV_GetTextLineSize();
		}

		++string;
	}
}


void pie_DrawText270(const char *String, int XPos, int YPos)
{
	IVIS_FONT *Font = &iVFonts[ActiveFontID];

	YPos += iV_GetImageWidth(Font->FontFile, Font->AsciiTable[33]) + 1;

	pie_BeginTextRender(Font->FontColourIndex);

	while (*String != 0)
	{
		unsigned int Index = (unsigned char)*String;

		if (Index != ASCII_SPACE)
		{
			UWORD ImageID = Font->AsciiTable[Index];
			pie_TextRender270(Font->FontFile,ImageID,XPos,YPos);

			YPos -= (iV_GetImageWidth(Font->FontFile, ImageID) + 1);
		}
		else
		{
			YPos -= Font->FontSpaceSize;
		}

		++String;
	}
}

void pie_BeginTextRender(SWORD ColourIndex)
{
	TextColourIndex = ColourIndex;
	pie_SetRendMode(REND_TEXT);
	pie_SetBilinear(FALSE);
}

void pie_TextRender(IMAGEFILE *ImageFile,UWORD ID,int x,int y)
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
			Green= psPalette[TextColourIndex].g;
			Blue = psPalette[TextColourIndex].b;
			pie_SetColour(((Alpha<<24) | (Red<<16) | (Green<<8) | Blue));
		}
	}
	pie_SetColourKeyedBlack(TRUE);
	pie_DrawImageFileID(ImageFile,ID,x,y);
	pie_SetColourKeyedBlack(FALSE);
}

static void pie_TextRender270(IMAGEFILE *ImageFile, UWORD ImageID, int x, int y)
{
	UDWORD Red;
	UDWORD Green;
	UDWORD Blue;
	UDWORD Alpha = MAX_UB_LIGHT;
	IMAGEDEF *Image;
	PIEIMAGE pieImage;
	PIERECT dest;
	iColour* psPalette;

	Image = &(ImageFile->ImageDefs[ImageID]);

	//not coloured yet
	if (TextColourIndex == PIE_TEXT_WHITE)
	{
		pie_SetColour(PIE_TEXT_WHITE_COLOUR & 0x80ffffff);//special case semi transparent for rotated text
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
		Green= psPalette[TextColourIndex].g;
		Blue = psPalette[TextColourIndex].b;
		pie_SetColour(((Alpha<<24) | (Red<<16) | (Green<<8) | Blue));
	}

	pie_SetRendMode(REND_ALPHA_TEXT);
	pieImage.texPage = ImageFile->TPageIDs[Image->TPageID];
	pieImage.tu = Image->Tu;
	pieImage.tv = Image->Tv;
	pieImage.tw = Image->Width;
	pieImage.th = Image->Height;
	dest.x = x + Image->YOffset;
	dest.y = y + Image->XOffset - Image->Width;
	dest.w = Image->Width;
	dest.h = Image->Height;

	pie_SetColourKeyedBlack(TRUE);
	pie_DrawImage270( &pieImage, &dest );
	pie_SetColourKeyedBlack(FALSE);
}
