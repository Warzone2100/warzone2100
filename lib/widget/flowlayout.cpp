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

#include "flowlayout.h"

bool FlowLayout::shouldMergeFragment() const
{
    return !currentLine.empty() && !partialWord.empty() && currentLine.back().elementId == partialWord.front().elementId;
}

void FlowLayout::endWord()
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

void FlowLayout::pushFragment(FlowLayoutElementDescriptor const &elementDescriptor, size_t begin, size_t end)
{
    auto width = elementDescriptor.getWidth(begin, end - begin);
    partialWord.push_back({currentElementId, begin, end - begin, width, nextOffset + partialWordWidth});
    partialWordWidth += width;
}

void FlowLayout::placeLine(FlowLayoutElementDescriptor const &elementDescriptor, size_t begin, size_t end)
{
    auto current = begin;

    while (current < end)
    {
        long long fragmentFits;

        if (nextOffset + partialWordWidth + elementDescriptor.getWidth(current, end - current) > maxWidth)
        // fragment doesn't fit completely in the current line
        {
            fragmentFits = current - 1;
            size_t fragmentDoesntFit = end;
            while (fragmentDoesntFit - fragmentFits > 1)
            {
                auto middle = (fragmentFits + fragmentDoesntFit) / 2;
                if (nextOffset + partialWordWidth + elementDescriptor.getWidth(current, middle - current) > maxWidth)
                {
                    fragmentDoesntFit = middle;
                } else {
                    fragmentFits = middle;
                }
            }
        }
        else
        {
            fragmentFits = end;
        }

        auto whitespacePosition = fragmentFits + 1;
        while ((whitespacePosition >= elementDescriptor.size() || !elementDescriptor.isWhitespace(whitespacePosition - 1)) && whitespacePosition > current)
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

void FlowLayout::append(FlowLayoutElementDescriptor const &elementDescriptor)
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

void FlowLayout::end()
{
    endWord();

    if (!currentLine.empty()) {
        breakLine();
    }
}

void FlowLayout::breakLine()
{
    endWord();
    lines.push_back(currentLine);
    currentLine.clear();
    nextOffset = 0;
}
