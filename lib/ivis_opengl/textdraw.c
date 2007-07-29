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



int iV_GetTextWidth(const char *String)
{
	int Index;
	int MaxX = 0;
	UWORD ImageID;
	IVIS_FONT *Font = &iVFonts[ActiveFontID];

	while (*String!=0) {
		Index = (unsigned char)*String;

		if(Index != ASCII_COLOURMODE) {
			if(Index != ASCII_SPACE) {
				ImageID = (UWORD)Font->AsciiTable[Index];
				MaxX += iV_GetImageWidth(Font->FontFile, ImageID) + 1;
			} else {
				MaxX += Font->FontSpaceSize;
			}
		}
		String++;
	}

	return MaxX;
}


int iV_GetCharWidth(const char Char)
{
	int Index;
	UWORD ImageID;
	int Width = 0;
	IVIS_FONT *Font = &iVFonts[ActiveFontID];

	Index = (unsigned char)Char;

	if(Index != ASCII_COLOURMODE) {
		if(Index != ASCII_SPACE) {
			ImageID = (UWORD)Font->AsciiTable[Index];
			Width = iV_GetImageWidth(Font->FontFile, ImageID) + 1;
		} else {
			Width = Font->FontSpaceSize;
		}
	}

	return Width;
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
static UDWORD FFlags = FTEXTF_SKIP_TRAILING_SPACES | FTEXTF_INSERT_SPACE_ON_APPEND;
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

static RENDERTEXT_CALLBACK Indirect_pie_DrawText= pie_DrawText;

void SetIndirectDrawTextCallback( RENDERTEXT_CALLBACK routine)
{
	Indirect_pie_DrawText=routine;
}

RENDERTEXT_CALLBACK GetIndirectDrawTextCallback( void)
{
	return(Indirect_pie_DrawText);
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
//							FTEXT_LEFTJUSTIFYAPPEND
//	BOOL DrawBack		If TRUE then draws transparent box behind text.
//
// NOTE,
//	FTEXT_LEFTJUSTIFYAPPEND should only be used immediatly after calling with FTEXT_LEFTJUSTIFY
//  or FTEXT_LEFTJUSTIFYAPPEND
//
// Returns y coord of next text line.
//
UDWORD pie_DrawFormattedText( char *String, UDWORD x, UDWORD y, UDWORD Width, UDWORD Justify )
{
	int i,osi;
	UDWORD si, Len = strlen(String);
	int jx = x;		// Default to left justify.
	int jy = y;
	UDWORD WWidth;
	BOOL GotSpace;
	BOOL NewLine;
	BOOL AddLeadingSpace = FALSE;
	int t;
	int TWidth;

	si = 0;


//	DBPRINTF(("[%s] @(%d,%d) extentsmode=%d just=%d\n",String,x,y,ExtentsMode,Justify));

	while(si < Len) {
		// Remove leading spaces, usefull when doing centre justify.
		if(FFlags & FTEXTF_SKIP_LEADING_SPACES) {
			while( (si < strlen(String)) && (String[si] == ' ') ) {
				si++;
			}
		}

		FString[0] = 0;

		if(Justify == FTEXT_LEFTJUSTIFYAPPEND) {
			WWidth = LastTWidth;
			if(FFlags & FTEXTF_INSERT_SPACE_ON_APPEND) {
				AddLeadingSpace = TRUE;
			}
		} else {
			WWidth = 0;
			AddLeadingSpace = FALSE;
		}

		GotSpace = FALSE;
		NewLine = FALSE;

		// Parse through the string, adding words until width is achieved.
		while( (si < strlen((char*)String)) && (WWidth <= Width) && (!NewLine)) {
			osi = si;

			// Get the next word.
   			i = 0;
#ifdef TESTBED
			memset(FWord,0,256);
#endif
			if(AddLeadingSpace) {
   				WWidth += iV_GetCharWidth(' ');
   				if(WWidth <= Width) {
					FWord[i] = ' ';
					i++;
					GotSpace = TRUE;
					AddLeadingSpace = FALSE;
				}
			}

			while( (String[si] != 0) && (String[si] != ' ') && (WWidth <= Width)) {
				// Check for new line character.
				if(String[si] == ASCII_COLOURMODE) {	// If it's a colour mode toggle char then just add it to the word.
					FWord[i] = String[si];
   					i++;
   					si++;
				} else {
					// Update this lines pixel width.
					WWidth += iV_GetCharWidth(String[si]);

					// If width ok then add this character to the current word.
					if(WWidth <= Width) {
						FWord[i] = String[si];
	   					i++;
	   					si++;
					}
				}
   			}

   			// Don't forget the space.
   			if(String[si] == ' ') {
   				WWidth += iV_GetCharWidth(' ');
   				if(WWidth <= Width) {
					FWord[i] = ' ';
					i++;
					si++;
					GotSpace = TRUE;
				}
			}

			// If we've passed a space and the word goes past the width then rewind
			// to that space and finish this line.
			if(GotSpace) {
				if( (WWidth >= Width) ) {
					if(FWord[i-1] == ' ') {
						FWord[i] = 0;
					} else {
						si = osi;
						break;
					}
				}
			}

			// Terminate the word.
			FWord[i] = 0;

			// And add it to the output string.
			strcat(FString, FWord);
   		}


		// Remove trailing spaces, usefull when doing centre justify.
		if(FFlags & FTEXTF_SKIP_TRAILING_SPACES) {
			for (t = strlen(FString) - 1; t >= 0; t--) {
				if(FString[t] != ' ') {
					break;
				}
				FString[t] = 0;
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
		switch(Justify) {
			case	FTEXT_CENTRE:
				jx = x + (Width-TWidth)/2;
				break;

			case	FTEXT_RIGHTJUSTIFY:

				jx = x + Width-TWidth;
				break;

			case	FTEXT_LEFTJUSTIFY:
				jx = x;
				break;

			case	FTEXT_LEFTJUSTIFYAPPEND:
				jx = LastX;
				jy = LastY;
				Justify = FTEXT_LEFTJUSTIFY;
				break;
		}

		// draw the text.
		// This is an indirect routine
		if (Indirect_pie_DrawText)
			Indirect_pie_DrawText(FString,jx,jy);


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
	int Index;
	UWORD ImageID;
	IVIS_FONT *Font = &iVFonts[ActiveFontID];

	/* Colour selection */
	pie_BeginTextRender(Font->FontColourIndex);

	while (*string!=0) {
		Index = (unsigned char)*string;

		// Toggle colour mode?
		if(Index == ASCII_COLOURMODE) {
			if(TextColourIndex >= 0) {
				OldTextColourIndex = TextColourIndex;
				TextColourIndex = -1;
			} else {
				if(OldTextColourIndex >= 0) {
					TextColourIndex = OldTextColourIndex;
				}
			}
		} else if(Index == ASCII_SPACE) {
			x += Font->FontSpaceSize;
		} else {
			ImageID = (UWORD)Font->AsciiTable[Index];
			pie_TextRender(Font->FontFile, ImageID, x, y);
			x += iV_GetImageWidth(Font->FontFile, ImageID) + 1;
		}

		// Don't use this any more, If the text needs to wrap then use
		// pie_DrawFormattedText() defined above.
		/* New bit to make text wrap */
		if(x > (pie_GetVideoBufferWidth() - Font->FontSpaceSize) )
		{
			/* Drop it to the next line if we hit screen edge */
			x = 0;
			y += iV_GetTextLineSize();
		}
		string++;
	}
}


void pie_DrawText270(const char *String, int XPos, int YPos)
{
	int Index;
	UWORD ImageID;
	IVIS_FONT *Font = &iVFonts[ActiveFontID];

	YPos += iV_GetImageWidth(Font->FontFile,(UWORD)Font->AsciiTable[33]) + 1;

	pie_BeginTextRender(Font->FontColourIndex);

	while (*String!=0)
	{
		Index = (unsigned char)*String;

		if (Index != ASCII_SPACE) {
			ImageID = (UWORD)Font->AsciiTable[Index];
			pie_TextRender270(Font->FontFile,ImageID,XPos,YPos);

			YPos -= (iV_GetImageWidth(Font->FontFile,ImageID) +1) ;
		} else {
			YPos -= (Font->FontSpaceSize);
		}
		String++;
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


	if ((TextColourIndex == PIE_TEXT_WHITE) || (TextColourIndex == 255)) {
		pie_SetColour(MAX_LIGHT);
	} else {
		if (TextColourIndex == PIE_TEXT_WHITE) {
			pie_SetColour(PIE_TEXT_WHITE_COLOUR);
		} else if (TextColourIndex == PIE_TEXT_LIGHTBLUE) {
			pie_SetColour(PIE_TEXT_LIGHTBLUE_COLOUR);
		} else if (TextColourIndex == PIE_TEXT_DARKBLUE) {
			pie_SetColour(PIE_TEXT_DARKBLUE_COLOUR);
		} else {
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
	if (TextColourIndex == PIE_TEXT_WHITE) {
		pie_SetColour(PIE_TEXT_WHITE_COLOUR & 0x80ffffff);//special case semi transparent for rotated text
	} else if (TextColourIndex == PIE_TEXT_LIGHTBLUE) {
		pie_SetColour(PIE_TEXT_LIGHTBLUE_COLOUR);
	} else if (TextColourIndex == PIE_TEXT_DARKBLUE) {
		pie_SetColour(PIE_TEXT_DARKBLUE_COLOUR);
	} else {
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
	pieImage.tw = Image->Width / OLD_TEXTURE_SIZE_FIX;
	pieImage.th = Image->Height / OLD_TEXTURE_SIZE_FIX;
	dest.x = x + Image->YOffset;
	dest.y = y + Image->XOffset - Image->Width;
	dest.w = Image->Width;
	dest.h = Image->Height;

	pie_SetColourKeyedBlack(TRUE);
	pie_DrawImage270( &pieImage, &dest );
	pie_SetColourKeyedBlack(FALSE);
}
