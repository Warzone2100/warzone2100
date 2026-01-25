/*
 *	This file is part of Warzone 2100.
 *	Copyright (C) 2025  Warzone 2100 Project
 *
 *	Warzone 2100 is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Warzone 2100 is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with Warzone 2100; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "stringsearch.h"
#include <algorithm>

constexpr int ALPHABET_SIZE = 256;

PreprocessedASCIISearchString::PreprocessedASCIISearchString(const std::string& pattern_)
: pattern(pattern_)
{
	size_t length = pattern.size();

	// Initialize bad char table
	badCharTable.resize(ALPHABET_SIZE, -1);
	for (size_t i = 0; i < length; ++i)
	{
		badCharTable[static_cast<unsigned char>(pattern[i])] = static_cast<int>(i);
	}

	// Initialize good suffix tables
	borderPos.resize(length + 1);
	shift.resize(length + 1);

	// Good Suffix rule
	//
	// Case 1: Finding borders for suffixes
	int i = static_cast<int>(length), j = static_cast<int>(length + 1);
	borderPos[i] = j; // Base case for empty suffix
	while (i > 0)
	{
		while (j <= length && pattern[i - 1] != pattern[j - 1])
		{
			if (shift[j] == 0) // If this shift hasn't been set yet for this border
			{
				shift[j] = j - i;
			}
			j = borderPos[j];
		}
		i--; j--;
		borderPos[i] = j;
	}

	// Case 2: Matching a prefix of the pattern to a suffix of the text
	//
	// Set remaining shift values using the widest border of the entire pattern
	//
	// The value borderPos[0] stores the length of the longest border (prefix that is also a suffix) of the whole pattern
	if (shift[j] == 0)
	{
		shift[j] = j; // Shift by length of the border itself
	}

	// Fill remaining entries using the widest border (prefix that is also a suffix)
	//
	// If a suffix has no internal matching occurrence, shift by the length of the matching prefix
	// that is also a suffix of the entire pattern.
	for (i = 0; i <= length; ++i)
	{
		if (shift[i] == 0)
		{
			shift[i] = borderPos[0]; // Shift by the length of the longest border
		}
	}
}

/**
 * @brief Searches for the preprocessed pattern in the given text using Boyer-Moore type rules
 *        (bad character and strong good suffix rules).
 * @param text The text to search within (const char* and length).
 * @param textLength The length of the text.
 * @param preprocessedData The preprocessed data for the pattern.
 * @return True if the pattern is found, false otherwise.
 */
bool containsSubstringPreprocessed(const char* text, size_t textLength, const PreprocessedASCIISearchString& preprocessedData)
{
	size_t patternLength = preprocessedData.pattern.size();
	const char* pattern = preprocessedData.pattern.c_str();
	const std::vector<int>& badCharTable = preprocessedData.badCharTable;
	const std::vector<int>& goodSuffixShift = preprocessedData.shift;

	if (patternLength == 0)
	{
		return true; // Empty pattern found
	}
	if (textLength == 0 || patternLength > textLength)
	{
		return false; // Empty text or pattern longer than text
	}

	size_t s = 0; // Current shift of the pattern with respect to text
	while (s <= (textLength - patternLength))
	{
		int j = static_cast<int>(patternLength) - 1; // Index for pattern (start from right)

		// Keep reducing index j of pattern while characters of pattern and text match
		while (j >= 0 && pattern[j] == text[s + j])
		{
			j--;
		}

		// If the pattern is found (j becomes -1), return true
		if (j < 0)
		{
			return true; // Pattern found
		}
		else
		{
			// Mismatch occurred at pattern[j] vs text[s + j]
			// Calculate shift based on bad character rule
			int badCharShift = j - badCharTable[static_cast<unsigned char>(text[s + j])];

			// Calculate shift based on good suffix rule
			// j is the index of the mismatching character in the pattern.
			int goodSuffixRuleShift = goodSuffixShift[j + 1];

			s += std::max(badCharShift, goodSuffixRuleShift);
		}
	}
	return false; // Pattern not found
}

/**
 * @brief Searches for the preprocessed pattern in the given text using Boyer-Moore type rules (bad character and strong good suffix rules).
 * @param text The text to search within
 * @param preprocessedData The preprocessed data for the pattern.
 * @return True if the pattern is found, false otherwise.
 */
bool containsSubstringPreprocessed(const std::string& text, const PreprocessedASCIISearchString& preprocessedData)
{
	return containsSubstringPreprocessed(text.c_str(), text.size(), preprocessedData);
}
