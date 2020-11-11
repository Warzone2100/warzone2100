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

#include "lib/framework/frame.h"
#include "widget.h"
#include "widgint.h"
#include "paragraph.h"
#include "form.h"
#include "tip.h"

#include <algorithm>

class ParagraphLine: public WIDGET
{
public:
	ParagraphLine(W_INIT *init): WIDGET(init, WIDG_UNSPECIFIED_TYPE) {}

	Paragraph *getParagraph()
	{
		return (Paragraph *)parent();
	}

	void display(int xOffset, int yOffset)
	{
		text.render(xOffset + x(), yOffset + y() - text.aboveBase(), getParagraph()->fontColour);
	}

	void setText(std::string const &newText)
	{
		text.setText(newText, font_regular);
		setGeometry(x(), y(), text.width(), text.lineSize());
	}

	WzText text;
};

Paragraph::Paragraph(W_INIT const *init)
	: WIDGET(init, WIDG_UNSPECIFIED_TYPE)
	, fontColour(WZCOL_FORM_TEXT)
{
	state.width = init->width;
}

void Paragraph::updateLayout()
{
	if (state == renderState) {
		return;
	}

	renderState = state;

	auto aTextLines = iV_FormatText(state.string.toUtf8().c_str(), state.width, FTEXT_LEFTJUSTIFY, font_regular, false);

	int requiredHeight = 0;
	if (!aTextLines.empty())
	{
		requiredHeight = aTextLines.back().offset.y + iV_GetTextLineSize(font_regular);
	}

	auto textWidth = 0;
	resizeLines(aTextLines.size());
	auto lineIt = lines.begin();
	for (const auto& line : aTextLines)
	{
		textWidth = std::max(textWidth, line.dimensions.x);
		(*lineIt)->setText(line.text);
		lineIt++;
	}

	auto currentTop = 0;
	for (const auto& line : lines)
	{
		line->setGeometry(0, currentTop, line->width(), line->height());
		currentTop += line->height();
	}

	setGeometry(x(), y(), state.width, requiredHeight);
}

void Paragraph::resizeLines(uint32_t size)
{
	while (lines.size() > size)
	{
		auto last = lines.back();
		lines.pop_back();
		detach(last);
		delete last;
	}

	while (lines.size() < size)
	{
		W_INIT init;
		auto newLine = new ParagraphLine(&init);
		lines.push_back(newLine);
		attach(newLine);
	}
}

void Paragraph::geometryChanged()
{
	state.width = width();
	updateLayout();
}

void Paragraph::displayRecursive(WidgetGraphicsContext const& context)
{
	updateLayout();
	WIDGET::displayRecursive(context);
}
