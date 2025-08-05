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

#pragma once

#include <vector>
#include <string>

/**
 * @brief Stores preprocessed data for quick searching (bad character and good suffix / shifting). Should be created once for each pattern for best efficiency.
 *        Espects a pattern of ASCII characters. (*NOT* UTF-8)
 */
class PreprocessedASCIISearchString {
public:
	PreprocessedASCIISearchString(const std::string& pattern);
public:
	std::vector<int> badCharTable;
	std::vector<int> borderPos; // bpos: stores the starting index of the border for the suffix starting at position i in the pattern
	std::vector<int> shift;     // Stores the good suffix shift for each suffix length (or mismatch position)
	std::string pattern;
};


/**
 * @brief Searches for the preprocessed pattern in the given text using the Boyer-Moore algorithm (bad character and strong good suffix rules).
 * @param text The text to search within (const char* and length).
 * @param textLength The length of the text.
 * @param preprocessedData The preprocessed data for the pattern.
 * @return True if the pattern is found, false otherwise.
 */
bool containsSubstringPreprocessed(const char* text, size_t textLength, const PreprocessedASCIISearchString& preprocessedData);


/**
 * @brief Searches for the preprocessed pattern in the given text using the Boyer-Moore algorithm (bad character and strong good suffix rules).
 * @param text The text to search within
 * @param preprocessedData The preprocessed data for the pattern.
 * @return True if the pattern is found, false otherwise.
 */
bool containsSubstringPreprocessed(const std::string& text, const PreprocessedASCIISearchString& preprocessedData);
