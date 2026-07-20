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

class FlowLayoutElementDescriptor
{
public:
	virtual ~FlowLayoutElementDescriptor() = default;
	virtual unsigned int getWidth(size_t position, size_t length) const = 0;
	virtual size_t getMaxElementLengthForWidth(size_t position, size_t maxWidth) const = 0;
	virtual size_t size() const = 0;
	virtual bool isWhitespace(size_t position) const = 0;
	virtual bool isLineBreak(size_t position) const = 0;

	virtual bool isWord(size_t position) const
	{
		return position < size() && !isWhitespace(position) && !isLineBreak(position);
	}
};

/**
 * FlowLayout implements a word wrapping algorithm.
 *
 * It doesn't specify how the text should be structured, it only requires that the elements of the text are described
 * by FlowLayoutElementDescriptor.
 *
 * Each element can contain 0 or more words, and a single word can be part of two sequential elements.
 *
 * Example:
 * - Element 1 = "first sec"
 * - Element 2 = "ond third"
 *
 * In the example above, the resulting text is "first second third", so if possible the algorithm will keep
 * the second word completely in the same line.
 **/
struct FlowLayout
{
private:
	bool shouldMergeFragment()
	{
		return !currentLine.empty() && !partialWord.empty() && currentLine.back().elementId == partialWord.front().elementId;
	}

	void endWord()
	{
		if (shouldMergeFragment())
		{
			auto partialWordFront = &partialWord.front();
			auto currentLineBack = currentLine.back();
			currentLine.pop_back();
			partialWordFront->length = partialWordFront->length + partialWordFront->begin - currentLineBack.begin;
			partialWordFront->width = partialWordFront->offset + partialWordFront->width - currentLineBack.offset;
			partialWordFront->begin = currentLineBack.begin;
			partialWordFront->offset = currentLineBack.offset;
		}

		for (auto fragment: partialWord)
		{
			currentLine.push_back(fragment);
		}

		nextOffset += partialWordWidth;
		partialWordWidth = 0;
		partialWord.clear();
	}

	void pushFragment(FlowLayoutElementDescriptor const &elementDescriptor, size_t begin, size_t end)
	{
		auto width = elementDescriptor.getWidth(begin, end - begin);
		partialWord.push_back({currentElementId, begin, end - begin, width, nextOffset + partialWordWidth});
		partialWordWidth += width;
	}

	void placeLine(FlowLayoutElementDescriptor const &elementDescriptor, size_t begin, size_t end)
	{
		auto current = begin;
		size_t elementDescriptorSize = elementDescriptor.size();

		while (current < end)
		{
			auto spaceUsedForCurrentLine = nextOffset + partialWordWidth;
			size_t fitWidth = elementDescriptor.getMaxElementLengthForWidth(current, (spaceUsedForCurrentLine <= maxWidth) ? maxWidth - spaceUsedForCurrentLine : 0);
			size_t fragmentFits = 0;
			if (fitWidth < end - current)
			{
				fragmentFits = current + fitWidth;
			}
			else
			{
				fragmentFits = end;
			}

			size_t whitespacePosition = (fragmentFits >= elementDescriptorSize) ? elementDescriptorSize : fragmentFits + 1;
			while (whitespacePosition > current && !elementDescriptor.isWhitespace(whitespacePosition - 1))
			{
				whitespacePosition--;
			}

			if (whitespacePosition > current)
			// the fragment ending with a whitespace fits within the line
			{
				pushFragment(elementDescriptor, current, whitespacePosition - 1);
				endWord();
				nextOffset += elementDescriptor.getWidth(whitespacePosition - 1, 1);
				current = whitespacePosition;
			}
			else if (fragmentFits == end)
			// the fragment is a single word, and fits in the line,
			// but it doesn't end in whitespace and the next element might be part of this word
			{
				pushFragment(elementDescriptor, current, fragmentFits);
				current = fragmentFits;
			}
			else if (nextOffset > 0)
			// word doesn't fit in the current line
			{
				 breakLine();
			}
			else
			// word doesn't fit in an empty line
			{
				auto fragmentEnd = std::max((size_t)fragmentFits, current + 1);
				pushFragment(elementDescriptor, current, fragmentEnd);
				current = fragmentEnd;
				breakLine();
			}
		}
	}

public:
	FlowLayout(unsigned int maxWidth): maxWidth(maxWidth)
	{

	}

	void append(FlowLayoutElementDescriptor const &elementDescriptor)
	{
		size_t position = 0;
		while (position < elementDescriptor.size())
		{
			auto lineEnd = position;

			while (lineEnd < elementDescriptor.size() && !elementDescriptor.isLineBreak(lineEnd))
			{
				lineEnd++;
			}

			placeLine(elementDescriptor, position, lineEnd);

			if (lineEnd < elementDescriptor.size())
			{
				breakLine();
			}

			position = lineEnd + 1;
		}

		currentElementId++;
	}

	void end()
	{
		endWord();

		if (!currentLine.empty()) {
			breakLine();
		}
	}

	void breakLine()
	{
		endWord();
		lines.push_back(currentLine);
		currentLine.clear();
		nextOffset = 0;
	}

	std::vector<std::vector<FlowLayoutFragment>> getLines()
	{
		return lines;
	}

private:
	std::vector<FlowLayoutFragment> currentLine;
	std::vector<std::vector<FlowLayoutFragment>> lines;
	unsigned int maxWidth;
	unsigned int currentElementId = 0;
	unsigned int nextOffset = 0;
	std::vector<FlowLayoutFragment> partialWord;
	unsigned int partialWordWidth = 0;
};

/**
 * Used to render a fragment of text in the paragraph.
 *
 * Can be an entire line, or part of a line, but never more than one line.
 **/
class ParagraphTextWidget: public WIDGET
{
public:
	ParagraphTextWidget(WzString text, ParagraphTextStyle const &textStyle):
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
		pie_UniTransBoxFill(x0, y0 + 1, x0 + width(), y0 + 1 + height(), textStyle.shadeColour);
		cachedText->render(x0, y0 - cachedText->aboveBase(), textStyle.fontColour);
	}

	void run(W_CONTEXT *) override
	{
		cachedText.tick();
	}

	void setFontColour(PIELIGHT colour)
	{
		textStyle.fontColour = colour;
	}

private:
	WzCachedText cachedText;
	ParagraphTextStyle textStyle;
};

class FlowLayoutStringDescriptor : public FlowLayoutElementDescriptor
{
	WzString text;
	std::vector<uint32_t> textUtf32;
	bool rightToLeft = false;
	iV_fonts font;

public:
	const WzString& getText() const { return text; }

	FlowLayoutStringDescriptor(WzString const &newText, iV_fonts newFont, bool rtl = false): text(newText), textUtf32(newText.toUtf32()), rightToLeft(rtl), font(newFont) {}

	unsigned int getWidth(size_t position, size_t length) const
	{
		return iV_GetTextWidth(text.substr(position, length), font);
	}

	size_t getMaxElementLengthForWidth(size_t position, size_t maxWidth) const
	{
		return iV_GetMaxTextRunLenForWidth(text.substr(position), font, static_cast<uint32_t>(maxWidth), rightToLeft);
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
	ParagraphTextElement(WzString const &newText, ParagraphTextStyle const &style, bool rtl = false): style(style)
	{
		text = newText;
		rightToLeft = rtl;
	}

	void appendTo(FlowLayout &layout) override
	{
		layout.append(FlowLayoutStringDescriptor(text, style.font, rightToLeft));
	}

	std::shared_ptr<WIDGET> createFragmentWidget(Paragraph &paragraph, FlowLayoutFragment const &fragment) override
	{
		auto widget = std::make_shared<ParagraphTextWidget>(text.substr(fragment.begin, fragment.length), style);
		widget->setTransparentToMouse(true);
		paragraph.attach(widget);
		fragments.push_back(widget);
		return widget;
	}

	bool isLayoutDirty() const override
	{
		return false;
	}

	void destroyFragments(Paragraph &paragraph) override
	{
		for (auto& fragment: fragments)
		{
			paragraph.detach(fragment);
		}

		fragments.clear();
	}

	int32_t getAboveBase() const override
	{
		return iV_GetTextAboveBase(style.font);
	}

	void setFontColour(PIELIGHT colour)
	{
		style.fontColour = colour;
		for (auto& fragment: fragments)
		{
			fragment->setFontColour(colour);
		}
	}

private:
	WzString text;
	ParagraphTextStyle style;
	bool rightToLeft = false;
	std::vector<std::shared_ptr<ParagraphTextWidget>> fragments;
};

struct ParagraphWidgetElement: public ParagraphElement, FlowLayoutElementDescriptor
{
	ParagraphWidgetElement(const std::shared_ptr<WIDGET> &widget, int32_t aboveBase): widget(widget), aboveBase(aboveBase)
	{
	}

	unsigned int getWidth(size_t position, size_t length) const override
	{
		return position == 0 ? widget->width() : 0;
	}

	size_t getMaxElementLengthForWidth(size_t position, size_t maxWidth) const override
	{
		if (position == 0)
		{
			if (widget->width() <= maxWidth)
			{
				return size();
			}
			else
			{
				return 0;
			}
		}
		if (position > size())
		{
			return 0;
		}
		return size() - position;
	}

	size_t size() const override
	{
		return 2; // + 1 for "whitespace" at end
	}

	bool isWhitespace(size_t position) const override
	{
		return (position > 0);
	}

	bool isLineBreak(size_t position) const override
	{
		return false;
	}

	void appendTo(FlowLayout &layout) override
	{
		layout.append(*this);
	}

	std::shared_ptr<WIDGET> createFragmentWidget(Paragraph &paragraph, FlowLayoutFragment const &fragment) override
	{
		layoutWidth = widget->width();
		layoutHeight = widget->height();
		return widget;
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
	scrollSnapOffsets.clear();

	auto nextLineOffset = 0;
	auto totalHeight = 0;
	auto linesLayout = calculateLinesLayout();
	scrollSnapOffsets.reserve(linesLayout.size());
	for (const auto& line : linesLayout)
	{
		std::vector<WIDGET *> lineFragments;
		auto aboveBase = 0;
		auto belowBase = 0;

		scrollSnapOffsets.push_back(nextLineOffset);

		for (auto fragmentDescriptor: line)
		{
			auto fragment = elements[fragmentDescriptor.elementId]->createFragmentWidget(*this, fragmentDescriptor);
			auto fragmentAboveBase = -elements[fragmentDescriptor.elementId]->getAboveBase();
			aboveBase = std::max(aboveBase, fragmentAboveBase);
			belowBase = std::max(belowBase, fragment->height() - fragmentAboveBase);
			fragment->setGeometry(
				fragmentDescriptor.offset,
				nextLineOffset - fragmentAboveBase,
				std::min(width(), fragment->width()),
				fragment->height()
			);
			lineFragments.push_back(fragment.get());
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

void Paragraph::addText(const WzString &text)
{
	layoutDirty = true;
	auto textRuns = iV_SplitTextParagraphIntoRuns(text, textStyle.font);
	if (textRuns.size() > 1)
	{
		for (const auto& run : textRuns)
		{
			auto runText = text.substr(run.startOffset, run.endOffset - run.startOffset);
			elements.push_back(std::make_unique<ParagraphTextElement>(runText, textStyle, run.rightToLeft));
		}
	}
	else
	{
		elements.push_back(std::make_unique<ParagraphTextElement>(text, textStyle));
	}
}

void Paragraph::addWidget(const std::shared_ptr<WIDGET> &widget, int32_t aboveBase)
{
	layoutDirty = true;
	attach(widget);
	elements.push_back(std::make_unique<ParagraphWidgetElement>(widget, aboveBase));
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

	auto overrideContext = context
		.setAllowChildDisplayRecursiveIfSelfClipped(true);

	WIDGET::displayRecursive(overrideContext);
}

void Paragraph::display(int xOffset, int yOffset)
{
	if (!isMouseDown || onClickHandlers.empty()) { return; }

	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	iV_Box(x0, y0, x0 + width(), y0 + height(), WZCOL_RED);
}

void Paragraph::addOnClickHandler(const W_PARAGRAPH_ONCLICK_FUNC& onClickFunc)
{
	if (!onClickFunc)
	{
		return;
	}
	onClickHandlers.push_back(onClickFunc);
}

void Paragraph::clicked(W_CONTEXT *, WIDGET_KEY key)
{
	dirty = true;
	isMouseDown = true;
}

/* Respond to a mouse button up */
void Paragraph::released(W_CONTEXT *, WIDGET_KEY key)
{
	if (!isMouseDown)
	{
		return;
	}

	isMouseDown = false;

	/* Call all onClick event handlers */
	for (auto it = onClickHandlers.begin(); it != onClickHandlers.end(); it++)
	{
		auto onClickHandler = *it;
		if (onClickHandler)
		{
			onClickHandler(*this, key);
		}
	}
	dirty = true;
}

/* Respond to the mouse moving off the widget */
void Paragraph::highlightLost()
{
	isMouseDown = false;
	dirty = true;
}

nonstd::optional<std::vector<uint32_t>> Paragraph::getScrollSnapOffsets()
{
	return scrollSnapOffsets;
}

void Paragraph::forceSetAllFontColor(PIELIGHT colour)
{
	// set for any newly-added text
	setFontColour(colour);

	// set for all existing text
	for (auto &element: elements)
	{
		auto psTextElement = dynamic_cast<ParagraphTextElement*>(element.get());
		if (psTextElement)
		{
			psTextElement->setFontColour(colour);
		}
	}
}
