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
 *  Definitions for the label widget.
 */

#ifndef __INCLUDED_LIB_WIDGET_LABEL_H__
#define __INCLUDED_LIB_WIDGET_LABEL_H__

#include "widget.h"
#include "widgbase.h"
#include "paragraph.h"
#include "lib/ivis_opengl/textdraw.h"
#include <string>

struct LabelDisplayCache {
	std::vector<WzCachedText> wzText;
};

class W_LABEL : public WIDGET
{

public:
	W_LABEL(W_LABINIT const *init);
	W_LABEL();

	void display(int xOffset, int yOffset) override;

	WzString getString() const override;
	void setString(WzString string) override;
	void setTip(std::string string) override;
	std::string getTip() override;

	void run(W_CONTEXT *) override;

	// Sets a string for the label
	// - line-wraps at max width
	// Returns the height of the formatted text
	int setFormattedString(WzString string, uint32_t MaxWidth, iV_fonts fontID, int lineSpacing = 0, bool ignoreNewlines = false);

	void setFont(iV_fonts font)
	{
		FontID = font;
	}
	void setFontColour(PIELIGHT colour)
	{
		fontColour = colour;
	}
	void setFont(iV_fonts font, PIELIGHT colour)
	{
		setFont(font);
		setFontColour(colour);
	}
	void setTextAlignment(WzTextAlignment align);
	void setCanTruncate(bool _canTruncate)
	{
		canTruncate = _canTruncate;
	}

	int getMaxLineWidth();

	void setCacheNeverExpires(bool value)
	{
		cacheNeverExpires = value;
	}

	using WIDGET::setTip;

private:
	std::vector<TextLine> aTextLines;   // text lines on the label
	int maxLineWidth = -1;
	iV_fonts FontID;
	std::string		pTip;          		// The tool tip for the button
	PIELIGHT fontColour;
	LabelDisplayCache displayCache;
	int lineSpacing = 0;
	bool cacheNeverExpires = false;
	bool canTruncate = false;
	bool isTruncated = false;
};

#endif // __INCLUDED_LIB_WIDGET_LABEL_H__
