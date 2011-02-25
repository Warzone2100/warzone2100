/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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

#include "lib/framework/frame.h"
#include "lib/framework/opengl.h"
#include <stdlib.h>
#include <string.h>
#include "lib/framework/string_ext.h"
#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/ivis_opengl/bitimage.h"
#include "src/multiplay.h"

#define ASCII_SPACE			(32)
#define ASCII_NEWLINE			('@')
#define ASCII_COLOURMODE		('#')

/** Draws formatted text with word wrap, long word splitting, embedded newlines
 *  (uses '@' rather than '\n') and colour toggle mode ('#') which enables or
 *  disables font colouring.
 *
 *  @param String   the string to display.
 *  @param x,y      X and Y coordinates of top left of formatted text.
 *  @param width    the maximum width of the formatted text (beyond which line
 *                  wrapping is used).
 *  @param justify  The alignment style to use, which is one of the following:
 *                  FTEXT_LEFTJUSTIFY, FTEXT_CENTRE or FTEXT_RIGHTJUSTIFY.
 *  @return the Y coordinate for the next text line.
 */
int iV_DrawFormattedText(const char* String, UDWORD x, UDWORD y, UDWORD Width, UDWORD Justify)
{
	char FString[256];
	char FWord[256];
	int i;
	int jx = x;		// Default to left justify.
	int jy = y;
	UDWORD WWidth;
	int TWidth;
	const char* curChar = String;

	while (*curChar != 0)
	{
		bool GotSpace = false;
		bool NewLine = false;

		// Reset text draw buffer
		FString[0] = 0;

		WWidth = 0;

		// Parse through the string, adding words until width is achieved.
		while (*curChar != 0 && WWidth < Width && !NewLine)
		{
			const char* startOfWord = curChar;
			const unsigned int FStringWidth = iV_GetTextWidth(FString);

			// Get the next word.
			i = 0;
			for (; *curChar != 0
			    && *curChar != ASCII_SPACE
			    && *curChar != ASCII_NEWLINE
			    && *curChar != '\n';
			     ++i, ++curChar)
			{
				if (*curChar == ASCII_COLOURMODE) // If it's a colour mode toggle char then just add it to the word.
				{
					FWord[i] = *curChar;

					// this character won't be drawn so don't deal with its width
					continue;
				}

				// Update this line's pixel width.
				WWidth = FStringWidth + iV_GetCountedTextWidth(FWord, i + 1);

				// If this word doesn't fit on the current line then break out
				if (WWidth > Width)
				{
					break;
				}

				// If width ok then add this character to the current word.
				FWord[i] = *curChar;
			}

			// Don't forget the space.
			if (*curChar == ASCII_SPACE)
			{
				// Should be a space below, not '-', but need to work around bug in QuesoGLC
				// which was fixed in CVS snapshot as of 2007/10/26, same day as I reported it :) - Per
				WWidth += iV_GetCharWidth('-');
				if (WWidth <= Width)
				{
					FWord[i] = ' ';
					++i;
					++curChar;
					GotSpace = true;
				}
			}
			// Check for new line character.
			else if (*curChar == ASCII_NEWLINE
			      || *curChar == '\n')
			{
				if (!bMultiPlayer)
				{
					NewLine = true;
				}
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
			sstrcat(FString, FWord);
		}


		// Remove trailing spaces, useful when doing center alignment.
		{
			// Find the string length (the "minus one" part
			// guarantees that we get the length of the string, not
			// the buffer size required to contain it).
			size_t len = strnlen1(FString, sizeof(FString)) - 1;

			for (; len != 0; --len)
			{
				// As soon as we encounter a non-space character, break out
				if (FString[len] != ASCII_SPACE)
					break;

				// Cut off the current space character from the string
				FString[len] = '\0';
			}
		}

		TWidth = iV_GetTextWidth(FString);

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

		// and move down a line.
		jy += iV_GetTextLineSize();
	}

	return jy;
}
