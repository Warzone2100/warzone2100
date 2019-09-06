/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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
#include "gfx_api.h"
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
	void setText(const std::string &text, iV_fonts fontID/*, bool delayRender = false*/);
	~WzText();
	// Width (in points)
	int width();
	// Height (in points)
	int height();
	void render(Vector2i position, PIELIGHT colour, float rotation = 0.0f);
	void render(int x, int y, PIELIGHT colour, float rotation = 0.0f) { render(Vector2i{x,y}, colour, rotation); }
	int aboveBase(); // (in points)
	int belowBase(); // (in points)
	int lineSize(); // (in points)

public:
	WzText(const WzText& other) = delete; // TODO: Make this copyable?
	WzText& operator=(WzText&& other);
	WzText(WzText&& other);

protected:
	const std::string& getText() { return mText; }
	iV_fonts getFontID() { return mFontID; }

private:
	void drawAndCacheText(const std::string &text, iV_fonts fontID);
	void redrawAndCacheText();
	void updateCacheIfNecessary();
private:
	std::string mText;
	gfx_api::texture* texture = nullptr;
	int mPtsAboveBase = 0;
	int mPtsBelowBase = 0;
	int mPtsLineSize = 0;
	Vector2i offsets = Vector2i(0, 0);
	Vector2i dimensions = Vector2i(0, 0);
	float mRenderingHorizScaleFactor = 0.f;
	float mRenderingVertScaleFactor = 0.f;
	iV_fonts mFontID = font_count;
	Vector2i layoutMetrics = Vector2i(0, 0);
};

class WidthLimitedWzText: public WzText
{
private:
	std::string mFullText;
	size_t mLimitWidthPts = 0;

public:
	// Sets the text, truncating to a desired width limit (in *points*) if needed
	// returns: the length of the string that will be drawn (may be less than the input text.length() if truncated)
	size_t setTruncatableText(const std::string &text, iV_fonts fontID, size_t limitWidthInPoints);
};

/**
 Initialize the text rendering subsystem.

 The scale factors are used to scale the rendering / rasterization of the text to a higher DPI.
 It is expected that they will be >= 1.0.

 @param horizScaleFactor The horizontal DPI scale factor.
 @param vertScaleFactor The vertical DPI scale factor.
 */
void iV_TextInit(float horizScaleFactor, float vertScaleFactor);

/**
 Reinitializes the text rendering subsystem with a new horizontal & vertical scale factor.

 (See iv_TextInit for more details.)

 This function is useful if the DPI used for rendering / rasterization of the text must change.

 Keep in mind that this function merely reinitializes the text rendering subsystem - if any
 prior rendering output of the text rendering subsystem is stored or cached, it may be desirable
 to re-render that text once the text subsystem has reinitialized.
 (WzText instances handle run-time changes of the text rendering scale factor automatically.)

 @param horizScaleFactor The new horizontal DPI scale factor.
 @param vertScaleFactor The new vertical DPI scale factor.
 */
void iV_TextUpdateScaleFactor(float horizScaleFactor, float vertScaleFactor);
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
