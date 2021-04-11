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
/** @file
 *  Functions for the label widget.
 */

#include "lib/framework/frame.h"
#include "widget.h"
#include "widgint.h"
#include "label.h"
#include "form.h"
#include "tip.h"

#include <algorithm>
#include <sstream>

#define LABEL_DEFAULT_CACHE_EXPIRY 250

W_LABINIT::W_LABINIT()
	: FontID(font_regular)
{}

W_LABEL::W_LABEL(W_LABINIT const *init)
	: WIDGET(init, WIDG_LABEL)
	, FontID(init->FontID)
	, pTip(init->pTip)
	, fontColour(WZCOL_FORM_TEXT)
{
	ASSERT((init->style & ~(WLAB_PLAIN | WLAB_ALIGNLEFT | WLAB_ALIGNRIGHT | WLAB_ALIGNCENTRE | WLAB_ALIGNTOP | WLAB_ALIGNBOTTOM | WIDG_HIDDEN)) == 0, "Unknown button style");
	setString(init->pText);
}

W_LABEL::W_LABEL()
	: WIDGET(WIDG_LABEL)
	, FontID(font_regular)
	, fontColour(WZCOL_FORM_TEXT)
{}

int W_LABEL::setFormattedString(WzString string, uint32_t MaxWidth, iV_fonts fontID, int _lineSpacing /*= 0*/, bool ignoreNewlines /*= false*/)
{
	lineSpacing = _lineSpacing;
	FontID = fontID;
	aTextLines = iV_FormatText(string.toUtf8().c_str(), MaxWidth, FTEXT_LEFTJUSTIFY, fontID, ignoreNewlines);

	int requiredHeight = 0;
	if (!aTextLines.empty())
	{
		requiredHeight = aTextLines.back().offset.y + iV_GetTextLineSize(fontID);
		requiredHeight += ((static_cast<int>(aTextLines.size()) - 1) * lineSpacing);
	}

	maxLineWidth = 0;
	for (const auto& line : aTextLines)
	{
		maxLineWidth = std::max(maxLineWidth, line.dimensions.x);
	}

	displayCache.wzText.clear();
	for (size_t idx = 0; idx < aTextLines.size(); idx++)
	{
		displayCache.wzText.push_back(WzCachedText(aTextLines[idx].text, FontID, LABEL_DEFAULT_CACHE_EXPIRY));
	}

	return requiredHeight;
}

#ifdef DEBUG_BOUNDING_BOXES
# include "lib/ivis_opengl/pieblitfunc.h"
#endif

void W_LABEL::display(int xOffset, int yOffset)
{
	int maxWidth = 0;
	int textTotalHeight = 0;
	for (size_t idx = 0; idx < displayCache.wzText.size(); idx++)
	{
		maxWidth = std::max(maxWidth, displayCache.wzText[idx]->width());
		textTotalHeight += displayCache.wzText[idx]->lineSize();
		if (idx < (displayCache.wzText.size() - 1))
		{
			textTotalHeight += lineSpacing;
		}
	}

	if (displayCache.wzText.empty()) return;

	Vector2i textBoundingBoxOffset(0, 0);
	if ((style & WLAB_ALIGNTOPLEFT) != 0)  // Align top
	{
		textBoundingBoxOffset.y = yOffset + y();
	}
	else if ((style & WLAB_ALIGNBOTTOMLEFT) != 0)  // Align bottom
	{
		textBoundingBoxOffset.y = yOffset + y() + (height() - textTotalHeight);
	}
	else
	{
		textBoundingBoxOffset.y = yOffset + y() + (height() - textTotalHeight) / 2;
	}

	int jy = 0;
	isTruncated = false;
	for (auto& wzTextLine : displayCache.wzText)
	{
		int fx = 0;
		if (style & WLAB_ALIGNCENTRE)
		{
			int fw = wzTextLine->width();
			fx = xOffset + x() + (width() - fw) / 2;
		}
		else if (style & WLAB_ALIGNRIGHT)
		{
			int fw = wzTextLine->width();
			fx = xOffset + x() + width() - fw;
		}
		else
		{
			fx = xOffset + x();
		}


		float fy = float(textBoundingBoxOffset.y) + float(jy) - float(wzTextLine->aboveBase());

#ifdef DEBUG_BOUNDING_BOXES
		// Display bounding boxes.
		PIELIGHT col;
		col.byte.r = 128 + iSinSR(realTime, 2000, 127); col.byte.g = 128 + iSinSR(realTime + 667, 2000, 127); col.byte.b = 128 + iSinSR(realTime + 1333, 2000, 127); col.byte.a = 128;
		iV_Box(textBoundingBoxOffset.x + fx, textBoundingBoxOffset.y + jy + baseLineOffset, textBoundingBoxOffset.x + fx + wzTextLine.width() - 1, textBoundingBoxOffset.y + jy + baseLineOffset + wzTextLine.lineSize() - 1, col);
#endif
		int lineWidthLimit = -1;
		if (canTruncate && (wzTextLine->width() > width()))
		{
			// text would render outside the width of the label, so figure out a maxLineWidth that can be displayed (leaving room for ellipsis)
			lineWidthLimit = width() - iV_GetEllipsisWidth(FontID) - 2;
		}
		wzTextLine->render(textBoundingBoxOffset.x + fx, fy, fontColour, 0.0f, lineWidthLimit);
		if (lineWidthLimit > -1)
		{
			// Render ellipsis
			iV_DrawEllipsis(FontID, Vector2i(textBoundingBoxOffset.x + fx + lineWidthLimit + 2, fy), fontColour);
			isTruncated = true;
		}
		jy += wzTextLine->lineSize() + lineSpacing;
	}
}

std::string W_LABEL::getTip()
{
	if (pTip.empty() && !isTruncated) {
		return "";
	}

	if (!isTruncated) {
		return pTip;
	}

	std::stringstream labelText;
	for (const auto& line : aTextLines)
	{
		labelText << line.text << "\n";
	}

	if (!pTip.empty())
	{
		labelText << "\n(" << pTip << ")";
	}

	return labelText.str();
}

WzString W_LABEL::getString() const
{
	if (aTextLines.empty()) return WzString();
	return WzString::fromUtf8(aTextLines.front().text);
}

void W_LABEL::setString(WzString string)
{
	if ((aTextLines.size() == 1) && (aTextLines.front().text == string.toStdString()))
	{
		// no change - skip
		return;
	}
	aTextLines.clear();
	aTextLines.push_back({string.toStdString(), Vector2i(0,0), Vector2i(0,0)});
	displayCache.wzText.clear();
	displayCache.wzText.push_back(WzCachedText(string.toStdString(), FontID, LABEL_DEFAULT_CACHE_EXPIRY));
	maxLineWidth = -1; // delay calculating line width until it's requested
	dirty = true;
}

void W_LABEL::setTip(std::string string)
{
	pTip = string;
}

void W_LABEL::setTextAlignment(WzTextAlignment align)
{
	style &= ~(WLAB_ALIGNLEFT | WLAB_ALIGNCENTRE | WLAB_ALIGNRIGHT);
	style |= align;
	dirty = true;
}

void W_LABEL::run(W_CONTEXT *)
{
	if (!cacheNeverExpires)
	{
		for (auto& wzTextLine : displayCache.wzText)
		{
			wzTextLine.tick();
		}
	}
}

int W_LABEL::getMaxLineWidth()
{
	if (maxLineWidth > -1) { return maxLineWidth; }
	if (aTextLines.empty()) { return 0; }
	// delayed calculation of maxLineWidth until first requested
	maxLineWidth = iV_GetTextWidth(aTextLines[0].text.c_str(), FontID);
	return maxLineWidth;
}
