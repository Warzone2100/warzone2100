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

#include <string>
#include "lib/ivis_opengl/textdraw.h"
#include "lib/gamelib/gtime.h"
#include "widget.h"
#include "widgbase.h"

class Paragraph;
struct FlowLayout;
struct FlowLayoutFragment;

/**
 * Provide information about an element appended to the paragraph.
 **/
struct ParagraphElement
{
	virtual ~ParagraphElement() = default;

	virtual void appendTo(FlowLayout &layout) = 0;
	virtual std::shared_ptr<WIDGET> createFragmentWidget(Paragraph &paragraph, FlowLayoutFragment const &fragment) = 0;
	virtual void destroyFragments(Paragraph &paragraph) = 0;
	virtual bool isLayoutDirty() const = 0;
	virtual int32_t getAboveBase() const = 0;
};

struct ParagraphTextStyle
{
	iV_fonts font = font_regular;
	PIELIGHT shadeColour = pal_RGBA(0, 0, 0, 0);
	PIELIGHT fontColour = WZCOL_BLACK;
};

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

struct FlowLayoutFragment
{
    size_t elementId;
    size_t begin;
    size_t length;
    unsigned int width;
    unsigned int offset;
};

class Paragraph : public WIDGET
{
public:
	Paragraph(): WIDGET() {}

	void addText(std::string const &text);
	void addWidget(const std::shared_ptr<WIDGET> &widget, int32_t aboveBase);

	void setFont(iV_fonts font)
	{
		textStyle.font = font;
	}

	void setFontColour(PIELIGHT colour)
	{
		textStyle.fontColour = colour;
	}

	void setShadeColour(PIELIGHT colour)
	{
		textStyle.shadeColour = colour;
	}

	void setLineSpacing(uint32_t newLineSpacing)
	{
		lineSpacing = newLineSpacing;
		layoutDirty = true;
	}

	ParagraphTextStyle const &getTextStyle() const
	{
		return textStyle;
	}

	void geometryChanged() override;
	void displayRecursive(WidgetGraphicsContext const &context) override;

private:
	std::vector<std::unique_ptr<ParagraphElement>> elements;
	bool layoutDirty = true;
	uint32_t layoutWidth = 0;
	ParagraphTextStyle textStyle;
	uint32_t lineSpacing = 0;

	bool hasElementWithLayoutDirty() const;
	void updateLayout();
	std::vector<std::vector<FlowLayoutFragment>> calculateLinesLayout();
};

#endif // __INCLUDED_LIB_WIDGET_PARAGRAPH_H__
