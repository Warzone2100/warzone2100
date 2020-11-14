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
#include "lib/gamelib/gtime.h"
#include "widget.h"
#include "widgint.h"
#include "paragraph.h"
#include "form.h"
#include "tip.h"

#include <algorithm>

class WzCachedText
{
public:
	WzCachedText(std::string text, iV_fonts font, uint32_t cacheDurationMs = 100):
		text(text),
		font(font),
		cacheDurationMs(cacheDurationMs)
	{}

	void tick()
	{
		if (cachedText && cacheExpireAt < realTime)
		{
			cachedText = nullptr;
		}
	}

	WzText *operator ->()
	{
		if (!cachedText)
		{
			cachedText = std::unique_ptr<WzText>(new WzText(text, font));
		}

		cacheExpireAt = realTime + (cacheDurationMs * GAME_TICKS_PER_SEC) / 1000;
		return cachedText.get();
	}

private:
	std::string text;
	iV_fonts font;
	uint32_t cacheDurationMs;
	std::unique_ptr<WzText> cachedText = nullptr;
	uint32_t cacheExpireAt = 0;
};

class ParagraphLine: public WIDGET
{
public:
	ParagraphLine(Paragraph *parent):
		WIDGET(parent, WIDG_UNSPECIFIED_TYPE),
		cachedText("", parent->font)
	{}

	void display(int xOffset, int yOffset) override
	{
		cachedText->render(xOffset + x(), yOffset + y() - cachedText->aboveBase(), getParagraph()->fontColour);
	}

	void setText(std::string const &newText)
	{
		cachedText = WzCachedText(newText, getParagraph()->font);
		setGeometry(x(), y(), iV_GetTextWidth(newText.c_str(), getParagraph()->font), iV_GetTextHeight(newText.c_str(), getParagraph()->font));
	}

	void run(W_CONTEXT *) override
	{
		cachedText.tick();
	}

private:
	WzCachedText cachedText;

	Paragraph *getParagraph()
	{
		return (Paragraph *)parent();
	}
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

	auto aTextLines = iV_FormatText(state.string.toUtf8().c_str(), state.width, FTEXT_LEFTJUSTIFY, font, false);

	int requiredHeight = 0;
	if (!aTextLines.empty())
	{
		requiredHeight = aTextLines.back().offset.y + iV_GetTextLineSize(font);
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

void Paragraph::resizeLines(size_t size)
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
		lines.push_back(new ParagraphLine(this));
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
