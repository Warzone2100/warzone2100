/*
	This file is part of Warzone 2100.
	Copyright (C) 2020  Warzone 2100 Project

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

#ifndef __INCLUDED_LIB_WIDGET_PARAGRAPH_H__
#define __INCLUDED_LIB_WIDGET_PARAGRAPH_H__

#include "widget.h"
#include "widgbase.h"
#include "lib/ivis_opengl/textdraw.h"
#include <string>

struct ParagraphState {
	int width = 0;
	WzString string;

	inline bool operator==(ParagraphState const & other) const
	{
		return width == other.width && string == other.string;
	}
};

class ParagraphLine;
class Paragraph : public WIDGET
{
public:
	Paragraph(W_INIT const *init);

	WzString getString() const override
	{
		return state.string;
	}

	void setString(WzString newString) override
	{
		state.string = newString;
	}

	void setFontColour(PIELIGHT colour)
	{
		fontColour = colour;
	}

	void geometryChanged() override;
	void displayRecursive(WidgetGraphicsContext const &context) override;

private:
	int textWidth = 0;
	PIELIGHT fontColour;
	std::vector<ParagraphLine *> lines;
	ParagraphState state;
	ParagraphState renderState;

	void updateLayout();
	void resizeLines(uint32_t size);

	friend class ParagraphLine;
};

#endif // __INCLUDED_LIB_WIDGET_PARAGRAPH_H__
