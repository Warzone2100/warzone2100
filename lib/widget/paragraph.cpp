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

#include <algorithm>
#include "lib/framework/frame.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "widget.h"
#include "widgint.h"
#include "paragraph.h"
#include "form.h"
#include "tip.h"

/**
 * Used to render a fragment of text in the paragraph.
 *
 * Can be an entire line, or part of a line, but never more than one line.
 **/
class ParagraphTextWidget: public WIDGET
{
public:
	ParagraphTextWidget(std::string text, ParagraphTextStyle const &textStyle):
		WIDGET(WIDG_UNSPECIFIED_TYPE),
		cachedText(text, textStyle.font),
		textStyle(textStyle)
	{
		setGeometry(x(), y(), cachedText->width(), cachedText->lineSize());
	}

	void display(int xOffset, int yOffset) override
	{
		auto x0 = xOffset + x();
		auto y0 = yOffset + y();
		pie_UniTransBoxFill(x0, y0, x0 + width() - 1, y0 + height() - 1, textStyle.shadeColour);
		cachedText->render(x0, y0 - cachedText->aboveBase(), textStyle.fontColour);
	}

	void run(W_CONTEXT *) override
	{
		cachedText.tick();
	}

private:
	WzCachedText cachedText;
	ParagraphTextStyle textStyle;
};

class FlowLayoutStringDescriptor : public FlowLayoutElementDescriptor
{
    WzString text;
    std::vector<uint32_t> textUtf32;
	iV_fonts font;

public:
    FlowLayoutStringDescriptor(WzString const &newText, iV_fonts newFont): text(newText), textUtf32(newText.toUtf32()), font(newFont) {}

    unsigned int getWidth(size_t position, size_t length) const
    {
        return iV_GetTextWidth(text.substr(position, length).toUtf8().c_str(), font);
    }

    size_t size() const
    {
        return textUtf32.size();
    }

    bool isWhitespace(size_t position) const
    {
		switch (textUtf32[position])
		{
		case ' ':
		case '\t':
			return true;
		default:
			return false;
		}
    }

    bool isLineBreak(size_t position) const
    {
        return textUtf32[position] == '\n';
    }
};

struct ParagraphTextElement: public ParagraphElement
{
	ParagraphTextElement(std::string const &newText, ParagraphTextStyle const &style): style(style)
	{
		text = WzString::fromUtf8(newText);
	}

	void appendTo(FlowLayout &layout) override
	{
		layout.append(FlowLayoutStringDescriptor(text, style.font));
	}

	WIDGET *createFragmentWidget(Paragraph &paragraph, FlowLayoutFragment const &fragment) override
	{
		auto widget = std::make_shared<ParagraphTextWidget>(text.substr(fragment.begin, fragment.length).toUtf8(), style);
		paragraph.attach(widget);
		fragments.push_back(widget);
		return widget.get();
	}

	bool isLayoutDirty() const override
	{
		return false;
	}

	void destroyFragments(Paragraph &paragraph) override
	{
		for (auto fragment: fragments)
		{
			paragraph.detach(fragment);
		}

		fragments.clear();
	}

	int32_t getAboveBase() const override
	{
		return iV_GetTextAboveBase(style.font);
	}

private:
	WzString text;
	ParagraphTextStyle style;
	std::vector<std::shared_ptr<WIDGET>> fragments;
};

struct ParagraphWidgetElement: public ParagraphElement, FlowLayoutElementDescriptor
{
	ParagraphWidgetElement(std::shared_ptr<WIDGET> widget, int32_t aboveBase): widget(widget), aboveBase(aboveBase)
	{
	}

    unsigned int getWidth(size_t position, size_t length) const override
    {
        return position == 0 ? widget->width() : 0;
    }

    size_t size() const override
    {
        return 1;
    }

    bool isWhitespace(size_t position) const override
    {
		return false;
    }

    bool isLineBreak(size_t position) const override
    {
        return false;
    }

	void appendTo(FlowLayout &layout) override
	{
		layout.append(*this);
	}

	WIDGET *createFragmentWidget(Paragraph &paragraph, FlowLayoutFragment const &fragment) override
	{
		layoutWidth = widget->width();
		layoutHeight = widget->height();
		return widget.get();
	}

	void destroyFragments(Paragraph &paragraph) override
	{
	}

	bool isLayoutDirty() const override
	{
		return widget->width() != layoutWidth || widget->height() != layoutHeight;
	}

	int32_t getAboveBase() const override
	{
		return aboveBase;
	}

private:
	std::shared_ptr<WIDGET> widget;
	uint32_t layoutWidth = 0;
	uint32_t layoutHeight = 0;
	int32_t aboveBase;
};

Paragraph::Paragraph(W_INIT const *init): WIDGET(init)
{
}

bool Paragraph::hasElementWithLayoutDirty() const
{
	for (auto &element: elements)
	{
		if (element->isLayoutDirty())
		{
			return true;
		}
	}

	return false;
}

void Paragraph::updateLayout()
{
	if (!layoutDirty && !hasElementWithLayoutDirty()) {
		return;
	}

	layoutDirty = false;
	layoutWidth = width();

	for (auto &element: elements)
	{
		element->destroyFragments(*this);
	}

	auto nextLineOffset = 0;
	auto totalHeight = 0;
	for (const auto& line : calculateLinesLayout())
	{
		std::vector<WIDGET *> lineFragments;
		auto aboveBase = 0;
		auto belowBase = 0;
		for (auto fragmentDescriptor: line)
		{
			auto fragment = elements[fragmentDescriptor.elementId]->createFragmentWidget(*this, fragmentDescriptor);
			auto fragmentAboveBase = -elements[fragmentDescriptor.elementId]->getAboveBase();
			aboveBase = std::max(aboveBase, fragmentAboveBase);
			belowBase = std::max(belowBase, fragment->height() - fragmentAboveBase);
			fragment->setGeometry(fragmentDescriptor.offset, nextLineOffset - fragmentAboveBase, fragment->width(), fragment->height());
			lineFragments.push_back(fragment);
		}

		for (auto fragment: lineFragments)
		{
			fragment->setGeometry(fragment->x(), fragment->y() + aboveBase, fragment->width(), fragment->height());
		}

		totalHeight = nextLineOffset + aboveBase + belowBase;
		nextLineOffset = totalHeight + lineSpacing;
	}

	setGeometry(x(), y(), width(), totalHeight);
}

std::vector<std::vector<FlowLayoutFragment>> Paragraph::calculateLinesLayout()
{
    FlowLayout flowLayout(width());
	for (auto &element: elements)
	{
		element->appendTo(flowLayout);
	}
	flowLayout.end();

	return flowLayout.getLines();
}

void Paragraph::addText(std::string const &text)
{
	layoutDirty = true;
	elements.push_back(std::unique_ptr<ParagraphTextElement>(new ParagraphTextElement(text, textStyle)));
}

void Paragraph::addWidget(std::shared_ptr<WIDGET> widget, int32_t aboveBase)
{
	layoutDirty = true;
	attach(widget);
	elements.push_back(std::unique_ptr<ParagraphWidgetElement>(new ParagraphWidgetElement(widget, aboveBase)));
}

void Paragraph::geometryChanged()
{
	if (layoutWidth != width())
	{
		layoutDirty = true;
	}

	updateLayout();
}

void Paragraph::displayRecursive(WidgetGraphicsContext const& context)
{
	updateLayout();
	WIDGET::displayRecursive(context);
}
