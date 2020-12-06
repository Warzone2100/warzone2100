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

/**
 * FlowLayout implements a word wrapping algorithm.
 *
 * It doesn't specify how the text should be structured, it only requires that the elements of the text are described
 * by FlowLayoutElementDescriptor. This also means that the element doesn't need to be only text.
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

#ifndef __INCLUDED_LIB_WIDGET_FLOWLAYOUT_H__
#define __INCLUDED_LIB_WIDGET_FLOWLAYOUT_H__

#include <stddef.h>
#include <vector>

class FlowLayoutElementDescriptor
{
public:
    virtual ~FlowLayoutElementDescriptor() = default;
    virtual unsigned int getWidth(size_t position, size_t length) const = 0;
    virtual size_t size() const = 0;
    virtual bool isWhitespace(size_t position) const = 0;
    virtual bool isLineBreak(size_t position) const = 0;

    virtual bool isWord(size_t position) const
    {
        return position < size() && !isWhitespace(position) && !isLineBreak(position);
    }
};

struct FlowLayoutFragment
{
    size_t elementId;
    size_t begin;
    size_t length;
    unsigned int width;
    unsigned int offset;
};

struct FlowLayout
{
private:
    bool shouldMergeFragment() const;

    void endWord();

    void pushFragment(FlowLayoutElementDescriptor const &elementDescriptor, size_t begin, size_t end);

    void placeLine(FlowLayoutElementDescriptor const &elementDescriptor, size_t begin, size_t end);

public:
    FlowLayout(unsigned int maxWidth): maxWidth(maxWidth) {}

    void append(FlowLayoutElementDescriptor const &elementDescriptor);

    void end();

    void breakLine();

    std::vector<std::vector<FlowLayoutFragment>> getLines() const
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

#endif // __INCLUDED_LIB_WIDGET_FLOWLAYOUT_H__
