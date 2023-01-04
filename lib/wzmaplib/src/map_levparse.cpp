/*
	This file is part of Warzone 2100.
	Copyright (C) 2022  Warzone 2100 Project

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

#include "map_levparse.h"
#include "map_internal.h"

#include <vector>
#include <cstdio>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;


// MARK: - Parsing .lev files

namespace WzMap {

#if !defined(__clang__) && defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

bool levParseBasic(const char* data, size_t dataLen, IOProvider& mapIO, std::function<void (const std::string& command, const std::string& arg)> handleCommand, std::function<void (const std::string& commentLine)> handleCommentLine /*= nullptr*/, LoggingProtocol* pCustomLogger /*= nullptr*/)
{
	if (dataLen == 0)
	{
		return false;
	}

	enum class ParsingMode {
		Normal,
		QuotedText,
		SingleLineComment,
		Comment
	};
	ParsingMode mode = ParsingMode::Normal;
	std::vector<std::string> currentCommandToken(3);
	size_t currentCommandTokenIdx = 0;
	std::string currentCommentLine;
	bool commentStartedAtBeginningOfLine = false;
	size_t line = 0;
	size_t lastModeChangeStartPos = 0;
	size_t startPosOfLine = 0;
	size_t idx = 0;
	auto lookAhead = [&idx, &data, dataLen](size_t byChars) -> optional<char>{
		if (idx + byChars >= dataLen)
		{
			return nullopt;
		}
		return data[idx + byChars];
	};
	auto addCharToCommandPart = [&](char c) {
		if (currentCommandTokenIdx < currentCommandToken.size())
		{
			currentCommandToken[currentCommandTokenIdx] += c;
		}
	};
	auto handleLine = [&]() {
		if (mode != ParsingMode::Comment)
		{
			mode = ParsingMode::Normal;
			commentStartedAtBeginningOfLine = false;
			lastModeChangeStartPos = idx + 1;
		}
		else
		{
			commentStartedAtBeginningOfLine = true;
		}
		if (!currentCommandToken[0].empty())
		{
			if (!currentCommandToken[2].empty())
			{
				// LEV FORMAT ERROR: More than two tokens on a line
				// (Expected: <command> <argument>)
				debug(pCustomLogger, LOG_ERROR, "LEV File Parse Error: `Syntax Error` at line %zu: at least one unexpected token: %s", line, currentCommandToken[2].c_str());
			}
			handleCommand(currentCommandToken[0], currentCommandToken[1]);
			for (auto& token : currentCommandToken)
			{
				token.clear();
			}
			currentCommandTokenIdx = 0;
		}
		if (!currentCommentLine.empty())
		{
			handleCommentLine(currentCommentLine);
			currentCommentLine.clear();
		}
	};
	for (; idx < dataLen; idx++)
	{
		char c = data[idx];
		auto nextChar = lookAhead(1);
		if (c == '\n' || c == '\r' || c == '\0')
		{
			// handle newline (or NULL)
			handleLine();
			line++;
			startPosOfLine = idx + 1;
		}
		else if (mode == ParsingMode::QuotedText)
		{
			if (c == '\"')
			{
				mode = ParsingMode::Normal;
				lastModeChangeStartPos = idx + 1;
			}
			else
			{
				addCharToCommandPart(c);
			}
		}
		else if (c == '/' && (nextChar.value_or(0) == '/' || nextChar.value_or(0) == '*') && (mode != ParsingMode::Comment && mode != ParsingMode::SingleLineComment))
		{
			if (nextChar.value_or(0) == '/')
			{
				mode = ParsingMode::SingleLineComment;
			}
			else
			{
				mode = ParsingMode::Comment;
			}
			commentStartedAtBeginningOfLine = (idx == startPosOfLine);
			idx++;
			lastModeChangeStartPos = idx + 1;
		}
		else if (mode == ParsingMode::Comment && c == '*' && nextChar.value_or(0) == '/')
		{
			mode = ParsingMode::Normal;
			if (!currentCommentLine.empty())
			{
				handleCommentLine(currentCommentLine);
				currentCommentLine.clear();
			}
			idx++;
			lastModeChangeStartPos = idx + 1;
		}
		else if (mode == ParsingMode::Normal && c == '\"')
		{
			mode = ParsingMode::QuotedText;
			lastModeChangeStartPos = idx + 1;
		}
		else
		{
			if (mode == ParsingMode::Normal && (c == ' ' || c == '\t' || c == '\f'))
			{
				// whitespace is treated as a separator for command and arg - if a command has already been read
				if (currentCommandTokenIdx < currentCommandToken.size() && !currentCommandToken[currentCommandTokenIdx].empty())
				{
					currentCommandTokenIdx++;
				}
				// otherwise, just ignore it - must be whitespace prior to a command
			}
			else
			{
				// append the character to the appropriate type of token (if needed)
				switch (mode)
				{
					case ParsingMode::Normal:
					case ParsingMode::QuotedText:
						addCharToCommandPart(c);
						break;
					case ParsingMode::SingleLineComment:
					case ParsingMode::Comment:
						if (commentStartedAtBeginningOfLine)
						{
							currentCommentLine += c;
						}
						break;
				}
			}
		}
	}

	// handle last line (if present)
	handleLine();

	return true;
}

#if !defined(__clang__) && defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
#pragma GCC diagnostic pop
#endif

optional<MAP_TILESET> convertLevMapTilesetType(std::string dataset)
{
	unsigned t = 0, c = 0;

	if (sscanf(dataset.c_str(), "MULTI_CAM_%u", &c) != 1)
	{
		sscanf(dataset.c_str(), "MULTI_T%u_C%u", &t, &c);
	}

	switch (c)
	{
	case 1:
		return MAP_TILESET::ARIZONA;
		break;
	case 2:
		return MAP_TILESET::URBAN;
		break;
	case 3:
		return MAP_TILESET::ROCKIES;
		break;
	}

	return nullopt;
}

optional<std::string> mapTilesetToClassicLevType(MapType mapType, MAP_TILESET mapTileset, unsigned techLevel, LoggingProtocol* pCustomLogger)
{
	if (mapType != MapType::SKIRMISH)
	{
//		debug(pCustomLogger, LOG_ERROR, "MapType is not yet supported in this function: %s", to_string(mapType).c_str());
		return nullopt;
	}

	std::string campaignNum = "0";
	switch (mapTileset)
	{
		case MAP_TILESET::ARIZONA:
			campaignNum = "1";
			break;
		case MAP_TILESET::URBAN:
			campaignNum = "2";
			break;
		case MAP_TILESET::ROCKIES:
			campaignNum = "3";
			break;
	}

	if (techLevel == 1)
	{
		return std::string("MULTI_CAM_") + campaignNum;
	}
	else
	{
		return std::string("MULTI_T") + std::to_string(techLevel) + "_C" + campaignNum;
	}
}

optional<int> mapTypeToClassicLevType(MapType mapType, unsigned techLevel, LoggingProtocol* pCustomLogger)
{
	switch (mapType)
	{
		case MapType::SKIRMISH:
			switch (techLevel)
			{
				case 1:
					return static_cast<int>(SUPPORTED_LEVEL_TYPES::SKIRMISH);
				case 2:
					return static_cast<int>(SUPPORTED_LEVEL_TYPES::MULTI_SKIRMISH2);
				case 3:
					return static_cast<int>(SUPPORTED_LEVEL_TYPES::MULTI_SKIRMISH3);
				case 4:
					return static_cast<int>(SUPPORTED_LEVEL_TYPES::MULTI_SKIRMISH4);
				default:
					break;
			}
		default:
			// not yet supported
			break;
	}

//	debug(pCustomLogger, LOG_ERROR, "MapType (%s) / techLevel (%u) combination is not yet supported in this function", to_string(mapType).c_str(), techLevel);
	return nullopt;
}

} // namespace WzMap
