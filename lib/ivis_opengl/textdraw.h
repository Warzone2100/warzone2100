	/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

#include <string>

#include "lib/framework/vector.h"
#include "lib/framework/opengl.h"
#include "pietypes.h"

enum iV_fonts
{
	font_regular,
	font_large,
	font_medium,
	font_small,
	font_bar,
	font_scaled,
	font_count
};

class WzText
{
public:
	WzText() {}
	WzText(const std::string &text, iV_fonts fontID);
	void setText(const std::string &text, iV_fonts fontID);
	~WzText();
	int width() { return dimensions.x; }
	int height() { return dimensions.y; }
	void render(Vector2i position, PIELIGHT colour, float rotation = 0.0f);
	void render(int x, int y, PIELIGHT colour, float rotation = 0.0f) { render(Vector2i{x,y}, colour, rotation); }
	int aboveBase() { return mAboveBase; }
	int belowBase() { return mBelowBase; }
	int lineSize() { return mLineSize; }

private:
	iV_fonts mFontID = font_count;
	std::string mText;
	GLuint texture = 0;
	int mAboveBase = 0;
	int mBelowBase = 0;
	int mLineSize = 0;
	Vector2i offsets;
	Vector2i dimensions;
};

void iV_TextInit();
void iV_TextShutdown();
void iV_font(const char *fontName, const char *fontFace, const char *fontFaceBold);

int iV_GetTextAboveBase(iV_fonts fontID);
int iV_GetTextBelowBase(iV_fonts fontID);
int iV_GetTextLineSize(iV_fonts fontID);
unsigned int iV_GetTextWidth(const char *String, iV_fonts fontID);
unsigned int iV_GetCountedTextWidth(const char *string, size_t string_length, iV_fonts fontID);
unsigned int iV_GetCharWidth(uint32_t charCode, iV_fonts fontID);

unsigned int iV_GetTextHeight(const char *string, iV_fonts fontID);
void iV_SetTextColour(PIELIGHT colour);

/// Valid values for "Justify" argument of iV_DrawFormattedText().
enum
{
	FTEXT_LEFTJUSTIFY,			// Left justify.
	FTEXT_CENTRE,				// Centre justify.
	FTEXT_RIGHTJUSTIFY,			// Right justify.
};

int iV_DrawFormattedText(const char *String, UDWORD x, UDWORD y, UDWORD Width, UDWORD Justify, iV_fonts fontID);
void iV_DrawTextRotated(const char *string, float x, float y, float rotation, iV_fonts fontID);

/// Draws text with a printf syntax to the screen.
static inline void iV_DrawText(const char *string, float x, float y, iV_fonts fontID)
{
	iV_DrawTextRotated(string, x, y, 0.f, fontID);
}

#endif // _INCLUDED_TEXTDRAW_
