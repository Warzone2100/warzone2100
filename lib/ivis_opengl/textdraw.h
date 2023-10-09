/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
#include <vector>

#include "lib/framework/vector.h"
#include "lib/framework/wzstring.h"
#include "gfx_api.h"
#include "pietypes.h"

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

enum iV_fonts
{
	font_regular,
	font_large,
	font_medium,
	font_small,
	font_bar,
	font_scaled,
	font_regular_bold,
	font_medium_bold,
	font_count
};

class WzText
{
public:
	WzText() {}
	WzText(const WzString &text, iV_fonts fontID);
	void setText(const WzString &text, iV_fonts fontID/*, bool delayRender = false*/);
	~WzText();
	// Width (in points)
	int width();
	// Height (in points)
	int height();
	void render(Vector2f position, PIELIGHT colour, float rotation = 0.0f, int maxWidth = -1, int maxHeight = -1);
	void render(float x, float y, PIELIGHT colour, float rotation = 0.0f, int maxWidth = -1, int maxHeight = -1) { render(Vector2f{x,y}, colour, rotation, maxWidth, maxHeight); }
	void renderOutlined(int x, int y, PIELIGHT colour, PIELIGHT outlineColour);
	int aboveBase(); // (in points)
	int belowBase(); // (in points)
	int lineSize(); // (in points)

public:
	WzText(const WzText& other) = delete; // non-copyable
	WzText& operator=(const WzText&) = delete; // non-copyable
	WzText& operator=(WzText&& other);
	WzText(WzText&& other);

public:
	const WzString& getText() const { return mText; }
	iV_fonts getFontID() const { return mFontID; }

private:
	void drawAndCacheText(const WzString &text, iV_fonts fontID);
	void redrawAndCacheText();
	void updateCacheIfNecessary();
private:
	WzString mText;
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
	WzString mFullText;
	size_t mLimitWidthPts = 0;

public:
	// Sets the text, truncating to a desired width limit (in *points*) if needed
	// returns: the length of the string that will be drawn (may be less than the input text.length() if truncated)
	size_t setTruncatableText(const WzString &text, iV_fonts fontID, size_t limitWidthInPoints);
};

/**
 Initialize the text rendering subsystem.

 The scale factors are used to scale the rendering / rasterization of the text to a higher DPI.
 It is expected that they will be >= 1.0.

 @param horizScalePercentage The new horizontal DPI scale percentage (100, 125, 150, 200, etc).
 @param vertScalePercentage The new vertical DPI scale percentage (100, 125, 150, 200, etc).
 */
void iV_TextInit(unsigned int horizScalePercentage, unsigned int vertScalePercentage);

/**
 Reinitializes the text rendering subsystem with a new horizontal & vertical scale factor.

 (See iv_TextInit for more details.)

 This function is useful if the DPI used for rendering / rasterization of the text must change.

 Keep in mind that this function merely reinitializes the text rendering subsystem - if any
 prior rendering output of the text rendering subsystem is stored or cached, it may be desirable
 to re-render that text once the text subsystem has reinitialized.
 (WzText instances handle run-time changes of the text rendering scale factor automatically.)

 @param horizScalePercentage The new horizontal DPI scale percentage (100, 125, 150, 200, etc).
 @param vertScalePercentage The new vertical DPI scale percentage (100, 125, 150, 200, etc).
 */
void iV_TextUpdateScaleFactor(unsigned int horizScalePercentage, unsigned int vertScalePercentage);
void iV_TextShutdown();
void iV_font(const char *fontName, const char *fontFace, const char *fontFaceBold);

int iV_GetEllipsisWidth(iV_fonts fontID);
void iV_DrawEllipsis(iV_fonts fontID, Vector2f position, PIELIGHT colour, float rotation = 0.0f);

int iV_GetTextAboveBase(iV_fonts fontID);
int iV_GetTextBelowBase(iV_fonts fontID);
int iV_GetTextLineSize(iV_fonts fontID);
unsigned int iV_GetTextWidth(const WzString& String, iV_fonts fontID);
unsigned int iV_GetCountedTextWidth(const char *string, size_t string_length, iV_fonts fontID);
unsigned int iV_GetCharWidth(uint32_t charCode, iV_fonts fontID);

unsigned int iV_GetTextHeight(const char *string, iV_fonts fontID);
void iV_SetTextColour(PIELIGHT colour);

optional<iV_fonts> iV_ShrinkFont(iV_fonts fontID);

/// Valid values for "Justify" argument of iV_FormatText().
enum
{
	FTEXT_LEFTJUSTIFY,			// Left justify.
	FTEXT_CENTRE,				// Centre justify.
	FTEXT_RIGHTJUSTIFY,			// Right justify.
};

struct TextLine
{
	std::string text;
	Vector2i dimensions;
	Vector2i offset;
};
std::vector<TextLine> iV_FormatText(const WzString& String, UDWORD MaxWidth, UDWORD Justify, iV_fonts fontID, bool ignoreNewlines = false);
void iV_DrawTextRotated(const char *string, float x, float y, float rotation, iV_fonts fontID);

/// Draws text with a printf syntax to the screen.
static inline void iV_DrawText(const char *string, float x, float y, iV_fonts fontID)
{
	iV_DrawTextRotated(string, x, y, 0.f, fontID);
}

#endif // _INCLUDED_TEXTDRAW_
